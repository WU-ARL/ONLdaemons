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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <glob.h>

#include "halMev2Api.h"
#include "hal_sram.h"
#include "hal_scratch.h"
#include "hal_global.h"
#include "uclo.h"

#include "shared.h"

#include "nprd_types.h"
#include "nprd_configuration.h"

#include <npr_tcam/tcamd_cmds.h>

using namespace npr;

Configuration::Configuration(unsigned long n, rtm_mac_addrs *ms) throw(configuration_exception)
{
  unsigned short port = DEFAULT_TCAMD_PORT;
  unsigned int tcamd_addr;

  struct ifreq ifr;
  char addr_str[INET_ADDRSTRLEN];
  struct sockaddr_in sa,*temp;
  int socklen;

  int temp_sock;
  tcamd_conn = -1;

  int status;

  if(n != 1 && n != 0)
  {
    throw configuration_exception("NPU is invalid");
  }

  npu = n;

  // get my ip addr
  if((temp_sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
  {
    throw configuration_exception("socket failed");
  }
  strcpy(ifr.ifr_name, "eth0");
  if(ioctl(temp_sock, SIOCGIFADDR, &ifr) < 0)
  {
    throw configuration_exception("ioctl failed");
  }
  close(temp_sock);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  temp = (struct sockaddr_in *)&ifr.ifr_addr;
  sa.sin_addr.s_addr = temp->sin_addr.s_addr;
  control_address = (unsigned int) sa.sin_addr.s_addr;
  if(inet_ntop(AF_INET,&sa.sin_addr,addr_str,INET_ADDRSTRLEN) == NULL)
  {
    throw configuration_exception("inet_ntop failed");
  }
  // done getting ip addr

  // npu is 1 if we are NPUA, 0 if NPUB
  tcamd_addr = control_address - (1 - npu);

  // connect to the tcam daemon on NPUA
  if((tcamd_conn = socket(PF_INET,SOCK_STREAM,0)) < 0)
  {
    throw configuration_exception("socket failed");
  }

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(tcamd_addr);

  socklen = sizeof(struct sockaddr_in);
  if(connect(tcamd_conn, (struct sockaddr *)&sa, socklen) < 0)
  {
    throw configuration_exception("connect failed");
  }
  // now we are ready to make changes to the TCAM

  // next call initialization for intel xscale libraries
  if((status = halMe_Init(0xff00ff)) != HALME_SUCCESS)
  {
    throw configuration_exception("failed to initialize Microengines!");
  }

  // last initialize all the locks used here
  pthread_mutex_init((&tcam_lock), NULL);
  pthread_mutex_init((&sram_lock), NULL);
  pthread_mutex_init((&scratch_lock), NULL);
  pthread_mutex_init((&conf_lock), NULL);

  state = STOP;
  used_me_mask = 0;
  router_base_handle = NULL;

  next_id = 0;
  srmap = NULL;

  sram_free_list = NULL;
  for(int i=0; i<=MAX_PREFIX_LEN; ++i)
  {
    mc_routes[i] = NULL;
    uc_routes[i] = NULL;
  }
  for(int i=0; i<=MAX_PRIORITY; ++i)
  {
    pfilters[i] = NULL;
    afilters[i] = NULL;
  }
  arps = NULL;

  for(unsigned int i=0; i<5; ++i)
  {
    macs.port_hi16[i] = ms->port_hi16[i];
    macs.port_low32[i] = ms->port_low32[i];
  }

  username = "";

  // get the chip type
  unsigned int prodId = GET_GLB_CSR(PRODUCT_ID);
  unsigned int majType = (prodId & PID_MAJOR_PROD_TYPE) >> PID_MAJOR_PROD_TYPE_BITPOS;
  unsigned int minType = (prodId & PID_MINOR_PROD_TYPE) >> PID_MINOR_PROD_TYPE_BITPOS;
  unsigned int majRev = (prodId & PID_MAJOR_REV) >> PID_MAJOR_REV_BITPOS;
  unsigned int minRev = (prodId & PID_MINOR_REV) >> PID_MINOR_REV_BITPOS;

  if(majType != 0)
  {
    throw configuration_exception("chip type not known: major type=" + int2str(majType) + ", minor type=" + int2str(minType));
  }

  if(minType == 0)
  {
    if(majRev == 3 && minRev == 0)
    {
      ixp_version = "2805";
      majRev -= 3;
    }
    else if(majRev == 1 && minRev == 1)
    {
      ixp_version = "2800";
    }
    else
    {
      throw configuration_exception("chip type not known: major type=" + int2str(majType) + ", minor type=" + int2str(minType));
    }
  }
  else if(minType == 1)
  {
    if(majRev == 1 && minRev == 1)
    {
      ixp_version = "2850";
    }
    else
    {
      throw configuration_exception("chip type not known: major type=" + int2str(majType) + ", minor type=" + int2str(minType));
    }
  }
  else
  {
    throw configuration_exception("chip type not known: major type=" + int2str(majType) + ", minor type=" + int2str(minType));
  }

  char rev = 'A';
  rev += majRev;

  write_log("Chip is an IXP" + ixp_version + " " + rev + int2str(minRev));
}

Configuration::~Configuration() throw()
{
  if(state == START)
  {
    stop_router();
  }

  pthread_mutex_destroy(&tcam_lock);
  pthread_mutex_destroy(&sram_lock);
  pthread_mutex_destroy(&scratch_lock);
  pthread_mutex_destroy(&conf_lock);

  halMe_DelLib();

  if(tcamd_conn != -1)
  {
    if(close(tcamd_conn) < 0)
    {
      write_log("close failed for TCAM daemon connection");
    }
    tcamd_conn = -1;
  }
}

void Configuration::start_router(std::string router_base_uof_file) throw(configuration_exception)
{
  unsigned int addr;

  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];
  
  int status;
  
  autoLock glock(conf_lock);

  if(state == START)
  {
    write_log("start_router called, but the router has already been started..");
    return;
  }

  write_log("start_router: entering");

  /* send the initialization command to the TCAM daemon */
  *cmd = tcamd::ROUTERINIT;
  
  if(write(tcamd_conn, buf, sizeof(tcamd::tcam_command)) != sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write failed");
  }
    
  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }
    
  if(*result != tcamd::SUCCESS)
  {
    throw configuration_exception("ROUTERINIT failed!");
  }
  /* TCAM init was successful */

  /* now initialize all the necessary memory and such */
  
  // clear out sram ring occupancy counters
  addr = SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE;
  for(int i=0; i<SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_NUM; i++)
  {
    SCRATCH_WRITE(addr, 0);
    addr += 4;
  }

  // clear MUX's scratch memory
  addr = SCR_MUX_MEMORY_BASE;
  for(int i=0; i<SCR_MUX_MEMORY_SIZE; i++)
  {
    SCRATCH_WRITE(addr, 0);
    addr += 4;
  }

  // initialize mux policy
  mux_quanta mq;
  mq.rx_quantum = DEF_MUX_RX_QUANTUM;
  mq.plugin_quantum = DEF_MUX_PL_QUANTUM;
  mq.xscaleqm_quantum = DEF_MUX_XSQM_QUANTUM;
  mq.xscalepl_quantum = DEF_MUX_XSPL_QUANTUM;
  set_mux_quanta(&mq);

  // initialize port rates
  port_rates pr;
  pr.port0_rate = DEF_PORT_RATE;
  pr.port1_rate = DEF_PORT_RATE;
  pr.port2_rate = DEF_PORT_RATE;
  pr.port3_rate = DEF_PORT_RATE;
  pr.port4_rate = DEF_PORT_RATE;
  set_port_rates(&pr);

  // zero out all queue descriptor fields for all queues
  for(int i=0, addr=QD_BASE_ADDR; i<QM_NUM_QUEUES; ++i, addr+=QD_UNIT_SIZE)
  {
    SRAM_WRITE(SRAM_BANK_2,addr,0);
    SRAM_WRITE(SRAM_BANK_2,addr+4,0);
    SRAM_WRITE(SRAM_BANK_2,addr+8,0);
    SRAM_WRITE(SRAM_BANK_2,addr+12,0);
  }

  // setup all queue parameters with initial values
  for(int i=0, addr=QPARAMS_BASE_ADDR; i<QM_NUM_QUEUES; ++i, addr+=QPARAMS_UNIT_SIZE)
  {
    SRAM_WRITE(SRAM_BANK_1,addr,0);
    SRAM_WRITE(SRAM_BANK_1,addr+4,DEF_Q_THRESHOLD);
    SRAM_WRITE(SRAM_BANK_1,addr+8,DEF_Q_QUANTUM);
    SRAM_WRITE(SRAM_BANK_1,addr+12,0);
  }

  // initialize header format control block (mac addys)
  addr = HF_BLOCK_CONTROL_MEM_BASE;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_hi16[0]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_low32[0]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_hi16[1]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_low32[1]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_hi16[2]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_low32[2]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_hi16[4]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_low32[3]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_hi16[4]);
  addr += 4;
  SRAM_WRITE(SRAM_BANK_3,addr,macs.port_low32[4]);

  char logstr[256];
  sprintf(logstr, "start_router: port 0 mac address: %.4x%.8x", macs.port_hi16[0],macs.port_low32[0]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 1 mac address: %.4x%.8x", macs.port_hi16[1],macs.port_low32[1]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 2 mac address: %.4x%.8x", macs.port_hi16[2],macs.port_low32[2]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 3 mac address: %.4x%.8x", macs.port_hi16[3],macs.port_low32[3]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 4 mac address: %.4x%.8x", macs.port_hi16[4],macs.port_low32[4]);
  write_log(logstr);

  // tell lookup which dbs to use
  addr = LOOKUP_BLOCK_CONTROL_MEM_BASE;
  unsigned int db_id_start = 0;
  if(npu == 0) { db_id_start += 4; }
  for(unsigned int d = db_id_start; d<(db_id_start+4); addr+=4, ++d)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,d);
  }

  // initialize sampling parameters
  aux_sample_rates asr;
  asr.rate00 = DEF_SAMPLE_RATE_00;
  asr.rate01 = DEF_SAMPLE_RATE_01;
  asr.rate10 = DEF_SAMPLE_RATE_10;
  asr.rate11 = DEF_SAMPLE_RATE_11;
  set_sample_rates(&asr);

  // initialize exception destinations
  exception_dests eds;
  eds.no_route = DEF_DEST_NR;
  eds.arp_needed = DEF_DEST_AN;
  eds.nh_invalid = DEF_DEST_NI;
  set_exception_dests(&eds);

  // zero out the stats memory block
  for(addr = ONL_ROUTER_STATS_BASE; addr < ONL_ROUTER_STATS_BASE + ONL_ROUTER_STATS_SIZE; addr += 4)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,0);
  }

  // zero out the global registers
  for(addr = ONL_ROUTER_REGISTERS_BASE; addr < ONL_ROUTER_REGISTERS_BASE + ONL_ROUTER_REGISTERS_SIZE; addr += 4)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,0);
  }

  // zero out plugin sram blocks
  for(addr = ONL_ROUTER_PLUGIN_0_BASE; addr < ONL_ROUTER_PLUGIN_0_BASE + ONL_ROUTER_PLUGIN_0_SIZE; addr += 4)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,0);
  }
  for(addr = ONL_ROUTER_PLUGIN_1_BASE; addr < ONL_ROUTER_PLUGIN_1_BASE + ONL_ROUTER_PLUGIN_1_SIZE; addr += 4)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,0);
  }
  for(addr = ONL_ROUTER_PLUGIN_2_BASE; addr < ONL_ROUTER_PLUGIN_2_BASE + ONL_ROUTER_PLUGIN_2_SIZE; addr += 4)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,0);
  }
  for(addr = ONL_ROUTER_PLUGIN_3_BASE; addr < ONL_ROUTER_PLUGIN_3_BASE + ONL_ROUTER_PLUGIN_3_SIZE; addr += 4)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,0);
  }
  for(addr = ONL_ROUTER_PLUGIN_4_BASE; addr < ONL_ROUTER_PLUGIN_4_BASE + ONL_ROUTER_PLUGIN_4_SIZE; addr += 4)
  {
    SRAM_WRITE(SRAM_BANK_3,addr,0);
  }
  /* done with memory initialization */

  write_log("start_router: done with memory initializaion");

  /* next get the tcam and sram free index lists initialized */
  sram_free_list = new indexlist();
  sram_free_list->left = 1;
  sram_free_list->right = RESULT_NUM_ENTRIES - 2;
  sram_free_list->next = NULL;

  write_log("start_router: done initializing sram_free_list");

  for(int i=MAX_PREFIX_LEN; i>=2; --i)
  {
    mc_routes[i] = new indexlist();
    mc_routes[i]->left = ROUTE_MC_FIRST_INDEX + (ROUTE_MC_NUM_PER_CLASS*(MAX_PREFIX_LEN-i));
    mc_routes[i]->right = mc_routes[i]->left + ROUTE_MC_NUM_PER_CLASS - 1;
    mc_routes[i]->next = NULL;

    uc_routes[i] = new indexlist();
    uc_routes[i]->left = ROUTE_UC_FIRST_INDEX + (ROUTE_UC_NUM_PER_CLASS*(MAX_PREFIX_LEN-i));
    uc_routes[i]->right = uc_routes[i]->left + ROUTE_UC_NUM_PER_CLASS - 1;
    uc_routes[i]->next = NULL;
  }
  // special case for prefix lens of 0 and 1: 1 gets 1 less than otherwise so there is room for one default route
  mc_routes[1] = new indexlist();
  mc_routes[1]->left = ROUTE_MC_FIRST_INDEX + (ROUTE_MC_NUM_PER_CLASS*(MAX_PREFIX_LEN-1));
  mc_routes[1]->right = mc_routes[1]->left + ROUTE_MC_NUM_PER_CLASS - 2;
  mc_routes[1]->next = NULL;
  mc_routes[0] = new indexlist();
  mc_routes[0]->left = mc_routes[1]->right + 1;
  mc_routes[0]->right = mc_routes[0]->left;
  mc_routes[0]->next = NULL;
  
  uc_routes[1] = new indexlist();
  uc_routes[1]->left = ROUTE_UC_FIRST_INDEX + (ROUTE_UC_NUM_PER_CLASS*(MAX_PREFIX_LEN-0));
  uc_routes[1]->right = uc_routes[1]->left + ROUTE_UC_NUM_PER_CLASS - 2;
  uc_routes[1]->next = NULL;
  uc_routes[0] = new indexlist();
  uc_routes[0]->left = uc_routes[1]->right + 1;
  uc_routes[0]->right = uc_routes[0]->left;
  uc_routes[0]->next = NULL;
  
  write_log("start_router: done initializing route lists");

  //for(int i=MAX_PRIORITY; i>=0; --i)
  for(int i=0; i<=MAX_PRIORITY; ++i)
  {
    pfilters[i] = new indexlist();
    //pfilters[i]->left = PFILTER_FIRST_INDEX + (PFILTER_NUM_PER_CLASS*(MAX_PRIORITY-i));
    pfilters[i]->left = PFILTER_FIRST_INDEX + (PFILTER_NUM_PER_CLASS*i);
    pfilters[i]->right = pfilters[i]->left + PFILTER_NUM_PER_CLASS - 1;
    pfilters[i]->next = NULL;

    afilters[i] = new indexlist();
    //afilters[i]->left = AFILTER_FIRST_INDEX + (AFILTER_NUM_PER_CLASS*(MAX_PRIORITY-i));
    afilters[i]->left = AFILTER_FIRST_INDEX + (AFILTER_NUM_PER_CLASS*i);
    afilters[i]->right = afilters[i]->left + AFILTER_NUM_PER_CLASS - 1;
    afilters[i]->next = NULL;
  }

  write_log("start_router: done initializing filter lists");

  arps = new indexlist();
  arps->left = ARP_FIRST_INDEX;
  arps->right = ARP_LAST_INDEX;
  arps->next = NULL;

  write_log("start_router: done initializing arp list");

  /* now get the router base uof file into memory, load the microengines and start them */
  std::string rbf = router_base_uof_file + "_" + ixp_version;
  if((status = UcLo_LoadObjFile(&router_base_handle, (char *)rbf.c_str())) != UCLO_SUCCESS)
  {
    throw configuration_exception("failed to load router base code, status=" + int2str(status));
  } 

  used_me_mask = used_me_mask | UcLo_GetAssignedMEs(router_base_handle);

  if((status = UcLo_WriteUimageAll(router_base_handle)) != UCLO_SUCCESS)
  {
    throw configuration_exception("failed to load router base code, status=" + int2str(status));
  }

  write_log("start_router: done setting up router base code");

  start_mes();

  /* the router is all ready to go now */
  state = START;

  // give the MEs a chance to get going before we do anything else
  sleep(2);

  glock.unlock();

  pfilter_key def_filter_key;
  pfilter_key def_filter_mask;
  pfilter_result def_filter_result;
 
  def_filter_key.daddr = 0;
  def_filter_key.saddr = 0;
  def_filter_key.ptag = 0;
  def_filter_key.port = 0;
  def_filter_key.proto = 0;
  def_filter_key.dport = 0;
  def_filter_key.sport = 0;
  def_filter_key.exceptions = 1;
  def_filter_key.tcp_flags = 0;
  def_filter_key.res = 0;

  def_filter_mask.daddr = 0;
  def_filter_mask.saddr = 0;
  def_filter_mask.ptag = 0;
  def_filter_mask.port = 0;
  def_filter_mask.proto = 0;
  def_filter_mask.dport = 0;
  def_filter_mask.sport = 0;
  def_filter_mask.exceptions = 1;
  def_filter_mask.tcp_flags = 0;
  def_filter_mask.res = 0;

  def_filter_result.entry_valid = 1;
  def_filter_result.nh_ip_valid = 0;
  def_filter_result.nh_mac_valid = 1; //CGW is this ok?  something needs to be done..
  def_filter_result.ip_mc_valid = 0;
  def_filter_result.uc_mc_bits = (1 << 6) | 0x5;
  def_filter_result.qid = 0;
  def_filter_result.stats_index = 0xffff;
  def_filter_result.nh_hi16 = 0;
  def_filter_result.nh_low32 = 0;

  add_pfilter(&def_filter_key, &def_filter_mask, 60, &def_filter_result);

  def_filter_key.exceptions = 2;
  def_filter_mask.exceptions = 2;
  add_pfilter(&def_filter_key, &def_filter_mask, 60, &def_filter_result);

  def_filter_key.exceptions = 4;
  def_filter_mask.exceptions = 4;
  add_pfilter(&def_filter_key, &def_filter_mask, 60, &def_filter_result);

  def_filter_key.exceptions = 8;
  def_filter_mask.exceptions = 8;
  add_pfilter(&def_filter_key, &def_filter_mask, 60, &def_filter_result);

  write_log("start_router: done adding default filters");

  write_log("start_router: done");

  pfilter_key query;
  pfilter_result query_res;

  query.daddr=0;
  query.saddr=0;
  query.ptag=0;
  query.port=0;
  query.dport=0;
  query.sport=0;
  query.exceptions=12;
  query.tcp_flags=0;
  query.res=0;

  try
  {
    query_pfilter(&query,&query_res);
    sprintf(logstr, "got result: 0x%.8x%.8x%.8x", query_res.v[0], query_res.v[1], query_res.v[2]);
    write_log(logstr);
  }
  catch(std::exception& ce)
  {
    write_log("exception in test query_pfilter: " + std::string(ce.what()));
  }

  return;
}

void Configuration::stop_router() throw()
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];
  
  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];
  
  int status; 

  autoLock glock(conf_lock);

  if(state == STOP)
  { 
    write_log("stop_router called, but the router is already stopped..");
    return;
  }

  write_log("stop_router: entering");

  /* first stop the microengines */
  try { stop_mes(); }
  catch(std::exception& e)
  {
    write_log("failed to stop the microengines");
  }

  /* now free up the memory used for all the free index lists (no need to explicitly remove the TCAM entries) */
  free_list_mem(&sram_free_list);
  for(int i=0; i<=MAX_PREFIX_LEN; ++i)
  {
    free_list_mem(&mc_routes[i]);
    free_list_mem(&uc_routes[i]);
  }
  for(int i=0; i<=MAX_PRIORITY; ++i)
  {
    free_list_mem(&pfilters[i]);
    free_list_mem(&afilters[i]);
  }
  free_list_mem(&arps);
  
  /* now send the stop command to the TCAM daemon */
  *cmd = tcamd::ROUTERSTOP;
  
  if(write(tcamd_conn, buf, sizeof(tcamd::tcam_command)) != sizeof(tcamd::tcam_command))
  {
    write_log("TCAM daemon ROUTERSTOP failed");
    return;
  } 
  
  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    write_log("TCAM daemon ROUTERSTOP failed");
    return;
  } 
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    write_log("TCAM daemon ROUTERSTOP failed");
    return;
  }
  
  if(*result != tcamd::SUCCESS)
  {
    write_log("TCAM daemon ROUTERSTOP failed");
  } 
  /* done with TCAM stop command */

  /* cleanup the memory associated with the uof file */
  if((status = UcLo_DeleObj(router_base_handle)) != UCLO_SUCCESS)
  {
    write_log("failed to delete the router base code");
  }
  /* done */

  state = STOP;
  used_me_mask = 0;

  write_log("stop_router: done");

  return;
}

unsigned int Configuration::router_started() throw()
{
  autoLock glock(conf_lock);
  if(state == START) return 1;
  return 0;
}

void
Configuration::configure_port(unsigned int port, std::string ip, std::string nexthop) throw(configuration_exception)
{
  // add default filters so that packets for the router are delivered

  struct in_addr addr;
  inet_pton(AF_INET, ip.c_str(), &addr);
  port_addrs[port] = addr.s_addr;
  write_log("Configuration::configure_port: port=" + int2str(port) + ", ipstr=" + ip + ", ip=" + int2str(port_addrs[port]));

  inet_pton(AF_INET, nexthop.c_str(), &addr);
  next_hops[port] = addr.s_addr;

  pfilter_key def_filter_key;
  pfilter_key def_filter_mask;
  pfilter_result def_filter_result;

  def_filter_key.daddr = port_addrs[port];
  def_filter_key.saddr = 0;
  def_filter_key.ptag = 0;
  def_filter_key.port = port;
  def_filter_key.proto = 0;
  def_filter_key.dport = 0;
  def_filter_key.sport = 0;
  def_filter_key.exceptions = 0;
  def_filter_key.tcp_flags = 0;
  def_filter_key.res = 0;

  def_filter_mask.daddr = 0xffffffff;
  def_filter_mask.saddr = 0;
  def_filter_mask.ptag = 0;
  def_filter_mask.port = 0x7;
  def_filter_mask.proto = 0;
  def_filter_mask.dport = 0;
  def_filter_mask.sport = 0;
  def_filter_mask.exceptions = 0;
  def_filter_mask.tcp_flags = 0;
  def_filter_mask.res = 0;

  def_filter_result.entry_valid = 1;
  def_filter_result.nh_ip_valid = 0;
  def_filter_result.nh_mac_valid = 1; //CGW is this ok?  something needs to be done..
  def_filter_result.ip_mc_valid = 0;
  def_filter_result.uc_mc_bits = (1 << 6) | 0x5;
  def_filter_result.qid = 0;
  def_filter_result.stats_index = 0xfff0;
  def_filter_result.nh_hi16 = 0;
  def_filter_result.nh_low32 = 0;

  add_pfilter(&def_filter_key, &def_filter_mask, 60, &def_filter_result);
}

unsigned int Configuration::find_plugins(std::string dir, plugin_file **pfl) throw(configuration_exception)
{
  int rv;
  glob_t gb;
  std::string globpattern;

  if(dir == "")
  {
    throw configuration_exception("invalid directory");
  }

  globpattern = dir + "/*/uof/*.uof";
  write_log("find_plugins: globbing for " + globpattern);

  autoLock glock(conf_lock);
  rv = glob(globpattern.c_str(), GLOB_NOSORT, NULL, &gb);
  glock.unlock();

  if(rv == GLOB_NOMATCH)
  {
    write_log("find_plugins: no matches found");
    return 0;
  }
  if(rv == GLOB_ABORTED || rv == GLOB_NOSPACE)
  {
    throw configuration_exception("glob failed");
  }

  unsigned int num_found = gb.gl_pathc;
  *pfl = new plugin_file[num_found];
  for(unsigned int i=0; i<gb.gl_pathc; ++i)
  {
/*
    ((*pfl)[i]).full_path = new char[strlen(gb.gl_pathv[i])+1];
    strcpy(((*pfl)[i]).full_path, gb.gl_pathv[i]);
*/
    ((*pfl)[i]).full_path = gb.gl_pathv[i];
 
    char *lastslash;
    lastslash = rindex(gb.gl_pathv[i], '/');
    if(lastslash == NULL)
    {
      throw configuration_exception("can't parse the plugin path");
    }
    lastslash++;
 
    char *dot;
    dot = index(lastslash, '.');
    if(dot == NULL)
    {
      throw configuration_exception("can't parse the plugin path");
    }
    *dot = '\0';

/*
    ((*pfl)[i]).label = new char[strlen(lastslash)+1];
    strcpy(((*pfl)[i]).label, lastslash);
*/
    ((*pfl)[i]).label = lastslash;

    write_log("find_plugins: found plugin " + ((*pfl)[i]).full_path + " with label " + ((*pfl)[i]).label);
  }

  glock.lock();
  globfree(&gb);
  glock.unlock();

  return num_found;
}

//void Configuration::add_plugin(std::string path_to_plugin, unsigned int pme) throw(configuration_exception)
void Configuration::add_plugin(std::string path_to_plugin, unsigned int pme) throw(configuration_exception)
{
  unsigned int status;

  write_log("add_plugin: add " + path_to_plugin + " on to plugin ME " + int2str(pme));

  if(pme > 4) 
  {
    throw configuration_exception("plugin ME " + int2str(pme) + " is invalid");
  }

  std::string full_path = path_to_plugin + "_" + int2str(pme) + "_" + ixp_version;
  
  struct stat fb;
  if(lstat(full_path.c_str(), &fb) == 0)
  {
    if((fb.st_mode & S_IFMT) != S_IFREG)
    {
      throw configuration_exception("plugin path " + path_to_plugin + " is invalid");
    }
  }

  autoLock glock(conf_lock);

  plugin_uof_files[pme] = full_path;

  // now get the plugin uof file into memory 
  if((status = UcLo_LoadObjFile(&plugin_uof_handles[pme], (char *)plugin_uof_files[pme].c_str())) != UCLO_SUCCESS)
  {
    throw configuration_exception("failed to load plugin code");
  }

  // have to stop the currently running MEs
  stop_mes();

  unsigned int plugin_mes = UcLo_GetAssignedMEs(plugin_uof_handles[pme]);

  // update the list of running MEs
  used_me_mask = used_me_mask | plugin_mes;
  
  // load the plugin onto the microengine
  if((status = UcLo_WriteUimageAll(plugin_uof_handles[pme])) != UCLO_SUCCESS)
  {
    start_mes();
    throw configuration_exception("failed to load the plugin code");
  }

  // restart the MEs
  start_mes();
}

void Configuration::del_plugin(unsigned int pme) throw(configuration_exception)
{
  unsigned int status;

  write_log("del_plugin: plugin me " + int2str(pme));

  if(pme > 4)
  {
    throw configuration_exception("plugin ME " + int2str(pme) + " is invalid");
  }

  autoLock glock(conf_lock);

  unsigned int plugin_mes = UcLo_GetAssignedMEs(plugin_uof_handles[pme]);

  // have to stop the plugin MEs for this plugin
  stop_mes(plugin_mes);
    
  // update the list of running MEs
  used_me_mask = used_me_mask ^ plugin_mes;

  plugin_uof_files[pme][0] = '\0';

  // remove the plugin image from memory
  if((status = UcLo_DeleObj(plugin_uof_handles[pme])) != UCLO_SUCCESS)
  {
    plugin_uof_handles[pme] = NULL;
    throw configuration_exception("failed to remove the plugin code"); 
  }
  plugin_uof_handles[pme] = NULL;

  // reset the mes
  halMe_Reset(plugin_mes, 0);
  halMe_ClrReset(plugin_mes);
  for(unsigned int me=0; me < 0x18; me++)
  {
    if(!(plugin_mes & (1<<me))) continue;
    SET_ME_CSR(me, SAME_ME_SIGNAL, (12<<3) | (0));
  }
}

void Configuration::query_route(route_key *query, route_result *result) throw(configuration_exception)
{
  unsigned int sram_addr;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  route_key *k = (route_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *tcamresult = (unsigned int *)&rbuf[4];

  write_log("query_route:");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYROUTE;
  *k = *query;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(route_key)+sizeof(tcamd::tcam_command)) != sizeof(route_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("querying the route failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no route matches that key!");
  }
  
  autoLock slock(sram_lock);
  // address is only the lower 21 bits
  sram_addr = (*tcamresult) & 0x1fffff;

  result->v[0] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR));
  result->v[1] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+4);
  result->v[2] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+8);
}

void Configuration::add_route(route_key *key, route_key *mask, route_result *result) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr;
  
  char buf[2000];
  tcamd::tcam_command *cmd = (tcamd::tcam_command *)&buf[0];
  tcamd::route_info *route = (tcamd::route_info *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (tcamd::tcam_response *)&rbuf[0];

  write_log("add_route: entering");

  *cmd = tcamd::ADDROUTE;
  route->key = *key;
  route->mask = *mask;

  // for routes, there are two cases: unicast and multicast
  // for unicast, we do LPM on destination IP addy, so TCAM entries need to be in LP first order on dest IP
  // for multicast, we do exact match on dest IP addy and LPM on source IP addy, so TCAM entries need to be in LP first order on source IP, and all multicast entries should come before the unicast entries

  autoLock glock(conf_lock);
  // get an index in the TCAM for this entry
  if(result->ip_mc_valid == 1) // multicast route
  {
    unsigned int prefix_length = get_prefix_length(mask->saddr);
    write_log("add_route: got multicast route, prefix length " + int2str(prefix_length));

    route->index = get_free_index(&mc_routes[prefix_length]);
    write_log("add_route: got tcam index " + int2str(route->index));
  }
  else // unicast route
  {
    unsigned int prefix_length = get_prefix_length(mask->daddr);
    write_log("add_route: got unicast route, prefix length " + int2str(prefix_length));

    route->index = get_free_index(&uc_routes[prefix_length]);
    write_log("add_route: got tcam index " + int2str(route->index));
  }

  // get an index into SRAM for the result
  sram_index = get_free_index(&sram_free_list); 
  write_log("add_route: got sram index " + int2str(sram_index));

  sram_addr = sram_index2addr(sram_index);
  route->result = sram_addr;
  
  unsigned int id = next_id;
  next_id++;
  add_sram_map(sram_addr, id);

  glock.unlock();

  // address stored in the TCAM result is not absolute, but we need the absolute address to write the actual result into the SRAM bank
  sram_addr = sram_addr + RESULT_BASE_ADDR;

  write_log("add_route: about to write SRAM result");

  char blah[256];
  sprintf(blah, "add_route: SRAM result word 0: 0x%.8x", result->v[0]);
  write_log(blah);
  sprintf(blah, "add_route: SRAM result word 1: 0x%.8x", result->v[1]);
  write_log(blah);
  sprintf(blah, "add_route: SRAM result word 2: 0x%.8x", result->v[2]);
  write_log(blah);
  sprintf(blah, "add_route: SRAM result word 3: 0x%.8x", route->index);
  write_log(blah);

  autoLock slock(sram_lock);
  // now write the result to SRAM before adding the TCAM entry
  SRAM_WRITE(SRAM_BANK_0, sram_addr, result->v[0]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+4, result->v[1]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+8, result->v[2]);

  // we have the room in SRAM, so I'm going to cheat and add the index to the 4th word of the rest TCAM
  SRAM_WRITE(SRAM_BANK_0, sram_addr+12, route->index);
  slock.unlock();

  write_log("add_route: about to send message to TCAM");

  autoLock tlock(tcam_lock);
  // now add the entry to the TCAM
  if(write(tcamd_conn, buf, sizeof(tcamd::route_info)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::route_info)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult != tcamd::SUCCESS)
  {
    throw configuration_exception("adding the route failed!");
  }

  write_log("add_route: done");

  return;
}

void Configuration::del_route(route_key *key) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr, prefix_len;
  tcamd::tcam_index tcamindex;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  route_key *k = (route_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *result = (unsigned int *)&rbuf[4];

  write_log("del_route: entering");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYROUTE;
  *k = *key;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(route_key)+sizeof(tcamd::tcam_command)) != sizeof(route_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the route failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no route matches that key!");
  }
  tlock.unlock();

  write_log("del_route: done with queryroute");

  // address is only the lower 21 bits
  sram_addr = (*result) & 0x1fffff;
  sram_index = sram_addr2index(sram_addr);

  autoLock slock(sram_lock);
  tcamindex = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+12);
  slock.unlock();
  if((tcamindex >= ROUTE_MC_FIRST_INDEX) && (tcamindex <= ROUTE_MC_LAST_INDEX))
  {
    prefix_len = MAX_PREFIX_LEN - ((tcamindex - ROUTE_MC_FIRST_INDEX) / ROUTE_MC_NUM_PER_CLASS);
    if((prefix_len == 1) && (tcamindex == (ROUTE_MC_FIRST_INDEX + (ROUTE_MC_NUM_PER_CLASS*(MAX_PREFIX_LEN)) - 1)))
    {
      prefix_len = 0;
    }
  }
  else
  {
    prefix_len = MAX_PREFIX_LEN - ((tcamindex - ROUTE_UC_FIRST_INDEX) / ROUTE_UC_NUM_PER_CLASS);
    if((prefix_len == 1) && (tcamindex == (ROUTE_UC_FIRST_INDEX + (ROUTE_UC_NUM_PER_CLASS*(MAX_PREFIX_LEN)) - 1)))
    {
      prefix_len = 0;
    }
  }
  
  write_log("del_route: about to do delroute");

  // now we know where the route exists in both the TCAM and SRAM

  // now delete the route from the TCAM
  *cmd = tcamd::DELROUTE;
  tcamd::tcam_index *index = (tcamd::tcam_index *)&buf[4];
  *index = tcamindex;
  
  tlock.lock();
  if(write(tcamd_conn, buf, sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }
  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the route failed!");
  }
  // delete from TCAM ok, now add TCAM index and SRAM index back to free set
  autoLock glock(conf_lock);
  add_free_index(&sram_free_list, sram_index);
  if((tcamindex >= ROUTE_MC_FIRST_INDEX) && (tcamindex <= ROUTE_MC_LAST_INDEX))
  {
    add_free_index(&mc_routes[prefix_len], tcamindex);
  }
  else
  {
    add_free_index(&uc_routes[prefix_len], tcamindex);
  }
  del_sram_map(sram_addr);

  write_log("del_route: done");

  return;
}

unsigned int Configuration::conv_str_to_uint(std::string str) throw(configuration_exception)
{
  const char* cstr = str.c_str();
  char* endptr;
  unsigned int val;
  errno = 0;
  val = strtoul(cstr, &endptr, 0);
  if((errno == ERANGE && (val == ULONG_MAX)) || (errno != 0 && val == 0))
  {
    throw configuration_exception("invalid value");
  }
  if(endptr == cstr)
  {
    throw configuration_exception("invalid value");
  }
  if(*endptr != '\0')
  {
    throw configuration_exception("invalid value");
  }
  return val;
}

unsigned int Configuration::get_proto(std::string proto_str) throw(configuration_exception)
{
  if(proto_str == "icmp" || proto_str == "ICMP")
  {
    return 1;
  }
  if(proto_str == "igmp" || proto_str == "IGMP")
  {
    return 2;
  }
  if(proto_str == "tcp" || proto_str == "tcp")
  {
    return 6;
  }
  if(proto_str == "udp" || proto_str == "udp")
  {
    return 17;
  }
  if(proto_str == "*")
  {
    return WILDCARD_VALUE;
  }
  unsigned int val;
  try
  {
    val = conv_str_to_uint(proto_str);
  }
  catch(std::exception& e)
  {
    throw configuration_exception("protocol string is not valid");
  }
  return val;
}

unsigned int Configuration::get_tcpflags(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(configuration_exception)
{
  unsigned val = 0;

  if(fin == 0 || fin == WILDCARD_VALUE) { }
  else if(fin == 1) { val = val | 1; }
  else { throw configuration_exception("tcpfin value is not valid"); }

  if(syn == 0 || syn == WILDCARD_VALUE) { }
  else if(syn == 1) { val = val | 2; }
  else { throw configuration_exception("tcpsyn value is not valid"); }

  if(rst == 0 || rst == WILDCARD_VALUE) { }
  else if(rst == 1) { val = val | 4; }
  else { throw configuration_exception("tcprst value is not valid"); }

  if(psh == 0 || psh == WILDCARD_VALUE) { }
  else if(psh == 1) { val = val | 8; }
  else { throw configuration_exception("tcppsh value is not valid"); }

  if(ack == 0 || ack == WILDCARD_VALUE) { }
  else if(ack == 1) { val = val | 16; }
  else { throw configuration_exception("tcpack value is not valid"); }

  if(urg == 0 || urg == WILDCARD_VALUE) { }
  else if(urg == 1) { val = val | 32; }
  else { throw configuration_exception("tcpurg value is not valid"); }

  return val;
}

unsigned int Configuration::get_tcpflags_mask(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(configuration_exception)
{
  unsigned val = 0;

  if(fin == WILDCARD_VALUE) { }
  else if(fin == 0 || fin == 1) { val = val | 1; }
  else { throw configuration_exception("tcpfin value is not valid"); }

  if(syn == WILDCARD_VALUE) { }
  else if(syn == 0 || syn == 1) { val = val | 2; }
  else { throw configuration_exception("tcpsyn value is not valid"); }

  if(rst == WILDCARD_VALUE) { }
  else if(rst == 0 || rst == 1) { val = val | 4; }
  else { throw configuration_exception("tcprst value is not valid"); }

  if(psh == WILDCARD_VALUE) { }
  else if(psh == 0 || psh == 1) { val = val | 8; }
  else { throw configuration_exception("tcppsh value is not valid"); }

  if(ack == WILDCARD_VALUE) { }
  else if(ack == 0 || ack == 1) { val = val | 16; }
  else { throw configuration_exception("tcpack value is not valid"); }

  if(urg == WILDCARD_VALUE) { }
  else if(urg == 0 || urg == 1) { val = val | 32; }
  else { throw configuration_exception("tcpurg value is not valid"); }

  return val;
}

unsigned int Configuration::get_exceptions(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(configuration_exception)
{
  unsigned val = 0;

  if(nonip == 0 || nonip == WILDCARD_VALUE) { }
  else if(nonip == 1) { val = val | 1; }
  else { throw configuration_exception("nonip value is not valid"); }

  if(arp == 0 || arp == WILDCARD_VALUE) { }
  else if(arp == 1) { val = val | 2; }
  else { throw configuration_exception("arp value is not valid"); }

  if(ipopt == 0 || ipopt == WILDCARD_VALUE) { }
  else if(ipopt == 1) { val = val | 4; }
  else { throw configuration_exception("ipopt value is not valid"); }

  if(ttl == 0 || ttl == WILDCARD_VALUE) { }
  else if(ttl == 1) { val = val | 8; }
  else { throw configuration_exception("ttl value is not valid"); }

  return val;
}

unsigned int Configuration::get_exceptions_mask(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(configuration_exception)
{
  unsigned val = 0;

  if(nonip == WILDCARD_VALUE) { }
  else if(nonip == 0 || nonip == 1) { val = val | 1; }
  else { throw configuration_exception("nonip value is not valid"); }

  if(arp == WILDCARD_VALUE) { }
  else if(arp == 0 || arp == 1) { val = val | 2; }
  else { throw configuration_exception("arp value is not valid"); }

  if(ipopt == WILDCARD_VALUE) { }
  else if(ipopt == 0 || ipopt == 1) { val = val | 4; }
  else { throw configuration_exception("ipopt value is not valid"); }

  if(ttl == WILDCARD_VALUE) { }
  else if(ttl == 0 || ttl == 1) { val = val | 8; }
  else { throw configuration_exception("ttl value is not valid"); }

  return val;
}

unsigned int Configuration::get_pps(std::string pps_str, bool multicast) throw(configuration_exception)
{
  if(pps_str == "plugin(unicast)")
  {
    if(multicast) { throw configuration_exception("invalid port_plugin_selection for multicast filter"); }
    return 1;
  }
  if(pps_str == "port(unicast)")
  {
    if(multicast) { throw configuration_exception("invalid port_plugin_selection for multicast filter"); }
    return 0;
  }
  if(pps_str == "ports and plugins (multicast)")
  {
    if(!multicast) { throw configuration_exception("invalid port_plugin_selection for unicast filter"); }
    return 0;
  }
  if(pps_str == "plugins(multicast)")
  {
    if(!multicast) { throw configuration_exception("invalid port_plugin_selection for unicast filter"); }
    return 1;
  }
  throw configuration_exception("invalid port_plugin_selection");
  return 0;
}

unsigned int Configuration::get_output_port(std::string port_str) throw(configuration_exception)
{
  if(port_str == "")
  {
    return 0;
  }
  if(port_str == "use route")
  {
    return FILTER_USE_ROUTE;
  }
  
  unsigned int val;
  try
  {
    val = conv_str_to_uint(port_str);
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output port is not valid");
  }
  if(val > 4)
  {
    throw configuration_exception("output port is not valid");
  }
  return val;
}

unsigned int Configuration::get_output_plugin(std::string plugin_str) throw(configuration_exception)
{
  if(plugin_str == "")
  {
    return 0;
  }

  unsigned int val;
  try
  {
    val = conv_str_to_uint(plugin_str);
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output plugin is not valid");
  }
  if(val > 4)
  {
    throw configuration_exception("output plugin is not valid");
  }
  return val;
}

unsigned int Configuration::get_outputs(std::string port_str, std::string plugin_str) throw(configuration_exception)
{
  unsigned int ports = 0;
  char portcstr[port_str.size()+1];
  strcpy(portcstr, port_str.c_str());
  char *tok, *saveptr;
  tok = strtok_r(portcstr, ",", &saveptr);
  try
  {
    while(tok)
    {
      std::string tmp = tok;
      unsigned int val = conv_str_to_uint(tmp);
      if(val > 4)
      {
        throw configuration_exception("output port is not valid");
      }
      ports = ports | (1 << val); 
    
      tok = strtok_r(NULL, ",", &saveptr);
    }
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output port is not valid");
  }

  unsigned int plugins = 0;
  char plugincstr[plugin_str.size()+1];
  strcpy(plugincstr, plugin_str.c_str());
  char *tok2, *saveptr2;
  tok2 = strtok_r(plugincstr, ",", &saveptr2);
  try
  {
    while(tok2)
    {
      std::string tmp = tok2;
      unsigned int val = conv_str_to_uint(tmp);
      if(val > 4)
      {
        throw configuration_exception("output plugin is not valid");
      }
      plugins = plugins | (1 << val);

      tok2 = strtok_r(NULL, ",", &saveptr2);
    }
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output plugin is not valid");
  }

  return ((ports << 5) | (plugins));
}

void Configuration::query_pfilter(pfilter_key *query, pfilter_result *result) throw(configuration_exception)
{
  unsigned int sram_addr;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  pfilter_key *k = (pfilter_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *tcamresult = (unsigned int *)&rbuf[4];

  write_log("query_pfilter:");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYPFILTER;
  *k = *query;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(pfilter_key)+sizeof(tcamd::tcam_command)) != sizeof(pfilter_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("querying the pfilter failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no pfilter matches that key!");
  }
  
  autoLock slock(sram_lock);
  // address is only the lower 21 bits
  sram_addr = (*tcamresult) & 0x1fffff;

  result->v[0] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR));
  result->v[1] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+4);
  result->v[2] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+8);
}

void Configuration::add_pfilter(pfilter_key *key, pfilter_key *mask, unsigned int priority, pfilter_result *result) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr;
  
  char buf[2000];
  tcamd::tcam_command *cmd = (tcamd::tcam_command *)&buf[0];
  tcamd::pfilter_info *pfilter = (tcamd::pfilter_info *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (tcamd::tcam_response *)&rbuf[0];

  write_log("add_pfilter: entering");

  if(priority > MAX_PRIORITY)
  {
    throw configuration_exception("invalid priority");
  }

  *cmd = tcamd::ADDPFILTER;
  pfilter->key = *key;
  pfilter->mask = *mask;

  autoLock glock(conf_lock);
  // get an index in the TCAM for this entry
  pfilter->index = get_free_index(&pfilters[priority]);
  write_log("add_pfilter: got tcam index " + int2str(pfilter->index));

  // get an index into SRAM for the result
  sram_index = get_free_index(&sram_free_list); 
  write_log("add_pfilter: got sram index " + int2str(sram_index));

  sram_addr = sram_index2addr(sram_index);
  pfilter->result = (priority << 21) | sram_addr;

  if(result->ip_mc_valid == 0)
  {
    unsigned int out_port_val = (result->uc_mc_bits >> 3) & 0x7;
    if(out_port_val == FILTER_USE_ROUTE)
    {
      pfilter->result = (1 << 28) | pfilter->result;
    }
  }

  unsigned int id = next_id;
  next_id++;
  add_sram_map(sram_addr, id);

  glock.unlock();

  // address stored in the TCAM result is not absolute, but we need the absolute address to write the actual result into the SRAM bank
  sram_addr = sram_addr + RESULT_BASE_ADDR;

  write_log("add_pfilter: about to write SRAM result");

  char blah[256];
  sprintf(blah, "add_pfilter: SRAM result word 0: 0x%.8x", result->v[0]);
  write_log(blah);
  sprintf(blah, "add_pfilter: SRAM result word 1: 0x%.8x", result->v[1]);
  write_log(blah);
  sprintf(blah, "add_pfilter: SRAM result word 2: 0x%.8x", result->v[2]);
  write_log(blah);
  sprintf(blah, "add_pfilter: SRAM result word 3: 0x%.8x", pfilter->index);
  write_log(blah);

  autoLock slock(sram_lock);
  // now write the result to SRAM before adding the TCAM entry
  SRAM_WRITE(SRAM_BANK_0, sram_addr, result->v[0]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+4, result->v[1]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+8, result->v[2]);

  // we have the room in SRAM, so I'm going to cheat and add the index to the 4th word of the rest TCAM
  SRAM_WRITE(SRAM_BANK_0, sram_addr+12, pfilter->index);
  slock.unlock();

  write_log("add_pfilter: about to send message to TCAM");

  autoLock tlock(tcam_lock);
  // now add the entry to the TCAM
  if(write(tcamd_conn, buf, sizeof(tcamd::pfilter_info)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::pfilter_info)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult != tcamd::SUCCESS)
  {
    throw configuration_exception("adding the pfilter failed!");
  }

  write_log("add_pfilter: done");

  return;
}

void Configuration::del_pfilter(pfilter_key *key) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr, priority;
  tcamd::tcam_index tcamindex;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  pfilter_key *k = (pfilter_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *result = (unsigned int *)&rbuf[4];

  write_log("del_pfilter: entering");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYPFILTER;
  *k = *key;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(pfilter_key)+sizeof(tcamd::tcam_command)) != sizeof(pfilter_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the pfilter failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no pfilter matches that key!");
  }
  tlock.unlock();

  write_log("del_pfilter: done with querypfilter");

  // address is only the lower 21 bits
  sram_addr = (*result) & 0x1fffff;
  sram_index = sram_addr2index(sram_addr);

  autoLock slock(sram_lock);
  tcamindex = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+12);
  slock.unlock();

  priority = MAX_PRIORITY - ((tcamindex - PFILTER_FIRST_INDEX) / PFILTER_NUM_PER_CLASS);
  
  write_log("del_pfilter: about to do delpfilter");

  // now we know where the route exists in both the TCAM and SRAM

  // now delete the route from the TCAM
  *cmd = tcamd::DELPFILTER;
  tcamd::tcam_index *index = (tcamd::tcam_index *)&buf[4];
  *index = tcamindex;
  
  tlock.lock();
  if(write(tcamd_conn, buf, sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }
  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the pfilter failed!");
  }
  // delete from TCAM ok, now add TCAM index and SRAM index back to free set
  autoLock glock(conf_lock);
  add_free_index(&sram_free_list, sram_index);
  add_free_index(&pfilters[priority], tcamindex);
  del_sram_map(sram_addr);

  write_log("del_pfilter: done");

  return;
}

void Configuration::query_afilter(afilter_key *query, afilter_result *result) throw(configuration_exception)
{
  unsigned int sram_addr;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  afilter_key *k = (afilter_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *tcamresult = (unsigned int *)&rbuf[4];

  write_log("query_afilter:");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYAFILTER;
  *k = *query;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(afilter_key)+sizeof(tcamd::tcam_command)) != sizeof(afilter_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("querying the afilter failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no afilter matches that key!");
  }
  
  autoLock slock(sram_lock);
  // address is only the lower 21 bits
  sram_addr = (*tcamresult) & 0x1fffff;

  result->v[0] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR));
  result->v[1] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+4);
  result->v[2] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+8);
}

void Configuration::add_afilter(afilter_key *key, afilter_key *mask, unsigned int priority, afilter_result *result) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr;
  
  char buf[2000];
  tcamd::tcam_command *cmd = (tcamd::tcam_command *)&buf[0];
  tcamd::afilter_info *afilter = (tcamd::afilter_info *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (tcamd::tcam_response *)&rbuf[0];

  write_log("add_afilter: entering");

  if(priority > MAX_PRIORITY)
  {
    throw configuration_exception("invalid priority");
  }

  *cmd = tcamd::ADDAFILTER;
  afilter->key = *key;
  afilter->mask = *mask;

  autoLock glock(conf_lock);
  // get an index in the TCAM for this entry
  afilter->index = get_free_index(&afilters[priority]);
  write_log("add_afilter: got tcam index " + int2str(afilter->index));

  // get an index into SRAM for the result
  sram_index = get_free_index(&sram_free_list); 
  write_log("add_afilter: got sram index " + int2str(sram_index));

  sram_addr = sram_index2addr(sram_index);
  afilter->result = sram_addr;

  if(result->out_port == FILTER_USE_ROUTE)
  {
    afilter->result = (1 << 28) | afilter->result;
  }

  unsigned int id = next_id;
  next_id++;
  add_sram_map(sram_addr, id);

  glock.unlock();

  // address stored in the TCAM result is not absolute, but we need the absolute address to write the actual result into the SRAM bank
  sram_addr = sram_addr + RESULT_BASE_ADDR;

  write_log("add_afilter: about to write SRAM result");

  char blah[256];
  sprintf(blah, "add_afilter: SRAM result word 0: 0x%.8x", result->v[0]);
  write_log(blah);
  sprintf(blah, "add_afilter: SRAM result word 1: 0x%.8x", result->v[1]);
  write_log(blah);
  sprintf(blah, "add_afilter: SRAM result word 2: 0x%.8x", result->v[2]);
  write_log(blah);
  sprintf(blah, "add_afilter: SRAM result word 3: 0x%.8x", afilter->index);
  write_log(blah);

  autoLock slock(sram_lock);
  // now write the result to SRAM before adding the TCAM entry
  SRAM_WRITE(SRAM_BANK_0, sram_addr, result->v[0]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+4, result->v[1]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+8, result->v[2]);

  // we have the room in SRAM, so I'm going to cheat and add the index to the 4th word of the rest TCAM
  SRAM_WRITE(SRAM_BANK_0, sram_addr+12, afilter->index);
  slock.unlock();

  write_log("add_afilter: about to send message to TCAM");

  autoLock tlock(tcam_lock);
  // now add the entry to the TCAM
  if(write(tcamd_conn, buf, sizeof(tcamd::afilter_info)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::afilter_info)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult != tcamd::SUCCESS)
  {
    throw configuration_exception("adding the afilter failed!");
  }

  write_log("add_afilter: done");

  return;
}

void Configuration::del_afilter(afilter_key *key) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr, priority;
  tcamd::tcam_index tcamindex;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  afilter_key *k = (afilter_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *result = (unsigned int *)&rbuf[4];

  write_log("del_afilter: entering");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYAFILTER;
  *k = *key;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(afilter_key)+sizeof(tcamd::tcam_command)) != sizeof(afilter_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the afilter failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no afilter matches that key!");
  }
  tlock.unlock();

  write_log("del_afilter: done with queryafilter");

  // address is only the lower 21 bits
  sram_addr = (*result) & 0x1fffff;
  sram_index = sram_addr2index(sram_addr);

  autoLock slock(sram_lock);
  tcamindex = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+12);
  slock.unlock();

  priority = MAX_PRIORITY - ((tcamindex - AFILTER_FIRST_INDEX) / AFILTER_NUM_PER_CLASS);
  
  write_log("del_afilter: about to do delafilter");

  // now we know where the route exists in both the TCAM and SRAM

  // now delete the route from the TCAM
  *cmd = tcamd::DELAFILTER;
  tcamd::tcam_index *index = (tcamd::tcam_index *)&buf[4];
  *index = tcamindex;
  
  tlock.lock();
  if(write(tcamd_conn, buf, sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }
  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the afilter failed!");
  }
  // delete from TCAM ok, now add TCAM index and SRAM index back to free set
  autoLock glock(conf_lock);
  add_free_index(&sram_free_list, sram_index);
  add_free_index(&afilters[priority], tcamindex);
  del_sram_map(sram_addr);

  write_log("del_afilter: done");

  return;
}

void Configuration::get_sample_rates(aux_sample_rates *rates) throw()
{
  unsigned int addr = COPY_BLOCK_CONTROL_MEM_BASE;

  write_log("get_sample_rates:");
  
  autoLock slock(sram_lock);

  rates->rate00 = SRAM_READ(SRAM_BANK_3, addr);
  rates->rate01 = SRAM_READ(SRAM_BANK_3, addr+4);
  rates->rate10 = SRAM_READ(SRAM_BANK_3, addr+8);
  rates->rate11 = SRAM_READ(SRAM_BANK_3, addr+12);
}

void Configuration::set_sample_rates(aux_sample_rates *rates) throw()
{
  unsigned int addr = COPY_BLOCK_CONTROL_MEM_BASE;

  write_log("set_sample_rates:");

  autoLock slock(sram_lock);

  SRAM_WRITE(SRAM_BANK_3, addr, rates->rate00);
  SRAM_WRITE(SRAM_BANK_3, addr+4, rates->rate01);
  SRAM_WRITE(SRAM_BANK_3, addr+8, rates->rate10);
  SRAM_WRITE(SRAM_BANK_3, addr+12, rates->rate11);
}

void Configuration::query_arp(arp_key *query, arp_result *result) throw(configuration_exception)
{
  unsigned int sram_addr;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  arp_key *k = (arp_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *tcamresult = (unsigned int *)&rbuf[4];

  write_log("query_arp:");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYARP;
  *k = *query;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(arp_key)+sizeof(tcamd::tcam_command)) != sizeof(arp_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("arp query failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no arp entry matches that key!");
  }
  
  autoLock slock(sram_lock);
  // address is only the lower 21 bits
  sram_addr = (*tcamresult) & 0x1fffff;

  result->v[0] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR));
  result->v[1] = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+4);
}

void Configuration::update_arp(arp_key *query, arp_result *result) throw(configuration_exception)
{
  unsigned int sram_addr;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  arp_key *k = (arp_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *tcamresult = (unsigned int *)&rbuf[4];

  write_log("update_arp:");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYARP;
  *k = *query;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(arp_key)+sizeof(tcamd::tcam_command)) != sizeof(arp_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("arp query failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no arp entry matches that key!");
  }
  
  autoLock slock(sram_lock);
  // address is only the lower 21 bits
  sram_addr = (*tcamresult) & 0x1fffff;

  SRAM_WRITE(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR), result->v[0]);
  SRAM_WRITE(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+4, result->v[1]);
}

void Configuration::add_arp(arp_key *key, arp_key *mask, arp_result *result) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr;

  char buf[2000];
  tcamd::tcam_command *cmd = (tcamd::tcam_command *)&buf[0];
  tcamd::arp_info *arp = (tcamd::arp_info *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (tcamd::tcam_response *)&rbuf[0];

  write_log("in add_arp");

  *cmd = tcamd::ADDARP;
  arp->key = *key;
  arp->mask = *mask;
  arp->mask.daddr = 0xffffffff;
  arp->mask.res1 = 0x0;
  arp->mask.res2 = 0x0;

  // for arp entries, there is no ordering or anything, just one big list

  autoLock glock(conf_lock);
  // get an index in the TCAM for this entry
  arp->index = get_free_index(&arps);

  // get an index into SRAM for the result
  sram_index = get_free_index(&sram_free_list); 
  glock.unlock();
  sram_addr = sram_index2addr(sram_index);
  arp->result = sram_addr;

  // address stored in the TCAM result is not absolute, but we need the absolute address to write the actual result into the SRAM bank
  sram_addr = sram_addr + RESULT_BASE_ADDR;

  autoLock slock(sram_lock);
  // now write the result to SRAM before adding the TCAM entry
  SRAM_WRITE(SRAM_BANK_0, sram_addr, result->v[0]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+4, result->v[1]);
  SRAM_WRITE(SRAM_BANK_0, sram_addr+8, 0);

  // we have the room in SRAM, so I'm going to cheat and add the index to the 4th word of the rest TCAM
  SRAM_WRITE(SRAM_BANK_0, sram_addr+12, arp->index);
  slock.unlock();

  autoLock tlock(tcam_lock);
  // now add the entry to the TCAM
  if(write(tcamd_conn, buf, sizeof(tcamd::arp_info)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::arp_info)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult != tcamd::SUCCESS)
  {
    throw configuration_exception("adding the arp entry failed!");
  }

  return;
}

void Configuration::del_arp(arp_key *key) throw(configuration_exception)
{
  unsigned int sram_index, sram_addr;
  tcamd::tcam_index tcamindex;

  char buf[2000];
  tcamd::tcam_command *cmd = (unsigned int *)&buf[0];
  arp_key *k = (arp_key *)&buf[4];

  char rbuf[2000];
  int bytes_read;
  tcamd::tcam_response *cmdresult = (unsigned int *)&rbuf[0];
  tcamd::tcam_result *result = (unsigned int *)&rbuf[4];

  write_log("in del_arp");

  // first have to query the database with the key to get the address of the SRAM result
  *cmd = tcamd::QUERYARP;
  *k = *key;

  autoLock tlock(tcam_lock);
  if(write(tcamd_conn, buf, sizeof(arp_key)+sizeof(tcamd::tcam_command)) != sizeof(arp_key)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response) + sizeof(tcamd::tcam_result))
  {
    throw configuration_exception("response from tcamd too small");
  }

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the arp entry failed!");
  }
  else if(*cmdresult == tcamd::NOTFOUND)
  {
    throw configuration_exception("no arp entry matches that key!");
  }
  tlock.unlock();

  // address is only the lower 21 bits
  sram_addr = (*result) & 0x1fffff;
  sram_index = sram_addr2index(sram_addr);

  autoLock slock(sram_lock);
  tcamindex = SRAM_READ(SRAM_BANK_0, (sram_addr+RESULT_BASE_ADDR)+12);
  slock.unlock();
  
  // now we know where the arp entry exists in both the TCAM and SRAM

  // now delete the route from the TCAM
  *cmd = tcamd::DELARP;
  tcamd::tcam_index *index = (tcamd::tcam_index *)&buf[4];
  *index = tcamindex;
  
  tlock.lock();
  if(write(tcamd_conn, buf, sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command)) != sizeof(tcamd::tcam_index)+sizeof(tcamd::tcam_command))
  {
    throw configuration_exception("write to tcamd failed!");
  }

  bytes_read = read(tcamd_conn, rbuf, 2000);
  if(bytes_read < 1)
  {
    throw configuration_exception("read failed");
  }
  else if(bytes_read != sizeof(tcamd::tcam_response))
  {
    throw configuration_exception("response from tcamd too small");
  }
  tlock.unlock();

  if(*cmdresult == tcamd::FAILURE)
  {
    throw configuration_exception("deleting the arp entry failed!");
  }
  // delete from TCAM ok, now add TCAM index and SRAM index back to free set
  autoLock glock(conf_lock);
  add_free_index(&sram_free_list, sram_index);
  add_free_index(&arps, tcamindex);

  return;
}

unsigned int Configuration::get_queue_quantum(unsigned int port, unsigned int queue) throw()
{
  unsigned int quantum;
  unsigned int addr = QPARAMS_BASE_ADDR;

  queue = queue | ((port+1) << 13);
  write_log("get_queue_quantum: queue " + int2str(queue));

  autoLock slock(sram_lock);

  addr += queue * QPARAMS_UNIT_SIZE;
  quantum = SRAM_READ(SRAM_BANK_1, addr+8);
  return quantum;
}

void Configuration::set_queue_quantum(unsigned int port, unsigned int queue, unsigned int quantum) throw()
{
  unsigned int addr = QPARAMS_BASE_ADDR;

  queue = queue | ((port+1) << 13);
  write_log("set_queue_quantum: queue " + int2str(queue) + ", quantum " + int2str(quantum));

  autoLock slock(sram_lock);

  addr += queue * QPARAMS_UNIT_SIZE;
  SRAM_WRITE(SRAM_BANK_1, addr+8, quantum);
}

unsigned int Configuration::get_queue_threshold(unsigned int port, unsigned int queue) throw()
{
  unsigned int threshold;
  unsigned int addr = QPARAMS_BASE_ADDR;

  queue = queue | ((port+1) << 13);
  write_log("get_queue_threshold: queue " + int2str(queue));

  autoLock slock(sram_lock);

  addr += queue * QPARAMS_UNIT_SIZE;
  threshold = SRAM_READ(SRAM_BANK_1, addr+4);
  return threshold;
}

void Configuration::set_queue_threshold(unsigned int port, unsigned int queue, unsigned int threshold) throw()
{
  unsigned int addr = QPARAMS_BASE_ADDR;

  queue = queue | ((port+1) << 13);
  write_log("set_queue_threshold: queue " + int2str(queue) + ", threshold " + int2str(threshold));

  autoLock slock(sram_lock);

  addr += queue * QPARAMS_UNIT_SIZE;
  SRAM_WRITE(SRAM_BANK_1, addr+4, threshold);
}

// rates are in Kbps
void Configuration::get_port_rates(port_rates *rates) throw()
{
  unsigned int addr = QPORT_RATES_BASE_ADDR;
  unsigned int z;

  write_log("get_port_rates:");
  
  autoLock slock(sram_lock);

  z = SRAM_READ(SRAM_BANK_1, addr);
  rates->port0_rate = z * QPORT_RATE_CONVERSION;
  addr += 4;

  z = SRAM_READ(SRAM_BANK_1, addr);
  rates->port1_rate = z * QPORT_RATE_CONVERSION;
  addr += 4;

  z = SRAM_READ(SRAM_BANK_1, addr);
  rates->port2_rate = z * QPORT_RATE_CONVERSION;
  addr += 4;

  z = SRAM_READ(SRAM_BANK_1, addr);
  rates->port3_rate = z * QPORT_RATE_CONVERSION;
  addr += 4;

  z = SRAM_READ(SRAM_BANK_1, addr);
  rates->port4_rate = z * QPORT_RATE_CONVERSION;
  addr += 4;
}

// rates are in Kbps
void Configuration::set_port_rates(port_rates *rates) throw()
{
  unsigned int addr = QPORT_RATES_BASE_ADDR;
  unsigned int z;

  write_log("set_port_rates:");
  
  autoLock slock(sram_lock);

  z = rates->port0_rate / QPORT_RATE_CONVERSION;
  SRAM_WRITE(SRAM_BANK_1, addr, z);
  addr += 4;

  z = rates->port1_rate / QPORT_RATE_CONVERSION;
  SRAM_WRITE(SRAM_BANK_1, addr, z);
  addr += 4;

  z = rates->port2_rate / QPORT_RATE_CONVERSION;
  SRAM_WRITE(SRAM_BANK_1, addr, z);
  addr += 4;

  z = rates->port3_rate / QPORT_RATE_CONVERSION;
  SRAM_WRITE(SRAM_BANK_1, addr, z);
  addr += 4;

  z = rates->port4_rate / QPORT_RATE_CONVERSION;
  SRAM_WRITE(SRAM_BANK_1, addr, z);
  addr += 4;
}

// rates should be in Kbps
unsigned int Configuration::get_port_rate(unsigned int port) throw(configuration_exception)
{
  write_log("get_port_rate:");

  if(port > 4)
  {
    throw configuration_exception("invalid port");
  }

  unsigned int addr = QPORT_RATES_BASE_ADDR + (port*4);

  autoLock slock(sram_lock);

  unsigned int z = SRAM_READ(SRAM_BANK_1, addr);

  return (z * QPORT_RATE_CONVERSION);
}

// rates should be in Kbps
void Configuration::set_port_rate(unsigned int port, unsigned int rate) throw(configuration_exception)
{
  write_log("set_port_rate:");

  if(port > 4)
  {
    throw configuration_exception("invalid port");
  } 
  
  unsigned int addr = QPORT_RATES_BASE_ADDR + (port*4);

  autoLock slock(sram_lock);

  unsigned int z = rate / QPORT_RATE_CONVERSION;
  SRAM_WRITE(SRAM_BANK_1, addr, z);
}

void Configuration::get_mux_quanta(mux_quanta *quanta) throw()
{
  unsigned int addr = SCR_MUX_MEMORY_BASE;

  write_log("get_mux_quanta:");
  
  autoLock scrlock(scratch_lock);

  quanta->rx_quantum = SCRATCH_READ(addr);
  quanta->plugin_quantum = SCRATCH_READ(addr+4);
  quanta->xscaleqm_quantum = SCRATCH_READ(addr+8);
  quanta->xscalepl_quantum = SCRATCH_READ(addr+12);
}

void Configuration::set_mux_quanta(mux_quanta *quanta) throw()
{
  unsigned int addr = SCR_MUX_MEMORY_BASE;

  write_log("set_mux_quanta:");
  
  autoLock scrlock(scratch_lock);

  SCRATCH_WRITE(addr, quanta->rx_quantum);
  SCRATCH_WRITE(addr+4, quanta->plugin_quantum);
  SCRATCH_WRITE(addr+8, quanta->xscaleqm_quantum);
  SCRATCH_WRITE(addr+12, quanta->xscalepl_quantum);
}

void Configuration::get_exception_dests(exception_dests *dests) throw()
{
  unsigned int addr = COPY_BLOCK_CONTROL_MEM_BASE;
  unsigned int val;

  write_log("get_exception_dests:");
  
  autoLock slock(sram_lock);

  val = SRAM_READ(SRAM_BANK_3, addr+16);

  dests->no_route = (val >> 29) & 0x7;
  dests->arp_needed = (val >> 26) & 0x7;
  dests->nh_invalid = (val >> 23) & 0x7;
}

void Configuration::set_exception_dests(exception_dests *dests) throw()
{
  unsigned int addr = COPY_BLOCK_CONTROL_MEM_BASE;
  unsigned int val;

  write_log("set_exception_dests: entering");
  
  autoLock slock(sram_lock);

  val = (dests->no_route << 29) | (dests->arp_needed << 26) | (dests->nh_invalid << 23);
  SRAM_WRITE(SRAM_BANK_3, addr+16, val);

  write_log("set_exception_dests: done");
}

unsigned int Configuration::get_port_mac_addr_hi16(unsigned int port) throw(configuration_exception)
{
  if(port > 4)
  {
    throw configuration_exception("invalid port");
  }

  return macs.port_hi16[port];
}

unsigned int Configuration::get_port_mac_addr_low32(unsigned int port) throw(configuration_exception)
{
  if(port > 4)
  {
    throw configuration_exception("invalid port");
  }

  return macs.port_low32[port];
}

unsigned int Configuration::get_port_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return port_addrs[port];
}

unsigned int Configuration::get_next_hop_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return next_hops[port];
}

void Configuration::set_username(std::string un) throw()
{
  username = un;
  write_log("set_username: got user name " + username);
}

std::string Configuration::get_username() throw()
{
  return username;
}

void Configuration::start_mes() throw(configuration_exception)
{
  for(unsigned int me=0; me < 0x18; me++)
  {
    if(!(used_me_mask & (1<<me))) continue;
    if(halMe_Start(me, ME_ALL_CTX) != HALME_SUCCESS)
    {
      throw configuration_exception("failed to state microengine " + int2str(me));
    }
  }
}

void Configuration::stop_mes(unsigned int me_mask) throw(configuration_exception)
{
  for(unsigned int me=0; me < 0x18; me++)
  {
    if(!(me_mask & (1<<me))) continue;
    if(halMe_Stop(me, ME_ALL_CTX) != HALME_SUCCESS)
    {
      throw configuration_exception("failed to stop microengine " + int2str(me));
    }
  }
}

void Configuration::stop_mes() throw(configuration_exception)
{
  stop_mes(used_me_mask);
}

void Configuration::add_sram_map(unsigned int addr, unsigned int id) throw()
{
  write_log("add_sram_map: adding map addr " + int2str(addr) + " -> id " + int2str(id));

/*
  if(srmap == NULL)
  {
    srmap = new sram_result_map();
    srmap->sram_addr = addr;
    srmap->id = id;
    srmap->next = NULL;
    return;
  }
*/
  sram_result_map *node = srmap; 
  srmap = new sram_result_map();
  srmap->sram_addr = addr;
  srmap->id = id;
  srmap->next = node;
}

void Configuration::del_sram_map(unsigned int addr) throw(configuration_exception)
{
  write_log("del_sram_map: addr " + int2str(addr));

  sram_result_map *node = srmap;
  sram_result_map *last = NULL;
  while(node != NULL && node->sram_addr != addr)
  {
    last = node;
    node = node->next;
  }
  if(node == NULL)
  {
    throw configuration_exception("no such addr in list!");
  }
  if(last == NULL)
  {
    srmap = srmap->next;
  }
  else
  {
    last->next = node->next;
  }
  delete node;
}

unsigned int Configuration::get_sram_map(unsigned int addr) throw(configuration_exception)
{
  write_log("get_sram_map: addr " + int2str(addr));

  autoLock glock(conf_lock);

  sram_result_map *node = srmap;
  while(node != NULL && node->sram_addr != addr)
  {
    node = node->next;
  }
  if(node == NULL)
  {
    throw configuration_exception("no such addr in list!");
  }
  return node->id;
}

void Configuration::increment_drop_counter() throw()
{
  unsigned int val = (0xA << 28) | DROP_COUNTER;
  if(SCRATCH_RING_GET_IS_FULL_STATUS(COUNTER_RING))
  {
    halMe_AtomicAdd(HALME_MEM_SCRATCH, XSCALE_STATS_DROPPED_REQUESTS_ADDR, 1);
  }
  else
  {
    SCRATCH_RING_PUT(COUNTER_RING, val);
  }
}

void Configuration::update_stats_index(unsigned int index, unsigned int data) throw()
{
  unsigned int val = (0x3 << 28) | ((data & 0xfff) << 16) | (index & 0xffff);
  if(SCRATCH_RING_GET_IS_FULL_STATUS(COUNTER_RING))
  {
    halMe_AtomicAdd(HALME_MEM_SCRATCH, XSCALE_STATS_DROPPED_REQUESTS_ADDR, 1);
  }
  else
  {
    SCRATCH_RING_PUT(COUNTER_RING, val);
  }
}

unsigned int Configuration::get_prefix_length(unsigned int addr_mask) throw()
{
  unsigned int length = 0;
  for(int i=31; i>=0; --i)
  {
    if((addr_mask & (1<<i)) != 0)
    {
      ++length;
    }
    else
    {
      break;
    }
  }
  return length;
}

unsigned int Configuration::get_free_index(indexlist **head) throw(configuration_exception)
{
  indexlist *node;
  unsigned int index;

  if((*head) == NULL)
  {
    throw configuration_exception("no more free TCAM entries");
  }

  // we simply take the index from the left side of the first free range
  index = (*head)->left;

  // if the range has more entries, simply update the left value
  if((*head)->left != (*head)->right)
  {
    (*head)->left++;
  }
  // otherwise, remove the current node from the list
  else
  {
    node = *head;
    *head = (*head)->next;
    delete node;
  }

  return index;
}

void Configuration::add_free_index(indexlist **head, unsigned int index) throw()
{
  indexlist *node, *last, *tmp;

  if((*head) == NULL)
  {
    *head = new indexlist();
    (*head)->left = index;
    (*head)->right = index;
    (*head)->next = NULL;
  }
  else
  {
    last = *head;
    node = *head;
    while((node != NULL) && (node->right < index))
    {
      last = node;
      node = node->next;
    }

    if(last->right == (index - 1))
    {
      last->right = index;
      if((node != NULL) && (node->left == (index + 1)))
      {
        last->right = node->right;
        last->next = node->next;
        delete node;
      }
    }
    else if(node == NULL)
    {
      node = new indexlist();
      node->left = index;
      node->right = index;
      node->next = NULL;
      last->next = node;
    }
    else if(node->left == (index + 1))
    {
      node->left = index;
    }
    else
    {
      tmp = new indexlist();
      tmp->left = index;
      tmp->right = index;
      tmp->next = node;
      if(last == node)
      {
        *head = tmp;
      }
      else
      {
        last->next = tmp;
      }
    }
  }
}

void Configuration::free_list_mem(indexlist **list) throw()
{
  indexlist *tmp;
  while(*list != NULL)
  {
    tmp = *list;
    (*list) = (*list)->next;
    delete tmp;
  }
} 

unsigned int Configuration::sram_index2addr(unsigned int index) throw()
{
  return (index*RESULT_UNIT_SIZE);
}

unsigned int Configuration::sram_addr2index(unsigned int addr) throw()
{
  return (addr/RESULT_UNIT_SIZE);
}
