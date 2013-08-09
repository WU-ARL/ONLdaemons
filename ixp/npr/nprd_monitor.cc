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
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

#include "shared.h"

#include "nprd_types.h"
#include "nprd_configuration.h"
#include "nprd_monitor.h"

#include "halMev2Api.h"
#include "hal_sram.h"

using namespace npr;

Monitor::Monitor() throw()
{
}

Monitor::~Monitor() throw()
{ 
}

// index is one set of couters, counter is one of: pre-q packet, pre-q byte, post-q packet, post-q byte
unsigned int Monitor::read_stats_counter(unsigned int index, unsigned int counter) throw (monitor_exception)
{
  unsigned int addr = ONL_ROUTER_STATS_BASE;
  unsigned int val;
  
  write_log("read_stats_counter: index " + int2str(index) + ", counter " + int2str(counter));

  if(counter > MAX_COUNTER)
  {
    throw monitor_exception("counter is not valid");
  }
  if(index > ONL_ROUTER_STATS_INDEX_MAX)
  {
    throw monitor_exception("index is not valid");
  }

  addr += index*ONL_ROUTER_STATS_UNIT_SIZE + counter*4;

  val = SRAM_READ(SRAM_BANK_3, addr);
  
  return val;
}

unsigned int Monitor::read_stats_register(unsigned int index) throw(monitor_exception)
{
  unsigned int addr = ONL_ROUTER_REGISTERS_BASE;
  unsigned int val;

  write_log("read_stats_register: index " + int2str(index));

  if(index > ONL_ROUTER_REGISTERS_MAX)
  {
    throw monitor_exception("index is not valid");
  }

  addr += index*ONL_ROUTER_REGISTERS_UNIT_SIZE;

  val = SRAM_READ(SRAM_BANK_3, addr);
  
  return val;
}

unsigned int Monitor::read_queue_length(unsigned int port, unsigned int queue) throw(monitor_exception)
{
  unsigned int addr = QPARAMS_BASE_ADDR;
  unsigned int val;

  write_log("read_queue_length: port " + int2str(port) + ", queue " + int2str(queue));

  if(port >= QM_NUM_PORTS)
  {
    throw monitor_exception("port is not valid");
  }
  if(queue >= QM_NUM_QUEUES)
  {
    throw monitor_exception("queue is not valid");
  }

  queue = queue | ((port+1) << 13);
  addr += queue * QPARAMS_UNIT_SIZE;

  val = SRAM_READ(SRAM_BANK_1, addr);
  return val;
}
