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
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>

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

#include "halMev2Api.h"
#include "hal_dram.h"
#include "hal_sram.h"
#include "hal_scratch.h"

using namespace npr;

IPv4::IPv4() throw()
{
}

IPv4::~IPv4() throw()
{
}

void IPv4::send_ttl_expired(DataPathMessage *dpm) throw()
{
  unsigned int i;
  char logstr[256];

  write_log("send_ttl_expired: entering");

  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();

  unsigned int bd_addr = msg->buf_handle << 2;
  unsigned int offset = (SRAM_READ(SRAM_BANK_2, bd_addr+4)) & 0xffff;
  unsigned int dram_addr = (msg->buf_handle << 8) + offset;
  
  ipv4_hdr orig_pkt_hdr;
  unsigned int *ip_options = NULL;
  unsigned int num_opt_words = 0;
  unsigned int orig_pkt_data[8];

  read_in(dram_addr, &orig_pkt_hdr.v[0], 5);
  dram_addr += 20;
  for(i=0; i<5; ++i)
  {
    sprintf(logstr, "send_ttl_expired: ipv4 header word %u: 0x%.8x", i, orig_pkt_hdr.v[i]);
    write_log(logstr);
  }

  if(orig_pkt_hdr.ip_hdr_len > 5)
  {
    num_opt_words = orig_pkt_hdr.ip_hdr_len - 5;
    ip_options = new unsigned int[num_opt_words];
  }

  if(num_opt_words > 0)
  {
    read_in(dram_addr, &ip_options[0], num_opt_words);
    dram_addr += num_opt_words*4;
    for(i=0; i<num_opt_words; ++i)
    {
      sprintf(logstr, "send_ttl_expired: ipv4 header word %u: 0x%.8x", i+5, ip_options[i]);
      write_log(logstr);
    }
  }

  read_in(dram_addr, &orig_pkt_data[0], 2);
  dram_addr += 8;
  for(i=0; i<2; ++i)
  {
    sprintf(logstr, "send_ttl_expired: ipv4 data word %u: 0x%.8x", i, orig_pkt_data[i]);
    write_log(logstr);
  }

  //unsigned int my_addr = 0xc0a80000 | (router_number << 8) | ((msg->in_port + 1)*16 + 15);
  //unsigned int my_addr = conf->get_base_addr() | ((msg->in_port + 1)*16 + 15);
  unsigned int my_addr = configuration->get_port_addr(msg->in_port);

  // if it's already been to the xscale, then drop it
  if(orig_pkt_hdr.ip_src_addr == my_addr)
  {
    write_log("send_ttl_expired: got a packet from the xscale.. dropping");
    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    if(num_opt_words > 0)
    {
      delete[] ip_options;
    }
    return;
  }
  
  buffer_desc bd;
  bd.buffer_next = 0xff;
  bd.buffer_size = 28+28+(num_opt_words*4); // 28 for ipv4 + icmp hdr, 28 for orig ipv4 hdr + 8 bytes data, options
  bd.offset = 0x18e;
  bd.packet_size = 28+28+(num_opt_words*4);
  bd.freelist_id = 0;
  bd.res1 = 0;
  bd.ref_count = 1;
  bd.stats_index = 0; // this is going back to get classified
  bd.nh_mac_hi16 = 0; // this is going back to get classified
  bd.nh_mac_low32 = 0; // this is going back to get classified
  bd.ethertype = 0x0800;
  bd.res2 = 0;
  bd.res3 = 0;
  bd.packet_next = 0;

  ipv4_hdr pkt_ip_hdr;
  icmp_hdr pkt_icmp_hdr;
  pkt_ip_hdr.ip_version = 4;
  pkt_ip_hdr.ip_hdr_len = 5;
  pkt_ip_hdr.ip_tos = 0;
  pkt_ip_hdr.ip_len = 28+28+(num_opt_words*4);
  pkt_ip_hdr.ip_id = 0;
  pkt_ip_hdr.ip_flags = 2;
  pkt_ip_hdr.ip_frag_offset = 0;
  pkt_ip_hdr.ip_ttl = 64;
  pkt_ip_hdr.ip_proto = 1;
  pkt_ip_hdr.ip_hdr_cksm = 0;
  pkt_ip_hdr.ip_src_addr = my_addr;
  pkt_ip_hdr.ip_dest_addr = orig_pkt_hdr.ip_src_addr;

  pkt_ip_hdr.ip_hdr_cksm = compute_cksm(&pkt_ip_hdr.v[0], 5);

  pkt_icmp_hdr.icmp_type = 11;
  pkt_icmp_hdr.icmp_code = 0;
  pkt_icmp_hdr.icmp_cksm = 0;
  pkt_icmp_hdr.icmp_id = 0;
  pkt_icmp_hdr.icmp_seq = 0;

  pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &pkt_icmp_hdr.v[0], 2);
  pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &orig_pkt_hdr.v[0], 5);
  if(num_opt_words > 0)
  {
    pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &ip_options[0], num_opt_words);
  }
  pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &orig_pkt_data[0], 2);
  pkt_icmp_hdr.icmp_cksm = 0xffff - pkt_icmp_hdr.icmp_cksm;

  xscale_to_mux_data mux_data;
  mux_data.res1 = 0;
  mux_data.out_port = 0;
  mux_data.buf_handle = SRAM_QUEUE_DEQUEUE(SRAM_BANK_2, BUF_FREE_LIST);
  if(mux_data.buf_handle == 0)
  {
    write_log("send_ttl_expired: free list is out of descriptors.. not sending icmp message");
    if(num_opt_words > 0)
    {
      delete[] ip_options;
    }

    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    return;
  }

  mux_data.l3_pkt_length = 28+28+(num_opt_words*4);
  mux_data.qid = 0;
  mux_data.ptag = 0;
  mux_data.in_port = msg->in_port; // same as above
  mux_data.res2 = 0;
  mux_data.pass_through = 0;
  mux_data.stats_index = 0;

  bd_addr = mux_data.buf_handle << 2;
  for(i=0; i<8; ++i)
  {
    sprintf(logstr, "send_ttl_expired: buffer descriptor word %d: %.8x", i, bd.v[i]);
    write_log(logstr);
    SRAM_WRITE(SRAM_BANK_2, bd_addr+(i*4), bd.v[i]);
  }

  dram_addr = (mux_data.buf_handle << 8) + 0x18e;
  write_out(dram_addr, &pkt_ip_hdr.v[0], 5);
  dram_addr += 20;
  for(i=0; i<5; ++i)
  {
    sprintf(logstr, "send_ttl_expired: buffer word %d: %.8x", i, pkt_ip_hdr.v[i]);
    write_log(logstr);
  }

  write_out(dram_addr, &pkt_icmp_hdr.v[0], 2);
  dram_addr += 8;
  for(i=0; i<2; ++i)
  {
    sprintf(logstr, "send_ttl_expired: buffer word %d: %.8x", i+5, pkt_icmp_hdr.v[i]);
    write_log(logstr);
  }

  write_out(dram_addr, &orig_pkt_hdr.v[0], 5);
  dram_addr += 20;
  for(i=0; i<5; ++i)
  {
    sprintf(logstr, "send_ttl_expired: buffer word %d: %.8x", i+7, orig_pkt_hdr.v[i]);
    write_log(logstr);
  }

  if(num_opt_words > 0)
  {
    write_out(dram_addr, &ip_options[0], num_opt_words);
    dram_addr += num_opt_words*4;
    for(i=0; i<num_opt_words; ++i)
    {
      sprintf(logstr, "send_ttl_expired: buffer word %d: %.8x", i+12, ip_options[i]);
      write_log(logstr);
    }
  }

  write_out(dram_addr, &orig_pkt_data[0], 2);
  dram_addr += 8;
  for(i=0; i<2; ++i)
  {
    sprintf(logstr, "send_ttl_expired: buffer word %d: %.8x", i+12+num_opt_words, orig_pkt_data[i]);
    write_log(logstr);
  }

  for(i=0; i<XSCALE_TO_MUX_DATA_NUM_WORDS; ++i)
  {
    sprintf(logstr, "send_ttl_expired: mux data word %d: %.8x", i, mux_data.v[i]);
    write_log(logstr);
  }

  DataPathMessage *newdpm = new DataPathMessage((unsigned int *)&mux_data.v[0], MSG_TO_MUX);
  data_path_conn->writemsg(newdpm);
  delete newdpm;

  if(num_opt_words > 0)
  {
    delete[] ip_options;
  }

  xscale_to_flm_data drop_pkt;
  drop_pkt.v[0] = msg->v[0];
  DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
  data_path_conn->writemsg(sm);
  delete sm;
  configuration->increment_drop_counter();
}

void IPv4::send_no_route(DataPathMessage *dpm) throw()
{
  unsigned int i;
  char logstr[256];

  write_log("send_no_route: entering");

  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();

  unsigned int bd_addr = msg->buf_handle << 2;
  unsigned int offset = (SRAM_READ(SRAM_BANK_2, bd_addr+4)) & 0xffff;
  unsigned int dram_addr = (msg->buf_handle << 8) + offset;
  
  ipv4_hdr orig_pkt_hdr;
  unsigned int *ip_options = NULL;
  unsigned int num_opt_words = 0;
  unsigned int orig_pkt_data[8];

  read_in(dram_addr, &orig_pkt_hdr.v[0], 5);
  dram_addr += 20;
  for(i=0; i<5; ++i)
  {
    sprintf(logstr, "send_no_route: ipv4 header word %u: 0x%.8x", i, orig_pkt_hdr.v[i]);
    write_log(logstr);
  }

  if(orig_pkt_hdr.ip_hdr_len > 5)
  {
    num_opt_words = orig_pkt_hdr.ip_hdr_len - 5;
    ip_options = new unsigned int[num_opt_words];
  }

  if(num_opt_words > 0)
  {
    read_in(dram_addr, &ip_options[0], num_opt_words);
    dram_addr += num_opt_words*4;
    for(i=0; i<num_opt_words; ++i)
    {
      sprintf(logstr, "send_no_route: ipv4 header word %u: 0x%.8x", i+5, ip_options[i]);
      write_log(logstr);
    }
  }

  read_in(dram_addr, &orig_pkt_data[0], 8);
  dram_addr += 32;
  for(i=0; i<8; ++i)
  {
    sprintf(logstr, "send_no_route: ipv4 data word %u: 0x%.8x", i, orig_pkt_data[i]);
    write_log(logstr);
  }

  //unsigned int my_addr = 0xc0a80000 | (router_number << 8) | ((msg->in_port + 1)*16 + 15);
  //unsigned int my_addr = conf->get_base_addr() | ((msg->in_port + 1)*16 + 15);
  unsigned int my_addr = configuration->get_port_addr(msg->in_port);

  // if it's already been to the xscale, then drop it
  if(orig_pkt_hdr.ip_src_addr == my_addr)
  {
    write_log("send_no_route: got a packet from the xscale.. dropping");
    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    if(num_opt_words > 0)
    {
      delete[] ip_options;
    }
    return;
  }
  
  buffer_desc bd;
  bd.buffer_next = 0xff;
  bd.buffer_size = 28+28+(num_opt_words*4); // 28 for ipv4 + icmp hdr, 28 for orig ipv4 hdr + 8 bytes data + options
  bd.offset = 0x18e;
  bd.packet_size = 28+28+(num_opt_words*4);
  bd.freelist_id = 0;
  bd.res1 = 0;
  bd.ref_count = 1;
  bd.stats_index = 0; // this is going back to get classified
  bd.nh_mac_hi16 = 0; // this is going back to get classified
  bd.nh_mac_low32 = 0; // this is going back to get classified
  bd.ethertype = 0x0800;
  bd.res2 = 0;
  bd.res3 = 0;
  bd.packet_next = 0;

  ipv4_hdr pkt_ip_hdr;
  icmp_hdr pkt_icmp_hdr;
  pkt_ip_hdr.ip_version = 4;
  pkt_ip_hdr.ip_hdr_len = 5;
  pkt_ip_hdr.ip_tos = 0;
  pkt_ip_hdr.ip_len = 28+28+(num_opt_words*4);
  pkt_ip_hdr.ip_id = 0;
  pkt_ip_hdr.ip_flags = 0;
  pkt_ip_hdr.ip_frag_offset = 0;
  pkt_ip_hdr.ip_ttl = 64;
  pkt_ip_hdr.ip_proto = 1;
  pkt_ip_hdr.ip_hdr_cksm = 0;
  pkt_ip_hdr.ip_src_addr = my_addr;
  pkt_ip_hdr.ip_dest_addr = orig_pkt_hdr.ip_src_addr;

  pkt_ip_hdr.ip_hdr_cksm = compute_cksm(&pkt_ip_hdr.v[0], 5);

  pkt_icmp_hdr.icmp_type = 3;
  //unsigned int low_addr = 0xc0a80000 | (router_number << 8) | 16;
  //unsigned int hi_addr = 0xc0a80000 | (router_number << 8) | 5*16+15;
/*
  unsigned int rba = conf->get_base_addr();
  unsigned int low_addr = rba | 16;
  unsigned int hi_addr = rba | 5*16+15;
  if(orig_pkt_hdr.ip_dest_addr > low_addr && orig_pkt_hdr.ip_dest_addr < hi_addr)
  {
    pkt_icmp_hdr.icmp_code = 1;
  }
  else
  {
    pkt_icmp_hdr.icmp_code = 0;
  }
*/
  pkt_icmp_hdr.icmp_code = 0;
  pkt_icmp_hdr.icmp_cksm = 0;
  pkt_icmp_hdr.icmp_id = 0;
  pkt_icmp_hdr.icmp_seq = 0;

  pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &pkt_icmp_hdr.v[0], 2);
  pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &orig_pkt_hdr.v[0], 5);
  if(num_opt_words > 0)
  {
    pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &ip_options[0], num_opt_words);
  }
  pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &orig_pkt_data[0], 8);
  pkt_icmp_hdr.icmp_cksm = 0xffff - pkt_icmp_hdr.icmp_cksm;

  xscale_to_mux_data mux_data;
  mux_data.res1 = 0;
  mux_data.out_port = 0;
  mux_data.buf_handle = SRAM_QUEUE_DEQUEUE(SRAM_BANK_2, BUF_FREE_LIST);
  if(mux_data.buf_handle == 0)
  {
    write_log("send_no_route: free list is out of descriptors.. not sending icmp message");
    if(num_opt_words > 0)
    {
      delete[] ip_options;
    }

    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    return;
  }

  mux_data.l3_pkt_length = 28+28+(num_opt_words*4);
  mux_data.qid = 0;
  mux_data.ptag = 0;
  mux_data.in_port = msg->in_port; // same as above
  mux_data.res2 = 0;
  mux_data.pass_through = 0;
  mux_data.stats_index = 0;

  bd_addr = mux_data.buf_handle << 2;
  for(i=0; i<8; ++i)
  {
    sprintf(logstr, "send_no_route: buffer descriptor word %d: %.8x", i, bd.v[i]);
    write_log(logstr);
    SRAM_WRITE(SRAM_BANK_2, bd_addr+(i*4), bd.v[i]);
  }

  dram_addr = (mux_data.buf_handle << 8) + 0x18e;
  write_out(dram_addr, &pkt_ip_hdr.v[0], 5);
  dram_addr += 20;
  for(i=0; i<5; ++i)
  {
    sprintf(logstr, "send_no_route: buffer word %d: %.8x", i, pkt_ip_hdr.v[i]);
    write_log(logstr);
  }

  write_out(dram_addr, &pkt_icmp_hdr.v[0], 2);
  dram_addr += 8;
  for(i=0; i<2; ++i)
  {
    sprintf(logstr, "send_no_route: buffer word %d: %.8x", i+5, pkt_icmp_hdr.v[i]);
    write_log(logstr);
  }

  write_out(dram_addr, &orig_pkt_hdr.v[0], 5);
  dram_addr += 20;
  for(i=0; i<5; ++i)
  {
    sprintf(logstr, "send_no_route: buffer word %d: %.8x", i+7, orig_pkt_hdr.v[i]);
    write_log(logstr);
  }

  if(num_opt_words > 0)
  {
    write_out(dram_addr, &ip_options[0], num_opt_words);
    dram_addr += num_opt_words*4;
    for(i=0; i<num_opt_words; ++i)
    {
      sprintf(logstr, "send_no_route: buffer word %d: %.8x", i+12, ip_options[i]);
      write_log(logstr);
    }
  }

  write_out(dram_addr, &orig_pkt_data[0], 8);
  dram_addr += 32;
  for(i=0; i<8; ++i)
  {
    sprintf(logstr, "send_no_route: buffer word %d: %.8x", i+12+num_opt_words, orig_pkt_data[i]);
    write_log(logstr);
  }

  for(i=0; i<XSCALE_TO_MUX_DATA_NUM_WORDS; ++i)
  {
    sprintf(logstr, "send_no_route: mux data word %d: %.8x", i, mux_data.v[i]);
    write_log(logstr);
  }

  DataPathMessage *newdpm = new DataPathMessage((unsigned int *)&mux_data.v[0], MSG_TO_MUX);
  data_path_conn->writemsg(newdpm);
  delete newdpm;

  if(num_opt_words > 0)
  {
    delete[] ip_options;
  }

  xscale_to_flm_data drop_pkt;
  drop_pkt.v[0] = msg->v[0];
  DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
  data_path_conn->writemsg(sm);
  delete sm;
  configuration->increment_drop_counter();
}

void IPv4::handle_ip_options(DataPathMessage *dpm) throw()
{
  write_log("handle_ip_options: got a packet.. dropping it");
  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();
  xscale_to_flm_data drop_pkt;
  drop_pkt.v[0] = msg->v[0];
  DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
  data_path_conn->writemsg(sm);
  delete sm;
  configuration->increment_drop_counter();
}

void IPv4::handle_next_hop_invalid(DataPathMessage *dpm) throw()
{
  write_log("handle_next_hop_invalid: got a packet.. dropping it");
  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();
  xscale_to_flm_data drop_pkt;
  drop_pkt.v[0] = msg->v[0];
  DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
  data_path_conn->writemsg(sm);
  delete sm;
  configuration->increment_drop_counter();
}

void IPv4::handle_local_delivery(DataPathMessage *dpm) throw()
{
  char logstr[256];

  write_log("handle_local_delivery: entering");

  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();

  unsigned int bd_addr = msg->buf_handle << 2;
  unsigned int offset = (SRAM_READ(SRAM_BANK_2, bd_addr+4)) & 0xffff;
  unsigned int dram_addr = (msg->buf_handle << 8) + offset;

  ipv4_hdr pkt_hdr;
  read_in(dram_addr, &pkt_hdr.v[0], 5);
  dram_addr += 20;
  for(unsigned int i=0; i<5; ++i)
  {
    sprintf(logstr, "handle_local_delivery: ipv4 header word %u: 0x%.8x", i, pkt_hdr.v[i]);
    write_log(logstr);
  }

  // if the packet is ICMP, see if it is an echo request and if so respond
  if(pkt_hdr.ip_proto == 1)
  {
    icmp_hdr pkt_icmp_hdr;
    read_in(dram_addr, &pkt_icmp_hdr.v[0], 2);
    dram_addr += 8;
    for(unsigned int i=0; i<2; ++i)
    {
      sprintf(logstr, "handle_local_delivery: icmp header word %u: 0x%.8x", i, pkt_icmp_hdr.v[i]);
      write_log(logstr);
    }
    if(pkt_icmp_hdr.icmp_type == 8)
    {
      write_log("handle_local_delivery: packet is an echo request");

      // this is an icmp echo request, so re-write hdr info and send reply back
      pkt_hdr.ip_ttl = 64;
      pkt_hdr.ip_hdr_cksm = 0;
      unsigned int temp = pkt_hdr.ip_src_addr;
      pkt_hdr.ip_src_addr = pkt_hdr.ip_dest_addr;
      pkt_hdr.ip_dest_addr = temp;
      pkt_hdr.ip_hdr_cksm = compute_cksm(&pkt_hdr.v[0], 5);  // options are processed elsewhere

      write_log("handle_local_delivery: done updating ip header");

      pkt_icmp_hdr.icmp_type = 0;
      pkt_icmp_hdr.icmp_cksm = 0;
      
      pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &pkt_icmp_hdr.v[0], 2);
      unsigned int val[1];
      unsigned int j;
      for(j=28; j<=pkt_hdr.ip_len-4; j+=4)
      { 
        //val[0] = DRAM_READ(dram_addr);
        read_in(dram_addr, &val[0], 1);
        dram_addr += 4;
        pkt_icmp_hdr.icmp_cksm = compute_partial_cksm(pkt_icmp_hdr.icmp_cksm, &val[0], 1);
      }
      if(j != pkt_hdr.ip_len)
      {
        unsigned int carry;
        //temp = DRAM_READ(dram_addr);
        read_in(dram_addr, &val[0], 1);
        if(j == pkt_hdr.ip_len - 1)
        {
          pkt_icmp_hdr.icmp_cksm += (val[0] >> 16) & 0xff00;
          carry = pkt_icmp_hdr.icmp_cksm >> 16;
          pkt_icmp_hdr.icmp_cksm = (pkt_icmp_hdr.icmp_cksm & 0xffff) + carry;
        }
        else if(j == pkt_hdr.ip_len - 2)
        {
          pkt_icmp_hdr.icmp_cksm += val[0] >> 16;
          carry = pkt_icmp_hdr.icmp_cksm >> 16;
          pkt_icmp_hdr.icmp_cksm = (pkt_icmp_hdr.icmp_cksm & 0xffff) + carry;
        }
        else // j == pkt_hdr.ip_len - 3
        {
          pkt_icmp_hdr.icmp_cksm += val[0] >> 16;
          carry = pkt_icmp_hdr.icmp_cksm >> 16;
          pkt_icmp_hdr.icmp_cksm = (pkt_icmp_hdr.icmp_cksm & 0xffff) + carry;

          pkt_icmp_hdr.icmp_cksm += val[0] & 0xff00;
          carry = pkt_icmp_hdr.icmp_cksm >> 16;
          pkt_icmp_hdr.icmp_cksm = (pkt_icmp_hdr.icmp_cksm & 0xffff) + carry;
        }
      }
      pkt_icmp_hdr.icmp_cksm = 0xffff - pkt_icmp_hdr.icmp_cksm;

      write_log("handle_local_delivery: done updating icmp header");

      // nothing in the buffer descriptor needs to change, so leave it alone.

      // update the packet
      unsigned int dram_addr = (msg->buf_handle << 8) + offset;
      //only the last three words of the IPv4 header and the first word of the ICMP header changed
      unsigned int vals[4];
      vals[0] = pkt_hdr.v[2];
      vals[1] = pkt_hdr.v[3];
      vals[2] = pkt_hdr.v[4];
      vals[3] = pkt_icmp_hdr.v[0];
      write_out(dram_addr+8, &vals[0], 4);
      /*
      DRAM_WRITE(dram_addr+8, pkt_hdr.v[2]);
      DRAM_WRITE(dram_addr+12, pkt_hdr.v[3]);
      DRAM_WRITE(dram_addr+16, pkt_hdr.v[4]);
      DRAM_WRITE(dram_addr+20, pkt_icmp_hdr.v[0]);
      */

      write_log("handle_local_delivery: done updating the packet");

      // now send the packet back to MUX
      xscale_to_mux_data mux_data;
      mux_data.res1 = 0;
      mux_data.out_port = 0;
      mux_data.buf_handle = msg->buf_handle;
      mux_data.l3_pkt_length = msg->l3_pkt_length;
      mux_data.qid = 0;
      mux_data.ptag = msg->ptag;
      mux_data.in_port = msg->in_port; // same as above
      mux_data.res2 = 0;
      mux_data.pass_through = 0;
      mux_data.stats_index = 0;

      write_log("handle_local_delivery: done setting up the mux ring data");

      DataPathMessage *newdpm = new DataPathMessage(&mux_data.v[0], MSG_TO_MUX);
      data_path_conn->writemsg(newdpm);

      write_log("handle_local_delivery: done sending the message to mux");

      delete newdpm;
      //delete dpm;

      return;
    }
  }

  write_log("handle_local_delivery:: got a packet that I don't know what to do with.. dropping");
  xscale_to_flm_data drop_pkt;
  drop_pkt.v[0] = msg->v[0];
  DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
  data_path_conn->writemsg(sm);
  delete sm;
  configuration->increment_drop_counter();
}

void IPv4::handle_general_error(DataPathMessage *dpm) throw()
{
  write_log("handle_general_error: got a packet.. dropping it");
  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();
  xscale_to_flm_data drop_pkt;
  drop_pkt.v[0] = msg->v[0];
  DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
  data_path_conn->writemsg(sm);
  delete sm;
  configuration->increment_drop_counter();
}

unsigned int IPv4::compute_cksm(unsigned int *data, unsigned int num) throw()
{
  unsigned int cksm = 0;
  cksm = compute_partial_cksm(cksm, data, num);
  cksm = 0xffff - cksm;
  return cksm;
}

unsigned int IPv4::compute_partial_cksm(unsigned int cksm, unsigned int *data, unsigned int num) throw()
{
  unsigned int carry;
  for(unsigned int i=0; i<num; ++i)
  {
    cksm += data[i] >> 16;
    carry = cksm >> 16;
    cksm = (cksm & 0xffff) + carry;

    cksm += data[i] & 0xffff;
    carry = cksm >> 16;
    cksm = (cksm & 0xffff) + carry;
  }
  return cksm;
}

void IPv4::read_in(unsigned int addr, unsigned int *data, unsigned int num) throw()
{
  unsigned int byte_offset = addr % 4;
  if(byte_offset == 0)
  {
    for(unsigned int i=0; i<num; ++i)
    {
      data[i] = DRAM_READ(addr);
      addr += 4;
    }
  }
  else
  {
    unsigned int aligned_addr = addr - byte_offset;
    unsigned int *vals = new unsigned int[num+1];
    for(unsigned int i=0; i<num+1; ++i)
    {
      vals[i] = DRAM_READ(aligned_addr);
      aligned_addr += 4;
    }

    unsigned int shift1 = byte_offset * 8;
    unsigned int shift2 = 32 - shift1;
    for(unsigned int i=0; i<num; ++i)
    {
      data[i] = (vals[i] << shift1) | (vals[i+1] >> shift2);
    }
    delete[] vals;
  }
}

void IPv4::write_out(unsigned int addr, unsigned int *data, unsigned int num) throw()
{
  unsigned int byte_offset = addr % 4;
  if(byte_offset == 0)
  {
    for(unsigned int i=0; i<num; ++i)
    {
      DRAM_WRITE(addr, data[i]);
      addr += 4;
    }
  }
  else
  {
    unsigned int aligned_addr = addr - byte_offset;
    unsigned int shift1 = byte_offset * 8;
    unsigned int shift2 = 32 - shift1;

    unsigned int *vals = new unsigned int[num+1];
    vals[0] = (DRAM_READ(aligned_addr) & (0xffffffff << shift2)) | data[0] >> shift1;
    for(unsigned int i=1; i<num; ++i)
    {
      vals[i] = (data[i-1] << shift2) | (data[i] >> shift1);
    }
    vals[num] = (data[num-1] << shift2) | (DRAM_READ(aligned_addr+(num*4)) & (0xffffffff >> shift1));

    for(unsigned int i=0; i<num+1; ++i)
    {
      DRAM_WRITE(aligned_addr, vals[i]);
      aligned_addr += 4;
    }
    delete[] vals;
  }
}
