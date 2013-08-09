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
 * $Revision: 1.17 $
 * $Name:  $
 *
 * File:         proxy.cc
 * Created:      02/01/2005 03:36:20 PM CST
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
#include <algorithm>

#include <iostream>
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
#include <signal.h>

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
#include <wuPP/util.h>

using namespace Proxy;

// time given for all threads to exit, otherwise the process is terminated
#define PROXY_TIMEOUT_SEC 2

Manager::Manager *Manager::manager = NULL;
Manager::Status Manager::status = Manager::Down;

const wulogMod_t wulogProxy = wulogMod_t(wulogUserStart);
const int modCnt = 1;

const wulogInfo_t proxyInfo[] = {
  {wulogProxy, wulogInfo, "Proxy", "network core"},
};

Manager::Manager *Manager::getManager(const std::string& ip, in_port_t pn, int pr)
{
  try {
    if (Manager::manager == NULL)
      Manager::manager = new Manager(ip, pn, pr);
  } catch (...) {
    wulog(wulogProxy, wulogFatal,
        "Manager::getManager: Unable to allocate mamanger object: Unknown exception\n");
  }

  return Manager::manager;
}

Manager::Manager(const std::string& saddr, in_port_t sp, int pr)
  : lsock_(wunet::inetAddr(saddr.c_str(), sp, pr), SOCK_STREAM),
    csock_(wunet::inetAddr(saddr.c_str(), sp+1, pr), SOCK_STREAM),
    maxsd_(0) 
{
  try {
    if (!lsock_ || !csock_) {
      wulog(wulogProxy, wulogFatal, "Manager::Manager: Unable to init sockets\n");
      open_ = false;
      return;
    }

    lsock_.listen();
    csock_.listen();
    // Now initialize the svec mutex
    if (pthread_mutex_init((&svecLock_), NULL) != 0) {
      wulog(wulogProxy, wulogFatal, "Manager::Manager: Unable to init svecLock\n");
      open_ = false;
      return;
    }

    if (pthread_cond_init (&svecWait_, NULL) != 0) {
      wulog(wulogProxy, wulogFatal, "Manager::Manager: Unable to init svecWait_\n");
      open_ = false;
      return;
    }

    FD_ZERO(&rset_);
    FD_SET(lsock_.sd(), &rset_);
    FD_SET(csock_.sd(), &rset_);
    maxsd_ = std::max(lsock_.sd(), csock_.sd()) + 1;

    sessCmd[wuctl::sessionCmd::cmdNull] = &Manager::do_null;
    sessCmd[wuctl::sessionCmd::cmdShutdown] = NULL;
    sessCmd[wuctl::sessionCmd::cmdCloseConn] = NULL;
    sessCmd[wuctl::sessionCmd::cmdSlave] = NULL;
    //sessCmd[wuctl::sessionCmd::cmdSlaveResp] = &Manager::do_slaveresp;
    sessCmd[wuctl::sessionCmd::cmdSlaveResp] = NULL;
    sessCmd[wuctl::sessionCmd::cmdMaster] = &Manager::do_master;
    sessCmd[wuctl::sessionCmd::cmdMasterResp] = NULL;

    open_ = true;
    status = Running;
  } catch (wunet::netErr& e) {
    wulog(wulogProxy, wulogError, "Manager::Manager: Caught a netErr exception (calling listen):\n\t%s",
          e.toString().c_str());
    open_ = false;
    lsock_.close();
    csock_.close();
    return;
  } catch (...) {
    wulog(wulogProxy, wulogError, "Manager::Manager: Caught a Unknown exception (calling listen).\n");
    open_ = false;
    lsock_.close();
    csock_.close();
    return;
  }
}

void Manager::sigTerm(int)
{
  // alright, I shouldn't do this but I'll cross my fingers
  // write(STDOUT_FILENO, "XXX", 4);
  // wulog(wulogProxy, wulogFatal, "Received signal %d, killing all sessions\n", sig);

  Manager::status = Shutdown;
  return;
}

void Manager::sigFail(int)
{
  // alright, I shouldn't do this but I'll cross my fingers
  // write(STDOUT_FILENO, "XXX", 4);
  // wulog(wulogProxy, wulogFatal, "Received signal %d, terminating process\n", sig);

  _exit(1);
  return;
}

void Manager::sigHandler(int)
{
  // wulog(wulogProxy, wulogInfo, "Manager::sigHandler: Got signal %d\n", signo);
  return;
}

void Manager::run()
{
  int r;
  fd_set rxSet;

  // First set up signal handlers and masks. These will be
  // inhereted by all threads we spawn.
  struct sigaction sa;
  // ------------------------- Install Signal Handlers -----------------------
  // Signals to Ignore
  // Note: para sa to sigaction is declared to be a const so we need not worry
  // about the system call changing the memeber values. fk
  sa.sa_handler = SIG_IGN;
  sa.sa_flags   = 0;
  if (sigemptyset(&sa.sa_mask) != 0) {
    wulog(wulogProxy, wulogError, "Manager::run: Unable to zero sigmask\n");
    goto run_cleanup;
  }
  if (sigaction(SIGHUP, &sa, NULL) != 0) {
    wulog(wulogProxy, wulogError, "run: Unable to install SIGHUP handler\n");
    goto run_cleanup;
  }
  if (sigaction(SIGPIPE, &sa, NULL) != 0) {
    wulog(wulogProxy, wulogError, "Manager::run: seting SIGPIPE to ignore, errno = %d\n", errno);
    goto run_cleanup;
  }
  wulog(wulogProxy, wulogInfo, "Manager::run: Ignoring signals SIGHUP and SIGPIPE\n");

  // Using sigHandler
  sa.sa_handler = Manager::sigHandler;
  if (sigaction(SIGUSR1, &sa, NULL) != 0) {
    wulog(wulogProxy, wulogError, "Manager::run: Unable to install SIGUSR1 handler\n");
    goto run_cleanup;
  }
  wulog(wulogProxy, wulogInfo, "Manager::run: Installing 'sigHandler' for SIGUSR1\n");

  // Using sigTerm
  sa.sa_handler = Manager::sigTerm;
  if (sigaction(SIGTERM, &sa, NULL) != 0) {
    wulog(wulogProxy, wulogError, "run: Unable to install SIGTERM handler\n");
    goto run_cleanup;
  }
  if (sigaction(SIGINT, &sa, NULL) != 0) {
    wulog(wulogProxy, wulogError, "run: Unable to install SIGINT handler\n");
    goto run_cleanup;
  }
  sa.sa_handler = Manager::sigFail;
  if (sigaction(SIGSEGV, &sa, NULL) != 0) {
    wulog(wulogProxy, wulogError, "run: Unable to install SIGSEGV handler\n");
    goto run_cleanup;
  }
  wulog(wulogProxy, wulogInfo, "Manager::run: Installing 'sigTerm' for SIGTERM, SIGSEGV and SIGINT\n");
  // -------------------- Done installing signal handlers ---------------------

  wulog(wulogProxy, wulogInfo, "Manager::run: Proxy listening on sockets (RLI):\n%s\nand (Control) %s\n",
      (lsock_.toString()).c_str(), (csock_.toString()).c_str());

  while (status == Running && isOpen()) {
  
    rxSet = rset_;

    if ((r = select(maxsd_, &rxSet, NULL, NULL, NULL)) < 0) {
      // EBADF, EINTR, EINVAL, ENOMEM
      if (errno == EINTR) {
        wulog(wulogProxy, wulogVerb, "Manager::run: Manager thread got EINTR from select call\n");
        continue;
      }
      wulog(wulogProxy, wulogError, "Manager::run: Manager thread got errno %d from select call\n", errno);
      break;
    }
    try {
      wulog(wulogProxy, wulogVerb, "Manager::run: got msg\n");
      wunet::inetSock *client;
      if (FD_ISSET(lsock_.sd(), &rxSet)) {
        // If client is NULL then no messages 
        if ((client = lsock_.accept()) && client->isOpen()) {
          client->nodelay(1);
          WULOG((wulogProxy, wulogVerb, "Manager::run: Client has Connected ...\n"));
          // mkSession creates a thread and binds it to the new session object
          // the new thread starts in initSession()
          mkSession(client);
        }
      } else if (FD_ISSET(csock_.sd(), &rxSet)) {
        if ((client = csock_.accept()) != NULL) {
          client->nodelay(1);
          wulog(wulogProxy, wulogVerb,
                "Manager::run: Proxy got control command on socket %s\n", (csock_.toString()).c_str());
          doControl(client);
          delete client;
        }
      } else {
        std::vector<sRec>::iterator sit;
        for (sit = svec_.begin() ; sit != svec_.end() ; ++ sit) {
          if (FD_ISSET(sit->rcfd, &rxSet)) {
            wulog(wulogProxy, wulogVerb, "Manager::run: Got message from thread %d\n", sit->sobj->cid());
            recvFromSess(&(*sit));
          }
        }
      }
    } catch (wunet::badSock& e) {
      wulog(wulogProxy, wulogError, "Manager::run: Caught a badSock exception:\n\t%s",
            e.toString().c_str());
    } catch (wunet::netErr& e) {
      wulog(wulogProxy, wulogError, "Manager::run: Caught a netErr exception:\n\t%s",
            e.toString().c_str());
    } catch (...) {
      wulog(wulogProxy, wulogError, "Manager::run: Caught a Unknown exception.\n");
    }
  } // while loop

run_cleanup:
  wulog(wulogProxy, wulogInfo, "Manager::run: main thread exiting\n");
  shutdown();
}

void Manager::do_null(wuctl::sessionCmd::arg_type arg, sRec *rec)
{
  if (arg != wuctl::sessionCmd::nullArg) {
    wulog(wulogProxy, wulogWarn,
          "Manager::do_null: Ignoring arg\n");
  }
  if (rec == NULL) {
    wulog(wulogProxy, wulogWarn,
          "Manager::do_null: rec is null.. something odd is happening\n");
  }
  wulog(wulogProxy, wulogLoud, "Manager::do_null: Doing nothing\n");
}

void Manager::do_master(wuctl::sessionCmd::arg_type arg, sRec *rec)
{
  if (arg == wuctl::sessionCmd::nullArg) {
    wulog(wulogProxy, wulogError,
          "Manager::do_state: Arg is null! Ignoring command\n");
    return;
  }
  if (rec == NULL) {
    wulog(wulogProxy, wulogWarn,
          "Manager::do_state: record is null! Ignoring command\n");
    return;
  }

  wulog(wulogProxy, wulogVerb, "do_master: session %d wants to be slave to %d\n",
        rec->sobj->cid(), arg);

  sRec *master = getSession(arg);
  if(master == NULL) {
    wuctl::sessionCmd rmsg(wuctl::sessionCmd::cmdMasterResp, wuctl::sessionCmd::masterNotExist);
    if (rmsg.writeCmd(rec->wcfd) < 0) {
        wulog(wulogProxy, wulogError,
              "Manager::do_master: Error sending response to session %d\n",
              rec->sobj->cid());
    }
    return;
  }
 
  wuctl::sessionCmd mmsg(wuctl::sessionCmd::cmdSlave, rec->sobj->cid());
  if (mmsg.writeCmd(master->wcfd) < 0) {
    wulog(wulogProxy, wulogError, "Manager::do_master: Error sending message to session %d\n",
              master->sobj->cid());
    wuctl::sessionCmd rmsg(wuctl::sessionCmd::cmdMasterResp, wuctl::sessionCmd::masterError);
    if (rmsg.writeCmd(rec->wcfd) < 0) {
        wulog(wulogProxy, wulogError,
              "Manager::do_master: Error sending response to session %d\n",
              rec->sobj->cid());
    }
    return;
  }

  wuctl::sessionCmd mrmsg;
  if (mrmsg.readCmd(master->rcfd) < 0) {
    wulog(wulogProxy, wulogError, "Manager::do_master: Error reading response from %d\n", master->rcfd);
    wuctl::sessionCmd rmsg(wuctl::sessionCmd::cmdMasterResp, wuctl::sessionCmd::masterError);
    if (rmsg.writeCmd(rec->wcfd) < 0) {
        wulog(wulogProxy, wulogError,
              "Manager::do_master: Error sending response to session %d\n",
              rec->sobj->cid());
    }
    return;
  }

  wulog(wulogProxy, wulogVerb, "do_master: session %d sends result %d\n",
        master->sobj->cid(), mrmsg.arg());

  wuctl::sessionCmd::arg_type res = wuctl::sessionCmd::masterError;
  if (mrmsg.arg() == wuctl::sessionCmd::slaveOK) {
    res = wuctl::sessionCmd::masterOK;
    //CGW remove res from session table
  }
  else if (mrmsg.arg() == wuctl::sessionCmd::slaveReject) {
    res = wuctl::sessionCmd::masterReject;
  }

  wuctl::sessionCmd rmsg(wuctl::sessionCmd::cmdMasterResp, res);
  if (rmsg.writeCmd(rec->wcfd) < 0) {
      wulog(wulogProxy, wulogError,
            "Manager::do_master: Error sending response to session %d\n",
            rec->sobj->cid());
  }
}

/*
void Manager::do_slaveresp(wuctl::sessionCmd::arg_type arg, sRec *rec)
{
  if (arg == wuctl::sessionCmd::nullArg) {
    wulog(wulogProxy, wulogError,
          "Manager::do_slaveresp: Arg is null! Ignoring command\n");
    return;
  }
  if (rec == NULL) {
    wulog(wulogProxy, wulogWarn,
          "Manager::do_slaveresp: record is null! Ignoring command\n");
    return;
  }

  wulog(wulogProxy, wulogVerb, "do_slaveresp: session %d sends result %d\n",
        rec->sobj->cid(), arg);

  wuctl::sessionCmd arg_type res = wuctl::sessionCmd::masterError;
  if (arg == wuctl::slaveOK) {
    res = wuctl::sessionCmd::masterOK;
  }
  else if (arg == wuctl::slaveReject) {
    res = wuctl::sessionCmd::masterReject;
  }

  wuctl::sessionCmd rmsg(wuctl::sessionCmd::cmdMasterResp, res);
  if (rmsg.writeCmd(rec->wcfd) < 0) {
      wulog(wulogProxy, wulogError,
            "Manager::do_master: Error sending response to session %d\n",
            rec->sobj->cid());
  }

}
*/

void Manager::recvFromSess(sRec *rec)
{
  if(rec->sobj == NULL) {
    wulog(wulogProxy, wulogWarn, "Manager::recvFromSess: sobj is null\n");
    return;
  }

  wulog(wulogProxy, wulogVerb, "Manager::recvFromSess: got msg from %d\n",rec->sobj->cid());

  try {
    wuctl::sessionCmd cmsg;
    if (cmsg.readCmd(rec->rcfd) < 0) {
      wulog(wulogProxy, wulogError,
            "Manager::recvFromSess: Error reading control pipe %d\n", rec->rcfd);

      wuctl::sessionCmd rmsg(wuctl::sessionCmd::cmdShutdown, wuctl::sessionCmd::nullArg);
      if (rmsg.writeCmd(rec->wcfd) < 0) {
        wulog(wulogProxy, wulogError,
              "Manager::recvFromSess: Error sending command to session %d, doing a kill\n",
              rec->sobj->cid());
        pthread_kill(rec->sobj->tid(), SIGUSR1); // tells the thread to kill the session and exit
      }
      return;
    }

    if(cmsg.cmd() >= wuctl::sessionCmd::cmdCount || sessCmd[cmsg.cmd()] == NULL)
    {
      wulog(wulogProxy, wulogWarn, "Manager::recvFromSess: Invalid command %d\n",(int)cmsg.cmd());
      return;
    }

    (this->*sessCmd[cmsg.cmd()])(cmsg.arg(), rec);

    return;
  } catch (wupp::errorBase& e) {
    wulog(wulogProxy, wulogWarn,
          "Manager::recvFromSess: wupp Exception while processing command\n\tException = %s\n",
          e.toString().c_str());
    return;
  } catch (...) {
    wulog(wulogProxy, wulogWarn,
          "Manager::recvFromSess: Exception caught while processing command\n");
    return;
  }
}

void Manager::doControl(wunet::inetSock* client)
{
  wunet::msgBuf<wuctl::hdr_t> mbufRx;
  wunet::msgBuf<wuctl::hdr_t> mbufTx;
  try {
    if (wupp::isError(mbufRx.readmsg(client))) {
      wulog(wulogProxy, wulogError, "doControl: EOF reading control message\n");
      return;
    }
    mbufTx.hdr().reset();
    mbufTx.hdr().flags(wuctl::hdr_t::replyFlag);
    mbufTx.hdr().cmd(mbufRx.hdr().cmd());
    mbufTx.hdr().msgID(mbufRx.hdr().msgID());
    mbufTx.hdr().dlen(0);
    mbufTx.pclear();

    switch(mbufRx.hdr().cmd()) {
      case wuctl::hdr_t::getSessions:
        {
          wulog(wulogProxy, wulogVerb, "doControl: received getSessions command\n");
          std::vector<wuctl::connInfo*> bvec;
          std::vector<sRec>::const_iterator sit  = svec_.begin();

          bvec.clear();
          for (; sit != svec_.end() ; sit++)
            (*sit).sobj->mkConnList(bvec);

          if (bvec.size() > 0) {
            size_t len = wuctl::connInfo::dlen() * bvec.size();
            mbufTx.psize(len); // make sure there is enough space
            mbufTx.pclear();   // reset internal payload length to zero

            std::vector<wuctl::connInfo*>::const_iterator bit  = bvec.begin();
            std::vector<wuctl::connInfo*>::const_iterator bend = bvec.end();
            for (; bit != bend; bit++) {
              char buf[wuctl::connInfo::dlen()];
              (*bit)->copyOut((char *)buf);
              mbufTx.append((char *)buf, wuctl::connInfo::dlen());
            }

            // update payload size
            mbufTx.hdr().dlen(len);
          }
          //WULOG((wulogProxy, wulogLoud, "doControl: sending back\n%s\n", toString(bvec).c_str()));
        }
        break;
      case wuctl::hdr_t::remSession:
        if (mbufRx.plen() != sizeof(wucmd::conID_t)) {
          wulog(wulogProxy, wulogError, "Manager::docontrol: Incorrect payload size\n");
          mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
        } else {
          wucmd::conID_t sid = ntohs(*((wucmd::conID_t *)mbufRx.ppullup()));
          wulog(wulogProxy, wulogLoud, "doControl: received remSession command, sid = %d\n", int(sid));

          sRec *srec = getSession((int)sid);
          if (srec == NULL) {
            wulog(wulogProxy, wulogError, "Manager::docontrol: Invalid connection ID %d\n", sid);
            mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
          } else {
            wuctl::sessionCmd cmsg(wuctl::sessionCmd::cmdShutdown, wuctl::sessionCmd::nullArg);
            if (cmsg.writeCmd(srec->wcfd) < 0) {
              mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
              wulog(wulogProxy, wulogError, "Manager::docontrol: Error sending command to session %d, doing a kill\n",
                    srec->sobj->cid());
              pthread_kill(srec->sobj->tid(), SIGUSR1); // tells the thread to kill the session and exit
            }
          }
        }
        break;
      case wuctl::hdr_t::remConnection:
        if (mbufRx.plen() != sizeof(wucmd::conID_t)) {
          wulog(wulogProxy, wulogError, "Manager::docontrol: Incorrect payload size\n");
          mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
        } else {
          wucmd::conID_t cid = ntohs(*((wucmd::conID_t *)mbufRx.ppullup()));
          wulog(wulogProxy, wulogLoud, "doControl: received remConnection command, cid = %d\n", (int)cid);

          sRec *srec = getConnection((int)cid);
          if (srec == NULL) {
            wulog(wulogProxy, wulogError, "Manager::docontrol: Invalid connection ID %d\n", cid);
            mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
          } else {
            wuctl::sessionCmd cmsg(wuctl::sessionCmd::cmdCloseConn, cid);
            if (cmsg.writeCmd(srec->wcfd) < 0) {
              mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
              wulog(wulogProxy, wulogError, "Manager::docontrol: Error sending command close conn %d\n", cid);
              mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
            }
          }
        }
        break;
      case wuctl::hdr_t::setLogLevel:
        if (mbufRx.plen() != sizeof(int)) {
          wulog(wulogProxy, wulogError, "Manager::docontrol: Incorrect payload size\n");
          mbufTx.hdr().flags(wuctl::hdr_t::replyFlag | wuctl::hdr_t::errorFlag | wuctl::hdr_t::badParams);
        } else {
          int lvl = ntohl(*(int *)mbufRx.ppullup());
          wulog(wulogProxy, wulogLoud, "Manager::docontrol: Setting log level to %d\n", lvl);
          wulogSetDef((wulogLvl_t) lvl);
        }
        break;
      case wuctl::hdr_t::nullCommand:
        wulog(wulogProxy, wulogLoud, "Manager::docontrol: Got a NULL command\n");
        break;
      default:
        wulog(wulogProxy, wulogError,
              "Manager::docontrol: Unknown command %d\n",
              (int)mbufRx.hdr().cmd());
        break;
    }
    if (wupp::isError(mbufTx.writemsg(client)))
      wulog(wulogProxy, wulogWarn, "Manager::doControl: mbufTx.writemsg failed -- No exception!!\n");
  } catch (wupp::errorBase& e) {
    // XXX
    wulog(wulogProxy, wulogWarn, "Manager::doControl: Caught exception %s\n", e.toString().c_str());
  } catch (...) {
    wulog(wulogProxy, wulogWarn, "Manager::doControl: Unknown exception types\n");
    // XXX
    return;
  }
  return;
}

Manager::~Manager()
{
  if (open_)
    shutdown();
  pthread_mutex_destroy(&svecLock_);
  pthread_cond_destroy(&svecWait_);
}

void Manager::shutdown()
{
  // could be more graceful about this ... there are threads allocated for
  // each Session object. Deleting the object outright risks the thread waking
  // and attempting to use the now invalid session object.  I could either
  // cancel the threads or send a signal and let them cleanup their sessions.
  // This thread would then wait for all threads to complete before
  // continuing.  But for now I'll bury my head in the sand ... fK
  wulog(wulogProxy, wulogInfo, "Shutting down proxy manager object, closing all connections ...\n");
  // close control connections so no more session can be created.
  lsock_.close();
  csock_.close();

  autoLock slock(svecLock_);
  if (!slock)
    wulog(wulogProxy, wulogError, "Manager::shutdown: unable to lock svecLock!\n");

  std::vector<sRec>::iterator sit = svec_.begin();
  for (;sit != svec_.end(); sit++) {
    wulog(wulogProxy, wulogVerb,
          "Manager::shutdown: sending shutdown command to session %d\n",
          sit->sobj->cid());
    wuctl::sessionCmd cmsg(wuctl::sessionCmd::cmdShutdown, wuctl::sessionCmd::nullArg);
    if (cmsg.writeCmd(sit->wcfd) < 0) {
      wulog(wulogProxy, wulogError,
            "Manager::shutdown: Error sending command to session %d, doing a kill\n",
            sit->sobj->cid());
      pthread_kill(sit->sobj->tid(), SIGUSR1); // tells the thread to kill the session and exit
    }
  }

  // Now wait untill all threads have exited
  while(svec_.size() != 0) {
    struct timespec tout;
    struct timeval now;
    gettimeofday(&now, NULL);
    tout.tv_sec  = now.tv_sec + PROXY_TIMEOUT_SEC;
    tout.tv_nsec = now.tv_usec * 1000; // convert from msecs
    if (pthread_cond_timedwait(&svecWait_, &svecLock_, &tout) == ETIMEDOUT) {
      wulog(wulogProxy, wulogError, "Manager::shutdown: Timeout waiting for all threads to exit!\n");
      break;
    }
  }
  // slock automatically unlocked on method exit

  open_ = false;
}

void *initSession(void *);
struct initArgs {
  Proxy::Manager::sRec srec;
  Proxy::Manager *manager;
};
void *initSession(void *x)
{
  initArgs *args = (initArgs *)x;

  if (args->srec.sobj == NULL || !(*args->srec.sobj)) {
    wulog(wulogProxy, wulogError, "initSession: Passed an invalid session object!");
    if (args->srec.sobj)
      delete args->srec.sobj;
    delete args;

    return NULL;
  } else if (args->manager == NULL) {
    wulog(wulogProxy, wulogError, "initSession: Passed an invalid manager pointer!");
    if (args->srec.sobj)
      delete args->srec.sobj;
    delete args;

    return NULL;
  }
  wulog(wulogProxy, wulogInfo,
        "initSession: Created session and thread,\n\t%s\n",
        (args->srec.sobj->sock()->toString()).c_str());

  try {
    args->manager->addSession(args->srec);
    args->srec.sobj->run();
    args->srec.sobj->close();
  } catch (...) {
    wulog(wulogProxy, wulogError,
         "initSession: Was thrown out of args->session->run by an exception, not good! %s\n",
         (args->srec.sobj->sock()->toString()).c_str());
  }
  // must not throw an exception
  args->manager->remSession(args->srec);

  if(args->srec.sobj->notSlave()) {
    delete args->srec.sobj;
    // need to think about this.. srec.sobj can't be deleted if the session is now slave
    //   but, args should be deleted regardless because no one else is going to clean it up
    delete args;
  }

  return NULL;
}

void Manager::mkSession(wunet::inetSock *client)
{
  int r;
  pthread_attr_t tattr;
  wucmd::Session  *sobj = NULL;
  initArgs *args = NULL;

  // First make control channel (pipe)
  int toSessPipe[2], fromSessPipe[2];
  if (pipe(toSessPipe) < 0) {
    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Unable to create control pipe");
  }
  if (pipe(fromSessPipe) < 0) {
    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Unable to create control pipe");
  }
  // now create a thread to manage this sesssion
  try {
    sobj = new wucmd::Session(client, toSessPipe[0], fromSessPipe[1], this);
    args = new initArgs;
    args->srec.sobj = sobj;
    args->srec.wcfd = toSessPipe[1];
    args->srec.rcfd = fromSessPipe[0];
    args->manager   = this;
    wulog(wulogProxy, wulogLoud, "mkSession: wcfd = %d, rcfd = %d\n", args->srec.wcfd, args->srec.rcfd);

  } catch (...) {
    if (sobj)
      delete sobj;
    if (args)
      delete args;
    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Unable to allocate new Session object");
  }
  if (! (*sobj) ) {
    delete sobj;
    delete args;
    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Error initializing new Session object");
  }

  if ((r = pthread_attr_init(&tattr)) != 0) {
    delete sobj;
    delete args;
    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Unable to allocate pthread_attr", r);
  }
  if ((r = pthread_attr_setdetachstate (&tattr, PTHREAD_CREATE_DETACHED)) != 0) {
    delete sobj;
    delete args;
    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Unable to set pthread_attr", r);
  }
  if ((r = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM)) != 0) {
    delete sobj;
    delete args;
    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Unable to set pthread_attr", r);
  }

  FD_SET(args->srec.rcfd, &rset_);
  maxsd_ = std::max(maxsd_ -1, args->srec.rcfd) + 1;
  wulog(wulogProxy, wulogLoud, "Manager::mkSession: Added fd %d to rset, new max is %d\n", args->srec.rcfd, maxsd_);

  if (pthread_create(&sobj->tid(),  &tattr, initSession, (void *)args) != 0) {
    delete sobj;
    delete args;

    FD_CLR(args->srec.rcfd, &rset_);
    if ((args->srec.rcfd +1) == maxsd_) {
      resetMaxsd();
    }
    ::close(args->srec.rcfd);
    ::close(args->srec.wcfd);

    throw wunet::netErr(wupp::resourceErr, "Manager::mkSession: Error calling pthread_create", r);
  }
  pthread_attr_destroy(&tattr);
}

Manager::sRec* Manager::getSession(int sid)
{
  std::vector<sRec>::iterator sit = svec_.begin();
  for (;sit != svec_.end(); sit++) {
    if (sit->sobj->cid() == sid)
      return &(*sit);
  }
  return NULL;
}

wucmd::Session* Manager::getSessionObj(int sid)
{
  std::vector<sRec>::iterator sit = svec_.begin();
  for (;sit != svec_.end(); sit++) {
    if (sit->sobj->cid() == sid)
      return sit->sobj;
  }
  return NULL;
}

Manager::sRec* Manager::getConnection(int cid)
{
  std::vector<sRec>::iterator sit = svec_.begin();
  for (;sit != svec_.end(); ++sit) {
    if (sit->sobj->getConn(cid) != NULL)
      return &(*sit);
  }
  return NULL;
}

void Manager::addSession(sRec &srec)
{
  if (srec.sobj == NULL) return;

  autoLock slock(svecLock_);
  if (!slock)
    wulog(wulogProxy, wulogError, "Manager::addSession: unable to lock svecLock!\n");

  svec_.push_back(srec);

  wulog(wulogProxy, wulogInfo, "Manager::addSession: Added session %d\n", srec.sobj->cid());
}

void Manager::remSession(sRec &srec)
{
  if (srec.sobj == NULL) return;

  autoLock slock(svecLock_);
  if (!slock)
    wulog(wulogProxy, wulogError, "Manager::remSession: unable to lock svecLock!\n");


  std::vector<sRec>::iterator sit = svec_.begin();
  for (; sit != svec_.end() ; ++ sit) {
    if (sit->sobj == srec.sobj)
      break;
  }
  if (sit == svec_.end())
    wulog(wulogProxy, wulogError, "Manager::remSession: can\'t locate session object in list of sessions!\n");
  else
    svec_.erase(sit);

  FD_CLR(sit->rcfd, &rset_);
  if ((sit->rcfd +1) == maxsd_) {
    resetMaxsd();
  }
  ::close(sit->rcfd);
  ::close(sit->wcfd);

  if (svec_.size() == 0)
    pthread_cond_signal(&svecWait_);

  wulog(wulogProxy, wulogInfo, "Manager::remSession: Removed session %d\n", srec.sobj->cid());
}

void Manager::resetMaxsd()
{
  std::vector<sRec>::iterator sit = svec_.begin();
  maxsd_ = 0;

  for (; sit != svec_.end() ; ++ sit) {
    if (sit->rcfd >= maxsd_)
      maxsd_ = sit->rcfd + 1;
  }
  if(lsock_.sd() >= maxsd_)
    maxsd_ = lsock_.sd() + 1;
  if(csock_.sd() >= maxsd_)
    maxsd_ = csock_.sd() + 1;
}
