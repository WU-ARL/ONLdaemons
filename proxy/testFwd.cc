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
 * $Revision: 1.16 $
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
  cout << prog << " --paddr proxy --pport pn --saddr server --sport pn --pr proto --cnt n --cpy n --lvl lvl\n";
}
//
// Signal handlers
void sigHandler(int signo)
{
  std::cout << "***** Got signal " << signo << "\n\n";
  return;
}

using namespace wunet;

union ctlArgs {
  wucmd::conID_t cid; // connection/session id
  int            lvl; // wulog level
};

void printSessions(char *data, size_t cnt);
int doControl(wunet::inetSock *sock, wuctl::cmd_t cmd, ctlArgs arg);
int doControl(wunet::inetSock *sock, wuctl::cmd_t cmd, ctlArgs arg)
try {
  static wuctl::msgID_t mid = 0;

  wunet::msgBuf<wuctl::hdr_t> mbuf;
  wuctl::hdr_t &hdr = mbuf.hdr();

  hdr.reset();
  hdr.cmd(cmd);
  hdr.msgID(mid++);
  hdr.flags(0);

  // set hdr.dlen(???) and mbuf.plen();
  switch (cmd) {
    case wuctl::hdr_t::getSessions:
      hdr.dlen(0); 
      mbuf.plen(0);
      break;
    case wuctl::hdr_t::remSession:
      hdr.dlen(sizeof(wucmd::conID_t));
      mbuf.plen(sizeof(wucmd::conID_t));
      *((wucmd::conID_t *)mbuf.ppullup()) = htons(arg.cid);
      break;               
    case wuctl::hdr_t::remConnection:
      hdr.dlen(sizeof(wucmd::conID_t));
      mbuf.plen(sizeof(wucmd::conID_t));
      *((wucmd::conID_t *)mbuf.ppullup()) = htons(arg.cid);
      break;     
    case wuctl::hdr_t::setLogLevel:
      hdr.dlen(sizeof(int));
      mbuf.plen(sizeof(int));
      *(int *)mbuf.ppullup() = htonl((int)arg.lvl);
      break; 
    default: 
      wulog(wulogApp, wulogError, "doControl: Unrecognized control command %d\n", (int)cmd);
      return -1;
  }

  wulog(wulogApp, wulogVerb, "doControl: Sending command msg:\n%s\n",
        mbuf.toString().c_str());
  wupp::errType et = mbuf.writemsg(sock);
  if (wupp::isError(et)) {
    wulog(wulogApp, wulogWarn,
          "doControl: Unexpected Error sending message -- no exception thrown\n");
    return -1;
  }

  mbuf.preset();
  et = mbuf.readmsg(sock);
  if (wupp::isError(et)) {
    wulog(wulogApp, wulogError,
          "doControl: Unexpected Error eding response message -- no exception thrown\n");
    return -1;
  } else if (et == wupp::EndOfFile) {
    wulog(wulogApp, wulogWarn, "doControl: Unexpected end-of-file waiting for response message\n");
    return -1;
  }

  wulog(wulogApp, wulogVerb, "doControl: Reply Msg:\n%s\n",
        mbuf.toString().c_str());

  switch (cmd) {
    case wuctl::hdr_t::getSessions:
      {
        size_t cnt = hdr.dlen() / wuctl::connInfo::dlen();
        char *data = (char *)mbuf.ppullup();

        if (hdr.cmdSucceeded()) {
          wulog(wulogApp, wulogInfo, "doControl: getSessions command succeeded\n");
          printSessions(data, cnt);
        } else {
          wulog(wulogApp, wulogWarn, "doControl: Command failed\n\tMsg: %s\n", mbuf.toString().c_str());
        }
      }
      break;
    case wuctl::hdr_t::remSession:
      if (hdr.cmdSucceeded())
        wulog(wulogApp, wulogInfo, "doControl: remSession command succeeded for sid %d\n", (int)arg.cid);
      else
        wulog(wulogApp, wulogWarn, "doControl: remSession command failed for sid %d\n\tMsg: %s\n",
              (int)arg.cid, mbuf.toString().c_str());
    case wuctl::hdr_t::remConnection:
      if (hdr.cmdSucceeded())
        wulog(wulogApp, wulogInfo, "doControl: remConnection command succeeded for cid %d\n", (int)arg.cid);
      else
        wulog(wulogApp, wulogWarn, "doControl: remConnection command failed for cid %d\n\tMsg: %s\n",
              (int)arg.cid, mbuf.toString().c_str());
    case wuctl::hdr_t::setLogLevel:
      if (hdr.cmdSucceeded())
        wulog(wulogApp, wulogInfo, "doControl: setLogLevel command succeeded\n");
      else
        wulog(wulogApp, wulogWarn, "doControl: setLogLevel command failed\n\tMsg: %s\n", mbuf.toString().c_str());
      break;
  }
  return 0;
} catch (wupp::errorBase& e) {
  wulog(wulogApp, wulogError, "doControl: Caught wupp exception: e = \n\t%s\n",
          e.toString().c_str());
  return -1;
} catch (std::exception &e) {
  wulog(wulogApp, wulogError, "doControl: Caught STL exception: %s", e.what());
  return -1;
} catch (...) {
  wulog(wulogApp, wulogError, "doControl: Caught unknown exception\n");
  return -1;
}

void printSessions(char *data, size_t cnt)
{
  wuctl::connInfo cinfo;

  cout << "List of active sessions: \n";
  if (cnt == 0) {
    cout << "\tNo active sessions found\n";
    return;
  }

  for (size_t indx = 0; indx < cnt; ++indx) {
    cinfo.copyIn(data);
    if (cinfo.flags() & wuctl::connInfo::flagSession)
      cout << "\tSession:\n";
    cout << "\t\t" << cinfo.toString() << "\n\n";
    data += wuctl::connInfo::dlen();
  }

  return;
}

wucmd::conID_t openConn(wunet::inetSock *sock, const wunet::inetAddr &target);

wucmd::conID_t openConn(wunet::inetSock *sock, const wunet::inetAddr &target)
{
  wunet::msgBuf<wucmd::hdr_t> mbuf;
  wucmd::hdr_t& hdr = mbuf.hdr();
  hdr.reset();
  hdr.cmd(wucmd::openConn);
  hdr.conID(0);
  hdr.msgID(1);

  wucmd::connData cd(target.host().c_str(), target.port(), target.proto());
  hdr.dlen(cd.dlen());
  mbuf.plen(hdr.dlen());

  char *msgData = (char *)mbuf.ppullup();
  if (msgData == NULL) {
    wulog(wulogApp, wulogWarn, "openConn: Pullup failed!!\n");
    return 0;
  }
  if (cd.copyOut(msgData) == NULL) {
    wulog(wulogApp, wulogError, "openConn: Error copying connData into message body\n");
    return 0;
  }
  cout << "\n\nSending Open:\n" << mbuf << "\n ... \n";
  try {
    mbuf.writemsg(sock);
    if (mbuf.readmsg(sock) == wupp::EndOfFile) {
      wulog(wulogApp, wulogWarn, "openConn: Got EOF while waiting for reply!\n");
      return 0;
    }
    if (mbuf.hdr().cmd() != wucmd::cmdReply) {
      cout << "openConn: Comm open reply error, expecting cmdReply\n";
      return 0;
    } else {
      wucmd::replyData *rdata = (wucmd::replyData *)((char *)mbuf.ppullup() + mbuf.hdr().hlen());
      if (rdata->cmdStatus() == wucmd::cmdFailure) {
        cout << "openConn: Command failed\n";
        return 0;
      }
    }
  } catch (wupp::errorBase& e) {
    wulog(wulogApp, wulogError, "openConn: writemsg/readmsg threw exception: e = \n\t%s\n",
            e.toString().c_str());
    return 0;
  } catch (...) {
    wulog(wulogApp, wulogError, "openConn: writemsg/readmsg threw unknown exception");
    return 0;
  }

  cout << "openConn: Got answer:\n\t" << mbuf << "\n\n";

  return hdr.conID();
}

int doEcho(wunet::inetSock *sock, const char *data, size_t dlen)
{
  wunet::msgBuf<wucmd::hdr_t> mbuf;
  wucmd::hdr_t& hdr = mbuf.hdr();
  static wucmd::msgID_t mid = 0;

  hdr.reset();
  hdr.cmd(wucmd::cmdEcho);
  hdr.conID(0);
  hdr.msgID(mid++);

  mbuf.plen(dlen);
  mbuf.pcopyin(data, dlen);
  hdr.dlen(mbuf.plen());

  try {
    cout << "Writing:\n\t" << mbuf << "\n";
    wupp::errType et = mbuf.writemsg(sock);
    if (wupp::isError(et)) {
      wulog(wulogApp, wulogWarn, "Error sending message\n");
      return 1;
    }
    et = mbuf.readmsg(sock);
    if (wupp::isError(et)) {
      wulog(wulogApp, wulogWarn, "Error reading message\n");
      return 1;
    }
  } catch (wupp::errorBase& e) {
    wulog(wulogApp, wulogError, "writemsg/readmsg threw exception: e = \n\t%s\n",
                        e.toString().c_str());
    return 1;
  } catch (...) {
    wulog(wulogApp, wulogError, "writemsg/readmsg threw unknown exception");
    return 1;
  }

  cout << "Got answer:\nMsg:\n\t " << mbuf << "\n";

  if (mbuf.plen() != dlen) {
    wulog(wulogApp, wulogError, "doEcho: Bad return byte count\n");
    return -1;
  }

  char *ptr = (char *)mbuf.ppullup();
  for (size_t indx = 0; indx < dlen; ++indx) {
    if (data[indx] != ptr[indx]) {
      wulog(wulogApp, wulogError, "doEcho: Byte offset %d not equal\n", indx);
      return -1;
    }
  }

  return 0;
}

int
main(int argc, char **argv)
{
#define OPT_NOARG   0 /* no argument required */
#define OPT_REQARG  1 /* required argument */
#define OPT_OPTARG  2 /* argument is optional */
  static struct option longopts[] =
  {{"help",   OPT_NOARG,  NULL, 'h'},
    {"paddr", OPT_REQARG, NULL,  1},
    {"pport", OPT_REQARG, NULL,  2},
    {"saddr", OPT_REQARG, NULL,  3},
    {"sport", OPT_REQARG, NULL,  4},
    {"pr",    OPT_REQARG, NULL,  5},
    {"lvl",   OPT_REQARG, NULL, 'l'},
    {"cnt",   OPT_REQARG, NULL, 'c'},
    {"cpy",   OPT_REQARG, NULL, 'p'},
    {"mode",  OPT_REQARG, NULL, 'm'},
    {NULL,            0,  NULL,  0}
  };
  // {name, has_arg, flag, val}

  string prog = argv[0];
  string servIP[2] = {"127.0.0.1", "127.0.0.1"};
  in_port_t  servPort[2] = {7070, 6060};
  int connProto = IPPROTO_TCP;
  int cnt = 1, cpy = 1;
  bool byteSend = false;

  wulogInit(proxyInfo, modCnt, wulogVerb, "Proxy", wulog_useLocal);

  /* Commands:
   *   pkts, bytes, time, reset
   */
  int c;
  while ((c = getopt_long(argc, argv, "p:c:hl:m:", longopts, NULL)) != -1) {
    switch (c) {
      case 'm':
        if (string(optarg) == "byte")
          byteSend = true;
        else
          byteSend = false;
        break;
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
        break;
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
      case 'p':
        if ((cpy = atoi(optarg)) < 0) {
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
    << " via host " << servIP[0] << " and port " << servPort[0]
    << "\n";

  wunet::inetAddr laddr(IPPROTO_TCP);
  wunet::inetSock dataSock(laddr);
  wunet::inetSock ctlSock(laddr);

  if (!dataSock || !ctlSock) {
    cerr << "Error onening socket:\n" << dataSock << endl;
    return 1;
  }

  try {
    dataSock.connect(wunet::inetAddr(servIP[0].c_str(), servPort[0], connProto));
    dataSock.nodelay(1);
    ctlSock.connect(wunet::inetAddr(servIP[0].c_str(), servPort[0]+1, connProto));
    ctlSock.nodelay(1);
  } catch (wupp::errorBase& e) {
    cerr << "***** Caught exception while connecting to server" << endl;
    cerr << "\tdataSock:\n\t" << dataSock << endl;
    cerr << "\tctlSock:\n\t" << ctlSock << endl;
    cerr << e << endl;;
    return 1;
  } catch (...) {
    cerr << "***** Caught some exception while connecting to server" << endl;
    return 1;
  }

  // cout << "Connected to server ...\n\tSock:\n\t" << dataSock << endl;
  wunet::inetAddr target(servIP[1].c_str(), servPort[1], connProto);
  // wunet::inetAddr target2(servIP[1].c_str(), (servPort[1] + 1), connProto);
  wucmd::conID_t cid;
  if ((cid = openConn(&dataSock, target)) == 0) {
    cout << "Open failed, exiting ...\n";
    return 1;
  }
  // wucmd::conID_t cid2 = openConn(&dataSock, target2);

  char data[64];
  strcpy(data, "Data: Payload is a string, and I am done");

  // ------------------   Send Echo Command ------------------------
  doEcho(&dataSock, data, strlen(data)+1);

  // ------------------ Get Connection (cid) Status --------------------
  {
    ctlArgs arg;
    arg.cid = cid;
    if (doControl(&ctlSock, wuctl::hdr_t::getSessions, arg) < 0) {
      wulog(wulogApp, wulogWarn, "Error sending getSessions command\n");
    }
  }

  // --------------- Forward data over this new connection -----------
#define dLen 1024
  uint8_t dbuf[dLen];
  char *msg = (char *)dbuf;
  int   msgLen  = dLen;
  int   msgHlen = sizeof(int);
  int   msgPlen = msgLen - msgHlen;
  int  *msgHdr  = (int *)dbuf;

  // Simulate an NCCP Message
  //   Header: 4 Byte Message length (excluding the header's length)
  *msgHdr = htonl(msgPlen);
  for (int i = msgHlen; i < msgLen; ++i)
    dbuf[i] = (uint8_t)i;

  wunet::msgBuf<wucmd::hdr_t> mbuf;
  wucmd::hdr_t& hdr = mbuf.hdr();

  hdr.reset();
  hdr.cmd(wucmd::fwdData);
  hdr.conID(cid);
  hdr.dlen(msgLen);

  mbuf.plen(msgLen);
  mbuf.pcopyin(msg, (size_t)msgLen);

  wunet::msgBuf<wucmd::hdr_t> mbuf2;
  wucmd::hdr_t& hdr2 = mbuf2.hdr();
  wulog(wulogApp, wulogInfo, "\n\nSending data to echo server, Cnt = %d\n",
      cnt);
  for (int i = 0; i < cnt; ++i) {
    hdr.msgID(i+10);
    if (byteSend) {
      char *buf = (char *)mbuf.hpullup();
      wupp::errType stat = wupp::noError;
      //dataSock.write(buf, 2, stat);
      // cout << "\nTell me when to strat up again\n";
      // string s;
      // cin >> s;
      // cout << "Finishing\n";
      // for (size_t i = 2; i < hdr.hlen() && ! isError(stat);  ++i)
      //   dataSock.write((buf + i), 1, stat);
      // dataSock.write(buf, hdr.hlen(), stat);
      for (size_t i = 0; i < hdr.hlen() && ! wupp::isError(stat);  ++i)
        dataSock.write((buf+i), 1, stat);
      if (wupp::isError(stat)) {
        wulog(wulogApp, wulogError, "Error writing msg byte-by-byte\n");
        return 1;
      }
      buf = (char *)mbuf.ppullup();
      for (size_t i = 0; i < mbuf.plen() && ! wupp::isError(stat);  ++i)
        dataSock.write((buf+i), 1, stat);
      // dataSock.write(buf, mbuf.plen(), stat);
      if (wupp::isError(stat)) {
        wulog(wulogApp, wulogError, "Error writing msg byte-by-byte\n");
        return 1;
      }
    } else {
      hdr.conID(cid);
      if (wupp::isError(mbuf.writemsg(&dataSock))) {
        wulog(wulogApp, wulogWarn, "Error sending message\n");
        return 1;
      }
      // hdr.conID(cid2);
      // if (wupp::isError(mbuf.writemsg(&dataSock))) {
      //   wulog(wulogApp, wulogWarn, "Error sending message\n");
      //   return 1;
      // }
    }
    for (int j = 0; j < cpy; ++j) {
      if (wupp::isError(mbuf2.readmsg(&dataSock))) {
        wulog(wulogApp, wulogWarn, "Error reading message\n");
        return 1;
      }

      if (mbuf2.hdr().cmd() != wucmd::fwdData) {
        wulog(wulogApp, wulogError,
              "did not receive fwdData command!\n\t%s\n", mbuf2.hdr().toString(&mbuf2).c_str());
        return 1;
      }
      if (hdr2.dlen() != msgLen) {
        wulog(wulogApp, wulogError,
            "Error: returned message size incorrect. Wanted %u, got %u!\n", 
            msgLen, hdr2.dlen());
        return 1;
      }
      uint8_t *ptr = (uint8_t *)mbuf2.ppullup();
      for (int j = 0; j < msgLen; ++j) {
        if (ptr[j] != dbuf[j])
          wulog(wulogApp, wulogError, "ptr[%u] (%hhu) != msg[%u] (%hhu)\n", j, ptr[j], j, msg[j]);
      }
    }
  }
  // ------------------- Close Connection -----------------------------
  hdr.reset();
  mbuf.pclear();
  hdr.cmd(wucmd::closeConn);
  hdr.conID(cid);
  // hdr.conID(cid2);
  hdr.msgID(100);
  hdr.dlen(0);

  cout << "\n\nSending closeConn: " << mbuf << "\n";
  if (wupp::isError(mbuf.writemsg(&dataSock))) {
    wulog(wulogApp, wulogWarn, "Error sending message\n");
  }
  cout << "Reading reply ...\n\n";
  if (wupp::isError(mbuf.readmsg(&dataSock))) {
    wulog(wulogApp, wulogWarn, "Error reading message\n");
  }
  cout << "Got answer ... \n\t" << mbuf << "\n\n";

  // -- Now close our end -----------
  cout << "Done closing conn\n";
  dataSock.close();
  cout << "Connection closed\n";

  return(0);
}
