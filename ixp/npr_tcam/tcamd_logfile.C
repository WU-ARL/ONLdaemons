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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "tcamd_logfile.h"

using namespace tcamd;

LogFile::LogFile(std::string fn, unsigned short log_rotate, std::string l) throw(log_exception)
{
  if(log_rotate == LOG_ROTATE)
  {
    for(int i=4; i>=0; i--)
    {
      struct stat file_stats;
      std::ostringstream o1,o2;
      std::string f1;
      o1 << i;
      if(i==0) { f1 = fn; }
      else { f1 = fn + "." + o1.str(); }
      o2 << i+1;
      std::string f2 = fn + "." + o2.str();
      if(lstat(f1.c_str(), &file_stats))
      {
        continue;
      }
      if(rename(f1.c_str(),f2.c_str()) < 0)
      {
        std::string e;
        char err[30];
        strerror_r(errno, err, 30);
        e = "log rotate failed:  " + std::string(err);
        throw log_exception(e); 
      }
    }
  }

  if((fd = open(fn.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) < 0)
  {
    std::string e;
    char err[30];
    strerror_r(errno, err, 30);
    e = "open failed:  " + std::string(err);
    throw log_exception(e); 
  }

  if(l == "LOG_NORMAL")
  {
    debuglevel = LOG_NORMAL;
  }
  else if(l == "LOG_VERBOSE")
  {
    debuglevel = LOG_VERBOSE;
  }
  else
  {
    throw log_exception("invalid debug log level!"); 
  }
}

LogFile::~LogFile() throw()
{
  close(fd);
}

void LogFile::write(unsigned int level, std::string s) throw(log_exception)
{
  ssize_t rv;
  char tstr[30];
  time_t t;

  if(level <= debuglevel)
  {
    t = time(0);
    ctime_r(&t, tstr);
    tstr[20] = '\0';

    s = tstr + s + "\n";
    rv = ::write(fd, s.c_str(), s.length());
    if(rv != (ssize_t)s.length())
    {
      std::string e;
      char err[30];
      strerror_r(errno, err, 30);
      e = "logfile write failed:  " + std::string(err);
      throw log_exception(e); 
    }
  }
}

std::string LogFile::int2str(unsigned int i)
{
  std::string s;
  std::ostringstream o;
  o << i;
  s = o.str();
  return s;
}
