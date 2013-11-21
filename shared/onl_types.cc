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

using namespace onld;

nccp_string::nccp_string()
{
  nccp_str[0] = '\0';
  nccp_str[255] = '\0';
  length = 0;
}

nccp_string::nccp_string(const char *c)
{
  nccp_str[255] = '\0';
  size_t l = strlen(c);
  if (l > 255) { length = 255; }
  else { length = l; }
  strncpy(nccp_str, c, length);
  nccp_str[length] = '\0';
}

nccp_string::nccp_string(const char *c, uint8_t len)
{
  nccp_str[255] = '\0';
  length = len;
  strncpy(nccp_str, c, length);
  nccp_str[length] = '\0';
}

nccp_string::nccp_string(const nccp_string& s)
{
  nccp_str[255] = '\0';
  length = s.length;
  strncpy(nccp_str, s.nccp_str, length);
  nccp_str[length] = '\0';
}

nccp_string::~nccp_string()
{
}

nccp_string &
nccp_string::operator=(const nccp_string& s)
{
  length = s.length;
  strncpy(nccp_str, s.nccp_str, length);
  nccp_str[length] = '\0';
  return *this;
}

nccp_string &
nccp_string::operator=(const char* c)
{
  size_t l = strlen(c);
  if (l > 255) { length = 255; }
  else { length = l; }
  strncpy(nccp_str, c, length);
  nccp_str[length] = '\0';
  return *this;
}

bool
nccp_string::operator==(const nccp_string& s)
{
  if(strcmp(nccp_str, s.nccp_str) == 0) { return true; }
  return false;
}

bool
nccp_string::operator<(const nccp_string& s) const
{
  if(strcmp(nccp_str, s.nccp_str) < 0) { return true; }
  return false;
}

bool
nccp_string::operator>(const nccp_string& s) const
{
  if(strcmp(nccp_str, s.nccp_str) > 0) { return true; }
  return false;
}

uint32_t
nccp_string::getAsIP()
{
  struct in_addr tmp;
  if(inet_pton(AF_INET, nccp_str, &tmp) < 1)
  {
    return 0;
  }
  return ntohl(tmp.s_addr);
}

byte_buffer&
onld::operator<<(byte_buffer& buf, nccp_string& s)
{
  uint32_t l = (uint32_t)s.length;
  buf << l;
  for(uint8_t i=0; i<s.length; i++)
  {
    buf << s.nccp_str[i];
  }
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, nccp_string& s)
{
  uint32_t l;
  buf >> l;
  if(l > 255) { s.length = 255; }
  else { s.length = l; }
  for(uint8_t i=0; i<s.length; i++)
  {
    buf >> s.nccp_str[i];
  }
  s.nccp_str[s.length] = '\0';
  return buf;
}

experiment_info::experiment_info()
{
}

experiment_info::experiment_info(const experiment_info& eid)
{
  ID = eid.ID;
  expName = eid.expName;
  userName = eid.userName;
}

experiment_info::~experiment_info()
{
}

experiment_info&
experiment_info::operator=(const experiment_info& eid)
{
  ID = eid.ID;
  expName = eid.expName;
  userName = eid.userName;
  return *this;
}

bool
experiment_info::operator==(const experiment_info& eid)
{
  if((ID == eid.ID) && (expName == eid.expName) && (userName == eid.userName))
  {
    return true;
  }
  return false;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, experiment_info& eid)
{
  buf << eid.ID;
  buf << eid.expName;
  buf << eid.userName;
  return buf;
} 

byte_buffer&
onld::operator>>(byte_buffer& buf, experiment_info& eid)
{
  buf >> eid.ID;
  buf >> eid.expName;
  buf >> eid.userName;
  return buf;
} 

component::component()
{
  type = "";
  id = 0;
  parent = 0;
  label = "";
  cp = "";
  is_router = false;
}

component::component(std::string t, uint32_t i)
{
  type = t.c_str();
  id = i;
  parent = 0;
  label = "";
  cp = "";
  is_router = false;
}

component::component(const component& c)
{
  type = c.type;
  id = c.id;
  parent = c.parent;
  label = c.label;
  cp = c.cp;
  is_router = c.is_router;
}

component::~component()
{
}

component&
component::operator=(const component& c)
{
  type = c.type;
  id = c.id;
  parent = c.parent;
  label = c.label;
  cp = c.cp;
  is_router = c.is_router;
  return *this;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, component& c)
{
  buf << c.type;
  buf << c.id;
  buf << c.parent;
  buf << c.label;
  buf << c.cp;
  //cgw, it would be nice to change this back once we move completely to the new stuff
  //buf << c.is_router;
  if(c.is_router) { buf << (uint8_t)1; }
  else { buf << (uint8_t)0; }
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, component& c)
{
  buf >> c.type;
  buf >> c.id;
  buf >> c.parent;
  buf >> c.label;
  buf >> c.cp;
  //buf >> c.is_router;
  uint8_t tmp_ir = 0; buf >> tmp_ir;
  if(tmp_ir == 0) { c.is_router = false; }
  else { c.is_router = true; }
  return buf;
}

cluster::cluster(): component()
{
  numHardware = 0;
  for(uint32_t i=0; i<=255; i++)
  {
    hardware[i] = 0;
  }
}

cluster::cluster(std::string t, uint32_t i): component(t,i)
{
  numHardware = 0;
  for(uint32_t i=0; i<=255; i++)
  {
    hardware[i] = 0;
  }
}

cluster::cluster(const cluster& c): component(c)
{
  numHardware = c.numHardware;
  for(uint32_t i=0; i<numHardware; ++i)
  {
    hardware[i] = c.hardware[i];
  }
}

cluster::~cluster()
{
}

cluster&
cluster::operator=(const cluster& c)
{
  this->component::operator=(c);
  numHardware = c.numHardware;
  for(uint32_t i=0; i<numHardware; ++i)
  {
    hardware[i] = c.hardware[i];
  }
  return *this;
}

void
cluster::parseExtra(byte_buffer& buf)
{
  // cgw, this is broken right now, b/c the RLI doesn't send the is_router 
  // byte with the component stuff if it's a cluster.  fortunately, no 
  // one uses this info, so we just won't call parseExtra or <</>> for a 
  // cluster type for now.  this assumes that nothing comes after the cluster
  // stuff, which it doesn't for now..
  numHardware = 0;
  buf >> numHardware;
  for(uint32_t i=0; i<numHardware; ++i)
  {
    buf >> hardware[i];
  }
}

byte_buffer&
onld::operator<<(byte_buffer& buf, cluster& c)
{
  buf << (component&)c;
  buf << c.numHardware;
  for(uint32_t i=0; i<c.numHardware; ++i)
  {
    buf << c.hardware[i];
  }
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, cluster& c)
{
  buf >> (component&)c;
  buf >> c.numHardware;
  for(uint32_t i=0; i<c.numHardware; ++i)
  {
    buf >> c.hardware[i];
  }
  return buf;
}

experiment::experiment()
{ 
  port = 0;
}

experiment::experiment(std::string a, uint32_t p, experiment_info& ei)
{
  addr = a.c_str();
  port = p;
  info = ei;
}

experiment::experiment(const experiment& e)
{
  addr = e.addr;
  port = e.port;
  info = e.info;
}

experiment::~experiment()
{
}

experiment&
experiment::operator=(const experiment& e)
{
  addr = e.addr;
  port = e.port;
  info = e.info;
  return *this;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, experiment& e)
{
  buf << e.addr;
  buf << e.port;
  buf << e.info;
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, experiment& e)
{
  buf >> e.addr;
  buf >> e.port;
  buf >> e.info;
  return buf;
}

param::param()
{
  pt = string_param;
  int_val = 0;
  double_val = 0;
  bool_val = false;
  string_val = "";
}

param::param(std::string s)
{
  pt = string_param;
  int_val = 0;
  double_val = 0;
  bool_val = false;
  string_val = s.c_str();
}

param::param(const param& p)
{
  pt = p.pt;
  int_val = p.int_val;
  double_val = p.double_val;
  bool_val = p.bool_val;
  string_val = p.string_val;
}

param::~param()
{
}

param&
param::operator=(const param& p)
{
  pt = p.pt;
  int_val = p.int_val;
  double_val = p.double_val;
  bool_val = p.bool_val;
  string_val = p.string_val;
  return *this;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, param& p)
{
  buf << p.pt;
  switch (p.pt)
  {
    case int_param:    buf << p.int_val;    break;
    case double_param: // doubles sent/received as strings
    case string_param: buf << p.string_val; break;
    case bool_param:
    {
      if(p.bool_val) { buf << (uint8_t)1; }
      else { buf << (uint8_t)0; }
      break;
    }
    default: break;
  }
  return buf;
} 

byte_buffer&
onld::operator>>(byte_buffer& buf, param& p)
{
  buf >> p.pt;
  if(p.pt > max_param_type) { p.pt = unknown_param; }
  switch (p.pt)
  {
    case int_param:    buf >> p.int_val;    break;
    case double_param: buf >> p.string_val;
                       p.double_val = atof(p.string_val.getCString()); break;
    case string_param: buf >> p.string_val; break;
    case bool_param:
    {
      uint8_t tmp_ir = 0;
      buf >> tmp_ir;
      if(tmp_ir == 0) { p.bool_val = false; }
      else { p.bool_val = true; }
      break;
    }
    default: break;
  }
  return buf;
} 

byte_buffer&
onld::operator<<(byte_buffer& buf, std::list<param>& plist)
{
  uint32_t num_params = plist.size();
  buf << num_params;
  std::list<param>::iterator it;
  for(it = plist.begin(); it != plist.end(); it++)
  {
    buf << *it;
  }
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, std::list<param>& plist)
{
  uint32_t num_params;
  buf >> num_params;
  for(uint32_t i=0; i<num_params; ++i)
  {
    param p;
    buf >> p;
    plist.push_back(p);
  }
  return buf;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, std::vector<param>& pvec)
{
  uint32_t num_params = pvec.size();
  buf << num_params;
  for(uint32_t i=0; i<num_params; ++i)
  {
    buf << pvec[i];
  }
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, std::vector<param>& pvec)
{
  uint32_t num_params;
  buf >> num_params;
  for(uint32_t i=0; i<num_params; ++i)
  {
    param p;
    buf >> p;
    pvec.push_back(p);
  }
  return buf;
}

node_info::node_info()
{
  ipaddr = "";
  subnet = "";
  port = 0;
  real_port = 0;
  remote_type = "";
  is_remote_router = false;
  nexthop_ipaddr = "";
  vlanid = 0;
  bandwidth = 1000;
}

node_info::node_info(std::string ip, std::string sn, uint32_t portnum, std::string rtype, bool rrouter, std::string next_hop_ip, uint32_t rp)
{
  ipaddr = ip.c_str();
  subnet = sn.c_str();
  remote_type = rtype.c_str();
  is_remote_router = rrouter;
  port = portnum;
  nexthop_ipaddr = next_hop_ip.c_str();
  real_port = rp;
  vlanid = 0;
  bandwidth = 1000;
}

node_info::node_info(const node_info& ni)
{
  ipaddr = ni.ipaddr;
  subnet = ni.subnet;
  port = ni.port;
  remote_type = ni.remote_type;
  is_remote_router = ni.is_remote_router;
  nexthop_ipaddr = ni.nexthop_ipaddr;
  real_port = ni.real_port;
  vlanid = ni.vlanid;
  bandwidth = ni.bandwidth;
}

node_info::~node_info()
{
}

node_info&
node_info::operator=(const node_info& ni)
{
  ipaddr = ni.ipaddr;
  subnet = ni.subnet;
  port = ni.port;
  remote_type = ni.remote_type;
  is_remote_router = ni.is_remote_router;
  nexthop_ipaddr = ni.nexthop_ipaddr;
  real_port = ni.real_port;
  vlanid = ni.vlanid;
  bandwidth = ni.bandwidth;
  return *this;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, node_info& ni)
{
  buf << ni.ipaddr;
  buf << ni.subnet;
  buf << ni.remote_type;
  buf << ni.is_remote_router;
  buf << ni.port;
  buf << ni.nexthop_ipaddr;
  buf << ni.real_port;
  buf << ni.vlanid;
  buf << ni.bandwidth;
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, node_info& ni)
{
  buf >> ni.ipaddr;
  buf >> ni.subnet;
  buf >> ni.remote_type;
  buf >> ni.is_remote_router;
  buf >> ni.port;
  buf >> ni.nexthop_ipaddr;
  buf >> ni.real_port;
  buf >> ni.vlanid;
  buf >> ni.bandwidth;
  return buf;
}
