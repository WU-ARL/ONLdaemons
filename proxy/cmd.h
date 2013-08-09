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
 * $Date: 2007/03/23 21:05:04 $
 * $Revision: 1.7 $
 * $Name:  $
 *
 * File:         cmd.h
 * Created:      01/19/2005 02:02:46 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WUCMD_HDR_H
#define _WUCMD_HDR_H

namespace wucmd {

  typedef uint16_t mlen_t;
  typedef uint16_t cmd_t;
  typedef uint16_t msgID_t;
  typedef uint16_t conID_t;

  static const conID_t invalidCID  = 0;

  static const cmd_t cmdEcho       =  0; // RLI <--> proxy
  static const cmd_t fwdData       =  1; // RLI <--> proxy
  static const cmd_t openConn      =  2; // RLI x--> proxy
  static const cmd_t closeConn     =  3; // RLI x--> proxy
  static const cmd_t getConnStatus =  4; // RLI x--> proxy
  static const cmd_t cmdReply      =  5; // RLI <--x proxy
  static const cmd_t connStatus    =  6; // RLI <--x proxy
  static const cmd_t echoReply     =  7; // RLI <--x proxy
  static const cmd_t shareConn     =  8; // RLI x--> proxy
  static const cmd_t peerFwd       =  9; // RLI <--> proxy
  static const cmd_t copyFwd       = 10; // RLI <--x proxy
  static const cmd_t getSID        = 11; // RLI x--> proxy
  static const cmd_t retSID        = 12; // RLI <--x proxy
  static const cmd_t shareReply    = 13; // RLI <--x proxy
  static const cmd_t cmdCount      = 14;

  inline std::string cmd2str(cmd_t cmd);

  typedef uint32_t flags_t;
  // command status
  static const flags_t statusMask  = 0x0000000F;
  static const flags_t cmdSuccess  = 0x00000000; // commend completed with no errors
  static const flags_t cmdFailure  = 0x00000001; // command failed, see error flags
  static const flags_t cmdPending  = 0x00000002; // command has not completed.
                                                 // Not used unless we use async
                                                 // commands with strict request/response
  // connection status
  static const flags_t connMask    = 0x00000FF0; 
  static const flags_t connOpen    = 0x00000010; // connect is open and available
                                                 // for use if this bit is not
                                                 // set then it can be assumed
                                                 // that the connection is closed.
  static const flags_t connPending = 0x00000020; // connection state is changing,
                                                 // used if needed for async command
  static const flags_t connError   = 0x00000040; // Connection had an error and is not available
                                                 // Send a close command to free resources.
  // Error Codes
  static const flags_t errMask     = 0x000FF000;
  static const flags_t noError     = 0x00000000;
  static const flags_t paramError  = 0x00001000; // one or more parameters are in error
  static const flags_t authError   = 0x00002000; // 'User' is not authorized to perform requested action
  static const flags_t timeOut     = 0x00003000; // requested action timed out without completing. If it
                                                 // should complete at a later time then it will be 'undone'
  static const flags_t dstError    = 0x00004000; // unable to connect to target system (bad address)
  static const flags_t netError    = 0x00005000; // General network error, most likely transient
  static const flags_t badConnID   = 0x00006000; // connection ID has not been assigned
  static const flags_t connNotOpen = 0x00007000; // connection is assigned but is not in the Open state
  static const flags_t connFailure = 0x00008000; // Existing connection has 'failed', could be because the
                                                 // other end closed the connection.
  static const flags_t sysError    = 0x00010000; //

  // Command data structs
  // cmdEcho - None
  // fwdData - None
  // openConn:
  class connData {
#define wucmd_conndata_minLen (sizeof(uint16_t)+sizeof(uint8_t))
    public:
      static size_t minLen() {return (size_t)wucmd_conndata_minLen;}
      bool   validHost() const;
      size_t      dlen() const;

      char *setHost(const char *host, size_t hlen);

      connData();
      connData(const char *host, uint16_t port, uint8_t proto);
      virtual ~connData();

      uint16_t port() const;
      void     port(uint16_t p);
      uint8_t proto() const;
      void    proto(uint8_t p);
      char    *host() const;
      void     host(const char *h);

      // buf needs to be made large enough to hold the host_ string. A call to
      // dlen() should be used to determine total memory required before calling
      // copyOut.  data = (char *)malloc(conn->dlen());
      char *copyOut(char *buf) const;
      const char *copyIn(const char *buf, size_t dlen);

      std::string toString() const;
    private:
      uint16_t port_;
      uint8_t  proto_;
      char    *host_;

  };
  inline bool     connData::validHost() const {return host_ && *host_;}
  inline size_t   connData::dlen() const
                  {return (size_t)connData::minLen() + (validHost() ? strlen(host_) : 0);}
  inline uint16_t connData::port() const {return port_;}
  inline void     connData::port(uint16_t p) {port_ = p;}
  inline uint8_t  connData::proto() const {return proto_;}
  inline void     connData::proto(uint8_t p) {proto_ = p;}
  inline char    *connData::host() const {return host_;}
  inline void     connData::host(const char *h) {setHost(h, (h ? strlen(h) : 0));}

  // closeConn: None
  // getConnStatus: None
  // cmdReply and connStatus and shareReply
  struct replyData {
#define wucmd_condata_dlen sizeof(uint32_t)
    flags_t flags_;

    static size_t dlen();

    flags_t flags() const;
    void    flags(uint32_t f);

    flags_t cmdStatus() const;
    void    cmdStatus(flags_t f);

    flags_t connStatus() const;
    void    connStatus(flags_t f);

    flags_t errFlags() const;
    void    errFlags(flags_t f);

    replyData();
    std::string toString() const;
  };
  inline wucmd::flags_t replyData::flags() const {return ntohl(flags_);}
  inline void           replyData::flags(uint32_t f) {flags_ = htonl(f);};
  inline wucmd::flags_t replyData::cmdStatus() const {return flags() & statusMask;}
  inline void           replyData::cmdStatus(flags_t f) {flags((flags() & ~statusMask) | (statusMask & (f)));}
  inline wucmd::flags_t replyData::connStatus() const {return flags() & connMask;}
  inline void           replyData::connStatus(flags_t f) {flags((flags() & ~connMask) | (connMask & (f)));}
  inline wucmd::flags_t replyData::errFlags() const {return ntohl(flags_) & errMask;}
  inline void           replyData::errFlags(flags_t f) {flags((flags() & ~errMask) | (errMask & (f)));}

  // shareConn:
  struct sessData {
#define wucmd_sessdata_dlen sizeof(conID_t)
    conID_t sessID_;

    static size_t dlen();

    conID_t sessID() const;
    void sessID(conID_t s);
    const char *copyIn(const char *);

    sessData();
    std::string toString() const;
  };
  inline wucmd::conID_t sessData::sessID() const {return ntohs(sessID_);}
  inline void sessData::sessID(conID_t s) {sessID_ = htons(s);}

  //peerFwd: none
  //copyFwd: none
  //getSID: none

  //retSID: 
  struct replySessData {
    struct replyData rd_;
    struct sessData sd_;

    static size_t dlen();

    struct replyData *replydata();
    struct sessData *sessdata();

    replySessData();
    std::string toString() const;
  };
  inline struct replyData *replySessData::replydata() {return &rd_;};
  inline struct sessData *replySessData::sessdata() {return &sd_;};

  struct hdr_t {
    mlen_t   len_;
    cmd_t    cmd_;
    msgID_t  mid_;
    conID_t  cid_;
#define wucmd_hdr_hlen  8

    hdr_t() {reset();}
    void reset() {mlen(hdrlen()); cmd(0); conID(0); msgID(0);}

    static size_t hdrlen() {return (size_t)wucmd_hdr_hlen;}
    // bound the max command data, for example ReplyData or Flags
    // In other words, think of this as a variable sized header, this const
    // gives the upper bound on the size
    static const size_t maxHdrDataLen = 64; // 8 4-Byte words is max command data

    size_t hlen() const {return hdrlen();}
    mlen_t mlen() const {return ntohs(len_);}

    mlen_t dlen() const {return mlen() - hlen();}
    void   dlen(mlen_t x) {len_ = htons(x + hlen());}

    cmd_t cmd() const {return ntohs(cmd_);}
    void  cmd(cmd_t x) {cmd_ = htons(x);}

    conID_t conID() const {return ntohs(cid_);}
    void    conID(conID_t x) {cid_ = htons(x);}

    msgID_t msgID() const {return ntohs(mid_);}
    void    msgID(msgID_t x) {mid_ = htons(x);}

    static flags_t err2flag(wupp::errType);

    std::string toString(const wunet::mbuf *) const;

    private:
    void     mlen(mlen_t x) {len_ = htons(x);}
  };

  inline std::ostream& operator<<(std::ostream& os, const hdr_t& mhdr);

}
#endif
