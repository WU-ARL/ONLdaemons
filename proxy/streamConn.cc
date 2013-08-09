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
 * $Author: cgw1 $
 * $Date: 2007/03/23 21:05:05 $
 * $Revision: 1.7 $
 * $Name:  $
 *
 * File:         session.cc.c
 * Created:      12/29/2004 10:56:15 AM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */
#include <iostream>
#include <sstream>
#include <vector>

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>

#include <pthread.h>  /* pthread */
#include <wulib/wulog.h>


#include <wuPP/net.h>
#include <wuPP/addr.h>
#include <wuPP/sock.h>
#include <wuPP/buf.h>
#include <wuPP/msg.h>

#include "control.h"
#include "cmd.h"
#include "nccp.h"
#include "basicConn.h"
#include "streamConn.h"
#include "session.h"
#include "proxy.h"

using namespace wucmd;

void StreamConn::handler()
{
  // will not throw an exception
  if (doRecv(mbuf_) == false) {
    wulog(wulogSession, wulogError,
        "StreamConn::handler(%s): Error reading message\n\tConn %s\n\tMsg %s\n",
        this->idString().c_str(), toString().c_str(), mbuf_.toString().c_str());
    session_->reportStatus(mbuf_, wucmd::connStatus, cid());
    return;
  }
  if (! mbuf_.msgComplete())
    return;

  mbuf_.hdr().reset();
  mbuf_.hdr().cmd(wucmd::fwdData);
  mbuf_.hdr().msgID(0);
  mbuf_.hdr().conID(cid()); // conID from which this message was received
  mbuf_.hdr().dlen(mbuf_.plen());
  
  session_->setTxNextSlave();

  if (session_->doSend(mbuf_) == false) {
    wulog(wulogSession, wulogError,
        "StreamConn::handler(%s): session->doSend failed\n\tConn %s\n\tMsg %s\n",
        this->idString().c_str(), session_->toString().c_str(), mbuf_.toString().c_str());
  }
  return ;
}


wuctl::connInfo* StreamConn::toData(wuctl::connInfo* cd)
{
  if (cd == NULL)
    return NULL;

  memset((void *)cd, 0, sizeof(wuctl::connInfo));

  cd->cid(cid());
  if (sock() != NULL) {
    cd->laddr(sock()->self().addr());
    cd->lport(sock()->self().port());
    cd->daddr(sock()->peer().addr());
    cd->dport(sock()->peer().port());
    cd->proto(sock()->self().proto());
  }
  uint8_t flags = 0;
  if (isOpen())
    flags |= wuctl::connInfo::flagOpen;
  if (isRxPending() || isTxPending())
    flags |= wuctl::connInfo::flagPending;

  cd->flags(flags);

  //stats
  cd->stats(stats_);

  return cd;
}

std::string StreamConn::idString()
{
  std::ostringstream ss;
  int sid = session_ ? session_->cid() : -1;
  ss << sid << ":" << this->cid();
  return ss.str();
}

bool StreamConn::doSend(wunet::msgBuf<wucmd::hdr_t>& mbuf)
try {
  // returns:
  //   message fully written : dataComplete
  //   only partial written  : dataPartial
  //   end of file           : EndOfFile
  //   Otherwise wupp::isError(et) will return true
  wupp::errType et = mbuf.writePload(sock());
  if (et == wupp::dataComplete) {
    session_->clearTxHandler();
    setTxPending(false);
  } else if (et == wupp::dataPartial) {
    if (!session_->setTxHandler(this, &mbuf)) {
      wulog(wulogSession, wulogError, "StreamConn::doSend(%s): setTxHandler failed\n",
            this->idString().c_str());
      etype() = wupp::sysError;
      close();
      return false;
    }
    setTxPending(true);
  } else if (et == wupp::EndOfFile) {
     wulog(wulogSession, wulogError, "StreamConn::doSend(%s): EOF on send\n",
           this->idString().c_str());
    etype() = et;
    close();
    return false;
  } else {
    // wupp::isError(et) must return true
    etype() = et;
    wulog(wulogSession, wulogError, "StreamConn::doSend(%s): Error on send\n",
        this->idString().c_str());
    std::cout << mbuf << std::endl;
    std::cout << toString() << std::endl;
    close();
    return false;
  }

  txMsgStats(mbuf);
  return true;

} catch (wupp::errorBase& e) {
  wulog(wulogSession, wulogError, "StreamConn::doSend(%s): wupp exception:\n\t%s\n",
        this->idString().c_str(), e.toString().c_str());
  etype() = e.etype();
  session_->clearTxHandler();
  close();
  return false;
} catch (...) {
  wulog(wulogSession, wulogError, "StreamConn::doSend(%s): unknown exception\n", 
      this->idString().c_str());
  etype() = wupp::sysError;
  session_->clearTxHandler();
  close();
  return false;
}
