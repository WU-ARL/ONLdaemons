/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/streamConn.h,v $
 * $Author: cgw1 $
 * $Date: 2008/04/09 18:39:35 $
 * $Revision: 1.7 $
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
#ifndef _STREAMCONN_H
#define _STREAMCONN_H

namespace wucmd {

  class Session;

  class StreamConn : public wunet::BasicConn {
    public:
      StreamConn(Session *s, wunet::inetSock *ss)
        : wunet::BasicConn(ss), session_(s) {}
      ~StreamConn() {session_ = NULL;} // Base class calls the close method
      int handler();
      virtual wuctl::connInfo* toData(wuctl::connInfo*);

      // Will not throw exceptions
      virtual bool doRecv(wunet::msgBuf<wucmd::hdr_t>&);
      // Will not throw exceptions
      virtual bool doSend(wunet::msgBuf<wucmd::hdr_t>&);

      std::ostream &print(std::ostream &ss)
      {
        ss << "StreamConn: ";
        return wunet::BasicConn::print(ss);
      }
      std::string idString();

    private:
      Session *session_;
  };

  inline bool StreamConn::doRecv(wunet::msgBuf<wucmd::hdr_t>& mbuf)
  try {
    if (! mbuf.msgPartial())
      mbuf.preset();

    // returns:
    //   message fully read : dataComplete
    //   only partial read  : dataPartial
    //   end of file        : EndOfFile
    //   Otherwise wupp::isError(et) will return true
    wupp::errType et = mbuf.readPload<nccp_t>(sock());
    if (wupp::isError(et)) {
      wulog(wulogSession, wulogError, "Session::doRecv(%s): Connection Error\n",
          this->idString().c_str());
       etype() = et;
       close();
       return false;
    } else if (et == wupp::EndOfFile) {
      wulog(wulogSession, wulogInfo, "Session::doRecv(%s): Connection EOF\n",
          this->idString().c_str());
      etype() = et;
      close();
      return false;
    }
    wulog(wulogSession, wulogLoud, "doRecv(%s): Read nccp message, state == %d\n",
          this->idString().c_str(), (int)mbuf.state());

    setRxPending(mbuf.msgPartial());

    rxMsgStats(mbuf);
    return true;

  } catch (wupp::errorBase& e) {
    wulog(wulogSession, wulogError, "StreamConn::doRecv(%s): %s\n",
          this->idString().c_str(), e.toString().c_str());
    etype() = e.etype();
    close();
    return false;
  } catch (...) {
    wulog(wulogSession, wulogError, "StreamConn::doRecv(%s): Unknown exception type\n",
          this->idString().c_str());
    etype() = wupp::sysError;
    close();
    return false;
  }

};
#endif
