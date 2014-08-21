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
 */

#ifndef _VMD_REQUESTS_H
#define _VMD_REQUESTS_H

namespace vmd
{
  class start_experiment_req : public onld::start_experiment
  {
  public:
    start_experiment_req(uint8_t *mbuf, uint32_t size);
    virtual ~start_experiment_req();
 
    virtual bool handle();
  }; // class start_experiment_req

  class configure_node_req : public onld::configure_node
  {
  public:
    configure_node_req(uint8_t *mbuf, uint32_t size);
    virtual ~configure_node_req();
 
    virtual bool handle();
  }; // class configure_node_req

  class end_configure_node_req : public onld::end_configure_node
  {
  public:
    end_configure_node_req(uint8_t *mbuf, uint32_t size);
    virtual ~end_configure_node_req();
 
    virtual bool handle();
  }; // class end_configure_node_req

  class refresh_req : public onld::refresh
  {
  public:
    refresh_req(uint8_t *mbuf, uint32_t size);
    virtual ~refresh_req();
 
    virtual bool handle();
  }; // class refresh_req

  class end_experiment_req : public onld::end_experiment
  {
  public:
    end_experiment_req(uint8_t *mbuf, uint32_t size);
    virtual ~end_experiment_req();
 
    virtual bool handle();
  };
};

#endif // _VMD_REQUESTS_H
