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
#include <set>
#include <exception>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "shared.h"
#include "onl_database.h"

#include "onlcrd_reservation.h"
#include "onlcrd_session.h"
#include "onlcrd_component.h"
#include "onlcrd_session_manager.h"
#include "onlcrd_globals.h"
#include "onlcrd_requests.h"
#include "onlcrd_responses.h"
#include "onlcrd_session_sharing.h"

using namespace onlcrd;

session_response::session_response(request *req, std::string eid): response(req)
{
  status = NCCP_Status_Fine;
  id = eid.c_str();
  error_code = 0;
}

session_response::session_response(request *req, nccp_string& eid): response(req)
{
  status = NCCP_Status_Fine;
  id = eid;
  error_code = 0;
}

session_response::session_response(request *req, std::string msg, Session_Error err): response(req)
{
  status = NCCP_Status_Failed;
  status_msg = msg.c_str();
  error_code = err;
}

session_response::session_response(request *req, nccp_string& eid, std::string msg, Session_Error err): response(req)
{
  status = NCCP_Status_Failed;
  id = eid;
  status_msg = msg.c_str();
  error_code = err;
}

session_response::~session_response()
{
}

void
session_response::write()
{
  response::write();
  
  buf << status;
  buf << id;
  buf << status_msg;
  buf << error_code;
}

reservation_response::reservation_response(request *req, std::string msg): response(req)
{
  status = NCCP_Status_Failed;
  status_msg = msg.c_str();
  error_code = Session_No_Err;
}

reservation_response::reservation_response(request *req, std::string msg, Session_Error err): response(req)
{
  status = NCCP_Status_Failed;
  status_msg = msg.c_str();
  error_code = err;
}

reservation_response::reservation_response(request *req, std::string msg, std::string start): response(req)
{
  status = NCCP_Status_Fine;
  status_msg = msg.c_str();
  error_code = Session_No_Err;
  start_time = start.c_str();
}

reservation_response::reservation_response(request *req, NCCP_StatusType stat, std::string msg, Session_Error err): response(req)
{
  status = stat;
  status_msg = msg.c_str();
  error_code = err;
}

reservation_response::~reservation_response()
{
}

void
reservation_response::write()
{
  response::write();
  
  buf << status;
  buf << status_msg;
  buf << error_code;
  buf << start_time;
}

rlicrd_response::rlicrd_response(rlicrd_request *req, NCCP_StatusType stat): response(req)
{
  status = stat;
  exp.setExpInfo(req->getExpInfo()); 
  comp = req->getComponent();
}

rlicrd_response::~rlicrd_response()
{
}

void
rlicrd_response::write()
{
  response::write();

  buf << status;
  buf << exp;
  buf << comp;
}

component_response::component_response(rlicrd_request *req, std::string node, uint32_t port): rlicrd_response(req, NCCP_Status_Fine)
{
  cp = node.c_str();
  cp_port = port;
}

component_response::component_response(rlicrd_request *req, NCCP_StatusType stat): rlicrd_response(req, stat)
{
  cp_port = 0;
}

component_response::~component_response()
{
}

void
component_response::write()
{
  rlicrd_response::write();

  buf << cp;
  buf << cp_port;
}

cluster_response::cluster_response(rlicrd_request *req, NCCP_StatusType stat): rlicrd_response(req, stat)
{
  cluster_index = 0;
}

cluster_response::cluster_response(rlicrd_request *req, NCCP_StatusType stat, uint32_t index): rlicrd_response(req, stat)
{
  cluster_index = index;
}

cluster_response::~cluster_response()
{
}

void
cluster_response::write()
{
  rlicrd_response::write();

  buf << cluster_index;
}
