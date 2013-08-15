#include <wuPP/sockInet.h>

namespace wunet {

  int sockInet::proto2stype(int proto)
  {
    int stype = SOCK_RAW;
    switch (proto) {
      case IPPROTO_IP:
        stype = SOCK_RAW;
        break;
      case IPPROTO_TCP:
        stype = SOCK_STREAM;
        break;
      case IPPROTO_UDP:
        stype = SOCK_DGRAM;
        break;
    }
    // IPPROTO_IP IPPROTO_HOPOPTS IPPROTO_ICMP IPPROTO_IGMP IPPROTO_IPIP
    // IPPROTO_TCP IPPROTO_EGP IPPROTO_PUP IPPROTO_UDP IPPROTO_IDP
    // IPPROTO_TP IPPROTO_IPV6 IPPROTO_ROUTING IPPROTO_FRAGMENT IPPROTO_RSVP
    // IPPROTO_GRE IPPROTO_ESP IPPROTO_AH IPPROTO_ICMPV6 IPPROTO_NONE
    // IPPROTO_DSTOPTS IPPROTO_MTP IPPROTO_ENCAP IPPROTO_PIM IPPROTO_COMP
    // IPPROTO_SCTP IPPROTO_RAW IPPROTO_MAX SOCK_STREAM SOCK_DGRAM  SOCK_RAW
    // SOCK_RDM SOCK_SEQPACKET  SOCK_PACKET
    return stype;
  }

  // void sockInet::init(wusock_t &oldsock)
  // {
  //   if (oldsock.st_ != AF_INET) {
  //     sock_.et_ = wupp::paramError;
  //     sock_.einfo_.el_ = wun_el_Err;
  //     throw badSock(sock_.et_, "sockInet: Init method passed non-INET socket", 0);
  //   }
  //   sockType::init(oldsock);
  // }
  struct sockaddr_in &sockInet::self(struct sockaddr_in &sin)
  {
    sok_update_saddr(&sock_, (struct sockaddr *)&sin);
    return sin;
  }
  struct sockaddr_in &sockInet::peer(struct sockaddr_in &sin)
  {
    sok_get_peer(&sock_, (struct sockaddr *)&sin);
    return sin;
  }
  inetAddr sockInet::self() {struct sockaddr_in sin; return inetAddr(self(sin));}
  inetAddr sockInet::peer() {struct sockaddr_in sin; return inetAddr(peer(sin));}

}; /* namespace wunet */
