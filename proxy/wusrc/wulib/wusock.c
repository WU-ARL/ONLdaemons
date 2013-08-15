/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/wusock.c,v $
 * $Author: fredk $
 * $Date: 2008/02/22 16:36:32 $
 * $Revision: 1.43 $
 * $Name:  $
 *
 * File:         net.cc
 * Created:      01/05/2005 04:32:38 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */
#include <sys/uio.h>
// printf
#include <stdio.h>
// for malloc
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
// inet_ntoa
#include <arpa/inet.h>
// gethostbyname
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>


#ifndef __CYGWIN__
#  include <netpacket/packet.h>
#  include <net/ethernet.h>
#endif
#include <sys/ioctl.h>
#include <net/if.h>

#include <wulib/wulog.h>
#include <wulib/keywords.h>
#include <wulib/wusock.h>

/* static function declarations */

static void sockError(wusock_t *sock);
static inline ssize_t _sok_read(wusock_t *sock, void *buf, size_t len);
static inline ssize_t _sok_recv(wusock_t *sock, void *buf, size_t len, int flags);
static inline ssize_t _sok_write(wusock_t *sock, const void *buf, size_t len);
static inline ssize_t _sok_writev(wusock_t *sock, const struct iovec *vecs, int vcnt);
static inline ssize_t _sok_writeto(wusock_t *sock, const void *buf, size_t len, struct sockaddr *to);
static int sok_set_sockopt(wusock_t *, int, int, int);
static wusock_t *_sok_accept(wusock_t *sock, wusock_t *newSock);
static int sok_set_sockopt2(wusock_t *sock, int lvl, int cmd, const void *optval, socklen_t optlen);

/* Function definitions */
static void sockError(wusock_t *sock)
{
  if (sock->einfo_.el_ == makeNetLvlName(Err)) {
    sok_close(sock);
#ifdef WUSOCK_DEBUG
    makeNetName(errPrint)(&sock->einfo_, sock->et_);
#endif
  }
  return;
}

wusock_t *sok_create(int dom, int st, int pr)
{
  wusock_t *sock;

  if ((sock = (wusock_t *)calloc(1, sizeof(wusock_t))) == NULL) {
    wulog(wulogNet, wulogWarn, "sok_create: Error allocating memory for wusock\n");
    return NULL;
  }

  if (sok_init(sock, dom, st, pr) == -1) {
    free(sock);
    sock = NULL;
  }

  return sock;
}

int sok_assign(wusock_t *sock, int sd)
{
  socklen_t slen;
  int stype;
  struct sockaddr_storage saddr;

  slen = (socklen_t)sizeof(stype);
  if (getsockopt(sd, SOL_SOCKET, SO_TYPE, &stype, &slen) == -1)
    return -1;

  slen = sizeof(saddr);
  memset((void *)&saddr, 0, slen);
  if (getsockname(sd, (struct sockaddr *)&saddr, &slen) == -1)
    return -1;

  sok_init(sock, saddr.ss_family, stype, 0);
  sock->sd_ = sd;
  return 0;
}

int sok_init(wusock_t *sock, int dom, int st, int pr)
{
  WUASSERT(sock);

  sock->sd_    = -1;
  sock->dom_   = dom;
  sock->st_    = st;
  sock->pr_    = pr;

  sock->flags_ = 0;
  /* Error info */
  sock->et_ = wun_et_noError;
  wun_errReset(&sock->einfo_);

  return 0;
}

void sok_delete(wusock_t *sock)
{
  WUASSERT(sock);

  sok_clean(sock);
  free(sock);

  return;
}

void sok_copy(wusock_t *oldsock, wusock_t *newsock)
{
  WUASSERT(oldsock && newsock);

  // initialize any internal objects
  sok_init(newsock, oldsock->dom_, oldsock->st_, oldsock->pr_);
  newsock->sd_    = oldsock->sd_;
  newsock->flags_ = oldsock->flags_;
  newsock->et_    = oldsock->et_;
  memcpy((void *)&newsock->einfo_, (void *)&oldsock->einfo_, sizeof(makeNetTypeName(errInfo)));

  oldsock->sd_    = -1; // so calling sok_clean(oldsock) doesn't close an open socket
  sok_clean(oldsock);

  return;
}

void sok_clean(wusock_t *sock)
{
  WUASSERT(sock);

  // in case it is open
  sok_close(sock);
  sok_init(sock, -1, -1, -1);

  return;
}

int sok_open(wusock_t *sock)
{
  WUASSERT(sock);
  WUASSERT(sock->sd_ == -1);

  sock->et_ = wun_et_noError;
  if ((sock->sd_ = socket(sock->dom_, sock->st_, sock->pr_)) == -1) {
    sok_cksockerr(sock, errno, "sok_open");
    return -1;
  }

  // Its assumed that the flags_ field will always be zero when calling
  // sok_open. In other words, you can't manipulate a socket's state until it
  // has been opened.
  sock->flags_ = sok_Flg_Open;

  return sock->sd_;
}

void sok_close(wusock_t *sock)
{
  if (sock == NULL || sock->sd_ == -1)
    return;
  close(sock->sd_);
  sock->sd_ = -1;
  // Want to leave the last error visible so do not reset sock->et_
  sock->flags_ = 0;
  return;
}


void sok_update_saddr(wusock_t *sock, struct sockaddr *saddr)
{
  socklen_t len;

  WUASSERT(sock && saddr);

  sock->et_ = wun_et_noError;
  len = sok_dom2size(sock->dom_);
  if (getsockname(sock->sd_, saddr, &len) == -1)
    sok_cksockerr(sock, errno, "sok_update_saddr");

  return;
}

void sok_get_peer(wusock_t *sock, struct sockaddr *saddr)
{
  // XXX should verify that socket is connection oriented
  socklen_t len;

  WUASSERT(sock && saddr);

  sock->et_ = wun_et_noError;
  len = sok_dom2size(sock->dom_);
  if (getpeername(sock->sd_, saddr, &len) == -1)
    sok_cksockerr(sock, errno, "sok_get_peer");

  return;
}

static inline ssize_t _sok_read(wusock_t *sock, void *buf, size_t len)
{
  ssize_t n;

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = read(sock->sd_, buf, len)) <= 0) {
      if (n == 0)
        sock->et_ = makeNetErrName(EndOfFile);
      else
        sok_cksockerr(sock, errno, "_sok_read");
    }
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock));

  return n;
}

static inline ssize_t _sok_recv(wusock_t *sock, void *buf, size_t len, int flags)
{
  ssize_t n;

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = recv(sock->sd_, buf, len, flags)) <= 0) {
      if (n == 0)
        sock->et_ = makeNetErrName(EndOfFile);
      else
        sok_cksockerr(sock, errno, "_sok_recv");
    }
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock));

  return n;
}


ssize_t sok_read(wusock_t *sock, void *buf, size_t len)
{
  WUASSERT(sock && buf != 0);
  return _sok_read(sock, buf, len);
}

ssize_t sok_readn(wusock_t *sock, void *buf, size_t len)
{
  // n == 0 on EOF
  int nleft = len, n;
  char *ptr = (char *)buf;

  WUASSERT(sock && buf != 0);

  sock->et_ = wun_et_dataSuccess; // in case nleft starts at 0
  while (nleft > 0) {
#ifdef __CYGWIN__
    n = _sok_recv(sock, buf, nleft, 0);
#else
    n = _sok_recv(sock, buf, nleft, MSG_WAITALL);
#endif
    if (n == -1)
      return -1;

    nleft -= n;
    ptr   += n;
  }

  return len;
}

ssize_t sok_readv(wusock_t *sock, const struct iovec *vecs, int vcnt)
{
  int n;

  WUASSERT(sock);

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = readv(sock->sd_, vecs, vcnt)) <= 0) {
      if (n == 0)
        sock->et_ = makeNetErrName(EndOfFile);
      else
        sok_cksockerr(sock, errno, "sok_readv");
    }
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock));

  return n;
}

ssize_t sok_rdfrom(wusock_t *sock, void *buf, size_t len, struct sockaddr *from)
{
  int n;
  socklen_t slen, *slptr;

  WUASSERT(sock);
  slptr = from ? &slen : NULL;

  sock->et_ = wun_et_dataSuccess;
  do {
    slen = from ? sok_dom2size(sock->dom_) : 0;
    if ((n = recvfrom(sock->sd_, buf, len, 0, from, slptr)) <= 0) {
      if (n == 0)
        sock->et_ = makeNetErrName(EndOfFile);
      else
        sok_cksockerr(sock, errno, "sok_rdfrom");
    }
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock)) ;

  return n;
}

ssize_t sok_recvmsg(wusock_t *sock, struct msghdr *msg, int flags)
{
  int n;
  WUASSERT(sock);

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = recvmsg(sock->sd_, msg, flags)) <= 0) {
      if (n == 0)
        sock->et_ = makeNetErrName(EndOfFile);
      else
        sok_cksockerr(sock, errno, "sok_recvmsg");
    }
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock)) ;

  return n;
}

static inline ssize_t _sok_write(wusock_t *sock, const void *buf, size_t len)
{
  int n;

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = write(sock->sd_, buf, len)) == -1)
      sok_cksockerr(sock, errno, "_sok_write");
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock));

  return n;
}

static inline ssize_t _sok_writev(wusock_t *sock, const struct iovec *vecs, int vcnt)
{
  int n;

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = writev(sock->sd_, vecs, vcnt)) == -1)
      sok_cksockerr(sock, errno, "_sok_writev");
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock));

  return n;
}

static inline ssize_t _sok_writeto(wusock_t *sock, const void *buf, size_t len, struct sockaddr *to)
{
  int n;
  socklen_t slen = sok_addr2size(to);

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = sendto(sock->sd_, buf, len, 0, to, slen)) == -1)
      sok_cksockerr(sock, errno, "_sok_writeto");
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock));

  return n;
}

ssize_t sok_sendmsg(wusock_t *sock, const struct msghdr *msg, int flags)
{
  int n;
  WUASSERT(sock);

  sock->et_ = wun_et_dataSuccess;
  do {
    if ((n = sendmsg(sock->sd_, msg, flags)) == -1)
      sok_cksockerr(sock, errno, "_sok_sendmsg");
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock));

  return n;
}

ssize_t sok_write(wusock_t *sock, const void *buf, size_t len)
{
  WUASSERT(sock && buf);
  return _sok_write(sock, buf, len);
}

ssize_t sok_writev(wusock_t *sock, const struct iovec *vecs, int vcnt)
{
  WUASSERT(sock && vecs);
  return _sok_writev(sock, vecs, vcnt);
}

ssize_t sok_writeto(wusock_t *sock, const void *buf, size_t len, struct sockaddr *to)
{
  WUASSERT(sock && buf);
  return _sok_writeto(sock, buf, len, to);
}

ssize_t sok_writen(wusock_t *sock, const void *buf, size_t len)
{
  int nleft, n;
  const char *ptr;

  WUASSERT(sock && buf);

  ptr = (char *)buf; nleft = len;
  do {
    if ((n = _sok_write(sock, buf, nleft)) == -1)
      return -1;

    nleft -= n;
    ptr   += n;
  } while (nleft > 0);

  return len;
}

int sok_bind(wusock_t *sock, const struct sockaddr *saddr)
{
  WUASSERT(sock && saddr);

  sock->et_ = wun_et_noError;
  if (bind(sock->sd_, saddr, sok_addr2size(saddr)) == -1) {
    sok_cksockerr(sock, errno, "sok_bind");
    return -1;
  }

  return 0;
}

// establishing connects
int sok_connect(wusock_t *sock, const struct sockaddr *serv)
{
  WUASSERT(sock && serv);

  sock->et_ = wun_et_noError;
  if (connect(sock->sd_, serv, sok_addr2size(serv)) == -1) {
    sok_cksockerr(sock, errno, "sok_connect");
    return -1;
  }
  sok_set_flag(sock, sok_Flg_Connect, 1);
  return 0;
}

int sok_listen(wusock_t *sock)
{
  WUASSERT(sock);

  sock->et_ = wun_et_noError;
  if (listen(sock->sd_, WUNET_SOCK_BACKLOG_DEF) == -1) {
    sok_cksockerr(sock, errno, "sok_listen");
    return -1;
  }
  sok_set_flag(sock, sok_Flg_Listen, 1);
  return 0;
}

static wusock_t *_sok_accept(wusock_t *sock, wusock_t *newSock)
{
  do {
    sock->et_ = wun_et_noError;
   if ((newSock->sd_ = accept(sock->sd_,(struct sockaddr *) NULL, NULL)) == -1)
      sok_cksockerr(sock, errno, "_sok_accept");
  } while (sock->et_ == wun_et_intrSysCall && sok_isRetry(sock)) ;

  if (newSock->sd_ == -1)
    return NULL;

  return newSock;
}

wusock_t *sok_accept(wusock_t *sock)
{
  wusock_t *newSock;
  WUASSERT(sock);

  if ((newSock = sok_create(sock->dom_, sock->st_, sock->pr_)) == NULL) {
    wulog(wulogNet, wulogError, "sock_accept: Unable to allocate new socket object\n");
    return NULL;
  }

  if (_sok_accept(sock, newSock) == NULL) {
    sok_delete(newSock);
    newSock = NULL;
  }

  return newSock;
}

wusock_t *sok_accept2(wusock_t *sock, wusock_t *newSock)
{
  WUASSERT(sock && newSock);
  sok_init(newSock, sock->dom_, sock->st_, sock->pr_);

  return _sok_accept(sock, newSock);
}

int sok_stype2proto(int family, int stype)
{
  int proto;
  if (family == PF_INET || family == PF_INET6) {
    switch(stype) {
      case SOCK_STREAM:
        proto = IPPROTO_TCP;
        break;
      case SOCK_DGRAM:
        proto = IPPROTO_UDP;
        break;
      default:
        proto = 0;
    }
  } else {
    // there is no generic mapping for UNIX (a value of 0 is always used)
    // and for raw sockets a specific protocol must be specified.
    proto = 0;
  }
  return proto;
}

// define max expected string length
KeyInfo_t sok_AF2StrMap[] = {
  {.flag = AF_INET,            .name = "inet",   .desc = "Address family for IPv4"},
//  {.flag = AF_INET6,           .name = "inet6",  .desc = "Address family for IPv6"},
  {.flag = AF_UNIX,            .name = "unix",   .desc = "Address family for UNIX domain sockets"},
  {.flag = AF_LOCAL,           .name = "local",  .desc = "Another name for UNIX domain sockets"},
#ifndef __CYGWIN__
  {.flag = AF_PACKET,          .name = "packet", .desc = "Address family for RAW Link layer sockets"},
#endif
  {.flag = AF_UNSPEC,          .name = "unspec", .desc = "Unspecified address family"},
  WULIB_KEYINFO_NULL_ENTRY
};

// socket type to string
KeyInfo_t sok_SType2Str[] = {
  {.flag = SOCK_STREAM,    .name = "stream", .desc = "Stream oriented protocols (TCP in ipv4)"},
  {.flag = SOCK_DGRAM,     .name = "dgram",  .desc = "Datagram oriented protocols (UDP in ipv4)"},
  {.flag = SOCK_RAW,       .name = "raw",    .desc = "Raw or Network layer protocols"},
  {.flag = SOCK_RDM,       .name = "rdm",    .desc = "Reliable Delivery, message oriented"},
  {.flag = SOCK_SEQPACKET, .name = "seqpkt", .desc = "Sequenced, reliable, connection-orienmted, datagrams"},
#ifdef SOCK_PACKET
  {.flag = SOCK_PACKET,    .name = "pkt",    .desc = "Linux Packet"},
#endif
  WULIB_KEYINFO_NULL_ENTRY
};

// just some of them
KeyInfo_t sok_IPProto2Str[] = {
  {.flag = IPPROTO_IP,   .name = "ipv4", .desc = "IP"},
  {.flag = IPPROTO_ICMP, .name = "icmp", .desc = "ICMP"},
  {.flag = IPPROTO_TCP,  .name = "tcp",  .desc = "TCP"},
  {.flag = IPPROTO_UDP,  .name = "udp",  .desc = "UDP"},
//  {.flag = IPPROTO_IPV6, .name = "ipv6", .desc = "IPV6"},
  {.flag = IPPROTO_RAW,  .name = "raw",  .desc = "Raw IP"},
  WULIB_KEYINFO_NULL_ENTRY
};

static const char *sok_Unknown = "Unknown";

const char *sok_af2str(int af)
{
  const char *str = flag2str((wuKeyFlag_t)af, sok_AF2StrMap);
  if (str == NULL)
    str = sok_Unknown;
  return str;
}
int sok_str2af(const char *str) {return (int)str2flag(str, sok_AF2StrMap);}

const char *sok_stype2str(int st)
{
  const char *str =  flag2str((wuKeyFlag_t)st, sok_SType2Str);
  if (str == NULL)
    str = sok_Unknown;
  return str;
}
int sok_str2stype(const char *str) {return (int)str2flag(str, sok_SType2Str);}

const char *sok_iproto2str(int pr)
{
  const char *str = flag2str((wuKeyFlag_t)pr, sok_IPProto2Str);
  if (str == NULL)
    str = sok_Unknown;
  return str;
}
int sok_str2iproto(const char *str) {return (int)str2flag(str, sok_IPProto2Str);}

char *sok_sa2str(const struct sockaddr *sa, char *buf, size_t blen)
{
  if (buf == NULL) return NULL;
  if (sa == NULL) {buf[0] = '\0'; return buf;}

  switch(sa->sa_family) {
    case PF_INET:
      sok_sin2str((struct sockaddr_in *)sa, buf, blen);
      break;
    case PF_UNIX:
      sok_sun2str((struct sockaddr_un *)sa, buf, blen);
      break;
#ifndef __CYGWIN__
    case PF_PACKET:
      sok_sll2str((struct sockaddr_ll *)sa, buf, blen);
      break;
#endif
    default:
      buf[0] = '\0';
      strncat(buf, "Unknown", blen-1);
      break;
  }
  return buf;
}

char *sok_desc2str(int sd, char *buf, size_t blen)
{
  struct sockaddr sname;
  socklen_t slen = sizeof(struct sockaddr);
  int n, stype;
  char *addrBuf;
  
  if (buf == NULL || blen == 0)
    return NULL;

  buf[0] = '\0';
  if (getsockname(sd, &sname, &slen) == -1) {
    wulog(wulogNet, wulogWarn, "sok_desc2str: Error calling getsockname. errno = %d\n\t%s\n",
        errno, strerror(errno));
    return buf;
  }
  slen = sizeof(stype);
  if (getsockopt(sd, SOL_SOCKET, SO_TYPE, (void *)&stype, &slen) == -1) {
    wulog(wulogNet, wulogWarn, "sok_desc2str: Error getting SO_TYPE, errno %d, %s\n", errno, strerror(errno));
    return buf;
  }

  addrBuf = malloc(WULOG_MSG_MAXLEN);
  if (sok_sa2str(&sname, addrBuf, WULOG_MSG_MAXLEN) == NULL)
    addrBuf[0] = '\0';

  n = snprintf(buf, blen, "sd %d, type %d (%s), addr %s",
      sd, stype, sok_stype2str(stype), addrBuf);
  free(addrBuf);

  if (n >= (int)blen)
    buf[blen-1] = '\0';
  return buf;
}

void sok_print(const wusock_t *sock, const char *pre)
{
  char *descBuf;
  char *addrBuf;

  if ((addrBuf = malloc(WULOG_MSG_MAXLEN)) == NULL) {
    wulog(wulogNet, wulogWarn, "sok_print: Unable to allocate buffer memory\n");
    return;
  }

  if ((descBuf = malloc(WULOG_MSG_MAXLEN)) == NULL) {
    free(addrBuf);
    wulog(wulogNet, wulogWarn, "sok_print: Unable to allocate buffer memory\n");
    return;
  }

  printf("%ssd %d, st %d (%s), flags %x, et %d (%s), Addr [%s]\n",
         (pre ? pre : "Sock: "),
         sock->sd_,
         sock->st_, sok_stype2str(sock->st_),
         sock->flags_,
         (int)sock->et_, wun_err2str(sock->et_),
         sok_desc2str(sock->sd_, descBuf, WULOG_MSG_MAXLEN));

  free(descBuf);
  free(addrBuf);
  return;
}

wun_errType_t sok_ckneterr(wun_errInfo_t *einfo, int stype, int syserr, const char *who)
{
  wun_errType_t etype;
  switch (syserr) {
    case 0:
      etype = wun_et_noError;
      wun_errInit(einfo, makeNetLvlName(Info), syserr, who, "No Error");
      break;
    case EADDRINUSE:
      etype = wun_et_dupError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "address in use");
      break;
    case EAFNOSUPPORT:
    case EPROTONOSUPPORT:
      etype = wun_et_badProto;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "address domain or protocol not supported");
      break;
    case ETIMEDOUT:
      etype = wun_et_timeOut;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "connection timedout (ETIMEDOUT)");
      break;
    case EALREADY:
      etype = wun_et_dupError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "duplicate operation (connect)");
      break;
    case EINPROGRESS:
      // connect error: returned on non-blocking sockets
      // can use select to check on status.
      etype = wun_et_inProgress;
      wun_errInit(einfo, makeNetLvlName(Info), syserr, who, "Operation in progress");
      break;
    case ECONNREFUSED:
      if (stype == SOCK_DGRAM) {
        etype = wun_et_dstError;
        wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "UDP socket must have received an ICMP not reachable");
      } else if (stype == SOCK_STREAM) {
        etype = wun_et_dstError;
        wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "TCP connection refused, is the server running?");
      } else {
        etype = wun_et_dstError;
        wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "connection refused");
      }
      break;
    case ECONNRESET:
      etype = wun_et_connFailure;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "reset by peer (ECONNRESET)");
      break;
    case EPIPE:
      etype = wun_et_connFailure;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "other end has closed (EPIPE)");
      break;
    case EINTR:
      // Similar behavior as EAGAIN in that we assume we
      // would have blocked (or were blocked) when a signal arrived.
      etype = wun_et_intrSysCall;
      wun_errInit(einfo, makeNetLvlName(Info), syserr, who, "EINTR");
      break;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      // EAGAIN => Resource temporarily unavailable
      // EWOULDBLOCK => operation would block, this must be a non-blocking socket
      // EWOULDBLOCK == EAGAIN on Linux. This is a temporary error, does not
      // imply this process is already using to many resources.
      etype = wun_et_dataNone;
      wun_errInit(einfo, makeNetLvlName(Info), syserr, who, "EAGAIN/EWOULDBLOCK");
      break;
    case EINVAL:
      etype = wun_et_paramError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "Invalid parameter");
      break;
    case EMFILE:
    case ENFILE:
      etype = wun_et_resourceErr;
      wun_errInit(einfo, makeNetLvlName(Warn), syserr, who, "no file descriptors available (EMFILE/ENFILE)");
      break;
    case ENETUNREACH:
      etype = wun_et_netError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "No route to host");
      break;
    case ENOBUFS:
    case ENOMEM:
      etype = wun_et_resourceErr;
      wun_errInit(einfo, makeNetLvlName(Warn), syserr, who, "no resources (ENOBUFS/ENOMEM)");
      break;
    case ECONNABORTED:
      etype = wun_et_dstError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "accept - 3-way handshake aborted");
      break;
    case EACCES:
    case EPERM:
      etype = wun_et_authError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "permission error");
      break;
    case EBADF:
      etype = wun_et_paramError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "invalid socket descriptor");
      break;
    case ENOTSOCK:
      etype = wun_et_paramError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "Descriptor is not a socket");
      break;
    case EMSGSIZE:
      etype = wun_et_paramError;
      wun_errInit(einfo, makeNetLvlName(Warn), syserr, who, "Too large of a message");
      break;
    case EISCONN:
      etype = wun_et_dupError;
      wun_errInit(einfo, makeNetLvlName(Warn), syserr, who, "Socket already connected");
      break;
    case EFAULT:
      etype = wun_et_addrError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "Socket address is outside process namespace");
      break;
    default:
      etype = wun_et_sysError;
      wun_errInit(einfo, makeNetLvlName(Err), syserr, who, "Unexpected system error!");
      break;
  }

  return etype;
}

// Guarantee the following for stat:
//     if EAGAIN : wun_et_dataNone
//     else stat indicates the error
wun_errType_t sok_cksockerr(wusock_t *sock, int syserr, const char *who)
{
  if (sock == NULL) return wun_et_noError;
  sock->et_ = sok_ckneterr(&sock->einfo_, sok_stype(sock), syserr, who);
  if (wun_isError(sock->et_))
    sockError(sock);
  return sock->et_;
}
/****************************************************************************
 *                      Socket Options/State                                *
 ****************************************************************************/
int sok_ifn2indx(char *ifn)
{
  struct ifreq req;
  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd == -1) {
    wulog(wulogNet, wulogWarn, "sok_ifn2indx: Error opening generic socket\n\terrno %d, %s\n",
          errno, strerror(errno));
    return -1;
  }
  if (strlen(optarg)  >= IFNAMSIZ) {
    wulog(wulogNet, wulogWarn, "sok_ifn2indx: Interface name (%s) is too long (>%d)\n",
          ifn, IFNAMSIZ);
    return -1;
  }

  strcpy(req.ifr_name, ifn);
#ifndef __CYGWIN__
  if (ioctl(sd, SIOCGIFINDEX, (void *)&req) == -1) {
    if (errno == ENODEV)
      wulog(wulogSession, wulogError, "sok_ifn2indx: No device named %s\n", ifn);
    else if (errno == EPERM)
      wulog(wulogSession, wulogError, "sok_ifn2indx: Permission error opening generic (INET) Socket\n");
    else
      wulog(wulogSession, wulogError, "sok_ifn2indx: ioctl to get ifindex fauled\n\t errno = %d, %s\n",
            errno, strerror(errno));
    return -1;
  }
  return req.ifr_ifindex;
#else
  return 0;
#endif

}

static int sok_set_sockopt(wusock_t *sock, int lvl, int cmd, int val)
{
  return sok_set_sockopt2(sock, lvl, cmd, (const void *)&val, sizeof(val));
}

static int sok_set_sockopt2(wusock_t *sock, int lvl, int cmd,
                            const void *optval, socklen_t optlen)
{
  WUASSERT(sock);
  sock->et_ = wun_et_noError;
  if (setsockopt(sock->sd_, lvl, cmd, optval, optlen) == -1) {
    sok_cksockerr(sock, errno, "sok_set_sockopt");
    return -1;
  }
  switch(cmd) {
    case SO_REUSEADDR:
      sok_set_flag(sock, sok_Flg_ReuseAddr, *(int *)optval);
      break;
    case TCP_NODELAY:
      sok_set_flag(sock, sok_Flg_NoDelay, *(int *)optval);
      break;
    case SO_RCVTIMEO:
      {
        struct timeval *tval = (struct timeval *)optval;
        if (tval->tv_sec == 0 && tval->tv_usec == 0)
          sok_set_flag(sock, sok_Flg_RxTout, 0);
        else
          sok_set_flag(sock, sok_Flg_RxTout, 1);
      }
      break;
    case SO_SNDTIMEO:
      {
        struct timeval *tval = (struct timeval *)optval;
        if (tval->tv_sec == 0 && tval->tv_usec == 0)
          sok_set_flag(sock, sok_Flg_TxTout, 0);
        else
          sok_set_flag(sock, sok_Flg_TxTout, 1);
      }
      break;
  }

  return 0;
}

int sok_set_ttl(wusock_t *sock, int ttl)
{
  return sok_set_sockopt(sock, IPPROTO_IP, IP_TTL, ttl);
}

int sok_set_multicast_ttl(wusock_t *sock, int ttl)
{
  return sok_set_sockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, ttl);
}

int sok_sendbuf(wusock_t *sock, int val)
{
  return sok_set_sockopt(sock, SOL_SOCKET, SO_RCVBUF, val);
}

int sok_recvbuf(wusock_t *sock, int val)
{
  return sok_set_sockopt(sock, SOL_SOCKET, SO_RCVBUF, val);
}

int sok_reuseaddr(wusock_t *sock, int val)
{
  return sok_set_sockopt(sock, SOL_SOCKET, SO_REUSEADDR, val);
}

int sok_nodelay(wusock_t *sock, int val)
{
  return sok_set_sockopt(sock, IPPROTO_TCP, TCP_NODELAY, val);
}

int sok_rxtout(wusock_t *sock, const struct timeval *tout)
{
  return sok_set_sockopt2(sock, SOL_SOCKET, SO_RCVTIMEO, (const void *)tout, sizeof(struct timeval));
}

int sok_txtout(wusock_t *sock, const struct timeval *tout)
{
  return sok_set_sockopt2(sock, SOL_SOCKET, SO_SNDTIMEO, (const void *)tout, sizeof(struct timeval));
}

void sok_set_flag(wusock_t *sock, sockFlags_t flag, int val)
{
  if (val)
    sock->flags_ |= flag;
  else
    sock->flags_ &= ~flag;
}

void sok_retry(wusock_t *sock, int val)
{
  sok_set_flag(sock, sok_Flg_Retry, val);
}

// Manipulating the Socket state/config params
int sok_nonblock(wusock_t *sock, int on)
{
  int flags;

  WUASSERT(sock);

  sock->et_ = wun_et_noError;
  if ((flags = fcntl(sock->sd_, F_GETFL)) == -1) {
    sok_cksockerr(sock, errno, "sok_nonblock1");
    return -1;
  }

  if (on)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;

  if (fcntl(sock->sd_, F_SETFL, flags) == -1) {
    sok_cksockerr(sock, errno, "sok_nonblock2");
    return -1;
  }

  sok_set_flag(sock, sok_Flg_Async, on);
  return 0;
}

/****************************************************************************
 *                           INET Domain Sockets                            *
 ****************************************************************************/
int sok_getaddr_sin(const char *host, struct in_addr *inaddr)
{
  struct hostent *hp;

  if (host == NULL || *host == '\0') {
    inaddr->s_addr = htonl(INADDR_ANY);
    return 0;
  }

  if (isdigit((int)*host)) {
    if (inet_aton((const char *)host, inaddr) == 0) {
      wulog(wulogNet, wulogWarn, "getaddr: inet_aton returned error for %s\n", host);
      return -1;
    }
  } else {
    /* got a hostname, we need to do a DNS/hosts lookup */
    if ((hp = gethostbyname(host)) == NULL) {
      wulog(wulogNet, wulogWarn, "getaddr: Invalid host name/addr: %s\n", host);
      return -1;
    }
    inaddr->s_addr = ((struct in_addr *)hp->h_addr)->s_addr;
  }

  return 0;
}

// assume ipaddr and port are already in NBO
struct sockaddr_in *sok_init_sin(struct sockaddr_in *sin, struct in_addr ipaddr, uint16_t port)
{
  return sok_init_sin2(sin, ipaddr.s_addr, port);
}

struct sockaddr_in *sok_init_sin2(struct sockaddr_in *sin, uint32_t ipaddr, uint16_t port)
{
  if (sin == NULL)
    return NULL;
  memset((void *)sin, 0, sizeof(struct sockaddr_in));
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = ipaddr;
  sin->sin_port   = port;

  return sin;
}

char *sok_addr2host_sin(struct in_addr addr, char *buf, size_t len)
{
  char *str = inet_ntoa(addr);
  if (buf == NULL || len < 2)
    return NULL;
  buf[0] = '\0';

  if (str == NULL)
    return buf;

  strncat(buf, str, len);
  return buf;
}

char *sok_sin2str(struct sockaddr_in *sin, char *buf, size_t blen)
{
  int n;

  if (buf == NULL) return buf;
  if (sin == NULL) {buf[0] = '\0'; return buf;}

  n = snprintf(buf, blen, "%s:%s/%d",
               sok_af2str(sin->sin_family),
               inet_ntoa(sin->sin_addr),
               (int)ntohs(sin->sin_port));

  if (n > 0 && (size_t)n >= blen)
    buf[blen-1] = '\0';

  return buf;
}

/****************************************************************************
 *                           UNIX Domain Sockets                            *
 ****************************************************************************/
char *sok_mkuname(const char *dir, const char *fname)
{
  size_t len;
  char *uname;

  if (fname == NULL)
    return NULL;
  // add 2 to strlen: dir'/'fname'\0'
  len = strlen(fname) + (dir ? strlen(dir)+2 : 2);
  if ((uname = malloc(len)) == NULL) {
    wulog(wulogNet, wulogWarn, "sok_mkuname: Unable to allocate memory for string\n");
    return NULL;
  }
  if (dir)
    sprintf(uname, "%s/%s", dir, fname);
  else
    strcpy(uname, fname);

  return uname;
}

void sok_rmuname(char *uname)
{
  if (uname == NULL) return;
  *uname = '\0'; // just being cautious
  free(uname);
}

char *sok_sun2str(struct sockaddr_un *sun, char *buf, size_t blen)
{
  int n;

  if (buf == NULL) return NULL;
  if (sun == NULL) {buf[0] = '\0'; return buf;}

  n = snprintf(buf, blen, "%s:%s",
               sok_af2str(sun->sun_family),
               (sun->sun_path ? sun->sun_path : "NULL"));

  if (n > 0 && (size_t)n >= blen)
    buf[blen-1] = '\0';

  return buf;
}

void sok_iaddr_print(const char *pre, struct sockaddr_in *sin)
{
  printf("%s ip %s, port %d\n",
         (pre ? pre : "ipAddr:"),
         (sin ? inet_ntoa(sin->sin_addr) : "Null"),
         (sin ? (int)ntohs(sin->sin_port) : 0));
  return;
}

void sok_uaddr_print(const char *pre, struct sockaddr_un *uaddr)
{
  if (uaddr == NULL) {
    printf("sok_uaddr_print: uaddr == NULL\n");
    return ;
  }
  printf("%s sun_family = %d, sun_path = %s\n",
      (pre ? pre : "sok_uaddr_print:"),
      uaddr->sun_family,
      (uaddr->sun_path ? uaddr->sun_path : "NULL"));
  return;
}

struct sockaddr_un *sok_init_sun(struct sockaddr_un *sun, const char *fname)
{
  if (sun == NULL)
    return NULL;

  memset((void *)sun, (int)0, sizeof(struct sockaddr_un));
  sun->sun_family = AF_UNIX;
  if (fname)
    wustrncpy(sun->sun_path, fname, sizeof(sun->sun_path)-1);
  else
    *sun->sun_path = '\0';
  return sun;
}

#ifndef __CYGWIN__
/****************************************************************************
 *                           Link Layer Sockets                             *
 ****************************************************************************/
struct sockaddr_ll *sok_init_eth(struct sockaddr_ll *sll, uint16_t pr, int ifindx, uint8_t *addr)
{
  int indx;
  if (sll == NULL) return NULL;

  memset((void *)sll, 0, sizeof(struct sockaddr_ll));
  sll->sll_family   = PF_PACKET;
  sll->sll_protocol = pr;
  sll->sll_ifindex  = ifindx;
  sll->sll_halen    = ETH_ALEN;
  for (indx = 0; indx < ETH_ALEN; ++indx)
    sll->sll_addr[indx] = addr ? addr[indx] : 0;

  return sll;
}

char *sok_sll2str(struct sockaddr_ll *sll, char *buf, size_t blen)
{
  int n, tmp, indx=0;
  size_t x;

  if (buf == NULL) return buf;
  if (sll == NULL) {buf[0] = '\0'; return buf;}

  n = snprintf(buf, blen, "%s: ifindx %d, etype %d Addr ",
               sok_af2str(sll->sll_family),
               sll->sll_ifindex,
               (int)ntohs(sll->sll_protocol));
  if (n < 0) {buf[0] = '\0'; return buf;}
  if ((size_t)n >= blen) {buf[blen-1] = '\0'; return buf;}

  x = n + sll->sll_halen * 3 + 1; // need 3 chars per byte XX:
  if (x <= blen) {
    char *ptr = (char *)buf+n;
    size_t len = blen-n;
    for(indx = 0; (size_t)n < blen && indx < sll->sll_halen; ++indx) {
      tmp = snprintf(ptr, len, "%02x:", (unsigned char)sll->sll_addr[indx]);
      if (tmp < 0) {buf[0] = '\0'; return buf;}
      n += tmp;
      ptr += tmp;
    }
    buf[n-1] = '\0'; // just in case
  }

  return buf;
}
#endif
