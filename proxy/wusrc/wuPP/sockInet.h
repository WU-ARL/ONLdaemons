

#ifndef _WUNET_SOCKINET_H
#define _WUNET_SOCKINET_H


#include <wuPP/wusock.h>

namespace wunet {

  class sockInet : public sockType {
    public:
      //Create new objects
      // Add:
      //   sockInet(const inetAddr& myaddr, int stype=0);
      sockInet() : sockType(AF_INET, -1, -1) {}
      sockInet(sockInet &sock) : sockType(sock) {}
      sockInet(int stype, int proto=0) : sockType(AF_INET, stype, proto) { }
      sockInet(wusock_t &oldsock) { init(oldsock); }
      virtual ~sockInet() {}

      int proto2stype(int proto);

      //void init(wusock_t &oldsock);
      // XXX void init(int stype, int proto) { sockType::init(AF_INET, stype, proto); }

      int bind(const struct sockaddr_in &sin) { return sockType::bind((struct sockaddr *)&sin); }
      int bind(const inetAddr &addr) {return sockType::bind(addr.sockAddr());}
      int bind(uint32_t ipaddr, uint16_t port)
      {
        struct sockaddr_in saddr;
        sok_init_sin2(&saddr, ipaddr, port);
        return sockType::bind((struct sockaddr *)&saddr);
      }
      int bind(struct in_addr &ipaddr, uint16_t port) { return bind(ipaddr.s_addr, port); }
      int bind() { return bind((uint32_t)INADDR_ANY, (uint16_t)0); }


      int connect(const inetAddr &addr) {return sockType::connect(addr.sockAddr());}
      int connect(const struct sockaddr_in &sin) {return sockType::connect((struct sockaddr *)&sin); }
      int connect(struct in_addr &ipaddr, uint16_t port)
      {
        return connect(ipaddr.s_addr, port);
      }
      int connect(uint32_t ipaddr, uint16_t port)
      {
        struct sockaddr_in saddr;
        sok_init_sin2(&saddr, ipaddr, port);
        return sockType::connect((struct sockaddr *)&saddr);
      }

      sockInet *clone()
      {
        return new sockInet(stype(), proto());
      }
      sockInet *accept()
      {
        sockInet *sock = clone();
        sockType::accept(*sock);
        return sock;
      }

      struct sockaddr_in &self(struct sockaddr_in &sin);
      struct sockaddr_in &peer(struct sockaddr_in &sin);
      inetAddr self();
      inetAddr peer();

  }; /* class sockInet */

}; /* namespace wunet */
#endif /* _WUNET_SOCKINET_H */ 
