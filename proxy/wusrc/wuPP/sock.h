/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/Attic/sock.h,v $
 * $Author: fredk $
 * $Date: 2007/01/25 18:45:56 $
 * $Revision: 1.21 $
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

#ifndef _WUNET_SOCK_H
#define _WUNET_SOCK_H

#include <wuPP/net.h>
#include <wuPP/addr.h>
#include <sys/uio.h>

namespace wunet {

  typedef unsigned int sockFlags_t;
  inline std::string print_stype(int st);

  class badSock : public netErr {
    public:
      badSock() : netErr(wupp::noError) {}
      badSock(wupp::errType et, int sysErr=0) : netErr(et, "sockErr", sysErr) {};
      badSock(wupp::errType et, const std::string& lm, int sysErr=0) : netErr(et, lm, sysErr) {};
      badSock(wupp::errType et, const std::string& lm, int sysErr, const std::string& sm)
        : netErr(et, lm, sysErr, sm) {}

      virtual ~badSock() {};

      std::string toString() const {
        std::ostringstream ss;
        print(ss);
        return ss.str();
      }

      std::ostream& print(std::ostream& os = std::cerr) const
      {
        os << "Socket Error: ";
        wunet::netErr::print(os);
        return os;
      }
  };
  inline std::string print_stype(int st)
  {
    std::ostringstream ss;
    if (st == SOCK_STREAM)
      ss << "Stream";
    else if (st == SOCK_DGRAM)
      ss << "DGram";
    else if (st == SOCK_RAW)
      ss << "Raw";
    else
      ss << st;
    return ss.str();
  }


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
   *   XXX EDESTADDRREQ, EINVAL, EACCES, EINVAL, ENOBUFS
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

  class inetSock {
    public:
      static const sockFlags_t None       = 0x00000000;
      static const sockFlags_t OPFlags    = 0x000000FF;
      static const sockFlags_t Async      = 0x00000001; // socket is nonblocking
      static const sockFlags_t Retry      = 0x00000002; // retry on interrupt
      static const sockFlags_t NoDelay    = 0x00000004; //
      static const sockFlags_t ReuseAddr  = 0x00000008; //
      static const sockFlags_t ErrFlags   = 0x0000FF00;
      static const sockFlags_t Error      = 0x00000200;
      static const sockFlags_t StateFlags = 0x00FF0000;
      static const sockFlags_t Open       = 0x00010000;
      static const sockFlags_t Connect    = 0x00020000;
      static const sockFlags_t Listen     = 0x00040000;

      inetSock(); // useful for active connect (clients)
      inetSock(const inetAddr& myaddr, int stype=0);
      inetSock(int sd, const inetAddr& myaddr, const inetAddr& peer, int stype, sockFlags_t flags);

      virtual ~inetSock() {this->close();}

      void init(const inetAddr& myaddr, int stype=0);

      // Manipulating the Socket state/config params
      void reuseaddr(int val);
      // on == true => turn on nonblocking
      void nonblock(bool on);

      // TCP specific option!
      void nodelay(int val);

      // establishing connects
      // throws an exception on error
      void connect(const inetAddr& serv);
      // throws an exception on error
      void listen();
      // Return: valid sock object or NULL if nonblock and would block
      // Otherwise throw an Exception
      inetSock* accept();
      void close();

      // reading and writing to/from socket
      // for all read's and write's they either
      // succeed (n > 0) or return EOF (n == 0)
      // throws an exception on error
      ssize_t read(void *buf, size_t len, wupp::errType &);
      ssize_t read(void *buf, size_t len) {wupp::errType stat; return read(buf, len, stat);}
      // throws an exception on error
      ssize_t readn(void *buf, size_t len, wupp::errType &);
      ssize_t readn(void *buf, size_t len) {wupp::errType stat; return readn(buf, len, stat);}
      // throws an exception on error
      ssize_t readv(const struct iovec *, int, wupp::errType &);
      ssize_t readv(const struct iovec *iov, int len) {wupp::errType stat; return readv(iov, len, stat);}
      // throws an exception on error
      ssize_t write(void *buf, size_t len, wupp::errType &);
      ssize_t write(void *buf, size_t len) {wupp::errType stat; return write(buf, len, stat);}
      // throws an exception on error
      ssize_t writen(void *buf, size_t len, wupp::errType &);
      ssize_t writen(void *buf, size_t len) {wupp::errType stat; return writen(buf, len, stat);}
      // throws an exception on error
      ssize_t writev(const struct iovec *, int, wupp::errType &);
      ssize_t writev(const struct iovec *iov, int len) {wupp::errType stat; return writev(iov, len, stat);}
      //ssize_t recvmsg(int s, struct msghdr *msg, int flags);
      //ssize_t sendmsg(int s, const struct msghdr *msg, int flags);

      // about this socket
      sockFlags_t state() const {return flags_ & StateFlags;}
      sockFlags_t flags() const {return flags_;}

      bool isFlagOn(const sockFlags_t flag) const {return flags_ & flag;}
      // Operational Flags
      bool isAsync()   const {return isFlagOn(Async);}
      bool isRetry()   const {return isFlagOn(Retry);}
      bool isNoDelay() const {return isFlagOn(NoDelay);}
      // Error Flags
      bool isError()   const {return isFlagOn(Error);}
      // State Flags
      bool isOpen()    const {return isFlagOn(Open);}
      bool isConenct() const {return isFlagOn(Connect);}
      bool isListen()  const {return isFlagOn(Listen);}

      bool operator!() const {return isError();}
      const badSock &lastError() const {return bs_;}

      const inetAddr& self() const {return self_;}
      const inetAddr& peer() const {return peer_;}
      int socktype() const {return st_;}
      int sd() const {return sd_;}

      static int proto2stype(int proto);

      std::string toString() const {
        std::ostringstream ss;
        print(ss);
        return ss.str();
      }

      std::ostream& print(std::ostream& os = std::cout) const {
        os << "(Sock: " << print_stype(st_)
           << ", "    << std::hex << flags_  << std::dec
           << ", "    << sd_
           << ", Local " << self_
           << ", Peer "  << peer_
           << ")";
        return os;
      }

    private:
      void setFlag(const sockFlags_t flag, bool on)
      {
        if (on) flags_ |= flag; else flags_ &= ~flag;
      }

#     define SockErrLog(et, lm, err) \
      do { \
        std::cerr << "inetSock Error: File " << __FILE__ << ", Function " << __func__ << ", Line " << __LINE__ << "\n";\
        sockerr((et), (lm), (err)); \
      } while (/* */ 0)
      // sockinfo will just print message
      void sockinfo(wupp::errType et, const std::string& lm, int err=0);
      // sockwarn will just throw exception.
      void sockwarn(wupp::errType et, const std::string& lm, int err=0);
      // sockerr will close socket and put into error state and throw exception
      void sockerr(wupp::errType et, const std::string& lm, int err=0);

      inetSock& operator=(inetSock&); // no assignment operator
      inetSock(const inetSock&); // and no copy constructor

      // throws an exception on error
      void open();
      // throws an exception on error
      void updateSelf(int, inetAddr&);
      // throws an exception on error
      void updatePeer(int, inetAddr&);

      // throws exceptions for all but:
      //   wupp::dataSuccess, EndOfFile
      wupp::errType ckSockErr(int n, int syserr);

      // ------------------ Private Data Members ----------------------------
      inetAddr      self_; // local inet address
      int             st_; // socket type (stream, dgram, raw or seqpkt)
      inetAddr      peer_; // remote address if connected
      sockFlags_t  flags_; //
      badSock         bs_; // Copy of the last Exception (error condition)
      int             sd_; // socket descriptor
#     define SOCK_LISTEN_BACKLOG    10
      int          bklog_; // backlog for listen, only used for TCP.
  };

  // new functions addedto the wunet namespace
  inline std::ostream& operator<<(std::ostream&, const inetSock&);
  inline std::ostream& operator<<(std::ostream&, sockFlags_t);
  inline std::ostream& operator<<(std::ostream&, const badSock&);

  inline inetSock::inetSock()
    : flags_(0), sd_(-1), bklog_(SOCK_LISTEN_BACKLOG)
  {
    st_ = proto2stype(self_.proto());
  }

  inline void inetSock::init(const inetAddr& myaddr, int stype)
  {
    if (isOpen()) {
      // throw an exception
      sockerr(wupp::progError, "sock::init: Attempt to initialize an open socket");
    }
    self_ = myaddr;
    st_ = (stype > 0) ? stype : proto2stype(self_.proto());
    this->open();
    return;
  }

  inline inetSock::inetSock(const inetAddr& addr, int stype)
    : self_(addr), flags_(0), sd_(-1), bklog_(SOCK_LISTEN_BACKLOG)
  {
    try {
      st_ = (stype > 0) ? stype : proto2stype(self_.proto());
      this->open();
      return;
    } catch (badSock &bs) {
      // is open fails then the socket _is_ in an error state
      // there is nothing else to do but make sure the exception does not
      // propagate up. The user can call the operator!() to check socket state.
      // just in case make sure we show the socket in an error state
      setFlag(Error, true);
      return;
    } catch (...) {
      bs_ = badSock(wupp::progError, "Unknown exception in inetSock constructor(inetAddr ...)");
      setFlag(Error, true);
      return;
    }
  }

  inline inetSock::inetSock(int sd, const inetAddr& myaddr, const inetAddr& peer, int stype, unsigned int flags)
    : self_(myaddr), peer_(peer), flags_(flags), sd_(sd), bklog_(SOCK_LISTEN_BACKLOG)
  {
    st_ = (stype > 0) ? stype : proto2stype(self_.proto());
    // assume sd is valid (i.e. an open and connected socket)
    setFlag(Open, true);
    setFlag(Connect, true);
    return;
  }

  // throws exception for all except: dataSuccess or EndOfFile
  // Guarantee the following for stat:
  //   n > 0 : dataSuccess
  //   n == 0: EndOfFile
  //   n < 0 && AGAIN : dataNone
  //   else an error and stat is set to representthe type of error.
  inline wupp::errType inetSock::ckSockErr(int n, int syserr)
  {
    wupp::errType stat = wupp::noError;

    if (n > 0) {
      stat = wupp::dataSuccess;
    } else if (n == 0) {
      stat = wupp::EndOfFile;
    } else {
      switch (syserr) {
        case ETIMEDOUT:
          stat = wupp::timeOut;
          sockerr(stat, "inetSock::ckSockErr: connection timedout (ETIMEDOUT)", syserr);
          break;
        case ECONNRESET:
          stat = wupp::connFailure;
          sockerr(stat, "inetSock::ckSockErr: reset by peer (ECONNRESET)", syserr);
          break;
        case EPIPE:
          stat = wupp::connFailure;
          sockerr(stat, "inetSock::ckSockErr: other end has closed (EPIPE)", syserr);
          break;
        case EINTR:
          // Similar behavior as EAGAIN in that we assume we
          // would have blocked (or were blocked) when a signal arrived.
          stat = wupp::intrSysCall;
          if (! isRetry())
            sockwarn(stat, "inetSock::ckSockErr: system call interrupted (EINTR)", syserr);
          break;
        case EAGAIN: // or EWOULDBLOCK which == EAGAIN on Linux
          // do nothing, let the caller decide if they want to try the
          // read again. 
          stat = wupp::dataNone;
          if (! isAsync())
            sockwarn(stat, "inetSock::ckSockErr: No data available (EAGAIN)", syserr);
          break;
        case ENOBUFS:
        case ENOMEM:
          stat = wupp::resourceErr;
          sockwarn(stat, "inetSock::ckSockErr: no resources (ENOBUFS or ENOMEM)", syserr);
        default:
          stat = wupp::sysError;
          sockerr(stat, "inetSock::ckSockErr: unknown system error! possible program error", syserr);
          break;
      }  
    }
    return stat;
  }

  inline ssize_t inetSock::read(void *buf, size_t len, wupp::errType &stat)
  {
    int n;
    do {
      n = ::read(sd_, buf, len);
      stat = ckSockErr(n, errno);
    } while (stat == wupp::intrSysCall && isRetry());
    return n;
  }

  inline ssize_t inetSock::readn(void *buf, size_t len, wupp::errType &stat)
  {
    // n == 0 on EOF
    int nleft = len, n;
    char *ptr = (char *)buf;

    while (nleft > 0) {
#ifdef __CYGWIN__
      // cygwin doesn't support the MSG_WAITALL flag
      n = ::recv(sd_, buf, nleft, 0);
#else
      n = ::recv(sd_, buf, nleft, MSG_WAITALL);
#endif
      do {
        stat = ckSockErr(n, errno);
        // I don't check for dataNone since I prefer to not spin
        // if socket is nonblocking.
      } while (stat == wupp::intrSysCall && isRetry());
      if (stat == wupp::dataSuccess) {
        nleft -= n;
        ptr   += n;
      } else {
        // dataNone, EOF
        return n;
      }
    }
    return len;
  }

  inline ssize_t inetSock::readv(const struct iovec *vecs, int count, wupp::errType &stat)
  {
    int n;
    do {
      n = ::readv(sd_, vecs, count);
      stat = ckSockErr(n, errno);
    } while (stat == wupp::intrSysCall && isRetry());
    return n;
  }

  inline ssize_t inetSock::write(void *buf, size_t len, wupp::errType &stat)
  {
    int n;
    do {
      n = ::write(sd_, buf, len);
      stat = ckSockErr(n, errno);
    } while (stat == wupp::intrSysCall && isRetry());
    return n;
  }

  inline ssize_t inetSock::writen(void *buf, size_t len, wupp::errType &stat)
  {
    int nleft, n;
    const char *ptr;
    
    ptr = (char *)buf; nleft = len;

    while (nleft > 0) {
      do {
        // on an error ckSockErr will throw an exception
        n = ::write(sd_, buf, nleft);
        stat = ckSockErr(n, errno);
      } while (stat == wupp::intrSysCall && isRetry());
      if (stat == wupp::dataSuccess) {
        nleft -= n;
        ptr   += n;
      } else {
        // dataNone or EOF
        return n;
      }
    }
    return len;
  }

  inline ssize_t inetSock::writev(const struct iovec *vecs, int count, wupp::errType &stat)
  {
    int n;
    do {
      n = ::writev(sd_, vecs, count);
      stat = ckSockErr(n, errno);
    } while (stat == wupp::intrSysCall && isRetry());
    return n;
  }

  inline std::ostream& operator<<(std::ostream& os, const badSock& bs)
  {
    return bs.print(os);
  }

  inline std::ostream& operator<<(std::ostream& os, const inetSock& so)
  {
    return so.print(os);
  }

}

#endif
