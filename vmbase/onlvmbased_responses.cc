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

#include "shared.h"

#include "onlbased_userdata.h"
#include "onlbased_globals.h"
#include "onlbased_requests.h"
#include "onlbased_responses.h"

using namespace onlbased;

rli_relay_resp::rli_relay_resp(rli_response* r, rli_relay_req* rq): rli_response(rq)
{
  orig_resp = r;

  if(orig_resp->more_message()) { end_of_message = false; }
  else { end_of_message = true; }
  
  status = orig_resp->getStatus();
  time_stamp = orig_resp->getTimeStamp();
  clock_rate = orig_resp->getClockRate();
  value = orig_resp->getValue();
  status_msg = orig_resp->getStatusMsg();
}

rli_relay_resp::~rli_relay_resp()
{
}

void
rli_relay_resp::write()
{
  rli_response::write();
  
  uint8_t* bytes = orig_resp->getBuffer()->getData();
  uint32_t len = orig_resp->getBuffer()->getSize();
  uint32_t i = orig_resp->getBuffer()->getCurrentIndex();

  for(; i<len; ++i)
  {
    buf << bytes[i];
  }
}

crd_relay_resp::crd_relay_resp(crd_response* r, crd_relay_req* rq): crd_response(rq)
{
  orig_resp = r;

  if(orig_resp->more_message()) { end_of_message = false; }
  else { end_of_message = true; }

  status = orig_resp->getStatus();
  exp = orig_resp->getExperiment();
  comp = orig_resp->getComponent();
}

crd_relay_resp::~crd_relay_resp()
{
}

void
crd_relay_resp::write()
{
  crd_response::write();

  uint8_t* bytes = orig_resp->getBuffer()->getData();
  uint32_t len = orig_resp->getBuffer()->getSize();
  uint32_t i = orig_resp->getBuffer()->getCurrentIndex();

  for(; i<len; ++i)
  {
    buf << bytes[i];
  }
}
