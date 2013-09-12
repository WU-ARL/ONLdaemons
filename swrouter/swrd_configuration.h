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


#ifndef _SWRD_CONFIGURATION_H
#define _SWRD_CONFIGURATION_H

namespace swr
{
  // Do we initialize the _nic_info statically, or wait for RLI messages to configure them?
  typedef struct _nic_info
  {
    bool configured;  // Has this nic been configured?
    std::string nic;  // data0, data1, ...
    uint32_t    rate; // in kbits/s
  } nic_info;

  typedef struct _port_info 
  {
    bool configured;  // Has this port been configured?
    uint16_t  nicIndex; // index into nicTable
    std::string ip_addr;
    std::string netmask;
    std::string next_hop;
    uint16_t    vlan;
    uint32_t    rate; // in kbits/s
  } port_info;

  class Configuration_exception: public std::runtime_error
  {
    public:
      Configuration_exception(const std::string& e): std::runtime_error("Configuration exception: " + e) {}
  };

  class Configuration
  {
    public:

      Configuration() throw(Configuration_exception);
      ~Configuration() throw();

      void start_router() throw(Configuration_exception);
      void stop_router() throw();
      unsigned int router_started() throw();
      void configure_port(unsigned int port, std::string ip, std::string nexthop) throw(Configuration_exception);

/*
 *     void add_route(route_key *, route_key *, route_result *) throw(Configuration_exception);
 *     void del_route(route_key *) throw(Configuration_exception);

 *     //following methods are for converting user entered RLI values into something that means something to the system
 *     unsigned int conv_str_to_uint(std::string str) throw(Configuration_exception);
 *     unsigned int get_proto(std::string proto_str) throw(Configuration_exception);
 *     unsigned int get_tcpflags(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(Configuration_exception);
 *     unsigned int get_tcpflags_mask(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(Configuration_exception);
 *     unsigned int get_exceptions(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(Configuration_exception);
 *     unsigned int get_exceptions_mask(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(Configuration_exception);
 *     unsigned int get_pps(std::string pps_str, bool multicast) throw(Configuration_exception);
 *     unsigned int get_output_port(std::string port_str) throw(Configuration_exception);
 *     unsigned int get_output_plugin(std::string plugin_str) throw(Configuration_exception);
 *     unsigned int get_outputs(std::string port_str, std::string plugin_str) throw(Configuration_exception);
 *     //end RLI value conversion

 *     void query_filter(pfilter_key *, pfilter_result *) throw(Configuration_exception);
 *     void add_filter(pfilter_key *, pfilter_key *, unsigned int, pfilter_result *) throw(Configuration_exception);
 *     void del_filter(pfilter_key *) throw(Configuration_exception);

 *     unsigned int get_queue_quantum(unsigned int, unsigned int) throw();
 *     void set_queue_quantum(unsigned int, unsigned int, unsigned int) throw();
 *     unsigned int get_queue_threshold(unsigned int, unsigned int) throw();
 *     void set_queue_threshold(unsigned int, unsigned int, unsigned int) throw();

 *     void get_port_rates(port_rates *) throw();
 *     void set_port_rates(port_rates *) throw();
 *     unsigned int get_port_rate(unsigned int) throw(Configuration_exception); 
 *     void set_port_rate(unsigned int, unsigned int) throw(Configuration_exception); 

 *     unsigned int get_port_mac_addr_hi16(unsigned int) throw(Configuration_exception);
 *     unsigned int get_port_mac_addr_low32(unsigned int) throw(Configuration_exception);

 *     unsigned int get_port_addr(unsigned int port) throw();
 *     unsigned int get_next_hop_addr(unsigned int port) throw();
 *     void set_username(std::string un) throw();
 *     std::string get_username() throw();
*/
      
      void set_port_info(uint32_t portnum, std::string nic, uint16_t vlan, std::string ip, std::string netmask, uint32_t maxRate, uint32_t minRate );

      // Add route to main central routing table with no gateway
      void add_route_main(uint32_t prefix, uint32_t mask, uint16_t outputPort);
      // Add route to main central routing table with gateway
      void add_route_main(uint32_t prefix, uint32_t mask, uint16_t outputPort, uint32_t nextHopIpAddr);

      // Add route to per port routing table with no gateway
      void add_route_port(uint16_t portNum, std::string prefix, uint16_t length, uint16_t outputPort);
      // Add route to per port routing table with gateway
      void add_route_port(uint16_t portNum, std::string prefix, uint16_t length, uint16_t outputPort, uint32_t nextHopIpAddr);

      // Delete route from main central routing table with no gateway
      void del_route_main(std::string prefix, uint16_t length, uint16_t outputPort);
      // Delete route from main central routing table with gateway
      void del_route_main(std::string prefix, uint16_t length, uint16_t outputPort, uint32_t nextHopIpAddr);

      // Delete route from per port routing table with no gateway
      void del_route_port(uint16_t portNum, std::string prefix, uint16_t length, uint16_t outputPort);
      // Delete route from per port routing table with gateway
      void del_route_port(uint16_t portNum, std::string prefix, uint16_t length, uint16_t outputPort, uint32_t nextHopIpAddr);



      std::string addr_int2str(uint32_t addr);

    private:
      unsigned int control_address;
      std::string username;

      pthread_mutex_t conf_lock;    // anything else

      #define STOP  0
      #define START 1
      int state;

      // For now, we will limit the software router to 32 ports.
      static const uint32_t max_port = 32;
      port_info portTable[max_port];
      uint32_t numConfiguredPorts;

      // For now, we will assume a limit of 4 nics
      static const uint32_t max_nic = 4;
      nic_info nicTable[max_nic];
      uint32_t numConfiguredNics;

      int system_cmd(std::string cmd);
  };
};

#endif // _SWRD_CONFIGURATION_H
