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


#ifndef _ONL_HOSTF_CONFIGURATION_H
#define _ONL_HOSTF_CONFIGURATION_H


#include <boost/shared_ptr.hpp>
#include <netlink/route/tc.h>

namespace hostf
{
  typedef struct _port_info 
  {
    bool configured;  // Has this port been configured?
    std::string ip_addr;
    std::string netmask;
    std::string next_hop;
    //uint16_t    vlan;
    uint32_t    mark;
    uint32_t    rate; // in kbits/s
    uint32_t    max_rate; //in kbits/s
    std::string nic;  // data0, data1, ...
  } port_info;

  typedef struct _filter_info
  {
    std::string dest_prefix;
    int dest_mask;
    std::string src_prefix;
    int src_mask;
    std::string protocol;
    int dest_port;
    int src_port;
    int tcp_fin;
    int tcp_syn;
    int tcp_rst;
    int tcp_psh;
    int tcp_ack;
    int tcp_urg;
    bool unicast_drop;
    std::string output_port;
    int sampling;
    int mark;
    std::string table;
    int rule_priority;
    int qid;
  } filter_info;

  typedef boost::shared_ptr<filter_info> filter_ptr;

  class configuration_exception: public std::runtime_error
  {
    public:
      configuration_exception(const std::string& e): std::runtime_error("Configuration exception: " + e) {}
  };

  class monitor_exception: public std::runtime_error
  {
    public:
      monitor_exception(const std::string& e): std::runtime_error("Monitor exception: " + e) {}
  };

  class HostConfiguration
  {
    public:

      HostConfiguration() throw(configuration_exception);
      ~HostConfiguration() throw();

      void start() throw(configuration_exception);
      void stop() throw();
      unsigned int started() throw();
      void configure_port(unsigned int port, std::string ip, std::string maskStr, uint32_t rate, std::string nhip) throw(configuration_exception);
      
      void set_username(std::string un) throw();
      std::string get_username() throw();

/*
      unsigned int get_port_addr(unsigned int port) throw();
      unsigned int get_next_hop_addr(unsigned int port) throw();
*/
      bool add_route(uint16_t port, std::string prefix, uint32_t mask, std::string nexthop);
      bool delete_route(std::string prefix, uint32_t mask);

      unsigned int get_port_rate(unsigned int) throw(configuration_exception); 
      void set_port_rate(unsigned int, uint32_t rate) throw(configuration_exception); 

      // void set_port_info(uint32_t portnum, std::string nic, uint16_t vlan, std::string ip, std::string netmask, uint32_t maxRate, uint32_t minRate );

      // // Add route to main central routing table with no gateway
      // void add_route_main(uint32_t prefix, uint32_t mask, uint16_t outputPort) throw(configuration_exception);
      // // Add route to main central routing table with gateway
      // void add_route_main(uint32_t prefix, uint32_t mask, uint16_t outputPort, uint32_t nextHopIpAddr) throw(configuration_exception);

      // // Add route to per port routing table with no gateway
      // void add_route_port(uint16_t portNum, uint32_t prefix, uint32_t mask, uint16_t outputPort) throw(configuration_exception);
      // // Add route to per port routing table with gateway
      // void add_route_port(uint16_t portNum, uint32_t prefix, uint32_t mask, uint16_t outputPort, uint32_t nextHopIpAddr) throw(configuration_exception);

      // // Delete route from main central routing table with no gateway
      // void del_route_main(uint32_t prefix, uint32_t mask, uint16_t outputPort) throw(configuration_exception);
      // // Delete route from main central routing table with gateway
      // void del_route_main(uint32_t prefix, uint32_t mask, uint16_t outputPort, uint32_t nextHopIpAddr) throw(configuration_exception);

      // // Delete route from per port routing table with no gateway
      // void del_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t outputPort) throw(configuration_exception);
      // // Delete route from per port routing table with gateway
      // void del_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t outputPort, uint32_t nextHopIpAddr) throw(configuration_exception);

      void add_filter(filter_ptr f) throw(configuration_exception);
      void del_filter(filter_ptr f) throw(configuration_exception);
      uint32_t filter_stats(filter_ptr f, bool ispkts) throw(configuration_exception);

      //adds or changes queue parameters
      void add_queue(uint16_t port, uint32_t qid, uint32_t rate, uint32_t burst, 
		     uint32_t ceil_rate, uint32_t cburst, uint32_t mtu, bool change=false) throw(configuration_exception);
      void delete_queue(uint16_t port, uint32_t qid) throw(configuration_exception);
      //adds or changes netem queue parameters for existing htb queue <port+1>:qid
      void add_netem_queue(uint16_t port, uint32_t qid, uint32_t dtime, uint32_t jitter, uint32_t loss, 
			   uint32_t corrupt, uint32_t duplicate, bool change=false) throw(configuration_exception);
      void delete_netem_queue(uint16_t port, uint32_t qid) throw(configuration_exception);

      //Add delay on port for outgoing traffic
      void add_delay_port(uint16_t port, uint32_t dtime, uint32_t jitter) throw(configuration_exception);
      //Delete delay on port for outgoing traffic
      void delete_delay_port(uint16_t port) throw(configuration_exception);


      std::string addr_int2str(uint32_t addr);


      //MONITORING
      uint64_t read_stats_pkts(int port, int qid=0, bool use_class=false) throw(monitor_exception);
      uint64_t read_stats_bytes(int port, int qid=0, bool use_class=false) throw(monitor_exception);
      uint64_t read_stats_qlength(int port, int qid=0, bool use_class=false) throw(monitor_exception);
      uint64_t read_stats_drops(int port, int qid=0, bool use_class=false) throw(monitor_exception);
      uint64_t read_stats_backlog(int port, int qid=0, bool use_class=false) throw(monitor_exception);
      uint64_t read_link_stats_rxpkts(int port) throw(monitor_exception);
      uint64_t read_link_stats_txpkts(int port) throw(monitor_exception);
      uint64_t read_link_stats_rxbytes(int port) throw(monitor_exception);
      uint64_t read_link_stats_txbytes(int port) throw(monitor_exception);
      uint64_t read_link_stats_rxerrors(int port) throw(monitor_exception);
      uint64_t read_link_stats_txerrors(int port) throw(monitor_exception);
      uint64_t read_link_stats_rxdrops(int port) throw(monitor_exception);
      uint64_t read_link_stats_txdrops(int port) throw(monitor_exception);


    private:
      unsigned int control_address;
      std::string username;
      unsigned int delay_index;

      pthread_mutex_t conf_lock;    // anything else
      pthread_mutex_t iptable_lock;    // anything else

      std::list<filter_ptr> filters;

      #define STOP  0
      #define START 1
      int state;

      // For now, we will limit the software router to 32 ports.
      static const uint32_t max_port = 5;
      port_info portTable[max_port];
    //uint32_t numConfiguredPorts;

      void filter_command(filter_ptr f, bool isdel) throw(configuration_exception); //iptables and ip rule commands for filters

      int system_cmd(std::string cmd);
      uint64_t read_stats(int port, enum rtnl_tc_stat sid, int qid=0) throw(monitor_exception);
      uint64_t read_stats_class(int port, enum rtnl_tc_stat sid, int qid=0) throw(monitor_exception);
      uint64_t read_link_stats(int port, rtnl_link_stat_id_t sid) throw(monitor_exception);
      filter_ptr get_filter(filter_ptr f);
      filter_ptr get_filter(int port, int qid);
      int get_next_mark();
    //int get_next_priority();
      int next_mark;
    //int next_priority;
      int start_priority;
  };
};

#endif // _ONL_HOSTF_CONFIGURATION_H
