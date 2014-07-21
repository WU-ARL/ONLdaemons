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

#ifndef _ONL_REQUESTS_H
#define _ONL_REQUESTS_H

namespace onld
{
  static const NCCP_OperationType NCCP_Operation_IAmUp = 57;
  class i_am_up: public request
  {
    public:
      i_am_up(uint8_t *mbuf, uint32_t size);
      i_am_up();
      virtual ~i_am_up();

      std::string get_cp() { return cp.getString(); }

      virtual void parse();
      virtual void write();

    protected:
      nccp_string cp;
  }; //class i_am_up

  class crd_request : public request
  {
    public:
      crd_request(uint8_t *mbuf, uint32_t size);
      crd_request();
      crd_request(experiment& e, component& c);
      virtual ~crd_request();

      virtual void parse();
      virtual void write();

      experiment getExperiment() { return exp; }
      component getComponent() { return comp; }

    protected:
      experiment exp;
      component comp;
  }; //class crd_request

  static const NCCP_OperationType NCCP_Operation_StartExperiment = 55;
  class start_experiment : public crd_request
  {
    public:
      start_experiment(uint8_t *mbuf, uint32_t size);
      start_experiment(experiment& e, component& c, std::string ip);
      virtual ~start_experiment();

      virtual void parse();
      virtual void write();

      void set_init_params(std::list<param>& params);
      void set_cores(uint32_t c) { cores = c;}
      void set_memory(uint32_t m) { memory = m;}

    protected:
      nccp_string ipaddr;
      uint32_t cores;
      uint32_t memory;
      std::list<param> init_params;
  }; //class start_experiment

  // cgw, do we need to keep this here?  it isn't sent to daemons from the crd or rli..
  static const NCCP_OperationType NCCP_Operation_EndExperiment = 43;
  class end_experiment : public crd_request
  {
    public:
      end_experiment(uint8_t *mbuf, uint32_t size);
      end_experiment(experiment& e, component& c);
      virtual ~end_experiment();

      virtual void parse();
      virtual void write();

    protected:
  }; //class end_experiment

  static const NCCP_OperationType NCCP_Operation_Refresh = 35;
  class refresh : public crd_request
  {
    public:
      refresh(uint8_t *mbuf, uint32_t size);
      refresh(experiment& e, component& c);
      virtual ~refresh();

      virtual void parse();
      virtual void write();

      void set_reboot_params(std::list<param>& params);

    protected:
      std::list<param> reboot_params;
  }; //class refresh

  static const NCCP_OperationType NCCP_Operation_CfgNode = 36;
  static const NCCP_OperationType NCCP_Operation_EndCfgNode = 37;
  static const NCCP_OperationType NCCP_Operation_LinkUp = 56; // cgw, do we really need this?
  class configure_node : public crd_request
  {
    public:
      configure_node(uint8_t *mbuf, uint32_t size);
      configure_node(experiment& e, component& c, node_info& ni);
      virtual ~configure_node();

      virtual void parse();
      virtual void write();


    protected:
      node_info node_conf;
  }; // class configure_node

  class end_configure_node : public crd_request
  {
    public:
      end_configure_node(uint8_t *mbuf, uint32_t size);
      end_configure_node(experiment& e, component& c);
      virtual ~end_configure_node();

  }; // class end_configure_node

  class rli_request : public request
  {
    public:
      rli_request(uint8_t *mbuf, uint32_t size);
      virtual ~rli_request();

      virtual void parse();

    protected:
      uint32_t id;
      uint16_t port;
      nccp_string version;
      std::vector<param> params;
  }; //class rli_request

	//ard: Start of vm code
  static const NCCP_OperationType NCCP_Operation_startVM = 67;
  class start_vm : public request
  {
	public:
	  start_vm(uint8_t *mbuf, uint32_t size);
		virtual ~start_vm();
		
		virtual void parse();
		virtual void write();
		
  protected:
		nccp_string name;
  }; //class start_vm
};

#endif // _ONL_REQUESTS_H
