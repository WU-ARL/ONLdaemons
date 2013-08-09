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
#include <exception>

#include <unistd.h> 
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#include "tcamd_logfile.h"
#include "tcamd_util.h"
#include "tcamd_ops.h"
#include <npr/nprd_types.h>
#include "tcamd_cmds.h"

namespace tcamd
{
  LogFile *tcamlog;

  void print_usage(char *prog)
  {
    std::cout << prog << " [--help] [--port num] [--logfile log] [--debuglevel level] [--tcamconf conf.ini]\n"
              << "\t--help: print this message\n"
              << "\t--port num: default is " << DEFAULT_TCAMD_PORT << "\n"
              << "\t--logfile log: default is " << DEFAULT_TCAMD_LOG << "\n"
              << "\t--debuglevel level: default is LOG_NORMAL (option: LOG_VERBOSE)\n"
              << "\t--tcamconf conf.ini: default is " << DEFAULT_TCAM_CONF_FILE << "\n\n";
    exit(0);
  }

  void write_err(unsigned int level, std::string e)
  {
    char err[30];
    strerror_r(errno, err, 30);
    try {
      tcamlog->write(level, e + ": " + std::string(err));
    } catch (log_exception& le) {
      std::string ee = "Couldn't write to log file!" + std::string(le.what());
      print_err(ee);
    }
  }

  void write_log(unsigned int level, std::string s)
  {
    try {
      tcamlog->write(level, s);
    } catch (log_exception& le) {
      std::string ee = "Couldn't write to log file!" + std::string(le.what());
      print_err(ee);
    }
  }

  void print_err(std::string s)
  {
    char tstr[30];
    time_t t = time(0);
    ctime_r(&t, tstr);
    tstr[20] = '\0';
    std::cerr << tstr << s << std::endl;
  }

}; // namespace tcamd
