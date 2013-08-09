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
 *
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

#include "nprd_types.h"
#include "nprd_configuration.h"
#include "nprd_monitor.h"
#include "nprd_pluginmessage.h"
#include "nprd_pluginconn.h"
#include "nprd_datapathmessage.h"
#include "nprd_datapathconn.h"
#include "nprd_plugincontrol.h"
#include "nprd_ipv4.h"
#include "nprd_arp.h"
#include "nprd_globals.h"
#include "nprd_requests.h"
#include "nprd_responses.h"

using namespace npr;

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
    configuration->set_username(exp.getExpInfo().getUserName());
    configuration->configure_port(node_conf.getPort(), node_conf.getIPAddr(), node_conf.getNHIPAddr());
  }
  catch(std::exception& e)
  { 
    status = NCCP_Status_Failed;
  }

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  return true;
}

add_route_req::add_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_route_req::~add_route_req()
{
}

bool
add_route_req::handle()
{
  rli_response* rliresp;

  route_key rkey;
  route_key rmask;
  route_result rres;

  rkey.daddr = prefix;
  rkey.saddr = 0;
  rkey.ptag = 0;
  rkey.port = port;

  write_log("add_route_req::handle(): rkey(daddr,saddr,ptag,port)=(" + int2str(rkey.daddr) + "," + int2str(rkey.saddr) + "," + int2str(rkey.ptag) + "," + int2str(rkey.port) + ")");
  
  rmask.daddr = (~0 << (32-mask));
  rmask.saddr = 0;
  rmask.ptag = 0;
  rmask.port = 0x7;

  write_log("add_route_req::handle(): rmask(daddr,saddr,ptag,port)=(" + int2str(rmask.daddr) + "," + int2str(rmask.saddr) + "," + int2str(rmask.ptag) + "," + int2str(rmask.port) + ")");

  rres.entry_valid = 1;
  rres.nh_ip_valid = 1;
  rres.nh_mac_valid = 0;
  rres.ip_mc_valid = 0;
  rres.uc_mc_bits = (output_port << 3);
  rres.qid = 0;
  rres.nh_hi16 = 0;
  rres.nh_low32 = nexthop_ip;
  if(rres.nh_low32 == 0)
  {
    rres.nh_ip_valid = 0;
  }
  rres.stats_index = stats_index;

  write_log("add_route_req::handle(): rres(uc_mc_bits,nh_low32,stats_index)=(" + int2str(rres.uc_mc_bits) + "," + int2str(rres.nh_low32) + "," + int2str(rres.stats_index) + ")");

  try
  {
    configuration->add_route(&rkey, &rmask, &rres);

    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("add_route_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
add_route_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
  mask = params[1].getInt();
  output_port = params[2].getInt();
  nexthop_ip = params[3].getInt();
  stats_index = params[4].getInt();
}
 
delete_route_req::delete_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

delete_route_req::~delete_route_req()
{
}

bool
delete_route_req::handle()
{
  rli_response* rliresp;

  route_key rkey;

  rkey.daddr = prefix;
  rkey.saddr = 0;
  rkey.ptag = 0;
  rkey.port = port;

  write_log("delete_route_req::handle(): rkey(daddr,saddr,ptag,port)=(" + int2str(rkey.daddr) + "," + int2str(rkey.saddr) + "," + int2str(rkey.ptag) + "," + int2str(rkey.port) + ")");

  try
  {
    configuration->del_route(&rkey);

    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("delete_route_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
delete_route_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
}
 
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

  if(!aux)
  {
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
      configuration->add_pfilter(&pkey, &pmask, priority, &pres);

      rliresp = new rli_response(this, NCCP_Status_Fine);
    }
    catch(std::exception& e)
    {
      std::string msg = e.what();
      write_log("add_filter_req::handle(): got exception: " + msg);
      rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    }
  }
  else // auxiliary filter
  {
    afilter_key akey;
    afilter_key amask;
    afilter_result ares;
  
    if(output_port_val == FILTER_USE_ROUTE)
    {
      std::string msg = "auxiliary filters do not support use route";
      write_log("add_filter_req::handle(): exception: " + msg);
      rliresp = new rli_response(this, NCCP_Status_Failed, msg);
      rliresp->send();
      delete rliresp;
      return true;
    }

    akey.daddr = dest_prefix;
    akey.saddr = src_prefix;
    akey.ptag = plugin_tag;
    akey.port = port;
    akey.proto = protocol_val;
    akey.dport = dest_port;
    akey.sport = src_port;
    akey.exceptions = exceptions;
    akey.tcp_flags = tcp_flags;
    akey.res = 0;

    write_log("add_filter_req::handle(): auxiliary filter akey(daddr,saddr,ptag,port,proto,dport,sport,exceptions,tcp_flags)=(" + int2str(akey.daddr) + "," + int2str(akey.saddr) + "," + int2str(akey.ptag) + "," + int2str(akey.port) + "," + int2str(akey.proto) + "," + int2str(akey.dport) + "," + int2str(akey.sport) + "," + int2str(akey.exceptions) + "," + int2str(akey.tcp_flags) + ")");
   
    amask.daddr = (~0 << (32-dest_mask));
    amask.saddr = (~0 << (32-src_mask));;
    if(plugin_tag == WILDCARD_VALUE) { amask.ptag = 0; }
    else { amask.ptag = 0x1f; }
    amask.port = 0x7;
    if(protocol_val == WILDCARD_VALUE) { amask.proto = 0; }
    else { amask.proto = 0xff; }
    if(dest_port == WILDCARD_VALUE) { amask.dport = 0; }
    else { amask.dport = 0xffff; }
    if(src_port == WILDCARD_VALUE) { amask.sport = 0; }
    else { amask.sport = 0xffff; }
    amask.exceptions = exceptions_mask;
    amask.tcp_flags = tcp_flags_mask;;
    amask.res = 0;

    write_log("add_filter_req::handle(): auxiliary filter amask(daddr,saddr,ptag,port,proto,dport,sport,exceptions,tcp_flags)=(" + int2str(amask.daddr) + "," + int2str(amask.saddr) + "," + int2str(amask.ptag) + "," + int2str(amask.port) + "," + int2str(amask.proto) + "," + int2str(amask.dport) + "," + int2str(amask.sport) + "," + int2str(amask.exceptions) + "," + int2str(amask.tcp_flags) + ")");

    ares.entry_valid = 1;
    ares.nh_mac_valid = 0;
    ares.ip_mc_valid = 0;
    ares.sampling = sampling_bits;
    ares.stats_index = stats_index;
    ares.res = 0;
    ares.qid = qid;
    ares.nh_hi16 = 0;
    ares.nh_low32 = configuration->get_next_hop_addr(output_port_val);
    if(ares.nh_low32 == 0) { ares.nh_ip_valid = 0; }
    else { ares.nh_ip_valid = 1; }
    ares.drop = unicast_drop;
    ares.pps = pps_val;
    ares.out_port = output_port_val;
    ares.out_plugin = output_plugin_val;

    write_log("add_filter_req::handle(): auxiliary filter ares(drop,pps,out_port,out_plugin,nh_low32,qid,stats_index,sampling)=(" + int2str(ares.drop) + "," + int2str(ares.pps) + "," + int2str(ares.out_port) + "," + int2str(ares.out_plugin) + "," + int2str(ares.nh_low32) + "," + int2str(ares.qid) + "," + int2str(ares.stats_index) + "," + int2str(ares.sampling) + ")");

    try
    {
      configuration->add_afilter(&akey, &amask, priority, &ares);

      rliresp = new rli_response(this, NCCP_Status_Fine);
    }
    catch(std::exception& e)
    {
      std::string msg = e.what();
      write_log("add_filter_req::handle(): got exception: " + msg);
      rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    }
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
add_filter_req::parse()
{
  rli_request::parse();

  aux = params[0].getBool();
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

  if(!aux)
  {
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
  }
  else // auxiliary filter
  {
    afilter_key akey;
  
    akey.daddr = dest_prefix;
    akey.saddr = src_prefix;
    akey.ptag = plugin_tag;
    akey.port = port;
    akey.proto = protocol_val;
    akey.dport = dest_port;
    akey.sport = src_port;
    akey.exceptions = exceptions;
    akey.tcp_flags = tcp_flags;
    akey.res = 0;

    write_log("delete_filter_req::handle(): auxiliary filter akey(daddr,saddr,ptag,port,proto,dport,sport,exceptions,tcp_flags)=(" + int2str(akey.daddr) + "," + int2str(akey.saddr) + "," + int2str(akey.ptag) + "," + int2str(akey.port) + "," + int2str(akey.proto) + "," + int2str(akey.dport) + "," + int2str(akey.sport) + "," + int2str(akey.exceptions) + "," + int2str(akey.tcp_flags) + ")");
   
    try
    {
      configuration->del_afilter(&akey);

      rliresp = new rli_response(this, NCCP_Status_Fine);
    }
    catch(std::exception& e)
    {
      std::string msg = e.what();
      write_log("delete_filter_req::handle(): got exception: " + msg);
      rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    }
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
delete_filter_req::parse()
{
  rli_request::parse();

  aux = params[0].getBool();
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
 
set_sample_rates_req::set_sample_rates_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

set_sample_rates_req::~set_sample_rates_req()
{
}

bool
set_sample_rates_req::handle()
{
  rli_response* rliresp;
  aux_sample_rates rates;
  int x = 0;

  rates.rate00 = 0xffff;
  x = (int)(rate1*16/100);
  rates.rate01 = (~(0xffffffff << x));
  x = (int)(rate2*16/100);
  rates.rate10 = (~(0xffffffff << x));
  x = (int)(rate3*16/100);
  rates.rate11 = (~(0xffffffff << x));

  write_log("set_sample_rates_req::handle(): rates(rate00,rate01,rate10,rate11)=(" + int2str(rates.rate00) + "," + int2str(rates.rate01) + "," + int2str(rates.rate10) + "," + int2str(rates.rate11) + ")");

  try
  {
    configuration->set_sample_rates(&rates);
    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("set_sample_rates_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
set_sample_rates_req::parse()
{
  rli_request::parse();

  rate1 = params[0].getDouble();
  rate2 = params[1].getDouble();
  rate3 = params[2].getDouble();
}
 
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
    configuration->set_port_rate(port, rate);
    //cgw, how does this fit into the general hw model?
    uint32_t actual_rate = configuration->get_port_rate(port);
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
 
get_plugins_req::get_plugins_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

get_plugins_req::~get_plugins_req()
{
}

bool
get_plugins_req::handle()
{
  get_plugins_resp* rliresp;
  Configuration::plugin_file *plugins = NULL;
  int num_plugins;

  try
  {
    num_plugins = configuration->find_plugins(root_dir.getString(), &plugins);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_plugins_req::handle(): got exception: " + msg);
    rliresp = new get_plugins_resp(this, NCCP_Status_Failed);
    rliresp->send();
    delete rliresp;
    return true;
  }

  std::vector<std::string> labels;
  std::vector<std::string> paths;

  int i=0;
  while(i < num_plugins)
  {
    for(int j=0; j<5; ++j)
    {
      if(i < num_plugins)
      {
        labels.push_back(plugins[i].label);
        paths.push_back(plugins[i].full_path);
        ++i;
      }
      else
      {
        break;
      }
    }
    rliresp = new get_plugins_resp(this, labels, paths);
    if (i < num_plugins) rliresp->set_more_message(true);
    rliresp->send();
    delete rliresp;

    labels.clear();
    paths.clear();
  }

  delete[] plugins;

  return true;
}

void
get_plugins_req::parse()
{
  request::parse();

  buf >> root_dir;
}
 
add_plugin_req::add_plugin_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_plugin_req::~add_plugin_req()
{
}

bool
add_plugin_req::handle()
{
  rli_response* rliresp;
  try
  {
    configuration->add_plugin(path, microengine);
    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("add_plugin_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
add_plugin_req::parse()
{
  rli_request::parse();

  microengine = params[0].getInt();
  path = params[1].getString();
}
 
delete_plugin_req::delete_plugin_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

delete_plugin_req::~delete_plugin_req()
{
}

bool
delete_plugin_req::handle()
{
  rli_response* rliresp;
  try
  {
    configuration->del_plugin(microengine);
    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("delete_plugin_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
delete_plugin_req::parse()
{
  rli_request::parse();

  microengine = params[0].getInt();
}
 
plugin_debug_req::plugin_debug_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

plugin_debug_req::~plugin_debug_req()
{
}

bool
plugin_debug_req::handle()
{
  rli_response* rliresp;
  try
  {
    plugin_control->start_plugin_debugging(path);
    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("plugin_debug_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
plugin_debug_req::parse()
{
  rli_request::parse();

  path = params[0].getString();
}
 
plugin_control_msg_req::plugin_control_msg_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

plugin_control_msg_req::~plugin_control_msg_req()
{
}

bool
plugin_control_msg_req::handle()
{
  rli_response* rliresp;
  try
  {
    plugin_control->send_msg_to_plugin(microengine, msg, this);
    return false;
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("plugin_control_msg_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;
  return true;
}

void
plugin_control_msg_req::send_plugin_response(std::string msg)
{
  rli_response* rliresp;
  rliresp = new rli_response(this, NCCP_Status_Fine, msg);
  rliresp->send();
  delete rliresp;
}

void
plugin_control_msg_req::parse()
{
  rli_request::parse();

  microengine = params[0].getInt();
  msg = params[1].getString();
}
 
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
 
get_stats_preq_pkt_req::get_stats_preq_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_stats_preq_pkt_req::~get_stats_preq_pkt_req()
{
}

bool
get_stats_preq_pkt_req::handle()
{
  rli_response* rliresp;
  uint32_t stats_counter = 1;
  try
  {
    uint32_t val = monitor->read_stats_counter(stats_index, stats_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_stats_preq_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_stats_preq_pkt_req::parse()
{
  rli_request::parse();
  
  stats_index = params[0].getInt();
}
 
get_stats_postq_pkt_req::get_stats_postq_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_stats_postq_pkt_req::~get_stats_postq_pkt_req()
{
}

bool
get_stats_postq_pkt_req::handle()
{
  rli_response* rliresp;
  uint32_t stats_counter = 3;
  try
  {
    uint32_t val = monitor->read_stats_counter(stats_index, stats_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_stats_postq_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_stats_postq_pkt_req::parse()
{
  rli_request::parse();
  
  stats_index = params[0].getInt();
}
 
get_stats_preq_byte_req::get_stats_preq_byte_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_stats_preq_byte_req::~get_stats_preq_byte_req()
{
}

bool
get_stats_preq_byte_req::handle()
{
  rli_response* rliresp;
  uint32_t stats_counter = 0;
  try
  {
    uint32_t val = monitor->read_stats_counter(stats_index, stats_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_stats_preq_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_stats_preq_byte_req::parse()
{
  rli_request::parse();
  
  stats_index = params[0].getInt();
}
 
get_stats_postq_byte_req::get_stats_postq_byte_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_stats_postq_byte_req::~get_stats_postq_byte_req()
{
}

bool
get_stats_postq_byte_req::handle()
{
  rli_response* rliresp;
  uint32_t stats_counter = 2;
  try
  {
    uint32_t val = monitor->read_stats_counter(stats_index, stats_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_stats_postq_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_stats_postq_byte_req::parse()
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
 
get_plugin_counter_req::get_plugin_counter_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_plugin_counter_req::~get_plugin_counter_req()
{
}

bool
get_plugin_counter_req::handle()
{
  rli_response* rliresp;
  uint32_t stats_counter = ONL_ROUTER_PLUGIN_0_CNTR_0 + (microengine * 4) + counter;
  try
  {
    uint32_t val = monitor->read_stats_register(stats_counter);
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_plugin_counter_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_plugin_counter_req::parse()
{
  rli_request::parse();

  microengine = params[0].getInt();
  counter = params[1].getInt();
}
