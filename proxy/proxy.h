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
 * $Revision: 1.8 $
 * $Name:  $
 *
 * File:         proxy.h
 * Created:      02/01/2005 03:35:53 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WU_PROXY_H
#define _WU_PROXY_H

#include <pthread.h>

extern const wulogMod_t wulogProxy;
extern const int modCnt;
extern const wulogInfo_t proxyInfo[];

namespace Proxy {

  class Manager {
    public:
      static Manager *getManager(const std::string& ip, in_port_t pn, int pr=IPPROTO_TCP);

      struct sRec {
        wucmd::Session *sobj;
        int             rcfd;
        int             wcfd;
      };

      void run();
      ~Manager();
      void shutdown();
      void doControl(wunet::inetSock* client);

      sRec* getSession(int);
      wucmd::Session* getSessionObj(int);
      sRec* getConnection(int);
      void addSession(sRec &);
      void remSession(sRec &);

      static void sigHandler(int);
      static void sigTerm(int);
      static void sigFail(int);

      bool operator!() {return ! isOpen();}
      bool isOpen()
      {
        return lsock_.isOpen() && csock_.isOpen() && open_;
      }

      enum Status {Running, Shutdown, Down};

    private:
      static Manager *manager;
      // status is chnaged to Running when Manager object is created.
      // Otherwise only the signal handlers or error handler will change the
      // state to shutdown so there is no race condition.
      static Status    status;
      // singleton so hide constructor
      Manager(const std::string& ip, in_port_t pn, int pr=IPPROTO_TCP);
      void mkSession(wunet::inetSock*);
      wunet::inetSock lsock_;
      wunet::inetSock csock_;
      int maxsd_;
      fd_set rset_;
      void recvFromSess(sRec*);
      void resetMaxsd();
      std::vector<sRec> svec_;
      pthread_mutex_t  svecLock_;
      pthread_cond_t   svecWait_;
      bool open_;

      // commands from session:
      void do_null(wuctl::sessionCmd::arg_type, sRec*);
      void do_slaveresp(wuctl::sessionCmd::arg_type, sRec*);
      void do_master(wuctl::sessionCmd::arg_type, sRec*);
      // array of session command handlers
      void (Manager::*sessCmd[wuctl::sessionCmd::cmdCount])(wuctl::sessionCmd::arg_type, sRec*);
  };

};

#endif
