/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/pingProxy.cc,v $
 * $Author: fredk $
 * $Date: 2006/12/08 20:35:00 $
 * $Revision: 1.2 $
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

using std::string;
using std::cout;
using std::cerr;
using std::endl;

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

const string defaultServer("128.252.153.116"); // onl03
const in_port_t  defaultPort = 7071;
const int defaultProtocol = IPPROTO_TCP;

void print_usage(string& prog)
{
  cout << prog << " [--id id] [--addr serv] [--port dp] [-pr pr] [--lvl lvl]\n"
       << "\t--addr serv : servers address, default = " << defaultServer << "\n"
       << "\t--port dp : port server is listening on, default = " << (int)defaultPort << "\n"
       << "\t--pr pr : protocol to use (only support TCP for now), default = " << defaultProtocol << "\n"
       << "\t--lvl lvl : debug level\n";
}

using namespace wuctl;

int
main(int argc, char **argv)
try {
#define OPT_NOARG   0 /* no argument required */
#define OPT_REQARG  1 /* required argument */
#define OPT_OPTARG  2 /* argument is optional */
  static struct option longopts[] =
  {{"help", OPT_NOARG,  NULL, 'h'},
   {"addr", OPT_REQARG, NULL,  1},
   {"port", OPT_REQARG, NULL,  2},
   {"pr",   OPT_REQARG, NULL,  5},
   {NULL,   0,          NULL,  0}
  };
  // {name, has_arg, flag, val}

  string        prog = argv[0];
  string      servIP = defaultServer;
  in_port_t servPort = defaultPort;
  int          proto = defaultProtocol;

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
        proto = int(pent->p_proto);
        break;
      default:
        cerr << "Unknown option " << c << "\n";
        return 1;
        break;
    }
  }

  // set local address to the default values (os picks port and ip)
  wunet::inetAddr laddr(IPPROTO_TCP);
  // call connect with Address of server
  wunet::inetAddr saddr(servIP.c_str(), servPort, proto);

  // do active connect
  wunet::inetSock sock(laddr);
  
  // call connect with Address of server
  sock.connect(saddr);
  sock.nodelay(1);

  wunet::msgBuf<hdr_t> mbuf;
  hdr_t &hdr = mbuf.hdr();

  // set header fields
  hdr.reset();
  hdr.cmd(wuctl::hdr_t::nullCommand);
  hdr.msgID(1);
  hdr.flags(0);

  hdr.dlen(0);
  mbuf.plen(0);

  if (wupp::isError(mbuf.writemsg(&sock))) {
    wulog(wulogApp, wulogWarn,
          "Unexpected Error sending message -- no exception thrown!\n");
    return 1;
  }

  mbuf.reset();
  if (wupp::isError(mbuf.readmsg(&sock))) {
    wulog(wulogApp, wulogWarn, "Unexpected EOF\n");
    return 1;
  }

  return ((hdr.cmdSucceeded()) ? 0 : 1);

} catch (wupp::errorBase& e) {
  cerr << "***** Caught wupp exception -- read" << endl;
  cerr << e << endl;
  return 1;
} catch (...) {
  cerr << "***** Caught some unknown exception -- read" << endl;
  return 1;
}
