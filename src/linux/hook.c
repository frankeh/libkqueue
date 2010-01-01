/*
 * Copyright (c) 2009 Mark Heily <mark@heily.com>
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

#include <sys/epoll.h>
#include <stdlib.h>

#include "sys/event.h"
#include "private.h"

static int gcfd;

static int
gcfd_ctl(int op, struct kqueue *kq)
{
    struct epoll_event evt;

    evt.data.ptr = kq;
    evt.events = 0;
    return (epoll_ctl(gcfd, op, kq->kq_sockfd[0], &evt));
}

int
kqueue_init_hook(void)
{
    int epfd;

    if ((epfd = epoll_create(1)) < 0) {
        dbg_perror("epoll_create(2)");
        return (-1);
    }

    gcfd = epfd;

    return (0);
}

int
kqueue_gc(void)
{
    struct epoll_event evt;
    struct kqueue *kq;
    int rv;

    do {
        rv = epoll_wait(gcfd, &evt, 1, 0);
        if (rv > 0) {
            kq = (struct kqueue *)evt.data.ptr;
            rv = gcfd_ctl(EPOLL_CTL_DEL, kq);
            kqueue_free(kq);
        } else if (rv < 0) {
            dbg_perror("epoll_wait(2)");
        }
    } while (rv > 0);

    return (rv);
}

int
kqueue_create_hook(struct kqueue *kq)
{
    return (gcfd_ctl(EPOLL_CTL_ADD, kq));
}

int
kevent_wait(struct kqueue *kq, const struct timespec *timeout)
{
    int n;

    dbg_puts("waiting for events");
    kq->kq_rfds = kq->kq_fds;
    n = pselect(kq->kq_nfds, &kq->kq_rfds, NULL , NULL, timeout, NULL);
    if (n < 0) {
        if (errno == EINTR) {
            dbg_puts("signal caught");
            return (-1);
        }
        dbg_perror("pselect(2)");
        return (-1);
    }

    return (n);
}

int
kevent_copyout(struct kqueue *kq, int nready,
        struct kevent *eventlist, int nevents)
{
    struct filter *filt;
    int i, rv, nret;

    nret = 0;
    for (i = 0; (i < EVFILT_SYSCOUNT && nready > 0 && nevents > 0); i++) {
//        dbg_printf("eventlist: n = %d nevents = %d", nready, nevents);
        filt = &kq->kq_filt[i]; 
//        dbg_printf("pfd[%d] = %d", i, filt->kf_pfd);
        if (FD_ISSET(filt->kf_pfd, &kq->kq_rfds)) {
            dbg_printf("pending events for %s", 
                    filter_name(filt->kf_id));
            filter_lock(filt);
            rv = filt->kf_copyout(filt, eventlist, nevents);
            if (rv < 0) {
                filter_unlock(filt);
                dbg_puts("kevent_copyout failed");
                return (-1);
            }
            nret += rv;
            eventlist += rv;
            nevents -= rv;
            nready--;
            filter_unlock(filt);
        }
    }

    return (nret);
}