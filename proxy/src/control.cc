/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/control.cc,v $
 * $Author: cgw1 $
 * $Date: 2008/04/09 18:39:35 $
 * $Revision: 1.9 $
 * $Name:  $
 *
 * File:         control.cc
 * Created:      01/25/2005 11:36:07 AM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <wuPP/net.h>
#include <wuPP/buf.h>
#include <wuPP/msg.h>
#include "control.h"
#include "cmd.h"

using namespace wuctl;

const size_t wuctl::hdr_t::HLEN;
const size_t wuctl::connInfo::DLEN;

wuctl::connInfo::connInfo() {reset();}
wuctl::connInfo::connInfo(const connInfo &ci)  : msg_(ci.msg_) {}
wuctl::connInfo::connInfo(const msgInfo_t &mi) : msg_(mi) {}

std::string wuctl::connInfo::flags2String(uint8_t f)
{
  std::ostringstream ss;
  if (f == flagInvalid) {
    ss << "Invalid Entry";
    return ss.str();
  }

  if (f & flagSession)
    ss << "Session";
  else
    ss << "Fwd";
  if (f & flagOpen)
    ss << ", Open";
  else
    ss << ", Closed";
  if (f & flagPending)
    ss << ", Pending";

  return ss.str();
}

std::string wuctl::connInfo::toString() const
{
  std::ostringstream ss;

  WUAddrStr_t Lstr, Dstr;
  WUAddr_t    Laddr, Daddr;

  Laddr.addr  = laddr();
  Laddr.port  = ntohs(lport());
  Laddr.proto = proto();
  wunet_Addr2String(&Laddr, &Lstr);

  Daddr.addr  = daddr();
  Daddr.port  = ntohs(dport());
  Daddr.proto = proto();
  wunet_Addr2String(&Daddr, &Dstr);

  ss << "ID " << cid() << ", Flags (" << connInfo::flags2String(flags()) << ")"
              << ", Proto "   << Lstr.proto << " (" << int(proto()) << ")"
     << "\n\tLocal:  " << Lstr.haddr.host << "/" << Lstr.port
              << " -- (" << std::hex << ntohl(laddr()) << "/" << ntohs(lport()) << std::dec << ")"
     << "\n\tRemote: " << Dstr.haddr.host << "/" << Dstr.port
              << " -- (" << std::hex << ntohl(daddr()) << "/" << ntohs(dport()) << std::dec << ")"
     << "\n" << stats().toString("\t");
  return ss.str();
}

std::ostream& wuctl::print(std::ostream& os, const std::vector<connInfo*>& cvec)
{
  std::vector<connInfo*>::const_iterator dit = cvec.begin();
  std::vector<connInfo*>::const_iterator dend = cvec.end();
  for(; dit != dend; dit++) {
    if ((*dit)->flags() & connInfo::flagSession)
      os << "Session (RLI<-->Proxy): \n";
    os << (*dit)->toString() << "\n";
  }
  return os;
}

std::string wuctl::toString(const std::vector<wuctl::connInfo*>& cvec)
{
  std::ostringstream os;
  wuctl::print(os, cvec);
  return os.str();
}

char *wuctl::connInfo::copyOut(char *buf) const
{
  char *ptr = buf;
  *(uint32_t *)ptr = msg_.Laddr_;       ptr += sizeof(msg_.Laddr_);
  *(uint32_t *)ptr = msg_.Daddr_;       ptr += sizeof(msg_.Daddr_);
  *(uint16_t *)ptr = msg_.Lport_;       ptr += sizeof(msg_.Lport_);
  *(uint16_t *)ptr = msg_.Dport_;       ptr += sizeof(msg_.Dport_);
  *(uint16_t *)ptr = msg_.cid_;         ptr += sizeof(msg_.cid_);
  *(uint8_t  *)ptr = msg_.proto_;       ptr += sizeof(msg_.proto_);
  *(uint8_t  *)ptr = msg_.flags_;       ptr += sizeof(msg_.flags_);
  return msg_.stats_.copyOut(ptr);
}

char *wuctl::connInfo::copyIn(char *buf)
{
  char *ptr = buf;
  msg_.Laddr_       = *(uint32_t *)ptr; ptr += sizeof(msg_.Laddr_);
  msg_.Daddr_       = *(uint32_t *)ptr; ptr += sizeof(msg_.Daddr_);
  msg_.Lport_       = *(uint16_t *)ptr; ptr += sizeof(msg_.Lport_);
  msg_.Dport_       = *(uint16_t *)ptr; ptr += sizeof(msg_.Dport_);
  msg_.cid_         = *(uint16_t *)ptr; ptr += sizeof(msg_.cid_);
  msg_.proto_       = *(uint8_t  *)ptr; ptr += sizeof(msg_.proto_);
  msg_.flags_       = *(uint8_t  *)ptr; ptr += sizeof(msg_.flags_);
  return  msg_.stats_.copyIn(ptr);
}

wuctl::BasicStats wuctl::connInfo::stats() const
{
  BasicStats temp;
  temp.rxMsgCnt  = ntohl(msg_.stats_.rxMsgCnt);
  temp.rxByteCnt = ntohl(msg_.stats_.rxByteCnt);
  temp.rxMsgPrt  = ntohl(msg_.stats_.rxMsgPrt);
  temp.rxMsgErr  = ntohl(msg_.stats_.rxMsgErr);
  temp.txMsgCnt  = ntohl(msg_.stats_.txMsgCnt);
  temp.txByteCnt = ntohl(msg_.stats_.txByteCnt);
  temp.txMsgPrt  = ntohl(msg_.stats_.txMsgPrt);
  temp.txMsgErr  = ntohl(msg_.stats_.txMsgErr);
  return temp;
}

void wuctl::connInfo::stats(const BasicStats &stats)
{
  msg_.stats_.rxMsgCnt  = ntohl(stats.rxMsgCnt);
  msg_.stats_.rxByteCnt = ntohl(stats.rxByteCnt);
  msg_.stats_.rxMsgPrt  = ntohl(stats.rxMsgPrt);
  msg_.stats_.rxMsgErr  = ntohl(stats.rxMsgErr);
  msg_.stats_.txMsgCnt  = ntohl(stats.txMsgCnt);
  msg_.stats_.txByteCnt = ntohl(stats.txByteCnt);
  msg_.stats_.txMsgPrt  = ntohl(stats.txMsgPrt);
  msg_.stats_.txMsgErr  = ntohl(stats.txMsgErr);
}


int wuctl::sessionCmd::readCmd(int fd)
{
  int r;
  char buf[msgLen];

  if (fd < 0) {
    wulog(wulogCmd, wulogError, "ctl::readCmd: Passed an invalid control file descriptor\n");
    cmd_ = wuctl::sessionCmd::cmdNull;
    arg_ = wuctl::sessionCmd::nullArg;
    return -1;
  }
  r = read(fd, &buf, msgLen);
  if (r == 0) {
    wulog(wulogCmd, wulogLoud, "ctl::readCmd: read EOF\n");
    cmd_ = wuctl::sessionCmd::cmdEOF;
    arg_ = wuctl::sessionCmd::nullArg;
    return 1;
  }
  else if (r != msgLen) {
    wulog(wulogCmd, wulogError, "ctl::readCmd: Error reading command, errno = %d\n", errno);
    cmd_ = wuctl::sessionCmd::cmdNull;
    arg_ = wuctl::sessionCmd::nullArg;
    return -1;
  }
  buf2cmd(buf);
  return 1;
}

int wuctl::sessionCmd::writeCmd(int fd)
{
  char buf[msgLen];
  if (fd < 0) {
    wulog(wulogCmd, wulogError, "ctl::writeCmd: Passed an invalid control file descriptor\n");
    return -1;
  }
  cmd2buf(buf);
  if (write(fd, buf, msgLen) != msgLen) {
    wulog(wulogCmd, wulogError, "ctl::writecmd: Error writing command, errno = %d\n", errno);
    return -1;
  }
  return 0;
}
char *wuctl::sessionCmd::cmd2buf(char *buf)
{
  if (buf == NULL)
    return NULL;
  *(wuctl::sessionCmd::cmd_type *)buf = cmd_;
  *(wuctl::sessionCmd::arg_type *)(buf + sizeof(wuctl::sessionCmd::cmd_type)) = arg_;
  return buf;
}

void wuctl::sessionCmd::buf2cmd(char *buf)
{
  if (buf == NULL) {
    cmd_ = cmdNull;
    arg_ = nullArg;
  } else {
    cmd_ = *(wuctl::sessionCmd::cmd_type *)buf;
    arg_ = *(wuctl::sessionCmd::arg_type *)(buf + sizeof(wuctl::sessionCmd::cmd_type));
  }
  return;
}
std::string wuctl::hdr_t::cmd2str(cmd_t cmd)
{
  char *str = NULL;
  switch (cmd) {
    case wuctl::hdr_t::getSessions:
      str = "List sessions";
      break;
    case wuctl::hdr_t::remSession:
      str = "Remove session";
      break;
    case wuctl::hdr_t::remConnection:
      str = "Remove connection";
      break;
    case wuctl::hdr_t::setLogLevel:
      str = "Set log level";
      break;
    case wuctl::hdr_t::nullCommand:
      str = "Ping";
      break;
    default:
      str = "Unknown command";
      break;
  }
  return str;
}

std::string wuctl::hdr_t::flags2str(flags_t flgs)
{
  std::ostringstream ss;
  if (flgs & wuctl::hdr_t::replyFlag)
    ss << "Reply";
  else
    ss << "Cmd";
  if (flgs & wuctl::hdr_t::errorFlag)
    ss << "|Error";
  else
    ss << "|OK";
  if (flgs & wuctl::hdr_t::badParams)
    ss << "|Bad Params";
  return ss.str();
}

std::string wuctl::hdr_t::toString() const
{
  std::ostringstream ss;
  ss << "cmd \'"     << wuctl::hdr_t::cmd2str(cmd()) << "\' (" << (int)cmd() << ")"
      << ", len "   << (int)mlen() << " (" << (int)hlen() << "/" << (int)dlen() << ")"
      << ", msgID " << (int)msgID()
      << ", flags \'" << wuctl::hdr_t::flags2str(flags())
                      << "\' (" << std::hex << (unsigned int)flags() << std::dec << ")";
  return ss.str();
}

std::ostream& wuctl::hdr_t::print(std::ostream&os) const
{
  return os << toString();
}

std::string wuctl::hdr_t::toString(const wunet::mbuf *mb) const
{
  std::ostringstream ss;
  this->print(ss);

  if (mb != NULL)
    mb->printData(ss);

  return ss.str();
}

wuctl::BasicStats::BasicStats()
  : rxMsgCnt(0), rxByteCnt(0), rxMsgPrt(0), rxMsgErr(0),
    txMsgCnt(0), txByteCnt(0), txMsgPrt(0), txMsgErr(0)
{}
char *wuctl::BasicStats::copyOut(char *buf) const
{
  uint32_t *ptr = (uint32_t *)buf;
  *ptr++ = rxMsgCnt;
  *ptr++ = rxByteCnt;
  *ptr++ = rxMsgPrt;
  *ptr++ = rxMsgErr;
  *ptr++ = txMsgCnt;
  *ptr++ = txByteCnt;
  *ptr++ = txMsgPrt;
  *ptr++ = txMsgErr;
  return (char *)ptr;
}

char *wuctl::BasicStats::copyIn(char *buf)
{
  uint32_t *ptr = (uint32_t *)buf;
  rxMsgCnt  = *ptr++;
  rxByteCnt = *ptr++;
  rxMsgPrt  = *ptr++;
  rxMsgErr  = *ptr++;
  txMsgCnt  = *ptr++;
  txByteCnt = *ptr++;
  txMsgPrt  = *ptr++;
  txMsgErr  = *ptr++;
  return (char *)ptr;
}

void wuctl::BasicStats::reset()
{
  rxMsgCnt = 0;
  rxByteCnt = 0;
  rxMsgPrt = 0;
  rxMsgErr = 0;
  txMsgCnt = 0;
  txByteCnt = 0;
  txMsgPrt = 0;
  txMsgErr = 0;
}
std::string wuctl::BasicStats::toString(const std::string &pre) const
{
  std::ostringstream ss;
  ss << pre << "Rx Msg/B " << rxMsgCnt << "/" << rxByteCnt
                    << ", Part/Err " << rxMsgPrt << "/" << rxMsgErr << "\n";
  ss << pre << "Tx Msg/B " << txMsgCnt << "/" << txByteCnt
                    << ", Part/Err " << txMsgPrt << "/" << txMsgErr;
  return ss.str();
}
