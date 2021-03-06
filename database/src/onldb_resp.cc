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
#include <list>
#include <boost/shared_ptr.hpp>

#include "onldb_resp.h"

using namespace onl;

// onldb_resp stuff starts here

onldb_resp::onldb_resp(int res, std::string msg) throw()
{
  r = res;
  m = msg;
}

onldb_resp::~onldb_resp() throw()
{
}

int onldb_resp::result() throw()
{
  return r;
}

std::string onldb_resp::msg() throw()
{
  return m;
}

node_info::node_info() throw()
{
  node_ = "";
  state_ = "";
  has_cp_ = false;
  do_keeboot_ = false;
  cp_ = "";
  cp_port_ = 0;
  type_ = "";
  is_dependent_ = false;
}

node_info::node_info(std::string node, std::string state, bool has_cp, bool do_keeboot, std::string cp, unsigned short cp_port, std::string type, bool is_dependent, std::string cluster) throw()
{
  node_ = node;
  state_ = state;
  has_cp_ = has_cp;
  do_keeboot_ = do_keeboot;
  cp_ = cp;
  cp_port_ = cp_port;
  type_ = type;
  is_dependent_ = is_dependent;
  if (cluster == "NULL")
    cluster_ = "";
  else
    cluster_ = cluster;
}

node_info::node_info(const node_info& ni) throw()
{
  node_ = ni.node_;
  state_ = ni.state_;
  has_cp_ = ni.has_cp_;
  do_keeboot_ = ni.do_keeboot_;
  cp_ = ni.cp_;
  cp_port_ = ni.cp_port_;
  type_ = ni.type_;
  is_dependent_ = ni.is_dependent_;
}

node_info::~node_info() throw()
{
}

node_info&
node_info::operator=(const node_info& ni)
{
  node_ = ni.node_;
  state_ = ni.state_;
  has_cp_ = ni.has_cp_;
  do_keeboot_ = ni.do_keeboot_;
  cp_ = ni.cp_;
  cp_port_ = ni.cp_port_;
  type_ = ni.type_;
  is_dependent_ = ni.is_dependent_;
  cluster_ = ni.cluster_;
  return *this;
}

std::string node_info::node() throw()
{
  return node_;
}

std::string node_info::state() throw()
{
  return state_;
}

std::string node_info::cluster() throw()
{
  return cluster_;
}

bool node_info::has_cp() throw()
{
  return has_cp_;
}

bool node_info::do_keeboot() throw()
{
  return do_keeboot_;
}

std::string node_info::cp() throw()
{
  return cp_;
}

unsigned short node_info::cp_port() throw()
{
  return cp_port_;
}

std::string node_info::type() throw()
{
  return type_;
}

bool
node_info::is_dependent() throw()
{
  return is_dependent_;
}

switch_port_info::switch_port_info() throw()
{
  switch_name = "";
  port = 0;
  interswitch_port = false;
  pass_tag = false;
}

switch_port_info::switch_port_info(std::string sw, unsigned int portnum, bool is_interswitch_port, bool pt) throw()
{
  switch_name = sw;
  port = portnum;
  interswitch_port = is_interswitch_port;
  pass_tag = pt;
}

switch_port_info::switch_port_info(const switch_port_info& spi) throw()
{
  switch_name = spi.switch_name;
  port = spi.port;
  interswitch_port = spi.interswitch_port;
  pass_tag = spi.pass_tag;
}

switch_port_info::~switch_port_info() throw()
{
}

switch_port_info&
switch_port_info::operator=(const switch_port_info& spi)
{
  switch_name = spi.switch_name;
  port = spi.port;
  interswitch_port = spi.interswitch_port;
  pass_tag = spi.pass_tag;
  return *this;
}

switch_info::switch_info() throw()
{
  switch_name = "";
  num_ports = 0;
  mgmt_port = 0;
}

switch_info::switch_info(std::string sw, unsigned int numports, unsigned int mgmtport) throw()
{
  switch_name = sw;
  num_ports = numports;
  mgmt_port = mgmtport;
}

switch_info::switch_info(const switch_info& si) throw()
{
  switch_name = si.switch_name;
  num_ports = si.num_ports;
  mgmt_port = si.mgmt_port;
}

switch_info::~switch_info() throw()
{
}

switch_info&
switch_info::operator=(const switch_info& si)
{
  switch_name = si.switch_name;
  num_ports = si.num_ports;
  mgmt_port = si.mgmt_port;
  return *this;
}
