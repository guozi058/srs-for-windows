#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>
#include "common.h"
#include <map>

using namespace std;
//extern int fds[FD_SETSIZE+5];
extern map<int32_t, int32_t> fds;

//static struct _st_seldata {
//	fd_set fd_read_set, fd_write_set, fd_exception_set;
//	//int fd_ref_cnts[FD_SETSIZE][3];
//	map<int32_t, map<uint8_t, uint32_t>> fd_ref_cnts;
//	int maxfd;
//} 
_st_seldata *_st_select_data;

#define _ST_SELECT_MAX_OSFD      (_st_select_data->maxfd)
#define _ST_SELECT_READ_SET      (_st_select_data->fd_read_set)
#define _ST_SELECT_WRITE_SET     (_st_select_data->fd_write_set)
#define _ST_SELECT_EXCEP_SET     (_st_select_data->fd_exception_set)
#define _ST_SELECT_READ_CNT(fd)  (_st_select_data->fd_ref_cnts[fd][0])
#define _ST_SELECT_WRITE_CNT(fd) (_st_select_data->fd_ref_cnts[fd][1])
#define _ST_SELECT_EXCEP_CNT(fd) (_st_select_data->fd_ref_cnts[fd][2])

_st_eventsys_t *_st_eventsys = NULL;


/*****************************************
 * select event system
 */

ST_HIDDEN int _st_select_init(void)
{
    //_st_select_data = (struct _st_seldata *) malloc(sizeof(*_st_select_data));
	_st_select_data = new _st_seldata;
    if (!_st_select_data)
        return -1;

    //memset(_st_select_data, 0, sizeof(*_st_select_data));
	memset((void*)(&_st_select_data->fd_exception_set), 0, sizeof(fd_set));
	memset((void*)(&_st_select_data->fd_read_set), 0, sizeof(fd_set));
	memset((void*)(&_st_select_data->fd_write_set), 0, sizeof(fd_set));
	
    _st_select_data->maxfd = -1;

    return 0;
}

ST_HIDDEN int _st_select_pollset_add(struct pollfd *pds, int npds)
{
    struct pollfd *pd;
    struct pollfd *epd = pds + npds;

    /* Do checks up front */
    for (pd = pds; pd < epd; pd++) {
		if (pd->fd < 0 || !pd->events || //pd->fd >= FD_SETSIZE ||
            (pd->events & ~(POLLIN | POLLOUT | POLLPRI))) { 
            errno = EINVAL;
            return -1;
        }
    }

    for (pd = pds; pd < epd; pd++) {
        if (pd->events & POLLIN) {
			FD_SET(fds[pd->fd], &_ST_SELECT_READ_SET);
            _ST_SELECT_READ_CNT(pd->fd)++;
        }
        if (pd->events & POLLOUT) {
			FD_SET(fds[pd->fd], &_ST_SELECT_WRITE_SET);
            _ST_SELECT_WRITE_CNT(pd->fd)++;
        }
        if (pd->events & POLLPRI) {
			FD_SET(fds[pd->fd], &_ST_SELECT_EXCEP_SET);
            _ST_SELECT_EXCEP_CNT(pd->fd)++;
        }
        if (_ST_SELECT_MAX_OSFD < pd->fd)
            _ST_SELECT_MAX_OSFD = pd->fd;
    }

    return 0;
}

ST_HIDDEN void _st_select_pollset_del(struct pollfd *pds, int npds)
{
    struct pollfd *pd;
    struct pollfd *epd = pds + npds;

    for (pd = pds; pd < epd; pd++) {
        if (pd->events & POLLIN) {
            if (--_ST_SELECT_READ_CNT(pd->fd) == 0)
                FD_CLR(fds[pd->fd], &_ST_SELECT_READ_SET);
        }
        if (pd->events & POLLOUT) {
            if (--_ST_SELECT_WRITE_CNT(pd->fd) == 0)
                FD_CLR(fds[pd->fd], &_ST_SELECT_WRITE_SET);
        }
        if (pd->events & POLLPRI) {
            if (--_ST_SELECT_EXCEP_CNT(pd->fd) == 0)
                FD_CLR(fds[pd->fd], &_ST_SELECT_EXCEP_SET);
        }
    }
}

ST_HIDDEN void _st_select_find_bad_fd(void)
{
    _st_clist_t *q;
    _st_pollq_t *pq;
    int notify;
    struct pollfd *pds, *epds;
    int pq_max_osfd, osfd;
    short events;
	unsigned long noBlock = 0;

    _ST_SELECT_MAX_OSFD = -1;

    for (q = _ST_IOQ.next; q != &_ST_IOQ; q = q->next) {
        pq = _ST_POLLQUEUE_PTR(q);
        notify = 0;
        epds = pq->pds + pq->npds;
        pq_max_osfd = -1;
      
        for (pds = pq->pds; pds < epds; pds++) {
			osfd = pds->fd;
            pds->revents = 0;
            if (pds->events == 0)
                continue;
			if (ioctlsocket(fds[osfd], FIONBIO , &noBlock ) < 0){
                pds->revents = POLLNVAL;
                notify = 1;
            }
            if (osfd > pq_max_osfd) {
                pq_max_osfd = osfd;
            }
        }

        if (notify) {
            ST_REMOVE_LINK(&pq->links);
            pq->on_ioq = 0;
            /*
             * Decrement the count of descriptors for each descriptor/event
             * because this I/O request is being removed from the ioq
             */
            for (pds = pq->pds; pds < epds; pds++) {
				osfd = pds->fd;
                events = pds->events;
                if (events & POLLIN) {
                    if (--_ST_SELECT_READ_CNT(osfd) == 0) {
                        FD_CLR(fds[osfd], &_ST_SELECT_READ_SET);
                    }
                }
                if (events & POLLOUT) {
                    if (--_ST_SELECT_WRITE_CNT(osfd) == 0) {
                        FD_CLR(fds[osfd], &_ST_SELECT_WRITE_SET);
                    }
                }
                if (events & POLLPRI) {
                    if (--_ST_SELECT_EXCEP_CNT(osfd) == 0) {
                        FD_CLR(fds[osfd], &_ST_SELECT_EXCEP_SET);
                    }
                }
            }

            if (pq->thread->flags & _ST_FL_ON_SLEEPQ)
                _ST_DEL_SLEEPQ(pq->thread);
            pq->thread->state = _ST_ST_RUNNABLE;
            _ST_ADD_RUNQ(pq->thread);
        } else {
            if (_ST_SELECT_MAX_OSFD < pq_max_osfd)
                _ST_SELECT_MAX_OSFD = pq_max_osfd;
        }
    }
}

ST_HIDDEN void _st_select_dispatch(void)
{
    struct timeval timeout, *tvp;
    fd_set r, w, e;
    fd_set *rp, *wp, *ep;
    int nfd, pq_max_osfd, osfd;
    _st_clist_t *q;
    st_utime_t min_timeout;
    _st_pollq_t *pq;
    int notify;
    struct pollfd *pds, *epds;
    short events, revents;

    /*
     * Assignment of fd_sets
     */
    r = _ST_SELECT_READ_SET;
    w = _ST_SELECT_WRITE_SET;
    e = _ST_SELECT_EXCEP_SET;

    rp = &r;
    wp = &w;
    ep = &e;

    if (_ST_SLEEPQ == NULL) {
        tvp = NULL;
    } else {
        min_timeout = (_ST_SLEEPQ->due <= _ST_LAST_CLOCK) ? 0 :
            (_ST_SLEEPQ->due - _ST_LAST_CLOCK);
        timeout.tv_sec  = (int) (min_timeout / 1000000);
        timeout.tv_usec = (int) (min_timeout % 1000000);
        tvp = &timeout;
    }

    /* Check for I/O operations */
    nfd = select(_ST_SELECT_MAX_OSFD + 1, rp, wp, ep, tvp);

    /* Notify threads that are associated with the selected descriptors */
    if (nfd > 0) {
        _ST_SELECT_MAX_OSFD = -1;
        for (q = _ST_IOQ.next; q != &_ST_IOQ; q = q->next) {
            pq = _ST_POLLQUEUE_PTR(q);
            notify = 0;
            epds = pq->pds + pq->npds;
            pq_max_osfd = -1;
      
            for (pds = pq->pds; pds < epds; pds++) {
				osfd = pds->fd;
                events = pds->events;
                revents = 0;
                if ((events & POLLIN) && FD_ISSET(fds[osfd], rp)) {
                    revents |= POLLIN;
                }
                if ((events & POLLOUT) && FD_ISSET(fds[osfd], wp)) {
                    revents |= POLLOUT;
                }
                if ((events & POLLPRI) && FD_ISSET(fds[osfd], ep)) {
                    revents |= POLLPRI;
                }
                pds->revents = revents;
                if (revents) {
                    notify = 1;
                }
                if (osfd > pq_max_osfd) {
                    pq_max_osfd = osfd;
                }
            }
            if (notify) {
                ST_REMOVE_LINK(&pq->links);
                pq->on_ioq = 0;
                /*
                 * Decrement the count of descriptors for each descriptor/event
                 * because this I/O request is being removed from the ioq
                 */
                for (pds = pq->pds; pds < epds; pds++) {
					osfd = pds->fd;
                    events = pds->events;
                    if (events & POLLIN) {
                        if (--_ST_SELECT_READ_CNT(osfd) == 0) {
                            FD_CLR(fds[osfd], &_ST_SELECT_READ_SET);
                        }
                    }
                    if (events & POLLOUT) {
                        if (--_ST_SELECT_WRITE_CNT(osfd) == 0) {
                            FD_CLR(fds[osfd], &_ST_SELECT_WRITE_SET);
                        }
                    }
                    if (events & POLLPRI) {
                        if (--_ST_SELECT_EXCEP_CNT(osfd) == 0) {
                            FD_CLR(fds[osfd], &_ST_SELECT_EXCEP_SET);
                        }
                    }
                }

                if (pq->thread->flags & _ST_FL_ON_SLEEPQ)
                    _ST_DEL_SLEEPQ(pq->thread);
                pq->thread->state = _ST_ST_RUNNABLE;
                _ST_ADD_RUNQ(pq->thread);
            } else {
                if (_ST_SELECT_MAX_OSFD < pq_max_osfd)
                    _ST_SELECT_MAX_OSFD = pq_max_osfd;
            }
        }
    } else if (nfd < 0) {
        /*
         * It can happen when a thread closes file descriptor
         * that is being used by some other thread -- BAD!
         */
        if (errno == EBADF)
            _st_select_find_bad_fd();
    }
}

ST_HIDDEN int _st_select_fd_new(int osfd)
{
    //if (osfd >= FD_SETSIZE) {
    //    errno = EMFILE;
    //    return -1;
    //}

    return 0;
}

ST_HIDDEN int _st_select_fd_close(int osfd)
{
    if (_ST_SELECT_READ_CNT(osfd) || _ST_SELECT_WRITE_CNT(osfd) ||
        _ST_SELECT_EXCEP_CNT(osfd)) {
        errno = EBUSY;
        return -1;
    }

    return 0;
}

ST_HIDDEN int _st_select_fd_getlimit(void)
{
    return FD_SETSIZE;
}
static _st_eventsys_t _st_select_eventsys = {
    "select",
    ST_EVENTSYS_SELECT,
    _st_select_init,
    _st_select_dispatch,
    _st_select_pollset_add,
    _st_select_pollset_del,
    _st_select_fd_new,
    _st_select_fd_close,
    _st_select_fd_getlimit
};



/*****************************************
 * Public functions
 */

int st_set_eventsys(int eventsys)
{
    if (_st_eventsys) {
        errno = EBUSY;
        return -1;
    }

    switch (eventsys) {
    case ST_EVENTSYS_DEFAULT:
		_st_eventsys = &_st_select_eventsys;
        break;
    case ST_EVENTSYS_SELECT:
        _st_eventsys = &_st_select_eventsys;
        break;
    default:
        errno = EINVAL;
        return -1;
    }

    return 0;
}

