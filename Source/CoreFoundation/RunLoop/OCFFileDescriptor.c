//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/RunLoop/OCFFileDescriptor.h"

#include "CoreFoundation/String/OCFString.h"

#include <Block.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <unistd.h>

// Inferred organization: the native CFFileDescriptor functions lie in one cluster of the iOS Simulator __text
// section, __CFFileDescriptorDeallocate (0x1c019a594) through _OUTLINED_FUNCTION_0_9 (0x1c019c200), with
// their .cold entries at 0x1c0308290-0x1c03082b0 and __CFFileDescriptorClass (0x22279dbd0,
// __DATA_CONST,__const); the family is kept here under RunLoop/ in native address order (the native source
// path is not in the evidence). _OUTLINED_FUNCTION_0_9 (dispatch_once(&CFFileDescriptorGetTypeID.initOnce,
// ___block_literal_global_3)) and the .cold.1 entries that branch to it fold into their callers as the inline
// check of dispatch_once. __CFFileDescriptorDeallocate (0x1c019a594) with its block (0x1c019bfac), and
// CFFileDescriptorGetTypeID (0x1c019a848) with its .cold.1 (0x1c0308290) and once block (0x1c019a874), are
// restored from the IDA capture Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. Not restored (not
// in the evidence): CFFileDescriptorGetNativeDescriptor (0x1c019af48), CFFileDescriptorGetContext
// (0x1c019afc8) and CFFileDescriptorDisableCallBacks (0x1c019b5d8, with its block 0x1c019b6c8). Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full; pseudocode used as a guide only); iOS Device 24A446: unread. Reading: not
// yet written; evidence Local/Evidence/cf-core/iphone-simulator-corpus-delta5.json and
// iphone-simulator-corpus-delta6.json.
//
// The dispatch source types are the GOT slots 0x2224171d0 (the read sources) and 0x222417200 (the write
// sources), taken to be DISPATCH_SOURCE_TYPE_READ and DISPATCH_SOURCE_TYPE_WRITE by use (confidence: medium).

enum {
    kFileDescriptorTypeID = 60,
    kInfoCloseOnInvalidate = 0x2,
};

// The native unit calls pselect through its unlimited-descriptor variant (pselect$DARWIN_EXTSN), which the
// SDK selects only under _DARWIN_C_SOURCE or _DARWIN_UNLIMITED_SELECT; it is bound here by symbol name.
extern int pselectUnlimited(
    int nfds,
    fd_set *readfds,
    fd_set *writefds,
    fd_set *errorfds,
    const struct timespec *timeout,
    const sigset_t *sigmask
) __asm("_pselect$DARWIN_EXTSN");

// CFFileDescriptorGetTypeID.initOnce (0x22e6d2830, __DATA,__bss, non-external) and ___block_literal_global_3,
// the global block whose invoke is ___CFFileDescriptorGetTypeID_block_invoke (0x1c019a874): the once that
// every entry point makes sure of before its type check. The predicate and block names are local descriptive
// spellings; both are defined with CFFileDescriptorGetTypeID below.
static dispatch_once_t fileDescriptorTypeIDInitOnce;
static void (^const fileDescriptorTypeIDInitBlock)(void);

static void __CFFileDescriptorDeallocate(OCFTypeRef cf);
static OCFStringRef __CFFileDescriptorCopyDescription(OCFTypeRef cf);
static dispatch_queue_t __fdQueue(void);
static Boolean cffd_is_readable(int fd);
static Boolean cffd_is_writeable(int fd);
static void __CFFileDescriptorSchedule(void *info, OCFRunLoopRef rl, OCFRunLoopMode mode);
static void __CFFileDescriptorCancel(void *info, OCFRunLoopRef rl, OCFRunLoopMode mode);
static void __CFFileDescriptorPerform(void *info);

// Native __CFFileDescriptorClass, 0x22279dbd0 (__DATA_CONST,__const). Hidden (was a private external);
// version 0, name "CFFileDescriptor", finalize and the debug description (the record's words, read with
// macho.py).
const OCFRuntimeClass __CFFileDescriptorClass = {
    0,
    "CFFileDescriptor",
    NULL,
    NULL,
    __CFFileDescriptorDeallocate,
    NULL,
    NULL,
    NULL,
    __CFFileDescriptorCopyDescription,
    NULL,
    NULL,
    0,
};

// The helpers below reproduce code the native functions inline at each use (no native symbols): the fork
// check (__CF_FORK_STATE read as a volatile byte; the entry points that stop in a forked child read it a
// third time), the type ID once and check, and the info-word flag set with a compare-and-swap loop.
static inline void checkForFork(void) {
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
}

static inline bool processHasForked(void) {
    return (*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0;
}

static inline void assertFileDescriptor(OCFFileDescriptorRef f) {
    dispatch_once(&fileDescriptorTypeIDInitOnce, fileDescriptorTypeIDInitBlock);
    OCFTypeID typeID = _CFGetNonObjCTypeID(f);
    if (typeID != kFileDescriptorTypeID) {
        _CFAssertMismatchedTypeID(kFileDescriptorTypeID, typeID);
    }
}

static inline void setInfoBits(OCFTypeRef cf, uint64_t bits) {
    uint64_t *info = &((OCFRuntimeBase *)cf)->info;
    uint64_t expected = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    while (!__atomic_compare_exchange_n(
        info, &expected, expected | bits, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
    }
}

// Native __CFFileDescriptorDeallocate, 0x1c019a594, with _____CFFileDescriptorDeallocate_block_invoke
// (0x1c019bfac). Non-external; the finalize callback of __CFFileDescriptorClass. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. The fork check stops a forked child. As
// CFFileDescriptorInvalidate without its type check and retain: synchronously on the descriptor queue (a
// block with the two __block variables, the valid flag captured first), a valid object (state 0) becomes
// invalidating (state 1, then a full barrier); each source is cancelled, resumed when marked suspended (the
// mark cleared) and cleared; the run loop source is taken; with info bit 1 (an acquire load) the descriptor
// is shut down (both directions), /dev/null (read-only) is duplicated over it and both are closed; the
// descriptor becomes -1. After the block, a valid object's run loop source (when taken) is invalidated and
// released, the context info is cleared and given to the context release (when set) and the state becomes 2
// (then a full barrier). The state then becomes 3 in every case.
static void __CFFileDescriptorDeallocate(OCFTypeRef cf) {
    OCFFileDescriptorRef f = (OCFFileDescriptorRef)cf;
    checkForFork();
    if (processHasForked()) {
        return;
    }
    __block OCFRunLoopSourceRef source = NULL;
    __block Boolean wasValid = false;
    dispatch_sync(__fdQueue(), ^{
      wasValid = f->state == 0;
      if (!wasValid) {
          return;
      }
      f->state = 1;
      __atomic_thread_fence(__ATOMIC_SEQ_CST);
      if (f->readSource != NULL) {
          dispatch_source_cancel(f->readSource);
          if (f->readSuspended) {
              f->readSuspended = 0;
              dispatch_resume(f->readSource);
          }
          f->readSource = NULL;
      }
      if (f->writeSource != NULL) {
          dispatch_source_cancel(f->writeSource);
          if (f->writeSuspended) {
              f->writeSuspended = 0;
              dispatch_resume(f->writeSource);
          }
          f->writeSource = NULL;
      }
      source = f->rls;
      f->rls = NULL;
      if ((__atomic_load_n(&f->base.info, __ATOMIC_ACQUIRE) & kInfoCloseOnInvalidate) != 0) {
          shutdown(f->fd, SHUT_RDWR);
          int nullFd = open("/dev/null", O_RDONLY);
          dup2(nullFd, f->fd);
          close(nullFd);
          close(f->fd);
      }
      f->fd = -1;
    });
    if (wasValid) {
        if (source != NULL) {
            OCFRunLoopSourceInvalidate(source);
            OCFRelease(source);
        }
        void *info = f->context.info;
        f->context.info = NULL;
        if (f->context.release != NULL) {
            f->context.release(info);
        }
        f->state = 2;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }
    f->state = 3;
}

// Native __CFFileDescriptorCopyDescription, 0x1c019a708. Non-external. The context description is the
// client's copy-description result when the context has info and the callback; otherwise, or when that gives
// NULL, "<CFFileDescriptor context %p>" with the info (read again after the callback) in
// kCFAllocatorSystemDefault. The callout's name is its dladdr symbol when dladdr succeeds with a symbol whose
// address is the callout's, else "???". The result is "<CFFileDescriptor %p [%p]>{valid = %s, fd = %x, source
// = %p, callout = %s (%p), context = %@}" with the object, its allocator, "Yes" for state 0 (else "No"), the
// descriptor, the run loop source, the name, the callout and the context description (released afterwards),
// in kCFAllocatorSystemDefault.
static OCFStringRef __CFFileDescriptorCopyDescription(OCFTypeRef cf) {
    OCFFileDescriptorRef f = (OCFFileDescriptorRef)cf;
    void *info = f->context.info;
    OCFStringRef contextDesc = NULL;
    if (info != NULL && f->context.copyDescription != NULL) {
        contextDesc = f->context.copyDescription(info);
        if (contextDesc == NULL) {
            info = f->context.info;
        }
    }
    if (contextDesc == NULL) {
        contextDesc = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("<CFFileDescriptor context %p>"), info
        );
    }
    Dl_info dlInfo;
    memset(&dlInfo, 0, sizeof(dlInfo));
    void *addr = (void *)f->callout;
    const char *name = "???";
    if (dladdr(addr, &dlInfo) != 0 && dlInfo.dli_sname != NULL && dlInfo.dli_saddr == addr) {
        name = dlInfo.dli_sname;
    }
    OCFStringRef result = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR(
            "<CFFileDescriptor %p [%p]>{valid = %s, fd = %x, source = %p, callout = %s (%p), context = %@}"
        ),
        cf,
        OCFGetAllocator(cf),
        f->state == 0 ? "Yes" : "No",
        f->fd,
        f->rls,
        name,
        addr,
        contextDesc
    );
    if (contextDesc != NULL) {
        OCFRelease(contextDesc);
    }
    return result;
}

// Native ___CFFileDescriptorGetTypeID_block_invoke, 0x1c019a874, the invoke of ___block_literal_global_3.
// Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. Raises the soft descriptor limit:
// with RLIMIT_NOFILE (getrlimit, the limit zeroed first) and kern.maxfilesperproc (sysctl {1, 29}, a 32-bit
// value that starts at 0, size 4) both read, a hard limit below the sysctl value (sign-extended) lowers that
// value to the hard limit's low 32 bits; with getrlimit failed nothing is changed; with only sysctl failed
// its value (0) is used. When the soft limit is below that value (sign-extended), the soft limit becomes the
// smaller of soft limit + 2304 and that value (unsigned comparisons) and setrlimit applies it with the hard
// limit unchanged.
static void (^const fileDescriptorTypeIDInitBlock)(void) = ^{
  struct rlimit limits = {0, 0};
  int rlimitResult = getrlimit(RLIMIT_NOFILE, &limits);
  int mib[2] = {CTL_KERN, KERN_MAXFILESPERPROC};
  int maxFiles = 0;
  size_t size = sizeof(maxFiles);
  int sysctlResult = sysctl(mib, 2, &maxFiles, &size, NULL, 0);
  if ((sysctlResult | rlimitResult) == 0) {
      if (limits.rlim_max < (rlim_t)(int64_t)maxFiles) {
          maxFiles = (int)limits.rlim_max;
      }
  } else if (rlimitResult != 0) {
      return;
  }
  rlim_t maximum = (rlim_t)(int64_t)maxFiles;
  if (limits.rlim_cur < maximum) {
      struct rlimit raised = limits;
      rlim_t current = limits.rlim_cur + 0x900;
      raised.rlim_cur = current < maximum ? current : maximum;
      setrlimit(RLIMIT_NOFILE, &raised);
  }
};

// Native CFFileDescriptorGetTypeID, 0x1c019a848, with CFFileDescriptorGetTypeID.cold.1 (0x1c0308290, a branch
// to _OUTLINED_FUNCTION_0_9, the dispatch_once call). External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. The once (the inline predicate check of
// dispatch_once), then the constant 60.
OCFTypeID OCFFileDescriptorGetTypeID(void) {
    dispatch_once(&fileDescriptorTypeIDInitOnce, fileDescriptorTypeIDInitBlock);
    return kFileDescriptorTypeID;
}

// Native __fdQueue, 0x1c019ab88, with its static ___fdQueue.__fddq (0x22e6d2838, __DATA,__bss). Non-external.
// The serial queue "com.apple.CFFileDescriptor", created on first use and published with a compare-and-swap
// from NULL (a losing queue is released); the static is read again for the result.
static dispatch_queue_t __fdQueue(void) {
    static dispatch_queue_t __fddq;
    if (__fddq == NULL) {
        dispatch_queue_t queue = dispatch_queue_create("com.apple.CFFileDescriptor", NULL);
        dispatch_queue_t expected = NULL;
        if (!__atomic_compare_exchange_n(
                &__fddq, &expected, queue, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
            )) {
            dispatch_release(queue);
        }
    }
    return __fddq;
}

// Native CFFileDescriptorCreate, 0x1c019a96c, with ___CFFileDescriptorCreate_block_invoke (0x1c019abdc, with
// its .cold.1 0x1c0308294), _block_invoke_2 (0x1c019ae38, the read event handler), _block_invoke_3
// (0x1c019ae98, the read cancel handler), _block_invoke_4 (0x1c019aec0, the write event handler) and
// _block_invoke_5 (0x1c019af20, the write cancel handler). External. The fork check stops a forked child
// (NULL). The descriptor is checked with fstat (zeroed stat; failure gives NULL). It is readable when its
// mode has neither 0400 nor 0200, or has 0400; a readable descriptor gets a read duplicate (fcntl
// F_DUPFD_CLOEXEC, minimum 3), then a mode with 0200 a write duplicate; a failed needed duplicate closes the
// duplicates made and gives NULL. On the descriptor queue (synchronously): after the type ID once, a type 60
// instance (0x60 extra bytes) is made (NULL ends the block); closeOnInvalidate sets info bit 1; the
// descriptors are stored and the sources cleared. A read duplicate gets a read source on the descriptor queue
// whose event handler marks a read fired, suspends the source once (marking it suspended), and signals and
// wakes the run loop source when there is one, and whose cancel handler releases the source and closes the
// duplicate (a value captured from the block); the write duplicate likewise. Both sources start marked
// suspended (they are never resumed here); the callout and, with a context, its copy with the info retained
// (when the context has a retain callback) are stored; the state becomes 0. Without an object, both
// duplicates are closed (also when -1).
OCFFileDescriptorRef OCFFileDescriptorCreate(
    OCFAllocatorRef allocator,
    OCFFileDescriptorNativeDescriptor fd,
    Boolean closeOnInvalidate,
    OCFFileDescriptorCallBack callout,
    const OCFFileDescriptorContext *context
) {
    checkForFork();
    if (processHasForked()) {
        return NULL;
    }
    struct stat statBuffer;
    memset(&statBuffer, 0, sizeof(statBuffer));
    if (fstat(fd, &statBuffer) < 0) {
        return NULL;
    }
    mode_t mode = statBuffer.st_mode;
    bool readable = (mode & 0x180) == 0 || (mode & 0x100) != 0;
    int readFd = -1;
    int writeFd = -1;
    if (readable) {
        readFd = fcntl(fd, F_DUPFD_CLOEXEC, 3);
    }
    if ((mode & 0x80) != 0) {
        writeFd = fcntl(fd, F_DUPFD_CLOEXEC, 3);
    }
    if ((readFd < 0 && readable) || (writeFd < 0 && (mode & 0x80) != 0)) {
        if (readFd >= 0) {
            close(readFd);
        }
        if (writeFd >= 0) {
            close(writeFd);
        }
        return NULL;
    }
    __block OCFFileDescriptorRef memory = NULL;
    dispatch_sync(__fdQueue(), ^{
      dispatch_once(&fileDescriptorTypeIDInitOnce, fileDescriptorTypeIDInitBlock);
      OCFFileDescriptorRef f =
          (OCFFileDescriptorRef)_OCFRuntimeCreateInstance(allocator, kFileDescriptorTypeID, 0x60, NULL);
      if (f == NULL) {
          return;
      }
      if (closeOnInvalidate) {
          setInfoBits(f, kInfoCloseOnInvalidate);
      }
      f->fd = fd;
      f->readFd = readFd;
      f->writeFd = writeFd;
      f->readSource = NULL;
      f->writeSource = NULL;
      if (readFd >= 0) {
          dispatch_source_t source =
              dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, readFd, 0, __fdQueue());
          dispatch_source_set_event_handler(source, ^{
            f->readFired = 1;
            if (!f->readSuspended) {
                dispatch_suspend(source);
                f->readSuspended = 1;
            }
            if (f->rls != NULL) {
                OCFRunLoopSourceSignal(f->rls);
                _CFRunLoopSourceWakeUpRunLoops(f->rls);
            }
          });
          int sourceFd = readFd;
          dispatch_source_set_cancel_handler(source, ^{
            dispatch_release(source);
            close(sourceFd);
          });
          f->readSource = source;
      }
      if (writeFd >= 0) {
          dispatch_source_t source =
              dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, writeFd, 0, __fdQueue());
          dispatch_source_set_event_handler(source, ^{
            f->writeFired = 1;
            if (!f->writeSuspended) {
                dispatch_suspend(source);
                f->writeSuspended = 1;
            }
            if (f->rls != NULL) {
                OCFRunLoopSourceSignal(f->rls);
                _CFRunLoopSourceWakeUpRunLoops(f->rls);
            }
          });
          int sourceFd = writeFd;
          dispatch_source_set_cancel_handler(source, ^{
            dispatch_release(source);
            close(sourceFd);
          });
          f->writeSource = source;
      }
      f->readSuspended = 1;
      f->writeSuspended = 1;
      f->callout = callout;
      if (context != NULL) {
          f->context = *context;
          void *info = context->info;
          if (context->retain != NULL) {
              info = context->retain(info);
          }
          f->context.info = info;
      }
      f->state = 0;
      memory = f;
    });
    OCFFileDescriptorRef result = memory;
    if (result == NULL) {
        close(readFd);
        close(writeFd);
        result = memory;
    }
    return result;
}

// Native _CFFileDescriptorInduceFakeReadCallBack, 0x1c019b030, with
// ____CFFileDescriptorInduceFakeReadCallBack_block_invoke (0x1c019b0bc). Hidden (was a private external).
// Asynchronously on the descriptor queue: an object with a read source behaves as if the read source fired
// (marks a read fired, suspends the source once, signals and wakes the run loop source when there is one). No
// type or fork check.
void _CFFileDescriptorInduceFakeReadCallBack(OCFFileDescriptorRef f) {
    dispatch_async(__fdQueue(), ^{
      dispatch_source_t source = f->readSource;
      if (source == NULL) {
          return;
      }
      f->readFired = 1;
      if (!f->readSuspended) {
          dispatch_suspend(source);
          f->readSuspended = 1;
      }
      if (f->rls != NULL) {
          OCFRunLoopSourceSignal(f->rls);
          _CFRunLoopSourceWakeUpRunLoops(f->rls);
      }
    });
}

// Native CFFileDescriptorEnableCallBacks, 0x1c019b120, with its .cold.1 (0x1c03082a0) and
// ___CFFileDescriptorEnableCallBacks_block_invoke (0x1c019b210). External. The type ID once and check precede
// the fork check (a forked child stops). Synchronously on the descriptor queue, a valid object (state 0)
// handles read then write: when the type is asked (bits 0 and 1 of the low byte) and its source exists, a
// descriptor that is ready now (cffd_is_readable or cffd_is_writeable on the descriptor) behaves as if the
// source fired (the source suspended once, the run loop source signalled and woken when there is one);
// otherwise a suspended source is marked running and resumed.
void OCFFileDescriptorEnableCallBacks(OCFFileDescriptorRef f, OCFOptionFlags callBackTypes) {
    assertFileDescriptor(f);
    checkForFork();
    if (processHasForked()) {
        return;
    }
    dispatch_sync(__fdQueue(), ^{
      if (f->state != 0) {
          return;
      }
      if ((callBackTypes & kOCFFileDescriptorReadCallBack) != 0 && f->readSource != NULL) {
          if (cffd_is_readable(f->fd)) {
              f->readFired = 1;
              if (!f->readSuspended) {
                  dispatch_suspend(f->readSource);
                  f->readSuspended = 1;
              }
              if (f->rls != NULL) {
                  OCFRunLoopSourceSignal(f->rls);
                  _CFRunLoopSourceWakeUpRunLoops(f->rls);
              }
          } else if (f->readSuspended) {
              f->readSuspended = 0;
              dispatch_resume(f->readSource);
          }
      }
      if ((callBackTypes & kOCFFileDescriptorWriteCallBack) != 0 && f->writeSource != NULL) {
          if (cffd_is_writeable(f->fd)) {
              f->writeFired = 1;
              if (!f->writeSuspended) {
                  dispatch_suspend(f->writeSource);
                  f->writeSuspended = 1;
              }
              if (f->rls != NULL) {
                  OCFRunLoopSourceSignal(f->rls);
                  _CFRunLoopSourceWakeUpRunLoops(f->rls);
              }
          } else if (f->writeSuspended) {
              f->writeSuspended = 0;
              dispatch_resume(f->writeSource);
          }
      }
    });
}

// Native cffd_is_readable, 0x1c019b328. Non-external. A descriptor of 0x100000 or more (unsigned) traps (BRK
// #1). A heap set of ((fd + 8) >> 3) + 7 bytes (32-bit arithmetic, malloc_type_malloc type 0xA8AFE1CA) is
// zeroed and given the descriptor's bit (word fd >> 5, bit fd mod 32) when __darwin_check_fd_set_overflow
// allows it, then polled with pselect (the unlimited-descriptor variant) for reading with a fresh copy of the
// timeout {0 s, 1000 ns} and no signal mask; EINTR and EAGAIN start again from the zeroing. A failure or a
// zero count answers false; otherwise the answer is the bit (tested only when the overflow check allows it).
// The set is freed.
static Boolean cffd_is_readable(int fd) {
    if ((uint32_t)fd >= 0x100000) {
        __builtin_trap();
    }
    size_t size = (((uint32_t)fd + 8) >> 3) + 7;
    uint32_t *set = malloc_type_malloc(size, 0xA8AFE1CA);
    uint32_t bit = 1u << (fd & 31);
    uint32_t word = (uint32_t)fd >> 5;
    const struct timespec timeout = {0, 1000};
    Boolean result;
    for (;;) {
        bzero(set, size);
        if (__darwin_check_fd_set_overflow(fd, set, 1)) {
            set[word] |= bit;
        }
        struct timespec waitTime = timeout;
        int count = pselectUnlimited(fd + 1, (fd_set *)set, NULL, NULL, &waitTime, NULL);
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            result = false;
        } else if (count == 0) {
            result = false;
        } else {
            result = __darwin_check_fd_set_overflow(fd, set, 1) && (set[word] & bit) != 0;
        }
        break;
    }
    free(set);
    return result;
}

// Native cffd_is_writeable, 0x1c019b480. Non-external. As cffd_is_readable with the descriptor in the write
// set and malloc_type_malloc type 0x3B6252B5.
static Boolean cffd_is_writeable(int fd) {
    if ((uint32_t)fd >= 0x100000) {
        __builtin_trap();
    }
    size_t size = (((uint32_t)fd + 8) >> 3) + 7;
    uint32_t *set = malloc_type_malloc(size, 0x3B6252B5);
    uint32_t bit = 1u << (fd & 31);
    uint32_t word = (uint32_t)fd >> 5;
    const struct timespec timeout = {0, 1000};
    Boolean result;
    for (;;) {
        bzero(set, size);
        if (__darwin_check_fd_set_overflow(fd, set, 1)) {
            set[word] |= bit;
        }
        struct timespec waitTime = timeout;
        int count = pselectUnlimited(fd + 1, NULL, (fd_set *)set, NULL, &waitTime, NULL);
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            result = false;
        } else if (count == 0) {
            result = false;
        } else {
            result = __darwin_check_fd_set_overflow(fd, set, 1) && (set[word] & bit) != 0;
        }
        break;
    }
    free(set);
    return result;
}

// Native CFFileDescriptorInvalidate, 0x1c019b73c, with its .cold.1 (0x1c03082a8) and
// ___CFFileDescriptorInvalidate_block_invoke (0x1c019b8f8). External. The type ID once and check precede the
// fork check (a forked child stops). The object is retained for the call. Synchronously on the descriptor
// queue, a valid object (state 0) becomes invalidating (state 1, then a full barrier); each source is
// cancelled, resumed when marked suspended (the mark cleared) and cleared; the run loop source is taken; with
// info bit 1 the descriptor is shut down (both directions), /dev/null (read-only) is duplicated over it and
// both are closed; the descriptor becomes -1. After the block, a valid object's run loop source (when taken)
// is invalidated and released, the context info is cleared and given to the context release (when set) and
// the state becomes 2 (then a full barrier). The object is released.
void OCFFileDescriptorInvalidate(OCFFileDescriptorRef f) {
    assertFileDescriptor(f);
    checkForFork();
    if (processHasForked()) {
        return;
    }
    OCFRetain(f);
    __block OCFRunLoopSourceRef source = NULL;
    __block Boolean wasValid = false;
    dispatch_sync(__fdQueue(), ^{
      wasValid = f->state == 0;
      if (!wasValid) {
          return;
      }
      f->state = 1;
      __atomic_thread_fence(__ATOMIC_SEQ_CST);
      if (f->readSource != NULL) {
          dispatch_source_cancel(f->readSource);
          if (f->readSuspended) {
              f->readSuspended = 0;
              dispatch_resume(f->readSource);
          }
          f->readSource = NULL;
      }
      if (f->writeSource != NULL) {
          dispatch_source_cancel(f->writeSource);
          if (f->writeSuspended) {
              f->writeSuspended = 0;
              dispatch_resume(f->writeSource);
          }
          f->writeSource = NULL;
      }
      source = f->rls;
      f->rls = NULL;
      if ((__atomic_load_n(&f->base.info, __ATOMIC_ACQUIRE) & kInfoCloseOnInvalidate) != 0) {
          shutdown(f->fd, SHUT_RDWR);
          int nullFd = open("/dev/null", O_RDONLY);
          dup2(nullFd, f->fd);
          close(nullFd);
          close(f->fd);
      }
      f->fd = -1;
    });
    if (wasValid) {
        if (source != NULL) {
            OCFRunLoopSourceInvalidate(source);
            OCFRelease(source);
        }
        void *info = f->context.info;
        f->context.info = NULL;
        if (f->context.release != NULL) {
            f->context.release(info);
        }
        f->state = 2;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }
    OCFRelease(f);
}

// Native CFFileDescriptorIsValid, 0x1c019ba1c, with its .cold.1 (0x1c03082ac). External. After the type ID
// once and check (no fork check): an object that is not in state 0 is invalid; otherwise a descriptor that
// fails fstat (zeroed stat) invalidates the object and answers false.
Boolean OCFFileDescriptorIsValid(OCFFileDescriptorRef f) {
    assertFileDescriptor(f);
    if (f->state != 0) {
        return false;
    }
    struct stat statBuffer;
    memset(&statBuffer, 0, sizeof(statBuffer));
    if (fstat(f->fd, &statBuffer) < 0) {
        OCFFileDescriptorInvalidate(f);
        return false;
    }
    return true;
}

// Native CFFileDescriptorCreateRunLoopSource, 0x1c019bae8, with its .cold.1 (0x1c03082b0) and
// ___CFFileDescriptorCreateRunLoopSource_block_invoke (0x1c019bc2c). External. The type ID once and check
// precede the fork check (a forked child gets NULL); an invalid object (CFFileDescriptorIsValid) gets NULL.
// Synchronously on the descriptor queue, a valid object (state 0) keeps a valid run loop source; an invalid
// one is released and cleared, and then (or without one) a new source is created with the allocator, the
// order and a version 0 context whose info is the object (CFRetain, CFRelease, the description callback, no
// equal or hash, __CFFileDescriptorSchedule, __CFFileDescriptorCancel, __CFFileDescriptorPerform); a new
// source is signalled and its run loops woken once for a fired read and once for a fired write. The result is
// the object's source, retained (when set).
OCFRunLoopSourceRef
OCFFileDescriptorCreateRunLoopSource(OCFAllocatorRef allocator, OCFFileDescriptorRef f, OCFIndex order) {
    assertFileDescriptor(f);
    checkForFork();
    if (processHasForked()) {
        return NULL;
    }
    if (!OCFFileDescriptorIsValid(f)) {
        return NULL;
    }
    __block OCFRunLoopSourceRef result = NULL;
    dispatch_sync(__fdQueue(), ^{
      if (f->state != 0) {
          return;
      }
      OCFRunLoopSourceRef rls = f->rls;
      if (rls != NULL) {
          Boolean valid = OCFRunLoopSourceIsValid(rls);
          rls = f->rls;
          if (valid) {
              if (rls != NULL) {
                  result = (OCFRunLoopSourceRef)OCFRetain(rls);
                  return;
              }
          } else {
              OCFRelease(rls);
              f->rls = NULL;
          }
      }
      OCFRunLoopSourceContext context = {
          0,
          f,
          OCFRetain,
          OCFRelease,
          __CFFileDescriptorCopyDescription,
          NULL,
          NULL,
          __CFFileDescriptorSchedule,
          __CFFileDescriptorCancel,
          __CFFileDescriptorPerform,
      };
      f->rls = OCFRunLoopSourceCreate(allocator, order, &context);
      if (f->rls != NULL) {
          if (f->readFired) {
              OCFRunLoopSourceSignal(f->rls);
              _CFRunLoopSourceWakeUpRunLoops(f->rls);
          }
          if (f->writeFired) {
              OCFRunLoopSourceSignal(f->rls);
              _CFRunLoopSourceWakeUpRunLoops(f->rls);
          }
      }
      rls = f->rls;
      if (rls != NULL) {
          OCFRetain(rls);
      }
      result = rls;
    });
    return result;
}

// Native __CFFileDescriptorSchedule, 0x1c019bd68. Non-external; the run loop source's schedule callback.
// Wakes the run loop (a tail call).
static void __CFFileDescriptorSchedule(void *info, OCFRunLoopRef rl, OCFRunLoopMode mode) {
    (void)info;
    (void)mode;
    OCFRunLoopWakeUp(rl);
}

// Native __CFFileDescriptorCancel, 0x1c019bd70. Non-external; the run loop source's cancel callback. Does
// nothing.
static void __CFFileDescriptorCancel(void *info, OCFRunLoopRef rl, OCFRunLoopMode mode) {
    (void)info;
    (void)rl;
    (void)mode;
}

// Native __CFFileDescriptorPerform, 0x1c019bd74, with _____CFFileDescriptorPerform_block_invoke
// (0x1c019c0d0). Non-external; the run loop source's perform callback. The fork check stops a forked child.
// Synchronously on the descriptor queue, for a valid object (state 0): a fired read is cleared and confirmed
// with cffd_is_readable on the read duplicate, a fired write likewise with cffd_is_writeable on the write
// duplicate; when either is confirmed and the context has a retain callback, the info is retained for the
// callout and the context release taken, otherwise the info is used as is. After the block, a valid object
// with a confirmed event calls the callout (when set) with the confirmed types (write 2 plus read 1) and the
// info, gives the info to the taken release (when set) and makes the fork check again.
static void __CFFileDescriptorPerform(void *info) {
    OCFFileDescriptorRef f = info;
    checkForFork();
    if (processHasForked()) {
        return;
    }
    __block Boolean readable = false;
    __block Boolean writeable = false;
    __block Boolean valid = false;
    __block void *contextInfo = NULL;
    __block void (*contextRelease)(void *info) = NULL;
    dispatch_sync(__fdQueue(), ^{
      valid = f->state == 0;
      if (!valid) {
          return;
      }
      readable = f->readFired;
      if (readable) {
          f->readFired = 0;
          readable = cffd_is_readable(f->readFd);
      }
      writeable = f->writeFired;
      if (writeable) {
          f->writeFired = 0;
          writeable = cffd_is_writeable(f->writeFd);
      }
      if (f->context.retain != NULL && (readable || writeable)) {
          contextInfo = f->context.retain(f->context.info);
          contextRelease = f->context.release;
      } else {
          contextInfo = f->context.info;
      }
    });
    if (!valid || !(readable || writeable)) {
        return;
    }
    if (f->callout != NULL) {
        OCFOptionFlags callBackTypes = (writeable ? kOCFFileDescriptorWriteCallBack : 0) +
                                       (readable ? kOCFFileDescriptorReadCallBack : 0);
        f->callout(f, callBackTypes, contextInfo);
    }
    if (contextRelease != NULL) {
        contextRelease(contextInfo);
    }
    checkForFork();
    (void)processHasForked();
}
