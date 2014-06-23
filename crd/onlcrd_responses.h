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

#ifndef _ONLCRD_RESPONSES_H
#define _ONLCRD_RESPONSES_H

namespace onlcrd
{
  typedef uint32_t Session_Error;
  static const Session_Error Session_No_Err      = 0;
  static const Session_Error Session_Auth_Err    = 1;
  static const Session_Error Session_No_Res_Err  = 2;
  static const Session_Error Session_Alloc_Err   = 3;
  static const Session_Error Session_Expired     = 4;
  static const Session_Error Session_Alert       = 5;
  static const Session_Error Session_Version_Err = 6;
  static const Session_Error Session_Observe_Req = 7;
  static const Session_Error Session_Ack         = 8;

  class session_response : public onld::response
  {
    public:
      session_response(request *req, std::string eid);
      session_response(request *req, nccp_string& eid);
      session_response(request *req, std::string msg, Session_Error err);
      session_response(request *req, nccp_string& eid, std::string msg, Session_Error err);
      virtual ~session_response();

      virtual void write();

    protected:
      NCCP_StatusType status;
      nccp_string id;
      nccp_string status_msg;
      Session_Error error_code; 
  }; // class session_response

  class reservation_response : public onld::response
  {
    public:
      reservation_response(request *req, std::string msg);
      reservation_response(request *req, std::string msg, Session_Error err);
      reservation_response(request *req, std::string msg, std::string start);
      reservation_response(request *req, NCCP_StatusType stat, std::string msg, Session_Error err);
      virtual ~reservation_response();

      virtual void write();

    protected:
      NCCP_StatusType status;
      nccp_string status_msg;
      Session_Error error_code;
      nccp_string start_time;
  }; // class reservation_response

  class rlicrd_response : public onld::response
  {
    public:
      rlicrd_response(rlicrd_request *req, NCCP_StatusType stat);
      virtual ~rlicrd_response();
   
      virtual void write();
  
    protected:
      NCCP_StatusType status;
      experiment exp;
      component comp;
  }; // class rlicrd_response

  class component_response : public rlicrd_response
  {
    public: 
      component_response(rlicrd_request *req, NCCP_StatusType stat);
      component_response(rlicrd_request *req, std::string node, uint32_t port, std::string vmn, NCCP_StatusType stat=NCCP_Status_Fine);
      virtual ~component_response();

      virtual void write();

    protected:
      nccp_string cp; 
      uint32_t cp_port;
      nccp_string vmname; //only for RLI version > 8.0
  }; // class component_response

  class cluster_response : public rlicrd_response
  {
    public:
      cluster_response(rlicrd_request *req, NCCP_StatusType stat);
      cluster_response(rlicrd_request *req, NCCP_StatusType stat, uint32_t index);
      virtual ~cluster_response();

      virtual void write();

    protected:
      uint32_t cluster_index;
  }; // class component_response
};

#endif // _ONLCRD_RESPONSES_H
