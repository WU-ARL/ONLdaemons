/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/session.cc,v $
 * $Author: cgw1 $
 * $Date: 2008/04/09 18:39:35 $
 * $Revision: 1.17 $
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

void Session::do_null(wuctl::sessionCmd::arg_type arg)
{
  if (arg != wuctl::sessionCmd::nullArg) {
    wulog(wulogSession, wulogWarn,
          "Session::do_null(%s): Ignoring arg\n", this->idString().c_str());
  }
  wulog(wulogSession, wulogLoud, "Session::do_null(%s): Doing nothing\n", this->idString().c_str());
}

void Session::do_shutdown(wuctl::sessionCmd::arg_type arg)
{
  if (arg != wuctl::sessionCmd::nullArg) {
    wulog(wulogSession, wulogWarn,
          "Session::do_shutdown(%s): Ignoring arg\n", this->idString().c_str());
  }
  wulog(wulogSession, wulogLoud, "Session::do_shutdown(%s): Shutting Down\n", this->idString().c_str());
  close();
}

void Session::do_closeconn(wuctl::sessionCmd::arg_type arg)
{
  if (arg == wuctl::sessionCmd::nullArg) {
    wulog(wulogSession, wulogError,
          "Session::do_closeconn(%s): Arg is null! Ignoring command\n", this->idString().c_str());
    return;
  }
  conID_t conID = (conID_t)arg;
  if (validConn(conID) && conID != cid()) {
    wulog(wulogSession, wulogInfo, "Session::do_closeconn(%s): removing CID %d\n",
                this->idString().c_str(), conID);
    // this will close() and remove from list and delete object
    rmConn(conID);
  } else {
    wulog(wulogSession, wulogWarn,
          "Session::do_closeconn(%s): Requested to remove an invalid CID %d... closing\n",
          this->idString().c_str(), conID);
    close();
  }
}

void Session::do_slave(wuctl::sessionCmd::arg_type arg)
{
  if (arg == wuctl::sessionCmd::nullArg) {
    wulog(wulogSession, wulogError,
          "Session::do_slave(%s): Arg is null! Ignoring command\n", this->idString().c_str());
    return;
  }

  wulog(wulogSession, wulogVerb, "do_slave(%s): making %d my slave\n", 
        this->idString().c_str(), arg);

  wuctl::sessionCmd::arg_type sr;
  slave_ = manager_->getSessionObj(arg);
  if(slave_ == NULL) {
    sr = wuctl::sessionCmd::slaveReject;
  }
  else {
    sr = wuctl::sessionCmd::slaveOK;
  }

  wuctl::sessionCmd mmsg(wuctl::sessionCmd::cmdSlaveResp, sr);
  if (mmsg.writeCmd(this->wcfd()) < 0) {
        wulog(wulogSession, wulogError,
              "Manager::do_slave(%s): Error sending ok response to manager... closing\n",
              this->idString().c_str());
    slave_ = NULL;
    close();
  }
  
  if(slave_ == NULL) {
    return;
  }

  slave_->master_ = this;
  status_ = Master;

  ctlBuf_.reset();
  ctlBuf_.hdr().msgID(0);
  ctlBuf_.hdr().conID(slave_->cid());
  sendBack(ctlBuf_, wucmd::shareReply, wucmd::cmdSuccess, wucmd::connOpen, wucmd::noError);
  txNext_ = slave_;

  if(doSend(ctlBuf_) == false) {
      wulog(wulogSession, wulogError,
          "Session::do_slave(%s): Unable to send ok status back to RLI... closing\n",
          this->idString().c_str());
    close();
  }

  wunet::BasicConn *sbc;
  for (int sd = 0; sd < slave_->maxsd_; sd++)
  {
    if((sbc = slave_->getConn(sd)) != NULL) {
      addSlaveConn(sbc);
    }
  }
  

  return;
}

void Session::do_masterresp(wuctl::sessionCmd::arg_type arg)
{
  if (arg == wuctl::sessionCmd::nullArg) {
    wulog(wulogSession, wulogError,
          "Session::do_masterresp(%s): Arg is null! Ignoring command\n", this->idString().c_str());
    return;
  }

  wulog(wulogSession, wulogVerb, "do_masterresp(%s): got result %d\n", 
        this->idString().c_str(), arg);

  if(arg == wuctl::sessionCmd::masterOK)
  {
    status_ = Slave;
    return;
  }

  wucmd::flags_t ec = wucmd::sysError;
  if (arg == wuctl::sessionCmd::masterNotExist) {
    ec = wucmd::badConnID;
  }
  else if (arg == wuctl::sessionCmd::masterReject) {
    ec = wucmd::authError;
  }
  else if (arg == wuctl::sessionCmd::masterError) {
    ec = wucmd::connFailure;
  }
 
  ctlBuf_.reset();
  ctlBuf_.hdr().msgID(0);
  ctlBuf_.hdr().conID(cid());
  sendBack(ctlBuf_, wucmd::shareReply, wucmd::cmdFailure, wucmd::connError, ec);
  if (doSend(ctlBuf_) == false) {
      wulog(wulogSession, wulogError,
        "Session::do_masterresp(%s): Unable to send error status back to RLI.. closing\n",
        this->idString().c_str());
      close();
  }
  status_ = Normal;
  master_ = NULL;
}

bool Session::cmdHandler()
{
try {
  // read from control channel
  wuctl::sessionCmd cmsg;
  if (cmsg.readCmd(rcfd()) < 0) {
    wulog(wulogSession, wulogError,
          "Session::cmdHandler(%s): Error reading control pipe... closing\n",
          this->idString().c_str());
    close();
    return false;
  }

  if(cmsg.cmd() >= wuctl::sessionCmd::cmdCount || controlCmd[cmsg.cmd()] == NULL)
  {
    wulog(wulogSession, wulogWarn, "Session::cmdHandler(%s): Invalid command %d\n",
          this->idString().c_str(), (int)cmsg.cmd());
    return true;
  }

  (this->*controlCmd[cmsg.cmd()])(cmsg.arg());

  return true;
} catch (wupp::errorBase& e) {
  wulog(wulogSession, wulogWarn,
        "Session::cmdHandler(%s): wupp Exception while processing command\n\tException = %s\n",
        this->idString().c_str(), e.toString().c_str());
  close();
  return false;
} catch (...) {
  wulog(wulogSession, wulogWarn,
        "Session::cmdHandler(%s): Exception caught while processing command\n",
        this->idString().c_str());
  close();
  return false;
}
}

#define TX_TIMEOUT_SEC 0
#define TX_TIMEOUT_USEC 200000
void Session::run()
{
  int error = 0;
  int tryCount = 0;

  wulog(wulogSession, wulogInfo, "Session::run(%s): Starting ...\n", this->idString().c_str());
  while (check()) {
    fd_set rxSet;
    fd_set txSet;
    int dcnt;

    struct timeval timeoutval;
    struct timeval *curtimeout;

    if (txConn_)
    {
      if (tryCount++ == 3) {
        wulog(wulogSession, wulogError,
              "Session::run(%s): failed to complete partial write after three attempts.. closing\n",
              this->idString().c_str());
        close();
        etype() = wupp::sysError;
        break;
      }

      FD_ZERO(&rxSet);
      txSet = wset_;

      timeoutval.tv_sec = TX_TIMEOUT_SEC;
      timeoutval.tv_usec = TX_TIMEOUT_USEC;
      curtimeout = &timeoutval;
    }
    else
    {
      if(closing_) 
      {
        break;
      }
      if(removingSlave_)
      {
        wulog(wulogSession, wulogVerb, "Session::run(%s): removing slave\n", this->idString().c_str());
        rmSlaveSess();
        delete slave_;
        status_ = Normal;
        slave_ = NULL;
        removingSlave_ = false;
      }
      if (status_ != Slave)
      {
        tryCount = 0;

        FD_ZERO(&txSet);
        rxSet = rset_;

        curtimeout = NULL;
      }
      else
      {
        wulog(wulogSession, wulogInfo, "Session::run(%s): Now a slave (thread exiting)\n",
              this->idString().c_str());
        close();
        return;
      }
    }
    if ((dcnt = select(maxsd_, &rxSet, &txSet, NULL, curtimeout)) < 0) {
      // EBADF, EINTR, EINVAL, ENOMEM
      if (errno == EINTR)
        wulog(wulogSession, wulogError,
              "Session::run(%s): select returned with EINTR ... closing ...\n",
              this->idString().c_str());
      else
        wulog(wulogSession, wulogError,
              "Session::run(%s): select returned error %d, closing ...\n",
              this->idString().c_str(), errno);
      close();
      etype() = wupp::sysError;
      break;
    } else if (dcnt == 0) {
      wulog(wulogSession, wulogLoud,
            "Session::run(%s): No msgs after timeout.. closing\n",
            this->idString().c_str());
      close();
      etype() = wupp::sysError;
      break;
    }
  
    // There will only ever be one tx pending 
    if (txConn_) {
      // I will simply assume this is correct
      txConn_->doSend(*txBuf_);
    }

    if (status_ == Slave) {
      continue;
    }
    for (int sd = 0; sd < maxsd_; sd++) {
      if (! FD_ISSET(sd, &rxSet))
        continue;

      if (sd == rcfd()) {
        // blocking socket
        if(!cmdHandler())
        {
          error = 1;
        }
        break;
      }
      // non-blocking sockets
      BasicConn *conns = getConn(sd);
      WULOG((wulogSession, wulogLoud, "Session::run(%s): Got message on cid %d\n",
              this->idString().c_str(), int(sd)));
      if (conns == NULL) {
        wulog(wulogSession, wulogWarn, "Session::run(%s): cid %d in ready set but conns == NULL... closing\n",
              this->idString().c_str(), sd);
        close();
        error=1;
        break;
      } else if (conns->check() == false) {
        wulog(wulogSession, wulogWarn, "Session::run(%s): cid %d in ready set but conns is not open... closing\n",
            this->idString().c_str(), sd);
        close();
        error=1;
        break;
      }

      // handler() will not throw an exception
      if(conns->handler() == 0)
      {
        if (conns->check() == false)
          rmConn(conns);
      }
      break;
    } // loop over all open decriptors

    if(error)
      break;

  } // while open

  if(removingSlave_)
  {
    wulog(wulogSession, wulogVerb, "Session::run(%s): removing slave\n", this->idString().c_str());
    rmSlaveSess();
    delete slave_;
    status_ = Normal;
    slave_ = NULL;
    removingSlave_ = false;
  }
  if(closing_)
  {
    wulog(wulogSession, wulogVerb, "Session::run(%s): closing\n", this->idString().c_str());
    BasicConn::close();
    closing_ = false;
  }

  wulog(wulogSession, wulogInfo, "Session::run(%s): shutting down\n", this->idString().c_str());
}

Session::Session(wunet::inetSock* ss, int rctlFD, int wctlFD, Proxy::Manager *man)
  : BasicConn(ss), txConn_(NULL), txNext_(NULL), txBuf_(NULL), ctlBuf_(), rcfd_(rctlFD), 
    wcfd_(wctlFD), maxsd_(0), cnt_(0), master_(NULL), slave_(NULL), manager_(man)
{
  for (int i = 0; i < FD_SETSIZE; i++)
    cons_[i] = NULL;

  FD_ZERO(&rset_);
  FD_ZERO(&wset_);

  if (! validSock()) {
    wulog(wulogSession, wulogError,
          "Session::Session: invalid socket passed");
    return;
  } else if (! validCID(sd())) {
    wulog(wulogSession, wulogError,
          "Session::Session: Invalid CID (i.e. sd) value = %d\n", sd());
    return;
  } else if (! validCID(rcfd())) {
    wulog(wulogSession, wulogError,
        "Session::Session: Invalid rcfd value = %d\n", rcfd());
    return;
  } else if (! validCID(wcfd())) {
    wulog(wulogSession, wulogError,
        "Session::Session: Invalid wcfd value = %d\n", wcfd());
    return;
  } else if (pthread_mutex_init((&connLock_), NULL) != 0) {
    wulog(wulogSession, wulogError,
        "Session::Session: Error initializing connection table lock\n");
    return;
  } else if (manager_ == NULL) {
    wulog(wulogSession, wulogError,
        "Session::Session: Invalid manager object\n");
    return;
  }

  wulog(wulogSession, wulogLoud, "Session::Session: wcfd_ = %d, rcfd_ = %d\n", wcfd(), rcfd());

  FD_SET(sd(), &rset_);
  FD_SET(rcfd(), &rset_);
  maxsd_ = std::max(sd(), rcfd()) + 1;
  cons_[sd()] = this;

  controlCmd[wuctl::sessionCmd::cmdNull] = &Session::do_null;
  controlCmd[wuctl::sessionCmd::cmdShutdown] = &Session::do_shutdown;
  controlCmd[wuctl::sessionCmd::cmdCloseConn] = &Session::do_closeconn;
  controlCmd[wuctl::sessionCmd::cmdSlave] = &Session::do_slave;
  controlCmd[wuctl::sessionCmd::cmdSlaveResp] = NULL;
  controlCmd[wuctl::sessionCmd::cmdMaster] = NULL;
  controlCmd[wuctl::sessionCmd::cmdMasterResp] = &Session::do_masterresp;

  remoteCmd[wucmd::cmdEcho] = &Session::do_echo;
  remoteCmd[wucmd::fwdData] = &Session::do_fwd;
  remoteCmd[wucmd::openConn] = &Session::do_open;
  remoteCmd[wucmd::closeConn] = &Session::do_close;
  remoteCmd[wucmd::getConnStatus] = &Session::do_getstatus;
  remoteCmd[wucmd::cmdReply] = NULL;
  remoteCmd[wucmd::connStatus] = NULL;
  remoteCmd[wucmd::echoReply] = NULL;
  remoteCmd[wucmd::shareConn] = &Session::do_share;
  remoteCmd[wucmd::peerFwd] = &Session::do_peerfwd;
  remoteCmd[wucmd::copyFwd] = NULL;
  remoteCmd[wucmd::getSID] = &Session::do_getsid;
  remoteCmd[wucmd::retSID] = NULL;
  remoteCmd[wucmd::shareReply] = NULL;

  status_ = Normal;
  closing_ = false;
  removingSlave_ = false;
}

Session::~Session()
{
  wulog(wulogSession, wulogVerb,
        "Session::~Session(%s): Shutting down all connections for session\n",
        this->idString().c_str());
  //close();
  pthread_mutex_destroy(&connLock_);
}

void Session::do_echo()
{
  // exceptions caught one level up
  wulog(wulogSession, wulogVerb, "Session::do_echo(%s): Cmd = %s\n",
        this->idString().c_str(), mbuf_.toString().c_str());
  mbuf_.hdr().cmd(wucmd::echoReply);
  if (! doSend(mbuf_)) {
     wulog(wulogSession, wulogError,
           "Session::do_echo(%s)):Error echoing data\n\tConn = %s\n\tMsg = %s\n",
           this->idString().c_str(), toString().c_str(), mbuf_.toString().c_str());
     close();
  }
}

void Session::do_fwd()
{
  // exceptions caught one level up
  wucmd::conID_t conID = mbuf_.hdr().conID();
  BasicConn *conn = getConn(conID);

  if (conn == NULL || ! conn->check()) {
    wulog(wulogSession, wulogInfo,
        "Session::do_fwd(%s): Request to send on a unopen connection %d\n",
        this->idString().c_str(), conID);
    reportStatus(mbuf_, wucmd::connStatus, conID);
    return;
  }

  wulog(wulogSession, wulogVerb, "Session::do_fwd(%s): forward to connection %s\n",
        this->idString().c_str(),conn->idString().c_str());

  if (conn->doSend(mbuf_) == false) {
    wulog(wulogSession, wulogError,
          "Session::do_fwd(%s): Error forwarding to conn %s\n\tConn = %s\n\tMsg = %s\n",
          this->idString().c_str(), conn->idString().c_str(),
          conn->toString().c_str(), mbuf_.toString().c_str());
    reportStatus(mbuf_, wucmd::connStatus, conID);
    rmConn(conID);
  }
  return;
}

wunet::inetSock* Session::mkConnect()
{
  wunet::inetSock *s = NULL;

  wucmd::connData cd;
  char  *data = (char *)mbuf_.ppullup();
  if (data == NULL)
    throw wunet::netErr(wupp::progError, "mkConnect: Error getting pointer to payload buffer");

  cd.copyIn(data, mbuf_.hdr().dlen());
  wunet::inetAddr serv(cd.host(), ntohs(cd.port()), cd.proto());

  s = new wunet::inetSock(wunet::inetAddr(IPPROTO_TCP));
  s->connect(serv);
  s->nodelay(1);
  s->nonblock(true);

  WULOG((wulogSession, wulogVerb, "Session::mkConnect(%s): created new connection: %s\n",
          this->idString().c_str(), s->toString().c_str()));

  return s;
}

void Session::do_open()
{
  if (status_ == Slave || status_ == SlavePending) {
    wulog(wulogSession, wulogWarn,
          "Session::do_open(%s): ignoring message (slave)\n", this->idString().c_str());
    return;
  }

  wucmd::flags_t cmdStat  = wucmd::cmdSuccess;
  wucmd::flags_t connStat = wucmd::connOpen;
  wucmd::flags_t errCode  = wucmd::noError;
  wunet::inetSock* ss = NULL;
  
  try {
    ss = mkConnect();
    // this will not throw an exception
    wucmd::conID_t newID = addConn(ss);
    mbuf_.hdr().conID(newID);
    wulog(wulogSession, wulogVerb, "Session::do_open(%s): added new connection %d\n",
          this->idString().c_str(), int(newID));

  } catch (wupp::errorBase& e) {
    wulog(wulogSession, wulogWarn,
          "Session::do_open(%s): Error creating socket/connection\n\tMsg = %s\n\tException = %s\n",
          this->idString().c_str(), mbuf_.toString().c_str(), e.toString().c_str());
    if (ss != NULL)
      delete ss;
    ss = NULL;
    mbuf_.hdr().conID(wucmd::invalidCID);
    cmdStat  = wucmd::cmdFailure;
    connStat = wucmd::connError;
    errCode  = wucmd::hdr_t::err2flag(e.etype());
  } catch (...) {
    wulog(wulogSession, wulogWarn, "do_open(%s): Error creating socket/connection\n\tMsg = %s\n",
          this->idString().c_str(), mbuf_.toString().c_str());
    if (ss != NULL)
      delete ss;
    ss = NULL;
    mbuf_.hdr().conID(wucmd::invalidCID);
    cmdStat  = wucmd::cmdFailure;
    connStat = wucmd::connError;
    errCode  = wucmd::hdr_t::err2flag(wupp::sysError);
  }

  sendReply(mbuf_, cmdStat, connStat, errCode);
  if (doSend(mbuf_) == false)
  {
    wulog(wulogSession, wulogError,
          "Session::do_open(%s): Unable to send status back to RLI\n\tMsg = %s\n",
          this->idString().c_str(), mbuf_.toString().c_str());
    close();
  }

  return;
}

void Session::do_close()
{
  int sendreply = 1;
  wucmd::flags_t cmdStat  = wucmd::cmdSuccess;
  wucmd::flags_t errCode  = wucmd::noError;

  BasicConn *conn = getConn(mbuf_.hdr().conID());

  if(status_ == Normal)
  {
    if (conn == NULL)
    {
      wulog(wulogSession, wulogWarn, "Session::do_close(%s): Connection %d does not exist\n", this->idString().c_str(), mbuf_.hdr().conID());
      cmdStat = wucmd::cmdFailure;
      errCode = wucmd::connNotOpen;
    }
    else if (mbuf_.hdr().conID() == cid())
    {
      close();
      sendreply = 0;
    }
    else
    {
      rmConn(mbuf_.hdr().conID());
    }
  }
  else if(status_ == Master)
  {
    if (conn == NULL)
    {
      wulog(wulogSession, wulogWarn, "Session::do_close(%s): Connection %d does not exist\n", this->idString().c_str(), mbuf_.hdr().conID());
      cmdStat = wucmd::cmdFailure;
      errCode = wucmd::connNotOpen;
    }
    else if (mbuf_.hdr().conID() == cid())
    {
      close();
      sendreply = 0;
    }
    else if(mbuf_.hdr().conID() == slave_->cid())
    {
      removeSlave(slave_->cid());
      sendreply = 0;
    }
    else
    {
      rmConn(mbuf_.hdr().conID());
    }
  }
  else if(status_ == Slave)
  {
    if (conn == NULL)
    {
      wulog(wulogSession, wulogWarn, "Session::do_close(%s): Connection %d does not exist\n", this->idString().c_str(), mbuf_.hdr().conID());
      cmdStat = wucmd::cmdFailure;
      errCode = wucmd::connNotOpen;
    }
    else if(mbuf_.hdr().conID() == cid())
    {
      master_->removeSlave(cid());
      sendreply = 0;
    }
    else
    {
      rmConn(mbuf_.hdr().conID());
    }
  }
  else
  {
    wulog(wulogSession, wulogWarn, "Session::do_close(%s): ignoring message\n", this->idString().c_str());
    cmdStat = wucmd::cmdFailure;
    errCode = wucmd::connFailure;
    sendreply = 0;
  }

  if(sendreply)
  {
    // does not throw an exception
    sendReply(mbuf_, cmdStat, 0, errCode);
    if (doSend(mbuf_) == false)
    {
      wulog(wulogSession, wulogError,
            "Session::do_close(%s): Unable to send status back to RLI\n\tMsg = %s\n",
            this->idString().c_str(), mbuf_.toString().c_str());
      close();
    }
  }
  return;
}

void Session::do_getstatus()
{
  wulog(wulogSession, wulogVerb,
        "Session::do_getstatus(%s): conID %d \n",
        this->idString().c_str(), int(mbuf_.hdr().conID()));
  reportStatus(mbuf_, wucmd::cmdReply, mbuf_.hdr().conID());
}

void Session::reportStatus (wunet::msgBuf<wucmd::hdr_t>& mbuf, wucmd::cmd_t cmd, wucmd::conID_t connID)
{
  BasicConn *conn = getConn(connID);
  wucmd::flags_t st = 0;
  wucmd::flags_t er = wucmd::connNotOpen;
  st = conn2Flags(conn);
  if (conn) {
    er = wucmd::hdr_t::err2flag(conn->etype());
  }
  mbuf.hdr().conID(connID);

  sendBack(mbuf, cmd, wucmd::cmdSuccess, st, er);
  if (doSend(mbuf) == false)
  {
    wulog(wulogSession, wulogError,
          "Session::reportStatus(%s): Unable to send status back to RLI\n\t\n\tConn %s\n\tMsg = %s\n",
          this->idString().c_str(), toString().c_str(), mbuf.toString().c_str());
    close();
  }

  return;
}

void Session::do_share()
{
  if (status_ == Slave || status_ == SlavePending) {
    wulog(wulogSession, wulogWarn,
          "Session::do_share(%s): ignoring message (slave)\n", this->idString().c_str());
    return;
  }
  wucmd::sessData sd;

  char *pload = (char *)mbuf_.ppullup();
  if (pload == NULL) {
    throw wunet::netErr(wupp::progError, "do_share: Error getting pointer to payload buffer");
  }
  sd.copyIn(pload);
  
  wulog(wulogSession, wulogVerb,
        "Session::do_share(%s): becoming an observer to session %d, writing to fd %d\n",
        this->idString().c_str(), sd.sessID(), this->wcfd());

  wuctl::sessionCmd mmsg(wuctl::sessionCmd::cmdMaster, sd.sessID());
  if (mmsg.writeCmd(this->wcfd()) < 0) {
    wulog(wulogSession, wulogError, "Session::do_share: Error sending command to manager\n");

    sendBack(mbuf_, wucmd::cmdReply, wucmd::cmdFailure, 0, wucmd::sysError);
    if (doSend(mbuf_) == false) {
      wulog(wulogSession, wulogError,
          "Session::do_share(%s): Unable to send status back to RLI\n",
          this->idString().c_str());
      close();
    }
  }
  else {
    status_ = SlavePending;
    wulog(wulogSession, wulogVerb,
        "Session::do_share(%s): status set to SlavePending\n",
        this->idString().c_str());
  }

  return;
}

void Session::do_peerfwd()
{
  BasicConn *peer;
  if(status_ == Master)
  {
    if(slave_ == NULL) {
      wulog(wulogSession, wulogWarn,
        "Session::do_peerfwd(%s): in master context, slave is null\n", this->idString().c_str());
      return;
    }
    peer = slave_;
  }
  else if(status_ == Slave)
  {
    if(master_ == NULL) {
      wulog(wulogSession, wulogWarn,
        "Session::do_peerfwd(%s): in slave context, master is null\n", this->idString().c_str());
      return;
    }
    peer = master_;
  }
  else
  {
    wulog(wulogSession, wulogWarn,
      "Session::do_peerfwd(%s): Session not master or slave!\n", this->idString().c_str());
      return;
  }

  wulog(wulogSession, wulogVerb, "Session::do_peerfwd(%s): forward to connection %s\n",
        this->idString().c_str(),peer->idString().c_str());

  if (peer->doSend(mbuf_) == false) {
    wulog(wulogSession, wulogError,
          "Session::do_fwd(%s): Error forwarding to conn %s\n\tConn = %s\n\tMsg = %s\n",
          this->idString().c_str(), peer->idString().c_str(),
          peer->toString().c_str(), mbuf_.toString().c_str());
    reportStatus(mbuf_, wucmd::connStatus, mbuf_.hdr().conID());
  }
  return;
}

void Session::do_getsid()
{
  wucmd::flags_t cmdStat  = wucmd::cmdSuccess;
  wucmd::flags_t connStat = wucmd::connOpen;
  wucmd::flags_t errCode  = wucmd::noError;

  mbuf_.hdr().cmd(wucmd::retSID);
  mbuf_.plen(wucmd::replySessData::dlen());
  mbuf_.hdr().dlen(mbuf_.plen());
  wucmd::replySessData *rsd = (wucmd::replySessData *)mbuf_.ppullup();

  rsd->replydata()->flags(0);
  rsd->replydata()->cmdStatus(cmdStat);
  rsd->replydata()->connStatus(connStat);
  rsd->replydata()->errFlags(errCode);

  rsd->sessdata()->sessID(this->cid());

  wulog(wulogSession, wulogVerb, "Session::do_getsid(%s): session ID %d \n",
        this->idString().c_str(), int(rsd->sessdata()->sessID()));

  if (! doSend(mbuf_)) {
     wulog(wulogSession, wulogError,
           "Session::do_getsid(%s)):Error sending session id data\n\tMsg = %s\n",
           this->idString().c_str(), mbuf_.toString().c_str());
    close();
  }
}

// no exceptions should propagate up to this function!
void Session::doCmd()
try {
  if(mbuf_.hdr().cmd() >= wucmd::cmdCount || remoteCmd[mbuf_.hdr().cmd()] == NULL) {
    wulog(wulogSession, wulogWarn, "Session::doCmd: Invalid command %d\n",(int)mbuf_.hdr().cmd());
    return;
  }

  (this->*remoteCmd[mbuf_.hdr().cmd()])();

  return;
} catch (wupp::errorBase& e) {
  wulog(wulogSession, wulogError,
        "Session::doCmd(%s): Unexpected Exception\n\t%s\n", 
        this->idString().c_str(), e.toString().c_str());
  // assume socket is already closed
  etype() = e.etype();
  close();
  return;
} catch (...) {
  wulog(wulogSession, wulogError,
        "Session::doCmd(%s): Unexpected Exception, unknown exception type\n",
        this->idString().c_str());
  etype() = wupp::sysError;
  close();
  return;
}

int Session::handler()
{
  // neither doRecv nor doCmd() will throw exceptions
  if (! doRecv(mbuf_)) 
  {
    if(status_ == Slave)
    {
      master_->connLost(cid());
    }
    return -1;
  }

  if(mbuf_.msgComplete()) doCmd();

  return 0;
}

wuctl::connInfo* Session::toData(wuctl::connInfo* cd)
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

  uint8_t flags = wuctl::connInfo::flagSession;
  if (isOpen())
    flags |= wuctl::connInfo::flagOpen;
  if (isRxPending() || isTxPending())
    flags |= wuctl::connInfo::flagPending;

  cd->flags(flags);

  // stats
  cd->stats(stats_);

  return cd;
}

void Session::mkConnList(std::vector<wuctl::connInfo*>& bvec)
{
  bvec.push_back(this->toData(new wuctl::connInfo()));

  // don't want cons_ changing while it is being read
  autoLock cLock(connLock_);
  if (!cLock)
  {
    wulog(wulogSession, wulogError, "Session::mkConnList: Error locking connLock\n");
    close();
  }
  for (int i = 0; i < FD_SETSIZE; i++) {
    if (cons_[i] != NULL && cons_[i] != this)
      bvec.push_back(cons_[i]->toData(new wuctl::connInfo()));
  }
}
