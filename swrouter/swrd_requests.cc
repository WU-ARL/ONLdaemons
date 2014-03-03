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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "shared.h"

#include "swrd_types.h"
#include "swrd_configuration.h"
//#include "swrd_monitor.h"
#include "swrd_globals.h"
#include "swrd_requests.h"

using namespace swr;

configure_node_req::configure_node_req(uint8_t *mbuf, uint32_t size): configure_node(mbuf, size)
{
}

configure_node_req::~configure_node_req()
{
}

bool
configure_node_req::handle()
{
  write_log("configure_node_req::handle()");

  NCCP_StatusType status = NCCP_Status_Fine;

  try
  {
    uint32_t bw_kbits = node_conf.getBandwidth() * 1000;//rates are given in Mbits/s need to convert to kbits/s
    configuration->set_username(exp.getExpInfo().getUserName());
    configuration->configure_port(node_conf.getPort(), node_conf.getRealPort(), node_conf.getVLan(), node_conf.getIPAddr(), node_conf.getSubnet(), bw_kbits);
  }
  catch(std::exception& e)
  { 
    write_log("configure_node_req::failed exception(" + std::string(e.what()) + ")");
    status = NCCP_Status_Failed;
  }

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  return true;
}

add_route_main_req::add_route_main_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_route_main_req::~add_route_main_req()
{
}

bool
add_route_main_req::handle()
{
  rli_response* rliresp;

  write_log("add_route_main_req::handle(): route(prefix,mask,output port, nexthop ip)=(" + int2str(prefix) + "," + int2str(mask) + "," + int2str(port) + "," + int2str(nexthop_ip) + ")");

  try
  {
    configuration->add_route_main(prefix, mask, output_port, nexthop_ip);

    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("add_route_main_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
add_route_main_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
  mask = params[1].getInt();
  output_port = params[2].getInt();
  nexthop_ip = params[3].getInt();
}

add_route_port_req::add_route_port_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_route_port_req::~add_route_port_req()
{
}

bool
add_route_port_req::handle()
{
  rli_response* rliresp;

  write_log("add_route_port_req::handle(): route(port, prefix,mask,output port, nexthop ip)=(" +int2str(port) + "," + int2str(prefix) + "," + int2str(mask) + "," + int2str(output_port) + "," + int2str(nexthop_ip) + ")");

  try
  {
    configuration->add_route_port(port, prefix, mask, output_port, nexthop_ip);

    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("add_route_port_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
add_route_port_req::parse()
{
  rli_request::parse();

  // port is already part of rli_request
  prefix = params[0].getInt();
  mask = params[1].getInt();
  output_port = params[2].getInt();
  nexthop_ip = params[3].getInt();
}
 
del_route_main_req::del_route_main_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

del_route_main_req::~del_route_main_req()
{
}

bool
del_route_main_req::handle()
{
  rli_response* rliresp;

  write_log("del_route_main_req::handle(): route(prefix,mask,output port, nexthop ip)=(" + int2str(prefix) + "," + int2str(mask) + "," + int2str(port) + "," + int2str(nexthop_ip) + ")");

  try
  {
    configuration->del_route_main(prefix, mask, output_port, nexthop_ip);

    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("del_route_main_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
del_route_main_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
  mask = params[1].getInt();
  output_port = params[2].getInt();
  nexthop_ip = params[3].getInt();
}

del_route_port_req::del_route_port_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

del_route_port_req::~del_route_port_req()
{
}

bool
del_route_port_req::handle()
{
  rli_response* rliresp;

  write_log("del_route_port_req::handle(): route(port, prefix,mask,output port, nexthop ip)=(" +int2str(port) + "," + int2str(prefix) + "," + int2str(mask) + "," + int2str(output_port) + "," + int2str(nexthop_ip) + ")");

  try
  {
    configuration->del_route_port(port, prefix, mask, output_port, nexthop_ip);

    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("del_route_port_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
del_route_port_req::parse()
{
  rli_request::parse();

  // port is already part of rli_request
  prefix = params[0].getInt();
  mask = params[1].getInt();
  output_port = params[2].getInt();
  nexthop_ip = params[3].getInt();
}
 
/*
add_filter_req::add_filter_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_filter_req::~add_filter_req()
{
}

bool
add_filter_req::handle()
{
  rli_response* rliresp;

  unsigned int protocol_val;
  unsigned int tcp_flags;
  unsigned int tcp_flags_mask;
  unsigned int exceptions;
  unsigned int exceptions_mask;
  unsigned int pps_val;
  unsigned int output_port_val;
  unsigned int output_plugin_val;
  unsigned int outputs_val;
  try
  {
    protocol_val = configuration->get_proto(protocol);
    tcp_flags = configuration->get_tcpflags(tcp_fin, tcp_syn, tcp_rst, tcp_psh, tcp_ack, tcp_urg);
    tcp_flags_mask = configuration->get_tcpflags_mask(tcp_fin, tcp_syn, tcp_rst, tcp_psh, tcp_ack, tcp_urg);
    exceptions = configuration->get_exceptions(exception_nonip, exception_arp, exception_ipopt, exception_ttl);
    exceptions_mask = configuration->get_exceptions_mask(exception_nonip, exception_arp, exception_ipopt, exception_ttl);
    pps_val = configuration->get_pps(port_plugin_selection, multicast);
    if(!multicast)
    {
      output_port_val = configuration->get_output_port(output_port);
      output_plugin_val = configuration->get_output_plugin(output_plugin);
    }
    else
    {
      outputs_val = configuration->get_outputs(output_port, output_plugin);
    }
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("add_filter_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    rliresp->send();
    delete rliresp;
    return true;
  }

  pfilter_key pkey;
  pfilter_key pmask;
  pfilter_result pres;
  
  pkey.daddr = dest_prefix;
  pkey.saddr = src_prefix;
  pkey.ptag = plugin_tag;
  pkey.port = port;
  pkey.proto = protocol_val;
  pkey.dport = dest_port;
  pkey.sport = src_port;
  pkey.exceptions = exceptions;
  pkey.tcp_flags = tcp_flags;
  pkey.res = 0;
  
  write_log("add_filter_req::handle(): primary filter pkey(daddr,saddr,ptag,port,proto,dport,sport,exceptions,tcp_flags)=(" + int2str(pkey.daddr) + "," + int2str(pkey.saddr) + "," + int2str(pkey.ptag) + "," + int2str(pkey.port) + "," + int2str(pkey.proto) + "," + int2str(pkey.dport) + "," + int2str(pkey.sport) + "," + int2str(pkey.exceptions) + "," + int2str(pkey.tcp_flags) + ")");
  
  pmask.daddr = (~0 << (32-dest_mask));
  pmask.saddr = (~0 << (32-src_mask));;
  if(plugin_tag == WILDCARD_VALUE) { pmask.ptag = 0; }
  else { pmask.ptag = 0x1f; }
  pmask.port = 0x7;
  if(protocol_val == WILDCARD_VALUE) { pmask.proto = 0; }
  else { pmask.proto = 0xff; }
  if(dest_port == WILDCARD_VALUE) { pmask.dport = 0; }
  else { pmask.dport = 0xffff; }
  if(src_port == WILDCARD_VALUE) { pmask.sport = 0; }
    else { pmask.sport = 0xffff; }
  pmask.exceptions = exceptions_mask;
  pmask.tcp_flags = tcp_flags_mask;;
  pmask.res = 0;
  
  write_log("add_filter_req::handle(): primary filter pmask(daddr,saddr,ptag,port,proto,dport,sport,exceptions,tcp_flags)=(" + int2str(pmask.daddr) + "," + int2str(pmask.saddr) + "," + int2str(pmask.ptag) + "," + int2str(pmask.port) + "," + int2str(pmask.proto) + "," + int2str(pmask.dport) + "," + int2str(pmask.sport) + "," + int2str(pmask.exceptions) + "," + int2str(pmask.tcp_flags) + ")");

  pres.entry_valid = 1;
  pres.stats_index = stats_index;
  pres.qid = qid;
  pres.nh_hi16 = 0;
  
  pres.ip_mc_valid = multicast;
  if(pres.ip_mc_valid == 0)  // unicast
    {
      pres.nh_low32 = configuration->get_next_hop_addr(output_port_val);
      if(pres.nh_low32 == 0)
	{
	  pres.nh_ip_valid = 0;
	}
      else
	{
	  pres.nh_ip_valid = 1;
	}
      pres.nh_mac_valid = 0;
      
      pres.uc_mc_bits = (unicast_drop << 7) | (pps_val << 6) | (output_port_val << 3) | (output_plugin_val);
    }
  else // multicast
    {
      pres.nh_low32 = 0;
      pres.nh_ip_valid = 0;
      pres.nh_mac_valid = 0;
      
      pres.uc_mc_bits = (pps_val << 11) | (outputs_val << 1);
    }

  write_log("add_filter_req::handle(): primary filter pres(mc_valid,uc_mc_bits,nh_low32,qid,stats_index)=(" + int2str(pres.ip_mc_valid) + "," + int2str(pres.uc_mc_bits) + "," + int2str(pres.nh_low32) + "," + int2str(pres.qid) + "," + int2str(pres.stats_index) + ")");
  
  try
    {
      configuration->add_filter(&pkey, &pmask, priority, &pres);
      
      rliresp = new rli_response(this, NCCP_Status_Fine);
    }
  catch(std::exception& e)
    {
      std::string msg = e.what();
      write_log("add_filter_req::handle(): got exception: " + msg);
      rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    }
   
  rliresp->send();
  delete rliresp;
  
  return true;
}

void
add_filter_req::parse()
{
  rli_request::parse();

  dest_prefix = params[1].getInt();
  dest_mask = params[2].getInt();
  src_prefix = params[3].getInt();
  src_mask = params[4].getInt();
  plugin_tag = params[5].getInt();
  protocol = params[6].getString();
  dest_port = params[7].getInt();
  src_port = params[8].getInt();
  exception_nonip = params[9].getInt();
  exception_arp = params[10].getInt();
  exception_ipopt = params[11].getInt();
  exception_ttl = params[12].getInt();
  tcp_fin = params[13].getInt();
  tcp_syn = params[14].getInt();
  tcp_rst = params[15].getInt();
  tcp_psh = params[16].getInt();
  tcp_ack = params[17].getInt();
  tcp_urg = params[18].getInt();
  qid = params[19].getInt();
  stats_index = params[20].getInt();
  multicast = params[21].getBool();
  port_plugin_selection = params[22].getString();
  unicast_drop = params[23].getBool();
  output_port = params[24].getString();
  output_plugin = params[25].getString();
  sampling_bits = params[26].getInt();
  priority = params[27].getInt();
}
 
delete_filter_req::delete_filter_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

delete_filter_req::~delete_filter_req()
{
}

bool
delete_filter_req::handle()
{
  rli_response* rliresp;

  unsigned int protocol_val;
  unsigned int tcp_flags;
  unsigned int exceptions;
  try
  {
    protocol_val = configuration->get_proto(protocol);
    tcp_flags = configuration->get_tcpflags(tcp_fin, tcp_syn, tcp_rst, tcp_psh, tcp_ack, tcp_urg);
    exceptions = configuration->get_exceptions(exception_nonip, exception_arp, exception_ipopt, exception_ttl);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("delete_filter_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    rliresp->send();
    delete rliresp;
    return true;
  }

  pfilter_key pkey;
  
  pkey.daddr = dest_prefix;
  pkey.saddr = src_prefix;
  pkey.ptag = plugin_tag;
  pkey.port = port;
  pkey.proto = protocol_val;
  pkey.dport = dest_port;
  pkey.sport = src_port;
  pkey.exceptions = exceptions;
  pkey.tcp_flags = tcp_flags;
  pkey.res = 0;
  
  write_log("delete_filter_req::handle(): primary filter pkey(daddr,saddr,ptag,port,proto,dport,sport,exceptions,tcp_flags)=(" + int2str(pkey.daddr) + "," + int2str(pkey.saddr) + "," + int2str(pkey.ptag) + "," + int2str(pkey.port) + "," + int2str(pkey.proto) + "," + int2str(pkey.dport) + "," + int2str(pkey.sport) + "," + int2str(pkey.exceptions) + "," + int2str(pkey.tcp_flags) + ")");
  
  try
    {
      configuration->del_pfilter(&pkey);
      
      rliresp = new rli_response(this, NCCP_Status_Fine);
    }
  catch(std::exception& e)
    {
      std::string msg = e.what();
      write_log("delete_filter_req::handle(): got exception: " + msg);
      rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    }

  rliresp->send();
  delete rliresp;

  return true;
}

void
delete_filter_req::parse()
{
  rli_request::parse();

  dest_prefix = params[1].getInt();
  src_prefix = params[3].getInt();
  plugin_tag = params[5].getInt();
  protocol = params[6].getString();
  dest_port = params[7].getInt();
  src_port = params[8].getInt();
  exception_nonip = params[9].getInt();
  exception_arp = params[10].getInt();
  exception_ipopt = params[11].getInt();
  exception_ttl = params[12].getInt();
  tcp_fin = params[13].getInt();
  tcp_syn = params[14].getInt();
  tcp_rst = params[15].getInt();
  tcp_psh = params[16].getInt();
  tcp_ack = params[17].getInt();
  tcp_urg = params[18].getInt();
}
*/
 
/*
set_queue_params_req::set_queue_params_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

set_queue_params_req::~set_queue_params_req()
{
}

bool
set_queue_params_req::handle()
{
  rli_response* rliresp;
  try
  {
    configuration->set_queue_quantum(port, qid, quantum);
    configuration->set_queue_threshold(port, qid, threshold);
    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("set_queue_params_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
set_queue_params_req::parse()
{
  rli_request::parse();

  qid = params[0].getInt();
  threshold = params[1].getInt();
  quantum = params[2].getInt();
}
*/
 
set_port_rate_req::set_port_rate_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

set_port_rate_req::~set_port_rate_req()
{
}

bool
set_port_rate_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t bw_kbits = rate * 1000;//convert rate from Mbits/s to kbits/s
    configuration->set_port_rate(port, bw_kbits);
    //cgw, how does this fit into the general hw model?
    uint32_t actual_rate = (uint32_t)(configuration->get_port_rate(port)/1000);
    rliresp = new rli_response(this, NCCP_Status_Fine, actual_rate);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("set_port_rate_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
set_port_rate_req::parse()
{
  rli_request::parse();

  rate = params[0].getInt();
}


//MONITORING Requests
/*
 
get_rx_pkt_req::get_rx_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_rx_pkt_req::~get_rx_pkt_req()
{
}

bool
get_rx_pkt_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t reg_counter = (port*2) + 1;
    uint32_t val = monitor->read_stats_register(reg_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_rx_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_rx_pkt_req::parse()
{
  rli_request::parse();
}
 
get_rx_byte_req::get_rx_byte_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_rx_byte_req::~get_rx_byte_req()
{
}

bool
get_rx_byte_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t reg_counter = port*2;
    uint32_t val = monitor->read_stats_register(reg_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_rx_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_rx_byte_req::parse()
{
  rli_request::parse();
}
 
get_tx_pkt_req::get_tx_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_tx_pkt_req::~get_tx_pkt_req()
{
}

bool
get_tx_pkt_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t reg_counter = (port*2) + 11;
    uint32_t val = monitor->read_stats_register(reg_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_tx_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_tx_pkt_req::parse()
{
  rli_request::parse();
}
 
get_tx_byte_req::get_tx_byte_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_tx_byte_req::~get_tx_byte_req()
{
}

bool
get_tx_byte_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t reg_counter = (port*2) + 10;
    uint32_t val = monitor->read_stats_register(reg_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
    //write_log("get_tx_byte_req::handle(): rliresp->time_stamp " + int2str(rliresp->getTimeStamp()));
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_tx_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_tx_byte_req::parse()
{
  rli_request::parse();
}
 
get_reg_pkt_req::get_reg_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_reg_pkt_req::~get_reg_pkt_req()
{
}

bool
get_reg_pkt_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = monitor->read_stats_register(stats_index);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_reg_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_reg_pkt_req::parse()
{
  rli_request::parse();
  
  stats_index = params[0].getInt();
}
 
get_reg_byte_req::get_reg_byte_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_reg_byte_req::~get_reg_byte_req()
{
}

bool
get_reg_byte_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = monitor->read_stats_register(stats_index);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_reg_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_reg_byte_req::parse()
{
  rli_request::parse();
  
  stats_index = params[0].getInt();
}
 
get_queue_len_req::get_queue_len_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_queue_len_req::~get_queue_len_req()
{
}

bool
get_queue_len_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = monitor->read_queue_length(port, qid);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_queue_len_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_queue_len_req::parse()
{
  rli_request::parse();

  qid = params[0].getInt();
}
*/
