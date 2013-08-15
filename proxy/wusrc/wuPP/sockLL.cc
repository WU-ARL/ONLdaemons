

#include <wuPP/sockLL.h>

#ifndef __CYGWIN__

namespace wunet {
  void sockLL::init(wusock_t &oldsock)
  {
    if (oldsock.st_ != AF_UNIX) {
      sock_.et_ = wupp::paramError;
      sock_.einfo_.el_ = wun_el_Err;
      throw badSock(sock_.et_, "sockLL::sockLL: Init method passed Bad Domain socket", 0);
    }
    sockType::init(oldsock);
  }
  int sockLL::bind(uint16_t pr, int ifindx, uint8_t *addr)
  {
    struct sockaddr_ll sll;
    sok_init_eth(&sll, pr, ifindx, addr);
    return bind(sll);
  }
}; /* namespace wunet */

#endif
