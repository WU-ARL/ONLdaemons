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
#include <exception>

#include <unistd.h> 
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ioctl.h>
  
#include "halMev2Api.h"
#include "hal_sram.h"
#include "hal_scratch.h"
#include "uclo.h"

#if (IOSTYLE != HARDWARE)
#error IOSTYLE must be HARDWARE
#endif

#define SCR_MUX_MEMORY_BASE 0x80
#define SCR_MUX_MEMORY_SIZE 0x10
#define SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE 0x100
#define SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_NUM  18

#define QM_NUM_PORTS 5
#define QM_NUM_QUEUES 0x10000

/* SRAM bank 2 */
#define BUF_DESC_BASE 0
#define BUF_DESC_UNIT_SIZE 32
#define BUF_DESC_NUM 0x38000
#define BUF_DESC_ARRAY_SIZE (BUF_DESC_UNIT_SIZE * BUF_DESC_NUM)

#define QD_BASE_ADDR (BUF_DESC_BASE + BUF_DESC_ARRAY_SIZE)
#define QD_UNIT_SIZE 16
#define QD_ARRAY_SIZE (QM_NUM_QUEUES * 16)
/* end SRAM bank 2 */

/* SRAM bank 1 */
#define QPARAMS_BASE_ADDR 0
#define QPARAMS_UNIT_SIZE 16
#define QPARAMS_ARRAY_SIZE (QM_NUM_QUEUES * 16)

#define QM1_QSCHED_BASE_ADDR (QPARAMS_BASE_ADDR + QPARAMS_ARRAY_SIZE)
#define QSCHED_UNIT_SIZE 44
#define QSCHED_NUM 13109
#define QM2_QSCHED_BASE_ADDR (QM1_QSCHED_BASE_ADDR + (QSCHED_NUM * QSCHED_UNIT_SIZE))
#define QSCHED_ARRAY_SIZE (2*(QSCHED_NUM * QSCHED_UNIT_SIZE))
#define QM1_QFREELIST_BASE_ADDR (QM1_QSCHED_BASE_ADDR + QSCHED_ARRAY_SIZE)
#define QFREELIST_UNIT_SIZE 4
#define QFREELIST_NUM QSCHED_NUM
#define QM2_QFREELIST_BASE_ADDR (QM1_QFREELIST_BASE_ADDR + (QFREELIST_UNIT_SIZE * QFREELIST_NUM))
#define QFREELIST_ARRAY_SIZE (2*(QFREELIST_NUM * QFREELIST_UNIT_SIZE))

#define QPORT_RATES_BASE_ADDR (QM1_QFREELIST_BASE_ADDR + QFREELIST_ARRAY_SIZE)
#define QPORT_RATES_UNIT_SIZE 4
#define QPORT_RATES_NUM QM_NUM_PORTS
#define QPORT_RATES_ARRAY_SIZE (QPORT_RATES_NUM * QPORT_RATES_UNIT_SIZE)
/* end SRAM bank 1 */

/* SRAM bank 3 */
#define ONL_ROUTER_RESERVED_FOR_COMPILER_BASE 0
#define ONL_ROUTER_RESERVED_FOR_COMPILER_SIZE 0x10000
#define COPY_BLOCK_CONTROL_MEM_BASE (ONL_ROUTER_RESERVED_FOR_COMPILER_BASE + ONL_ROUTER_RESERVED_FOR_COMPILER_SIZE)
#define COPY_BLOCK_CONTROL_MEM_SIZE 0x200

#define HF_BLOCK_CONTROL_MEM_BASE (COPY_BLOCK_CONTROL_MEM_BASE + COPY_BLOCK_CONTROL_MEM_SIZE)
#define HF_BLOCK_CONTROL_MEM_SIZE 0x100

#define ONL_ROUTER_PLUGIN_0_BASE 0x100000
#define ONL_ROUTER_PLUGIN_0_SIZE 0x100000
#define ONL_ROUTER_PLUGIN_1_BASE (ONL_ROUTER_PLUGIN_0_BASE + ONL_ROUTER_PLUGIN_0_SIZE)
#define ONL_ROUTER_PLUGIN_1_SIZE 0x100000
#define ONL_ROUTER_PLUGIN_2_BASE (ONL_ROUTER_PLUGIN_1_BASE + ONL_ROUTER_PLUGIN_1_SIZE)
#define ONL_ROUTER_PLUGIN_2_SIZE 0x100000
#define ONL_ROUTER_PLUGIN_3_BASE (ONL_ROUTER_PLUGIN_2_BASE + ONL_ROUTER_PLUGIN_2_SIZE)
#define ONL_ROUTER_PLUGIN_3_SIZE 0x100000
#define ONL_ROUTER_PLUGIN_4_BASE (ONL_ROUTER_PLUGIN_3_BASE + ONL_ROUTER_PLUGIN_3_SIZE)
#define ONL_ROUTER_PLUGIN_4_SIZE 0x100000
#define ONL_ROUTER_STATS_BASE (ONL_ROUTER_PLUGIN_4_BASE + ONL_ROUTER_PLUGIN_4_SIZE)
#define ONL_ROUTER_STATS_NUM 0x00010000
#define ONL_ROUTER_STATS_UNIT_SIZE 16
#define ONL_ROUTER_STATS_SIZE   (ONL_ROUTER_STATS_NUM * ONL_ROUTER_STATS_UNIT_SIZE)

#define ONL_ROUTER_REGISTERS_BASE (ONL_ROUTER_STATS_BASE + ONL_ROUTER_STATS_SIZE)
#define ONL_ROUTER_REGISTERS_NUM 64
#define ONL_ROUTER_REGISTERS_UNIT_SIZE 4
#define ONL_ROUTER_REGISTERS_SIZE   (ONL_ROUTER_REGISTERS_NUM * ONL_ROUTER_REGISTERS_UNIT_SIZE)
/* end SRAM bank 3 */

void print_usage(char *);
std::string int2str(unsigned int);

void print_usage(char *prog)
{
  std::cout << prog << " [--help] [--base router_base_uof_file] [--plugin plugin_uof_file]\n"
            << "\t--help: print this message\n"
            << "\t--base router_base_uof_file: default is HW_NPUA.uof\n"
            << "\t--plugin plugin_uof_file: default is null.uof\n";
  exit(0);
}

std::string int2str(unsigned int i)
{
  std::string s;
  std::ostringstream o;
  o << i;
  s = o.str();
  return s;
}

int main(int argc, char **argv)
{
  std::string base_uof = "HW_NPUA.uof";
  std::string plugin_uof = "null.uof";
  char input[10];

  void *base_handle;
  void *plugin_handle;
  unsigned int base_me_mask;
  unsigned int plugin_me_mask;
  unsigned int me;
  unsigned int ctx_mask = ME_ALL_CTX;

  int status;

  unsigned int addr, endaddr;

  static struct option longopts[] =
  {{"help",   0, NULL, 'h'},
   {"base",   1, NULL, 'b'},
   {"plugin", 1, NULL, 'p'},
   {NULL,     0, NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "hb:p:", longopts, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        break;
      case 'b':
        base_uof = optarg;
        break;
      case 'p':
        plugin_uof = optarg;
        break;
      default:
        print_usage(argv[0]);
        exit(1);
    }
  }

  std::cout << "Calling halMe_Init...";
  if((status = halMe_Init(0xff00ff)) != HALME_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Clearing SRAM ring occupancy counters in scratch...";
  addr = SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE;
  for(int i=0; i<SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_NUM; i++)
  {
    SCRATCH_WRITE(addr, 0);
    addr += 4;
  }
  std::cout << "done.\n";

  std::cout << "Clearing MUX scratch memory...";
  addr = SCR_MUX_MEMORY_BASE;
  for(int i=0; i<SCR_MUX_MEMORY_SIZE; i++)
  {
    SCRATCH_WRITE(addr, 0);
    addr += 4;
  }
  std::cout << "done.\n";

  std::cout << "Initializing MUX policy...";
  addr = SCR_MUX_MEMORY_BASE;
  SCRATCH_WRITE(addr, 0x100);
  addr += 4;
  SCRATCH_WRITE(addr, 0x80);
  addr += 4;
  SCRATCH_WRITE(addr, 0x80);
  std::cout << "done.\n";

  std::cout << "Initializing port rates...";
  addr = QPORT_RATES_BASE_ADDR;
  endaddr = QPORT_RATES_BASE_ADDR + 2*QPORT_RATES_ARRAY_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(1,addr,1380);
    addr += 4;
  }
  std::cout << "done.\n";
  
  std::cout << "Zeroing out some queue descriptors for each port...";
  for(int i=0; i<=4; i++)
  {
    unsigned int base_qid = ((i+1) << 13);
    addr = (QD_BASE_ADDR + (base_qid * QD_UNIT_SIZE));
    endaddr = (addr + (128 * QD_UNIT_SIZE));
    while (addr < endaddr)
    {
      SRAM_WRITE(2,addr,0);
      addr += 4;
    }
  }
  std::cout << "done.\n";

  std::cout << "Setting up a few queues for each port...";
  for(int i=0; i<=4; i++)
  {
    unsigned int base_qid = ((i+1) << 13);
    addr = (QPARAMS_BASE_ADDR + (base_qid * QPARAMS_UNIT_SIZE));
    endaddr = (addr + (128 * QPARAMS_UNIT_SIZE));
    unsigned int addr1 = addr;
    unsigned int addr2 = addr + 4;
    unsigned int addr3 = addr + 8;
    unsigned int addr4 = addr + 12;
    while (addr1 < endaddr)
    {
      SRAM_WRITE(1,addr1,0);
      SRAM_WRITE(1,addr2,0xA0000);
      SRAM_WRITE(1,addr3,0x100);
      SRAM_WRITE(1,addr4,0);
      addr1 += QPARAMS_UNIT_SIZE;
      addr2 += QPARAMS_UNIT_SIZE;
      addr3 += QPARAMS_UNIT_SIZE;
      addr4 += QPARAMS_UNIT_SIZE;
    }
  }
  std::cout << "done.\n";

  std::cout << "Initializing HF control block (RTM mac addys)...";
  addr = HF_BLOCK_CONTROL_MEM_BASE;
  SRAM_WRITE(3,addr,0);
  addr += 4;
  SRAM_WRITE(3,addr,0x503312EA);
  addr += 4;
  SRAM_WRITE(3,addr,0);
  addr += 4;
  SRAM_WRITE(3,addr,0x503312EB);
  addr += 4;
  SRAM_WRITE(3,addr,0);
  addr += 4;
  SRAM_WRITE(3,addr,0x503312EC);
  addr += 4;
  SRAM_WRITE(3,addr,0);
  addr += 4;
  SRAM_WRITE(3,addr,0x503312ED);
  addr += 4;
  SRAM_WRITE(3,addr,0);
  addr += 4;
  SRAM_WRITE(3,addr,0x503312EE);
  std::cout << "done.\n";

  std::cout << "Initializing Copy control block (sampling params and exception dests)...";
  addr = COPY_BLOCK_CONTROL_MEM_BASE;
  SRAM_WRITE(3,addr,0x7FFF);
  addr += 4;
  SRAM_WRITE(3,addr,0x4000);
  addr += 4;
  SRAM_WRITE(3,addr,0x0CCC);
  addr += 4;
  SRAM_WRITE(3,addr,0x0147);
  addr += 4;
  unsigned int val = ((7 << 29) + (5 << 26) + (5 << 23));
  SRAM_WRITE(3,addr,val);
  std::cout << "done.\n";

  std::cout << "Zeroing out the Stats memory block...";
  addr = ONL_ROUTER_STATS_BASE;
  endaddr = addr + ONL_ROUTER_STATS_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(3,addr,0);
    addr += 4;
  }
  std::cout << "done.\n";

  std::cout << "Zeroing out the Global registers...";
  addr = ONL_ROUTER_REGISTERS_BASE;
  endaddr = addr + ONL_ROUTER_REGISTERS_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(3,addr,0);
    addr += 4;
  }
  std::cout << "done.\n";

  std::cout << "Zeroing out plugin scratch memory in SRAM...";
  addr = ONL_ROUTER_PLUGIN_0_BASE;
  endaddr = addr + ONL_ROUTER_PLUGIN_0_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(3,addr,0);
    addr += 4;
  }
  addr = ONL_ROUTER_PLUGIN_1_BASE;
  endaddr = addr + ONL_ROUTER_PLUGIN_1_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(3,addr,0);
    addr += 4;
  }
  addr = ONL_ROUTER_PLUGIN_2_BASE;
  endaddr = addr + ONL_ROUTER_PLUGIN_2_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(3,addr,0);
    addr += 4;
  }
  addr = ONL_ROUTER_PLUGIN_3_BASE;
  endaddr = addr + ONL_ROUTER_PLUGIN_3_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(3,addr,0);
    addr += 4;
  }
  addr = ONL_ROUTER_PLUGIN_4_BASE;
  endaddr = addr + ONL_ROUTER_PLUGIN_4_SIZE;
  while(addr < endaddr)
  {
    SRAM_WRITE(3,addr,0);
    addr += 4;
  }
  std::cout << "done.\n";

  std::cout << "Setting up some route lookup results in SRAM bank 0...";
  addr = 0x2000000;
  SRAM_WRITE(0,addr+16,0xA0080010);
  SRAM_WRITE(0,addr+20,0x00019B78);
  SRAM_WRITE(0,addr+24,0x00E0815E);
  SRAM_WRITE(0,addr+28,0x00000000);

  SRAM_WRITE(0,addr+32,0xA0000011);
  SRAM_WRITE(0,addr+36,0x00029A72);
  SRAM_WRITE(0,addr+40,0x00E0815E);
  SRAM_WRITE(0,addr+44,0x00000000);
  std::cout << "done.\n";

  std::cout << "Calling UcLo_LoadObjFile for the router base...";
  if((status = UcLo_LoadObjFile(&base_handle, (char *)base_uof.c_str())) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_LoadObjFile for the plugin...";
  if((status = UcLo_LoadObjFile(&plugin_handle, (char *)plugin_uof.c_str())) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  base_me_mask = UcLo_GetAssignedMEs(base_handle);
  plugin_me_mask = UcLo_GetAssignedMEs(plugin_handle);

  std::cout << "Calling UcLo_WriteUimageAll for router base...";
  if((status = UcLo_WriteUimageAll(base_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling halMe_Start for router base microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to add the plugin (note that the router base microengines will be stopped and restarted).\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Stop for router base microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_WriteUimageAll for the plugin...";
  if((status = UcLo_WriteUimageAll(plugin_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling halMe_Start for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if((!(base_me_mask & (1<<me))) && (!(plugin_me_mask & (1<<me)))) continue;
    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to stop the microengines.\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Stop for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if((!(base_me_mask & (1<<me))) && (!(plugin_me_mask & (1<<me)))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_DeleObj for the router base...";
  if((status = UcLo_DeleObj(base_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_DeleObj for the plugin...";
  if((status = UcLo_DeleObj(plugin_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed!\n";
    exit(1);
  }
  std::cout << "done.\n";

  return 0;
}
