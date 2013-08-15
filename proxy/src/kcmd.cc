/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/kcmd.cc,v $
 * $Author: fredk $
 * $Date: 2007/03/23 21:21:54 $
 * $Revision: 1.11 $
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
const string nullCmd("null");
const string remSess("srem");
const string remConn("crem");
const string List("list");
const string Level("lvl");
const wuctl::cmd_t defaultCmd = wuctl::hdr_t::nullCommand;
const string defaultCmdString = nullCmd;

void print_usage(string& prog)
{
  cout << prog << " [--id id] [--addr serv] [--port dp] [-pr pr] [--lvl lvl]\n";
  cout << "\t--id id : connection/session id\n"
       << "\t--addr serv : servers address, default = " << defaultServer << "\n"
       << "\t--port dp : port server is listening on, default = " << (int)defaultPort << "\n"
       << "\t--pr pr : protocol to use (only support TCP for now), default = " << defaultProtocol << "\n"
       << "\t--lvl lvl : debug level\n"
       << "\t--cmd null|srem|crem|list|lvl : [sc]rem = remove [sess|conn] id, list = list sessions, default = %d"
       << (int)defaultCmd << "\n";
}
//
// Signal handlers
void sigHandler(int signo)
{
  std::cout << "***** Got signal " << signo << "\n\n";
  return;
}

using namespace wuctl;

int
main(int argc, char **argv)
{
#define OPT_NOARG   0 /* no argument required */
#define OPT_REQARG  1 /* required argument */
#define OPT_OPTARG  2 /* argument is optional */
  static struct option longopts[] =
  {{"help", OPT_NOARG,  NULL, 'h'},
   {"addr", OPT_REQARG, NULL,  1},
   {"port", OPT_REQARG, NULL,  2},
   {"id",   OPT_REQARG, NULL,  3},
   {"pr",   OPT_REQARG, NULL,  5},
   {"lvl",  OPT_REQARG, NULL, 'l'},
   {"cmd",  OPT_REQARG, NULL, 'c'},
   {NULL,   0,          NULL,  0}
  };
  // {name, has_arg, flag, val}

  string        prog = argv[0];
  string      servIP = defaultServer;
  in_port_t servPort = defaultPort;
  int          proto = defaultProtocol;
  int             id = -1;
  cmd_t          cmd = defaultCmd;
  string      cmdStr = defaultCmdString;
  wulogLvl_t lvl = wulogInfo;

  wulogInit(proxyInfo, modCnt, lvl, "Proxy", wulog_useLocal);

  /* Commands:
   *   pkts, bytes, time, reset
   */
  int c;
  while ((c = getopt_long(argc, argv, "hl:c:", longopts, NULL)) != -1) {
    switch (c) {
      case 'c':
        {
          string str(optarg);
          if (str == remSess) {
            cmd = hdr_t::remSession;
            cmdStr = remSess;
          } else if (str == nullCmd) {
            cmd = hdr_t::nullCommand;
            cmdStr = nullCmd;
          } else if (str == remConn) {
            cmd = hdr_t::remConnection;
            cmdStr = remConn;
          } else if (str == List) {
            cmd = hdr_t::getSessions;
            cmdStr = List;
          } else if (str == Level) {
            cmd = hdr_t::setLogLevel;
            cmdStr = Level;
          } else {
            print_usage(prog);
            cerr << "Invalid command " << str << std::endl;
            return 1;
          }
        }
        break;
      case 'h':
        print_usage(prog);
        return 1;
        break;
      case 'l':
        {
          lvl = wulogName2Lvl(optarg);
          if (lvl == wulogInvLvl) {
            std::cerr << "Invalid wulog level specified: " << optarg << "\n";
            wulogPrintLvls();
            return 1;
          }
          wulogSetDef(lvl);
        }
        break;
      case 1:
        servIP = optarg;
        break;
      case 2:
        servPort = in_port_t(atoi(optarg));
        break;
      case 3:
        id = atoi(optarg);
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

  if ((cmd == hdr_t::remSession || cmd == hdr_t::remConnection) && id == -1) {
    cerr << "Must specify a valid ID with the remSession or remConnection commands\n";
    return 1;
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

  // set local address to the default values (os picks port and ip)
  wunet::inetAddr laddr(IPPROTO_TCP);
  // call connect with Address of server
  wunet::inetAddr saddr(servIP.c_str(), servPort, proto);

  // do active connect
  wunet::inetSock sock(laddr);
  try {
    // call connect with Address of server
    sock.connect(saddr);
    sock.nodelay(1);
  } catch (wupp::errorBase& e) {
    cerr << "***** Caught a wupp exception" << endl;
    cerr << e << endl;;
    return 1;
  } catch (...) {
    cerr << "***** Caught some unknown exception" << endl;
    return 1;
  }

  cout << "Connected to server ...\n\tSock:\n\t" << sock << endl;
  wunet::msgBuf<hdr_t> mbuf;

  try {
    hdr_t &hdr = mbuf.hdr();

    // set header fields
    hdr.reset();
    hdr.cmd(cmd);
    hdr.msgID(1);
    hdr.flags(0);

    switch (cmd) {
      case hdr_t::getSessions:
        hdr.dlen(0);
        mbuf.plen(0);
        break;
      case hdr_t::remSession:
        hdr.dlen(sizeof(uint16_t));
        mbuf.plen(sizeof(uint16_t));
        *(uint16_t *)mbuf.ppullup() = htons(uint16_t(id));
        break;
      case hdr_t::remConnection:
        hdr.dlen(sizeof(uint16_t));
        mbuf.plen(sizeof(uint16_t));
        *(uint16_t *)mbuf.ppullup() = htons(uint16_t(id));
        break;
      case hdr_t::setLogLevel:
        hdr.dlen(sizeof(int));
        mbuf.plen(sizeof(int));
        *(int *)mbuf.ppullup() = htonl((int)lvl);
        break;
      default:
        cerr << "Unknown command\n";
        return 1;
    }

    cout << "\n\nSending command msg:\n" << mbuf.toString() << endl;
    if (wupp::isError(mbuf.writemsg(&sock))) {
      wulog(wulogApp, wulogWarn,
            "Unexpected Error sending message -- no exception thrown!\n");
      return 1;
    }

    mbuf.preset();
    if (wupp::isError(mbuf.readmsg(&sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected EOF\n");
      return 1;
    }

    cout << "Reply Msg:\n" << mbuf.toString() << "\n" << endl;

    switch (cmd) {
      case hdr_t::getSessions:
        {
          size_t dlen = hdr.dlen();
          size_t cnt = dlen / wuctl::connInfo::dlen();
          char *data = (char *)mbuf.ppullup();
          connInfo cinfo;

          if (hdr.cmdSucceeded()) {
            if (cnt == 0) {
              cout << "No active sessions found\n";
            } else {
              cout << "List of active sessions: \n";
              for (size_t indx = 0; indx < cnt; ++indx) {
                cinfo.copyIn(data);
                if (cinfo.flags() & wuctl::connInfo::flagSession)
                  cout << "Session:\n";
                cout << "\t" << cinfo.toString() << "\n\n";
                data += wuctl::connInfo::dlen();
              }
            }
          } else {
            cout << "Command failed\n\tMsg: " << mbuf.toString() << endl;
          }
        }
        break;
      case hdr_t::remSession:
      case hdr_t::remConnection:
      case hdr_t::setLogLevel:
        break;
    }
  } catch (wupp::errorBase& e) {
    cerr << "***** Caught a wupp exception -- read" << endl;
    cerr << e << endl;
  } catch (...) {
    cerr << "***** Caught some unknown exception -- read" << endl;
  }

  return(0);
}
