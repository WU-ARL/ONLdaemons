/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/addr.cc,v $
 * $Author: fredk $
 * $Date: 2006/12/08 20:34:28 $
 * $Revision: 1.2 $
 * $Name:  $
 *
 * File:         addr.cc
 * Created:      01/05/2005 04:32:38 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#include <wulib/wunet.h>
#include <vector>

//#include <iostream>
//#include <sstream>

//#include <cstdio>
//#include <cstdlib>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <wulib/wulog.h>
//#include "net.h"
//#include "addr.h"

#if 0
void wunet::print_ainfo(struct addrinfo* ainfo)
{
  if (ainfo == NULL) {
    std::cout << "ainfo == NULL\n";
    return;
  }

  std::cout << "Addr Info:\n\tFlags:";
  if (ainfo->ai_flags & AI_PASSIVE)
    std::cout << " Passive";
  if (ainfo->ai_flags & AI_CANONNAME)
    std::cout << " CanonName";
  if (ainfo->ai_flags & AI_NUMERICHOST)
    std::cout << " Numeric";
  if (ainfo->ai_flags & AI_V4MAPPED)
    std::cout << " V4Mapped";
  if (ainfo->ai_flags & AI_ALL)
    std::cout << " All";
  if (ainfo->ai_flags & AI_ADDRCONFIG)
    std::cout << " AddrConfig";

  std::cout << "\n\tFamily: ";
  if (ainfo->ai_family == AF_INET)
    std::cout << "AF_INET";
  else if (ainfo->ai_family == AF_INET6)
    std::cout << "AF_INET6";
  else
    std::cout << ainfo->ai_family;

  std::cout << "\n\tSockType: " << ainfo->ai_socktype;

  std::cout << "\n\tProtocol: ";
  if (ainfo->ai_protocol == IPPROTO_IP)
    std::cout << "IP";
  else if (ainfo->ai_protocol == IPPROTO_TCP)
    std::cout << "TCP";
  else if (ainfo->ai_protocol == IPPROTO_UDP)
    std::cout << "UDP";
  else
    std::cout << ainfo->ai_protocol;

  struct sockaddr_in *addr = (struct sockaddr_in *)ainfo->ai_addr;
  if (addr == NULL) {
    std::cout << "\n\tSockaddr == NULL\n\t";
  } else {
    std::cout << "\n\tAddrlen: " << ainfo->ai_addrlen
         << "\n\tAddr: Family "    << addr->sin_family
         << ", Port " << ntohs(addr->sin_port)
         << ", Addr " << std::hex << ntohl(addr->sin_addr.s_addr)
         << "\n\t";
  }

  if (ainfo->ai_canonname)
    std::cout << "Host " << ainfo->ai_canonname;
  else
    std::cout << "No host name returned";

  std::cout << "\n";
  return;
}


// limited (OK no) support for getaddrinfo on older ssystems and cygwin
std::vector<wunet::inetAddr*>
wunet::inetAddr::make_inetAddrs(const char *host, in_port_t port, int pr)
{
  struct addrinfo hints;

  memset((void *)&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_protocol = pr;
  if (host == NULL) {
    // INADDR_ANY
    hints.ai_flags    = AI_PASSIVE;
  } else if (isdigit((int)*host)) {
    // xxx.xxx.xxx.xxx
    hints.ai_flags    = AI_CANONNAME | AI_NUMERICHOST;
  } else {
    // host name
    hints.ai_flags    = AI_CANONNAME;
  }

  struct addrinfo *ainfo;
  int res = getaddrinfo(host, NULL, &hints, &ainfo);
  if (res != 0) {
    throw netErr(addrError, "make_inetAddrs: Unable to convert address", res, gai_strerror(res));
  }
  // there is atleast one valid address returned.
  std::vector<inetAddr*> addrs;

  for (struct addrinfo *ptr = ainfo; ptr; ptr=ptr->ai_next) {
    //std::cout << "Adding address ...\n";
    //print_ainfo(ptr);
    struct sockaddr_in *sptr = (struct sockaddr_in *)ptr->ai_addr;
    sptr->sin_port = htons(port);
    addrs.push_back(new inetAddr(*sptr, ptr->ai_protocol));
  }
  freeaddrinfo(ainfo);

  return addrs;
}
#endif
