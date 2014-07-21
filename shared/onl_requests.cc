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

using namespace onld;

i_am_up::i_am_up(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

i_am_up::i_am_up(): request()
{
  op = NCCP_Operation_IAmUp;
  periodic_message = false;

  char* myhostname = new char[64+1];
  gethostname(myhostname, 64);
  cp = myhostname;
  delete[] myhostname;
}

i_am_up::~i_am_up()
{
}

void
i_am_up::parse()
{
  request::parse();

  buf >> cp;
}

void
i_am_up::write()
{
  request::write();

  buf << cp;
}

crd_request::crd_request(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
  timeout=60;
}

crd_request::crd_request(): request()
{
  timeout=60;
}

crd_request::crd_request(experiment& e, component& c): request()
{
  exp = e;
  comp = c;
  timeout=60;
}

crd_request::~crd_request()
{
}

void
crd_request::parse()
{
  request::parse();

  buf >> exp;
  buf >> comp;
}

void
crd_request::write()
{
  request::write();

  buf << exp;
  buf << comp;
}

start_experiment::start_experiment(uint8_t *mbuf, uint32_t size): crd_request(mbuf, size)
{
  timeout=120;
}

start_experiment::start_experiment(experiment& e, component& c, std::string ip): crd_request(e,c)
{
  timeout=120;
  op = NCCP_Operation_StartExperiment;
  periodic_message = false;
  ipaddr = ip.c_str();
}

start_experiment::~start_experiment()
{
}

void
start_experiment::parse()
{
  crd_request::parse();

  buf >> ipaddr;
  buf >> init_params;
  buf >> cores;
  buf >> memory;
}

void
start_experiment::write()
{
  crd_request::write();

  buf << ipaddr;
  buf << init_params;
  buf << cores;
  buf << memory;

}

void
start_experiment::set_init_params(std::list<param>& params)
{
  init_params = params;
}

end_experiment::end_experiment(uint8_t *mbuf, uint32_t size): crd_request(mbuf, size)
{
}

end_experiment::end_experiment(experiment& e, component& c): crd_request(e,c)
{
  op = NCCP_Operation_EndExperiment;
  periodic_message = false;
}

end_experiment::~end_experiment()
{
}

void
end_experiment::parse()
{
  crd_request::parse();
}

void
end_experiment::write()
{
  crd_request::write();
}

refresh::refresh(uint8_t *mbuf, uint32_t size): crd_request(mbuf, size)
{
}

refresh::refresh(experiment& e, component& c): crd_request(e,c)
{
  op = NCCP_Operation_Refresh;
  periodic_message = false;
}

refresh::~refresh()
{
}

void
refresh::parse()
{
  crd_request::parse();

  buf >> reboot_params;
}

void
refresh::write()
{
  crd_request::write();

  buf << reboot_params;
}

void
refresh::set_reboot_params(std::list<param>& params)
{
  reboot_params = params;
}

configure_node::configure_node(uint8_t *mbuf, uint32_t size): crd_request(mbuf, size)
{ 
}

configure_node::configure_node(experiment& e, component& c, node_info& ni): crd_request(e,c)
{
  op = NCCP_Operation_CfgNode;
  periodic_message = false;
  node_conf = ni;
}

configure_node::~configure_node()
{ 
}

void
configure_node::parse()
{
  crd_request::parse();

  buf >> node_conf;
}
  
void
configure_node::write()
{
  crd_request::write();

  buf << node_conf;
}

end_configure_node::end_configure_node(uint8_t *mbuf, uint32_t size): crd_request(mbuf, size)
{ 
}

end_configure_node::end_configure_node(experiment& e, component& c): crd_request(e,c)
{
  op = NCCP_Operation_EndCfgNode;
  periodic_message = false;
}

end_configure_node::~end_configure_node()
{ 
}

rli_request::rli_request(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

rli_request::~rli_request()
{
}

void
rli_request::parse()
{
  request::parse();

  buf >> id;
  buf >> port;
  buf >> version;
  buf >> params;
}

//ard: Start of vm code
start_vm::start_vm(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

start_vm::~start_vm()
{
}

void
start_vm::parse()
{
  request::parse();

  buf >> name;
}

void
start_vm::write()
{
	request::write();

	buf << name;
}

