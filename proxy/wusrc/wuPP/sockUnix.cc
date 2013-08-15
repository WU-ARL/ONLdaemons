
#include <wuPP/sockUnix.h>

namespace wunet {
  // void sockUnix::init(wusock_t &oldsock)
  // {
  //   if (oldsock.st_ != AF_UNIX) {
  //     sock_.et_ = wupp::paramError;
  //     sock_.einfo_.el_ = wun_el_Err;
  //     throw badSock(sock_.et_, "sockUnix: Init method passed non-Unix Domain socket", 0);
  //   }
  //   sockType::init(oldsock);
  // }
  int sockUnix::bind(const char *fname)
  {
    struct sockaddr_un saddr;
    sok_init_sun(&saddr, fname);
    return bind(saddr);
  }
}; /* namespace wunet */
