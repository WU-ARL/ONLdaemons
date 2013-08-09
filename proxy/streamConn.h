/*
 * Copyright (c) 2009-2013 Fred Kuhns, Charlie Wiseman, Jyoti Parwatikar, John DeHart 
 * and Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

/*
 * $Author: fredk $
 * $Date: 2006/12/08 20:35:00 $
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
#ifndef _STREAMCONN_H
#define _STREAMCONN_H

namespace wucmd {

  class Session;

  class StreamConn : public wunet::BasicConn {
    public:
      StreamConn(Session *s, wunet::inetSock *ss)
        : wunet::BasicConn(ss), session_(s) {}
      ~StreamConn() {session_ = NULL;} // Base class calls the close method
      void handler();
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
