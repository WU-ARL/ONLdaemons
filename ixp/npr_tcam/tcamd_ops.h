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

#ifndef _TCAMD_OPS_H
#define _TCAMD_OPS_H

namespace tcamd 
{
  #define DEFAULT_TCAM_CONF_FILE "/onl/bin/npr_tcam.ini"

  enum npus { NPRA, NPRB, NONE };

  enum tcam_states { INIT, STOP };

  int inittcam(std::string);

  int routerinit(int, char *);
  int routerstop(int, char *);

  int addroute(int, int, char *, char *);
  int delroute(int, int, char *, char *);
  int queryroute(int, int, char *, char *);

  int addpfilter(int, int, char *, char *);
  int delpfilter(int, int, char *, char *);
  int querypfilter(int, int, char *, char *);

  int addafilter(int, int, char *, char *);
  int delafilter(int, int, char *, char *);
  int queryafilter(int, int, char *, char *);

  int addarp(int, int, char *, char *);
  int delarp(int, int, char *, char *);
  int queryarp(int, int, char *, char *);
};
#endif // _TCAMD_OPS_H
