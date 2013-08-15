/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/wusock.h,v $
 * $Author: fredk $
 * $Date: 2008/02/22 16:36:33 $
 * $Revision: 1.31 $
 * $Name:  $
 *
 * File:         sock.h
 * Created:      01/05/2005 04:32:38 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WULIB_SOCK_H
#define _WULIB_SOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
// IPv4 domain sockets
#include <netinet/in.h>
#ifndef __CYGWIN__
#  include <netpacket/packet.h>
#  include <net/ethernet.h>
#endif
#include <sys/ioctl.h>
#include <net/if.h>
// UNIX domain sockets
#include <sys/un.h>
#include <wulib/net_util.h>
#include <wulib/wunet.h>
/*
  * Codes:
  *   L - logic error
  *   A - Async (Non-Blocking), OK
  *   T - Temporary problem, resource not available
  *   U - Unauthorized 
  *   P - Peer problem (not there)
  *   S - Signal received
  *   N - some wort of network problem (no route)
  *   R - RLI error
  *
  * errno from systems calls:
  * socket:
  * L EAFNOSUPPORT: address family not supported
  * L EPROTONOSUPPORT: The protocol is not supported by the address family,
  * T EMFILE: no file descriptors available for process
  * T ENFILE: no file descriptors available for system
  * U EACCES: The process does not have appropriate privileges.
  * T ENOBUFS, ENOMEM: Insufficient memory/resources was available
  *   XXX EPROTOTYPE: The socket type is not supported by the protocol.
  *
  * bind:
  * L EBADF:  sockfd is not a valid descriptor.
  * L EINVAL: The socket is already bound to an address.  This may  change  in
  *           the future: see linux/unix/sock.c for details.
  * U EACCES: The address is protected, and the user is not the super-user.
  * L ENOTSOCK: Argument is a descriptor for a file, not a socket.
  *   XXX EADDRINUSE, EADDRNOTAVAIL, EAFNOSUPPORT, EOPNOTSUPP, EINVAL,
  *   XXX EISCONN, ELOOP, ENAMETOOLONG
  *
  * accept:
  * A EAGAIN, EWOULDBLOCK: O_NONBLOCK is set for the socket file descriptor
  *           and no connections are present to be accepted.
  * L EBADF: The socket argument is not a valid file descriptor.
  * L ENOTSOCK: The socket argument does not refer to a socket.
  * L EOPNOTSUPP: The socket type of the specified socket does not support
  *          accepting connections (i.e. it is not a SOCK_STREAM).
  * S EINTR: The accept() function was interrupted by a signal that was
  *          caught before a valid connection arrived.
  * T EMFILE: {OPEN_MAX} file descriptors are currently open in the calling process.
  * T ENFILE: The maximum number of file descriptors in the system are already open.
  * P ECONNABORTED: A connection has been aborted.
  * L EINVAL: The socket is not accepting connections. i.e. it is not
  *           listening.
  * L EFAULT (Linux): The addr parameter is not in a writable part of the
  *           user address space.
  * T ENOBUFS: No buffer space is available.
  * T ENOMEM: There was insufficient memory available to complete the operation.
  * ? EPROTO: A protocol error has occurred; for example, the STREAMS
  *           protocol stack has not been initialized.
  * U EPERM (Linux): Firewall rules forbid connection.
  *
  * listen:
  * L EADDRINUSE (Linux): Another socket is already listening on the same port.
  * L EBADF: The socket argument is not a valid file descriptor.
  * L ENOTSOCK: The socket argument does not refer to a socket.
  * L EOPNOTSUPP: The socket protocol does not support listen().
  *   XXX EDESTADDRREQ, EINVAL, EACCES, ENOBUFS
  *       complete the call.
  *
  * connect:
  * L EBADF: The socket argument is not a valid file descriptor.
  * L EFAULT (Linux): The socket structure  address  is  outside  the
  *          user address space.
  * L ENOTSOCK: The socket argument does not refer to a socket.
  * L EISCONN: The specified socket is connection-mode and is already connected.
  * P ECONNREFUSED: The target address was not listening for connections or
  *          refused the connection request.
  * P ETIMEDOUT: The attempt to connect timed out before a connection was made.
  * N ENETUNREACH: No route to the network is present.
  * ? EADDRINUSE (Linux): Local address is already in use.
  * A EINPROGRESS: O_NONBLOCK is set for the file descriptor for the
  *      socket and the connection cannot be immediately established; the
  *      connection shall be established asynchronously.
  *   -- Linux note: It is possible to select(2) or poll(2)  for
  *      completion  by  selecting  the  socket for writing. After select
  *      indicates writability, use getsockopt(2) to read the SO_ERROR
  *      option at level SOL_SOCKET to determine whether connect completed
  *      successfully (SO_ERROR  is zero) or unsuccessfully (SO_ERROR is
  *      one of the usual error codes listed here, explaining the reason
  *      for the failure).
  * L EALREADY: A connection request is already in progress, i.e. connect
  *      already called.
  * T EAGAIN (Linux): No  more free local ports or insufficient entries in
  *      the routing cache. For PF_INET see the  net.ipv4.ip_local_port_range
  *      sysctl in ip(7) on how to increase the number of local ports.
  * R EAFNOSUPPORT: The specified address is not a valid address for the
  *      address family of the specified socket.
  * R EACCES, EPERM (Linux): The user tried to connect to a broadcast address
  *      without  having the  socket  broadcast  flag enabled  or the connection
  *      request failed because of a local firewall rule.
  *   XXX EADDRNOTAVAIL, EINTR, EPROTOTYPE, EIO, ELOOP, ENAMETOOLONG, ENOENT, ENOTDIR
  *
  * read, write: -- Linux Only --
  * S EINTR:  The call was interrupted by a signal before any data was read.
  * A EAGAIN: Non-blocking  I/O has been selected using O_NONBLOCK and no data
  *          was immediately available for reading.
  * L EIO    I/O error. This will happen for example when the process is in a
  *          background  process  group,  tries  to read from its controlling
  *          tty, and either it is ignoring or blocking SIGTTIN or  its  pro-
  *          cess  group is orphaned.  It may also occur when there is a low-
  *          level I/O error while reading from a disk or tape.
  *          OR
  *          A low-level I/O error occurred while modifying the inode.
  * L EISDIR fd refers to a directory.
  * L EBADF  fd is not a valid file descriptor or is not open for
  *          reading/writing
  * L EINVAL fd is attached to an object which is unsuitable for
  *          reading/writing
  * L EFAULT buf is outside your accessible address space.
  * L EFBIG (write)
  * P/S EPIPE (write): fd is connected to a pipe or socket whose reading end is
  *          closed.  When  this  happens the writing process will also receive a
  *          SIGPIPE signal.  (Thus, the write return value is seen only if the
  *          program catches, blocks or ignores this signal.)
  * T ENOSPC The device containing the file referred to by fd has no room for
  *          the data.
  *
  * */

// #define WUSOCK_DEBUG
typedef unsigned int sockFlags_t;
static const sockFlags_t sok_Flg_None       = 0x00000000;
static const sockFlags_t sok_Flg_OPFlags    = 0x000000FF;
static const sockFlags_t sok_Flg_Async      = 0x00000001; // socket is nonblocking
static const sockFlags_t sok_Flg_Retry      = 0x00000002; // retry on interrupt
static const sockFlags_t sok_Flg_NoDelay    = 0x00000004; //
static const sockFlags_t sok_Flg_ReuseAddr  = 0x00000008; //
static const sockFlags_t sok_Flg_RxTout     = 0x00000010; // socket has timeout set for RX
static const sockFlags_t sok_Flg_TxTout     = 0x00000020; // socket has timeout set for TX
static const sockFlags_t sok_Flg_StateFlags = 0x00FF0000;
static const sockFlags_t sok_Flg_Open       = 0x00010000;
static const sockFlags_t sok_Flg_Connect    = 0x00020000;
static const sockFlags_t sok_Flg_Listen     = 0x00040000;

#define WUNET_SOCK_BACKLOG_DEF   10

typedef struct wusock_t {
  /* the socket descriptor, -1 if not open */
  int                 sd_,
  /* the next 3 fields are just the arguments to the socket system call */
                      st_,  // socket type:
                            // one of {SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET,
                            //         SOCK_RAW,    SOCK_RDM,   SOCK_PACKET}
                      pr_,  // Protocol type, could be set to 0 by application
                            // Otherwise for IP use {IPPROTO_TCP, IPPROTO_UDP,
                            // IPPROTO_SCTP}
                     dom_;  // Communications Domain; also known as the protocol family.
                            // see the AF_XXX macros. Must be set to one of
                            // {AF_INET, AF_LOCAL/AF_UNIX, AF_PACKET}
  /* Config and State flags affecting sockets/wrapper behavior */
  sockFlags_t      flags_;  // Various flags to control socket processing/operations
                            // Operation flags: {Async, Retry, NoDelay, ReuseAddr} 
                            // State Flags: {Open, Connect, Listen}
  /* Status and error info */
  wun_errType_t       et_;  // result status from last operation
                            // Current error state, see net_util.h.  Broken
                            // into two sets of states: NoError and Error.
  wun_errInfo_t    einfo_;
} wusock_t;

/* Generic Interface (domain neutral)
 *    Currently supported domains:
 *      PF_UNIX (PF_LOCAL) - UNIX Domain Sockets
 *        stypes: (can assume all types are reliable with in order delivery)
 *          SOCK_STREAM - data appears as a byte stream
 *          SOCK_DGRAM  - to preserve message boundaries
 *        protocol: set to 0
 *        sockaddr data struct:
 *          struct sockaddr_un;
 *      PF_INET - IPv4 sockets
 *        stypes:
 *          SOCK_STREAM - TCP
 *          SOCK_DGRAM  - UDP
 *          SOCK_RAW    - Raw IPv4
 *        protocols:
 *          set to 0 (only one protocol per socket type).
 *        sockaddr data struct:
 *          struct sockaddr_in;
 *      Linux only: PF_PACKET - RAW link level sockets
 *        stypes:
 *          SOCK_RAW   - frames include link layer headers
 *          SOCK_DGRAM - frames stop at network layer header
 *        protocols:
 *          any valid ether type, see <linux/if_ether.h>
 *        sockaddr data struct:
 *          struct sockaddr_ll;
 */

/* For Dynamic object management */
wusock_t *sok_create(int domain, int stype, int proto);
void      sok_delete(wusock_t *sock);

/* Initialize internal state of object */
int       sok_init(wusock_t *sock, int domain, int stype, int proto);
void      sok_clean(wusock_t *sock);
void      sok_copy(wusock_t *oldsock, wusock_t *newsock); // destructive copy
int       sok_assign(wusock_t *sock, int sd);

/* Connection state */
int       sok_open(wusock_t *sock);
void      sok_close(wusock_t *sock);

static inline int sok_valid(wusock_t *sock);
static inline int sok_inError(wusock_t *sock);
static inline void sok_printerr(wusock_t *sock);

int       sok_bind(wusock_t *sock, const struct sockaddr *saddr);
int       sok_connect(wusock_t *sock, const struct sockaddr *serv);
int       sok_listen(wusock_t *sock);
wusock_t *sok_accept(wusock_t *sock);
wusock_t *sok_accept2(wusock_t *sock, wusock_t *newSock);

ssize_t   sok_read(wusock_t *sock, void *buf, size_t len);
ssize_t   sok_recv(wusock_t *sock, void *buf, size_t len);
ssize_t   sok_readn(wusock_t *sock, void *buf, size_t len);
ssize_t   sok_readv(wusock_t *sock, const struct iovec *vecs, int vcnt);
ssize_t   sok_rdfrom(wusock_t *sock, void *buf, size_t len, struct sockaddr *from);
ssize_t   sok_recvmsg(wusock_t *sock, struct msghdr *msg, int flags);
ssize_t   sok_write(wusock_t *sock, const void *buf, size_t len);
ssize_t   sok_writen(wusock_t *sock, const void *buf, size_t len);
ssize_t   sok_writev(wusock_t *sock, const struct iovec *vecs, int vcnt);
ssize_t   sok_writeto(wusock_t *sock, const void *buf, size_t len, struct sockaddr *to);
ssize_t   sok_sendmsg(wusock_t *sock, const struct msghdr *msg, int flags);

// Socket and Protocol options */
int sok_set_multicast_ttl(wusock_t *sock, int ttl);
int sok_set_ttl(wusock_t *sock, int ttl);
int sok_reuseaddr(wusock_t *sock, int val);
int sok_nodelay(wusock_t *sock, int val);
int sok_nonblock(wusock_t *sock, int on);
int sok_recvbuf(wusock_t *sock, int val);
int sok_sendbuf(wusock_t *sock, int val);
int sok_rxtout(wusock_t *sock, const struct timeval *tout);
int sok_txtout(wusock_t *sock, const struct timeval *tout);

void              sok_set_flag(wusock_t *sock, sockFlags_t flag, int val);
void              sok_retry(wusock_t *sock, int);
static inline int sok_isRetry(const wusock_t *);
static inline int sok_isAsync(const wusock_t *);

void        sok_print(const wusock_t *sock, const char *pre);
char       *sok_sa2str(const struct sockaddr *sa, char *buf, size_t blen);
char       *sok_desc2str(int sd, char *buf, size_t blen);

const char *sok_stype2str(int);
int         sok_stype2proto(int family, int stype);
int         sok_str2stype(const char *);

const char *sok_af2str(int);
int         sok_str2af(const char *);

const char *sok_iproto2str(int);
int         sok_str2iproto(const char *);

// Generic (proto neutral) Helper methods

void                     sok_update_saddr(wusock_t *sock, struct sockaddr *saddr);
void                     sok_get_peer(wusock_t *sock, struct sockaddr *saddr);
static inline size_t     sok_addr2size(const struct sockaddr *saddr);

static inline int        sok_stype(const wusock_t *sock);
static inline const char *sok_st2str(int stype);

static inline int        sok_domain(const wusock_t *sock);
static inline size_t     sok_dom2size(int domain);

static inline int        sok_proto(const wusock_t *sock);

wun_errType_t sok_ckneterr(wun_errInfo_t *einfo, int stype, int syserr, const char *who);
wun_errType_t sok_cksockerr(wusock_t *sock, int syserr, const char *who);

// if IPv4
struct sockaddr_in *sok_init_sin(struct sockaddr_in *sin, struct in_addr ipaddr, uint16_t port);
struct sockaddr_in *sok_init_sin2(struct sockaddr_in *sin, uint32_t ipaddr, uint16_t port);
char               *sok_addr2host_sin(struct in_addr addr, char *buf, size_t len);
int                 sok_getaddr_sin(const char *host, struct in_addr *inaddr);
char               *sok_sin2str(struct sockaddr_in *sin, char *buf, size_t blen);
void                sok_iaddr_print(const char *pre, struct sockaddr_in *iaddr);

// if UNIX
struct sockaddr_un *sok_init_sun(struct sockaddr_un *, const char *);
char               *sok_mkuname(const char *, const char *);
void                sok_rmuname(char *uname);
char               *sok_sun2str(struct sockaddr_un *uaddr, char *buf, size_t blen);
void                sok_uaddr_print(const char *, struct sockaddr_un *);

// packet
#ifndef __CYGWIN__
struct sockaddr_ll* sok_init_eth(struct sockaddr_ll *, uint16_t, int, uint8_t *);
char*               sok_sll2str(struct sockaddr_ll *sll, char *buf, size_t blen);
#endif
int                 sok_ifn2indx(char *ifn);

// ***************************************************************************
//                     inline'ed functions
// ***************************************************************************
// basic utility methods
static inline int sok_valid(wusock_t *sock)
{
  // it is possible that the et_ is set to an "error" condition but that the
  // socket implementation has decided that it is not a "real" error. For
  // example an interrupted system is not an error condition on the socket,
  // just on a particular operation.
  if (sock == NULL || sock->sd_ == -1 || sok_inError(sock))
    return 0;
  return 1;
}

static inline int sok_inError(wusock_t *sock)
{
  return sock->einfo_.el_ == makeNetLvlName(Err);
}

static inline void sok_printerr(wusock_t *sock)
{
  makeNetName(errPrint)(&sock->einfo_, sock->et_);
}

static inline int sok_isRetry(const wusock_t *sock)
{
  return sock->flags_ & sok_Flg_Retry;
}

static inline int sok_isAsync(const wusock_t *sock)
{
  return sock->flags_ & sok_Flg_Async;
}

static inline const char *sok_st2str(int st)
{
  return sok_stype2str(st);
}

static inline size_t sok_dom2size(int dom)
{
  size_t sz = 0;
  switch (dom) {
    case PF_UNIX:
#if PF_UNIX != PF_LOCAL
    case PF_LOCAL:
#endif
      sz = sizeof(struct sockaddr_un);
      break;
    case PF_INET:
      sz = sizeof(struct sockaddr_in);
      break;
#ifndef __CYGWIN__
    case PF_PACKET:
      sz = sizeof(struct sockaddr_ll);
      break;
#endif
    default:
      wulog(wulogNet, wulogWarn, "sok_dom2size: Unsupported protocol family %d\n", dom);
  }

  return sz;
}

static inline size_t sok_addr2size(const struct sockaddr *addr)
{
  if (addr == NULL)
    return 0;
  else if (addr->sa_family == PF_UNIX)
    return SUN_LEN((struct sockaddr_un *)addr);
  return sok_dom2size(addr->sa_family);
}

// *************************************************************************
// The wusock API

// valid domains: PF_UNIX/PCF_LOCAL, PF_INET
// socket types: SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET, SOCK_RAW,
//               SOCK_RDM, SOCK_PACKET
static inline int sok_domain(const wusock_t *sock)
{
  return (sock ? sock->dom_ : 0);
}
static inline int sok_proto(const wusock_t *sock)
{
  return (sock ? sock->pr_ : 0);
}
static inline int sok_stype(const wusock_t *sock)
{
  return (sock ? sock->st_ : 0);
}

#ifdef __cplusplus
} /* c++ */
#endif

#endif
