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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <netdb.h>
#include <pthread.h>

#include "util.h"
#include "byte_buffer.h"
#include "rendezvous.h"
#include "message.h"
#include "dispatcher.h"
#include "nccp_connection.h"

using namespace onld;

nccp_connection::nccp_connection(std::string addr, unsigned short port) throw(nccpconn_exception)
{
  struct sockaddr_in reqAddr;
  struct in_addr address;
  struct hostent *hp;
  struct hostent hs;
  char tmp[512];
  int hosterror;


  sockfd = -1;
  sock_ready = false;
  closed = false;
  finished = false;
  message_count = 0;

  std::string logstr = "nccp_connection::nccp_connection: connect to addr " + addr + " port " + int2str(port);
  write_log(logstr);

  if(port == 0)
  {
    return;
  }

/*
  if((hp = gethostbyname(addr.c_str())) == NULL)
  {
    throw nccpconn_exception("gethostbyname error");
  }
*/
  gethostbyname_r(addr.c_str(), &hs, tmp, 511, &hp, &hosterror);
  if(!hp)
  {
    throw nccpconn_exception("gethostbyname_r error");
  }

  memcpy(&address.s_addr, hp->h_addr_list[0], 4);

  local_addr = 0;
  local_port = 0;
  remote_addr = ntohl(address.s_addr);
  remote_port = port;

  bzero((char *)&reqAddr, sizeof(reqAddr));

  reqAddr.sin_family = AF_INET;
  reqAddr.sin_port = htons(port);
  reqAddr.sin_addr.s_addr = address.s_addr;

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    throw nccpconn_exception("socket creation error");
  }

  errno = 0; 
  if(connect(sockfd, (struct sockaddr *)&reqAddr, sizeof(reqAddr)) == -1)
  {
    switch(errno)
      {
        case EACCES:
          write_log("nccp_connection::connect error EACCES");
          break;
        case EPERM:
          write_log("nccp_connection::connect error EPERM");
          break;
        case EADDRINUSE:
          write_log("nccp_connection::connect error EADDRINUSE");
          break;
        case EAGAIN:
          write_log("nccp_connection::connect error EAGAIN");
          break;
        case EALREADY:
          write_log("nccp_connection::connect error EALREADY");
          break;
        case ECONNREFUSED:
          write_log("nccp_connection::connect error ECONNREFUSED");
          break;
        case EFAULT:
          write_log("nccp_connection::connect error EFAULT");
          break;
        case EINPROGRESS:
          write_log("nccp_connection::connect error EINPROGRESS");
          break;
        case ETIMEDOUT:
          write_log("nccp_connection::connect error ETIMEDOUT");
          break;
        default:
          write_log("nccp_connection::connect error " + int2str(errno));
       }
    throw nccpconn_exception("connect error");
  }

  pthread_mutex_init((&write_lock), NULL);
  pthread_mutex_init((&count_lock), NULL);

  dispatcher_ = dispatcher::get_dispatcher();
}

nccp_connection::nccp_connection(int sfd, uint32_t local_address, uint16_t local_portnum, uint32_t remote_address, uint16_t remote_portnum) throw(nccpconn_exception)
{
  sockfd = sfd;
  sock_ready = false;
  closed = false;
  finished = false;
  message_count = 0;
  local_addr = local_address;
  local_port = local_portnum;
  remote_addr = remote_address;
  remote_port = remote_portnum;

  pthread_mutex_init((&write_lock), NULL);
  pthread_mutex_init((&count_lock), NULL);

  dispatcher_ = dispatcher::get_dispatcher();
}

nccp_connection::~nccp_connection() throw()
{
  close();

  errno = 0;
  if(pthread_mutex_destroy(&write_lock) != 0)
  {
    if(errno != EBUSY)
    {
      write_log("nccp_connection:~nccp_connection(): warning: pthread_mutex_destroy failed");
    }
  }

  errno = 0;
  if(pthread_mutex_destroy(&count_lock) != 0)
  {
    if(errno != EBUSY)
    {
      write_log("nccp_connection:~nccp_connection(): warning: pthread_mutex_destroy failed");
    }
  }
}

bool
nccp_connection::operator==(const nccp_connection& c)
{
  if((local_addr == 0) || (local_port == 0)) return false;
  if((c.local_addr == 0) || (c.local_port == 0)) return false;

  if((local_addr == c.local_addr) && (local_port == c.local_port) &&
     (remote_addr == c.remote_addr) && (remote_port == c.remote_port))
  {
    return true;
  }
  return false;
}

void
nccp_connection::receive_messages(bool spawn) throw(nccpconn_exception)
{
  if(spawn)
  {
    pthread_t tid;
    pthread_attr_t tattr;
    if(pthread_attr_init(&tattr) != 0)
    {
      throw nccpconn_exception("pthread_attr_init failed");
    }
    if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
    {
      throw nccpconn_exception("pthread_attr_setdetachstate failed");
    }
    if(pthread_create(&tid, &tattr, nccp_connection_receive_messages_forever_wrapper, (void *)this) != 0)
    {
      throw nccpconn_exception("pthread_create failed");
    }
    return;
  }

  receive_messages_forever();
  finished = true;
  return;
}

void *
onld::nccp_connection_receive_messages_forever_wrapper(void *obj)
{
  nccp_connection *conn = (nccp_connection *)obj;
  conn->receive_messages_forever();
  if(conn->isFinished()) { delete conn; }
  else { conn->setFinished(); }
  return NULL;
}

void
nccp_connection::receive_messages_forever()
{
  while(1)
  {
    new_message_data* msginfo = NULL;
    try
    {
      if(closed || finished) return;
      if(!await_message()) continue;
      msginfo = get_message();
      if(msginfo == NULL) return;
    }
    catch(std::exception& e)
    {
      if(finished) return;
      std::string errmsg = "receive_messages: get_message failed: " + std::string(e.what());
      write_log(errmsg);
      return;
    }

    increment_msg_count();

    pthread_t tid;
    pthread_attr_t tattr;
    if(pthread_attr_init(&tattr) != 0)
    {
      write_log("pthread_attr_init failed", errno);
      break;
    }
    if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
    {
      write_log("pthread_attr_setdetachstate failed", errno);
      break;
    }
    if(pthread_create(&tid, &tattr, nccp_connection_process_message_wrapper, (void *)msginfo) != 0)
    {
      write_log("pthread_create failed", errno);
      break;
    }
  }
}

bool
nccp_connection::await_message() throw(nccpconn_exception)
{
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);

  struct timeval timeout;
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;

  int ret = select(sockfd+1, &readfds, (fd_set *)NULL, (fd_set *)NULL, &timeout);
  if(ret < 0)
  {
    throw nccpconn_exception("select failed");
  }
 
  if(ret == 0) return false;
  return true;
}

new_message_data*
nccp_connection::get_message() throw()
{
  uint8_t* buf = new uint8_t[NCCP_MAX_MESSAGE_SIZE];
  uint32_t msg_length;

  int n = read(sockfd, (char *)&msg_length, sizeof(msg_length));
  msg_length = ntohl(msg_length);
  if(n <= 0)
  {
    write_log("nccp_connection::get_message: got empty message, closing connection 1");
    close();
    delete[] buf;
    return NULL;
  }
//jp 3/14/11 possible fix for crash
  if (msg_length > NCCP_MAX_MESSAGE_SIZE)
  {
    std::string logstr = "nccp_connection::get_message: message larger than NCCP_MAX_MESSAGE_SIZE increasing buffer read " + int2str(n) + " bytes, len=" + int2str(msg_length);
    write_log(logstr);
    buf = new uint8_t[msg_length];
  }

  if(msg_length <= 0)
  {
    close();
    delete[] buf;
    return NULL;
  }

  uint32_t num_bytes_read = 0;
  while(num_bytes_read < msg_length)
  {
    n = read(sockfd, &buf[num_bytes_read], msg_length - num_bytes_read);
    if(n <= 0)
    {
      write_log("nccp_connection::get_message: got empty message, closing connection 2");
      close();
      delete[] buf;
      return NULL;
    }
    num_bytes_read += n;

    //logstr = "nccp_connection::get_message: read " + int2str(n) + " bytes";
    //write_log(logstr);
  }

  new_message_data* msginfo = new new_message_data();
  msginfo->msgbuf = buf;
  msginfo->msglen = msg_length;
  msginfo->conn = this;
  return msginfo;
}

void *
onld::nccp_connection_process_message_wrapper(void *obj)
{
  if(obj == NULL) return NULL;
  new_message_data* msginfo = (new_message_data*)obj;

  if((msginfo->conn != NULL) && (msginfo->msglen != 0) && (msginfo->msgbuf != NULL))
  {
    msginfo->conn->process_message(msginfo->msgbuf, msginfo->msglen);
  }

  if(msginfo->msgbuf != NULL) { delete[] msginfo->msgbuf; }
  delete msginfo;

  return NULL;
}

void
nccp_connection::process_message(uint8_t* mbuf, uint32_t size)
{
  message *msg = dispatcher_->create_message(mbuf, size);
//write_log("nccp_connection::process_message: Entered, after call to dispatcher->create_message\n" );
  if(msg == NULL) { 
  write_log("nccp_connection::process_message dispatcher->create_message returned null");
  return; 
  }
  msg->set_connection(this);

  //write_log("nccp_connection::process_message: Before switch(msg->get_type()\n" );//JP REMOVE
  switch(msg->get_type())
  {
    case NCCP_MessageRequest:
    {
      request *req = (request *)msg;
      bool delete_msg = true;
//write_log("nccp_connection::process_message: NCCP_MessageRequest\n" );
      if(req->is_periodic())
      {
//write_log("nccp_connection::process_message: NCCP_MessageRequest is_periodic\n" );
        dispatcher_->register_periodic_rendezvous(req);
        struct timespec cur_time, req_period, last_fire_time, next_fire_time, sleep_time, remaining_time;
        int ns_ret;
	bool first = true;
        
        while(!req->stopped())
        {
          clock_gettime(CLOCK_REALTIME, &cur_time);
          req->get_period(&req_period);  
          //timespec_add(next_fire_time, cur_time, req_period);
          if (first)  {
            first = false;
            timespec_add(next_fire_time, cur_time, req_period);
          }
          else {
            timespec_add(next_fire_time, last_fire_time, req_period);
          }
          last_fire_time = next_fire_time;

/*
      write_log("nccp_connection::process_message: NCCP_MessageRequest is_periodic while ! req->stopped\n" );

      write_log("nccp_connection::process_message: last_fire_time " + int2str((last_fire_time.tv_sec * 1000) + (last_fire_time.tv_nsec/1000000)) + "\n");
      write_log("nccp_connection::process_message: next_fire_time " + int2str((next_fire_time.tv_sec * 1000) + (next_fire_time.tv_nsec/1000000)) + "\n");
      write_log("nccp_connection::process_message: cur_time " + int2str((cur_time.tv_sec * 1000) + (cur_time.tv_nsec/1000000)) + "\n");
      write_log("nccp_connection::process_message: req_period " + int2str((req_period.tv_sec * 1000) + (req_period.tv_nsec/1000000)) + "\n");

      write_log("nccp_connection::process_message: NCCP_MessageRequest is_periodic while ! req->stopped calling handle\n" );
*/

          delete_msg = req->handle();
         
          clock_gettime(CLOCK_REALTIME, &cur_time);
          if(timespec_diff(sleep_time, next_fire_time, cur_time) == false)
          {
            continue;
          }

          while(((ns_ret = nanosleep(&sleep_time, &remaining_time)) != 0) && (errno == EINTR))
          {
            sleep_time.tv_sec = remaining_time.tv_sec;
            sleep_time.tv_nsec = remaining_time.tv_nsec;
          }
          if(ns_ret == -1)
          {
            write_log("nccp_connection::process_message(): nanosleep failed");
            break;
          }
        }

        dispatcher_->clear_periodic_rendezvous(req);
        write_log("nccp_connection::process_message(): periodic request stopped");
      }
      else
      {
//write_log("nccp_connection::process_message: NCCP_MessageRequest else of the if is_periodic\n" );
        delete_msg = req->handle();
      }
      if(delete_msg) { delete req; }
      break;
    }
 
    case NCCP_MessageResponse:
    {
      //write_log("nccp_connection::process_message: NCCP_MessageResponse\n" );//JP REMOVE
      if(dispatcher_->call_rendezvous((response *)msg)) { delete msg; }
      //write_log("nccp_connection::process_message: rendezvous" );//JP REMOVE
      break;
    }

    case NCCP_MessagePeriodic:
    {
//write_log("nccp_connection::process_message: NCCP_MessagePeriodic\n" );
      if(dispatcher_->call_rendezvous((periodic *)msg)) { delete msg; }
      break;
    }

    default:
    {
      write_log("nccp_connection::process_message: unknown type " + int2str(msg->get_type()));
      break;
    }
  }
}

bool
nccp_connection::send_message(onld::message* msg) throw()
{
  autoLock rlock(write_lock);

  msg->write();

  byte_buffer *buf = msg->getBuffer();

  uint32_t msg_length = htonl(buf->getSize());

  if(buf->getSize() == 0)
  {
    write_log("nccp_connection::send_message: message size is 0!");
    return false;
  }

  if(send(sockfd, (char *)&msg_length, sizeof(msg_length), MSG_NOSIGNAL) < 0)
  {
    write_log("nccp_connection::send_message: send message size failed!");
    close();
    return false;
  }

  if(send(sockfd, (char *)buf->getData(), buf->getSize(), MSG_NOSIGNAL) < 0)
  {
    write_log("nccp_connection::send_message: send message data failed!");
    close();
    return false;
  }

  return true;
}

void
nccp_connection::increment_msg_count()
{
  autoLock clock(count_lock);
  message_count++;
}

void
nccp_connection::decrement_msg_count()
{
  autoLock clock(count_lock);
  message_count--;
}

bool
nccp_connection::has_outstanding_msgs()
{
  if(message_count == 0) return false;
  return true;
}

std::string
nccp_connection::get_remote_hostname()
{
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(remote_addr);

  char host[NI_MAXHOST];

  if(getnameinfo((struct sockaddr *)&saddr, sizeof(saddr), host, NI_MAXHOST, NULL, 0, NI_NAMEREQD|NI_NOFQDN) != 0)
  {
    write_log("nccp_connection::get_remote_hostname(): getnameinfo failed");
    return "";
  }

  std::string fullname = host;
  size_t firstdot = fullname.find_first_of('.');
  std::string hostname = fullname.substr(0,firstdot);

  return hostname;
}

void
nccp_connection::close() throw()
{
  if(closed) return;
  closed = true;

  ::close(sockfd);
  sockfd = -1;
}


nccp_listener::nccp_listener(std::string addr, unsigned short port) throw(nccpconn_exception)
{
  struct ifconf ifc;
  char buf[1024];
  int tmpsock;
  struct ifreq *ifr;
  struct sockaddr_in *temp;
  char address[INET_ADDRSTRLEN];
  bool addr_found;
  struct sockaddr_in reqAddr;
  
  acceptfd = -1;

  std::string logstr = "nccp_connection::nccp_connection: listen on addr " + addr + " port " + int2str(port);
  write_log(logstr);

  if(port == 0)
  {
    return;
  }

  /* use the addr network interface (prefix or not) */
  addr_found = false;
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if((tmpsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    throw nccpconn_exception("socket creation failed");
  }
  if(ioctl(tmpsock, SIOCGIFCONF, &ifc) < 0)
  {
    throw nccpconn_exception("ioctl SIOCGIFCONF failed");
  }
  ifr = ifc.ifc_req;
  for(int i = (ifc.ifc_len / sizeof(struct ifreq)); --i >= 0; ifr++)
  {
    temp = (struct sockaddr_in *)&ifr->ifr_addr;
    if(inet_ntop(AF_INET, &temp->sin_addr, address, INET_ADDRSTRLEN) == NULL)
    {
      throw nccpconn_exception("inet_ntop failed");
    }
    if(strncmp(address, addr.c_str(), addr.length()) == 0)
    {
      addr_found = true;
      break;
    }
  }
  if(addr_found == false)
  {
    throw nccpconn_exception("no such interface found:" + addr);
  }

  local_addr = ntohl(temp->sin_addr.s_addr);
  local_port = port;

  /* Now we need to create the channel waiting for connection requests */
  bzero((char *)&reqAddr, sizeof(reqAddr));

  reqAddr.sin_family = AF_INET;
  reqAddr.sin_port = htons(port);
  reqAddr.sin_addr.s_addr = temp->sin_addr.s_addr;

  /* create the command socket */
  if((acceptfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    throw nccpconn_exception("socket creation error");
  }
 
  /* bind the master register socket */
  if(bind(acceptfd, (struct sockaddr *) &reqAddr, sizeof(reqAddr)) < 0)
  {
    throw nccpconn_exception("bind error");
  }
 
  if(listen(acceptfd, 256) < 0)
  {
    throw nccpconn_exception("listen failed");
  }
}

nccp_listener::~nccp_listener() throw()
{
  while(!connections.empty())
  {
    nccp_connection *conn = (nccp_connection *)connections.front();
    connections.pop_front();
    delete conn;
  }

  if(acceptfd != -1) close(acceptfd);
  acceptfd = -1;
}

void
nccp_listener::receive_messages(bool spawn) throw(nccpconn_exception)
{
  if(spawn)
  { 
    pthread_t tid;
    pthread_attr_t tattr;
    if(pthread_attr_init(&tattr) != 0)
    {
      throw nccpconn_exception("pthread_attr_init failed");
    }
    if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
    {
      throw nccpconn_exception("pthread_attr_setdetachstate failed");
    }
    if(pthread_create(&tid, &tattr, nccp_listener_receive_messages_forever_wrapper, (void *)this) != 0)
    {
      throw nccpconn_exception("pthread_create failed");
    }
    return;
  }
  
  receive_messages_forever();
  return;
}

void *
onld::nccp_listener_receive_messages_forever_wrapper(void *obj)
{
  nccp_listener *listener = (nccp_listener *)obj;
  listener->receive_messages_forever();
  delete listener;
  return NULL;
}

void
nccp_listener::receive_messages_forever()
{
  if(acceptfd == -1) return;

  while(1)
  {
    cleanup();

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(acceptfd, &readfds);

    if(select(acceptfd+1, &readfds, (fd_set *)NULL, (fd_set *)NULL, NULL) < 0)
    {
      throw nccpconn_exception("select failed");
    }

    add_connection();
  }
}

void
nccp_listener::add_connection() throw()
{
  int new_sd;
  struct sockaddr_in sockAddr;
  unsigned int clilen = sizeof(sockAddr);

  new_sd = accept(acceptfd, (struct sockaddr *)&sockAddr, &clilen);

  uint32_t remote_addr = ntohl(sockAddr.sin_addr.s_addr);
  uint32_t remote_port = ntohs(sockAddr.sin_port);

  char blah[256];
  sprintf(blah, "nccp_connection::add_connection accepted a connection on sd=%d sockAddr.sin_port=%d sockAddr.sin_addr.s_addr=0x%08x", new_sd, remote_port, remote_addr);
  write_log(blah);

  nccp_connection *new_conn = new nccp_connection(new_sd, local_addr, local_port, remote_addr, remote_port);

  connections.push_back(new_conn);

  new_conn->receive_messages(true);
}

void
nccp_listener::cleanup() throw()
{
  std::list<nccp_connection *>::iterator it = connections.begin();
  
  while(it != connections.end())
  {
    if((*it)->isFinished() && !((*it)->has_outstanding_msgs()))
    { 
      nccp_connection *closedc = (nccp_connection *)(*it);
      it = connections.erase(it);
      delete closedc;
    }
    else
    {
      ++it;
    }
  }
}
