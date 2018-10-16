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
#include <netlink/route/tc.h>
#include <netlink/route/link.h>

#include <boost/shared_ptr.hpp>

#include "swrd_types.h"
#include "swrd_router.h"
#include "swrd_globals.h"
#include "swrd_requests.h"

#define MINUS_1 0xffffffff

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
    router->set_username(exp.getExpInfo().getUserName());
    router->configure_port(node_conf.getPort(), node_conf.getRealPort(), node_conf.getVLan(), node_conf.getIPAddr(), node_conf.getSubnet(), bw_kbits, node_conf.getNHIPAddr());
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
    router->add_route_main(prefix, mask, output_port, nexthop_ip);

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
    router->add_route_port(port, prefix, mask, output_port, nexthop_ip);

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
    router->del_route_main(prefix, mask, output_port, nexthop_ip);

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
    router->del_route_port(port, prefix, mask, output_port, nexthop_ip);

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
 
filter_req::filter_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

filter_req::~filter_req()
{
}

bool
filter_req::handle()
{
  rli_response* rliresp;
  std::string req_name = "add_filter_req";
  if (get_op() == SWR_DeleteFilter) req_name = "delete_filter_req";
  try
  {
    filter_ptr filter(new filter_info());
    filter->dest_prefix = dest_prefix;
    filter->dest_mask = dest_mask;
    filter->src_prefix = src_prefix;
    filter->src_mask = src_mask;
    filter->protocol = protocol;
    if (dest_port == MINUS_1)
      filter->dest_port = -1;
    else
      filter->dest_port = dest_port;
    if (src_port == MINUS_1)
      filter->src_port = -1;
    else
      filter->src_port = src_port;
    if (tcp_fin > 1) filter->tcp_fin = -1;
    else filter->tcp_fin = tcp_fin;
    if (tcp_syn > 1) filter->tcp_syn = -1;
    else filter->tcp_syn = tcp_syn;
    if (tcp_rst > 1) filter->tcp_rst = -1;
    else filter->tcp_rst = tcp_rst;
    if (tcp_psh > 1) filter->tcp_psh = -1;
    else filter->tcp_psh = tcp_psh;
    if (tcp_ack > 1) filter->tcp_ack = -1;
    else filter->tcp_ack = tcp_ack;
    if (tcp_urg > 1) filter->tcp_urg = -1;
    else filter->tcp_urg = tcp_urg;
    filter->unicast_drop = unicast_drop;
    filter->output_port = output_port;
    filter->sampling = sampling;
    filter->qid = qid;
    filter->mark = mark;
    std::cout << " mark is " << mark << std::endl;

    bool ispkts = false;
    uint32_t val = 0;
    switch (get_op())
      {
      case SWR_AddFilter:
	router->add_filter(filter);
	break;
      case SWR_GetFilterPkts:
	ispkts = true;
      case SWR_GetFilterBytes:
	val = router->filter_stats(filter, ispkts);
	rliresp = new rli_response(this, NCCP_Status_Fine, val);
	rliresp->send();
	delete rliresp;
	return true;
      default://delete filter
	router->del_filter(filter);
	break;
      }
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log(req_name + "::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    rliresp->send();
    delete rliresp;
    return true;
  }

  int drop = 0;
  if (unicast_drop) drop = 1;
  
  write_log(req_name + "::handle(): filter(daddr,saddr,proto,dport,sport,tcp_flags,sampling,oport,drop,qid)=(" + dest_prefix + "/" + int2str(dest_mask) + "," + src_prefix + "/" + int2str(src_mask) + "," + protocol + "," + int2str(dest_port) + "," + int2str(src_port) + "," + int2str(tcp_fin) + " " + int2str(tcp_syn) + " " + int2str(tcp_rst) + " " + int2str(tcp_psh) + " " + int2str(tcp_ack) + " " + int2str(tcp_urg) + "," + int2str(sampling) + "," + output_port + "," + int2str(drop) + "," + int2str(qid) + ")");
  
   
  rliresp = new rli_response(this, NCCP_Status_Fine);
  rliresp->send();
  delete rliresp;
  
  return true;
}

void
filter_req::parse()
{
  rli_request::parse();

  dest_prefix = router->addr_int2str(params[0].getInt());
  dest_mask = params[1].getInt();
  src_prefix = router->addr_int2str(params[2].getInt());
  src_mask = params[3].getInt();
  protocol = params[4].getString();
  dest_port = params[5].getInt();
  src_port = params[6].getInt();
  tcp_fin = params[7].getInt();
  tcp_syn = params[8].getInt();
  tcp_rst = params[9].getInt();
  tcp_psh = params[10].getInt();
  tcp_ack = params[11].getInt();
  tcp_urg = params[12].getInt();
  unicast_drop = params[13].getBool();
  mark = 0;
  if (get_op() == SWR_AddFilter) 
    {
      output_port = params[14].getString();
      sampling = params[15].getInt();
      qid = params[16].getInt();
      if (params.size() > 17)
	{
	  mark = params[17].getInt();//+10 depending on if RLI allows specification of auto assignment start
	}
    }
  else sampling = 1;
  //priority = params[19].getInt(); 
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
    switch(get_op())
      {
      case SWR_AddQueue:
	router->add_queue(port, qid, rate, burst, ceil_rate, cburst, mtu);
	if (delay > 0 || jitter > 0 || loss_percent > 0 || corrupt_percent > 0 || duplicate_percent > 0)
	  router->add_netem_queue(port, qid, delay, jitter, loss_percent, corrupt_percent, duplicate_percent);
	rliresp = new rli_response(this, NCCP_Status_Fine);
	break;
      case SWR_ChangeQueue:
	router->add_queue(port, qid, rate, burst, ceil_rate, cburst, mtu, true);
	rliresp = new rli_response(this, NCCP_Status_Fine);
	break;
      case SWR_DeleteQueue:
	router->delete_queue(port, qid);
	rliresp = new rli_response(this, NCCP_Status_Fine);
	break;
      case SWR_AddNetemParams:
	if (delay > 0 || jitter > 0 || loss_percent > 0 || corrupt_percent > 0 || duplicate_percent > 0)
	  router->add_netem_queue(port, qid, delay, jitter, loss_percent, corrupt_percent, duplicate_percent);
	else
	  router->delete_netem_queue(port, qid);
	rliresp = new rli_response(this, NCCP_Status_Fine);
	break;
      case SWR_DeleteNetemParams:
	//if (delay > 0 || jitter > 0 || loss_percent > 0 || corrupt_percent > 0 || duplicate_percent > 0)
	router->delete_netem_queue(port, qid);
	rliresp = new rli_response(this, NCCP_Status_Fine);
	break;
      default:
	write_log("set_queue_params_req::handle(): invalid op");
	rliresp = new rli_response(this, NCCP_Status_Failed, "invalid op");
      }
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

  rate = 0;
  burst = 0;
  mtu = 0;
  delay = 0;
  jitter = 0;
  loss_percent = 0;
  corrupt_percent = 0;
  duplicate_percent = 0;
  int i = 0;
  qid = params[i++].getInt();
  switch(get_op())
    {
    case(SWR_DeleteQueue):
      break;
    case(SWR_AddQueue):
    case(SWR_ChangeQueue):
      rate = params[i++].getInt();
      burst = params[i++].getInt();
      mtu = params[i++].getInt();
      if (get_op() == SWR_ChangeQueue) break;
    case(SWR_AddNetemParams):
    case(SWR_DeleteNetemParams):
      delay = params[i++].getInt();
      jitter = params[i++].getInt();
      loss_percent = params[i++].getInt();
      corrupt_percent = params[i++].getInt();
      duplicate_percent = params[i++].getInt();
    }
  ceil_rate = rate; //params[3].getInt();
  cburst = burst; //params[4].getInt();
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
    router->set_port_rate(port, rate);
    rliresp = new rli_response(this, NCCP_Status_Fine, rate);
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

//Traffic Control
add_delay_req::add_delay_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_delay_req::~add_delay_req()
{
}

bool
add_delay_req::handle()
{
  rli_response* rliresp;
  try
  {
    router->add_delay_port(port, dtime, jitter);
    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("add_delay_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
add_delay_req::parse()
{
  rli_request::parse();

  dtime = params[0].getInt();
  jitter = params[1].getInt();
}

delete_delay_req::delete_delay_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

delete_delay_req::~delete_delay_req()
{
}

bool
delete_delay_req::handle()
{
  rli_response* rliresp;
  try
  {
    router->delete_delay_port(port);
    rliresp = new rli_response(this, NCCP_Status_Fine);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("delete_delay_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}


//MONITORING Requests

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
    bool use_class = false;
    if (get_op() == SWR_GetClassTXPkt) use_class = true;
    uint32_t val = (uint32_t)(0xffffffff & (router->read_stats_pkts(port, qid, use_class)));
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
  if (get_op() == SWR_GetQueueTXPkt)
    qid = params[0].getInt(); 
  else qid = 0;
}
 
get_tx_kbits_req::get_tx_kbits_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_tx_kbits_req::~get_tx_kbits_req()
{
}

bool
get_tx_kbits_req::handle()
{
  rli_response* rliresp;
  try
  {
    bool use_class = false;
    if (get_op() == SWR_GetClassTXKBits) use_class = true;
    uint32_t val = (uint32_t)(router->read_stats_bytes(port, qid, use_class)/125);//turn into kbits
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_tx_kbits_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_tx_kbits_req::parse()
{
  rli_request::parse();
  if (get_op() == SWR_GetQueueTXKBits)
    qid = params[0].getInt(); 
  else qid = 0;
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
    bool use_class = false;
    if (get_op() == SWR_GetClassLength) use_class = true;
    uint32_t val = (uint32_t)(0xffffffff & (router->read_stats_qlength(port, qid, use_class)));
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
  if (get_op() == SWR_GetQueueLength)
    qid = params[0].getInt(); 
  else qid = 0;
}
 
get_drops_req::get_drops_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_drops_req::~get_drops_req()
{
}

bool
get_drops_req::handle()
{
  rli_response* rliresp;
  try
  {
    bool use_class = false;
    if (get_op() == SWR_GetClassDrops) use_class = true;
    uint32_t val = (uint32_t)(0xffffffff & (router->read_stats_drops(port, qid, use_class)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_drops_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_drops_req::parse()
{
  rli_request::parse();
  if (get_op() == SWR_GetQueueDrops)
    qid = params[0].getInt(); 
  else qid = 0;
}

 
get_backlog_req::get_backlog_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_backlog_req::~get_backlog_req()
{
}

bool
get_backlog_req::handle()
{
  rli_response* rliresp;
  try
  {
    bool use_class = false;
    if (get_op() == SWR_GetClassBacklog) use_class = true;
    uint32_t val = (uint32_t)(0xffffffff & (router->read_stats_backlog(port, qid, use_class)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_backlog_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_backlog_req::parse()
{
  rli_request::parse();
  if (get_op() == SWR_GetQueueBacklog)
    qid = params[0].getInt(); 
  else qid = 0;
}


get_link_tx_pkt_req::get_link_tx_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_tx_pkt_req::~get_link_tx_pkt_req()
{
}

bool
get_link_tx_pkt_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(0xffffffff & (router->read_link_stats_txpkts(port)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_tx_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_tx_pkt_req::parse()
{
  rli_request::parse();
}


get_link_rx_pkt_req::get_link_rx_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_rx_pkt_req::~get_link_rx_pkt_req()
{
}

bool
get_link_rx_pkt_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(0xffffffff & (router->read_link_stats_rxpkts(port)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_rx_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_rx_pkt_req::parse()
{
  rli_request::parse();
}
 
get_link_tx_kbits_req::get_link_tx_kbits_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_tx_kbits_req::~get_link_tx_kbits_req()
{
}

bool
get_link_tx_kbits_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(router->read_link_stats_txbytes(port)/125);//turn into kbits
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_tx_kbits_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_tx_kbits_req::parse()
{
  rli_request::parse();
}
 
get_link_rx_kbits_req::get_link_rx_kbits_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_rx_kbits_req::~get_link_rx_kbits_req()
{
}

bool
get_link_rx_kbits_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(router->read_link_stats_rxbytes(port)/125);//turn into kbits
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_rx_kbits_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_rx_kbits_req::parse()
{
  rli_request::parse();
}


get_link_tx_drops_req::get_link_tx_drops_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_tx_drops_req::~get_link_tx_drops_req()
{
}

bool
get_link_tx_drops_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(0xffffffff & (router->read_link_stats_txdrops(port)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_tx_drops_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_tx_drops_req::parse()
{
  rli_request::parse();
}


get_link_rx_drops_req::get_link_rx_drops_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_rx_drops_req::~get_link_rx_drops_req()
{
}

bool
get_link_rx_drops_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(0xffffffff & (router->read_link_stats_rxdrops(port)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_rx_drops_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_rx_drops_req::parse()
{
  rli_request::parse();
}


get_link_tx_errors_req::get_link_tx_errors_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_tx_errors_req::~get_link_tx_errors_req()
{
}

bool
get_link_tx_errors_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(0xffffffff & (router->read_link_stats_txerrors(port)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_tx_errors_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_tx_errors_req::parse()
{
  rli_request::parse();
}


get_link_rx_errors_req::get_link_rx_errors_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_link_rx_errors_req::~get_link_rx_errors_req()
{
}

bool
get_link_rx_errors_req::handle()
{
  rli_response* rliresp;
  try
  {
    uint32_t val = (uint32_t)(0xffffffff & (router->read_link_stats_rxerrors(port)));
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_link_rx_errors_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_link_rx_errors_req::parse()
{
  rli_request::parse();
}
