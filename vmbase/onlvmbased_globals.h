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

#ifndef _ONLVMBASED_GLOBALS_H
#define _ONLVMBASED_GLOBALS_H

namespace onlvmbased
{
  extern dispatcher *the_dispatcher;
  extern nccp_listener *rli_conn;
  extern nccp_connection *spec_conn;
  extern session_manager *the_session_manager;

  extern bool using_spec_daemon;
  extern std::string user;
  extern bool testing;
  extern bool root_only;
}; // namespace onlvmbased

#endif // _ONLVMBASED_GLOBALS_H
