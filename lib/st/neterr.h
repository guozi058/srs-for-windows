#ifndef _NETERR_H
#define _NETERR_H
#define ENOTSOCK              216     /* Socket operation on non-socket */
#define EDESTADDRREQ          217     /* Destination address required */
#define EMSGSIZE              218     /* Message too long */
#define EPROTOTYPE            219     /* Protocol wrong type for socket */
#define ENOPROTOOPT           220     /* Protocol not available */
#define EPROTONOSUPPORT       221     /* Protocol not supported */
#define EOPNOTSUPP            223     /* Operation not supported */
#define EAFNOSUPPORT          225     /*Address family not supported by
                                          protocol family*/
#define EADDRINUSE            226     /* Address already in use */
#define EADDRNOTAVAIL         227     /* Can't assign requested address */

        /* operational errors */
#define ENETDOWN              228     /* Network is down */
#define ENETUNREACH           229     /* Network is unreachable */
#define ECONNABORTED          231     /* Software caused connection abort */
#define ECONNRESET            232     /* Connection reset by peer */
#define ENOBUFS               233     /* No buffer space available */
#define EISCONN               234     /* Socket is already connected */
#define ENOTCONN              235     /* Socket is not connected */
#define ETIMEDOUT             238     /* Connection timed out */
#define ECONNREFUSED          239     /* Connection refused */
#define EHOSTUNREACH          242     /* No route to host */
#define EALREADY                244     /* Operation already in progress */
#define EINPROGRESS             245     /* Operation now in progress */
#define EWOULDBLOCK             246     /* Operation would block */
#endif
