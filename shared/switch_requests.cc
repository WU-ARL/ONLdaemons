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
#include <netinet/in.h>

#include "util.h"
#include "byte_buffer.h"
#include "rendezvous.h"
#include "message.h"
#include "onl_types.h"
#include "onl_requests.h"
#include "switch_types.h"
#include "switch_requests.h"

using namespace onld;
using std::list;

add_vlan::add_vlan(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
  timeout = 360;
}

add_vlan::add_vlan(): request()
{
  op = NCCP_Operation_AddVlan;
  periodic_message = false;
  timeout = 360;
}

add_vlan::~add_vlan()
{
}

void
add_vlan::parse()
{
  request::parse();
}

void
add_vlan::write()
{
  request::write();
}

delete_vlan::delete_vlan(uint8_t *mbuf, uint32_t size): request(mbuf, size) 
{
}

delete_vlan::delete_vlan(switch_vlan vlan): request()
{
  op = NCCP_Operation_DeleteVlan;
  periodic_message = false;
  vlanid = vlan;
}

delete_vlan::~delete_vlan()
{
}

void
delete_vlan::parse()
{
  request::parse();

  buf >> vlanid;
}

void
delete_vlan::write()
{
  request::write();

  buf << vlanid;
}

add_to_vlan::add_to_vlan(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

add_to_vlan::add_to_vlan(switch_vlan vlan, switch_port& p): request()
{
  op = NCCP_Operation_AddToVlan;
  periodic_message = false;
  vlanid = vlan;
  port = p;
  timeout = 360;
}

add_to_vlan::~add_to_vlan()
{
}

void
add_to_vlan::parse()
{
  request::parse();

  buf >> vlanid;
  buf >> port;
}

void
add_to_vlan::write()
{
  request::write();

  buf << vlanid;
  buf << port;
}

delete_from_vlan::delete_from_vlan(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

delete_from_vlan::delete_from_vlan(switch_vlan vlan, switch_port& p): request()
{
  op = NCCP_Operation_DeleteFromVlan;
  periodic_message = false;
  vlanid = vlan;
  port = p;
  timeout = 360;
}

delete_from_vlan::~delete_from_vlan()
{
}

void
delete_from_vlan::parse()
{
  request::parse();

  buf >> vlanid;
  buf >> port;
}

void
delete_from_vlan::write()
{
  request::write();

  buf << vlanid;
  buf << port;
}

initialize::initialize(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

initialize::initialize(): request()
{
  op = NCCP_Operation_Initialize;
  periodic_message = false;
  timeout = 300;
}

initialize::~initialize()
{
}

void initialize::add_switch(switch_info newSwitch)
{
  switches.push_back(newSwitch);
}

void
initialize::parse()
{
  request::parse();
  uint32_t size = 0;
  switch_info curSwitch;
  buf >> size;
  for (uint32_t i = 0; i < size; i++) {
    buf >> curSwitch;
    switches.push_back(curSwitch);
  }
}

void
initialize::write()
{
  request::write();
  buf << (uint32_t)switches.size();
  for (list<switch_info>::iterator iter = switches.begin();
       iter != switches.end(); iter++) {
    buf << *iter;
  }
}
