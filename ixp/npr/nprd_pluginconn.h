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

#ifndef _NPRD_PLUGINCONN_H
#define _NPRD_PLUGINCONN_H

namespace npr
{
  class pluginconn_exception: public std::runtime_error
  {
    public:
      pluginconn_exception(const std::string& e): std::runtime_error("Pluginconn exception: " + e) {}
  };

  class PluginConn
  {
    public:
      PluginConn() throw();
      ~PluginConn() throw();
  
      bool check() throw();
      npr::PluginMessage *readmsg() throw(pluginconn_exception,pluginmessage_exception);
      void writemsg(npr::PluginMessage *) throw(pluginconn_exception,pluginmessage_exception);

    private:
      pthread_mutex_t read_lock;
      pthread_mutex_t write_lock;
 
      bool msg_ready;
      int plugin;
      plugin_control_header hdr;
  };
};

#endif // _NPRD_PLUGINCONN_H
