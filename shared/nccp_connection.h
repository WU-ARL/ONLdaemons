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

#ifndef _NCCP_CONNECTION_H
#define _NCCP_CONNECTION_H

namespace onld
{
  class nccpconn_exception : public std::runtime_error
  {
    public:
      nccpconn_exception(const std::string& e) : std::runtime_error("NCCP connection exception: " + e) {}
  };

  static const uint16_t Default_Proxy_Port = 7070;
  static const uint16_t Default_CRD_Port   = 3560;
  static const uint16_t Default_ND_Port    = 3551;
  static const uint16_t Default_NMD_Port   = 3551;

  typedef struct _new_message_data
  {
    uint8_t *msgbuf;
    uint32_t msglen;
    nccp_connection *conn;
  } new_message_data;

  class nccp_connection
  {
    public:
      // connect to addr:port
      nccp_connection(std::string addr, unsigned short port) throw(nccpconn_exception);
      // use already connected socket sfd
      nccp_connection(int sfd, uint32_t local_address, uint16_t local_portnum, uint32_t remote_address, uint16_t remote_portnum) throw(nccpconn_exception);
      ~nccp_connection() throw();

      bool operator==(const nccp_connection& c);

      void receive_messages(bool spawn) throw(nccpconn_exception);
      void receive_messages_forever();
      void process_message(uint8_t* mbuf, uint32_t size);

      bool send_message(onld::message *msg, bool print_debug=false) throw();
     
      bool isClosed() { return closed; }
      bool isFinished() { return finished; }
      void setFinished() { finished = true; }

      void increment_msg_count();
      void decrement_msg_count();
      bool has_outstanding_msgs();

      std::string get_remote_hostname();

    private:
      pthread_mutex_t write_lock;
      pthread_mutex_t count_lock;
      int sockfd;
      bool sock_ready;
      bool closed;
      bool finished;
      dispatcher* dispatcher_;
      int message_count;

      uint32_t local_addr;
      uint16_t local_port;
      uint32_t remote_addr;
      uint16_t remote_port;

      bool await_message() throw(nccpconn_exception);
      new_message_data* get_message() throw();
      void close() throw();
  }; // class nccp_connection

  void *nccp_connection_receive_messages_forever_wrapper(void *obj);
  void *nccp_connection_process_message_wrapper(void *obj);

  class nccp_listener
  {
    public:
      // listen for connections on addr(prefix):port
      nccp_listener(std::string& addr, unsigned short port) throw(nccpconn_exception);
      ~nccp_listener() throw();

      void receive_messages(bool spawn) throw(nccpconn_exception);
      void receive_messages_forever();

    private:
      std::list<nccp_connection *> connections;
      int  acceptfd;
      uint32_t local_addr;
      uint16_t local_port;

      void add_connection() throw();
      void cleanup() throw();
  };

  void *nccp_listener_receive_messages_forever_wrapper(void *obj);
};

#endif // _NCCP_CONNECTION_H
