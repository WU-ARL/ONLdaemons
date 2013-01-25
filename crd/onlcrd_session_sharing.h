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

#ifndef _ONLCRD_SESSION_SHARING_H
#define _ONLCRD_SESSION_SHARING_H

namespace onlcrd
{
  class observe_data
  {
    public:
      observe_data();
      observe_data(const observe_data& od);
      ~observe_data();

      observe_data& operator=(const observe_data& od);
  
      std::string getSID() { return sid.getString(); }
      std::string getObserver() { return observer.getString(); }
      std::string getObservee() { return observee.getString(); }
      std::string getPassword() { return password.getString(); }
      bool isAccept() { return accept; }
      uint32_t getProxySID() { return proxy_sid; }

      friend byte_buffer& operator<<(byte_buffer& buf, observe_data& od);
      friend byte_buffer& operator>>(byte_buffer& buf, observe_data& od);

    private: 
      nccp_string sid;
      nccp_string observer;
      nccp_string observee;
      nccp_string password;
      bool accept;
      uint32_t proxy_sid;
  }; // class observe_data
  
  byte_buffer& operator<<(byte_buffer& buf, observe_data& od);
  byte_buffer& operator>>(byte_buffer& buf, observe_data& od);

  // add an observer to a running session
  static const NCCP_OperationType NCCP_Operation_SessionAddObserver = 51;
  class add_observer_req : public rlicrd_request
  {
    public:
      add_observer_req(uint8_t* mbuf, uint32_t size);
      virtual ~add_observer_req();

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string username;
  }; // class add_observer_req

  // get list of observable sessions 
  static const NCCP_OperationType NCCP_Operation_GetObservableSessions = 52;
  class get_observable_req : public onld::request
  {
    public:
      get_observable_req(uint8_t* mbuf, uint32_t size);
      virtual ~get_observable_req();

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string username;
      nccp_string password;
  }; // class get_observable_req

  // send list of observable sessions back to user
  class observable_response : public onld::response
  {
    public:
      observable_response(request* req, std::string msg, Session_Error err);
      observable_response(request* req, std::list<session_ptr>& list);
      virtual ~observable_response();

      virtual void write();

    protected:
      NCCP_StatusType status;
      nccp_string status_msg;
      Session_Error error_code;
      std::list<session_ptr> sess_list;
  }; // class observable_response

  class observe_req : public onld::request
  {
    public:
      observe_req(uint8_t* mbuf, uint32_t size);
      virtual ~observe_req();

      observe_data& getObserveData() { return data; }

      virtual void parse();

    protected:
      observe_data data;
  }; // class observe_req

  // observe a session
  static const NCCP_OperationType NCCP_Operation_ObserveSession = 53;
  class observe_session_req : public observe_req
  {
    public:
      observe_session_req(uint8_t* mbuf, uint32_t size);
      virtual ~observe_session_req();

      virtual bool handle();
  }; // class observe_session_req

  static const NCCP_OperationType NCCP_Operation_GrantObserveSession = 54;
  class grant_observe_req : public observe_req
  {
    public:
      grant_observe_req(uint8_t* mbuf, uint32_t size);
      virtual ~grant_observe_req();

      virtual bool handle();
  }; // class grant_observe_req

  // send observe responses back to users
  class observe_response : public onld::response
  {
    public:
      observe_response(observe_req* req, NCCP_StatusType stat);
      observe_response(observe_req* req, std::string msg, Session_Error err);
      observe_response(observe_req* req, std::string msg, Session_Error err, NCCP_StatusType stat);
      observe_response(request* req, std::string msg, Session_Error err, NCCP_StatusType stat, observe_data& d);
      virtual ~observe_response();

      virtual void write();

    protected:
      NCCP_StatusType status;
      nccp_string status_msg;
      Session_Error error_code;
      observe_data data;
  }; // class observable_response
};

#endif // _ONLCRD_SESSION_SHARING_H
