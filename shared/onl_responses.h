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

#ifndef _ONL_RESPONSES_H
#define _ONL_RESPONSES_H

namespace onld
{
  typedef uint32_t NCCP_StatusType;
  static const NCCP_StatusType NCCP_Status_OperationIncomplete = 0;
  static const NCCP_StatusType NCCP_Status_Fine                = 1;
  static const NCCP_StatusType NCCP_Status_TimeoutLocal        = 2;
  static const NCCP_StatusType NCCP_Status_TimeoutRemote       = 3;
  static const NCCP_StatusType NCCP_Status_StillRemaining      = 4;
  static const NCCP_StatusType NCCP_Status_Failed              = 5;
  static const NCCP_StatusType NCCP_Status_AllocFailed         = 6;

  class crd_response: public response
  {
    public:
      crd_response(uint8_t *mbuf, uint32_t size);
      crd_response(crd_request *req);
      crd_response(crd_request *req, NCCP_StatusType stat);
      virtual ~crd_response();

      virtual void parse();
      virtual void write();

      NCCP_StatusType getStatus() { return status; }
      experiment& getExperiment() { return exp; }
      component& getComponent() { return comp; }

    protected:
      NCCP_StatusType status;
      experiment exp;
      component comp;
  }; //class crd_response

  class crd_hwresponse: public crd_response
  {
    public:
      crd_hwresponse(uint8_t *mbuf, uint32_t size);
      crd_hwresponse(crd_request *req, NCCP_StatusType stat, uint32_t i);
      virtual ~crd_hwresponse();

      virtual void parse();
      virtual void write();

    protected:
      uint32_t id;
  }; //class crd_hwresponse

  class rli_response: public response
  {
    public:
      rli_response(uint8_t *mbuf, uint32_t size);
      rli_response(rli_request* req);
      rli_response(rli_request* req, NCCP_StatusType stat);
      rli_response(rli_request* req, NCCP_StatusType stat, uint32_t val);
      rli_response(rli_request* req, NCCP_StatusType stat, uint32_t val, uint32_t ts, uint32_t rate);
      rli_response(rli_request* req, NCCP_StatusType stat, std::string msg);
      virtual ~rli_response();

      virtual void parse();
      virtual void write();

      NCCP_StatusType getStatus() { return status; }
      uint32_t getTimeStamp() { return time_stamp; }
      uint32_t getClockRate() { return clock_rate; }
      uint32_t getValue() { return value; }
      nccp_string getStatusMsg() { return status_msg; }

    protected:
      NCCP_StatusType status;
      uint32_t time_stamp;
      uint32_t clock_rate;
      uint32_t value;
      nccp_string status_msg;
  }; //class rli_response
};

#endif // _ONL_RESPONSES_H
