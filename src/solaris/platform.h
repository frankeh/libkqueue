/*
 * Copyright (c) 2011 Mark Heily <mark@heily.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef  _KQUEUE_SOLARIS_PLATFORM_H
#define  _KQUEUE_SOLARIS_PLATFORM_H

#include <sys/queue.h>

#define THREAD_ID (pthread_self())

/*
 * Atomic integer operations that override the ones in posix/platform.h
 */
#include <atomic.h>
#undef atomic_inc
#define atomic_inc      atomic_inc_32_nv
#undef atomic_dec
#define atomic_dec      atomic_dec_32_nv

/*
 * Event ports
 */
#include <port.h>
/* Used to set portev_events for PORT_SOURCE_USER */
#define X_PORT_SOURCE_SIGNAL  101
#define X_PORT_SOURCE_USER    102

void    solaris_kqueue_free(struct kqueue *);
int     solaris_kqueue_init(struct kqueue *);
int     solaris_kevent_wait(struct kqueue *, const struct timespec *);
int     solaris_kevent_copyout(struct kqueue *, int, struct kevent *, int);
void    port_event_dequeue(port_event_t *, struct kqueue *);

/*
 * Additional members of struct kqueue
 */
#define KQUEUE_PLATFORM_SPECIFIC \
    int    kq_port; \
    TAILQ_HEAD(event_buf_listhead,event_buf) kq_events

/*
 * Data structures
 */
struct event_buf {
    port_event_t pe;
    TAILQ_ENTRY(event_buf) entries;
};

#endif  /* ! _KQUEUE_SOLARIS_PLATFORM_H */