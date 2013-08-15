/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/Attic/sock.cc,v $
 * $Author: fredk $
 * $Date: 2007/01/25 18:45:55 $
 * $Revision: 1.17 $
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
#include <iostream>
#include <vector>
#include <sstream>

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <wulib/wulog.h>

#include <wuPP/net.h>
#include <wuPP/addr.h>
#include <wuPP/sock.h>

// only handles the simple cases
int wunet::inetSock::proto2stype(int proto)
{
  int stype = SOCK_RAW;
  switch (proto) {
    case IPPROTO_IP:
      stype = SOCK_RAW;
      break;
    case IPPROTO_TCP:
      stype = SOCK_STREAM;
      break;
    case IPPROTO_UDP:
      stype = SOCK_DGRAM;
      break;
  }
  // IPPROTO_IP IPPROTO_HOPOPTS IPPROTO_ICMP IPPROTO_IGMP IPPROTO_IPIP
  // IPPROTO_TCP IPPROTO_EGP IPPROTO_PUP IPPROTO_UDP IPPROTO_IDP IPPROTO_TP
  // IPPROTO_IPV6 IPPROTO_ROUTING IPPROTO_FRAGMENT IPPROTO_RSVP IPPROTO_GRE
  // IPPROTO_ESP IPPROTO_AH IPPROTO_ICMPV6 IPPROTO_NONE IPPROTO_DSTOPTS
  // IPPROTO_MTP IPPROTO_ENCAP IPPROTO_PIM IPPROTO_COMP IPPROTO_SCTP
  // IPPROTO_RAW IPPROTO_MAX
  // SOCK_STREAM SOCK_DGRAM  SOCK_RAW  SOCK_RDM  SOCK_SEQPACKET  SOCK_PACKET
  return stype;
}

void wunet::inetSock::updateSelf(int sd, inetAddr& addr)
{
  struct sockaddr_in ina;
  socklen_t len = (socklen_t)sizeof(struct sockaddr_in);
  if (getsockname(sd, (struct sockaddr *)&ina, &len) < 0) {
    if (errno == ENOBUFS) {
      sockwarn(wupp::resourceErr, "inetSock::updateSelf", errno);
    } else {
      sockwarn(wupp::progError, "inetSock::updateSelf", errno);
    }
    return ; // should never reach this line
  }
  if (addr.updateAddr(ina) < 0)
    sockerr(wupp::progError, "inetSock::updateSelf: update address failed.");

  return;
}

void wunet::inetSock::updatePeer(int sd, inetAddr& addr)
{
  struct sockaddr_in ina;
  socklen_t len = (socklen_t)sizeof(struct sockaddr_in);
  if (getpeername(sd, (struct sockaddr *)&ina, &len) < 0) {
    if (errno == ENOBUFS) {
      sockwarn(wupp::resourceErr, "inetSock::updatePeer", errno);
    } else {
      sockwarn(wupp::progError, "inetSock::updatePeer", errno);
    }
  }
  if (addr.updateAddr(ina) < 0)
    sockerr(wupp::progError, "inetSock::updatePeer: update address failed.");

  return;
}

void wunet::inetSock::sockinfo(wupp::errType et, const std::string& lm, int err)
{
  wulog(wulogNet, wulogInfo, "wunet::inetSock: %s\n\tet = 0x%08x, err = %d\n", lm.c_str(), (unsigned int)et, err);
}

void wunet::inetSock::sockwarn(wupp::errType et, const std::string& lm, int err)
{
  bs_ = badSock(et, lm, err);
  throw bs_;
}

void wunet::inetSock::sockerr(wupp::errType et, const std::string& lm, int err)
{
  this->close();
  setFlag(Error, true);
  bs_ = badSock(et, lm, err);
  throw bs_;
}

void
wunet::inetSock::open()
{
  if ((sd_ = ::socket(self_.family(), st_, 0)) == -1) {
    switch (errno) {
      case EAFNOSUPPORT:
      case EPROTONOSUPPORT:
        sockerr(wupp::paramError, "inetSock::open parameter error", errno);
        break;
      case EMFILE:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
        sockerr(wupp::resourceErr, "inetSock::open: no resources", errno);
        break;
      default:
        sockerr(wupp::progError, "inetSock::open: error opening sock", errno);
        break;
    }
    // in all cases above an exception is thrown
  }

  if (!self_.isAnyAddr() || !self_.isAnyPort()) {
    reuseaddr(1);
    if (::bind(sd_, self_.sockAddr(), self_.sockLen()) < 0) {
      SockErrLog(wupp::progError, "inetSock::open: error calling bind()", errno);
    }
  }
  if (self_.isWildcard())
    wunet::inetSock::updateSelf(sd_, self_);

  setFlag(Open, true);
  return;
}

void wunet::inetSock::reuseaddr(int val)
{
  if (setsockopt(sd_, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val)) < 0) {
    SockErrLog(wupp::progError, "inetSock::reuseaddr: error setting SO_REUSEADDR", errno);
  }
  setFlag(ReuseAddr, val);
}

void wunet::inetSock::nodelay(int val)
{
  if (setsockopt(sd_, self_.proto(), TCP_NODELAY, (char *)&val, sizeof(val)) < 0) {
    SockErrLog(wupp::progError, "inetSock::nodelay: error setting NoDelay", errno);
  }
  setFlag(NoDelay, val);
}

// Manipulating the Socket state/config params
// Throws excetpion on error. Does not retry when EINTR
void wunet::inetSock::nonblock(bool on)
{
  int flags = ::fcntl(sd_, F_GETFL);

  if (on)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;

  if (flags < 0) {
    // throws an exception
    SockErrLog(wupp::progError, "inetSock::nonblock: error getting current sock flags", errno);
  }
  
  if (::fcntl(sd_, F_SETFL, flags) < 0) {
    // throws an exception
    SockErrLog(wupp::progError, "inetSock::nonblock: error setting NONBLOCK flag", errno);
  }

  setFlag(Async, true);
}

void wunet::inetSock::close()
{
  if (sd_ != -1) {
    ::close(sd_);
    sd_ = -1;
  }
  flags_ = flags_ & ~StateFlags;
}

// establishing connects
void wunet::inetSock::connect(const inetAddr& serv)
{
  peer_ = serv;
  
  wulog(wulogNet, wulogVerb,
      "inetSock::connect: sd %d, peer %s\n",
      sd_, peer_.toString().c_str());
  if (::connect(sd_, peer_.sockAddr(), peer_.sockLen()) == 0) {
    // connection established
    setFlag(Connect, true);
    if (self_.isWildcard())
      wunet::inetSock::updateSelf(sd_, self_);
  } else {
    switch (errno) {
      case ECONNREFUSED:
        sockerr(wupp::dstError, "inetSock::connect: connection refused", errno);
        break;
      case ETIMEDOUT:
        sockerr(wupp::timeOut, "inetSock::connect: connect timed out", errno);
        break;
      case ENETUNREACH:
        sockerr(wupp::netError, "inetSock::connect: No route to host", errno);
        break;
      case EINPROGRESS:
        break;
      case EALREADY:
        sockerr(wupp::dupError, "inetSock::connect: duplicate connect", errno);
        break;
      case EAGAIN:
        sockerr(wupp::resourceErr, "inet::connect: no resources", errno);
        break;
      case EAFNOSUPPORT:
        sockerr(wupp::dstError, "inetSock::connect: bad destination address", errno);
        break;
      default:
        sockerr(wupp::progError, "inetSock::connect: program error!", errno);
        break;
    }
  }
}

void wunet::inetSock::listen()
{
  if (::listen(sd_, bklog_)) {
    switch (errno) {
      case EADDRINUSE:
        sockerr(wupp::dupError, "inetSock::listen: duplicate attempt to listen to socket", errno);
        break;
      default:
        sockerr(wupp::progError, "inetSock::listen: program error!", errno);
        break;
    }
  }
  if (self_.isWildcard())
    wunet::inetSock::updateSelf(sd_, self_);
  setFlag(Listen, true);
}

wunet::inetSock* wunet::inetSock::accept()
{
  int fd;
  struct sockaddr_in client;
  socklen_t len = sizeof(client);
  wunet::inetSock *newSock = NULL;
 
ac_retry:
  if ((fd = ::accept(sd_, (struct sockaddr*)&client, &len)) > 0) {
    // new connection accepted
    inetAddr maddr(self_.proto()), paddr(self_.proto());

    wunet::inetSock::updateSelf(fd, maddr);
    wunet::inetSock::updatePeer(fd, paddr);

    unsigned int newFlags = flags_ & OPFlags;

    newSock = new wunet::inetSock(fd, maddr, paddr, st_, newFlags);

  } else {
    switch(errno) {
      case EAGAIN:
      //case EWOULDBLOCK:
        break;
      case EINTR:
        // Check flags to see if we should retry
        if (isRetry())
          goto ac_retry;
        sockerr(wupp::intrSysCall, "inetSock::accept: no resources", errno);
        break;
      case ENFILE:
      case EMFILE:
      case ENOBUFS:
      case ENOMEM:
        sockerr(wupp::resourceErr, "inetSock::accept: no resources", errno);
        break;
      case ECONNABORTED:
        sockerr(wupp::dstError, "inetSock::accept: 3-way handshake aborted", errno);
        break;
      case EPERM:
        sockerr(wupp::authError, "inetSock::accept: permission error", errno);
        break;
      default:
        sockerr(wupp::progError, "inetSock::accept: prog error!", errno);
        break;
    }
  }
  return newSock;
}
