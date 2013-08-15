/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/addr.h,v $
 * $Author: fredk $
 * $Date: 2007/07/27 23:54:52 $
 * $Revision: 1.10 $
 * $Name:  $
 *
 * File:         net.h
 * Created:      01/05/2005 04:32:38 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WUNET_ADDR_H
#define _WUNET_ADDR_H

#include <wuPP/net.h>

namespace wunet {

#define AddrErrLog() \
  do { \
    std::cerr << "inetAddr Error: File " << __FILE__ << ", Function " << __func__ << ", Line " << __LINE__ << "\n";\
    this->va_ = false; \
  } while (/* */ 0)

#ifdef __CYGWIN__
  typedef unsigned short sa_family_t;
#endif
class inetAddr {
  private:
    void set_anyAddr();
  public:
    // Constructors: the default constructor will set the IP address to INADDR_ANY
    // (the wildcard address which happens to equal 0), port number to 0 (any port) and
    // family to AF_INET. Likewise the default protocol value is 0 == IPPROTO_IP.
    inetAddr(int pr=0);
    inetAddr(const inetAddr& addr)
      : sa_(addr.sa_), pr_(addr.pr_), ho_(addr.ho_), ip_(addr.ip_), va_(true) {}
    inetAddr(const struct sockaddr_in& sa, int pr=0);
    // addr and port must be in NBO
    inetAddr(in_addr_t addr, in_port_t port, int pr=0);

    // *********** port is in HBO ******************
    inetAddr(const char *host, in_port_t port, int pr=0);

    // addr and port in NBO
    int updateAddr(in_addr_t addr, in_port_t port, int pr=0);
    int updateAddr(const struct sockaddr_in& sa);

    const std::string& host() const {return ho_;}
    const std::string&   ip() const {return ip_;}

    in_port_t  port()  const {return sa_.sin_port;}
    in_port_t &port()  {return sa_.sin_port;}
    // valid proto's:
    //   IPPROTO_*: in particular 
    //     IPPROTO_TCP, IPPROTO_UDP
    int  proto() const {return pr_;}
    int &proto() {return pr_;}

    sa_family_t      family() const {return sa_.sin_family;};

    const struct sockaddr_in *sinAddr() const {return &sa_;}

    const struct sockaddr *sockAddr() const {return (struct sockaddr *)&sa_;}

    socklen_t           sockLen() {return sizeof(sa_);}
    const struct in_addr &in_addr() const {return sa_.sin_addr;}
    in_addr_t                addr() const {return sa_.sin_addr.s_addr;}

    bool isAnyAddr()  const {return addr() == in_addr_t(INADDR_ANY);}
    bool isAnyPort()  const {return port() == in_port_t(0);}
    bool isWildcard() const {return isAnyAddr() || isAnyPort();}
    bool isValid()    const {return va_;}
    bool operator!()  const {return !va_;}

    std::string toString() const;
  private:
    // family will always be AF_INET, so no reason to include it here
    struct sockaddr_in sa_; // family, addr and port
    int                pr_; // protocol
    std::string        ho_; // (logical or host) name std::string
    std::string        ip_; // address std::string
    bool               va_; // valid or invalid address
};

  // define functions added to the wunet namespace
  //void print_ainfo(struct addrinfo* ainfo);

  // net wunet inline'd funcs
  inline std::ostream& operator<<(std::ostream&, const inetAddr&);

  inline void inetAddr::set_anyAddr()
  {
    memset((void *)&sa_, 0, sizeof(struct sockaddr_in));
    sa_.sin_family = AF_INET;
    sa_.sin_port   = 0;
    sa_.sin_addr.s_addr = htonl(INADDR_ANY);
    ho_ = "localhost";
    ip_ = "0.0.0.0";
  }

  inline inetAddr::inetAddr(int pr)
    : pr_(pr), va_(true)
  {
    set_anyAddr();
  }

  inline inetAddr::inetAddr(const struct sockaddr_in& sa, int pr)
    : pr_(pr), va_(true)
  {
    if (updateAddr(sa) < 0) {
      va_ = false;
      set_anyAddr();
      wulog(wulogNet, wulogError, "inetAddr::inetAddr: Invalid sockaddr!\n");
    }
  }

  inline inetAddr::inetAddr(in_addr_t addr, in_port_t port, int pr)
    : pr_(pr), va_(true)
  {
    if (updateAddr(addr, port) < 0) {
      va_ = false;
      set_anyAddr();
      wulog(wulogNet, wulogError, "inetAddr::inetAddr: Invalid addr (0x%08x)/port(%d)!\n", addr, (int)ntohs(port));
    }
  }

 // *************** The only case were port is in HBO
  inline inetAddr::inetAddr(const char *host, in_port_t port, int pr)
    : pr_(pr), va_(true)
  {
    memset((void *)&sa_, 0, sizeof(struct sockaddr_in));
    if (host == NULL) {
      set_anyAddr();
      sa_.sin_port = htons(port);
      return;
    }

    WUHostAddr_t wuaddr;
    if (wunet_getAddr(host, &wuaddr) < 0) {
      // rather than throwing an exception from the constructor we reset to be
      // all zero and set the invalid flag.
      va_ = false;
      set_anyAddr();
      wulog(wulogNet, wulogError, "inetAddr::inetAddr: invalid address argument (%s)\n", host);
      return;
    }

    sa_.sin_family = AF_INET;
    sa_.sin_port = htons(port); // Assume port is in hbo!
    sa_.sin_addr.s_addr = wuaddr.addr;

    ip_ = wuaddr.ip;
    ho_ = wuaddr.host;
  }

  // ***** port is in NBO *****
  inline int inetAddr::updateAddr(in_addr_t addr, in_port_t port, int pr)
  {
    pr_ = pr;
    memset((void *)&sa_, 0, sizeof(struct sockaddr_in));
    sa_.sin_family      = AF_INET; // just in cased called from constructor
    sa_.sin_port        = port;
    sa_.sin_addr.s_addr = addr;

    WUHostAddr_t wuaddr;
    wuaddr.addr = addr;
    if (wunet_getHost(&wuaddr) < 0) {
      // OK, we allow for address which do not map to symbolic/domain names
      wulog(wulogNet, wulogWarn, "inetAddr::updateAddr: Unable to convert address 0x%08x", addr);
      ip_ = "";
      ho_ = "";
    } else {
      ip_ = wuaddr.ip;
      ho_ = wuaddr.host;
    }
    return 0;
  }

  inline int inetAddr::updateAddr(const struct sockaddr_in& sa)
  {
    if (sa.sin_family != AF_INET) {
      wulog(wulogNet, wulogError, "inetAddr::updateAddr: Invalid Address Family");
      va_ = false;
      set_anyAddr();
      return -1;
    }
    return updateAddr(sa.sin_addr.s_addr, sa.sin_port);
  }

  inline std::ostream& operator<<(std::ostream& os, const inetAddr& in)
  {
    in_port_t pn = ntohs(in.port());

    struct protoent *pent = ::getprotobynumber(in.proto());
    std::string pr = pent ? pent->p_name : "unknown";
    struct servent *sent = ::getservbyport(pn, pr.c_str());
    std::string sr = sent ? sent->s_name : "unknown";

    os << "<";
    if (in.host().empty())
      os << "null";
    else
      os << in.host();
    os << ", ";
    
    if (in.ip().empty())
      os << std::hex << ntohl(in.addr());
    else
      os << in.ip();
      
    os << "/" << std::dec << ntohs(in.port())
       << "/" << std::dec << in.proto();
    os << ">";
    
    return os;
  }

  inline std::string inetAddr::toString() const
  {
    std::ostringstream ss;
    ss << *this;
    return ss.str();
  }
}

#endif
