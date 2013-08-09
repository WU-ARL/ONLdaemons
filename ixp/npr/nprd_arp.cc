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

ARP::ARP() throw()
{
  pthread_mutex_init(&request_list_lock, NULL);
  
  request_list = NULL;
}

ARP::~ARP() throw()
{
  free_list_mem(&request_list);

  pthread_mutex_destroy(&request_list_lock);
}

void ARP::process_arp_packet(DataPathMessage *dpm) throw(arp_exception)
{
  arp_packet pkt;

  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();

  char logstr[256];

  write_log("process_arp_packet: entering");

  unsigned int bd_addr = msg->buf_handle << 2;
  unsigned int offset = (SRAM_READ(SRAM_BANK_2, bd_addr+4)) & 0xffff;
  unsigned int dram_addr = (msg->buf_handle << 8) + offset - 14;

  // CGW, need to consider aligned access here in case plugins used this packet
  for(unsigned i=0; i<=10; ++i)
  {
    pkt.v[i] = DRAM_READ(dram_addr+(i*4));
    sprintf(logstr, "process_arp_packet: packet word %u: 0x%.8x", i, pkt.v[i]);
    write_log(logstr);
  }

  if(pkt.hw_type != 1)
  {
    write_log("process_arp_packet: ARP packet has incorrect hardware type; dropping");
    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    return;
  }
  if(pkt.proto_type != 0x0800)
  {
    write_log("process_arp_packet: ARP packet has incorrect protocol type; dropping");
    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    return;
  }
  if(pkt.hw_length != 6)
  {
    write_log("process_arp_packet: ARP packet has incorrect hardware length; dropping");
    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    return;
  }
  if(pkt.proto_length != 4)
  {
    write_log("process_arp_packet: ARP packet has incorrect protocol length; dropping");
    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    return;
  }
  if(pkt.operation != 1 && pkt.operation != 2)
  {
    write_log("process_arp_packet: ARP packet has incorrect operation; dropping");
    xscale_to_flm_data drop_pkt;
    drop_pkt.v[0] = msg->v[0];
    DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
    data_path_conn->writemsg(sm);
    delete sm;
    configuration->increment_drop_counter();
    return;
  }
  
  sprintf(logstr, "process_arp_packet: good arp packet found with src proto addr 0x%.8x", pkt.src_proto_addr);
  write_log(logstr);

  // now we know we have a valid ARP request or reply
  // in either case, however, we can add the source hardware to protocol address mapping to our table
  autoLock rllock(request_list_lock);
  requestnode *req;
  if((req = find_request(request_list, pkt.src_proto_addr)) != NULL)
  {
    write_log("process_arp_packet: found existing request");

    // there is already a request, so update the mac address info and update all entries using it
    req->macaddr_hi16 = pkt.src_hw_addr_hi16;
    req->macaddr_low32 = pkt.src_hw_addr_low32;

    if(req->state == ARP_OUTSTANDING)
    {
      write_log("process_arp_packet: found outstanding request");
      if(req->dip_msg != NULL)
      {
        write_log("process_arp_packet: found waiting destination IP packet");
        send_packet_to_data_path(req, (plc_to_xscale_data *)req->dip_msg->getmsg());
        delete req->dip_msg;
        req->dip_msg = NULL;
      }
      nhrnode *node = req->nhr_list;
      while(node != NULL)
      {
        write_log("process_arp_packet: found waiting next hop router packet");
        if(node->nhr_msg != NULL)
        {
          send_packet_to_data_path(req, (plc_to_xscale_data *)node->nhr_msg->getmsg());
          delete node->nhr_msg;
          node->nhr_msg = NULL;
        }
        node = node->next;
      }
    }

    arp_key ak;
    arp_key am;
    arp_result ar;

    ak.daddr = req->ipaddr;
    ak.res1 = 0;
    ak.res2 = 0;

    am.daddr = 0xffffffff;
    am.res1 = 0;
    am.res2 = 0;

    ar.entry_valid = 1;
    ar.res = 0;
    ar.nh_hi16 = req->macaddr_hi16;
    ar.nh_low32 = req->macaddr_low32;

    if(req->valid_arp_db_entry == 1)
    {
      write_log("process_arp_packet: updating existing ARP db entry");
      // if there is already an entry for this node, just update the result
      try { configuration->update_arp(&ak,&ar); }
      catch(std::exception& ce)
      {
        rllock.unlock();
        throw arp_exception("updating arp entry failed: " + std::string(ce.what()));
      }
    }
    else
    {
      write_log("process_arp_packet: adding new ARP db entry");
      // otherwise add the node
      try { configuration->add_arp(&ak,&am,&ar); }
      catch(std::exception& ce)
      {
        rllock.unlock();
        throw arp_exception("adding arp entry failed: " + std::string(ce.what()));
      }
      req->valid_arp_db_entry = 1;
    }
 
    // also update any nhr entries
    nhrnode *node = req->nhr_list;
    while(node != NULL)
    {
      sprintf(logstr, "process_arp_packet: updating next hop router entry for sram_addr 0x%.8x", node->sram_addr);
      write_log(logstr);

      afilter_result afr;
      afr.v[0] = SRAM_READ(SRAM_BANK_0, node->sram_addr + RESULT_BASE_ADDR);
      afr.v[1] = SRAM_READ(SRAM_BANK_0, node->sram_addr+4 + RESULT_BASE_ADDR);
      afr.v[2] = SRAM_READ(SRAM_BANK_0, node->sram_addr+8 + RESULT_BASE_ADDR);

      sprintf(logstr, "process_arp_packet: filter result word 0: 0x%.8x", afr.v[0]);
      write_log(logstr);
      sprintf(logstr, "process_arp_packet: filter result word 1: 0x%.8x", afr.v[1]);
      write_log(logstr);
      sprintf(logstr, "process_arp_packet: filter result word 2: 0x%.8x", afr.v[2]);
      write_log(logstr);

      afr.nh_ip_valid = 0;
      afr.nh_mac_valid = 1;
      afr.nh_hi16 = req->macaddr_hi16;
      afr.nh_low32 = req->macaddr_low32;

      sprintf(logstr, "process_arp_packet: new filter result word 0: 0x%.8x", afr.v[0]);
      write_log(logstr);
      sprintf(logstr, "process_arp_packet: new filter result word 1: 0x%.8x", afr.v[1]);
      write_log(logstr);
      sprintf(logstr, "process_arp_packet: new filter result word 2: 0x%.8x", afr.v[2]);
      write_log(logstr);

      SRAM_WRITE(SRAM_BANK_0, node->sram_addr + RESULT_BASE_ADDR, afr.v[0]);
      SRAM_WRITE(SRAM_BANK_0, node->sram_addr+4 + RESULT_BASE_ADDR, afr.v[1]);
      SRAM_WRITE(SRAM_BANK_0, node->sram_addr+8 + RESULT_BASE_ADDR, afr.v[2]);

      node = node->next;
    }

    write_log("process_arp_packet: setting request to be finished");
    req->state = ARP_FINISHED;
    rllock.unlock();
  }
  else
  {
    write_log("process_arp_packet: adding new (empty) request since none were found");
    req = add_request(&request_list, pkt.src_proto_addr);

    req->macaddr_hi16 = pkt.src_hw_addr_hi16;
    req->macaddr_low32 = pkt.src_hw_addr_low32;
    req->state = ARP_FINISHED;
    req->valid_arp_db_entry = 0;
    req->dip_msg = NULL;
    req->nhr_list = NULL;

    arp_key ak;
    arp_key am;
    arp_result ar;

    ak.daddr = req->ipaddr;
    ak.res1 = 0;
    ak.res2 = 0;

    am.daddr = 0xffffffff;
    am.res1 = 0;
    am.res2 = 0;

    ar.entry_valid = 1;
    ar.res = 0;
    ar.nh_hi16 = req->macaddr_hi16;
    ar.nh_low32 = req->macaddr_low32;

    write_log("process_arp_packet: adding new ARP DB entry for new request");
    try { configuration->add_arp(&ak,&am,&ar); }
    catch(std::exception& ce)
    {
      rllock.unlock();
      throw arp_exception("adding arp entry failed: " + std::string(ce.what()));
    }
    req->valid_arp_db_entry = 1;

    rllock.unlock();
  }

  // if packet was a request, we need to send a reply if we own the mapping
  //unsigned int my_addr = 0xc0a80000 | (router_number << 8) | ((msg->in_port + 1)*16 + 15);
  //unsigned int my_addr = conf->get_base_addr() | ((msg->in_port + 1)*16 + 15);
  unsigned int my_addr = configuration->get_port_addr(msg->in_port);
  if(pkt.operation == 1 && pkt.trgt_proto_addr == my_addr)
  {
    // setup a buffer descriptor
    buffer_desc req_bd;
    req_bd.buffer_next = 0xff;
    req_bd.buffer_size = 28;
    req_bd.offset = 0x18e;
    req_bd.packet_size = 28;
    req_bd.freelist_id = 0;
    req_bd.res1 = 0;
    req_bd.ref_count = 1;
    req_bd.stats_index = 0xffff;
    req_bd.nh_mac_hi16 = pkt.src_hw_addr_hi16;
    req_bd.nh_mac_low32 = pkt.src_hw_addr_low32;
    req_bd.ethertype = 0x0806;
    req_bd.res2 = 0;
    req_bd.res3 = 0;
    req_bd.packet_next = 0;

    // setup an arp reply packet in DRAM
    arp_packet req_packet;
    req_packet.dest_mac_hi16 = pkt.src_hw_addr_hi16;
    req_packet.dest_mac_low32 = pkt.src_hw_addr_low32;
    req_packet.src_mac_hi16 = configuration->get_port_mac_addr_hi16(msg->in_port);
    req_packet.src_mac_low32 = configuration->get_port_mac_addr_low32(msg->in_port);
    req_packet.ethertype = 0x0806;
    req_packet.hw_type = 1;
    req_packet.proto_type = 0x0800;
    req_packet.hw_length = 6;
    req_packet.proto_length = 4;
    req_packet.operation = 2;
    req_packet.src_hw_addr_hi16 = req_packet.src_mac_hi16;
    req_packet.src_hw_addr_low32 = req_packet.src_mac_low32;
    req_packet.src_proto_addr = my_addr;
    req_packet.trgt_hw_addr_hi16 = pkt.src_hw_addr_hi16;
    req_packet.trgt_hw_addr_low32 = pkt.src_hw_addr_low32;
    req_packet.trgt_proto_addr = pkt.src_proto_addr;
    req_packet.res1 = 0;
    req_packet.res2 = 0;
    req_packet.res3 = 0;
    req_packet.res4 = 0;
    req_packet.res5 = 0;
    req_packet.res6 = 0;

    // have to send through PLC for now b/c we can't guarantee we won't get interleaved
    // with someone else when writing to the QM, since the XScale can only write 1 word atomically
    xscale_to_mux_data mux_data;
    mux_data.res1 = 0;
    mux_data.out_port = msg->in_port;
    mux_data.buf_handle = SRAM_QUEUE_DEQUEUE(SRAM_BANK_2, BUF_FREE_LIST);
    if(mux_data.buf_handle == 0)
    {
      write_log("process_arp_packet: free list is out of descriptors.. not sending reply");
      xscale_to_flm_data drop_pkt;
      drop_pkt.v[0] = msg->v[0];
      DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
      data_path_conn->writemsg(sm);
      delete sm;
      configuration->increment_drop_counter();
      return;
    }
    mux_data.l3_pkt_length = 28;
    mux_data.qid = ((msg->in_port+1) << 13) | 0x1fff;
    mux_data.ptag = 0;
    mux_data.in_port = msg->in_port;
    mux_data.res2 = 0;
    mux_data.pass_through = 1;
    mux_data.stats_index = 0xffff;

    unsigned int i;

/*
    unsigned int req_bd_addr = qm_data.buf_handle << 2;
*/
    unsigned int req_bd_addr = mux_data.buf_handle << 2;
    for(i = 0; i < 8; ++i)
    {
      sprintf(logstr, "process_arp_packet: buffer descriptor word %d: %.8x", i, req_bd.v[i]);
      write_log(logstr);
      SRAM_WRITE(SRAM_BANK_2, req_bd_addr+(i*4), req_bd.v[i]);
    }

/*
    unsigned int req_buf_addr = (qm_data.buf_handle << 8) + 0x180;
*/
    unsigned int req_buf_addr = (mux_data.buf_handle << 8) + 0x180;
    for(i = 0; i < 16; ++i)
    {
      sprintf(logstr, "process_arp_packet: buffer word %d: %.8x", i, req_packet.v[i]);
      write_log(logstr);
      DRAM_WRITE(req_buf_addr+(i*4), req_packet.v[i]);
    }

    for(i = 0; i < XSCALE_TO_MUX_DATA_NUM_WORDS; ++i)
    {
      sprintf(logstr, "process_arp_packet: mux data word %d: %.8x", i, mux_data.v[i]);
      write_log(logstr);
    }

    DataPathMessage *newdpm = new DataPathMessage(&mux_data.v[0], MSG_TO_MUX);
    data_path_conn->writemsg(newdpm);
    configuration->update_stats_index(mux_data.stats_index, mux_data.l3_pkt_length);
    delete newdpm;
  }

  // regardless we need to drop the incoming packet
  xscale_to_flm_data drop_pkt;
  drop_pkt.v[0] = msg->v[0];
  DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
  data_path_conn->writemsg(sm);
  delete sm;
  //delete dpm;
}

void ARP::send_arp_request(DataPathMessage *dpm) throw(arp_exception)
{
  unsigned int ipaddr;
  unsigned int type;
  unsigned int sram_addr;
  char logstr[256];
  
  plc_to_xscale_data *msg = (plc_to_xscale_data *)dpm->getmsg();

  write_log("send_arp_request: entering");

  type = dpm->type();
  if(type == DP_ARP_NEEDED_DIP)
  {
    ipaddr = msg->dest_value;
  }
  else if(type == DP_ARP_NEEDED_NHR)
  {
    unsigned int result_word_0;
    sram_addr = msg->dest_value & 0x1fffff;

    result_word_0 = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR));
    if((result_word_0 >> 29) & 0x1 == 1)
    {
      sprintf(logstr, "send_arp_request: sram addr: 0x%.8x has valid next hop MAC, dropping packet", sram_addr);
      write_log(logstr);
      xscale_to_flm_data drop_pkt;
      drop_pkt.v[0] = msg->v[0];
      DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
      data_path_conn->writemsg(sm);
      delete sm;
      configuration->increment_drop_counter();
      delete dpm;
      return;
    }

    ipaddr = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR+8));
  }
  else
  {
    throw arp_exception("invalid message type!");
  }

  sprintf(logstr, "send_arp_request: ip addr: 0x%.8x", ipaddr);
  write_log(logstr);

  autoLock rllock(request_list_lock);
  requestnode *req;
  if((req = find_request(request_list, ipaddr)) != NULL)
  {
    write_log("send_arp_request: found existing request");

    if(req->state == ARP_FAILED)
    { 
      write_log("send_arp_request: found failed request");
      req->state = ARP_OUTSTANDING; // try again
    }
    else // request is outstanding or done
    {
      if(type == DP_ARP_NEEDED_DIP)
      {
        // if there is already a valid arp db entry or we have an outstanding msg waiting, then drop this packet
        if(req->valid_arp_db_entry == 1 || req->dip_msg != NULL)
        {
          //rllock.unlock();

          // enqueue packet to free list manager
          xscale_to_flm_data drop_pkt;
          drop_pkt.v[0] = msg->v[0];
          DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
          data_path_conn->writemsg(sm);
          delete sm;
          configuration->increment_drop_counter();
          delete dpm;
        }
        // if arp is done (and we haven't had a valid dip msg) then use the result here to
        // send packet back to data path and then update arp db
        else if(req->state == ARP_FINISHED)
        {
          send_packet_to_data_path(req, msg);

          // update ARP DB
          arp_key ak;
          arp_key am;
          arp_result ar;

          ak.daddr = req->ipaddr;
          ak.res1 = 0;
          ak.res2 = 0;

          am.daddr = 0xffffffff;
          am.res1 = 0;
          am.res2 = 0;

          ar.entry_valid = 1;
          ar.res = 0;
          ar.nh_hi16 = req->macaddr_hi16;
          ar.nh_low32 = req->macaddr_low32;

          try { configuration->add_arp(&ak,&am,&ar); }
          catch(std::exception& ce)
          {
            throw arp_exception("adding arp entry failed: " + std::string(ce.what()));
          }
   
          req->valid_arp_db_entry = 1;
          delete dpm;
          //rllock.unlock();
        }
        // if arp is not done (and we haven't had a valid dip msg) then register this message
        // to be the DIP msg associated with this request and return
        else
        {
          req->dip_msg = dpm;

          //rllock.unlock();
        }
      }
      else // now we are dealing with a next hop router entry
      {
        write_log("send_arp_request: dealing with next hop router request");
        unsigned int id = configuration->get_sram_map(sram_addr);
        nhrnode *nhr = find_nhr(req, id);
        if(nhr != NULL) // already a next hop router node for this address so drop the packet
        {
          //rllock.unlock();

          write_log("send_arp_request: found existing next hop router entry.. dropping packet");

          // enqueue packet to free list manager
          xscale_to_flm_data drop_pkt;
          drop_pkt.v[0] = msg->v[0];
          DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
          data_path_conn->writemsg(sm);
          delete sm;
          configuration->increment_drop_counter();
          delete dpm;
        }
        // no next hop router entry, so if arp is done, then send packet back and update sram result
        else if(req->state == ARP_FINISHED)
        { 
          nhrnode *new_nhr_node = new nhrnode();
          //new_nhr_node->nhr_msg = msg;
          new_nhr_node->sram_addr = sram_addr;
          new_nhr_node->sram_id = id;
          new_nhr_node->next = req->nhr_list;
          req->nhr_list = new_nhr_node;

          send_packet_to_data_path(req, msg);

          // update sram result
          afilter_result afr;
          afr.v[0] = SRAM_READ(SRAM_BANK_0, sram_addr + RESULT_BASE_ADDR);
          afr.v[1] = SRAM_READ(SRAM_BANK_0, sram_addr+4 + RESULT_BASE_ADDR);
          afr.v[2] = SRAM_READ(SRAM_BANK_0, sram_addr+8 + RESULT_BASE_ADDR);

          afr.nh_ip_valid = 0;
          afr.nh_mac_valid = 1;
          afr.nh_hi16 = req->macaddr_hi16;
          afr.nh_low32 = req->macaddr_low32;

          SRAM_WRITE(SRAM_BANK_0, sram_addr + RESULT_BASE_ADDR, afr.v[0]);
          SRAM_WRITE(SRAM_BANK_0, sram_addr+4 + RESULT_BASE_ADDR, afr.v[1]);
          SRAM_WRITE(SRAM_BANK_0, sram_addr+8 + RESULT_BASE_ADDR, afr.v[2]);

          //rllock.unlock();
          delete dpm;
        }
        // no next hop router entry, and arp is outstanding, so register this message as a nhr message
        else
        {
          nhrnode *new_nhr_node = new nhrnode();
          new_nhr_node->nhr_msg = dpm;
          new_nhr_node->sram_addr = sram_addr;
          new_nhr_node->sram_id = id;
          new_nhr_node->next = req->nhr_list;
          req->nhr_list = new_nhr_node;

          //rllock.unlock();
        }
      }

      rllock.unlock();
      return;
    }
  }

  if(req == NULL)
  {
    write_log("send_arp_request: no existing request");
    req = add_request(&request_list, ipaddr);
    req->macaddr_hi16 = 0;
    req->macaddr_low32 = 0;
    req->state = ARP_OUTSTANDING;
    req->valid_arp_db_entry = 0;
    if(type == DP_ARP_NEEDED_DIP)
    {
      //req->valid_dip_msg = 1;
      req->dip_msg = dpm;
      req->nhr_list = NULL;
    }
    else
    {
      sprintf(logstr, "send_arp_request: new entry gets next hop router msg for sram_addr 0x%.8x", sram_addr);
      write_log(logstr);
      //req->valid_dip_msg = 0;
      req->dip_msg = NULL;
      req->nhr_list = new nhrnode();
      req->nhr_list->nhr_msg = dpm;
      req->nhr_list->sram_addr = sram_addr;
      req->nhr_list->sram_id = configuration->get_sram_map(sram_addr);
      req->nhr_list->next = NULL;
    }
  }
  else
  {
    write_log("send_arp_request: found a failed request, re-trying");
    if(type == DP_ARP_NEEDED_DIP)
    {
      //req->valid_dip_msg = 1;
      req->dip_msg = dpm;
    }
    else
    {
      unsigned int id = configuration->get_sram_map(sram_addr);
      nhrnode *nhr_node = find_nhr(req, id);
      if(nhr_node == NULL)
      {
        nhrnode *new_nhr_node = new nhrnode();
        new_nhr_node->nhr_msg = dpm;
        new_nhr_node->sram_addr = sram_addr;
        new_nhr_node->sram_id = id;
        new_nhr_node->next = req->nhr_list;
        req->nhr_list = new_nhr_node;
      }
      else
      {
        nhr_node->nhr_msg = dpm;
      }
    }
  }
  rllock.unlock();

  // now have a request with the correct msgs associated with it, so send an ARP request
  
  unsigned int port = (msg->uc_mc_bits >> 3) & 0x7;

  write_log("send_arp_request: about to setup ARP packet buffer descriptor");

  // setup a buffer descriptor
  buffer_desc req_bd;
  req_bd.buffer_next = 0xff;
  req_bd.buffer_size = 28;
  req_bd.offset = 0x18e;
  req_bd.packet_size = 28;
  req_bd.freelist_id = 0;
  req_bd.res1 = 0;
  req_bd.ref_count = 1;
  req_bd.stats_index = 0xffff;
  req_bd.nh_mac_hi16 = 0xffff;
  req_bd.nh_mac_low32 = 0xffffffff;
  req_bd.ethertype = 0x0806;
  req_bd.res2 = 0;
  req_bd.res3 = 0;
  req_bd.packet_next = 0;

  write_log("send_arp_request: about to setup ARP packet");

  // setup an arp request packet in DRAM
  arp_packet req_packet;
  req_packet.dest_mac_hi16 = 0xffff;
  req_packet.dest_mac_low32 = 0xffffffff;
  req_packet.src_mac_hi16 = configuration->get_port_mac_addr_hi16(port);
  req_packet.src_mac_low32 = configuration->get_port_mac_addr_low32(port);
  req_packet.ethertype = 0x0806;
  req_packet.hw_type = 1;
  req_packet.proto_type = 0x0800;
  req_packet.hw_length = 6;
  req_packet.proto_length = 4;
  req_packet.operation = 1;
  req_packet.src_hw_addr_hi16 = req_packet.src_mac_hi16;
  req_packet.src_hw_addr_low32 = req_packet.src_mac_low32;
  //req_packet.src_proto_addr = 0xc0a80000 | (router_number << 8) | ((port+1)*16 + 15);
  //req_packet.src_proto_addr = configuration->get_base_addr() | ((port+1)*16 + 15);
  req_packet.src_proto_addr = configuration->get_port_addr(port);
  req_packet.trgt_hw_addr_hi16 = 0;
  req_packet.trgt_hw_addr_low32 = 0;
  req_packet.trgt_proto_addr = ipaddr;
  req_packet.res1 = 0;
  req_packet.res2 = 0;
  req_packet.res3 = 0;
  req_packet.res4 = 0;
  req_packet.res5 = 0;
  req_packet.res6 = 0;

  write_log("send_arp_request: about to setup ring data");

  // have to send through PLC for now b/c we can't guarantee we won't get interleaved
  // with someone else when writing to the QM, since the XScale can only write 1 word atomically
  xscale_to_mux_data mux_data;
  mux_data.res1 = 0;
  mux_data.out_port = port;
  mux_data.buf_handle = 0;
  mux_data.l3_pkt_length = 28;
  mux_data.qid = ((port+1) << 13) | 0x1fff;
  mux_data.ptag = 0;
  mux_data.in_port = port;
  mux_data.res2 = 0;
  mux_data.pass_through = 1;
  mux_data.stats_index = 0xffff;

  unsigned int attempts;
  for(attempts = 0; attempts < 3; ++attempts)
  {
    write_log("send_arp_request: attempting to send arp request");
    // first get a buffer from the free list
    unsigned int buf_handle = SRAM_QUEUE_DEQUEUE(SRAM_BANK_2, BUF_FREE_LIST);
    if(buf_handle == 0)
    { 
      write_log("send_arp_request: no descriptors in free list.. skipping attempt");
      sleep(5);
      continue;
    }
/*
    qm_data.buf_handle = buf_handle;
*/
    mux_data.buf_handle = buf_handle;

    unsigned int i;

    unsigned int req_bd_addr = buf_handle << 2;
    for(i = 0; i < 8; ++i)
    {
      sprintf(logstr, "send_arp_request: buffer descriptor word %d: %.8x", i, req_bd.v[i]);
      write_log(logstr);
      SRAM_WRITE(SRAM_BANK_2, req_bd_addr+(i*4), req_bd.v[i]);
    }

    unsigned int req_buf_addr = (buf_handle << 8) + 0x180;
    for(i = 0; i < 16; ++i)
    {
      sprintf(logstr, "send_arp_request: buffer word %d: %.8x", i, req_packet.v[i]);
      write_log(logstr);
      DRAM_WRITE(req_buf_addr+(i*4), req_packet.v[i]);
    }

    for(i = 0; i < XSCALE_TO_MUX_DATA_NUM_WORDS; ++i)
    {
      sprintf(logstr, "send_arp_request: mux data word %d: %.8x", i, mux_data.v[i]);
      write_log(logstr);
    }

    DataPathMessage *newdpm = new DataPathMessage((unsigned int *)&mux_data.v[0], MSG_TO_MUX);
    data_path_conn->writemsg(newdpm);
    configuration->update_stats_index(mux_data.stats_index, mux_data.l3_pkt_length);
    delete newdpm;

    sleep(5);

    rllock.lock();
    if(req->state == ARP_FINISHED)
    {
      rllock.unlock();
      break;
    }
    rllock.unlock();
  }

  if(attempts == 3)
  {
    rllock.lock(); 
    req->state = ARP_FAILED;
    if(req->dip_msg != NULL)
    {
      xscale_to_flm_data drop_pkt;
      drop_pkt.v[0] = req->dip_msg->getword(0);
      DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
      data_path_conn->writemsg(sm);
      delete sm;
      configuration->increment_drop_counter();
      delete req->dip_msg;
      req->dip_msg = NULL;
    }
    nhrnode *node = req->nhr_list;
    nhrnode *tmp;
    while(node != NULL)
    {
      tmp = node;
      node = node->next;
      if(tmp->nhr_msg != NULL)
      {
        xscale_to_flm_data drop_pkt;
        drop_pkt.v[0] = tmp->nhr_msg->getword(0);
        DataPathMessage *sm = new DataPathMessage((unsigned int *)&drop_pkt.v[0], MSG_TO_FLM);
        data_path_conn->writemsg(sm);
        delete sm;
        configuration->increment_drop_counter();
        delete tmp->nhr_msg;
        tmp->nhr_msg = NULL;
      }
    }
    rllock.unlock();
  }

  return;
}

void ARP::send_packet_to_data_path(requestnode *req, plc_to_xscale_data *msg) throw(arp_exception)
{
  // first have to update the buffer descriptor for this packet
  unsigned int buf_desc_addr = (msg->buf_handle) << 2;
  SRAM_WRITE(SRAM_BANK_2, buf_desc_addr+12, (msg->stats_index << 16) | (req->macaddr_hi16));
  SRAM_WRITE(SRAM_BANK_2, buf_desc_addr+16, req->macaddr_low32);
  SRAM_WRITE(SRAM_BANK_2, buf_desc_addr+20, (msg->ethertype << 16));

  // next send packet to data path (qm or plugins)
  unsigned int upps = (msg->uc_mc_bits >> 6) & 1;
  if(upps == 0)
  {
    // have to send through PLC for now b/c we can't guarantee we won't get interleaved
    // with someone else when writing to the QM, since the XScale can only write 1 word atomically
    xscale_to_mux_data data;
    data.res1 = 0;
    data.out_port = (msg->uc_mc_bits >> 3) & 0x7;
    data.buf_handle = msg->buf_handle;
    data.l3_pkt_length = msg->l3_pkt_length;
    data.qid = ((data.out_port+1) << 13) | (msg->qid & 0x1fff);
    data.ptag = msg->ptag;
    data.in_port = msg->in_port;
    data.res2 = 0;
    data.pass_through = 1;
    data.stats_index = msg->stats_index;

    DataPathMessage *mux_msg = new DataPathMessage((unsigned int *)&data.v[0], MSG_TO_MUX);
    data_path_conn->writemsg(mux_msg);
    configuration->update_stats_index(data.stats_index, data.l3_pkt_length);
    delete mux_msg;
  }
  else // upps == 1
  {
    msg->dest_value = (req->macaddr_hi16 << 16) & (req->macaddr_low32 >> 16);
    msg->nh_mac_low16 = req->macaddr_low32 & 0xffff;
    msg->arp_source = 0;
    msg->arp_needed = 0;
    DataPathMessage *plugin_msg;
    unsigned int out_plugin = msg->uc_mc_bits & 0x7;
    if(out_plugin == 0)
    {
      plugin_msg = new DataPathMessage((unsigned int *)msg, MSG_TO_PLUGIN_0);
    }
    else if(out_plugin == 1)
    {
      plugin_msg = new DataPathMessage((unsigned int *)msg, MSG_TO_PLUGIN_1);
    }
    else if(out_plugin == 2)
    {
      plugin_msg = new DataPathMessage((unsigned int *)msg, MSG_TO_PLUGIN_2);
    }
    else if(out_plugin == 3)
    {
      plugin_msg = new DataPathMessage((unsigned int *)msg, MSG_TO_PLUGIN_3);
    }
    else if(out_plugin == 4)
    {
      plugin_msg = new DataPathMessage((unsigned int *)msg, MSG_TO_PLUGIN_4);
    }
    else
    {
      throw arp_exception("packet bound for invalid plugin!");
    }
    data_path_conn->writemsg(plugin_msg);
    delete plugin_msg;
  }
}

ARP::requestnode *ARP::add_request(requestnode **list, unsigned int addr)
{
  requestnode *node = new requestnode();
  node->ipaddr = addr;
  node->next = *list;
  *list = node;
  return node;
}

ARP::requestnode *ARP::find_request(requestnode *list, unsigned int addr)
{
  requestnode *node = list;
  while(node != NULL)
  {
    if(node->ipaddr == addr) { break; }
    node = node->next;
  }
  return node;
}

ARP::nhrnode *ARP::find_nhr(requestnode *req, unsigned int id)
{
  nhrnode *node = req->nhr_list;
  while(node != NULL)
  {
    if(node->sram_id == id) { break; }
    node = node->next;
  }
  return node;
}

void ARP::free_list_mem(requestnode **list)
{
  requestnode *tmp;
  nhrnode *nhr_list, *tmp2;
  while(*list != NULL)
  {
    tmp = *list;
    *list = (*list)->next;

    nhr_list = tmp->nhr_list;
    while(nhr_list != NULL)
    {
      tmp2 = nhr_list;
      nhr_list = nhr_list->next;
      if(tmp2->nhr_msg != NULL) { delete tmp2->nhr_msg; }
      delete tmp2;
    }
    if(tmp->dip_msg != NULL) { delete tmp->dip_msg; }
    delete tmp;
  }
}
