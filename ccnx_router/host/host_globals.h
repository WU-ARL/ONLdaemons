/*
 * ONL Notice
 *
 * Copyright (c) 2009-2013 Pierluigi Rolando, Charlie Wiseman, Jyoti Parwatikar, John DeHart
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
 *   limitations under the License.
 */

#ifndef _HOST_GLOBALS_H
#define _HOST_GLOBALS_H

#include <string>
#include <vector>
#include <pthread.h>

namespace host
{
  extern dispatcher* the_dispatcher;
  extern nccp_listener* rli_conn;
  extern configuration* conf;
  extern std::string username;
  extern std::vector<std::string> paths;
  extern std::vector<std::string> nexthops;
  extern pthread_mutex_t mutex;			// saves the world from race condition over vector updates
}; // namespace host

#endif // _HOST_GLOBALS_H
