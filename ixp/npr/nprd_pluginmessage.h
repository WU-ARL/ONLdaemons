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

#ifndef _NPRD_PLUGINMESSAGE_H
#define _NPRD_PLUGINMESSAGE_H

namespace npr
{
  class pluginmessage_exception: public std::runtime_error
  {
    public:
      pluginmessage_exception(const std::string& e): std::runtime_error("Pluginmessage exception: " + e) {}
  };

  class PluginMessage
  {
    public:
      #define PCONTROLMSG        0
      #define PCONTROLMSGRSP     1
      #define PDEBUGMSG          2
      #define PCONFIGMSGRSP      3
      #define PADDPLUGIN         4
      #define PREMPLUGIN         5
      #define PQUERYROUTE        6
      #define PQUERYROUTERSP     7
      #define PADDROUTE          8
      #define PDELROUTE          9
      #define PQUERYPFILTER      10
      #define PQUERYPFILTERRSP   11
      #define PADDPFILTER        12
      #define PDELPFILTER        13
      #define PQUERYAFILTER      14
      #define PQUERYAFILTERRSP   15
      #define PADDAFILTER        16
      #define PDELAFILTER        17
      #define PMODSAMPLERATES    18
      #define PMODQUEUEQUANTUM   19
      #define PMODQUEUETHRESHOLD 20
      #define PMODPORTRATES      21
      #define PMODMUXQUANTA      22
      #define PMODEXCEPTIONDESTS 23
      #define PMAXTYPE           23

      PluginMessage(unsigned int *, unsigned int) throw(pluginmessage_exception);
      ~PluginMessage() throw();

      unsigned int type() throw();
      unsigned int getnumwords() throw();
      unsigned int getplugin() throw();
      unsigned int getword(unsigned int) throw(pluginmessage_exception);
      plugin_control_header *gethdr() throw();
      unsigned int *getmsg() throw();
  
    private:
      plugin_control_header *hdr;
      unsigned int *msg;
      unsigned int plugin;
  };
};

#endif // _NPRD_PLUGINMESSAGE_H
