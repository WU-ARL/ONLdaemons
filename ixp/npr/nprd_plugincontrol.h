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

#ifndef _NPRD_PLUGINCONTROL_H
#define _NPRD_PLUGINCONTROL_H

namespace npr
{
  class plugin_control_msg_req;

  class plugincontrol_exception: public std::runtime_error
  {
    public:
      plugincontrol_exception(const std::string& e): std::runtime_error("Plugincontrol exception: " + e) {}
  };

  class PluginControl
  {
    public:
      PluginControl() throw();
      ~PluginControl() throw();

      void send_msg_to_plugin(unsigned int, std::string, plugin_control_msg_req *) throw(plugincontrol_exception, pluginconn_exception, pluginmessage_exception);
      void pass_result_to_rli(PluginMessage *) throw(plugincontrol_exception, pluginmessage_exception);

      void start_plugin_debugging(std::string) throw(plugincontrol_exception);
      void plugin_debug_msg(PluginMessage *) throw(pluginmessage_exception);

    private:
      pthread_mutex_t plugin0_control_lock;
      pthread_mutex_t plugin1_control_lock;
      pthread_mutex_t plugin2_control_lock;
      pthread_mutex_t plugin3_control_lock;
      pthread_mutex_t plugin4_control_lock;

      plugin_control_msg_req* plugin0_msg;
      plugin_control_msg_req* plugin1_msg;
      plugin_control_msg_req* plugin2_msg;
      plugin_control_msg_req* plugin3_msg;
      plugin_control_msg_req* plugin4_msg;

      log_file *plugin_debug_log;
  };
};

#endif // _NPRD_PLUGINCONTROL_H
