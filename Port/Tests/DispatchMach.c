//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// A test of libdispatch's Mach sources and channels on libmach (Port/libdispatch/patches 0003 and 0004, libmach's
// port watches) run under Node: a DISPATCH_SOURCE_TYPE_MACH_RECV source fires on its queue
// for messages another thread sends, once per event until re-armed, and stops at cancellation; a
// dispatch_mach channel delivers CONNECTED, the received messages (an inline one, one larger than the inline
// buffer, one carrying a send right), a receive barrier, DISCONNECTED and CANCELED in order, cancels from its
// own handler as Core Animation's channels do, and finishes a cancellation that came before the connection;
// a DISPATCH_SOURCE_TYPE_MACH_SEND source reports a dead name; a DISPATCH_SOURCE_TYPE_MEMORYPRESSURE source
// registers and never fires. Exit status 0 on success.

#include <dispatch/dispatch.h>
#include <mach/mach.h>

#define __DISPATCH_INDIRECT__
#include <private/mach_private.h>
#undef __DISPATCH_INDIRECT__

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int gFailures;

static void check(bool condition, const char *description) {
    printf("%s %s\n", condition ? "ok  " : "FAIL", description);
    if (!condition) {
        gFailures++;
    }
}

static mach_port_urefs_t references(mach_port_name_t name, mach_port_right_t right) {
    mach_port_urefs_t count = 0;
    mach_port_get_refs(mach_task_self(), name, right, &count);
    return count;
}

static bool waitFor(dispatch_semaphore_t semaphore) {
    return dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) == 0;
}

static mach_port_name_t makePort(void) {
    mach_port_name_t port = MACH_PORT_NULL;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
    mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
    mach_port_limits_t limits = {.mpl_qlimit = MACH_PORT_QLIMIT_MAX};
    mach_port_set_attributes(
        mach_task_self(), port, MACH_PORT_LIMITS_INFO, (mach_port_info_t)&limits, MACH_PORT_LIMITS_INFO_COUNT
    );
    return port;
}

static mach_msg_return_t sendSimple(mach_port_name_t port, mach_msg_id_t identifier) {
    mach_msg_header_t header = {0};
    header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header.msgh_size = sizeof(header);
    header.msgh_remote_port = port;
    header.msgh_id = identifier;
    return mach_msg(&header, MACH_SEND_MSG, sizeof(header), 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
}

typedef struct {
    mach_port_name_t port;
    mach_msg_id_t first;
    int count;
} Sender;

static void *sendFromThread(void *argument) {
    Sender *sender = argument;
    usleep(20000);
    for (int index = 0; index < sender->count; index++) {
        sendSimple(sender->port, sender->first + index);
    }
    return NULL;
}

static pthread_t startSender(Sender *sender) {
    pthread_t thread;
    pthread_create(&thread, NULL, sendFromThread, sender);
    return thread;
}

static char gQueueKey;

#pragma mark - DISPATCH_SOURCE_TYPE_MACH_RECV

static void testReceiveSource(void) {
    mach_port_name_t port = makePort();
    dispatch_queue_t queue = dispatch_queue_create("test.mach-recv", DISPATCH_QUEUE_SERIAL);
    dispatch_queue_set_specific(queue, &gQueueKey, &gQueueKey, NULL);
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, port, 0, queue);
    check(source != NULL, "mach-recv: the source is created");
    dispatch_semaphore_t received = dispatch_semaphore_create(0);
    dispatch_semaphore_t canceled = dispatch_semaphore_create(0);
    __block int events = 0;
    __block int messages = 0;
    __block bool onQueue = true;
    __block bool dataIsMessage = true;
    __block mach_msg_id_t lastIdentifier = 0;
    dispatch_source_set_event_handler(source, ^{
      events++;
      onQueue = onQueue && dispatch_get_specific(&gQueueKey) == &gQueueKey;
      dataIsMessage = dataIsMessage && dispatch_source_get_data(source) == 0x2;
      // One message per event: the source must fire again for the rest once it is re-armed.
      struct {
          mach_msg_header_t header;
          mach_msg_max_trailer_t trailer;
      } message;
      if (mach_msg(&message.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0, sizeof(message), port, 0, MACH_PORT_NULL) ==
          MACH_MSG_SUCCESS) {
          messages++;
          lastIdentifier = message.header.msgh_id;
          dispatch_semaphore_signal(received);
      }
    });
    dispatch_source_set_cancel_handler(source, ^{
      dispatch_semaphore_signal(canceled);
    });
    dispatch_resume(source);

    Sender sender = {port, 500, 3};
    pthread_t thread = startSender(&sender);
    bool all = waitFor(received) && waitFor(received) && waitFor(received);
    pthread_join(thread, NULL);
    check(all, "mach-recv: three messages sent by another thread are received");
    check(messages == 3 && lastIdentifier == 502, "mach-recv: in order");
    check(events >= 3, "mach-recv: the source fires again while messages remain");
    check(onQueue, "mach-recv: the handler runs on the source's queue");
    check(dataIsMessage, "mach-recv: the data is DISPATCH_MACH_RECV_MESSAGE");

    // A message queued before the source is resumed again after a suspension fires it on resume.
    dispatch_suspend(source);
    sendSimple(port, 600);
    usleep(20000);
    check(messages == 3, "mach-recv: a suspended source does not fire");
    dispatch_resume(source);
    check(waitFor(received) && lastIdentifier == 600, "mach-recv: resuming delivers the queued message");

    dispatch_source_cancel(source);
    check(waitFor(canceled), "mach-recv: the cancel handler runs");
    int before = events;
    sendSimple(port, 700);
    usleep(50000);
    check(events == before, "mach-recv: no event after cancellation");
    check(
        mach_port_get_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE, &(mach_port_urefs_t){0}) == KERN_SUCCESS,
        "mach-recv: the receive right stays the caller's"
    );
    dispatch_release(source);
    dispatch_release(queue);
    dispatch_release(received);
    dispatch_release(canceled);
    mach_port_destruct(mach_task_self(), port, -1, 0);
}

#pragma mark - dispatch_mach

typedef struct {
    dispatch_mach_reason_t reasons[16];
    _Atomic int count;
    mach_msg_id_t identifiers[4];
    size_t sizes[4];
    int received;
    mach_port_name_t disconnected;
    mach_port_name_t carried;
    bool onQueue;
    dispatch_semaphore_t done;
} ChannelLog;

static void recordReason(ChannelLog *log, dispatch_mach_reason_t reason) {
    int index = atomic_fetch_add(&log->count, 1);
    if (index < 16) {
        log->reasons[index] = reason;
    }
}

static bool reasonsAre(ChannelLog *log, const dispatch_mach_reason_t *expected, int count) {
    if (atomic_load(&log->count) != count) {
        return false;
    }
    for (int index = 0; index < count; index++) {
        if (log->reasons[index] != expected[index]) {
            return false;
        }
    }
    return true;
}

static void testChannel(void) {
    mach_port_name_t port = makePort();
    mach_port_name_t carried = makePort();
    dispatch_queue_t queue = dispatch_queue_create("test.channel", DISPATCH_QUEUE_SERIAL);
    dispatch_queue_set_specific(queue, &gQueueKey, &gQueueKey, NULL);
    __block ChannelLog log = {.onQueue = true, .done = dispatch_semaphore_create(0)};
    dispatch_semaphore_t messages = dispatch_semaphore_create(0);
    __block dispatch_mach_t channel = NULL;
    channel = dispatch_mach_create("test.channel", queue, ^(dispatch_mach_reason_t reason, dispatch_mach_msg_t message, mach_error_t error) {
      (void)error;
      recordReason(&log, reason);
      log.onQueue = log.onQueue && dispatch_get_specific(&gQueueKey) == &gQueueKey;
      if (reason == DISPATCH_MACH_MESSAGE_RECEIVED) {
          size_t size = 0;
          mach_msg_header_t *header = dispatch_mach_msg_get_msg(message, &size);
          if (log.received < 4) {
              log.identifiers[log.received] = header->msgh_id;
              log.sizes[log.received] = size - header->msgh_size;
          }
          if (header->msgh_id == 802) {
              mach_msg_port_descriptor_t *descriptor =
                  (mach_msg_port_descriptor_t *)((mach_msg_base_t *)header + 1);
              log.carried = descriptor->name;
          }
          log.received++;
          mach_msg_destroy(header);
          dispatch_semaphore_signal(messages);
      } else if (reason == DISPATCH_MACH_DISCONNECTED) {
          log.disconnected = dispatch_mach_msg_get_msg(message, NULL)->msgh_local_port;
      } else if (reason == DISPATCH_MACH_CANCELED) {
          // As Core Animation's channels do: the receive right goes, then the channel.
          mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE, -1);
          dispatch_release(channel);
          dispatch_semaphore_signal(log.done);
      }
    });
    check(channel != NULL, "channel: created");
    // Core Animation sets a QoS floor before connecting; the channel is inactive until then.
    dispatch_set_qos_class_floor(channel, QOS_CLASS_USER_INTERACTIVE, 0);
    dispatch_mach_connect(channel, port, MACH_PORT_NULL, NULL);

    // An inline message from another thread.
    Sender sender = {port, 800, 1};
    pthread_t thread = startSender(&sender);
    check(waitFor(messages), "channel: a message from another thread is delivered");
    pthread_join(thread, NULL);

    // A message larger than the inline receive buffer.
    size_t largeSize = 20000;
    mach_msg_header_t *large = calloc(1, largeSize);
    large->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    large->msgh_size = (mach_msg_size_t)largeSize;
    large->msgh_remote_port = port;
    large->msgh_id = 801;
    ((unsigned char *)large)[largeSize - 1] = 0x5A;
    check(
        mach_msg(large, MACH_SEND_MSG, (mach_msg_size_t)largeSize, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL) ==
            MACH_MSG_SUCCESS,
        "channel: a 20000-byte message is sent"
    );
    free(large);
    check(waitFor(messages), "channel: the large message is delivered");

    // A message carrying a send right, which the handler destroys with the message.
    struct {
        mach_msg_header_t header;
        mach_msg_body_t body;
        mach_msg_port_descriptor_t right;
    } complex = {0};
    complex.header.msgh_bits = MACH_MSGH_BITS_SET(MACH_MSG_TYPE_COPY_SEND, 0, 0, MACH_MSGH_BITS_COMPLEX);
    complex.header.msgh_size = sizeof(complex);
    complex.header.msgh_remote_port = port;
    complex.header.msgh_id = 802;
    complex.body.msgh_descriptor_count = 1;
    complex.right.name = carried;
    complex.right.disposition = MACH_MSG_TYPE_COPY_SEND;
    complex.right.type = MACH_MSG_PORT_DESCRIPTOR;
    check(
        mach_msg(&complex.header, MACH_SEND_MSG, sizeof(complex), 0, MACH_PORT_NULL, 0, MACH_PORT_NULL) ==
            MACH_MSG_SUCCESS,
        "channel: a message carrying a send right is sent"
    );
    check(waitFor(messages), "channel: the message carrying a right is delivered");
    check(log.carried == carried, "channel: the carried right arrives under its name");
    check(references(carried, MACH_PORT_RIGHT_SEND) == 1, "channel: destroying the message released the right");

    dispatch_semaphore_t barrier = dispatch_semaphore_create(0);
    dispatch_mach_receive_barrier(channel, ^{
      dispatch_semaphore_signal(barrier);
    });
    check(waitFor(barrier), "channel: the receive barrier runs");
    dispatch_mach_cancel(channel);
    check(waitFor(log.done), "channel: cancellation completes");

    dispatch_mach_reason_t expected[] = {
        DISPATCH_MACH_CONNECTED,
        DISPATCH_MACH_MESSAGE_RECEIVED,
        DISPATCH_MACH_MESSAGE_RECEIVED,
        DISPATCH_MACH_MESSAGE_RECEIVED,
        DISPATCH_MACH_BARRIER_COMPLETED,
        DISPATCH_MACH_DISCONNECTED,
        DISPATCH_MACH_CANCELED,
    };
    check(reasonsAre(&log, expected, 7), "channel: CONNECTED, 3 x MESSAGE_RECEIVED, BARRIER_COMPLETED, DISCONNECTED, CANCELED");
    check(
        log.identifiers[0] == 800 && log.identifiers[1] == 801 && log.identifiers[2] == 802,
        "channel: the messages in order"
    );
    check(
        log.sizes[0] == sizeof(dispatch_mach_trailer_t) && log.sizes[1] == sizeof(dispatch_mach_trailer_t),
        "channel: a message's size includes the AV trailer"
    );
    check(log.disconnected == port, "channel: DISCONNECTED names the receive right");
    check(log.onQueue, "channel: every event runs on the channel's target queue");
    check(references(port, MACH_PORT_RIGHT_RECEIVE) == 0, "channel: the handler released the receive right");
    mach_port_destruct(mach_task_self(), carried, -1, 0);
    dispatch_release(messages);
    dispatch_release(barrier);
    dispatch_release(log.done);
    dispatch_release(queue);
}

// Core Animation's sync-reply and destroy channels: no target queue, canceled from their own message handler
// after the expected number of messages; a message queued after cancellation stays on the port.
static void testChannelCanceledFromHandler(void) {
    mach_port_name_t port = makePort();
    __block ChannelLog log = {.done = dispatch_semaphore_create(0)};
    __block int pending = 2;
    __block dispatch_mach_t channel = NULL;
    channel = dispatch_mach_create("test.sync-replies", NULL, ^(dispatch_mach_reason_t reason, dispatch_mach_msg_t message, mach_error_t error) {
      (void)error;
      recordReason(&log, reason);
      if (reason == DISPATCH_MACH_MESSAGE_RECEIVED) {
          mach_msg_destroy(dispatch_mach_msg_get_msg(message, NULL));
          if (--pending == 0) {
              dispatch_mach_cancel(channel);
          }
      } else if (reason == DISPATCH_MACH_CANCELED) {
          dispatch_release(channel);
          dispatch_semaphore_signal(log.done);
      }
    });
    dispatch_mach_connect(channel, port, MACH_PORT_NULL, NULL);
    sendSimple(port, 900);
    sendSimple(port, 901);
    sendSimple(port, 902);
    check(waitFor(log.done), "channel canceled from its handler: cancellation completes");
    dispatch_mach_reason_t expected[] = {
        DISPATCH_MACH_CONNECTED,
        DISPATCH_MACH_MESSAGE_RECEIVED,
        DISPATCH_MACH_MESSAGE_RECEIVED,
        DISPATCH_MACH_DISCONNECTED,
        DISPATCH_MACH_CANCELED,
    };
    check(reasonsAre(&log, expected, 5), "channel canceled from its handler: two messages, then DISCONNECTED, CANCELED");
    mach_port_status_t status = {0};
    mach_msg_type_number_t count = MACH_PORT_RECEIVE_STATUS_COUNT;
    mach_port_get_attributes(mach_task_self(), port, MACH_PORT_RECEIVE_STATUS, (mach_port_info_t)&status, &count);
    check(status.mps_msgcount == 1, "channel canceled from its handler: the third message stays on the port");
    dispatch_release(log.done);
    mach_port_destruct(mach_task_self(), port, -1, 0);
}

static void testChannelCanceledBeforeConnect(void) {
    mach_port_name_t port = makePort();
    __block ChannelLog log = {.done = dispatch_semaphore_create(0)};
    dispatch_mach_t channel = dispatch_mach_create("test.early-cancel", NULL, ^(dispatch_mach_reason_t reason, dispatch_mach_msg_t message, mach_error_t error) {
      (void)message;
      (void)error;
      recordReason(&log, reason);
      if (reason == DISPATCH_MACH_CANCELED) {
          dispatch_semaphore_signal(log.done);
      }
    });
    sendSimple(port, 1000);
    dispatch_mach_cancel(channel);
    dispatch_mach_connect(channel, port, MACH_PORT_NULL, NULL);
    check(waitFor(log.done), "channel canceled before connecting: cancellation completes");
    dispatch_mach_reason_t expected[] = {
        DISPATCH_MACH_CONNECTED,
        DISPATCH_MACH_DISCONNECTED,
        DISPATCH_MACH_CANCELED,
    };
    check(reasonsAre(&log, expected, 3), "channel canceled before connecting: no message is received");
    dispatch_release(channel);
    dispatch_release(log.done);
    mach_port_destruct(mach_task_self(), port, -1, 0);
}

#pragma mark - DISPATCH_SOURCE_TYPE_MACH_SEND

static void testDeadName(void) {
    mach_port_name_t port = makePort();
    dispatch_queue_t queue = dispatch_queue_create("test.mach-send", DISPATCH_QUEUE_SERIAL);
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_SEND, port, DISPATCH_MACH_SEND_DEAD, queue);
    check(source != NULL, "mach-send: the source is created");
    dispatch_semaphore_t dead = dispatch_semaphore_create(0);
    __block uintptr_t data = 0;
    dispatch_source_set_event_handler(source, ^{
      data = dispatch_source_get_data(source);
      dispatch_semaphore_signal(dead);
    });
    dispatch_resume(source);
    usleep(20000);
    check(data == 0, "mach-send: no event while the receive right lives");
    mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE, -1);
    check(waitFor(dead), "mach-send: destroying the receive right fires the source");
    check(data == DISPATCH_MACH_SEND_DEAD, "mach-send: the data is DISPATCH_MACH_SEND_DEAD");
    mach_port_type_t type = 0;
    mach_port_type(mach_task_self(), port, &type);
    check(type == MACH_PORT_TYPE_DEAD_NAME, "mach-send: the name is a dead name");
    dispatch_source_cancel(source);
    dispatch_release(source);

    // A name that is dead already fires at registration.
    dispatch_source_t late = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_SEND, port, DISPATCH_MACH_SEND_DEAD, queue);
    data = 0;
    dispatch_source_set_event_handler(late, ^{
      data = dispatch_source_get_data(late);
      dispatch_semaphore_signal(dead);
    });
    dispatch_resume(late);
    check(waitFor(dead) && data == DISPATCH_MACH_SEND_DEAD, "mach-send: a dead name fires at once");
    dispatch_source_cancel(late);
    dispatch_release(late);
    mach_port_deallocate(mach_task_self(), port);
    dispatch_release(dead);
    dispatch_release(queue);
}

#pragma mark - DISPATCH_SOURCE_TYPE_MEMORYPRESSURE

static void testMemoryPressure(void) {
    dispatch_source_t source = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_MEMORYPRESSURE,
        0,
        DISPATCH_MEMORYPRESSURE_WARN,
        dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0)
    );
    check(source != NULL, "memory pressure: the source is created (warning level, as Core Animation)");
    __block _Atomic int fired = 0;
    dispatch_semaphore_t canceled = dispatch_semaphore_create(0);
    dispatch_source_set_event_handler(source, ^{
      atomic_fetch_add(&fired, 1);
    });
    dispatch_source_set_cancel_handler(source, ^{
      dispatch_semaphore_signal(canceled);
    });
    dispatch_resume(source);
    usleep(50000);
    check(atomic_load(&fired) == 0, "memory pressure: the source never fires");
    dispatch_source_cancel(source);
    check(waitFor(canceled), "memory pressure: the source cancels");
    dispatch_release(source);
    dispatch_release(canceled);
    check(
        dispatch_source_create(DISPATCH_SOURCE_TYPE_MEMORYPRESSURE, 0, 0x100, dispatch_get_main_queue()) == NULL,
        "memory pressure: an unknown level is refused"
    );
}

int main(void) {
    testReceiveSource();
    testChannel();
    testChannelCanceledFromHandler();
    testChannelCanceledBeforeConnect();
    testDeadName();
    testMemoryPressure();
    printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
