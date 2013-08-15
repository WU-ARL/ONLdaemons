/* vim: filetype=cpp
 *
 */
#ifndef _WUNET_SOCKLL_H
#define _WUNET_SOCKLL_H

#ifndef __CYGWIN__

#include <wuPP/error.h>
#include <wuPP/wusock.h>

namespace wunet {

  class sockLL : public sockType {
    public:
      sockLL() : sockType(PF_PACKET, -1, -1) {};
      sockLL(sockLL &oldsock) : sockType(PF_PACKET, -1, -1) {init(oldsock.sock_);}
      sockLL(wusock_t &oldsock) : sockType(PF_PACKET, -1, -1) {init(oldsock);}
      sockLL(int stype, int proto) : sockType(PF_PACKET, stype, proto) {}
      virtual ~sockLL() {}
      void init(wusock_t &oldsock);

      sockLL *clone() { return new sockLL(stype(), proto()); }

      int bind(const struct sockaddr_ll &ssl) { return sockType::bind((struct sockaddr *)&ssl); }
      int bind() { throw badSock(wupp::opError, "no default bind for sockLL"); }
      int bind(uint16_t pr, int ifindx, uint8_t *addr);
      sockLL *accept() { throw badSock(wupp::opError, "no accept for sockLL"); }
    private:
      sockLL& operator=(sockLL&);//no assignment operator
      sockLL(const sockLL &oldsock);


  }; /* class sockLL */
}; /* namespace wunet */
#endif /* ! __CYGWIN__ */
#endif /* ! _WUNET_SOCKLL_H */
