/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/new_msg.h,v $
 * $Author: fredk $
 * $Date: 2007/07/27 23:54:53 $
 * $Revision: 1.1 $
 * $Name:  $
 *
 * File:         msg.h
 * Created:      01/17/2005 02:42:25 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WUNET_WUMSG_H
#define _WUNET_WUMSG_H

#include <wuPP/net.h>
#include <wuPP/addr.h>
#include <wuPP/wusock.h>
#include <wuPP/buf.h>

namespace wunet {

  template <class HDRT> class msgBuf : public wunet::mbuf {
    private:
      HDRT *hdr_;
    public:
      // init buffer with initial space for a message header but no body
      msgBuf()
        : mbuf(sizeof(HDRT), maxMsgSize)
      {
        // Make sure the underlying buffer is the correct size
        hlen(sizeof(HDRT));
        // now map to the class HDRT pointer
        hdr_ = (HDRT *)hpullup();
        hdr_->reset();

        // ensure the payload is initialized with sufficient space
        // But we need to make sure there is no more than maxMsgSize bytes of
        // space available
        plen(maxMsgSize);
      }
      ~msgBuf() {};

      void reset() {hlen(sizeof(HDRT)); hdr().reset(); preset(); st_ = Ready;}
      void clear() {hlen(sizeof(HDRT)); hdr().reset(); pclear(); st_ = Ready;}

      HDRT& hdr() {return *hdr_;}
      const HDRT& hdr() const {return *hdr_;}
      HDRT* hptr() const {return hdr_;}

      // All read and write message operations either succeed or
      // throw an exception. Socket exceptions pass through as
      // type badSock, locally detected errors throw netErr.
      wupp::errType readmsg(sockType *sock);
      wupp::errType writemsg(sockType *sock);
      template <class PHDRT> wupp::errType readPload(sockType *sock);
      wupp::errType writePload(sockType *sock);

      std::string toString() const;

      std::ostream &print(std::ostream &os)
      {
        return os << "msgBuf: " << (*hdr).toString(this);
      }

    private:
      wupp::errType readHdr(sockType *);
      wupp::errType readBody(sockType *);
      static const size_t maxMsgSize = 2048;
  };

  template <class HDRT> std::string msgBuf<HDRT>::toString() const
  {
    return hdr().toString(this);
  }

  template <class HDRT> std::ostream& operator<<(std::ostream& os, msgBuf<HDRT>& mb);
  template <class HDRT> std::ostream& operator<<(std::ostream& os, msgBuf<HDRT>& mb)
  {
    os << mb.toString();
    return os;
  }

  // All message reading methods guarantee the following return values:
  //   Partial read/write  : dataPartial
  //   Complete read/write : dataComplete
  //   End of File         : EndOfFile
  //   Error               : Set so that wupp::isError(et) returns true
  template <class HDRT> wupp::errType msgBuf<HDRT>::readHdr(sockType *sock)
  {
    ssize_t n;
    wupp::errType stat;

    if (hlen() == 0) {
      hrewind();
      plen(hdr().dlen());
      return wupp::dataComplete;
    }

    // we should only see EOF or DataRead
    // if n < 0 or n == 0 and a real error then stat is set properly for us
    // if n > 0 then we may need to alter the stat value
    n = sock->readv(hvec(), hcnt());
    if (n == (ssize_t)hlen()) {
      // full header has been read
      hrewind(); 
      plen(hdr().dlen());
      stat = wupp::dataComplete;
    } else if (sock->etype() == wupp::dataNone) {
      // n == -1 and errno was EAGAIN
      stat = wupp::dataPartial;
    } else if (n > 0) {
      // partial read
      hseek(hoffset() + n);
      stat = wupp::dataPartial;
    } else if (sock->etype() == wupp::EndOfFile) {
      // no need to change the value of stat
      clear();
      stat = wupp::EndOfFile;
    } else if (wupp::isError(sock->etype())) {
      clear();
      stat = sock->etype();
    } else {
      clear();
      // some unspecified error to ensure isError(stat) returns true
      stat = wupp::sysError;
    }

    return stat;
  }

  // Sets the msg state st_
  template <class HDRT> wupp::errType msgBuf<HDRT>::readBody(sockType *sock)
  {
    wupp::errType stat;
     ssize_t n;

     // state can only be Header or Partial
     if (plen() == 0) {
       prewind();
       stat = wupp::dataComplete;
       return stat;
     }

     n = sock->readv(pvec(), pcnt());
     if (n == (ssize_t)plen()) {
       // hae read all of hte payload so message is complete
       prewind();
       stat = wupp::dataComplete;
     } else if (sock->etype() == wupp::dataNone) {
       // n == -1 and errno == EAGAIN
       stat = wupp::dataPartial;
     } else if (n > 0) {
       // partial payload read
       pseek(poffset()+n);
       stat = wupp::dataPartial;
     } else if (sock->etype() == wupp::EndOfFile) {
       // no need to change stat value
       clear();
       stat = wupp::EndOfFile;
     } else if (wupp::isError(sock->etype())) {
       clear();
       stat = sock->etype();
     } else {
       clear();
       // set stat so wupp::isError() returns true
       stat = wupp::sysError;
     }
     return stat;
  }

  // Manage all state transitions for message reading
  // States:
  //   Ready: Read new message
  //   HPartial: Already read part of the header, try to read more
  //   Header: Header has been read, now read the Body
  //   PPartial: Part of the Body has already been read
  //   Complete: Assume previous message has been read so reset msg
  //   Error: Last attempt resulted in an error so reset
  //
  // Guarantee on return value:
  //   message fully read : dataComplete
  //   only partial read  : dataPartial
  //   end of file        : EndOfFile
  //   Otherwise wupp::isError(et) will return true
  template <class HDRT> wupp::errType msgBuf<HDRT>::readmsg(sockType *sock)
  {
    wupp::errType stat;
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif

    if (st_ == Ready || st_ == Error || st_ == Complete)
      reset();

    if (st_ == Ready || st_ == HPartial) {
      // always read header in its entirety
      stat = readHdr(sock);
      switch (stat) {
        case wupp::dataComplete:
          st_  = Header;
          stat = wupp::dataPartial;
          break;
        case wupp::dataPartial:
          st_  = HPartial;
          break;
        case wupp::EndOfFile:
          st_ = Ready;
          break;
        default:
          st_ = Error;
      }
    }

    if (st_ == Header || st_ == PPartial) {
      // read body
      stat = readBody(sock);
      switch (stat) {
        case wupp::dataComplete:
          st_  = Complete;
          break;
        case wupp::dataPartial:
          st_  = PPartial;
          break;
        case wupp::EndOfFile:
          st_ = Ready;
          break;
        default:
          st_ = Error;
          return stat;
      }
    }

    // show what we got
    wulog(wulogMsg, wulogLoud, "readmsg(%d): State %s, Hdr %s\n",
          sock->sd(), mbuf::toString(state()).c_str(),  toString().c_str());

#   ifdef WUBUF_DEBUG
      MBUF_CKBUF2(hdr().dlen());
#   endif

    return stat;
  }

  template <class HDRT> wupp::errType msgBuf<HDRT>::writemsg(sockType *sock)
  {
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF2(hdr().dlen());
#   endif
    ssize_t n;
    wupp::errType stat;

    // assume header and paylod are properly initialized and if any partial
    // writes have been done they are properly reflected in the state variable
    // using the mlen(), mvec() and mcnt() methods.
    wulog(wulogMsg, wulogLoud, "writemsg(%d:%d): State %s, Hdr %s\n",
          sock->sd(), mlen(), mbuf::toString(state()).c_str(),  toString().c_str());

    if ((n = sock->writev(mvec(), mcnt())) == (ssize_t)mlen()) {
      // Done
      rewind();
      st_  = Complete;
      stat = wupp::dataComplete;
    } else if (sock->etype() == wupp::dataNone) {
      // n < 0 and errno == EAGAIN
      st_  = PPartial;
      stat = wupp::dataPartial;
    } else if (n > 0) {
      // partial write
      st_  = PPartial;
      stat = wupp::dataPartial;
      seek(offset() + n);
    } else if (sock->etype() == wupp::EndOfFile) {
      clear();
      st_ = Ready;
      stat = wupp::EndOfFile;
    } else if (wupp::isError(sock->etype())) {
      wulog(wulogMsg, wulogError, 
            "writemsg: writev returned -1, should never happen! n = %d\n\t%s",
             n, toString().c_str());
      clear();
      st_ = Error;
      stat = sock->etype();
    } else {
      wulog(wulogMsg, wulogError, "writemsg: writev returned an unexpected value!\n");
      clear();
      // making sure wupp::isError(et) returns true
      stat = wupp::sysError;
      st_  = Error;
    }

#   ifdef WUBUF_DEBUG
      MBUF_CKBUF2(hdr().dlen());
#   endif
      return stat;
    }

  template <class HDRT> template <class PHDRT> wupp::errType msgBuf<HDRT>::readPload(sockType *sock)
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
      wupp::errType stat;
    ssize_t n;

    size_t phlen = PHDRT::hdrlen();

    if (st_ == Ready || st_ == Error || st_ == Complete) {
      plen(phlen);
      st_ = Ready;
    }

    if (st_ == Ready || st_ == HPartial) {
      // the payload is set to only be phlen bytes so at most the header can
      // be read in to the buffer.
      n = sock->readv(pvec(), pcnt());
      // correct, partial or error
      if (n == (ssize_t)phlen) {
        prewind();
        PHDRT *phdr = (PHDRT *)ppullup();
        plen(phlen + phdr->dlen());
        pseek(phlen);
        st_   = Header;
        stat  = wupp::dataPartial;
      } else if (sock->etype() == wupp::dataNone) {
        // no data read
        st_  = HPartial;
        stat = wupp::dataPartial;
      } else if (n > 0) {
        pseek(poffset() + n);
        st_  = HPartial;
        stat = wupp::dataPartial;
      } else if (sock->etype() == wupp::EndOfFile) {
        // when n == 0 then stat == EndOfFile
        clear();
        st_ = Ready;
        stat = wupp::EndOfFile;
      } else if (wupp::isError(sock->etype())) {
        clear(); // show nothing was received
        st_ = Error;
        stat = sock->etype();
      } else {
        clear();
        // make sure wupp::isError(et) returns true
        stat = wupp::sysError;
        st_  = Error;
      }
    }

    if (st_ == Header || st_ == PPartial) {
      if (plen() == 0) {
        prewind();
        st_  = Complete;
        stat = wupp::dataComplete;
      } else {
        n = sock->readv(pvec(), pcnt());
        if (n == (ssize_t)plen()) {
          prewind();
          st_  = Complete;
          stat = wupp::dataComplete;
        } else if (sock->etype() == wupp::dataNone) {
          st_  = PPartial;
          stat = wupp::dataPartial;
        } else if (n > 0) {
          pseek(poffset() + n);
          st_  = PPartial;
          stat = wupp::dataPartial;
        } else if (sock->etype() == wupp::EndOfFile) {
          clear();
          st_ = Ready;
          stat = wupp::EndOfFile;
        } else if (wupp::isError(sock->etype())) {
          clear(); // show nothing was received
          st_ = Error;
          stat = sock->etype();
        } else {
          clear();
          // make sure wupp::isError(et) returns true
          stat = wupp::sysError;
          st_  = Error;
        }
      }
    }

    return stat;
  }

  template <class HDRT> wupp::errType msgBuf<HDRT>::writePload(sockType *sock)
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
    ssize_t n;
    wupp::errType stat = wupp::noError;

    // assume header and paylod are properly initialized and if any partial
    // writes have been done they are properly reflected in the state variable
    // using the mlen(), mvec() and mcnt() methods.
    wulog(wulogMsg, wulogLoud, "writePload(%d:%d): State %s, Hdr %s\n",
          sock->sd(), mlen(), mbuf::toString(state()).c_str(), toString().c_str());

    if (plen() == 0) {
      prewind();
      st_  = Complete;
      return wupp::dataComplete;
    }

    if ((n = sock->writev(pvec(), pcnt())) == (ssize_t)plen()) {
      prewind();
      st_  = Complete;
      stat = wupp::dataComplete;
    } else if (sock->etype() == wupp::dataNone) {
      st_  = PPartial;
      stat = wupp::dataPartial;
    } else if (n > 0) {
      pseek(poffset() + n);
      st_  = PPartial;
      stat = wupp::dataPartial;
    } else if (sock->etype() == wupp::EndOfFile) {
      // no change to stat
      clear();
      st_ = Error;
      stat = wupp::EndOfFile;
    } else if (wupp::isError(sock->etype())) {
      wulog(wulogMsg, wulogError,
            "writePload: writev returned error, should never happen! n = %d\n\t%s",
            n, toString().c_str());
      clear();
      st_ = Error;
      stat = sock->etype();
    } else {
      wulog(wulogMsg, wulogError, "writePload: writev returned an unexpected value");
      clear();
      // ensure wupp::isError() returns true
      stat = wupp::sysError;
      st_  = Error;
    }

#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
    return stat;
  }
}

#endif
