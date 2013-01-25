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

#include <string>
#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <list>
#include <vector>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.h"
#include "byte_buffer.h"
#include "onl_types.h"
#include "switch_types.h"

using namespace onld;

switch_port::switch_port()
{
  switchid = "";
  port = 0;
  is_interswitch_port = false;
}

switch_port::switch_port(const std::string switch_id, uint32_t portnum)
{
  switchid = switch_id.c_str();
  port = portnum;
  is_interswitch_port = false;
}

switch_port::switch_port(const std::string switch_id, uint32_t portnum, bool interswitch_port)
{
  switchid = switch_id.c_str();
  port = portnum;
  is_interswitch_port = interswitch_port;
}


switch_port::switch_port(const switch_port& sp)
{
  switchid = sp.switchid;
  port = sp.port;
  is_interswitch_port = sp.is_interswitch_port;
}

switch_port::~switch_port()
{
}

switch_port&
switch_port::operator=(const switch_port& sp)
{
  switchid = sp.switchid;
  port = sp.port;
  is_interswitch_port = sp.is_interswitch_port;
  return *this;
}

bool
switch_port::operator==(const switch_port& sp)
{
  return ((switchid == sp.switchid) &&
          (port == sp.port) && 
	  (is_interswitch_port == sp.is_interswitch_port));
}

bool
switch_port::operator<(const switch_port& sp) const
{
  if(switchid < sp.switchid) return true;
  if(switchid > sp.switchid) return false;
  if(port < sp.port) return true;
  return false;
}

bool
switch_port::isValid()
{
  if(port == 0) return false;
  return true;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, switch_port& sp)
{
  buf << sp.switchid;
  buf << sp.port;
  buf << sp.is_interswitch_port;
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, switch_port& sp)
{
  buf >> sp.switchid;
  buf >> sp.port;
  buf >> sp.is_interswitch_port;
  return buf;
}

ostream& 
onld::operator<<(ostream & os, switch_port& sp)
{
  if (sp.is_interswitch_port) {
    return os << "switch id:" << sp.switchid.getString() << " port:" 
              << sp.port << " inter-switch port:True";
  } else {
    return os << "switch id:" << sp.switchid.getString() << " port:" 
              << sp.port << " inter-switch port:False"; 
  }
}

switch_info::switch_info()
{
  switchid = "";
  ports = 0;
  management_port = 0;
}

switch_info::switch_info(const std::string switch_id, uint32_t num_ports)
{
  switchid = switch_id.c_str();
  ports = num_ports;
  management_port = 0; // 0 indicates none of the data ports are used for management
}

switch_info::switch_info(const switch_info& si)
{
  switchid = si.switchid;
  ports = si.ports;
  management_port = si.management_port;
}

switch_info::~switch_info()
{
}

switch_info&
switch_info::operator=(const switch_info& si)
{
  switchid = si.switchid;
  ports = si.ports;
  management_port = si.management_port;
  return *this;
}


byte_buffer&
onld::operator<<(byte_buffer& buf, switch_info& si)
{
  buf << si.switchid;
  buf << si.ports;
  buf << si.management_port;
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, switch_info&si)
{
  buf >> si.switchid;
  buf >> si.ports;
  buf >> si.management_port;
  return buf;
}

ostream&
onld::operator<<(ostream & os, switch_info& si)
{
  return os << "switch id:" << si.switchid.getString() << " number of ports:"
            << si.ports << " management port:" << si.management_port;
}
