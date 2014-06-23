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

using namespace onld;

crd_response::crd_response(uint8_t *mbuf, uint32_t size): response(mbuf, size)
{
}

crd_response::crd_response(crd_request *req): response(req)
{
  status = NCCP_Status_Fine;
  exp = req->getExperiment();
  comp = req->getComponent();
}

crd_response::crd_response(crd_request *req, NCCP_StatusType stat): response(req)
{
  status = stat;
  exp = req->getExperiment();
  comp = req->getComponent();
}

crd_response::~crd_response()
{
}

void
crd_response::parse()
{
  response::parse();

  buf >> status;
  buf >> exp;
  buf >> comp;
}

void
crd_response::write()
{
  response::write();

  buf << status;
  buf << exp;
  buf << comp;
}

crd_hwresponse::crd_hwresponse(uint8_t *mbuf, uint32_t size): crd_response(mbuf, size)
{
}

crd_hwresponse::crd_hwresponse(crd_request *req, NCCP_StatusType stat, uint32_t i): crd_response(req, stat)
{
  id = i;
}

crd_hwresponse::~crd_hwresponse()
{
}

void
crd_hwresponse::parse()
{
  crd_response::parse();

  buf >> id;
}

void
crd_hwresponse::write()
{
  crd_response::write();

  buf << id;
}


crd_endconfig_response::crd_endconfig_response(uint8_t *mbuf, uint32_t size) : crd_response(mbuf, size)
{
}
      
crd_endconfig_response::crd_endconfig_response(crd_request *req, NCCP_StatusType stat) : crd_response(req, stat)
{
}
      
crd_endconfig_response::crd_endconfig_response(crd_request *req, NCCP_StatusType stat, std::string vmn) : crd_response(req, stat)
{
  vmname = vmn.c_str();
}

crd_endconfig_response::~crd_endconfig_response()
{
}

void 
crd_endconfig_response::parse()
{
  crd_response::parse();
  buf >> vmname;
}

void 
crd_endconfig_response::write()
{
  crd_response::write();
  buf << vmname;
}

rli_response::rli_response(uint8_t *mbuf, uint32_t size): response(mbuf, size)
{
}

rli_response::rli_response(rli_request *req): response(req)
{
  status = NCCP_Status_Fine;
  time_stamp = 0;
  clock_rate = 0;
  value = 0;
  status_msg = "";
}

rli_response::rli_response(rli_request *req, NCCP_StatusType stat): response(req)
{
  status = stat;
  time_stamp = 0;
  clock_rate = 0;
  value = 0;
  status_msg = "";
}

rli_response::rli_response(rli_request *req, NCCP_StatusType stat, uint32_t val): response(req)
{
  status = stat;
  value = val;
  status_msg = "";
  clock_rate = 1000;

  struct timeval ts_tv;
  gettimeofday(&ts_tv, NULL);
  time_stamp = (ts_tv.tv_sec * clock_rate) + (ts_tv.tv_usec / clock_rate);
}

rli_response::rli_response(rli_request* req, NCCP_StatusType stat, uint32_t val, uint32_t ts, uint32_t rate): response(req)
{
  status = stat;
  value = val;
  time_stamp = ts;
  clock_rate = rate;
  status_msg = "";
}

rli_response::rli_response(rli_request *req, NCCP_StatusType stat, std::string msg): response(req)
{
  status = stat;
  time_stamp = 0;
  clock_rate = 0;
  value = 0;
  status_msg = msg.c_str();
}

rli_response::~rli_response()
{
}

void
rli_response::parse()
{
  response::parse();

  buf >> status;
  buf >> time_stamp;
  buf >> clock_rate;
  buf >> value;
  buf >> status_msg;
}

void
rli_response::write()
{
  response::write();

  buf << status;
  buf << time_stamp;
  buf << clock_rate;
  buf << value;
  buf << status_msg;
}
