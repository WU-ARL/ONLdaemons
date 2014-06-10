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

#ifndef _ONLVMBASED_REQUESTS_H
#define _ONLVMBASED_REQUESTS_H

namespace onlvmbased
{
  class start_experiment_req : public onld::start_experiment
  {
    public:
      start_experiment_req(uint8_t *mbuf, uint32_t size);
      virtual ~start_experiment_req();
 
      virtual bool handle();

    private:
      bool start_specialization_daemon(std::string specd);
      bool connect_to_specialization_daemon();
      int system_cmd(std::string cmd);
  }; // class start_experiment_req

  class refresh_req : public onld::refresh
  {
    public:
      refresh_req(uint8_t *mbuf, uint32_t size);
      virtual ~refresh_req();
 
      virtual bool handle();
  }; // class refresh_req

  static const NCCP_OperationType NCCP_Operation_UserData = 62;
  class user_data_req : public onld::rli_request
  {
    public:
      user_data_req(uint8_t *mbuf, uint32_t size);
      virtual ~user_data_req();

      virtual bool handle();

      virtual void parse();

    protected:
      std::string file;
      uint32_t field;
 
      userdata* user_data;
  }; // class user_data_req

  static const NCCP_OperationType NCCP_Operation_UserDataTS = 63;
  class user_data_ts_req : public onld::rli_request
  {
    public:
      user_data_ts_req(uint8_t *mbuf, uint32_t size);
      virtual ~user_data_ts_req();

      virtual bool handle();

      virtual void parse();

    protected:
      std::string file;
      uint32_t field;

      userdata* user_data;
  }; // class user_data_ts_req

  class rli_relay_req : public onld::rli_request
  {
    public:
      rli_relay_req(uint8_t *mbuf, uint32_t size);
      virtual ~rli_relay_req();

      virtual bool handle();

      virtual void parse();
      virtual void write();

    protected:
      byte_buffer data;
  }; // class rli_relay_req

  class crd_relay_req : public onld::crd_request
  {
    public:
      crd_relay_req(uint8_t *mbuf, uint32_t size);
      virtual ~crd_relay_req();

      virtual bool handle();

      virtual void parse();
      virtual void write();

    protected:
      byte_buffer data;
  }; // class crd_relay_req
};

#endif // _ONLVMBASED_REQUESTS_H
