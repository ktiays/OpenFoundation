//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleGrok.h"

#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/PlugIn/OCFBundleExecutable.h"

#include <crt_externs.h>
#include <ctype.h>
#include <fcntl.h>
#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xlocale.h>

// Inferred organization: the CFBundle_Grok functions are contiguous in the iOS Simulator __text section,
// from _CFBundleCreateInfoDictFromMainExecutable (0x1c025b0a8) to _CFBundleGrokMachTypeForFatFile
// (0x1c025f44c), and are kept in one translation unit in native order; no string names their source file.
// Definitions whose comment does not name an image identity: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.

// Local declarations of the libmacho section readers (libsystem cuts); the SDK's <mach-o/getsect.h> marks
// them deprecated, so the header is not included. The signatures are the SDK's LP64 declarations.
extern const struct segment_command_64 *getsegbyname(const char *segname);
extern const struct section_64 *
getsectbynamefromheader_64(const struct mach_header_64 *mhp, const char *segname, const char *sectname);

// Local declarations of the libmacho interfaces the fat-file reader calls (libsystem cuts). The SDK's
// <mach-o/arch.h> marks NXGetLocalArchInfo, NXFindBestFatArch and the NXArchInfo typedef deprecated, so the
// header is not included; the native code reads only the cputype (+8) and cpusubtype (+12) of the returned
// record, which ArchInfoPrefix spells.
struct NXArchInfo;
extern const struct NXArchInfo *NXGetLocalArchInfo(void);
extern struct fat_arch *NXFindBestFatArch(
    cpu_type_t cputype, cpu_subtype_t cpusubtype, struct fat_arch *fat_archs, uint32_t nfat_archs
);

typedef struct {
    const char *name;
    cpu_type_t cputype;
    cpu_subtype_t cpusubtype;
} ArchInfoPrefix;

// Native _CFBundleCurrentArchitecture (0x1c01a1960) answers the 32-bit cputype of NXGetLocalArchInfo, or 0
// (LDR W0, [X0,#8]); its caller here passes W0 as a cputype. It belongs to the bundle loader unit; this
// declaration states the native return width (see the report: PlugIn/OCFBundleBinary.h declares a pointer).
extern cpu_type_t _CFBundleCurrentArchitecture(void);

// Unaligned loads of the native LDR/LDRH from file bytes (local infrastructure helpers).
static inline uint32_t loadUInt32(const uint8_t *bytes) {
    uint32_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static inline uint16_t loadUInt16(const uint8_t *bytes) {
    uint16_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

// Native __CFBundleMagicNumbersArray, iOS Simulator 24A434, 0x1c0456b90 (__TEXT,__const, 64 32-bit words,
// 256 bytes), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: unread. _CFBundleGrokFileType compares the byte-reversed first word of a file with each
// entry in order (indices 0 to 63) and takes the extension of the first match.
static const uint32_t __CFBundleMagicNumbersArray[64] = {
    0xcafebabe, 0xbebafeca, 0xfeedface, 0xcefaedfe, 0xfeedfacf, 0xcffaedfe, 0x4a6f7921, 0x21796f4a,
    0x7f454c46, 0xffd8ffe0, 0x4d4d002a, 0x49492a00, 0x47494638, 0x89504e47, 0x69636e73, 0x00000100,
    0x7b5c7274, 0x25504446, 0x2e7261fd, 0x2e524d46, 0x2e736e64, 0x2e736400, 0x464f524d, 0x52494646,
    0x38425053, 0x000001b3, 0x000001ba, 0x4d546864, 0x504b0304, 0x53495421, 0x53495432, 0x53495435,
    0x53495444, 0x53747566, 0x30373037, 0x3c212d2d, 0x25215053, 0xd0cf11e0, 0x62656769, 0x3d796265,
    0x6b6f6c79, 0x3026b275, 0x0000000c, 0xfe370023, 0x09020600, 0x09040600, 0x4f676753, 0x664c6143,
    0x00010000, 0x74727565, 0x4f54544f, 0x41433130, 0xc809fe02, 0x0809fe02, 0x2356524d, 0x67696d70,
    0x3c435058, 0x28445746, 0x424f4d53, 0x49544f4c, 0x72746664, 0x63616666, 0x802a5fd7, 0x762f3101,
};

// Native unnamed table of _CFBundleGrokFileType, iOS Simulator 24A434, 0x1c0456c90 (64 five-byte NUL-padded
// extensions, 320 bytes, directly after __CFBundleMagicNumbersArray; one padding byte follows), UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: unread. The native code forms "mach" - 5 and adds 5 per compared entry, so the extension
// of magic index i is the table address plus 5 * i. kExtensions is a local descriptive name.
static const char kExtensions[64][5] = {
    "mach", "mach", "mach", "mach", "mach", "mach", "pef", "pef", "elf",  "jpeg", "tiff", "tiff", "gif",
    "png",  "icns", "ico",  "rtf",  "pdf",  "ra",   "rm",  "au",  "au",   "iff",  "riff", "psd",  "mpeg",
    "mpeg", "mid",  "zip",  "sit",  "sit",  "sit",  "sit", "sit", "cpio", "html", "ps",   "ole",  "uu",
    "ync",  "dmg",  "wmv",  "jp2",  "doc",  "xls",  "xls", "ogg", "flac", "ttf",  "ttf",  "otf",  "dwg",
    "dgn",  "dgn",  "wrl",  "xcf",  "cpx",  "dwf",  "bom", "lit", "rtfd", "caf",  "cin",  "exr",
};

// Native _CFBundleCreateInfoDictFromMainExecutable, iOS Simulator 24A434, 0x1c025b0a8. Hidden (was private
// external).
// When getsegbyname("__TEXT") finds the main executable's text segment, the __TEXT,__info_plist section from
// _CFBundleGetSectData and the low 32 bits of its size (LDR W1) are passed on; otherwise NULL and 0. The
// result is _CFBundleCreateInfoDictFromData's (a tail call).
OCFMutableDictionaryRef _CFBundleCreateInfoDictFromMainExecutable(void) {
    uint64_t size = 0;
    const UInt8 *section;
    uint32_t length;
    if (getsegbyname("__TEXT") != NULL) {
        section = _CFBundleGetSectData("__TEXT", "__info_plist", &size);
        length = (uint32_t)size;
    } else {
        section = NULL;
        length = 0;
    }
    return _CFBundleCreateInfoDictFromData(section, length);
}

// Native _CFBundleGetSectData, iOS Simulator 24A434, 0x1c025b124. Non-external.
// _dyld_image_count is read before _NSGetMachExecuteHeader. The image whose header equals the main
// executable's is searched from index 0; without one, or without the section (getsectbynamefromheader_64),
// the result is NULL and *size 0. Otherwise the result is the image's vmaddr slide plus the section address
// (+0x20, read before the slide) and *size the 64-bit section size (+0x28).
const UInt8 *_CFBundleGetSectData(const char *segname, const char *sectname, uint64_t *size) {
    const uint32_t count = _dyld_image_count();
    const struct mach_header_64 *header = _NSGetMachExecuteHeader();
    uint64_t sectionSize = 0;
    const UInt8 *result = NULL;
    for (uint32_t index = 0; index != count; ++index) {
        if ((const struct mach_header *)header == _dyld_get_image_header(index)) {
            const struct section_64 *section = getsectbynamefromheader_64(header, segname, sectname);
            if (section != NULL) {
                const uint64_t address = section->addr;
                result = (const UInt8 *)(uintptr_t)(_dyld_get_image_vmaddr_slide(index) + address);
                sectionSize = section->size;
            }
            break;
        }
    }
    *size = sectionSize;
    return result;
}

// Native _CFBundleCreateInfoDictFromData, iOS Simulator 24A434, 0x1c025b1d0. Non-external.
// _os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle") is asked first. With the feature, bytes and a
// nonzero length become a no-copy CFData (kCFAllocatorNull deallocator) for _CFBundleCreateInfoPlistWithData,
// the data is released, and a NULL result (or missing bytes) answers a new empty mutable dictionary with the
// CFType callbacks. Without it, missing bytes answer NULL; otherwise the data is parsed by
// CFPropertyListCreateWithData (options 1, mutable containers); a dictionary result is kept, anything else is
// released and replaced by a new empty mutable dictionary (a failed data creation goes there directly), and
// the dictionary, when not NULL, passes through _CFBundleInfoPlistProcessInfoDictionary.
OCFMutableDictionaryRef _CFBundleCreateInfoDictFromData(const UInt8 *bytes, uint32_t length) {
    const bool swiftBundle = _os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle");
    const bool hasBytes = bytes != NULL && length != 0;
    if (swiftBundle) {
        const OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        if (hasBytes) {
            OCFDataRef data = OCFDataCreateWithBytesNoCopy(allocator, bytes, length, kOCFAllocatorNull);
            OCFMutableDictionaryRef result = _CFBundleCreateInfoPlistWithData(data);
            OCFRelease(data);
            if (result != NULL) {
                return result;
            }
        }
        return OCFDictionaryCreateMutable(
            allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    if (!hasBytes) {
        return NULL;
    }
    const OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableDictionaryRef result = NULL;
    OCFDataRef data = OCFDataCreateWithBytesNoCopy(allocator, bytes, length, kOCFAllocatorNull);
    if (data != NULL) {
        OCFPropertyListRef plist = OCFPropertyListCreateWithData(allocator, data, 1, NULL, NULL);
        if (plist != NULL) {
            const OCFTypeID dictionaryTypeID = OCFDictionaryGetTypeID();
            if (dictionaryTypeID == OCFGetTypeID(plist)) {
                OCFRelease(data);
                result = (OCFMutableDictionaryRef)plist;
                _CFBundleInfoPlistProcessInfoDictionary(result);
                return result;
            }
            OCFRelease(plist);
        }
        OCFRelease(data);
    }
    result = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    if (result != NULL) {
        _CFBundleInfoPlistProcessInfoDictionary(result);
    }
    return result;
}

// Native _CFBundleGrokObjCImageInfoFromMainExecutable, iOS Simulator 24A434, 0x1c025b34c. Hidden (was a
// private external). Without a __DATA segment (getsegbyname) both words are 0 and the result false. Otherwise
// __DATA,__objc_imageinfo of the main executable (_CFBundleGetSectData) of at least 8 bytes gives its two
// 32-bit words and true; a missing or shorter section gives 0, 0 and false. Each word is stored only through
// a non-NULL pointer.
Boolean _CFBundleGrokObjCImageInfoFromMainExecutable(uint32_t *objcVersion, uint32_t *objcFlags) {
    uint64_t size = 0;
    Boolean result = false;
    uint32_t version = 0;
    uint32_t flags = 0;
    if (getsegbyname("__DATA") != NULL) {
        const uint32_t *section = (const uint32_t *)_CFBundleGetSectData("__DATA", "__objc_imageinfo", &size);
        if (section != NULL && size >= 8) {
            version = section[0];
            flags = section[1];
            result = true;
        }
    }
    if (objcVersion != NULL) {
        *objcVersion = version;
    }
    if (objcFlags != NULL) {
        *objcFlags = flags;
    }
    return result;
}

// Native _CFBundleGrokFileType, iOS Simulator 24A434, 0x1c025b494 (8,112 bytes; no .cold fragments).
// Non-external.
// The native function is one block graph; the labels below name its join points: `post` (0x1c025c2f8), `tail`
// (0x1c025cf74, the bz2/dmg test of a found extension), `dmg` (0x1c025cfa8), `finish` (0x1c025b6c0, the
// extension string) and `done` (0x1c025b74c). `heuristics` (0x1c025b694) and `afterHeuristics`
// (0x1c025b6fc) run only with an extension pointer and no extension yet.
// 1. Every output except *extension and *machtype is cleared. With a URL whose file system representation
//    (1,026 bytes) names a file that stat accepts and whose st_mode, as a signed 16-bit value, is below
//    -28672 (a regular file), open(path, O_RDONLY, 0777) and read 512 bytes; the bytes then come from the
//    file and fileLength is st_size. Otherwise, with data, its length and byte pointer are used (fileLength
//    is the data length); an empty data answers "txt". Without bytes the result is NULL.
// 2. With at least four bytes, the byte-reversed first word is looked up in __CFBundleMagicNumbersArray.
//    A Java class file (first word 0xcafebabe on disk with a nonzero 16-bit word at +4) is "class". From 32
//    bytes, fat files go to _CFBundleGrokMachTypeForFatFile (swapped for the big-endian on-disk magic) and
//    thin Mach-O files take filetype (+12, the mach type) and cputype (+4) directly or byte-swapped; for
//    each, an X11 check (filetype 2), the Info.plist of __TEXT,__info_plist and the Objective-C image
//    information are read as the outputs ask. Mach types 1 "o", 2 "tool" or "x11app", 4 "core", 6 "dylib", 8
//    "bundle" and "Joy!" 0x1000 "pef" decide; any other magic is checked by its own rule below, which keeps
//    the table extension, replaces it, or drops it.
// 3. Without an extension, the heuristics recognize headers (tga, mov/qtif/cwk, bplist and webarchive,
//    ISO media brands, bmp, pfb, hqx, MacBinary, tar, two-byte magics, scripts, tfm), then a scan of up to
//    512 bytes classifies text (html and xml tags at the start, "StartFontMetrics" "afm", else "txt") or,
//    when every byte is zero, a PICT header at 522.
// 4. A found "bz2", or no extension, with at least 512 bytes and a file of at least 512 bytes is tested for a
//    disk image trailer ("koly", or "cdsa" "encr" at the end). *extension becomes a no-copy UTF-8 string
//    (kCFAllocatorNull) or NULL, *machtype the mach type, the descriptor is closed, and the result tells
//    whether an extension was found (also without an extension pointer).
// Length comparisons are signed or unsigned as in the native code: a failed read gives length -1, which
// the signed tests treat as short ("txt"). The FORM test compares the second word with 0x00414946 where
// "AIFC" would be 0x41494643, so the native code never answers "aifc"; it is kept. The 3GP brand test is a
// vector compare of the brand against four masked values (xmmword_1C0456B70/B80 in __const), written here as
// the equivalent four scalar tests.
Boolean _CFBundleGrokFileType(
    OCFURLRef url,
    OCFDataRef data,
    OCFStringRef *extension,
    UInt32 *machtype,
    OCFArrayRef *architectures,
    OCFDictionaryRef *infodict,
    uint8_t *hasObjc,
    uint32_t *objcVersion,
    uint32_t *objcFlags
) {
    UInt8 path[1026];
    struct stat statBuffer;
    uint8_t header[512];
    const void *values[1];
    int32_t cpuType;
    uint8_t isX11 = 0;
    int fd = -1;
    uint8_t useBytes = 1;
    const uint8_t *bytes = NULL;
    OCFIndex length = 0;
    int64_t fileLength = 0;
    const char *ext = NULL;
    UInt32 type = 0;
    uint32_t rawMagic = 0;
    uint32_t magic = 0;
    uint8_t longerThanEleven = 0;
    uint8_t atLeastTwenty = 0;
    if (architectures != NULL) {
        *architectures = NULL;
    }
    if (infodict != NULL) {
        *infodict = NULL;
    }
    if (hasObjc != NULL) {
        *hasObjc = 0;
    }
    if (objcVersion != NULL) {
        *objcVersion = 0;
    }
    if (objcFlags != NULL) {
        *objcFlags = 0;
    }
    if (url != NULL) {
        const Boolean converted = OCFURLGetFileSystemRepresentation(url, true, path, 1026);
        memset(&statBuffer, 0, sizeof(statBuffer));
        if (converted && stat((const char *)path, &statBuffer) == 0 && (int16_t)statBuffer.st_mode < -28672) {
            fd = open((const char *)path, O_RDONLY, 0777);
            if (fd >= 0) {
                bytes = header;
                length = read(fd, header, 512);
                useBytes = 0;
                fileLength = statBuffer.st_size;
            }
        }
    }
    if (data != NULL && useBytes) {
        const OCFIndex dataLength = OCFDataGetLength(data);
        bytes = OCFDataGetBytePtr(data);
        fileLength = dataLength;
        if (dataLength == 0) {
            type = 0;
            ext = "txt";
            if (bytes == NULL) {
                goto finish;
            }
            length = 0;
            fileLength = 0;
            goto checked;
        }
        length = dataLength;
    }
    if (bytes == NULL) {
        type = 0;
        ext = NULL;
        goto finish;
    }
    if (length < 4) {
        type = 0;
        ext = NULL;
        goto afterHeuristics;
    }
    rawMagic = loadUInt32(bytes);
    magic = __builtin_bswap32(rawMagic);
    {
        uint64_t index = 0;
        uint32_t entry;
        for (;;) {
            entry = __CFBundleMagicNumbersArray[index];
            if (entry == magic || index >= 63) {
                break;
            }
            ++index;
        }
        if (entry != magic) {
            type = 0;
            ext = NULL;
            goto heuristics;
        }
        ext = kExtensions[index];
    }
    if (rawMagic == 0xbebafeca && (uint64_t)length >= 8 && loadUInt16(bytes + 4) != 0) {
        type = 0;
        ext = "class";
        goto post;
    }
    if ((uint64_t)length < 32) {
        type = 0;
        goto magicChecks;
    }
    {
        uint8_t *isX11Pointer = extension != NULL ? &isX11 : NULL;
        values[0] = NULL;
        if (extension != NULL) {
            isX11 = 0;
        }
        if (architectures != NULL) {
            *architectures = NULL;
        }
        if (infodict != NULL) {
            *infodict = NULL;
        }
        if (hasObjc != NULL) {
            *hasObjc = 0;
        }
        if (objcVersion != NULL) {
            *objcVersion = 0;
        }
        if (objcFlags != NULL) {
            *objcFlags = 0;
        }
        cpuType = 0;
        uint8_t swapped;
        uint8_t sixtyFour;
        uint32_t fileType;
        switch (rawMagic) {
            case 0xbebafeca:
            case 0xcafebabe:
                type = _CFBundleGrokMachTypeForFatFile(
                    fd,
                    bytes,
                    (size_t)length,
                    rawMagic == 0xbebafeca,
                    isX11Pointer,
                    architectures,
                    infodict,
                    hasObjc,
                    objcVersion,
                    objcFlags
                );
                goto machResult;
            case 0xfeedface:
            case 0xfeedfacf:
                swapped = 0;
                sixtyFour = rawMagic == 0xfeedfacf;
                fileType = loadUInt32(bytes + 12);
                cpuType = (int32_t)loadUInt32(bytes + 4);
                break;
            case 0xcefaedfe:
            case 0xcffaedfe:
                swapped = 1;
                sixtyFour = rawMagic == 0xcffaedfe;
                fileType = loadUInt32(bytes + 12);
                cpuType = (int32_t)__builtin_bswap32(loadUInt32(bytes + 4));
                break;
            case 0x21796f4a:
            case 0x4a6f7921:
                type = 0x1000;
                ext = "pef";
                goto checked;
            default:
                type = 0;
                goto magicChecks;
        }
        OCFNumberRef number = NULL;
        if (architectures != NULL) {
            number = OCFNumberCreate(kOCFAllocatorSystemDefault, kOCFNumberSInt32Type, &cpuType);
            values[0] = number;
        }
        if (extension != NULL && fileType == (swapped ? 0x2000000 : 2)) {
            isX11 = _CFBundleGrokX11FromFile(fd, bytes, (size_t)length, 0, swapped, sixtyFour);
        }
        if (swapped) {
            fileType = __builtin_bswap32(fileType);
        }
        if (infodict != NULL) {
            *infodict = _CFBundleCreateInfoDictFromFile(fd, bytes, (size_t)length, 0, swapped, sixtyFour);
        }
        if (hasObjc != NULL || objcVersion != NULL || objcFlags != NULL) {
            _CFBundleGrokObjcImageInfoFromFile(
                fd, bytes, (size_t)length, 0, swapped, sixtyFour, hasObjc, objcVersion, objcFlags
            );
        }
        type = fileType;
        if (architectures != NULL && number != NULL) {
            *architectures = OCFArrayCreate(kOCFAllocatorSystemDefault, values, 1, &kOCFTypeArrayCallBacks);
            number = (OCFNumberRef)values[0];
        }
        if (number != NULL) {
            OCFRelease(number);
        }
    }
machResult:
    switch (type) {
        case 1:
            ext = "o";
            goto checked;
        case 2:
            ext = isX11 == 0 ? "tool" : "x11app";
            goto checked;
        case 4:
            ext = "core";
            goto checked;
        case 6:
            ext = "dylib";
            goto checked;
        case 8:
            ext = "bundle";
            goto checked;
        case 0x1000:
            ext = "pef";
            type = 0x1000;
            goto checked;
        default:
            break;
    }
magicChecks:
    // The rules of 0x1c025c078: `post` keeps the table extension; `heuristics` drops it.
    switch (magic) {
        case 0x89504e47:
            if ((uint64_t)length >= 8 && loadUInt32(bytes + 4) == 0x0a1a0a0d) {
                goto post;
            }
            break;
        case 0xd0cf11e0:
            if ((uint64_t)length < 0x34) {
                goto post;
            }
            ext = _CFBundleGrokFileTypeForOLEFile(
                fd, bytes, (size_t)length, (uint32_t)((loadUInt32(bytes + 0x30) << 9) + 512)
            );
            goto checked;
        case 0x0000000c:
            if ((uint64_t)length >= 6 && loadUInt16(bytes + 4) == 0x506a) {
                goto post;
            }
            break;
        case 0x00010000:
            if ((uint64_t)length >= 6 && bytes[4] == 0) {
                goto post;
            }
            break;
        case 0x2356524d:
            if ((uint64_t)length >= 6 && loadUInt16(bytes + 4) == 0x204c) {
                goto post;
            }
            break;
        case 0x25215053:
            if ((uint64_t)length < 11) {
                break;
            }
            if (strncmp((const char *)bytes + 4, "-Adobe-", 7) == 0) {
                ext = "ps";
                goto checked;
            }
            if ((uint64_t)length >= 14 && strncmp((const char *)bytes + 4, "-AdobeFont", 10) == 0) {
                ext = "pfa";
                goto checked;
            }
            break;
        case 0x25504446:
            if ((uint64_t)length >= 6 && bytes[4] == '-') {
                goto post;
            }
            break;
        case 0x28445746:
            if ((uint64_t)length >= 6 && loadUInt16(bytes + 4) == 0x5620) {
                goto post;
            }
            break;
        case 0x3026b275:
            if ((uint64_t)length >= 8 && loadUInt32(bytes + 4) == 0x11cf668e) {
                goto post;
            }
            break;
        case 0x30373037:
            if ((uint64_t)length >= 6 && bytes[4] == '0' && (uint32_t)(bytes[5] - '0') < 10) {
                goto post;
            }
            break;
        case 0x3d796265:
            if ((uint64_t)length >= 12 && loadUInt32(bytes + 4) == 0x206e6967 &&
                (loadUInt32(bytes + 8) == 0x656e696c || loadUInt32(bytes + 8) == 0x74726170)) {
                goto post;
            }
            break;
        case 0x41433130:
            if ((uint64_t)length >= 6 && bytes[4] == '1' && (uint32_t)(bytes[5] - '0') < 10) {
                goto post;
            }
            break;
        case 0x424f4d53:
            if ((uint64_t)length >= 8 && loadUInt32(bytes + 4) == 0x65726f74) {
                goto post;
            }
            break;
        case 0x464f524d:
            if ((uint64_t)length < 12) {
                break;
            }
            if (__builtin_bswap32(loadUInt32(bytes + 8)) == 0x41494646) {
                ext = "aiff";
                goto checked;
            }
            if (__builtin_bswap32(loadUInt32(bytes + 8)) == 0x00414946) {
                ext = "aifc";
                goto checked;
            }
            break;
        case 0x47494638:
            if ((uint64_t)length >= 6 &&
                (loadUInt16(bytes + 4) == 0x6137 || loadUInt16(bytes + 4) == 0x6139)) {
                goto post;
            }
            break;
        case 0x49544f4c:
            if ((uint64_t)length >= 8 && loadUInt32(bytes + 4) == 0x534c5449) {
                goto post;
            }
            break;
        case 0x504b0304:
            ext = _CFBundleGrokFileTypeForZipFile(fd, bytes, (size_t)length, fileLength);
            goto checked;
        case 0x52494646:
            if ((uint64_t)length < 12) {
                break;
            }
            if (__builtin_bswap32(loadUInt32(bytes + 8)) == 0x57415645) {
                ext = "wav";
                goto checked;
            }
            if (__builtin_bswap32(loadUInt32(bytes + 8)) == 0x41564920) {
                ext = "avi";
                goto checked;
            }
            break;
        case 0x53747566:
            if ((uint64_t)length >= 8 && loadUInt32(bytes + 4) == 0x20744966) {
                goto post;
            }
            break;
        case 0x62656769:
            // uuencode: "begin " (checked as 'n', ' ' at 4 and 5), three digits and a space, then the first
            // line end at 10 or later; the next line starts with 'M' and holds 61 printable characters
            // (isgraph of the unsigned byte) before the 62nd, a line end.
            if ((uint64_t)length < 0x4c || bytes[4] != 'n' || bytes[5] != ' ' ||
                (uint32_t)(bytes[6] - '0') > 9 || (uint32_t)(bytes[7] - '0') > 9) {
                break;
            }
            ext = NULL;
            if ((uint32_t)(bytes[8] - '0') <= 9 && bytes[9] == ' ') {
                uint64_t count = 0;
                uint8_t character;
                OCFIndex next;
                do {
                    character = bytes[10 + count];
                    next = (OCFIndex)(count + 11);
                    ++count;
                } while (character != '\n' && next < length);
                const uint64_t lineEnd = character == '\n' ? count + 9 : 0;
                const uint64_t secondEnd = lineEnd + 62;
                if (lineEnd >= 10 && (OCFIndex)secondEnd < length && bytes[lineEnd + 1] == 'M' &&
                    bytes[secondEnd] == '\n') {
                    uint64_t position = lineEnd;
                    int printable;
                    uint64_t following;
                    do {
                        printable = isgraph(bytes[1 + position]);
                        following = position + 2;
                        ++position;
                    } while (printable != 0 && following < secondEnd);
                    ext = printable != 0 ? "uu" : NULL;
                }
            }
            goto heuristics;
        case 0x63616666:
            if ((uint64_t)length >= 12 && bytes[4] == 0 && loadUInt32(bytes + 8) == 0x63736564) {
                goto post;
            }
            break;
        case 0x67696d70:
            if ((uint64_t)length >= 8 && loadUInt32(bytes + 4) == 0x66637820) {
                goto post;
            }
            break;
        case 0x72746664:
            if ((uint64_t)length >= 8 && loadUInt32(bytes + 4) == 0) {
                goto post;
            }
            break;
        case 0x7b5c7274:
            if ((uint64_t)length >= 6 && bytes[4] == 'f') {
                goto post;
            }
            break;
        default:
            goto post;
    }
    ext = NULL;
heuristics:
    if (extension == NULL || ext != NULL) {
        goto post;
    }
    {
        const uint16_t firstUnit = loadUInt16(bytes);
        if (length == 4) {
            longerThanEleven = 0;
            atLeastTwenty = 0;
            goto twoBytes;
        }
        if (bytes[3] == 0 && bytes[4] == 0) {
            if (bytes[1] == 0) {
                if ((bytes[2] & 0xf6) == 2) {
                    ext = "tga";
                    goto tail;
                }
            } else if (bytes[1] == 1 && (bytes[2] & 0xf7) == 1) {
                ext = "tga";
                goto tail;
            }
        }
        if (length < 8) {
            longerThanEleven = 0;
            atLeastTwenty = 0;
            goto twoBytes;
        }
        const uint32_t second = loadUInt32(bytes + 4);
        if (second == 0x4f424f42) {
            ext = "cwk";
            goto tail;
        }
        if (second == 0x63736469 || second == 0x74616469) {
            ext = "qtif";
            goto tail;
        }
        if (second == 0x65646977 || second == 0x766f6f6d || second == 0x7461646d) {
            ext = "mov";
            goto tail;
        }
        if (rawMagic == 0x696c7062 && (second & 0xffff) == 0x7473 &&
            (uint32_t)(((second >> 16) & 0xff) - '0') <= 9 && (uint32_t)((second >> 24) - '0') <= 9) {
            ext = "plist";
            if ((uint64_t)length >= 0x18) {
                uint64_t position = 8;
                int difference;
                for (;;) {
                    difference = strncmp((const char *)bytes + position, "WebMainResource", 15);
                    if (difference == 0 || position > 0x7e) {
                        break;
                    }
                    const uint64_t end = position + 17;
                    ++position;
                    if (end > (uint64_t)length) {
                        break;
                    }
                }
                if (difference == 0) {
                    ext = "webarchive";
                }
            }
            goto tail;
        }
        longerThanEleven = (uint64_t)length > 11;
        if (firstUnit == 0 && (uint64_t)length >= 12) {
            if (second == 0x70797466) {
                const uint32_t brand = __builtin_bswap32(loadUInt32(bytes + 8));
                if ((int32_t)brand > 0x69736f31) {
                    if (brand - 0x6d703431 < 2 || brand == 0x69736f6d || brand == 0x69736f32) {
                        ext = "mp4";
                        goto tail;
                    }
                } else if ((int32_t)brand > 0x4d34561f) {
                    const uint32_t offset = brand - 0x4d345620;
                    if (offset <= 0x30 && ((UINT64_C(1) << offset) & UINT64_C(0x1010000000001)) != 0) {
                        ext = "m4v";
                        goto tail;
                    }
                } else if (brand == 0x4d344120) {
                    ext = "m4a";
                    goto tail;
                } else if (brand == 0x4d344220) {
                    ext = "m4b";
                    goto tail;
                } else if (brand == 0x4d345020) {
                    ext = "m4p";
                    goto tail;
                }
                if ((brand >> 16) != 0x3367) {
                    ext = NULL;
                    goto afterHeuristics;
                }
                ext = "3gp";
                if ((brand & 0xfffe) == 0x6536 || (brand & 0xffff) == 0x6736 || (brand & 0xfffe) == 0x7034 ||
                    (brand & 0xfdff) == 0x7036 || (brand & 0xfffe) == 0x7336) {
                    goto tail;
                }
                ext = (brand & 0xffff) == 0x3261 ? "3g2" : NULL;
                goto afterHeuristics;
            }
        } else if (firstUnit == 0x4d42 && (uint64_t)length >= 0x12) {
            const uint32_t headerSize = loadUInt32(bytes + 14);
            ext = "bmp";
            if (headerSize - 12 <= 0x34 &&
                ((UINT64_C(1) << (headerSize - 12)) & UINT64_C(0x10000010000001)) != 0) {
                goto tail;
            }
            if (headerSize == 0x6c || headerSize == 0x7c) {
                goto tail;
            }
            ext = NULL;
            goto afterHeuristics;
        }
        if ((uint64_t)length < 0x14) {
            atLeastTwenty = 1;
            goto twoBytes;
        }
        if (strncmp((const char *)bytes + 6, "%!PS-AdobeFont", 14) == 0) {
            ext = "pfb";
            goto tail;
        }
        if ((uint64_t)length < 0x28) {
            longerThanEleven = 1;
            atLeastTwenty = 1;
            goto twoBytes;
        }
        if (loadUInt32(bytes + 34) == 0x486e6942 && loadUInt16(bytes + 38) == 0x7865) {
            ext = "hqx";
            goto tail;
        }
        if ((uint64_t)length < 0x80) {
            longerThanEleven = 1;
            atLeastTwenty = 1;
            goto twoBytes;
        }
        ext = "bin";
        if (loadUInt32(bytes + 102) == 0x4e49426d) {
            goto tail;
        }
        if (bytes[0] == 0 && (uint32_t)(bytes[1] - 1) <= 0x3e && bytes[74] == 0 && bytes[82] == 0 &&
            (fileLength & 0x7f) == 0) {
            // MacBinary: the file length must equal the 128-byte header plus both forks rounded to 128 bytes.
            const uint32_t dataLength = __builtin_bswap32(loadUInt32(bytes + 83));
            const uint32_t resourceLength = __builtin_bswap32(loadUInt32(bytes + 87));
            const uint32_t blocks = ((dataLength + 127) >> 7) + ((resourceLength + 127) >> 7) + 1;
            if (!(fileLength == (int64_t)(uint32_t)(blocks << 7) && blocks > 1 && resourceLength < 0x800000 &&
                  dataLength < 0x800000)) {
                ext = NULL;
            }
            goto afterHeuristics;
        }
        if ((uint64_t)length >= 0x109 && loadUInt32(bytes + 257) == 0x61747375) {
            ext = "tar";
            const uint32_t version = loadUInt32(bytes + 261);
            if (version - 0x202000 == 0x72 || (version & 0xffff) == 0x72) {
                goto tail;
            }
        }
        longerThanEleven = 1;
        atLeastTwenty = 1;
    }
twoBytes:
    switch ((uint16_t)((bytes[0] << 8) | bytes[1])) {
        case 0x01da:
            if (bytes[2] <= 1 && (uint32_t)(bytes[3] - 1) < 15) {
                ext = "sgi";
                goto tail;
            }
            break;
        case 0x1f8b:
            ext = "gz";
            goto tail;
        case 0x1f9d:
            ext = "Z";
            goto tail;
        case 0x2321: {
            uint64_t count = 0;
            uint8_t character;
            OCFIndex next;
            do {
                character = bytes[count + 2];
                next = (OCFIndex)(count + 3);
                ++count;
            } while (character != '\n' && next < length);
            if (!(character == '\n' && count + 1 >= 4)) {
                ext = NULL;
                goto afterHeuristics;
            }
            int64_t position = (int64_t)count;
            int64_t slash;
            int64_t limit;
            do {
                limit = position + 1;
                slash = bytes[position] == '/' ? position : 0;
                --position;
            } while (limit >= 4 && slash == 0);
            if (slash == 0) {
                ext = NULL;
                goto afterHeuristics;
            }
            const char *interpreter = (const char *)bytes + slash + 1;
            if (strncmp(interpreter, "perl", 4) == 0) {
                ext = "pl";
            } else if (strncmp(interpreter, "python", 6) == 0) {
                ext = "py";
            } else {
                ext = strncmp(interpreter, "ruby", 4) == 0 ? "rb" : "sh";
            }
            goto tail;
        }
        case 0x425a:
            if ((uint32_t)(bytes[2] - '0') <= 9) {
                if ((uint32_t)(bytes[3] - '0') < 10) {
                    ext = "bz";
                    goto tail;
                }
            } else if (bytes[2] == 'h') {
                uint8_t skip = atLeastTwenty ^ 1;
                if ((uint32_t)(bytes[3] - '0') > 9) {
                    skip = 1;
                }
                if ((skip & 1) == 0) {
                    const uint32_t block = loadUInt32(bytes + 4);
                    if (block == 0x26594131 || block == 0x38457217) {
                        ext = "bz2";
                        goto tail;
                    }
                }
            }
            break;
        case 0x4357:
            if (bytes[2] == 'S') {
                ext = "swc";
                goto tail;
            }
            break;
        case 0x4657:
            if (bytes[2] == 'S') {
                ext = "swf";
                goto tail;
            }
            break;
        case 0x4944:
            if (bytes[2] == '3' && bytes[3] < 0x20) {
                ext = "mp3";
                goto tail;
            }
            break;
        case 0x71c7:
        case 0xc771:
            ext = "cpio";
            goto tail;
        case 0xf702:
            ext = "dvi";
            goto tail;
        case 0xffd8:
            if (bytes[2] == 0xff) {
                ext = "jpeg";
                goto tail;
            }
            break;
        case 0xfeff:
        case 0xfffe:
            ext = "txt";
            if (longerThanEleven) {
                const uint32_t first = loadUInt32(bytes);
                if (first == 0xfffe3c00) {
                    if (loadUInt32(bytes + 4) == 0x68007400 && loadUInt32(bytes + 8) == 0x6d006c00) {
                        ext = "html";
                    }
                } else if (first == 0x003cfeff) {
                    if (loadUInt32(bytes + 4) == 0x00740068 && loadUInt32(bytes + 8) == 0x006c006d) {
                        ext = "html";
                    }
                }
            }
            goto tail;
        default:
            break;
    }
    ext = "tfm";
    if (loadUInt16(bytes + 2) == 0x1100 || loadUInt16(bytes + 2) == 0x1200) {
        goto tail;
    }
    ext = NULL;
afterHeuristics:
    if (extension == NULL || ext != NULL) {
        goto post;
    }
    {
        OCFIndex index;
        uint8_t allZero;
        if (length >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb) {
            allZero = bytes[2] != 0xbf;
            index = bytes[2] == 0xbf ? 3 : 0;
        } else {
            index = 0;
            allZero = 1;
        }
        const OCFIndex limit = length < 512 ? length : 512;
        uint8_t doctypeAllowed = 1;
        uint8_t isText = 1;
        uint8_t leadingSpace = 1;
        while (index < limit) {
            const uint8_t *position = bytes + index;
            const uint8_t character = *position;
            if (doctypeAllowed && character == '<' && index + 14 <= length) {
                ext =
                    strncasecmp_l((const char *)position + 1, "!doctype html", 13, NULL) == 0 ? "html" : NULL;
            } else {
                ext = NULL;
            }
            if (leadingSpace && character == '<' && index + 14 <= length) {
                const char *tag = (const char *)position + 1;
                if (strncasecmp_l(tag, "!doctype html", 13, NULL) == 0 ||
                    strncasecmp_l(tag, "head", 4, NULL) == 0 || strncasecmp_l(tag, "title", 5, NULL) == 0 ||
                    strncasecmp_l(tag, "script", 6, NULL) == 0 || strncasecmp_l(tag, "html", 4, NULL) == 0) {
                    ext = "html";
                } else if (strncasecmp_l(tag, "?xml", 4, NULL) == 0) {
                    OCFIndex next = index + 4;
                    uint8_t found = ext != NULL;
                    if (ext == NULL && index <= 0x7b) {
                        OCFIndex bound = index + 24;
                        while (bound <= length) {
                            OCFIndex at = next;
                            const char *element = (const char *)bytes + at;
                            if (*element != '<') {
                                ext = NULL;
                            } else if (strncasecmp_l(element + 1, "abiword", 7, NULL) == 0) {
                                ext = "abw";
                            } else if (strncasecmp_l(element + 1, "!doctype svg", 12, NULL) == 0) {
                                ext = "svg";
                            } else if (strncasecmp_l(element + 1, "!doctype rdf", 12, NULL) == 0) {
                                ext = "rdf";
                            } else if (strncasecmp_l(element + 1, "!doctype x3d", 12, NULL) == 0) {
                                ext = "x3d";
                            } else if (strncasecmp_l(element + 1, "!doctype html", 13, NULL) == 0) {
                                ext = "html";
                            } else if (strncasecmp_l(element + 1, "!doctype posingfont", 19, NULL) == 0) {
                                ext = "sfont";
                            } else if (strncasecmp_l(element + 1, "!doctype plist", 14, NULL) == 0) {
                                const OCFIndex start = at + 14;
                                if (at + 30 > length) {
                                    ext = "plist";
                                    at = start;
                                } else {
                                    OCFIndex scan = start;
                                    OCFIndex after;
                                    int difference;
                                    for (;;) {
                                        difference =
                                            strncmp((const char *)bytes + scan, "WebMainResource", 15);
                                        after = scan + 1;
                                        if (difference == 0 || scan > 0xfe) {
                                            break;
                                        }
                                        const OCFIndex end = scan + 17;
                                        scan = after;
                                        if (end > length) {
                                            break;
                                        }
                                    }
                                    ext = difference == 0 ? "webarchive" : "plist";
                                    at = after;
                                }
                            } else {
                                ext = NULL;
                            }
                            next = at + 1;
                            found = ext != NULL;
                            if (ext != NULL || at > 0x7e) {
                                break;
                            }
                            bound = at + 21;
                        }
                    }
                    if (!found) {
                        ext = "xml";
                    }
                    index = next;
                }
            }
            const int8_t signedCharacter = (int8_t)character;
            if (signedCharacter != 0) {
                allZero = 0;
            }
            uint8_t space;
            if (allZero || signedCharacter == 0x7f) {
                isText = 0;
                space = 0;
                if (!allZero) {
                    space = isspace(signedCharacter) ? leadingSpace : 0;
                }
            } else {
                if (signedCharacter <= 31 && !isspace(signedCharacter)) {
                    isText = 0;
                }
                space = isspace(signedCharacter) ? leadingSpace : 0;
            }
            if (!isText && !allZero) {
                break;
            }
            ++index;
            leadingSpace = space;
            if (ext != NULL) {
                break;
            }
            doctypeAllowed = isText != 0;
        }
        if (ext != NULL) {
            goto post;
        }
        if (isText) {
            if (length < 16) {
                ext = "txt";
                goto tail;
            }
            if (strncmp((const char *)bytes, "StartFontMetrics", 16) == 0) {
                ext = "afm";
                goto post;
            }
            ext = "txt";
            goto tail;
        }
        ext = NULL;
        if (!allZero || length < 512 || fileLength < 0x20e) {
            goto post;
        }
        if (useBytes) {
            if ((uint64_t)length < 0x20e) {
                goto post;
            }
            if (loadUInt32(bytes + 522) == 0xff021100) {
                ext = "pict";
                goto tail;
            }
            goto postNoExtension;
        }
        if (lseek(fd, 512, SEEK_SET) != 512 || read(fd, header, 512) < 14) {
            ext = NULL;
            goto post;
        }
        if (loadUInt32(header + 10) == 0xff021100) {
            ext = "pict";
            goto tail;
        }
        goto postNoExtension;
    }
checked:
    if (extension == NULL) {
        goto done;
    }
    goto tail;
post:
    if (extension == NULL) {
        goto done;
    }
    if (ext != NULL) {
        goto tail;
    }
    ext = NULL;
postNoExtension:
    if (length < 512 || fileLength < 512) {
        goto finish;
    }
    goto dmg;
tail:
    if (strcmp(ext, "bz2") != 0 || length < 512 || fileLength <= 511) {
        goto finish;
    }
dmg:
    if (useBytes) {
        const uint8_t *end = bytes + length;
        if (loadUInt32(end - 512) == 0x796c6f6b ||
            (loadUInt32(end - 8) == 0x61736463 && loadUInt32(end - 4) == 0x72636e65)) {
            ext = "dmg";
        }
    } else {
        const off_t trailer = fileLength - 512;
        if (lseek(fd, trailer, SEEK_SET) == trailer && read(fd, header, 512) >= 512) {
            if (loadUInt32(header) == 0x796c6f6b ||
                (loadUInt32(header + 0x1f8) == 0x61736463 && loadUInt32(header + 0x1fc) == 0x72636e65)) {
                ext = "dmg";
            }
        }
    }
finish:
    if (extension != NULL) {
        // 0x08000100 is kCFStringEncodingUTF8.
        *extension = ext != NULL ? OCFStringCreateWithCStringNoCopy(
                                       kOCFAllocatorSystemDefault, ext, 0x08000100, kOCFAllocatorNull
                                   )
                                 : NULL;
    }
done:
    if (machtype != NULL) {
        *machtype = type;
    }
    if (fd >= 0) {
        close(fd);
    }
    return ext != NULL;
}

// Native _CFBundleCopyArchitecturesForExecutable, iOS Simulator 24A434, 0x1c025d5c4. Hidden (was a private
// external). _CFBundleGrokFileType is asked for the architectures only.
OCFArrayRef _CFBundleCopyArchitecturesForExecutable(OCFURLRef url) {
    OCFArrayRef architectures = NULL;
    _CFBundleGrokFileType(url, NULL, NULL, NULL, &architectures, NULL, NULL, NULL, NULL);
    return architectures;
}

// Native _CFBundleGetObjCImageInfo, iOS Simulator 24A434, 0x1c025d634. Hidden (was a private external). The
// executable URL's Objective-C image information comes from _CFBundleGrokFileType (both words 0 and no
// Objective-C without an executable); each word is stored only through a non-NULL pointer, and the result
// tells whether the executable has Objective-C.
Boolean _CFBundleGetObjCImageInfo(OCFBundleRef bundle, uint32_t *objcVersion, uint32_t *objcFlags) {
    uint32_t flags = 0;
    uint32_t version = 0;
    Boolean result;
    OCFURLRef executableURL = OCFBundleCopyExecutableURL(bundle);
    if (executableURL != NULL) {
        uint8_t hasObjc = 0;
        _CFBundleGrokFileType(executableURL, NULL, NULL, NULL, NULL, NULL, &hasObjc, &version, &flags);
        result = hasObjc;
        OCFRelease(executableURL);
    } else {
        result = false;
    }
    if (objcVersion != NULL) {
        *objcVersion = version;
    }
    if (objcFlags != NULL) {
        *objcFlags = flags;
    }
    return result;
}

// Native _CFBundleGrokBinaryType, iOS Simulator 24A434, 0x1c025d700. Exported natively
// (_CFBundleGrokBinaryType, spelled _OCFBundleGrokBinaryType). The default is 6 for a URL and 7 for NULL
// (CINC). _CFBundleGrokFileType is asked for the mach type only (the other outputs NULL). When it finds an
// extension, mach type 8 gives 3, 0x1000 gives 1, 2 gives 2, 6 gives 4 (signed comparisons with 7 as in the
// native CSEL chain); any other keeps the default.
UInt32 _OCFBundleGrokBinaryType(OCFURLRef executableURL) {
    UInt32 machtype = 0;
    UInt32 result = executableURL != NULL ? 6 : 7;
    if (_CFBundleGrokFileType(executableURL, NULL, NULL, &machtype, NULL, NULL, NULL, NULL, NULL)) {
        if ((int32_t)machtype > 7) {
            result = machtype == 8 ? 3 : (machtype == 0x1000 ? 1 : result);
        } else {
            result = machtype == 2 ? 2 : (machtype == 6 ? 4 : result);
        }
    }
    return result;
}

// Native unnamed table of _CFBundleGrokFileTypeForZipFile, iOS Simulator 24A434, 0x1c0456de5 (six
// five-byte NUL-padded extensions, 30 bytes, after _CFBundleGrokX11FromFile.libX11name), UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1809654e5, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// 30 bytes agree. The native code returns the table address plus 0, 5, 10, 15, 20 or 25.
// kSunExtensions is a local descriptive name.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
static const char kSunExtensions[6][5] = {"sxc", "sxd", "sxg", "sxi", "sxm", "sxw"};

// Native unnamed table of _CFBundleGrokFileTypeForZipFile, iOS Simulator 24A434, 0x1c0456e04 (nine
// five-byte NUL-padded extensions, 45 bytes), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified. Non-external.
// iOS Device 24A446: 0x180965504, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// 45 bytes agree. The native code returns the table address plus 0, 5, ..., 40.
// kOpenDocumentExtensions is a local descriptive name.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
static const char kOpenDocumentExtensions[9][5] = {
    "odc",
    "odf",
    "odg",
    "oth",
    "odi",
    "odm",
    "odp",
    "ods",
    "odt",
};

// Native _CFBundleGrokFileTypeForZipFile, iOS Simulator 24A434, 0x1c025d7c4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077acd4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokFileTypeForZipFile at 0x18077acd4 keeps the META-INF/MANIFEST.MF
// flag in a stack slot instead of a register, indexes the signature bytes from the buffer base and
// places blocks differently; every comparison, bound, call, argument and result agrees.
// The only caller, _CFBundleGrokFileType at 0x1c025c96c, passes the descriptor, the header buffer and its
// length; the code uses fileLength (signed) only as the end of the trailing 1,024-byte window. The first
// pass scans `bytes` for local ("PK\3\4", name length at +26, 30-byte header) and central ("PK\1\2",
// +28, 46 bytes) entries whose name fits; a local "mimetype" entry may decide the result. Otherwise the
// trailing window is scanned the same way (without the mimetype rule) and the recorded names decide.
// Comparisons are strncasecmp_l with a NULL locale. Native pointer comparisons are expressed with
// uintptr_t; they are the same comparisons.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
const char *_CFBundleGrokFileTypeForZipFile(int fd, const void *bytes, size_t length, off_t fileLength) {
    const uint8_t *buffer = (const uint8_t *)bytes;
    const char *result = "zip";
    int hasMetaInf = 0;
    int hasContentXML = 0;
    int hasManifestXML = 0;
    int hasPresentationXML = 0;
    int hasSpreadsheetXML = 0;
    int hasSMIL = 0;
    int hasOPF = 0;
    int hasDocumentXML = 0;
    int hasRels = 0;
    int hasContentTypes = 0;
    int hasManifestMF = 0;
    int keepScanning = 1;
    if (length >= 31) {
        const uintptr_t bufferEnd = (uintptr_t)buffer + length;
        const OCFIndex scanLimit = (OCFIndex)(length - 30);
        OCFIndex offset = 0;
        do {
            const uint8_t *entry = buffer + offset;
            uint32_t headerLength = 0;
            uint32_t lengthField = 0;
            int isEntry = 0;
            int isLocal = 0;
            if (entry[0] == 'P' && entry[1] == 'K') {
                if (entry[2] == 3) {
                    if (entry[3] == 4) {
                        isEntry = 1;
                        isLocal = 1;
                        headerLength = 30;
                        lengthField = 26;
                    }
                } else if (entry[2] == 1 && entry[3] == 2) {
                    isEntry = 1;
                    headerLength = 46;
                    lengthField = 28;
                }
            }
            keepScanning = 1;
            if (isEntry) {
                uint16_t nameLength;
                memcpy(&nameLength, entry + lengthField, sizeof(nameLength));
                if (offset + (OCFIndex)headerLength + (OCFIndex)nameLength <= (OCFIndex)length) {
                    const char *name = (const char *)entry + headerLength;
                    if (isLocal && nameLength == 8 && strncasecmp_l(name, "mimetype", 8, NULL) == 0) {
                        uint16_t fileNameLength;
                        uint16_t extraLength;
                        uint16_t method;
                        memcpy(&fileNameLength, entry + 26, sizeof(fileNameLength));
                        memcpy(&extraLength, entry + 28, sizeof(extraLength));
                        memcpy(&method, entry + 8, sizeof(method));
                        const uintptr_t content = (uintptr_t)entry + fileNameLength + extraLength + 30;
                        const char *contentType = (const char *)content;
                        if (content + 56 <= bufferEnd && method == 0 &&
                            (strncasecmp_l(contentType, "application/vnd.", 16, NULL) == 0 ||
                             strncasecmp_l(contentType, "application/x-vnd.", 18, NULL) == 0)) {
                            const char *kind = contentType + (contentType[15] == '.' ? 16 : 18);
                            if (strncasecmp_l(kind, "sun.xml.", 8, NULL) == 0) {
                                const char *application = kind + 8;
                                int index = -1;
                                if (strncasecmp_l(application, "calc", 4, NULL) == 0) {
                                    index = 0;
                                } else if (strncasecmp_l(application, "draw", 4, NULL) == 0) {
                                    index = 1;
                                } else if (strncasecmp_l(application, "writer.global", 13, NULL) == 0) {
                                    index = 2;
                                } else if (strncasecmp_l(application, "impress", 7, NULL) == 0) {
                                    index = 3;
                                } else if (strncasecmp_l(application, "math", 4, NULL) == 0) {
                                    index = 4;
                                } else if (strncasecmp_l(application, "writer", 6, NULL) == 0) {
                                    index = 5;
                                }
                                if (index >= 0) {
                                    result = kSunExtensions[index];
                                    keepScanning = 0;
                                }
                            } else if (strncasecmp_l(kind, "oasis.opendocument.", 19, NULL) == 0) {
                                const char *application = kind + 19;
                                int index = -1;
                                if (strncasecmp_l(application, "chart", 5, NULL) == 0) {
                                    index = 0;
                                } else if (strncasecmp_l(application, "formula", 7, NULL) == 0) {
                                    index = 1;
                                } else if (strncasecmp_l(application, "graphics", 8, NULL) == 0) {
                                    index = 2;
                                } else if (strncasecmp_l(application, "text-web", 8, NULL) == 0) {
                                    index = 3;
                                } else if (strncasecmp_l(application, "image", 5, NULL) == 0) {
                                    index = 4;
                                } else if (strncasecmp_l(application, "text-master", 11, NULL) == 0) {
                                    index = 5;
                                } else if (strncasecmp_l(application, "presentation", 12, NULL) == 0) {
                                    index = 6;
                                } else if (strncasecmp_l(application, "spreadsheet", 11, NULL) == 0) {
                                    index = 7;
                                } else if (strncasecmp_l(application, "text", 4, NULL) == 0) {
                                    index = 8;
                                }
                                if (index >= 0) {
                                    result = kOpenDocumentExtensions[index];
                                    keepScanning = 0;
                                }
                            }
                        } else {
                            uint32_t firstWord = 0;
                            uint32_t secondWord = 0;
                            int hasMagic = 0;
                            if (content + 41 <= bufferEnd && method == 8) {
                                memcpy(&firstWord, contentType, sizeof(firstWord));
                                if (firstWord == UINT32_C(0xc8282c4b)) {
                                    memcpy(&secondWord, contentType + 4, sizeof(secondWord));
                                    hasMagic = secondWord == UINT32_C(0x2c4e4cc9);
                                }
                            }
                            if (hasMagic) {
                                result = "odt";
                            } else if (
                                content + 29 <= bufferEnd &&
                                strncasecmp_l(contentType, "application/oebps-package+xml", 29, NULL) == 0
                            ) {
                                result = "epub";
                                keepScanning = 0;
                            } else if (
                                content + 20 <= bufferEnd &&
                                strncasecmp_l(contentType, "application/epub+zip", 20, NULL) == 0
                            ) {
                                result = "epub";
                                keepScanning = 0;
                            }
                        }
                    } else {
                        int checkSuffixes = 0;
                        switch (nameLength) {
                            case 9:
                                if (strncasecmp_l(name, "META-INF/", 9, NULL) == 0) {
                                    hasMetaInf = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 11:
                                if (strncasecmp_l(name, "content.xml", 11, NULL) == 0) {
                                    hasContentXML = 1;
                                } else if (strncasecmp_l(name, "_rels/.rels", 11, NULL) == 0) {
                                    hasRels = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 19:
                                if (strncasecmp_l(name, "[Content_Types].xml", 19, NULL) == 0) {
                                    hasContentTypes = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 20:
                                if (strncasecmp_l(name, "META-INF/MANIFEST.MF", 20, NULL) == 0) {
                                    hasManifestMF = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 21:
                                if (strncasecmp_l(name, "META-INF/manifest.xml", 21, NULL) == 0) {
                                    hasManifestXML = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            default:
                                checkSuffixes = nameLength >= 5;
                                break;
                        }
                        if (checkSuffixes) {
                            const char *nameEnd = name + nameLength;
                            if (strncasecmp_l(nameEnd - 4, ".opf", 4, NULL) == 0) {
                                hasOPF = 1;
                            } else if (strncasecmp_l(nameEnd - 4, ".sml", 4, NULL) == 0) {
                                hasSMIL = 1;
                            } else if (nameLength >= 6) {
                                if (strncasecmp_l(nameEnd - 5, ".smil", 5, NULL) == 0) {
                                    hasSMIL = 1;
                                } else if (nameLength >= 8) {
                                    if (strncasecmp_l(name, "xl/", 3, NULL) == 0 &&
                                        strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                        hasSpreadsheetXML = 1;
                                    } else if (nameLength != 8) {
                                        if (strncasecmp_l(name, "ppt/", 4, NULL) == 0 &&
                                            strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                            hasPresentationXML = 1;
                                        } else if (nameLength >= 10) {
                                            if (strncasecmp_l(name, "word/", 5, NULL) == 0 &&
                                                strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                hasDocumentXML = 1;
                                            } else if (nameLength != 10) {
                                                if (strncasecmp_l(name, "excel/", 6, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                    hasSpreadsheetXML = 1;
                                                } else if (
                                                    nameLength >= 16 &&
                                                    strncasecmp_l(name, "powerpoint/", 11, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0
                                                ) {
                                                    hasPresentationXML = 1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    offset += (uint32_t)(headerLength + nameLength - 1);
                }
            }
            if (!keepScanning) {
                break;
            }
            ++offset;
        } while (offset < scanLimit);
    }
    if (keepScanning) {
        uint8_t *allocated = NULL;
        const uint8_t *window = NULL;
        if (fileLength >= 1024) {
            const off_t windowOffset = fileLength - 1024;
            if (fd >= 0 && lseek(fd, windowOffset, SEEK_SET) == windowOffset) {
                allocated =
                    (uint8_t *)malloc_type_malloc(1024, (malloc_type_id_t)UINT64_C(0x100004077774924));
                if (allocated != NULL && read(fd, allocated, 1024) > 1023) {
                    window = allocated;
                }
            } else if (length >= 1024) {
                window = buffer + length - 1024;
            }
        }
        if (window != NULL) {
            OCFIndex offset = 0;
            do {
                const uint8_t *entry = window + offset;
                uint32_t headerLength = 0;
                uint32_t lengthField = 0;
                int isEntry = 0;
                if (entry[0] == 'P' && entry[1] == 'K') {
                    if (entry[2] == 3) {
                        if (entry[3] == 4) {
                            isEntry = 1;
                            headerLength = 30;
                            lengthField = 26;
                        }
                    } else if (entry[2] == 1 && entry[3] == 2) {
                        isEntry = 1;
                        headerLength = 46;
                        lengthField = 28;
                    }
                }
                if (isEntry) {
                    uint16_t nameLength;
                    memcpy(&nameLength, entry + lengthField, sizeof(nameLength));
                    if (offset + (OCFIndex)headerLength + (OCFIndex)nameLength <= 1024) {
                        const char *name = (const char *)entry + headerLength;
                        int checkSuffixes = 0;
                        switch (nameLength) {
                            case 9:
                                if (strncasecmp_l(name, "META-INF/", 9, NULL) == 0) {
                                    hasMetaInf = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 11:
                                if (strncasecmp_l(name, "content.xml", 11, NULL) == 0) {
                                    hasContentXML = 1;
                                } else if (strncasecmp_l(name, "_rels/.rels", 11, NULL) == 0) {
                                    hasRels = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 19:
                                if (strncasecmp_l(name, "[Content_Types].xml", 19, NULL) == 0) {
                                    hasContentTypes = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 20:
                                if (strncasecmp_l(name, "META-INF/MANIFEST.MF", 20, NULL) == 0) {
                                    hasManifestMF = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 21:
                                if (strncasecmp_l(name, "META-INF/manifest.xml", 21, NULL) == 0) {
                                    hasManifestXML = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            default:
                                checkSuffixes = nameLength >= 5;
                                break;
                        }
                        if (checkSuffixes) {
                            const char *nameEnd = name + nameLength;
                            if (strncasecmp_l(nameEnd - 4, ".opf", 4, NULL) == 0) {
                                hasOPF = 1;
                            } else if (strncasecmp_l(nameEnd - 4, ".sml", 4, NULL) == 0) {
                                hasSMIL = 1;
                            } else if (nameLength >= 6) {
                                if (strncasecmp_l(nameEnd - 5, ".smil", 5, NULL) == 0) {
                                    hasSMIL = 1;
                                } else if (nameLength >= 8) {
                                    if (strncasecmp_l(name, "xl/", 3, NULL) == 0 &&
                                        strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                        hasSpreadsheetXML = 1;
                                    } else if (nameLength != 8) {
                                        if (strncasecmp_l(name, "ppt/", 4, NULL) == 0 &&
                                            strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                            hasPresentationXML = 1;
                                        } else if (nameLength >= 10) {
                                            if (strncasecmp_l(name, "word/", 5, NULL) == 0 &&
                                                strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                hasDocumentXML = 1;
                                            } else if (nameLength != 10) {
                                                if (strncasecmp_l(name, "excel/", 6, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                    hasSpreadsheetXML = 1;
                                                } else if (
                                                    nameLength >= 16 &&
                                                    strncasecmp_l(name, "powerpoint/", 11, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0
                                                ) {
                                                    hasPresentationXML = 1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        offset += (uint32_t)(headerLength + nameLength - 1);
                    }
                }
            } while (offset++ < 993);
        }
        const int hasPackageParts = hasRels != 0 || hasContentTypes != 0;
        if (hasManifestMF) {
            result = "jar";
        } else if (hasPackageParts && hasDocumentXML) {
            result = "docx";
        } else if (hasSpreadsheetXML && hasPackageParts) {
            result = "xlsx";
        } else if ((uint8_t)hasPresentationXML != 0 && hasPackageParts) {
            result = "pptx";
        } else if (hasContentXML || hasManifestXML) {
            result = "odt";
        } else if (hasMetaInf) {
            result = "jar";
        } else if (hasOPF && hasSMIL) {
            result = "dtb";
        } else if (hasOPF) {
            result = "oeb";
        }
        if (allocated != NULL) {
            free(allocated);
        }
    }
    return result;
}

// Native _CFBundleGrokFileTypeForOLEFile, iOS Simulator 24A434, 0x1c025e57c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077bb0c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokFileTypeForOLEFile at 0x18077bb0c forms the entry address with
// BFI and counts the compared characters from one; every read, comparison and result agrees.
// The only caller, _CFBundleGrokFileType at 0x1c025c7b8, passes ((uint32_t)word at header +0x30 << 9)
// + 512 as offset. The 512 bytes at offset are read as four 128-byte entries. The byte at +64 of an entry,
// as a signed char halved toward zero, selects a name: 5 "Book" and 9 "Workbook" ("xls"), 13
// "WordDocument" ("doc"), 20 "PowerPoint Document" ("ppt"). That many characters minus one are compared
// with the low bytes of the entry's 16-bit units; the first matching entry decides, otherwise "ole".
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
const char *_CFBundleGrokFileTypeForOLEFile(int fd, const void *bytes, size_t length, off_t offset) {
    const char *result = "ole";
    uint8_t *allocated = NULL;
    const uint8_t *directory = NULL;
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == offset) {
        allocated = (uint8_t *)malloc_type_malloc(512, (malloc_type_id_t)UINT64_C(0x100004077774924));
        if (allocated == NULL) {
            return result;
        }
        if (read(fd, allocated, 512) > 511) {
            directory = allocated;
        }
    } else if (length >= (size_t)(offset + 512)) {
        directory = (const uint8_t *)bytes + offset;
    }
    if (directory != NULL) {
        for (uint64_t index = 0; index < 4; ++index) {
            const uint8_t *entry = directory + 128 * index;
            const int characters = (int8_t)entry[64] / 2;
            const char *name;
            const char *type;
            switch (characters) {
                case 5:
                    name = "Book";
                    type = "xls";
                    break;
                case 9:
                    name = "Workbook";
                    type = "xls";
                    break;
                case 13:
                    name = "WordDocument";
                    type = "doc";
                    break;
                case 20:
                    name = "PowerPoint Document";
                    type = "ppt";
                    break;
                default:
                    continue;
            }
            const uint64_t count = (uint32_t)(characters - 1);
            int equal = 1;
            for (uint64_t character = 0; character < count; ++character) {
                if (entry[character * 2] != (uint8_t)name[character]) {
                    equal = 0;
                    break;
                }
            }
            if (equal) {
                result = type;
                break;
            }
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    return result;
}

// Native _CFBundleGrokX11FromFile, iOS Simulator 24A434, 0x1c025e7cc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077bd98, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokX11FromFile at 0x18077bd98 compares load-command offsets instead
// of addresses and advances the offset before testing the result; every read, bound, call and result
// agrees.
// Callers: _CFBundleGrokFileType (offset 0, four swapped/sixtyFour pairs) and
// _CFBundleGrokMachTypeForFatFile (a 32-bit slice offset). A 4,096-byte window at offset holds a Mach-O
// header; ncmds (+16) and sizeofcmds (+20) are byte-swapped when `swapped`. Load commands start at 32
// (sixtyFour) or 28 and end at min(start + sizeofcmds, 4096). The result is 1 when an LC_LOAD_DYLIB
// (12) command's name, at its 32-bit offset +8 and with 20 bytes inside the end, begins with the 19
// characters of libX11name. Native pointer walks are expressed as offsets from the header; the
// comparisons are the same.
// CoreFoundation defect: _CFBundleGrokX11FromFile at 0x1c025e7cc (iOS Device 0x18077bd98; confidence:
// verified) checks only that a load command starts before the end, then reads its 32-bit cmd (+0), name
// offset (+8, LC_LOAD_DYLIB only) and cmdsize (+4); a command within 11 bytes of a 4,096-byte end reads
// past the heap window or the caller's bytes. The bytes-path check (uint32_t)(offset + 4096) <= length
// also wraps, so an offset above 0xfffff000 reads the window outside the caller's bytes. The restoration
// never reads outside the window or `bytes`: a field that does not fit ends the scan with result 0, and a
// window that does not fit returns 0. Every in-range result is unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
uint8_t _CFBundleGrokX11FromFile(
    int fd, const void *bytes, size_t length, uint32_t offset, uint8_t swapped, uint8_t sixtyFour
) {
    // Native _CFBundleGrokX11FromFile.libX11name, iOS Simulator 24A434, 0x1c0456dd1 (20 bytes with the
    // NUL; the sizeof bound of the name check), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
    // verified. Non-external. iOS Device 24A446: 0x1809654d1, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
    // confidence: verified; the 20 bytes agree.
    static const char libX11name[] = "/usr/X11R6/lib/libX";
    uint8_t *allocated = NULL;
    const uint8_t *header;
    uint64_t readable;
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == (off_t)offset) {
        allocated = (uint8_t *)malloc_type_malloc(4096, (malloc_type_id_t)UINT64_C(0xe7329b55));
        if (allocated == NULL) {
            return 0;
        }
        if (read(fd, allocated, 4096) <= 4095) {
            free(allocated);
            return 0;
        }
        header = allocated;
        readable = 4096;
    } else {
        if (length < (uint32_t)(offset + 4096)) {
            return 0;
        }
        // CoreFoundation defect: the wrapped window check above is kept; a window outside `bytes` is
        // not read.
        if ((uint64_t)offset + 4096 > length) {
            return 0;
        }
        header = (const uint8_t *)bytes + offset;
        readable = length - offset;
    }
    uint8_t result = 0;
    uint32_t commandCount;
    uint32_t commandsSize;
    memcpy(&commandCount, header + 16, sizeof(commandCount));
    memcpy(&commandsSize, header + 20, sizeof(commandsSize));
    if (swapped) {
        commandCount = __builtin_bswap32(commandCount);
        commandsSize = __builtin_bswap32(commandsSize);
    }
    const uint64_t headerSize = sixtyFour ? 32 : 28;
    const uint64_t endOffset = commandsSize > 4096 - headerSize ? 4096 : headerSize + commandsSize;
    if (commandCount != 0 && headerSize < endOffset) {
        uint64_t commandOffset = headerSize;
        uint32_t index = 1;
        for (;;) {
            // CoreFoundation defect: each load-command field must lie inside the readable bytes.
            if (readable - commandOffset < 4) {
                result = 0;
                break;
            }
            uint32_t command;
            memcpy(&command, header + commandOffset, sizeof(command));
            if (swapped) {
                command = __builtin_bswap32(command);
            }
            result = 0;
            if (command == 12) {
                if (readable - commandOffset < 12) {
                    break;
                }
                uint32_t nameOffset;
                memcpy(&nameOffset, header + commandOffset + 8, sizeof(nameOffset));
                if (swapped) {
                    nameOffset = __builtin_bswap32(nameOffset);
                }
                if (commandOffset + nameOffset + sizeof(libX11name) <= endOffset) {
                    result = strncmp(
                                 (const char *)header + commandOffset + nameOffset,
                                 libX11name,
                                 sizeof(libX11name) - 1
                             ) == 0;
                }
            }
            if (readable - commandOffset < 8) {
                result = 0;
                break;
            }
            uint32_t commandSize;
            memcpy(&commandSize, header + commandOffset + 4, sizeof(commandSize));
            if (swapped) {
                commandSize = __builtin_bswap32(commandSize);
            }
            if (result || index >= commandCount) {
                break;
            }
            commandOffset += commandSize;
            ++index;
            if (commandOffset >= endOffset) {
                break;
            }
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    return result;
}

// CoreFoundation defect support (local infrastructure helper): the native strncmp(name, literal, 16) over a
// name of which only `available` bytes are inside the buffer. A difference or the literal's NUL within those
// bytes decides as natively; otherwise the comparison counts as unequal.
static int compareReadableName(const uint8_t *name, uint64_t available, const char *literal) {
    if (available >= 16) {
        return strncmp((const char *)name, literal, 16);
    }
    const int difference = strncmp((const char *)name, literal, available);
    if (difference == 0 && available <= strlen(literal)) {
        return 1;
    }
    return difference;
}

// Native _CFBundleCreateInfoDictFromFile, iOS Simulator 24A434, 0x1c025ea74. Non-external.
// A descriptor that fstat accepts is mapped whole (mmap PROT_READ, flags 0x2002: MAP_PRIVATE |
// MAP_RESILIENT_CODESIGN); the mapping and st_size then replace bytes and length. A failed mapping is
// remembered as -1 and the caller's bytes are used; whatever was recorded (also -1) is passed to munmap with
// st_size before return. The Mach-O header is at `offset` (32-bit, zero-extended) and needs more than 32
// bytes. ncmds (+16) and sizeofcmds (+20) are byte-swapped when `swapped`; commands start at 32 (sixtyFour)
// or 28 and end at the smaller of start + sizeofcmds and the buffer end. In LC_SEGMENT_64 (0x19; nsects at
// +64, 80-byte sections from +72) or LC_SEGMENT (1; +48, 68-byte sections from +56) commands, a section
// named "__info_plist" (strncmp, 16 bytes, first) in segment "__TEXT" (16 bytes) stops the scan: its size
// (the 64-bit field at +40, which must fit in 32 bits, or +36) and 32-bit offset (+48, or +40) from the
// header, when inside the buffer, go to _CFBundleCreateInfoDictFromData. Sections advance while nsects and
// the end allow (each next start before the end); commands advance by cmdsize while ncmds, an offset of at
// least the header size and the end allow.
// CoreFoundation defect: _CFBundleCreateInfoDictFromFile at 0x1c025ea74 (iOS Device: unread; confidence:
// verified for the iOS Simulator) checks only that a load command or section starts before the end, then
// reads cmd and cmdsize (+0, +4), nsects (+64 or +48), both 16-byte names and the size and offset fields (up
// to 52 bytes past a section start) beyond the mapping or the caller's bytes. The restoration never reads
// outside the buffer: a name comparison that the readable bytes cannot decide counts as unequal, and any
// other field that does not fit ends the scan with no dictionary. Within a mapping, the native code reads the
// zero fill past st_size on the last page; the restoration treats those bytes as outside the buffer.
OCFMutableDictionaryRef _CFBundleCreateInfoDictFromFile(
    int fd, const void *bytes, size_t length, uint32_t offset, uint8_t swapped, uint8_t sixtyFour
) {
    struct stat statBuffer;
    memset(&statBuffer, 0, sizeof(statBuffer));
    void *mapped = NULL;
    const uint8_t *buffer = (const uint8_t *)bytes;
    if (fd >= 0 && fstat(fd, &statBuffer) == 0) {
        void *mapping = mmap(NULL, (size_t)statBuffer.st_size, PROT_READ, 0x2002, fd, 0);
        if (mapping == MAP_FAILED) {
            mapped = MAP_FAILED;
        } else {
            length = (size_t)statBuffer.st_size;
            mapped = mapping;
            buffer = (const uint8_t *)mapping;
        }
    }
    OCFMutableDictionaryRef result = NULL;
    if (length > (uint64_t)offset + 32) {
        const uint8_t *header = buffer + offset;
        const uint8_t *fileEnd = buffer + length;
        const uint64_t readable = length - offset;
        uint32_t commandCount = loadUInt32(header + 16);
        uint32_t commandsSize = loadUInt32(header + 20);
        if (swapped) {
            commandCount = __builtin_bswap32(commandCount);
            commandsSize = __builtin_bswap32(commandsSize);
        }
        const uint64_t headerSize = sixtyFour ? 32 : 28;
        const uint32_t segmentCommand = sixtyFour ? 0x19 : 1;
        const uint64_t sectionCountOffset = sixtyFour ? 64 : 48;
        const uint64_t sectionsOffset = sixtyFour ? 72 : 56;
        const uint64_t sectionStride = sixtyFour ? 80 : 68;
        const uint64_t sizeFieldOffset = sixtyFour ? 40 : 36;
        const uint64_t offsetFieldOffset = sixtyFour ? 48 : 40;
        const uint8_t *commandsEnd = header + headerSize + commandsSize;
        const uint8_t *end = commandsEnd > fileEnd ? fileEnd : commandsEnd;
        // Native pointer comparisons are expressed as offsets from the header; they are the same comparisons.
        const uint64_t endOffset = (uint64_t)(end - header);
        if (commandCount != 0 && headerSize < endOffset) {
            uint64_t commandOffset = headerSize;
            uint32_t index = 0;
            for (;;) {
                // CoreFoundation defect: each field read below must lie inside the buffer.
                if (readable - commandOffset < 4) {
                    break;
                }
                uint32_t command = loadUInt32(header + commandOffset);
                if (swapped) {
                    command = __builtin_bswap32(command);
                }
                int keepScanning = 1;
                if (command == segmentCommand) {
                    if (readable - commandOffset < sectionCountOffset + 4) {
                        break;
                    }
                    uint32_t sectionCount = loadUInt32(header + commandOffset + sectionCountOffset);
                    if (swapped) {
                        sectionCount = __builtin_bswap32(sectionCount);
                    }
                    if (sectionCount != 0 && commandOffset + sectionsOffset < endOffset) {
                        uint64_t sectionStart = commandOffset + sectionsOffset;
                        uint32_t sectionIndex = 1;
                        for (;;) {
                            const uint64_t available = readable - sectionStart;
                            const uint8_t *section = header + sectionStart;
                            if (compareReadableName(section, available, "__info_plist") == 0 &&
                                compareReadableName(
                                    section + 16, available > 16 ? available - 16 : 0, "__TEXT"
                                ) == 0) {
                                if (available < offsetFieldOffset + 4) {
                                    goto unmap;
                                }
                                uint64_t size;
                                uint32_t dataOffset;
                                if (sixtyFour) {
                                    size = loadUInt32(section + sizeFieldOffset) |
                                           (uint64_t)loadUInt32(section + sizeFieldOffset + 4) << 32;
                                    dataOffset = loadUInt32(section + offsetFieldOffset);
                                    if (swapped) {
                                        size = __builtin_bswap64(size);
                                        dataOffset = __builtin_bswap32(dataOffset);
                                    }
                                } else {
                                    size = loadUInt32(section + sizeFieldOffset);
                                    dataOffset = loadUInt32(section + offsetFieldOffset);
                                    if (swapped) {
                                        size = __builtin_bswap32((uint32_t)size);
                                        dataOffset = __builtin_bswap32(dataOffset);
                                    }
                                }
                                keepScanning = 0;
                                if ((size >> 32) == 0) {
                                    if ((uint64_t)dataOffset + size <= readable) {
                                        result = _CFBundleCreateInfoDictFromData(
                                            header + dataOffset, (uint32_t)size
                                        );
                                    }
                                }
                                break;
                            }
                            if (sectionIndex >= sectionCount) {
                                break;
                            }
                            ++sectionIndex;
                            sectionStart += sectionStride;
                            if (sectionStart >= endOffset) {
                                break;
                            }
                        }
                    }
                }
                if (readable - commandOffset < 8) {
                    break;
                }
                uint32_t commandSize = loadUInt32(header + commandOffset + 4);
                if (swapped) {
                    commandSize = __builtin_bswap32(commandSize);
                }
                if (!keepScanning) {
                    break;
                }
                if (++index >= commandCount) {
                    break;
                }
                commandOffset += commandSize;
                if (commandOffset < headerSize || commandOffset >= endOffset) {
                    break;
                }
            }
        }
    }
unmap:
    if (mapped != NULL) {
        munmap(mapped, (size_t)statBuffer.st_size);
    }
    return result;
}

// Native _CFBundleGrokObjcImageInfoFromFile, iOS Simulator 24A434, 0x1c025eedc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077c060, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokObjcImageInfoFromFile at 0x18077c060 compares load-command and
// section offsets instead of addresses and keeps more state in stack slots; every read, bound, call,
// store and result agrees.
// Callers: _CFBundleGrokFileType (offset 0) and _CFBundleGrokMachTypeForFatFile (a slice offset, not
// swapped, 64-bit). The Mach-O window is read as in _CFBundleGrokX11FromFile. In LC_SEGMENT_64 (0x19;
// nsects at +64, 80-byte sections from +72) or LC_SEGMENT (1; +48, 68-byte sections from +56) commands,
// each section's segment name (+16) is compared with "__DATA" or "__OBJC" and its name (+0) with
// "__objc_imageinfo" or "__image_info", 16 bytes each with strncmp, segment name first. A matching
// segment name sets *segmentFound; a section matching both stops the scan with its size (the low 32 bits
// of the 64-bit field at +40, or +36) and 32-bit offset (+48, or +40). With a size of at least 8, the two
// 32-bit words at offset + section offset (32-bit sum) are read from the file, or else from `bytes` when
// (uint32_t)(offset + section offset + 8) <= length; otherwise both words are 0. Every non-NULL output
// is stored after the window is freed, also on failure. Native pointer walks are expressed as offsets
// from the header; the comparisons are the same.
// CoreFoundation defect: _CFBundleGrokObjcImageInfoFromFile at 0x1c025eedc (iOS Device 0x18077c060;
// confidence: verified) checks only that a load command or section starts before the end. It then reads
// cmd, nsects (+64 or +48) and cmdsize, both 16-byte names and, on a match, the size and offset fields,
// up to 52 bytes past the start, outside the 4,096-byte heap window or the caller's bytes. The window
// check (uint32_t)(offset + 4096) <= length and the image-information check
// (uint32_t)(offset + section offset + 8) <= length wrap, and the bytes are then read at the unwrapped
// 64-bit address. The restoration never reads outside the window or `bytes`. A name comparison that the
// readable bytes cannot decide counts as unequal; any other field that does not fit ends the scan as if
// no section matched; a window or image-information words that do not fit are treated as failed checks.
// Every in-range result is unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
void _CFBundleGrokObjcImageInfoFromFile(
    int fd,
    const void *bytes,
    size_t length,
    uint32_t offset,
    uint8_t swapped,
    uint8_t sixtyFour,
    uint8_t *segmentFound,
    uint32_t *firstInfoWord,
    uint32_t *secondInfoWord
) {
    uint8_t foundSegment = 0;
    uint32_t firstWord = 0;
    uint32_t secondWord = 0;
    uint8_t *allocated = NULL;
    const uint8_t *header = NULL;
    uint64_t readable = 0;
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == (off_t)offset) {
        allocated = (uint8_t *)malloc_type_malloc(4096, (malloc_type_id_t)UINT64_C(0x298ae5f0));
        if (allocated != NULL && read(fd, allocated, 4096) > 4095) {
            header = allocated;
            readable = 4096;
        }
    } else if (length >= (uint32_t)(offset + 4096) && (uint64_t)offset + 4096 <= length) {
        // CoreFoundation defect: the second condition keeps the wrapped window check from reading
        // outside `bytes`.
        header = (const uint8_t *)bytes + offset;
        readable = length - offset;
    }
    if (header != NULL) {
        uint32_t commandCount;
        uint32_t commandsSize;
        memcpy(&commandCount, header + 16, sizeof(commandCount));
        memcpy(&commandsSize, header + 20, sizeof(commandsSize));
        if (swapped) {
            commandCount = __builtin_bswap32(commandCount);
            commandsSize = __builtin_bswap32(commandsSize);
        }
        const uint64_t headerSize = sixtyFour ? 32 : 28;
        const uint32_t segmentCommand = sixtyFour ? 0x19 : 1;
        const uint64_t sectionCountOffset = sixtyFour ? 64 : 48;
        const uint64_t sectionsOffset = sixtyFour ? 72 : 56;
        const uint64_t sectionStride = sixtyFour ? 80 : 68;
        const uint64_t sizeFieldOffset = sixtyFour ? 40 : 36;
        const uint64_t offsetFieldOffset = sixtyFour ? 48 : 40;
        const char *segmentName = sixtyFour ? "__DATA" : "__OBJC";
        const char *sectionName = sixtyFour ? "__objc_imageinfo" : "__image_info";
        const uint64_t segmentNameLength = 6;
        const uint64_t sectionNameLength = sixtyFour ? 16 : 12;
        const uint64_t endOffset = commandsSize > 4096 - headerSize ? 4096 : headerSize + commandsSize;
        uint32_t sectionLength = 0;
        uint32_t sectionOffset = 0;
        if (commandCount != 0 && headerSize < endOffset) {
            uint64_t commandOffset = headerSize;
            uint32_t index = 0;
            for (;;) {
                int keepScanning = 1;
                // CoreFoundation defect: each field read below must lie inside the readable bytes.
                if (readable - commandOffset < 4) {
                    break;
                }
                uint32_t command;
                memcpy(&command, header + commandOffset, sizeof(command));
                if (swapped) {
                    command = __builtin_bswap32(command);
                }
                if (command == segmentCommand) {
                    if (readable - commandOffset < sectionCountOffset + 4) {
                        break;
                    }
                    uint32_t sectionCount;
                    memcpy(&sectionCount, header + commandOffset + sectionCountOffset, sizeof(sectionCount));
                    if (swapped) {
                        sectionCount = __builtin_bswap32(sectionCount);
                    }
                    if (sectionCount != 0 && commandOffset + sectionsOffset < endOffset) {
                        uint64_t sectionStart = commandOffset + sectionsOffset;
                        uint32_t sectionIndex = 1;
                        for (;;) {
                            const uint64_t available = readable - sectionStart;
                            const char *section = (const char *)header + sectionStart;
                            int segmentDifference = 1;
                            if (available >= 32) {
                                segmentDifference = strncmp(section + 16, segmentName, 16);
                            } else if (available > 16) {
                                // CoreFoundation defect: an undecided comparison counts as unequal.
                                segmentDifference = strncmp(section + 16, segmentName, available - 16);
                                if (segmentDifference == 0 && available - 16 <= segmentNameLength) {
                                    segmentDifference = 1;
                                }
                            }
                            if (segmentDifference == 0) {
                                foundSegment = 1;
                            }
                            int sectionDifference;
                            if (available >= 16) {
                                sectionDifference = strncmp(section, sectionName, 16);
                            } else {
                                // CoreFoundation defect: an undecided comparison counts as unequal.
                                sectionDifference = strncmp(section, sectionName, available);
                                if (sectionDifference == 0 && available <= sectionNameLength) {
                                    sectionDifference = 1;
                                }
                            }
                            if ((sectionDifference | segmentDifference) == 0) {
                                if (available < offsetFieldOffset + 4) {
                                    goto scanned;
                                }
                                if (sixtyFour) {
                                    uint64_t size;
                                    memcpy(&size, section + sizeFieldOffset, sizeof(size));
                                    if (swapped) {
                                        size = __builtin_bswap64(size);
                                    }
                                    sectionLength = (uint32_t)size;
                                } else {
                                    memcpy(&sectionLength, section + sizeFieldOffset, sizeof(sectionLength));
                                    if (swapped) {
                                        sectionLength = __builtin_bswap32(sectionLength);
                                    }
                                }
                                memcpy(&sectionOffset, section + offsetFieldOffset, sizeof(sectionOffset));
                                if (swapped) {
                                    sectionOffset = __builtin_bswap32(sectionOffset);
                                }
                                keepScanning = 0;
                                break;
                            }
                            if (sectionIndex >= sectionCount) {
                                break;
                            }
                            ++sectionIndex;
                            sectionStart += sectionStride;
                            if (sectionStart >= endOffset) {
                                break;
                            }
                        }
                    }
                }
                if (readable - commandOffset < 8) {
                    break;
                }
                uint32_t commandSize;
                memcpy(&commandSize, header + commandOffset + 4, sizeof(commandSize));
                if (swapped) {
                    commandSize = __builtin_bswap32(commandSize);
                }
                if (!keepScanning) {
                    break;
                }
                if (++index >= commandCount) {
                    break;
                }
                commandOffset += commandSize;
                if (commandOffset < headerSize || commandOffset >= endOffset) {
                    break;
                }
            }
        }
    scanned:
        if (sectionLength >= 8) {
            const uint32_t position = offset + sectionOffset;
            uint32_t words[2];
            if (fd >= 0 && lseek(fd, position, SEEK_SET) == (off_t)position && read(fd, words, 8) >= 8) {
                firstWord = words[0];
                secondWord = words[1];
                if (swapped) {
                    firstWord = __builtin_bswap32(firstWord);
                    secondWord = __builtin_bswap32(secondWord);
                }
            } else if (
                length >= (uint32_t)(offset + sectionOffset + 8) &&
                (uint64_t)offset + sectionOffset + 8 <= length
            ) {
                // CoreFoundation defect: the second condition keeps the wrapped check from reading
                // outside `bytes`.
                const uint8_t *information = (const uint8_t *)bytes + offset + sectionOffset;
                memcpy(&firstWord, information, sizeof(firstWord));
                memcpy(&secondWord, information + 4, sizeof(secondWord));
                if (swapped) {
                    firstWord = __builtin_bswap32(firstWord);
                    secondWord = __builtin_bswap32(secondWord);
                }
            }
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    if (segmentFound != NULL) {
        *segmentFound = foundSegment;
    }
    if (firstInfoWord != NULL) {
        *firstInfoWord = firstWord;
    }
    if (secondInfoWord != NULL) {
        *secondInfoWord = secondWord;
    }
}

// Native _CFBundleGrokMachTypeForFatFile, iOS Simulator 24A434, 0x1c025f44c. Non-external.
// NXGetLocalArchInfo is called before _CFBundleCurrentArchitecture; then every output is cleared. Up to 512
// bytes (min(length, 512)) are copied into a local fat header, byte-swapped word by word when `swapped`.
// The architecture count is nfat_arch (+4), limited to (uint16_t)(copied - 8) / 20; zero answers 0. The
// slice is NXFindBestFatArch for the local cputype and cpusubtype, else (no local information or no match)
// for the current architecture with subtype 0 when that is nonzero, else the first architecture. With
// `architectures`, every architecture's cputype becomes a kCFNumberSInt32Type number appended once to a new
// mutable array (CFArrayGetFirstIndexOfValue over the whole array). The slice header comes from the
// descriptor (lseek to the 32-bit slice offset and a 32-byte read) or else from `bytes` when
// offset + 32 <= (uint32_t)length. A thin Mach-O magic of either byte order (+0) gives the filetype (+12,
// byte-swapped for 0xcefaedfe and 0xcffaedfe) as the result, and the X11 check (filetype 2), the Info.plist
// and the Objective-C image information are read at the slice offset as the outputs ask; any other magic
// answers 0. The swap flags passed on follow the slice magic, not `swapped`. The native callers pass at least
// 32 bytes; with fewer than 8, (uint16_t)(copied - 8) wraps and the architecture count can exceed the copied
// entries, as natively (no caller reaches it).
UInt32 _CFBundleGrokMachTypeForFatFile(
    int fd,
    const void *bytes,
    size_t length,
    uint8_t swapped,
    uint8_t *isX11,
    OCFArrayRef *architectures,
    OCFDictionaryRef *infodict,
    uint8_t *hasObjc,
    uint32_t *objcVersion,
    uint32_t *objcFlags
) {
    uint8_t fatHeader[512];
    uint8_t sliceHeader[32];
    const ArchInfoPrefix *localArchInfo = (const ArchInfoPrefix *)NXGetLocalArchInfo();
    const cpu_type_t currentArchitecture = _CFBundleCurrentArchitecture();
    if (isX11 != NULL) {
        *isX11 = 0;
    }
    if (architectures != NULL) {
        *architectures = NULL;
    }
    if (infodict != NULL) {
        *infodict = NULL;
    }
    if (hasObjc != NULL) {
        *hasObjc = 0;
    }
    if (objcVersion != NULL) {
        *objcVersion = 0;
    }
    if (objcFlags != NULL) {
        *objcFlags = 0;
    }
    const size_t copied = length < 512 ? length : 512;
    memmove(fatHeader, bytes, copied);
    if (swapped) {
        uint64_t position = 0;
        do {
            const uint32_t word = loadUInt32(fatHeader + position);
            const uint32_t swappedWord = __builtin_bswap32(word);
            memcpy(fatHeader + position, &swappedWord, sizeof(swappedWord));
            position += 4;
        } while (copied > position);
    }
    const uint32_t declaredCount = loadUInt32(fatHeader + 4);
    const uint32_t capacity = (uint16_t)(copied - 8) / 20u;
    const uint32_t count = declaredCount < capacity ? declaredCount : capacity;
    if (count == 0) {
        return 0;
    }
    struct fat_arch *fatArchs = (struct fat_arch *)(fatHeader + 8);
    struct fat_arch *best = NULL;
    if (localArchInfo != NULL) {
        best = NXFindBestFatArch(localArchInfo->cputype, localArchInfo->cpusubtype, fatArchs, count);
    }
    if (best == NULL && currentArchitecture != 0) {
        best = NXFindBestFatArch(currentArchitecture, 0, fatArchs, count);
    }
    if (architectures != NULL) {
        OCFMutableArrayRef array =
            OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
        for (uint32_t index = 0; index < count; ++index) {
            OCFNumberRef number =
                OCFNumberCreate(kOCFAllocatorSystemDefault, kOCFNumberSInt32Type, &fatArchs[index].cputype);
            if (OCFArrayGetFirstIndexOfValue(array, __OCFRangeMake(0, OCFArrayGetCount(array)), number) < 0) {
                OCFArrayAppendValue(array, number);
            }
            OCFRelease(number);
        }
        *architectures = array;
    }
    const uint32_t *sliceOffset = best != NULL ? &best->offset : &fatArchs[0].offset;
    const uint8_t *slice;
    if (fd >= 0 && lseek(fd, *sliceOffset, SEEK_SET) == (off_t)*sliceOffset &&
        read(fd, sliceHeader, 32) > 31) {
        slice = sliceHeader;
    } else {
        if ((uint64_t)*sliceOffset + 32 > (uint32_t)length) {
            return 0;
        }
        slice = (const uint8_t *)bytes + *sliceOffset;
    }
    uint8_t sliceSwapped;
    uint8_t sixtyFour;
    UInt32 fileType = loadUInt32(slice + 12);
    switch (loadUInt32(slice)) {
        case 0xfeedface:
            sliceSwapped = 0;
            sixtyFour = 0;
            break;
        case 0xfeedfacf:
            sliceSwapped = 0;
            sixtyFour = 1;
            break;
        case 0xcefaedfe:
            sliceSwapped = 1;
            sixtyFour = 0;
            break;
        case 0xcffaedfe:
            sliceSwapped = 1;
            sixtyFour = 1;
            break;
        default:
            return 0;
    }
    if (isX11 != NULL && fileType == (sliceSwapped ? 0x2000000 : 2)) {
        *isX11 = _CFBundleGrokX11FromFile(fd, bytes, length, *sliceOffset, sliceSwapped, sixtyFour);
    }
    if (sliceSwapped) {
        fileType = __builtin_bswap32(fileType);
    }
    if (infodict != NULL) {
        *infodict = _CFBundleCreateInfoDictFromFile(fd, bytes, length, *sliceOffset, sliceSwapped, sixtyFour);
    }
    if (hasObjc != NULL || objcVersion != NULL || objcFlags != NULL) {
        _CFBundleGrokObjcImageInfoFromFile(
            fd, bytes, length, *sliceOffset, sliceSwapped, sixtyFour, hasObjc, objcVersion, objcFlags
        );
    }
    return fileType;
}
