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
#include "dispatcher.h"

using namespace onld;

dispatcher* dispatcher::the_dispatcher = NULL;
dispatcher::constructor_template dispatcher::req_map[256];
dispatcher::constructor_template dispatcher::resp_map[256];

dispatcher::dispatcher()
{
  for(int i=0; i<256; i++)
  {
    req_map[i] = NULL;
    resp_map[i] = NULL;
    next_seq[i] = 1;
  }

  pthread_mutex_init(&seq_lock, NULL);
  //pthread_mutex_init(&map_lock, NULL); //JP added 9_4_2012 to try and fix seg fault
  pthread_rwlock_init(&map_lock, NULL); //JP added 11_26_2018 to try and fix seg fault again
}

dispatcher::~dispatcher()
{
  pthread_mutex_destroy(&seq_lock);
  //pthread_mutex_destroy(&map_lock); //JP added 9_4_2012 to try and fix seg fault
  pthread_rwlock_destroy(&map_lock); //JP added 11_26_2018 to try and fix seg fault again
}

dispatcher *
dispatcher::get_dispatcher()
{
  if(the_dispatcher == NULL) { the_dispatcher = new dispatcher(); }
  return the_dispatcher;
}

message *
dispatcher::create_message(uint8_t *mbuf, uint32_t size)
{
  message *rtn;

  // peek at first few bytes to get the message type and operation
  uint16_t type;
  memcpy(&type, &mbuf[0], 2);
  type = ntohs(type);
  uint8_t op = mbuf[3];

  write_log("dispatcher::create_message: type=" + int2str(type) + ", operation=" + int2str(op));

  if(type == NCCP_MessageRequest)
  {
    if(req_map[op] == NULL) { return NULL; }
    rtn = (message *)req_map[op](mbuf, size);
    rtn->parse();
  }
  else if(type == NCCP_MessageResponse)
  {
    if(resp_map[op] == NULL) { return NULL; }
    rtn = (message *)resp_map[op](mbuf, size);
    rtn->parse();
  }
  else if(type == NCCP_MessagePeriodic)
  {
    rtn = new periodic(mbuf, size);
    rtn->parse();
  }
  else
  {
    rtn = NULL;
  }

  return rtn;
}

void
dispatcher::register_rendezvous(request *req)
{
  if(req == NULL) return;

  uint8_t id = (uint8_t)req->get_op();
  autoLock slock(seq_lock);
  uint32_t seq = next_seq[id]++;
  slock.unlock();
  
  rendezvous rend(id,seq);
  req->set_rendezvous(rend);

  //autoLock mlock(map_lock); //JP added 9_4_2012 to try and fix seg fault
  autoWrLock mlock(map_lock);  //JP added 11_26_2018 to try and fix seg fault again
  rend_map[rend] = req;
  mlock.unlock(); //JP added 9_4_2012 to try and fix seg faultYY
}

void
dispatcher::register_periodic_rendezvous(request *req)
{
  if(req == NULL) return;

  uint8_t id = (uint8_t)req->get_op();
  autoLock slock(seq_lock);
  uint32_t seq = next_seq[id]++;
  slock.unlock();
  
  rendezvous return_rend(id,seq);
  req->set_return_rendezvous(return_rend);
  req->prepare_for_periodic();

  //autoLock mlock(map_lock); //JP added 9_4_2012 to try and fix seg fault
  autoWrLock mlock(map_lock);  //JP added 11_26_2018 to try and fix seg fault again
  rend_map[return_rend] = req;
  mlock.unlock(); //JP added 9_4_2012 to try and fix seg faultYY
}

bool
dispatcher::call_rendezvous(response *resp)
{
  rendezvous rend = resp->get_rendezvous();

  std::map<rendezvous, request *>::iterator it;
  autoRdLock mlock(map_lock);  //JP added 11_26_2018 to try and fix seg fault again
  it = rend_map.find(rend); 
  if(it == rend_map.end()) { return true; }
  request *req = (request *)(it->second);
  mlock.unlock(); //JP added 11_26_2018 to try and fix seg fault again

  return req->request_rendezvous(resp);
}

bool
dispatcher::call_rendezvous(periodic *per)
{
  rendezvous rend = per->get_rendezvous();

  std::map<rendezvous, request *>::iterator it;
  autoRdLock mlock(map_lock);  //JP added 11_26_2018 to try and fix seg fault again
  it = rend_map.find(rend); 
  if(it == rend_map.end()) { return true; }
  request *req = (request *)(it->second);
  mlock.unlock(); //JP added 11_26_2018 to try and fix seg fault again

  return req->request_rendezvous(per);
}

void
dispatcher::clear_rendezvous(request *req)
{
  rendezvous rend = req->get_rendezvous();
  //autoLock mlock(map_lock); //JP added 9_4_2012 to try and fix seg fault
  autoWrLock mlock(map_lock);  //JP added 11_26_2018 to try and fix seg fault again
  rend_map.erase(rend);
  mlock.unlock(); //JP added 9_4_2012 to try and fix seg fault

  rendezvous def_rend;
  req->set_rendezvous(def_rend);
}

void
dispatcher::clear_periodic_rendezvous(request *req)
{
  rendezvous return_rend = req->get_return_rendezvous();
  //autoLock mlock(map_lock); //JP added 9_4_2012 to try and fix seg fault
  autoWrLock mlock(map_lock);  //JP added 11_26_2018 to try and fix seg fault again
  rend_map.erase(return_rend);
  mlock.unlock(); //JP added 9_4_2012 to try and fix seg fault

  rendezvous def_rend;
  req->set_return_rendezvous(def_rend);
}
