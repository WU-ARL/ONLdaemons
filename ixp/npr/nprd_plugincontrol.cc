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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <pwd.h>
#include <pthread.h>

#include "shared.h"

#include "nprd_types.h"
#include "nprd_configuration.h"
#include "nprd_monitor.h"
#include "nprd_pluginmessage.h"
#include "nprd_pluginconn.h"
#include "nprd_datapathmessage.h"
#include "nprd_datapathconn.h"
#include "nprd_plugincontrol.h"
#include "nprd_ipv4.h"
#include "nprd_arp.h"
#include "nprd_globals.h"
#include "nprd_requests.h"

using namespace npr;

PluginControl::PluginControl() throw()
{
  pthread_mutex_init(&plugin0_control_lock, NULL);
  pthread_mutex_init(&plugin1_control_lock, NULL);
  pthread_mutex_init(&plugin2_control_lock, NULL);
  pthread_mutex_init(&plugin3_control_lock, NULL);
  pthread_mutex_init(&plugin4_control_lock, NULL);

  plugin0_msg = NULL;
  plugin1_msg = NULL;
  plugin2_msg = NULL;
  plugin3_msg = NULL;
  plugin4_msg = NULL;
  
  plugin_debug_log = NULL;
}

PluginControl::~PluginControl() throw()
{
  pthread_mutex_destroy(&plugin0_control_lock);
  pthread_mutex_destroy(&plugin1_control_lock);
  pthread_mutex_destroy(&plugin2_control_lock);
  pthread_mutex_destroy(&plugin3_control_lock);
  pthread_mutex_destroy(&plugin4_control_lock);
}

// take the message from the rli and give it to the plugin. store the rli message
// in the correct local msg (one per plugin) so that when the response comes back
// we can correctly get it back to the rli.  only one outstanding message to each 
// plugin is allowed at any one time, so other threads calling this method will
// block until the current message is cleared
void PluginControl::send_msg_to_plugin(unsigned int plugin, std::string msg, plugin_control_msg_req* rlim) throw(plugincontrol_exception, pluginconn_exception, pluginmessage_exception)
{
  write_log("PluginControl::send_msg_to_plugin");

  write_log("PluginControl::send_msg_to_plugin: got ME " + int2str(plugin));
  write_log("PluginControl::send_msg_to_plugin: got msg " + msg);

  unsigned int numbytes = msg.length() + 1;
  unsigned int numwords;
  if(numbytes % 4 == 0)
  {
    numwords = numbytes / 4;
  }
  else
  {
    numwords = (numbytes / 4) + 1;
  }

  char *padded_msg = new char[numwords*4];
  strcpy(padded_msg, msg.c_str());

  write_log("PluginControl::send_msg_to_plugin: calculate numwords to be " + int2str(numwords));

  unsigned int *msgwords = new unsigned int[numwords+1];
  plugin_control_header *hdr = (plugin_control_header *)&msgwords[0];
  for(unsigned int i=1; i<=numwords; ++i)
  {
    unsigned int si = (i-1)*4;
    msgwords[i]  = (padded_msg[si]   & 0xff) << 24;
    msgwords[i] |= (padded_msg[si+1] & 0xff) << 16;
    msgwords[i] |= (padded_msg[si+2] & 0xff) << 8;
    msgwords[i] |= (padded_msg[si+3] & 0xff);
  }

  write_log("PluginControl::send_msg_to_plugin: about to grab the lock for ME " + int2str(plugin));

  pthread_mutex_t *lock;
  if(plugin == 0)
  {
    lock = &plugin0_control_lock;
  }
  else if(plugin == 1)
  {
    lock = &plugin1_control_lock;
  }
  else if(plugin == 2)
  {
    lock = &plugin2_control_lock;
  }
  else if(plugin == 3)
  {
    lock = &plugin3_control_lock;
  }
  else if(plugin == 4)
  {
    lock = &plugin4_control_lock;
  }
  else
  {
    throw plugincontrol_exception("invalid plugin");
  }

  if(pthread_mutex_lock(lock) != 0)
  {
    throw plugincontrol_exception("system error");
  }

  if(plugin == 0)
  {
    plugin0_msg = rlim;
  }
  else if(plugin == 1)
  {
    plugin1_msg = rlim;
  }
  else if(plugin == 2)
  {
    plugin2_msg = rlim;
  }
  else if(plugin == 3)
  {
    plugin3_msg = rlim;
  }
  else// if(plugin == 4)
  {
    plugin4_msg = rlim;
  }

  hdr->type = PCONTROLMSG;
  hdr->response_needed = 1;
  hdr->num_words = numwords;
  hdr->mid = 0;
  
  for(unsigned int i=0; i<=numwords; ++i)
  {
    char logstr[256];
    sprintf(logstr, "PluginControl::send_msg_to_plugin: msgwords[%u] = 0x%.8x",i,msgwords[i]);
    write_log(logstr);
  }

  write_log("PluginControl::send_msg_to_plugin: about to allocate PluginMessage");

  PluginMessage *pm = new PluginMessage(msgwords, plugin);
  delete[] msgwords;
  delete[] padded_msg;

  write_log("PluginControl::send_msg_to_plugin: about to send message to plugin");

  plugin_conn->writemsg(pm);

  write_log("PluginControl::send_msg_to_plugin: done sending message");

  delete pm;
  
  write_log("PluginControl::send_msg_to_plugin: done");

  return;
}

// take the response from the plugin and formulate the response to the RLI
void PluginControl::pass_result_to_rli(PluginMessage *pm) throw(plugincontrol_exception, pluginmessage_exception)
{
  write_log("in pass_result_to_rli");

  unsigned int plugin = pm->getplugin();
  unsigned int num_words = pm->getnumwords();

  write_log("PluginControl::pass_result_to_rli: got ME " + int2str(plugin));
  write_log("PluginControl::pass_result_to_rli: got num_words " + int2str(num_words));

  unsigned int hdrval = pm->getword(0);
  char logstr[256];
  sprintf(logstr, "PluginControl::pass_result_to_rli: word(0) = 0x%.8x", hdrval);
  write_log(logstr);

  char *msg = new char[num_words*4+1];
  for(unsigned int i=0; i<num_words; ++i)
  {
    unsigned int word = pm->getword(i+1);

    sprintf(logstr, "PluginControl::pass_result_to_rli: word(%u) = 0x%.8x", i+1, word);
    write_log(logstr);

    msg[i*4]   = (word >> 24) & 0xff;
    msg[i*4+1] = (word >> 16) & 0xff;
    msg[i*4+2] = (word >> 8) & 0xff;
    msg[i*4+3] = (word) & 0xff;
  }
  msg[num_words*4] = '\0';

  pthread_mutex_t *lock;
  plugin_control_msg_req* orig_msg;
  if(plugin == 0)
  {
    lock = &plugin0_control_lock;
    orig_msg = plugin0_msg;
  }
  else if(plugin == 1)
  {
    lock = &plugin1_control_lock;
    orig_msg = plugin1_msg;
  }
  else if(plugin == 2)
  {
    lock = &plugin2_control_lock;
    orig_msg = plugin2_msg;
  }
  else if(plugin == 3)
  {
    lock = &plugin3_control_lock;
    orig_msg = plugin3_msg;
  }
  else if(plugin == 4)
  {
    lock = &plugin4_control_lock;
    orig_msg = plugin4_msg;
  }
  else
  {
    throw plugincontrol_exception("invalid plugin");
  }

  if(pthread_mutex_unlock(lock) != 0)
  {
    throw plugincontrol_exception("system error");
  }

  std::string smsg = msg;
  write_log("PluginControl::pass_result_to_rli: msg is (" + smsg + ")");
  write_log("PluginControl::pass_result_to_rli: about to send response");

  if(orig_msg == NULL)
  {
    delete[] msg;
    throw plugincontrol_exception("system error");
  }

  orig_msg->send_plugin_response(smsg);
  
  write_log("PluginControl::pass_result_to_rli: about to delete msg");
  delete[] msg;
  delete orig_msg;

  write_log("PluginControl::pass_result_to_rli: done");

  if(plugin == 0)
  {
    plugin0_msg = NULL;
  }
  else if(plugin == 1)
  {
    plugin1_msg = NULL;
  }
  else if(plugin == 2)
  {
    plugin2_msg = NULL;
  }
  else if(plugin == 3)
  {
    plugin3_msg = NULL;
  }
  else //if(plugin == 4)
  {
    plugin4_msg = NULL;
  }

  return;
}

void PluginControl::start_plugin_debugging(std::string filename) throw(plugincontrol_exception)
{
  write_log("PluginControl::start_plugin_debugging: filename " + filename);

  if(plugin_debug_log != NULL)
  {
    delete plugin_debug_log;
    plugin_debug_log = NULL;
  }

  if(filename[0] == '.' || filename[0] == '/' || filename[0] == '\\')
  {
    throw plugincontrol_exception("invalid filename");
  }

  std::string user = configuration->get_username();

  /* would like to use getpwnam or something here, but I couldn't get that to use the NIS map
   * so instead you get this ugliness
  struct passwd *passwd_entry = getpwnam(user);
  if(passwd_entry == NULL)
  {
    throw plugincontrol_exception("invalid user!");
  }
  */
  char shcmd[256];
  sprintf(shcmd, "/usr/sbin/ypcat passwd | /bin/egrep '^%s:' | cut -d: -s -f3 > /tmp/uid", user.c_str());
  if(system(shcmd) < 0)
  {
    throw plugincontrol_exception("system error");
  }

  FILE *uid_file = fopen("/tmp/uid", "r");
  if(uid_file == NULL)
  {
    throw plugincontrol_exception("system error");
  }

  int uid;
  int rv = fscanf(uid_file, "%d", &uid);
  if(rv != 1)
  {
    throw plugincontrol_exception("system error");
  }

  if(fclose(uid_file) != 0)
  {
    throw plugincontrol_exception("system error");
  }

  if(seteuid(uid) != 0)
  {
    throw plugincontrol_exception("system error");
  }

  std::string fn = "/users/" + user + "/" + filename;

  write_log("PluginControl::start_plugin_debugging: full filename " + fn);

  try
  {
    plugin_debug_log = new log_file(fn);
  }
  catch(std::exception& e)
  {
    plugin_debug_log = NULL;
    throw plugincontrol_exception("couldn't open file " + fn);
  }

  write_log("PluginControl::start_plugin_debugging: filename " + fn + " ready");
}

void PluginControl::plugin_debug_msg(PluginMessage *pm) throw(pluginmessage_exception)
{
  write_log("in plugin_debug_msg");

  if(plugin_debug_log == NULL)
  {
    return;
  }

  unsigned int plugin = pm->getplugin();
  unsigned int num_words = pm->getnumwords();
  char *msg = new char[num_words*4+1];

  for(unsigned int i=0; i<num_words; ++i)
  {
    unsigned int word = pm->getword(i+1);
    msg[i*4]   = (word >> 24) & 0xff;
    msg[i*4+1] = (word >> 16) & 0xff;
    msg[i*4+2] = (word >> 8) & 0xff;
    msg[i*4+3] = (word) & 0xff;
  }
  msg[num_words*4] = '\0';

  std::string msgstr = "Plugin " + int2str(plugin) + ": " + std::string(msg);

  try
  {
    plugin_debug_log->write(msgstr);
  }
  catch(std::exception& e)
  {
    plugin_debug_log = NULL;
    write_log("plugin_debug_msg: error writing msg " + msgstr);
  }

  delete[] msg;

  return;
}
