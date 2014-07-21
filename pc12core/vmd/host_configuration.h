/*
 * Copyright (c) 2009-2013 Charlie Wiseman, Jyoti Parwatikar, John DeHart 
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
 */

#ifndef _HOST_CONFIGURATION_H
#define _HOST_CONFIGURATION_H

namespace host
{
  typedef struct _port_info 
  {
    std::string nic;
    std::string ip_addr;
    std::string subnet;
    std::string next_hop;
  } port_info;

  class configuration
  {
    public:
      configuration();
      ~configuration();
   
      void set_network_info(uint32_t portnum, std::string ip, std::string subnet, std::string nexthop);
      
      bool add_route(uint16_t portnum, std::string prefix, uint32_t mask, std::string nexthop);
      bool delete_route(std::string prefix, uint32_t mask);

      std::string addr_int2str(uint32_t addr);

    private:
      static const uint32_t max_port = 1;
      port_info port[max_port+1];

      int system_cmd(std::string cmd);
  };
};

#endif // _HOST_CONFIGURATION_H
