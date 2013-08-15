/* vim: filetype=cpp 
 *
 *
 */
#ifndef _WUNET_SOCKUNIX_H
#define _WUNET_SOCKUNIX_H

#include <wuPP/error.h>
#include <wuPP/wusock.h>

namespace wunet {

  class sockUnix : public sockType {
    public:
      sockUnix() : sockType(AF_UNIX, -1, -1) {}
      sockUnix(sockUnix &oldsock) : sockType(AF_UNIX, -1, -1) { init(oldsock.sock_); }
      sockUnix(wusock_t &oldsock) : sockType(AF_UNIX, -1, -1) { init(oldsock); }
      sockUnix(int stype, int proto=0) : sockType(AF_UNIX, stype, proto) {}
      virtual ~sockUnix() {}

      //virtual void init(wusock_t &oldsock);
      // XXX void init(int stype, int proto) { sockType::init(AF_UNIX, stype, proto); }

      int bind(const struct sockaddr_un &saddr_un) { return sockType::bind((sockaddr *)&saddr_un); }
      int bind() { throw badSock(sock_.et_, "sockUnix: bind method does not support wild carding", 0); }
      int bind(const char *fname);

      sockUnix *clone() { return new sockUnix(stype(), proto()); }
  }; /* class sockUnix */
}; /* namespace wunet */
#endif /* _WUNET_SOCKUNIX_H */

