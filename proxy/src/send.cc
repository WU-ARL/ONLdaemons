/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/send.cc,v $
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
#include "session.h"
#include "proxy.h"

void print_usage(string& prog) {
  cout << prog << " --da1 serv1 --dp1 dp1 --da2 da2 --dp2 dp2 --pr proto --cnt n\n";
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
   {"da1",     OPT_REQARG, NULL,  1},
   {"dp1",     OPT_REQARG, NULL,  2},
   {"da2",     OPT_REQARG, NULL,  3},
   {"dp2",     OPT_REQARG, NULL,  4},
   {"pr",     OPT_REQARG, NULL,  5},
   {"lvl",    OPT_REQARG, NULL, 'l'},
   {"cnt",    OPT_REQARG, NULL, 'c'},
   {NULL,             0,  NULL,  0}
  };
  // {name, has_arg, flag, val}

  string prog = argv[0];
  string servIP[2] = {"127.0.0.1", "127.0.0.1"};
  in_port_t  servPort[2] = {6060, 7070};
  int connProto = IPPROTO_TCP;
  int cnt = 1;

  wulogInit(proxyInfo, modCnt, wulogVerb, "Proxy", wulog_useLocal);

  /* Commands:
   *   pkts, bytes, time, reset
   */
  int c;
  while ((c = getopt_long(argc, argv, "c:hl:", longopts, NULL)) != -1) {
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
        servIP[0] = optarg;
        break;
      case 2:
        servPort[0] = in_port_t(atoi(optarg));
        break;
      case 3:
        servIP[1] = optarg;
        break;
      case 4:
        servPort[1] = in_port_t(atoi(optarg));
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
      case 'c':
        if ((cnt = atoi(optarg)) < 0) {
          cout << "Invalid count\n";
          print_usage(prog);
          return 1;
        }
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

  cout << "Opening conn to host " << servIP[1] << " port " << servPort[1]
       << "via host " << servIP[0] << " and port " << servPort[0]
       << "\n";


  // do active connect
  wunet::inetSock* sock = NULL;
  try {
    sock  = new wunet::inetSock(wunet::inetAddr(IPPROTO_TCP)); // pick local addr
    sock->connect(wunet::inetAddr(servIP[0].c_str(), servPort[0], connProto));
    sock->nodelay(1);
    cout << "Connected to server: " << *sock << endl;
  } catch (wupp::errorBase &e) {
    cerr << "***** Caught wupp exception" << endl;
    cerr << e << endl;
    return 1;
  } catch (...) {
    cerr << "***** Caught some unknown exception" << endl;
    return 1;
  }

  cout << "Connected to server ...\n" << *sock << endl;
  wunet::msgBuf<wucmd::hdr_t> mbuf1;

  try {
    wucmd::hdr_t& hdr1 = mbuf1.hdr();

    //  -------------------   Open Connection ----------------
    hdr1.reset();
    mbuf1.preset();
    hdr1.cmd(wucmd::openConn);
    hdr1.conID(0);
    hdr1.msgID(1);

    // resolve IP address info.
    wunet::inetAddr target(servIP[1].c_str(), servPort[1], connProto);
    if (target.host().empty()) {
      wulog(wulogApp, wulogError, "Unable to map address arg %s to a hostname\n", servIP[1].c_str());
      return 1;
    }

    connData cd(target.host().c_str(), target.port(), target.proto);
    // Need to set the data length
    hdr1.dlen(cd.dlen);
    // Need to set the payload length
    mbuf1.plen(hdr1.dlen());
    // now copy into message payload
    char *msgData = (wucmd::connData *)mbuf1.ppullup();
    if (msgData == NULL) {
      wulog(wulogApp, wulogWarn, "Pullup failed!!\n");
      return 1;
    }
    if (cd.copyOut(msgData) == NULL) {
      wulog(wulogApp, wulogError, "Error copying connData into message body\n");
      return 1;
    }

    cout << "\n\nSending Open: " << mbuf1 << "\n ... \n";
    if (wupp::isError(mbuf1.writemsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected error sending message -- No Exception!!\n");
      return 1;
    }
    if (mbuf1.readmsg(sock) != wupp::noError) {
      wulog(wulogApp, wulogWarn, "Unexpected EOF reading message\n");
      return 1;
    }
    cout << "Got answer: " << mbuf1 << "\n\n";

    wucmd::conID_t cid = hdr1.conID();

    // ------------------   Send Echo Command ------------------------
    hdr1.reset();
    mbuf1.preset();

    hdr1.cmd(wucmd::cmdEcho);
    hdr1.conID(0);
    hdr1.msgID(2);
    char *data = (char *)mbuf1.ppullup();
    strcpy(data, "Data: Payload is a string");
    hdr1.dlen(strlen(data)+1);
    // before calling append plen = 0, after it is the lenght of the falue of
    // hdr1.dlen()
    mbuf1.append(data, hdr1.dlen());

    cout << "\n\nSending Echo: " << mbuf1 << "\n";
    if (wupp::isError(mbuf1.writemsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected error sending message -- No Exception\n");
      return 1;
    }
    if (wupp::isError(mbuf1.readmsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected EOF reading message\n");
      return 1;
    }
    cout << "Got answer " << mbuf1 << "\n";

    // ------------------ Get Connection (cid) Status --------------------
    hdr1.reset();
    mbuf1.preset();
    hdr1.cmd(wucmd::getConnStatus);
    hdr1.conID(cid);
    hdr1.msgID(3);
    hdr1.dlen(0);

    cout << "\n\nSending getStatus: " << mbuf1 << "\n";
    if (wupp::isError(mbuf1.writemsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected error sending message - No Exception!!\n");
      return 1;
    }
    if (wupp::isError(mbuf1.readmsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected ROF reading message\n");
      return 1;
    }
    cout << "Got answer " << mbuf1 << "\n";

    // --------------- Forward data over this new connection -----------
#define dLen 1024;
    uint8_t *dbuf[dLen];
    char *msg = (char *)dbuf;
    int   msgLen  = dLen;
    int   msgHlen = sizeof(int);
    int   msgPlen = msgLen - msgHlen;
    int  *msgHdr  = (int *)dbuf;

    *msgHdr = msgPlen;
    for (int i = msgHlen; i < msgLen; ++i)
      dbuf[i] = (uint8_t)i;


    hdr1.reset();
    hdr1.cmd(wucmd::cmdFwd);
    hdr1.conID(cid);
    hdr1.dlen(msgLen);

    mbuf1.plen(msgLen);
    mbuf1.copyin(pkt, msgLen);

    wunet::msgBuf<wucmd::hdr_t> mbuf2;
    wucmd::hdr_t& hdr2 = mbuf2.hdr();
    for (int i = 0; i < cnt; ++i) {
      hdr1.msgID(i+10);
      if (wupp::isError(mbuf1.writemsg(sock))) {
        wulog(wulogApp, wulogWarn, "Unexpected error sending message -- No Exception\n");
        return 1;
      }
      if (wupp::isError(mbuf2.readmsg(sock))) {
        wulog(wulogApp, wulogWarn, "Unexpected EOF reading message\n");
        return 1;
      }
      if (hdr2.dlen() != msgLen) {
        wulog(wulogApp, wulogError,
              "Error: returned message size incorrect. Wanted %u, got %u!\n", 
              msgLen, hdr2.dlen());
      } else {
        uint8_t *ptr = ppullup();
        for (int j = 0; j < msgLen; ++j) {
          if (ptr[j] != msg[j])
            wulog(wulogApp, wulogError, "ptr[%u] (%c) != msg[%u] (%hhu)\n", j, ptr[j], j, msg[j]);
        }
      }
    }
    // ------------------- Close Connection -----------------------------
    hdr1.reset();
    mbuf1.preset();
    hdr1.cmd(wucmd::closeConn);
    hdr1.conID(cid);
    hdr1.msgID(100);
    hdr1.dlen(0);

    cout << "\n\nSending closeConn: " << mbuf1 << "\n";
    if (wupp::isError(mbuf1.writemsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected error sending message -- No Exception\n");
    }
    cout << "Reading reply ...\n\n";
    if (wupp::isError(mbuf1.readmsg(sock))) {
      wulog(wulogApp, wulogWarn, "Unexpected EOF reading message\n");
    }
    cout << "Got answer " << mbuf1 << "\n";

    // -- Now close our end -----------
    cout << "Done closing conn\n";
    sock->close();
    cout << "Connection closed\n";

  } catch (wupp::errorBase& e) {
    cerr << "***** Caught wupp exception -- read" << endl;
    cerr << e << endl;
  } catch (...) {
    cerr << "***** Caught some unknown exception -- read" << endl;
  }

  return(0);
}
