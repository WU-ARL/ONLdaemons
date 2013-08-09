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
 * File:         control.h
 * Created:      02/07/2005 02:02:04 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WUCONTROL_H
#define _WUCONTROL_H

namespace wuctl {

  typedef uint16_t mlen_t;
  typedef uint16_t cmd_t;
  typedef uint16_t msgID_t;
  typedef uint16_t flags_t;

  // define separately to drive home the point that this is a physical mormat
  // for a message on the wire.
  struct msgHdr {
    mlen_t   len_;
    cmd_t    cmd_;
    msgID_t  mid_;
    flags_t  flg_;
  };

  class sessionCmd {
    public:
      typedef uint8_t  cmd_type;
      typedef uint16_t arg_type;
      static const cmd_type cmdNull       = 0; // manager <--> session
      static const cmd_type cmdShutdown   = 1; // manager x--> session
      static const cmd_type cmdCloseConn  = 2; // manager x--> session
      static const cmd_type cmdSlave      = 3; // manager x--> session
      static const cmd_type cmdSlaveResp  = 4; // manager <--x session
      static const cmd_type cmdMaster     = 5; // manager <--x session
      static const cmd_type cmdMasterResp = 6; // manager x--> session
      static const cmd_type cmdCount      = 7; 

      static const arg_type nullArg        = 0; 
   
      // master response possibilities
      static const arg_type masterOK       = 1; 
      static const arg_type masterNotExist = 2; 
      static const arg_type masterReject   = 3; 
      static const arg_type masterError    = 4; 

      // slave response possibilities
      static const arg_type slaveOK        = 1; 
      static const arg_type slaveReject    = 2; 

      static const int msgLen = (sizeof(cmd_type) + sizeof(arg_type));

      sessionCmd();
      sessionCmd(cmd_type cmd, arg_type arg);

      void set(cmd_type cmd, arg_type arg);
      void reset();
      cmd_type cmd() const;
      arg_type arg() const;

      int readCmd(int fd);;
      int writeCmd(int fd);
    private:
      cmd_type cmd_;
      arg_type arg_;

      char *cmd2buf(char *buf);
      void buf2cmd(char *buf);
  }; /* sessionCmd */
  inline sessionCmd::sessionCmd() {reset();}
  inline sessionCmd::sessionCmd(cmd_type cmd, arg_type arg) : cmd_(cmd), arg_(arg) {}
  inline void sessionCmd::set(cmd_type cmd, arg_type arg) {cmd_ = cmd; arg_ = arg;}
  inline void sessionCmd::reset() {cmd_ = cmdNull; arg_ = nullArg;}
  inline sessionCmd::cmd_type sessionCmd::cmd() const {return cmd_;}
  inline sessionCmd::arg_type sessionCmd::arg() const {return arg_;}


  class hdr_t {
    private:
      static const size_t HLEN = sizeof(msgHdr);
      msgHdr hdr_;

    public:
      static const cmd_t getSessions   = 0;
      static const cmd_t remSession    = 1;
      static const cmd_t remConnection = 2;
      static const cmd_t setLogLevel   = 3;
      static const cmd_t nullCommand   = 4;

      static std::string cmd2str(cmd_t cmd);
      static const flags_t replyFlag   = 0x01; // set for replies, otherwise a command
      static const flags_t errorFlag   = 0x10; // just used with replies, command did not succeed
      static const flags_t badParams   = 0x20; // original command had invalid params

      static std::string flags2str(flags_t flgs);

      hdr_t();
      hdr_t(const hdr_t &h);
      hdr_t(const msgHdr &h);
      hdr_t(mlen_t l, cmd_t c, msgID_t m, flags_t f);

      static size_t hdrlen();
      size_t hlen() const;

      mlen_t mlen() const;
      void   mlen(mlen_t ml);

      mlen_t dlen() const;
      void   dlen(int x);

      cmd_t cmd() const;
      void  cmd(cmd_t x);

      msgID_t msgID() const;
      void    msgID(msgID_t x);

      flags_t flags() const;
      void    flags(flags_t x);

      bool isReply()      const;
      bool cmdSucceeded() const;

      void reset();

      std::string toString() const;
      std::string toString(const wunet::mbuf *msg) const;
      std::ostream& print(std::ostream&os) const;
  };
  inline hdr_t::hdr_t() {reset();}
  inline hdr_t::hdr_t(const hdr_t &h) : hdr_(h.hdr_) {} // memberwise copy good
  inline hdr_t::hdr_t(const msgHdr &h) : hdr_(h) {}     // memberwise copy good
  inline hdr_t::hdr_t(mlen_t l, cmd_t c, msgID_t m, wuctl::flags_t f) {mlen(l); cmd(c); msgID(m), flags(f);}

  inline size_t hdr_t::hdrlen() {return (size_t)HLEN;}
  inline size_t hdr_t::hlen() const {return hdr_t::hdrlen();}

  inline mlen_t hdr_t::mlen() const {return ntohs(hdr_.len_);}
  inline void   hdr_t::mlen(mlen_t ml) {hdr_.len_ = htons(ml);}

  inline mlen_t hdr_t::dlen() const {return mlen() - hlen();}
  inline void   hdr_t::dlen(int x) {hdr_.len_ = htons(x + hlen());}

  inline cmd_t hdr_t::cmd() const {return ntohs(hdr_.cmd_);}
  inline void  hdr_t::cmd(cmd_t x) {hdr_.cmd_ = htons(x);}

  inline msgID_t hdr_t::msgID() const {return ntohs(hdr_.mid_);}
  inline void    hdr_t::msgID(msgID_t x) {hdr_.mid_ = htons(x);}

  inline wuctl::flags_t hdr_t::flags() const {return ntohs(hdr_.flg_);}
  inline void    hdr_t::flags(wuctl::flags_t x) {hdr_.flg_ = htons(x);}

  inline bool hdr_t::isReply()      const {return (flags() & replyFlag) == replyFlag;}
  inline bool hdr_t::cmdSucceeded() const {return (flags() == replyFlag);}

  inline void hdr_t::reset() {mlen(hdr_t::hdrlen()); cmd(0); msgID(0); flags(0);}

  struct BasicStats {
    uint32_t rxMsgCnt, rxByteCnt;
    uint32_t rxMsgPrt, rxMsgErr;
    uint32_t txMsgCnt, txByteCnt;
    uint32_t txMsgPrt, txMsgErr;
    time_t   started;

    BasicStats();
    char *copyOut(char *buf) const;
    char *copyIn(char *buf);
    void reset();
    std::string toString(const std::string &pre = "") const;
  };


  struct msgInfo_t {
    uint32_t       Laddr_; // local address
    uint32_t       Daddr_; // destination address
    uint16_t       Lport_; // local  port
    uint16_t       Dport_; // destination port
    uint16_t         cid_; // connection ID
    uint8_t        proto_; // protocol
    uint8_t        flags_;
    BasicStats     stats_;
  };

  struct connInfo {
    static const size_t DLEN = sizeof(msgInfo_t);
    msgInfo_t msg_;

    static size_t dlen() {return DLEN;}
    static const uint8_t flagSession = 0x01; // Session else forwarded conection
    static const uint8_t flagOpen    = 0x02; // Open else closed
    static const uint8_t flagPending = 0x04; // Open/Close pending
    static const uint8_t flagInvalid = 0xFF; // Invalid entry

    connInfo();
    connInfo(const connInfo &ci);
    connInfo(const msgInfo_t &mi);

    void reset();

    uint32_t laddr() const;
    void     laddr(uint32_t la);

    uint32_t daddr() const;
    void     daddr(uint32_t da);

    uint16_t lport() const;
    void     lport(uint16_t lp);

    uint16_t dport() const;
    void     dport(uint16_t dp);

    uint16_t cid()   const;
    void     cid(uint16_t x);
    void     cid(int x);

    uint8_t  proto() const;
    void     proto(uint8_t pr);
    void     proto(int pr);

    uint8_t  flags() const;
    void     flags(uint8_t st);

    BasicStats stats() const;
    void stats(const BasicStats &stats);

    char *copyOut(char *buf) const;
    char *copyIn(char *buf);

    std::string toString() const;
    static std::string flags2String(uint8_t);
  };

  inline void connInfo::reset()
         {
           msg_.Laddr_ = 0;
           msg_.Daddr_ = 0;
           msg_.Lport_ = 0;
           msg_.Dport_ = 0;
           msg_.cid_   = 0;
           msg_.proto_ = 0;
           msg_.flags_ = 0;
           msg_.stats_.reset();
         }

  inline uint32_t connInfo::laddr() const      {return msg_.Laddr_;}
  inline void     connInfo::laddr(uint32_t la) {msg_.Laddr_ = la;}

  inline uint32_t connInfo::daddr() const      {return msg_.Daddr_;}
  inline void     connInfo::daddr(uint32_t da) {msg_.Daddr_ = da;}

  inline uint16_t connInfo::lport() const      {return msg_.Lport_;}
  inline void     connInfo::lport(uint16_t lp) {msg_.Lport_ = lp;}

  inline uint16_t connInfo::dport() const      {return msg_.Dport_;}
  inline void     connInfo::dport(uint16_t dp) {msg_.Dport_ = dp;}

  inline uint16_t connInfo::cid()   const      {return ntohs(msg_.cid_);};
  inline void     connInfo::cid(uint16_t x)    {msg_.cid_ = htons(x);};
  inline void     connInfo::cid(int x)         {msg_.cid_ = htons(uint16_t(x));};

  inline uint8_t  connInfo::proto() const     {return msg_.proto_;}
  inline void     connInfo::proto(uint8_t pr) {msg_.proto_ = pr;};
  inline void     connInfo::proto(int pr)     {msg_.proto_ = uint8_t(pr);};

  inline uint8_t  connInfo::flags() const      {return msg_.flags_;}
  inline void     connInfo::flags(uint8_t st)  {msg_.flags_ = st;}

  std::ostream&  print(std::ostream&, const std::vector<connInfo*>&);
  std::string toString(const std::vector<connInfo*>& cvec);

};

#endif
