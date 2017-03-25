/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Portions created by SGI are Copyright (C) 2000 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): Silicon Graphics, Inc.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the ____ license (the  "[____] License"), in which case the provisions
 * of [____] License are applicable instead of those above. If you wish to
 * allow use of your version of this file only under the terms of the [____]
 * License and not to allow others to use your version of this file under the
 * NPL, indicate your decision by deleting  the provisions above and replace
 * them with the notice and other provisions required by the [____] License.
 * If you do not delete the provisions above, a recipient may use your version
 * of this file under either the NPL or the [____] License.
 */

#ifndef __ST_THREAD_H__
#define __ST_THREAD_H__

#include <time.h>
#include <sys/types.h>
#include <stddef.h>

#ifdef _WIN32
//#include "neterr.h"
#endif

//#ifndef ETIME
//#define ETIME ETIMEDOUT
//#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef int ssize_t;
typedef int mode_t;
typedef int pid_t;
#define APIEXPORT
#else
#define APIEXPORT
#endif
typedef unsigned long long  st_utime_t;
typedef struct _st_thread * st_thread_t;
typedef struct _st_cond *   st_cond_t;
typedef struct _st_mutex *  st_mutex_t;
typedef struct _st_netfd *  st_netfd_t;
#ifdef ST_SWITCH_CB
typedef void (*st_switch_cb_t)(void);
#endif

#define ST_EVENTSYS_DEFAULT 0
#define ST_EVENTSYS_SELECT  1
#define ST_EVENTSYS_POLL    ST_EVENTSYS_SELECT
#define ST_EVENTSYS_ALT     ST_EVENTSYS_SELECT

APIEXPORT int st_init(void);
APIEXPORT int st_getfdlimit(void);

APIEXPORT int st_set_eventsys(int eventsys);

APIEXPORT st_thread_t st_thread_self(void);
APIEXPORT void st_thread_exit(void *retval);
APIEXPORT int st_thread_join(st_thread_t thread, void **retvalp);
APIEXPORT void st_thread_interrupt(st_thread_t thread);
APIEXPORT st_thread_t st_thread_create(void *(*start)(void *arg), void *arg,
                                    int joinable, int stack_size);

APIEXPORT st_utime_t st_utime(void);
APIEXPORT st_utime_t st_utime_last_clock(void);
APIEXPORT int st_timecache_set(int on);
APIEXPORT time_t st_time(void);
APIEXPORT int st_usleep(st_utime_t usecs);
APIEXPORT int st_sleep(int secs);
APIEXPORT st_cond_t st_cond_new(void);
APIEXPORT int st_cond_destroy(st_cond_t cvar);
APIEXPORT int st_cond_timedwait(st_cond_t cvar, st_utime_t timeout);
APIEXPORT int st_cond_wait(st_cond_t cvar);
APIEXPORT int st_cond_signal(st_cond_t cvar);
APIEXPORT int st_cond_broadcast(st_cond_t cvar);
APIEXPORT st_mutex_t st_mutex_new(void);
APIEXPORT int st_mutex_destroy(st_mutex_t lock);
APIEXPORT int st_mutex_lock(st_mutex_t lock);
APIEXPORT int st_mutex_unlock(st_mutex_t lock);
APIEXPORT int st_mutex_trylock(st_mutex_t lock);

APIEXPORT int st_key_create(int *keyp, void (*destructor)(void *));
APIEXPORT int st_key_getlimit(void);
APIEXPORT int st_thread_setspecific(int key, void *value);
APIEXPORT void *st_thread_getspecific(int key);

APIEXPORT st_netfd_t st_netfd_open(int osfd);
APIEXPORT st_netfd_t st_netfd_open_socket(int osfd);
APIEXPORT void st_netfd_free(st_netfd_t fd);
APIEXPORT int st_netfd_close(st_netfd_t fd);
APIEXPORT int st_netfd_fileno(st_netfd_t fd);
APIEXPORT void st_netfd_setspecific(st_netfd_t fd, void *value,
                                 void (*destructor)(void *));
APIEXPORT void *st_netfd_getspecific(st_netfd_t fd);
APIEXPORT int st_netfd_serialize_accept(st_netfd_t fd);
APIEXPORT int st_netfd_poll(st_netfd_t fd, int how, st_utime_t timeout);

APIEXPORT int st_poll(struct pollfd *pds, int npds, st_utime_t timeout);
APIEXPORT st_netfd_t st_accept(st_netfd_t fd, struct sockaddr *addr, int *addrlen,
                            st_utime_t timeout);
APIEXPORT int st_connect(st_netfd_t fd, const struct sockaddr *addr, int addrlen,
                      st_utime_t timeout);
APIEXPORT ssize_t st_read(st_netfd_t fd, void *buf, size_t nbyte,
                       st_utime_t timeout);
APIEXPORT ssize_t st_read_fully(st_netfd_t fd, void *buf, size_t nbyte,
                             st_utime_t timeout);
APIEXPORT ssize_t st_write(st_netfd_t fd, const void *buf, size_t nbyte,
                        st_utime_t timeout);
APIEXPORT ssize_t st_writev(st_netfd_t fd, const struct iovec *iov, int iov_size,
                         st_utime_t timeout);
APIEXPORT int st_recvfrom(st_netfd_t fd, void *buf, int len,
                       struct sockaddr *from, int *fromlen,
                       st_utime_t timeout);
APIEXPORT int st_sendto(st_netfd_t fd, const void *msg, int len,
                     const struct sockaddr *to, int tolen, st_utime_t timeout);
APIEXPORT st_netfd_t st_open(const char *path, int oflags, mode_t mode);

//APIEXPORT st_errno(void);
APIEXPORT int st_errno(void);

#ifdef __cplusplus
}
#endif

#endif /* !__ST_THREAD_H__ */
