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
 * $Revision: 1.9 $
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
#include <iostream>
#include <sstream>
#include <vector>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>
#include <wulib/wulog.h>
#include <signal.h>

using namespace std;

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

const int default_port = 7070;

void print_usage(char *prog)
{
  std::cout << prog << " --help\n";
  std::cout << prog << " [-d] [--addr ip/host] [--port pn] [--proto pr] -l lvl\n";
  std::cout << "\t-d: run as a damon, log messages set to syslogd (local0)\n"
            << "\t-l lvl: Default logging level\n"
            << "\t--addr ip/host: default 0.0.0.0 (any interface)\n"
            << "\t--port pn: default is " << default_port << "\n"
            << "\t--proto pr: tcp is the only porotocol currently supported\n\n";
}

int
main(int argc, char **argv)
{
  std::string servIP = "0.0.0.0";
  in_port_t servPort = default_port;
  int connProto = IPPROTO_TCP;
  char *prog = argv[0];
  wulogFlags_t logFlags = wulog_useLocal | wulog_useTStamp;
  wulogLvl_t     logLvl = wulogInfo;
  int mode = 0;

#define OPT_NOARG   0 /* no argument required */
#define OPT_REQARG  1 /* required argument */
#define OPT_OPTARG  2 /* argument is optional */
  static struct option longopts[] =
  {{"help",   OPT_NOARG,  NULL, 'h'},
   {"addr",   OPT_REQARG, NULL,  1},
   {"port",   OPT_REQARG, NULL,  2},
   {"proto",  OPT_REQARG, NULL,  3},
   {"lvl",    OPT_REQARG, NULL, 'l'},
   {NULL,             0,  NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "hl:d", longopts, NULL)) != -1) {
    switch (c) {
      case 'd':
        logFlags = wulog_useSyslog;
        mode = 1;
        break;
      case 'h':
        print_usage(prog);
        return 1;
        break;
      case 'l':
        logLvl = wulogName2Lvl(optarg);
        if (logLvl == wulogInvLvl) {
          std::cerr << "Invalid wulog level specified: " << optarg << "\n";
          wulogPrintLvls();
          return 1;
        }
        break;
      case 1:
        servIP = optarg;
        break;
      case 2:
        servPort = in_port_t(atoi(optarg));
        break;
      case 3:
        struct protoent *pent;
        if (isalpha(*optarg))
          pent = getprotobyname(optarg);
        else
          pent = getprotobynumber(strtol(optarg, NULL, 0));
        if (pent == NULL) {
          std::cerr << "Invalid protocol " << optarg << "\n";
          return 1;
        }
        connProto = int(pent->p_proto);
        break;
      default:
        std::cerr << "Unknown option " << c << "\n";
        return 1;
        break;
    }
  }

  // {name, has_arg, flag, val}
  wulogInit(proxyInfo, modCnt, logLvl, "Proxy", logFlags);
  wulogSetLvl(wulogProxy, logLvl);

  if (mode)
    daemon(0, 0);

  wulog(wulogApp, wulogInfo, "Calling proxy manager (%s, %d, %d)\n", servIP.c_str(), (int)servPort, connProto);
  Proxy::Manager *proxy = Proxy::Manager::getManager(servIP, servPort, connProto);

  proxy->run();

  return(0);
}
