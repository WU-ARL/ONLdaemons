/* vim: filetype=cpp
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/wusock.h,v $
 * $Author: fredk $
 * $Date: 2008/02/22 16:38:51 $
 * $Revision: 1.20 $
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

#include <wuPP/error.h>
#include <wuPP/net.h>
#include <wuPP/addr.h>
#include <sys/uio.h>

#define WUNET_WUSOCK_CLASS
#include <wulib/wusock.h>

// #define WUSOCK_DEBUG

namespace wunet {

  typedef unsigned int sockFlags_t;

  class badSock : public netErr {
    public:
      badSock() : netErr(wupp::noError) {}
      badSock(wupp::errType et, const std::string& lm, int sysErr=0)
        : netErr(et, lm, sysErr) {};
      badSock(wupp::errType et, const wupp::errInfo &einfo) : netErr(et, einfo) {};

      virtual ~badSock() {};

      /* use default methods for:
       *   std::string toString() const;
       *   std::ostream& print(std::ostream& os = std::cerr) const;
       *   */
  };

  class sockType {
    protected:
      // ------------------ Private Data Members ----------------------------
      wusock_t sock_;
      badSock    bs_; // Copy of the last Exception (error condition)
#define SOCK_LISTEN_BACKLOG    10
    public:
      typedef sockFlags_t sflags_t;

      // Create new objects
      sockType();
      sockType(sockType &oldsock);
      sockType(wusock_t &oldsock);
      sockType(int domain, int stype, int proto=0);
      sockType(int sd);
      virtual ~sockType();

      // Throws an exception if it fails
      void init(wusock_t &oldsock);
      void init(int domain, int stype, int proto);
      void init(int sd);
      void clean() {sok_clean(&sock_);}

      // throws an exception on error
      virtual int  open();
      virtual void close();
      int  bind(const struct sockaddr *);
      virtual int bind() { throw badSock(wupp::opError, "bind: no default bind"); }

      int  connect(const struct sockaddr *);
      int  listen();
      virtual sockType *clone();
      sockType *accept();
      sockType &accept(sockType &newSock);

      // ***** All read and write ops will throw an exception on error *****
      // Make inline since these functions are most likely to be in
      // performance critical code. For all read's and write's they either
      // succeed (n > 0), return EOF (n == 0) or throw an exception.
      ssize_t read(void *buf, size_t len)
        {ssize_t n = sok_read(&sock_, buf, len); check_state(); return n;}

      ssize_t recv(void *buf, size_t len)
        {ssize_t n = sok_recv(&sock_, buf, len); check_state(); return n;}

      ssize_t readn(void *buf, size_t len)
        {ssize_t n = sok_readn(&sock_, buf, len); check_state(); return n;}

      ssize_t readv(const struct iovec *iov, int len)
        {ssize_t n = sok_readv(&sock_, iov, len); check_state(); return n;}

      ssize_t rdfrom(void *buf, size_t len, struct sockaddr *from)
        {ssize_t n = sok_rdfrom(&sock_, buf, len, from); check_state(); return n;}

      ssize_t recvmsg(struct msghdr *msg, int flags)
        {ssize_t n = sok_recvmsg(&sock_, msg, flags); check_state(); return n;}

      ssize_t write(void *buf, size_t len)
        {ssize_t n = sok_write(&sock_, buf, len); check_state(); return n;}

      ssize_t writen(void *buf, size_t len)
        {ssize_t n = sok_writen(&sock_, buf, len); check_state(); return n;}

      ssize_t writev(const struct iovec *iov, int len)
        {ssize_t n = sok_writev(&sock_, iov, len); check_state(); return n;}

      ssize_t writeto(void *buf, size_t len, struct sockaddr *to)
        {ssize_t n = sok_writeto(&sock_, buf, len, to); check_state(); return n;}

      ssize_t sendmsg(const struct msghdr *msg, int flags)
        {ssize_t n = sok_sendmsg(&sock_, msg, flags); check_state(); return n;}

      // ******** Manipulating the Socket state/config params **********
      void reuseaddr(int val);
      // TCP specific option!
      void nodelay(int val);
      // on == true => turn on nonblocking
      void nonblock(bool on);
      void recvbuf(int sz);
      void sendbuf(int sz);
      void set_rxtout(const struct timeval &tval);
      void set_rxtout(unsigned int msec);
      void set_txtout(const struct timeval &tval);
      void set_txtout(unsigned int msec);

      // real simple way to get timeouts on a socket read
      int rxselect(struct timeval);

      // about this socket
      sockFlags_t state() const {return sock_.flags_ & sok_Flg_StateFlags;}
      sockFlags_t flags() const {return sock_.flags_;}

      // set to true (i.e. 1) if you want system calls to be restarted after
      // a signal is caught.
      void retry(int val) {sok_retry(&sock_, val);}
      // generic test for true
      bool isFlagOn(const sockFlags_t flag) const {return sock_.flags_ & flag;}
      // Operational Flags
      bool isAsync()   const {return isFlagOn(sok_Flg_Async);}
      bool isRetry()   const {return isFlagOn(sok_Flg_Retry);}
      bool isNoDelay() const {return isFlagOn(sok_Flg_NoDelay);}
      // Error Flags
      bool isError()   {check_state(); return sok_inError(&sock_);}
      // State Flags
      bool isOpen()    {check_state(); return isFlagOn(sok_Flg_Open);}
      bool isConenct() const {return isFlagOn(sok_Flg_Connect);}
      bool isListen()  const {return isFlagOn(sok_Flg_Listen);}

      int socktype() const {return sock_.st_;}
      int stype()    const {return sock_.st_;}
      int sd()       const {return sock_.sd_;}
      int proto()    const {return sock_.pr_;}
      int domain()   const {return sock_.dom_;}
      wupp::errType etype() const {return sock_.et_;} // state after last operation

      // These are not const methods since an error can change the sockets state.
      void saddr(struct sockaddr *saddr) { sok_update_saddr(&sock_, saddr); }
      void peer(struct sockaddr *saddr)  { sok_get_peer(&sock_, saddr); }

      virtual void print(const char *pre) const {sok_print(&sock_, pre);}

      /* **************** Error Processing/Information *********************** */
      void check_state()
      {
        if (!sok_inError(&sock_)) return;

        bs_ = badSock(sock_.et_, sock_.einfo_);
#ifdef WUSOCK_DEBUG
        std::cout << "check_sock error: " << bs_ << std::endl;
#endif
        throw bs_;
      }

      bool operator!() {return isError();}
      const badSock &lastError() const {return bs_;}

      std::string toString() const {
        char descBuf[WULOG_MSG_MAXLEN];
        sok_desc2str(sock_.sd_, descBuf, WULOG_MSG_MAXLEN);

        std::ostringstream ss;
        ss << "sd "   << sock_.sd_
           << ", st " << sock_.st_
           << ", flags " << std::hex << sock_.flags_ << std::dec
           << ", et " << sock_.et_
           << ", Addr " << descBuf;
        return ss.str();
      }

      // All derived classes should adhere to this behavior:
      //   to copy the socket is to pass ownership on to another object. In
      //   other words this discourages to maintenance of multiple references
      //   to an open socket.  The same is true for the copy constructor.
      sockType& operator=(sockType& rhs)
      {
        if (this == &rhs)
          return *this;
        init(rhs.sock_);
        return *this;
      }

    private:
      void setFlag(const sockFlags_t flag, bool on)
      {
        if (on) sock_.flags_ |= flag; else sock_.flags_ &= ~flag;
      }
  };

};
static inline std::ostream &operator<<(std::ostream &os, const wunet::sockType &st);
static inline std::ostream &operator<<(std::ostream &os, const wunet::sockType &st)
{
  os << st.toString();
  return os;
}

#endif
