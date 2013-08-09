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
 * $Date: 2007/03/23 21:21:54 $
 * $Revision: 1.18 $
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
#ifndef _WU_SESSION_H
#define _WU_SESSION_H

#include <wuPP/util.h>
#include "control.h"
#include "proxy.h"

namespace wucmd {

  class Session : public wunet::BasicConn {
    public:
      // static void *init(void *x);
      void run();

      // Constructors/Destructors
      // The second and third args are for the control pipes
      Session(wunet::inetSock*, int, int, Proxy::Manager*);
      ~Session();

      // manipulating the connection list
      wucmd::conID_t addConn(wunet::inetSock* s);
      void           addSlaveConn(wunet::BasicConn*);
      void           rmConn(wunet::BasicConn *);
      // uses connLock_
      void            rmConn(wucmd::conID_t);
      wunet::BasicConn     *getConn(wucmd::conID_t);
      void           rmSlaveConn(wunet::BasicConn*);
      void           removeSlave();

      pthread_t& tid() {return tid_;}

      bool sendReply(wunet::msgBuf<wucmd::hdr_t>& mbuf,
                     wucmd::flags_t status,
                     wucmd::flags_t conn,
                     wucmd::flags_t err);
      bool  sendBack(wunet::msgBuf<wucmd::hdr_t>&mbuf,
                     wucmd::cmd_t cmd,
                     wucmd::flags_t status,
                     wucmd::flags_t conn,
                     wucmd::flags_t err);

      // Creating new connection objects
      wunet::inetSock* mkConnect();

      // Control helpers
      virtual wuctl::connInfo* toData(wuctl::connInfo*);

      // uses connLock_
      void mkConnList(std::vector<wuctl::connInfo*>& bvec);

      // the entry point
      void handler();

      // primary operations: receiving and sending messages
      // Will not throw an exception
      virtual bool doRecv(wunet::msgBuf<wucmd::hdr_t>&);
      // Will not throw an exception
      virtual bool doSend(wunet::msgBuf<wucmd::hdr_t>&);
      bool doSendNext(wunet::msgBuf<wucmd::hdr_t>&);

      bool cmdHandler();
      void doCmd();

      // Commands:
      void do_echo();
      void do_fwd();
      void do_open();
      void do_close();
      void do_getstatus();
      void reportStatus(wunet::msgBuf<wucmd::hdr_t>&, wucmd::cmd_t, wucmd::conID_t);
      void do_share();
      void do_peerfwd();
      void do_getsid();

      // array of command handlers
      void (Session::*remoteCmd[wucmd::cmdCount])();

      // commands from manager:
      void do_null(wuctl::sessionCmd::arg_type);
      void do_shutdown(wuctl::sessionCmd::arg_type);
      void do_closeconn(wuctl::sessionCmd::arg_type);
      void do_slave(wuctl::sessionCmd::arg_type);
      void do_masterresp(wuctl::sessionCmd::arg_type);

      // array of control command handlers
      void (Session::*controlCmd[wuctl::sessionCmd::cmdCount])(wuctl::sessionCmd::arg_type);

      std::string idString() {std::ostringstream ss; ss << cid(); return ss.str();}
      std::ostream &print(std::ostream &ss)
      {
        ss << "Session: Count " << cnt_ << ", ";
        return wunet::BasicConn::print(ss);
      }

      bool validCID(wucmd::conID_t id) const {return (id > wucmd::invalidCID && id < FD_SETSIZE);}
      bool validCID(int id)            const {return validCID(wucmd::conID_t(id));}
      bool validConn(int id)           const {return validCID(id) == true && cons_[id] != NULL;}

      void close();

      bool setTxHandler(wunet::BasicConn *conn, wunet::msgBuf<wucmd::hdr_t> *mbuf);

      void setTxNextSlave();

      void clearTxHandler();

      bool notSlave();
    
      enum Status {Normal, Master, SlavePending, Slave, Exiting};

    private:
      Status status_;
      int  rcfd() const {return rcfd_;}
      int &rcfd() {return rcfd_;}
      int  wcfd() const {return wcfd_;}
      int &wcfd() {return wcfd_;}
      wunet::BasicConn            *txConn_;
      wunet::BasicConn            *txNext_;
      wunet::msgBuf<wucmd::hdr_t> *txBuf_;
      wunet::msgBuf<wucmd::hdr_t> ctlBuf_;
      void shutdown(); // just the connections
      int          rcfd_; // control pipe file descriptor; read from manager
      int          wcfd_; // control pipe file descriptor; write to manager
      pthread_t    tid_;
      fd_set      rset_; // read set
      fd_set      wset_; // write set
      int        maxsd_; // max socket descriptor value + 1
      int          cnt_;
      Session *master_;
      Session *slave_;
      Proxy::Manager *manager_;
      wunet::BasicConn* cons_[FD_SETSIZE];
      //
      pthread_mutex_t connLock_;
 
  };

  inline wucmd::conID_t Session::addConn(wunet::inetSock* ss)
  {
    if (ss == NULL)
      throw(wunet::netErr(wupp::progError, "Session::addConn NULL socket passed"));
    if (!ss->isOpen())
      throw(wunet::netErr(wupp::progError, "Session::addConn Closed socket passed"));

    register int sd = ss->sd();
    if (!validCID(sd))
      throw(wunet::netErr(wupp::progError, "Session::addConn invalid socket descriptor (%d)\n", sd));

    // The remainder of this function will not throw an exception so
    // the function will complete normally after allocating this object.
    // If the object can not be allocated then cons_[sd] will not be altered.
    cons_[sd] = new StreamConn(this, ss);
    FD_SET(sd, &rset_);
    if (sd >= maxsd_)
      maxsd_ = sd + 1;
    ++cnt_;

    wulog(wulogSession, wulogInfo, "Session::addConn(%s): Added connection %d\n",
          this->idString().c_str(), sd);
    return wucmd::conID_t(sd);
  };

  inline void Session::addSlaveConn(wunet::BasicConn* bc)
  {
    cons_[bc->cid()] = bc;
    FD_SET(bc->cid(), &rset_);
    if (bc->cid() >= maxsd_)
      maxsd_ = bc->cid() + 1;
    ++cnt_;

    wulog(wulogSession, wulogInfo, "Session::addSlaveConn(%s): Added connection %d\n",
          this->idString().c_str(), bc->cid());
  }

  inline void Session::rmConn(wunet::BasicConn *conn)
  {
    if (conn)
      rmConn(wucmd::conID_t(conn->cid()));
    return;
  }

  inline void Session::removeSlave()
  {
    if(status_ != Master) {
      wulog(wulogSession, wulogWarn, "Session::removeSlave(%s): called when not in master context!\n",
          this->idString().c_str());
      return;
    }

    for (int i = 0; i < FD_SETSIZE; i++) {
      rmSlaveConn(slave_->cons_[i]);
    }

    status_ = Normal;
    slave_ = NULL;
    return;
  }

  inline void Session::rmSlaveConn(wunet::BasicConn *conn)
  {
    if(!conn)
      return;

    register int conID = int(wucmd::conID_t(conn->cid()));

    wulog(wulogSession, wulogVerb, "Session::rmSlaveConn(%s): Removing connection %d\n",
          this->idString().c_str(), conID);

    if (!validConn(conID) || conID == cid())
      return;

    autoLock cLock(connLock_);
    if (!cLock)
      wulog(wulogSession, wulogError, "Session::rmConn: Error locking connLock\n");

    cons_[conID] = NULL;

    FD_CLR(conID, &rset_);
    --cnt_;

    if ((conID +1) == maxsd_) {
      // recalculate the max conID
      maxsd_ = 0;
      for (int i = 0; i < FD_SETSIZE; i++) {
        if (cons_[i] && cons_[i]->sd() >= maxsd_)
          maxsd_ = cons_[i]->sd() + 1;
      }
    }
    wulog(wulogSession, wulogInfo, "Session::rmSlaveConn(%s): Removed connection %d\n",
          this->idString().c_str(), conID);
  }

  inline void Session::shutdown()
  {
    // rmConn will remove but not delete the session object from the list
    for (int i = 0; i < FD_SETSIZE; i++) {
      rmConn(cons_[i]);
    }
    return;
  }

  // will not close this (Session) object
  inline void Session::rmConn(wucmd::conID_t id)
  {
    register int conID = int(id);

    // verifies the index is within range and the entry is not NULL
    if (!validConn(conID) || conID == cid())
      return;

    autoLock cLock(connLock_);
    if (!cLock)
      wulog(wulogSession, wulogError, "Session::rmConn: Error locking connLock\n");

    cons_[conID]->close();
    delete cons_[conID];
    cons_[conID] = NULL;

    FD_CLR(conID, &rset_);
    --cnt_;

    if ((conID +1) == maxsd_) {
      // recalculate the max conID
      maxsd_ = 0;
      for (int i = 0; i < FD_SETSIZE; i++) {
        if (cons_[i] && cons_[i]->sd() >= maxsd_)
          maxsd_ = cons_[i]->sd() + 1;
      }
    }
    wulog(wulogSession, wulogInfo, "Session::remConn(%s): Removed connection %d\n",
          this->idString().c_str(), conID);
  }

  inline wunet::BasicConn *Session::getConn(wucmd::conID_t id)
  {
    if (validCID(id))
      return cons_[id];
    return NULL;
  }

  inline bool Session::sendReply(wunet::msgBuf<wucmd::hdr_t>& mbuf,
                                 wucmd::flags_t status,
                                 wucmd::flags_t conn,
                                 wucmd::flags_t err)
  {
    return sendBack(mbuf, wucmd::cmdReply, status, conn, err);
  }

  inline bool Session::sendBack(wunet::msgBuf<wucmd::hdr_t>&mbuf,
                                wucmd::cmd_t cmd,
                                wucmd::flags_t status,
                                wucmd::flags_t conn,
                                wucmd::flags_t err)
  {
    mbuf.plen(wucmd::replyData::dlen());
    // hdr: mid_ and cid_ are unchanged
    mbuf.hdr().cmd(cmd);
    mbuf.hdr().dlen(mbuf.plen());

    wucmd::replyData *rdata = (wucmd::replyData *)mbuf.ppullup();
    // First initialize flags to 0
    rdata->flags(0);
    rdata->cmdStatus(status);
    rdata->connStatus(conn); // closed
    rdata->errFlags(err);

    return true;
  }

  inline bool Session::doRecv(wunet::msgBuf<wucmd::hdr_t>& mbuf)
  try {
    if (! mbuf.msgPartial())
      mbuf.reset();

    // returns:
    //   message fully read : dataComplete
    //   only partial read  : dataPartial
    //   end of file        : EndOfFile
    //   Otherwise wupp::isError(et) will return true
    wupp::errType et = mbuf.readmsg(sock());
    if (wupp::isError(et)) {
      wulog(wulogSession, wulogError, "Session::doRecv(%s): Error reading conn\n",
          this->idString().c_str());
      etype() = et;
      status_ = Exiting;
      close();
      return false;
    } else if (et == wupp::EndOfFile) {
      wulog(wulogSession, wulogInfo, "Session::doRecv(%s): Session EOF\n",
             this->idString().c_str());
      etype() = et;
      status_ = Exiting;
      close();
      return false;
    }
    setRxPending(mbuf.msgPartial());

    rxMsgStats(mbuf);
    return true;

  } catch (wupp::errorBase& e) {
    wulog(wulogSession, wulogError, "Session::doRecv(%s): %s\n",
          this->idString().c_str(), e.toString().c_str());
    etype() = e.etype();
    status_ = Exiting;
    close();
    return false;
  } catch (...) {
    wulog(wulogSession, wulogError, "Session::doRecv(%s): unknown exception types\n",
          this->idString().c_str());
    etype() = wupp::sysError;
    status_ = Exiting;
    close();
    return false;
  }

  inline bool Session::doSendNext(wunet::msgBuf<wucmd::hdr_t>& mbuf)
  {
    if(status_ != Master) {
      wulog(wulogSession, wulogWarn, "Session::doSendNext(%s): called when not in master context!\n",
          this->idString().c_str());
      return false;
    }
    if(txNext_ == NULL) {
      return true;
    }

    BasicConn *next = txNext_;
    if(mbuf.hdr().cmd() == wucmd::shareReply) {
      mbuf.hdr().conID(cid());
      txNext_ = NULL;
    }
    if(mbuf.hdr().cmd() == wucmd::fwdData) {
      mbuf.hdr().cmd(wucmd::copyFwd);
      txNext_ = NULL;
    }
    return next->doSend(mbuf);
  }

  inline bool Session::doSend(wunet::msgBuf<wucmd::hdr_t>& mbuf)
  try {
    // returns NoError otherwise throws exception
    // returns:
    //   message fully written : dataComplete
    //   only partial written  : dataPartial
    //   end of file           : EndOfFile
    //   Otherwise wupp::isError(et) will return true
    wupp::errType et = mbuf.writemsg(sock());
    if (et == wupp::dataComplete) {
      clearTxHandler();
      setTxPending(false);
    } else if (et == wupp::dataPartial) {
      if (!setTxHandler(this, &mbuf)) {
        wulog(wulogSession, wulogError, "Session::doSend(%s): setTxHandler failed\n",
              this->idString().c_str());
        etype() = wupp::sysError;
        status_ = Exiting;
        close();
        return false;
      }
      setTxPending(true);
    } else if (et == wupp::EndOfFile) {
      wulog(wulogSession, wulogError, "Session::doSend(%s): EOF on send\n",
            this->idString().c_str());
      etype() = et;
      status_ = Exiting;
      close();
      return false;
    } else {
      // wupp::isError(et) must be true
      wulog(wulogSession, wulogError, "Session::doSend(%s): Error  on send\n",
          this->idString().c_str());
      etype() = et;
      status_ = Exiting;
      close();
      return false;
    }

    txMsgStats(mbuf);

    if(status_ != Normal && status_ != SlavePending) {
      if(slave_ != NULL && et == wupp::dataComplete) {
        return this->doSendNext(mbuf);
      }
      else if (master_ != NULL && et == wupp::dataComplete) {
        return master_->doSendNext(mbuf);
      }
    }
    return true;

  } catch (wupp::errorBase& e) {
    wulog(wulogSession, wulogError, "Session::doSend(%s): wupp exception:\n\t%s\n",
          this->idString().c_str(), e.toString().c_str());
    etype() = e.etype();
    clearTxHandler();
    status_ = Exiting;
    close();
    return false;
  } catch (...) {
    wulog(wulogSession, wulogError, "Session::doSend(%s): Unknown exception\n",
          this->idString().c_str());
    etype() = wupp::sysError;
    clearTxHandler();
    status_ = Exiting;
    close();
    return false;
  }

  inline bool Session::notSlave()
  {
    if(status_ == Slave) {
      return false;
    }
    return true;
  }

  inline void Session::close()
  {
    wulog(wulogSession, wulogInfo, "Session::close(%s): Closing control channels.\n",
          this->idString().c_str());

    // first close to let manager thread know this session control channel is not available
    if (rcfd() != -1)
      ::close(rcfd());
    if (wcfd() != -1)
      ::close(wcfd());
    rcfd() = -1;
    wcfd() = -1;

    if(status_ == Slave) {
      return;
    }

    wulog(wulogSession, wulogInfo, "Session::close(%s): Closing session and all related connections.\n",
          this->idString().c_str());

    if(master_) { 
      master_->removeSlave();
    }

    // shutdown will also close this object
    shutdown();
    BasicConn::close();
  }

  inline void Session::setTxNextSlave()
  {
    txNext_ = slave_;
  }
  
  inline bool Session::setTxHandler(wunet::BasicConn *conn, wunet::msgBuf<wucmd::hdr_t> *mbuf)
  {
    if(master_ != NULL) {
      return master_->setTxHandler(conn,mbuf);
    }
    if (txConn_ || conn == NULL || !conn->check() || mbuf == NULL)
      return false;
    // add session to TX Descriptor list and remove from rx descriptor
    // list.
    txConn_ = conn;
    txBuf_  = mbuf;
    FD_SET((int)conn->sd(), &wset_);
    return true;
  }

  inline void Session::clearTxHandler()
  {
    if (txConn_) {
      // since only one connection will ever be on hte write list just zero
      FD_ZERO(&wset_);
      txConn_ = NULL;
      txBuf_  = NULL;
    }
  }
};
#endif
