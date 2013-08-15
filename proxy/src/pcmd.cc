/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/pcmd.cc,v $
 * $Author: fredk $
 * $Date: 2007/03/23 21:21:54 $
 * $Revision: 1.8 $
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
#include <wulib/wulog.h>
#include <getopt.h>
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

void print_usage(string& prog) {
  cout << prog << " --addr serv --port dp -pr proto --lvl lvl\n";
}
//
// Signal handlers
void sigHandler(int signo)
{
  std::cout << "***** Got signal " << signo << "\n\n";
  return;
}

int
main(int argc, char **argv)
{
#define OPT_NOARG   0 /* no argument required */
#define OPT_REQARG  1 /* required argument */
#define OPT_OPTARG  2 /* argument is optional */
  static struct option longopts[] =
  {{"help",   OPT_NOARG,  NULL, 'h'},
   {"addr",     OPT_REQARG, NULL,  1},
   {"port",     OPT_REQARG, NULL,  2},
   {"pr",     OPT_REQARG, NULL,  5},
   {"lvl",    OPT_REQARG, NULL, 'l'},
   {NULL,             0,  NULL,  0}
  };
  // {name, has_arg, flag, val}

  string prog = argv[0];
  string servIP = "128.252.153.116"; // onl03

  in_port_t  servPort = 7071;
  int connProto = IPPROTO_TCP;

  wulogInit(proxyInfo, modCnt, wulogInfo, "Proxy", wulog_useLocal);

  /* Commands:
   *   pkts, bytes, time, reset
   */
  int c;
  while ((c = getopt_long(argc, argv, "hl:", longopts, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(prog);
        return 1;
        break;
      case 'l':
        {
          wulogLvl_t lvl = wulogName2Lvl(optarg);
          if (lvl == wulogInvLvl) {
            std::cerr << "Invalid wulog level specified: " << optarg << "\n";
            wulogPrintLvls();
            return 1;
          }
          wulogSetDef(lvl);
        }
      case 1:
        servIP = optarg;
        break;
      case 2:
        servPort = in_port_t(atoi(optarg));
        break;
      case 5:
        struct protoent *pent;
        if (isalpha(*optarg))
          pent = getprotobyname(optarg);
        else
          pent = getprotobynumber(strtol(optarg, NULL, 0));
        if (pent == NULL) {
          cerr << "Invalid protocol " << optarg << "\n";
          return 1;
        }
        connProto = int(pent->p_proto);
        break;
      default:
        cerr << "Unknown option " << c << "\n";
        return 1;
        break;
    }
  }

  // Install signal handlers
  struct sigaction sa;
  sa.sa_handler = sigHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL) < 0) {
    wulog(wulogProxy, wulogFatal, "proxy: Unable to install sigpipe handler\n");
    return 1;
  }

  // do active connect
  wunet::inetSock* sock = NULL;
  try {
    sock  = new wunet::inetSock(wunet::inetAddr(IPPROTO_TCP)); // pick local addr
    sock->connect(wunet::inetAddr(servIP.c_str(), servPort, connProto));
    sock->nodelay(1);
  } catch (wupp::errorBase& e) {
    cerr << "***** Caught a badSock exception" << endl;
    cerr << e << endl;;
    return 1;
  } catch (...) {
    cerr << "***** Caught some unknown exception" << endl;
    return 1;
  }

  // cout << "\nConnected to server ...\n\n" << *sock << endl;
  wunet::msgBuf<wuctl::hdr_t> mbuf;

  try {
    wuctl::hdr_t& hdr = mbuf.hdr();

    hdr.reset();
    mbuf.plen(0);

    hdr.cmd(wuctl::hdr_t::getSessions);
    hdr.dlen(0);
    hdr.msgID(1);
    hdr.flags(0);

    //cout << "\n\nSending command: " << mbuf << "\n ... \n";
    if (wupp::isError(mbuf.writemsg(sock))) {
      wulog(wulogApp, wulogWarn, "Error sending message -- no exception thrown!!\n");
      return 1;
    }

    mbuf.preset();
    if (wupp::isError(mbuf.readmsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected EOF reading message\n");
      return 1;
    }

    int dlen = hdr.dlen();
    int n = dlen / wuctl::connInfo::dlen();
    char *data = (char *)mbuf.ppullup();
    wuctl::connInfo cinfo;
    if (hdr.cmdSucceeded()) {
      if (hdr.dlen() == 0) {
        cout << "No active sessions found\n";
      } else {
        cout << "List of active sessions: \n";
        for (int i = 0; i < n; i++) {
          cinfo.copyIn(data);
          if (cinfo.flags() & wuctl::connInfo::flagSession)
            cout << "Session:\n";
          cout << "\t" << cinfo.toString() << "\n\n";
          data += wuctl::connInfo::dlen();
        }
      }
    } else {
      cout << "Command failed\n";
    } // command reply status

  } catch (wupp::errorBase& e) {
    cerr << "***** Caught wupp exception -- read" << endl;
    cerr << e << endl;
  } catch (...) {
    cerr << "***** Caught some unknown exception -- read" << endl;
  }

  return(0);
}
