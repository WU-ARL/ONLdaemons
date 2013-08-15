/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/basicConn.h,v $
 * $Author: cgw1 $
 * $Date: 2008/04/09 18:39:35 $
 * $Revision: 1.6 $
 * $Name:  $
 *
 * File:         try.c
 * Created:      12/29/2004 10:56:15 AM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */
#ifndef _BASICCONN_HDR__H
#define _BASICCONN_HDR__H

namespace wunet {

  class BasicConn {
    public:
      typedef uint16_t flags_t;
      static wucmd::flags_t conn2Flags(BasicConn *conn);

      static const flags_t flgNull        = 0x0000;
      static const flags_t flgOpen        = 0x0001;
      static const flags_t flgRxPending   = 0x0002;
      static const flags_t flgTxPending   = 0x0004;

      bool isOpen()      {return st_ & flgOpen;}
      bool isClosed()    {return !isOpen();}
      bool isRxPending() {return st_ & flgRxPending;}
      bool isTxPending() {return st_ & flgTxPending;}
      void setRxPending(bool on);
      void setTxPending(bool on);

      BasicConn(inetSock *ss);
      virtual ~BasicConn() {close();}
      virtual void close();

      bool operator!() {return ! check();}
      bool check()
      {
        if (isOpen() && validSock())
          return true;
        return false;
      }

      // ***** Must be defined by the derived classes *****
      // handler() must not throw an exception
      virtual int handler() = 0;
      virtual wuctl::connInfo* toData(wuctl::connInfo*) = 0;

      // doSend and doRecv must do the following:
      //   1) if incomplete read/write then set appropriate Pending flag
      //   2) if error reading/writing then close() this object
      //      and set et member object.
      //   3) call MsgStats with mbuf reference
      //   4) return true if no errors, the caller must check the Pending
      //      flags to determine if the message is complete.
      virtual bool doRecv(msgBuf<wucmd::hdr_t>&) = 0;
      virtual bool doSend(msgBuf<wucmd::hdr_t>&) = 0;
      virtual bool setTxHandler(wunet::BasicConn *, wunet::msgBuf<wucmd::hdr_t> *) {return false;};
      virtual void clearTxHandler() {};

      // ***** Socket and Connection accessors *****
      const inetSock* sock() const {return ss_;}
      inetSock*       sock() {return ss_;}
      int               sd() const {return validSock() ? ss_->sd() : -1;}
      int              cid() const {return cid_;}
      wupp::errType    etype() const {return et_;}
      wupp::errType    &etype() {return et_;}

      // ***** Statistics accessors *****
      const wuctl::BasicStats &stats() const {return stats_;}

      // ***** Methods used for identification *****
      virtual std::string idString() {std::ostringstream ss; ss << cid(); return ss.str();}

      std::string toString() {std::ostringstream ss; this->print(ss); return ss.str();}
      virtual std::ostream &print(std::ostream &ss)
      {
        ss << "Id "        << this->idString()
           << ", State "   << BasicConn::state2String(st_).c_str()
           << ", ErrCode " << et_
           << ", Sock ";
        if (ss_ == NULL)
          ss << "= 0";
        else if (ss_->isOpen())
          ss << "Open";
        else
          ss << "Not Open";
        return ss;
      }

    protected:
      static const int maxMsgSize = 2048;
      wunet::msgBuf<wucmd::hdr_t> mbuf_;

      bool validSock() const {return ss_ && ss_->isOpen();}
      wuctl::BasicStats stats_;
      void rxMsgStats(const wunet::mbuf &msg)
      {
        if (msg.state() == wunet::mbuf::HPartial || msg.state() == wunet::mbuf::PPartial) {
          ++stats_.rxMsgPrt;
        } else if (msg.state() == wunet::mbuf::Error) {
          ++stats_.rxMsgErr;
        } else {
          stats_.rxMsgCnt  += 1;
          stats_.rxByteCnt += msg.mlen();
        }
        return;
      }
      void txMsgStats(const wunet::mbuf &msg)
      {
        if (msg.state() == wunet::mbuf::HPartial || msg.state() == wunet::mbuf::PPartial) {
          ++stats_.txMsgPrt;
        } else if (msg.state() == wunet::mbuf::Error) {
          ++stats_.txMsgErr;
        } else {
          stats_.txMsgCnt  += 1;
          stats_.txByteCnt += msg.mlen();
        }
        return;
      }
    private:
      void setFlag(bool on, flags_t flag)
      {
        if (on)
          st_ |= flag;
        else
          st_ &= ~flag;
      }
      void setOpen(bool on) {setFlag(on, flgOpen);}

      wupp::errType et_;
      flags_t   st_;
      wunet::inetSock *ss_;
      wucmd::conID_t  cid_;

      static const std::string state2String(flags_t st)
      {
        std::string str;
        if (st & flgOpen)
          str = "Open";
        else 
          str = "Closed";
        if (st & flgRxPending)
          str += "|RxPending";
        if (st & flgTxPending)
          str += "|TxPending";
        return str;
      }
  };


  inline BasicConn::BasicConn(inetSock* ss)
    : et_(wupp::noError), st_(0), ss_(ss), cid_(0)
  {
    if (validSock()) {
      // Make non-blocking
      ss->nonblock(true);
      setOpen(true);
      cid_ = wucmd::cmd_t(ss_->sd());
    } else {
      wulog(wulogSession, wulogWarn, "BasicConn(%s): constructor passed unopen socket!\n",
            this->idString().c_str());
      // Assume we will only be passed a valid, open socket.
      // The caller need to delete the socket since this object has not
      // accepted ownership for it.
      ss_ = 0;
      et_ = wupp::progError;
    }
  }

  inline void BasicConn::close()
  {
    if (ss_) {
      ss_->close();
      delete ss_;
      ss_ = 0;
    }
    if (isOpen()) {
      setOpen(false);
      wulog(wulogSession, wulogVerb, "BasicConn::close(%s): Closing connection\n",
            this->idString().c_str());
    }
  }

  inline void BasicConn::setRxPending(bool on) {setFlag(on, flgRxPending);}
  inline void BasicConn::setTxPending(bool on) {setFlag(on, flgTxPending);}
};

#endif
