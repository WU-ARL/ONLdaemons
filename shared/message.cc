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
#include <time.h>

#include "util.h"
#include "byte_buffer.h"
#include "rendezvous.h"
#include "message.h"
#include "dispatcher.h"
#include "nccp_connection.h"

using namespace onld;

message::message()
{
  version = 0;

  nccpconn = NULL;
}

message::message(uint8_t *mbuf, uint32_t size)
{
  if(size >= buf.getMaxSize())
  {
    buf.resize(size);
  }

  for(uint32_t z=0; z<size; ++z)
  {
    buf << mbuf[z];
  }

  buf.reset();

  nccpconn = NULL;
}

message::~message()
{
//  std::string logstr = "message:~message: type = " + int2str(type) + ", op = " + int2str(op);
//  write_log(logstr);
  if(nccpconn != NULL) { nccpconn->decrement_msg_count(); }
}

void
message::set_connection(nccp_connection *conn)
{
  if(nccpconn == conn) { return; }

  if(nccpconn != NULL) { nccpconn->decrement_msg_count(); }
  nccpconn = conn;
  if(nccpconn != NULL) { nccpconn->increment_msg_count(); }
}

bool
message::send()
{
  if((nccpconn != NULL) && (!nccpconn->isClosed()))
  {
    buf.reset();
    return nccpconn->send_message(this);
  }
  return false;
}

void
message::parse()
{
  buf >> type;
  buf >> version;
  buf >> op;

  buf >> rend;

  buf >> periodic_message;
}

void
message::write()
{
  buf << type;
  buf << version;
  buf << op;

  buf << rend;

  buf << periodic_message;
}

request::request(): message()
{
  type = NCCP_MessageRequest;
  received_response = false;
  received_stop = false;
  received_ack = false;
  timeout = 30;
  pthread_mutex_init(&response_mutex, NULL);
  pthread_cond_init(&response_cond, NULL);
}

request::request(uint8_t *mbuf, uint32_t size): message(mbuf, size)
{
  timeout = 30;
  received_response = false;
  received_stop = false;
  received_ack = false;
  pthread_mutex_init(&response_mutex, NULL);
  pthread_cond_init(&response_cond, NULL);
}

request::~request()
{
}

bool
request::handle()
{
  return true;
}

bool
request::request_rendezvous(response *res)
{
  resp = res;
  
  //write_log("request::request_rendezvous 1"); //JP 8_22_12 to be removed
  pthread_mutex_lock(&response_mutex);
  received_response = true;
  //write_log("request::request_rendezvous 2"); //JP 8_22_12 to be removed
  pthread_cond_signal(&response_cond);
  //write_log("request::request_rendezvous 3"); //JP 8_22_12 to be removed
  pthread_mutex_unlock(&response_mutex);
  return false;
}

bool
request::wait_for_response()
{
  struct timespec abstimeout;
  int wait_rc = 0;

  int mutex_err = pthread_mutex_lock(&response_mutex);

  clock_gettime(CLOCK_REALTIME, &abstimeout);
  abstimeout.tv_sec += timeout;
  //write_log("request::wait_for_response mutex locked " + int2str(mutex_err)); //JP 8_22_12 to be removed

  while((!received_response) && (wait_rc != ETIMEDOUT))
  {
    wait_rc = pthread_cond_timedwait(&response_cond, &response_mutex, &abstimeout);
    //write_log("request::wait_for_response pthread_cond_timedwait " + int2str(received_response)); //JP 8_22_12 to be removed
  }
  //write_log("request::wait_for_response out of loop"); //JP 8_22_12 to be removed

  received_response = false;
  pthread_mutex_unlock(&response_mutex);
 
  if(wait_rc == ETIMEDOUT) { return false; }
  return true;
}

bool
request::request_rendezvous(periodic *per)
{
  if(per->get_op() == NCCP_Operation_StopPeriodic)
  {
    received_stop = true;
  }
  else if(per->get_op() == NCCP_Operation_AckPeriodic)
  {
    received_ack = true;
  }
  return true;
}

bool
request::send_and_wait()
{
  dispatcher* the_dispatcher = dispatcher::get_dispatcher();

  the_dispatcher->register_rendezvous(this);

  //write_log("request::send_and_wait sending"); //JP 8_22_12 to be removed

  if(!message::send())
  {
    write_log("request::send_and_wait send failed"); //JP 8_22_12 to be removed
    the_dispatcher->clear_rendezvous(this);
    return false;
  }
  //write_log("request::send_and_wait waiting"); //JP 8_22_12 to be removed

  bool got_reply = wait_for_response();
  //write_log("request::send_and_wait got_reply " + int2str(got_reply)); //JP 8_22_12 to be removed
  the_dispatcher->clear_rendezvous(this);
  return got_reply;
}

void
request::parse()
{
  message::parse();

  if(periodic_message)
  {
    buf >> period;
  }
}

void
request::write()
{
  message::write();
  
  if(periodic_message)
  {
    buf << period;
  }
}

void
request::get_period(struct timespec *t)
{
  t->tv_sec = period.tv_sec;
  t->tv_nsec = period.tv_usec * 1000;
}

response::response(uint8_t *mbuf, uint32_t size): message(mbuf, size)
{
  req = NULL;
  end_of_message = true;
}

response::response(request *r): message()
{
  req = r;
  end_of_message = true;

  type = NCCP_MessageResponse;

  version = r->get_version();
  op = r->get_op();
  rend = r->get_rendezvous();
  periodic_message = r->is_periodic();
  return_rend = r->get_return_rendezvous();
  set_connection(r->get_connection());
}

response::~response()
{
}

void
response::parse()
{
  message::parse();

  if(periodic_message)
  {
    buf >> return_rend;
  }

  buf >> end_of_message;
}

void
response::write()
{
  message::write();
  
  if(periodic_message)
  {
    buf << return_rend;
  }

  buf << end_of_message;
}

periodic::periodic()
{
  type = NCCP_MessagePeriodic;
}

periodic::periodic(uint8_t *mbuf, uint32_t size): message(mbuf, size)
{
}

periodic::~periodic()
{
}

void
periodic::parse()
{
  buf >> type;
  buf >> version;
  buf >> op;

  buf >> rend;

  buf >> return_rend;
}

void
periodic::write()
{
  buf << type;
  buf << version;
  buf << op;

  buf << rend;

  buf << return_rend;
}
