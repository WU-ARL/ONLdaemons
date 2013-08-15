/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/net.cc,v $
 * $Author: fredk $
 * $Date: 2008/09/30 18:59:10 $
 * $Revision: 1.1 $
 * $Name:  $
 *
 * File:   net.cc
 * Author: Fred Kuhns
 * Email:  fredk@arl.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  09/30/08 13:30:46 CDT
 * 
 * Copyright (C) 2008 Fred Kuhns and Washington University in St. Louis.
 * All rights reserved.
 *
 * Description:
 *
 */

#include <sstream>
#include <vector>
#include <wuPP/net.h>
#include <wuPP/stringUtils.h>
#include <stdlib.h>

namespace wunet {

  std::string num2ip(struct in_addr ipaddr)
  {
    return num2ip(ipaddr.s_addr);
  }

  std::string num2ip(uint32_t ipaddr)
  {
    std::ostringstream ss;
    ipaddr = ntohl(ipaddr);
    ss << ((ipaddr >> 24) & 0xFF) // MSB
       << "." << ((ipaddr >> 16) & 0xFF)
       << "." << ((ipaddr >>  8) & 0xFF)
       << "." << (ipaddr & 0xFF); // LSB
    return ss.str();
  }

  uint32_t ip2num(const std::string &ipstr)
  {
    std::vector< std::string > bytes;
    std::vector< std::string >::const_iterator iter;
    uint32_t ipa = 0;

    if (string2flds(bytes, ipstr, ".") != 4)
      throw netErr(wupp::libError, "ip2num: Invalid IP address format " + ipstr);

    ipa = (((uint32_t)strtol(bytes[0].c_str(), NULL, 0) << 24) & 0xFF000000) | // MSB
          (((uint32_t)strtol(bytes[1].c_str(), NULL, 0) << 16) & 0x00FF0000) |
          (((uint32_t)strtol(bytes[2].c_str(), NULL, 0) <<  8) & 0x0000FF00) |
           ((uint32_t)strtol(bytes[3].c_str(), NULL, 0) && 0xFF);              // LSB
    return ipa;
  }

};
