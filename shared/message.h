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

#ifndef _MESSAGE_H
#define _MESSAGE_H

namespace onld
{
  class nccp_connection;

  typedef uint16_t NCCP_MessageType; 
  typedef uint8_t NCCP_OperationType; 

  static const NCCP_MessageType NCCP_MessageRequest  = 1;
  static const NCCP_MessageType NCCP_MessageResponse = 2;
  static const NCCP_MessageType NCCP_MessagePeriodic = 3;

  static const NCCP_OperationType NCCP_Operation_NOP = 0;
  static const NCCP_OperationType NCCP_Operation_StopPeriodic = 33;
  static const NCCP_OperationType NCCP_Operation_AckPeriodic = 34;

  class message
  {
    #define PERIODIC 255
    #define NCCP_MAX_MESSAGE_SIZE 1024
    public:
      message();
      message(uint8_t *mbuf, uint32_t size);
      virtual ~message();

      byte_buffer *getBuffer() { return &buf; }

      NCCP_MessageType get_type() { return type; }
      int get_version() { return version; }
      NCCP_OperationType get_op() { return op; }
      bool is_periodic() { return periodic_message; }
      void set_rendezvous(rendezvous& r) { rend = r; }
      rendezvous& get_rendezvous() { return rend; }

      void set_return_rendezvous(rendezvous& r) { return_rend = r; }
      rendezvous& get_return_rendezvous() { return return_rend; }

      virtual bool send();
      void set_connection(nccp_connection *conn);
      nccp_connection* get_connection() { return nccpconn; }

      virtual void parse();
      virtual void write();
      
    protected:
      byte_buffer buf;

      NCCP_MessageType type;
      uint8_t version;
      NCCP_OperationType op;
      rendezvous rend;
      bool periodic_message; // stored as uint16_t

      rendezvous return_rend;

      nccp_connection *nccpconn;
  }; //class message

  class response;

  class periodic : public message
  {
    public:
      periodic();
      periodic(uint8_t *mbuf, uint32_t size);
      virtual ~periodic();

      virtual void parse();
      virtual void write();

    protected:
  }; //class response

  class request : public message
  {
    public:
      request();
      request(uint8_t *mbuf, uint32_t size);
      virtual ~request();

      virtual bool handle();
      virtual bool request_rendezvous(response *resp);
      virtual bool wait_for_response();
      virtual bool request_rendezvous(periodic *per);

      virtual bool send_and_wait();

      virtual void parse();
      virtual void write();

      response* get_response() { return resp; }
 
      void prepare_for_periodic() { received_stop = false; received_ack = false; }
      bool stopped() { return received_stop; }
      bool acked() { return received_ack; }
      void clear_ack() { received_ack = false; }

      void get_period(struct timespec *t);
   
    protected:
      struct timeval period; // sec and usec vals stored as uint32_t
     
      bool received_response;  // used when waiting for response
      uint32_t timeout; // number of seconds to wait for a response
      pthread_mutex_t response_mutex;
      pthread_cond_t response_cond;
      response *resp;

      bool received_stop; // set when a stop request comes in for a periodic request
      bool received_ack; // set when a periodic ack comes in for a periodic request
  }; //class request

  class response : public message
  {
    public:
      //response();
      response(uint8_t *mbuf, uint32_t size);
      response(request *r);

      virtual ~response();

      void set_request(request *r) { req = r; }
      request* get_request() { return req; }
      void set_more_message(bool more) { end_of_message = more; }
      bool more_message() { return !end_of_message; }

      virtual void parse();
      virtual void write();

    protected:
      request *req;
      bool end_of_message;
  }; //class response
};

#endif // _MESSAGE_H
