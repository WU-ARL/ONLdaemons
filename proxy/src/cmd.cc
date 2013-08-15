/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/cmd.cc,v $
 * $Author: cgw1 $
 * $Date: 2007/03/23 21:05:04 $
 * $Revision: 1.8 $
 * $Name:  $
 *
 * File:         cmd.cc
 * Created:      01/25/2005 11:36:07 AM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstdlib>


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

wucmd::flags_t wucmd::hdr_t::err2flag(wupp::errType et)
{
  wucmd::flags_t flag = wucmd::noError;

  if (! wupp::isError(et) || et == wupp::intrSysCall)
    return wucmd::noError;

  switch(et) {
    case wupp::connFailure: flag = wucmd::connFailure; break;
    case wupp::protoError:  flag = wucmd::netError; break;
    case wupp::timeOut:     flag = wucmd::timeOut; break;
    case wupp::paramError:  flag = wucmd::paramError; break;
    case wupp::authError:   flag = wucmd::authError; break;
    case wupp::addrError:   flag = wucmd::dstError; break;
    case wupp::dstError:    flag = wucmd::dstError; break;
    case wupp::badConn:     flag = wucmd::connNotOpen; break;
    case wupp::netError:    flag = wucmd::netError; break;
    case wupp::badClose:    flag = wucmd::connFailure; break;
    case wupp::fmtError:    flag = wucmd::connFailure; break;
    case wupp::dupError:    flag = wucmd::paramError; break;
    default: flag = wucmd::sysError;
  }

  return flag;
}

std::string wucmd::hdr_t::toString(const wunet::mbuf *msg) const
{
  std::ostringstream ss;
  ss << "cmd "      << cmd2str(cmd()) << " (" << int(cmd()) << ")"
    << ", len "     << mlen()         << " (" << hlen() << "/" << dlen() << ")"
    << ", connID "  << conID()
    << ", msgID "   << msgID();

  if (msg && dlen() > 0) {
    ss << "\n\tdata: ";

    switch (cmd()) {
      case cmdEcho:
        msg->printData(ss);
        break;
      case echoReply:
        msg->printData(ss);
        break;
      case fwdData:
        msg->printData(ss);
        break;
      case openConn:
        {
          char *data = NULL;
          try {
            if ((data = (char *)malloc(msg->plen())) == NULL) {
              ss << "openConn -- resource error reading message -- ";
            } else {
              msg->pcopyout(data, msg->plen());
              connData cd;
              cd.copyIn(data, dlen());
              ss << cd.toString();
              free(data);
              data = NULL;
            }
          } catch (...) {
            if (data)
              free(data);
          }
        }
        break;
      case closeConn:
        msg->printData(ss);
        break;
      case getConnStatus:
        msg->printData(ss);
        break;
      case cmdReply:
        {
          char *data = NULL;
          try {
            if ((data = (char *)malloc(msg->plen())) == NULL) {
              ss << "cmdReply -- resource error reading message -- ";
            } else {
              msg->pcopyout(data, msg->plen());
              wucmd::replyData *rd = (wucmd::replyData *)data;
              ss << rd->toString();
              free(data);
              data = NULL;
            }
          } catch (...) {
            if (data)
              free(data);
          }
        }
        break;
      case connStatus:
        {
          char *data = NULL;
          try {
            if ((data = (char *)malloc(msg->plen())) == NULL) {
              ss << "connStatus -- resource error reading message -- ";
            } else {
              msg->pcopyout(data, msg->plen());
              wucmd::replyData *rd = (wucmd::replyData *)data;
              ss << rd->toString();
              free(data);
              data = NULL;
            }
          } catch (...) {
            if (data)
              free(data);
          }
        }
        break;
      case shareConn:
        {
          char *data = NULL;
          try {
            if ((data = (char *)malloc(msg->plen())) == NULL) {
              ss << "shareConn -- resource error reading message -- ";
            } else {
              msg->pcopyout(data, msg->plen());
              wucmd::sessData *sd = (wucmd::sessData *)data;
              ss << sd->toString();
              free(data);
              data = NULL;
            }
          } catch (...) {
            if (data)
              free(data);
          }
        }
        break;
      case peerFwd:
        msg->printData(ss);
        break;
      case copyFwd:
        msg->printData(ss);
        break;
      case getSID:
        msg->printData(ss);
        break;
      case retSID:
        {
          char *data = NULL;
          try {
            if ((data = (char *)malloc(msg->plen())) == NULL) {
              ss << "retSID -- resource error reading message -- ";
            } else {
              msg->pcopyout(data, msg->plen());
              wucmd::replySessData *rsd = (wucmd::replySessData *)data;
              ss << rsd->toString();
              free(data);
              data = NULL;
            }
          } catch (...) {
            if (data)
              free(data);
          }
        }
        break;
      case shareReply:
        {
          char *data = NULL;
          try {
            if ((data = (char *)malloc(msg->plen())) == NULL) {
              ss << "shareReply -- resource error reading message -- ";
            } else {
              msg->pcopyout(data, msg->plen());
              wucmd::replyData *rd = (wucmd::replyData *)data;
              ss << rd->toString();
              free(data);
              data = NULL;
            }
          } catch (...) {
            if (data)
              free(data);
          }
        }
        break;


    }
  }
  return ss.str();
}

wucmd::connData::connData()
  : port_(0), proto_(0), host_(0) {};
wucmd::connData::connData(const char *host, uint16_t port, uint8_t proto)
  : port_(port), proto_(proto), host_(0)
  { setHost(host, (host ? strlen(host) : 0)); };
wucmd::connData::~connData()
  { if (host_) free(host_); host_ = NULL; }

char *wucmd::connData::setHost(const char *host, size_t hlen)
{
  // keeping it simple, just free and realloc
  if (host_) {
    free(host_);
    host_ = NULL;
  }

  if (host && hlen > 0) {
    host_ = (char *)malloc(hlen + 1);
    if (host_ == NULL) {
      wulog(wulogCmd, wulogError, "connData::setHost: Unable to allocate memory for host %s\n",
          ((host && *host) ? host : "null"));
      return NULL;
    }
    memcpy((void *)host_, (const void *)host, hlen);
    host_[hlen] = '\0';
  }
  return host_;
}
char *wucmd::connData::copyOut(char *buf) const
{
  *(uint16_t *)buf = port_;  buf += sizeof(uint16_t);
  *(uint8_t *)buf  = proto_; buf += sizeof(uint8_t);
  if (validHost())
    memcpy((void *)buf, (const void *)host_, strlen(host_));
  return buf;
}
const char *wucmd::connData::copyIn(const char *buf, size_t dlen)
{
  size_t hlen = dlen - connData::minLen();
  port_  = *(uint16_t *)buf; buf += sizeof(uint16_t);
  proto_ = *(uint8_t *)buf;  buf += sizeof(uint8_t);
  if (setHost(buf, hlen) == NULL)
    return NULL;
  return buf;
}

std::string wucmd::connData::toString() const
{
  std::ostringstream ss;
  ss << (validHost() ? host_ : "null")
    << "/"  << (int)ntohs(port_)
    << "/" << (int)proto_;
  return ss.str();
}

wucmd::replyData::replyData()
  : flags_(0) {}

size_t wucmd::replyData::dlen() {return wucmd_condata_dlen;}

std::string wucmd::replyData::toString() const
{
  std::ostringstream ss;
  ss << "Flags: " << std::hex << flags() <<std::dec << "; Status (";

  wucmd::flags_t cs = cmdStatus();
  if (cs == wucmd::cmdSuccess)
    ss << "Success";
  else if (cs == wucmd::cmdFailure)
    ss << "Failure";
  else if (cs == wucmd::cmdPending)
    ss << "Pending";
  else
    ss << "Unknown";

  ss << "); Conn (";
  cs = connStatus();
  if (cs & wucmd::connOpen)
    ss << "Open";
  else
    ss << "Closed";
  if (cs & wucmd::connError)
    ss << ",Error";
  if (cs & wucmd::connPending)
    ss << ",Pending";

  ss << "); Codes (";
  cs = errFlags();
  switch (cs) {
    case wucmd::noError:
      ss << "noError";
      break;
    case wucmd::paramError:
      ss << "paramError";
      break;
    case wucmd::authError:
      ss << "authError";
      break;
    case wucmd::timeOut:
      ss << "timeOut";
      break;
    case wucmd::dstError:
      ss << "dstError";
      break;
    case wucmd::netError:
      ss << "netError";
      break;
    case wucmd::badConnID:
      ss << "badConnID";
      break;
    case wucmd::connNotOpen:
      ss << "connNotOpen";
      break;
    case wucmd::connFailure:
      ss << "connFailure";
      break;
    case wucmd::sysError:
      ss << "sysError";
      break;
    default:
      ss << "Unknown";
      break;
  }
  ss << ")";

  return ss.str();
}

wucmd::sessData::sessData()
  : sessID_(0) {}

size_t wucmd::sessData::dlen() {return wucmd_sessdata_dlen;}

std::string wucmd::sessData::toString() const
{
  std::ostringstream ss;
  ss << "Session ID: " << sessID();
  return ss.str();
}

const char *wucmd::sessData::copyIn(const char *buf)
{
  sessID_ = *(uint16_t *)buf; buf += sizeof(uint16_t);
  return buf;
}


wucmd::replySessData::replySessData()
  { rd_.flags(0); sd_.sessID(0); }

size_t wucmd::replySessData::dlen() {return (replyData::dlen() + sessData::dlen());}

std::string wucmd::replySessData::toString() const
{
  std::ostringstream ss;
  ss << rd_.toString() << "; " << sd_.toString();
  return ss.str();
}

std::string wucmd::cmd2str(wucmd::cmd_t cmd)
{
  std::string str;
  switch(cmd) {
    case wucmd::cmdEcho:
      str = "Echo";
      break;
    case wucmd::fwdData:
      str = "fwdData";
      break;
    case wucmd::openConn:
      str = "openConn";
      break;
    case wucmd::closeConn:
      str = "closeConn";
      break;
    case wucmd::getConnStatus:
      str = "getConnStatus";
      break;
    case wucmd::cmdReply:
      str = "cmdReply";
      break;
    case wucmd::connStatus:
      str = "connStatus";
      break;
    case wucmd::echoReply:
      str = "echoReply";
      break;
    case wucmd::shareConn:
      str = "shareConn";
      break;
    case wucmd::peerFwd:
      str = "peerFwd";
      break;
    case wucmd::copyFwd:
      str = "copyFwd";
      break;
    case wucmd::getSID:
      str = "getSID";
      break;
    case wucmd::retSID:
      str = "retSID";
      break;
    case wucmd::shareReply:
      str = "shareReply";
      break;
    default:
      str = "Unknown";
      break;
  }
  return str;
}

std::ostream& operator<<(std::ostream& os, const wucmd::hdr_t& mhdr)
{
  os << mhdr.toString(NULL);
  return os;
}
