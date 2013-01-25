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
#include <sys/time.h>
#include <netinet/in.h>

#include "util.h"
#include "byte_buffer.h"
#include "rendezvous.h"
#include "message.h"
#include "onl_types.h"
#include "onl_requests.h"
#include "onl_responses.h"
#include "switch_types.h"
#include "switch_requests.h"
#include "switch_responses.h"

using namespace onld;

switch_response::switch_response(uint8_t *mbuf, uint32_t size): response(mbuf, size)
{
}

switch_response::switch_response(request *req, NCCP_StatusType stat): response(req)
{
  status = stat;
}

switch_response::~switch_response()
{
}

void
switch_response::parse()
{
  response::parse();

  buf >> status;
}

void
switch_response::write()
{
  response::write();

  buf << status;
}

add_vlan_response::add_vlan_response(uint8_t *mbuf, uint32_t size): switch_response(mbuf, size)
{
}

add_vlan_response::add_vlan_response(request *req, NCCP_StatusType stat, switch_vlan vlan): switch_response(req, stat)
{
  vlanid = vlan;
}

add_vlan_response::~add_vlan_response()
{
}

void
add_vlan_response::parse()
{
  switch_response::parse();

  buf >> vlanid;
}

void
add_vlan_response::write()
{
  switch_response::write();

  buf << vlanid;
}
