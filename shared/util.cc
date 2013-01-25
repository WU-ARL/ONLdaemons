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
#include <string>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include "util.h"

namespace onld
{
  std::string int2str(unsigned int i)
  {
    std::string s;
    std::ostringstream o;
    o << i;
    s = o.str();
    return s;
  }

  void write_stderr(std::string msg, int errnum)
  {
    char tstr[30];
    time_t t = time(NULL);
    ctime_r(&t, tstr);
    tstr[20] = '\0';
    std::cerr << tstr;

    if(errnum != 0)
    {
      char err[30];
      strerror_r(errnum, err, 30);
      std::cerr << err;
    }

    std::cerr << msg << std::endl;
  }

  void write_stdout(std::string msg)
  {
    char tstr[30];
    time_t t = time(NULL);
    ctime_r(&t, tstr);
    tstr[20] = '\0';
    std::cout << tstr << msg << std::endl;
  }

  log_file::log_file(std::string filename) throw(log_exception)
  {
    for(int i=4; i>=0; i--)
    {
      struct stat file_stats;
      std::ostringstream o1,o2;
      std::string f1;
      o1 << i;
      if(i==0){ f1 = filename; }
      else { f1 = filename + "." + o1.str(); }
      o2 << i+1;
      std::string f2 = filename + "." + o2.str();
      if(lstat(f1.c_str(), &file_stats))
      {
        continue;
      }
      if(rename(f1.c_str(),f2.c_str()) < 0)
      {
        throw log_exception("log rotate failed"); 
      }
    }

    mode_t tmp_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
    //if((fd = ::open(filename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) < 0)
    if((fd = ::open(filename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,tmp_mode)) < 0)
    {
      throw log_exception("open failed for file " + filename); 
    }
    chmod(filename.c_str(), tmp_mode);
  
    pthread_mutex_init(&write_lock, NULL);
  }
  
  log_file::~log_file() throw()
  {
    close(fd);
  
    pthread_mutex_destroy(&write_lock);
  }
  
  void log_file::write(std::string msg, int errnum) throw()
  {
    ssize_t rv;
    std::string s;
  
    char tstr[30];
    time_t t = time(NULL);
    ctime_r(&t, tstr);
    tstr[20] = '\0';
    s = tstr;
  
    pthread_t tid = pthread_self();
    s += "tid(" + int2str(tid) + ") ";

    if(errnum != 0)
    {
      char err[30];
      strerror_r(errnum, err, 30);
      s += err;
    }
  
    s += msg + "\n";
  
    autoLock wlock(write_lock);
  
    rv = ::write(fd, s.c_str(), s.length());
    if(rv != (ssize_t)s.length())
    {
      write_stderr("log_file::write failed: ", errno);
      write_stderr("          original msg: " + msg);
    }
  }

  log_file* log;

  void write_log(std::string msg, int errnum)
  {
    if(log != NULL) 
    {  
      log->write(msg, errnum);
      return;
    }
    if(errnum != 0)
    {
      write_stderr(msg, errnum);
      return;
    }
    write_stdout(msg);
  }

  log_file* usage_log = NULL;
  void write_usage_log(std::string msg, int errnum)
  {
    if(usage_log != NULL) 
    {  
      usage_log->write(msg, errnum);
      return;
    }
    if(errnum != 0)
    {
      write_stderr(msg, errnum);
      return;
    }
    write_stdout(msg);
  }

  // result = t1 + t2;
  void timespec_add(struct timespec& result, const struct timespec t1, const struct timespec t2)
  {
    result.tv_sec = t1.tv_sec + t2.tv_sec; 
    result.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    if(result.tv_nsec >= 1000000000)
    {
      result.tv_sec++;
      result.tv_nsec -= 1000000000;
    }
  }

  // result = t1 - t2, if t1<t2, returns false, o/w returns true
  bool timespec_diff(struct timespec& result, const struct timespec t1, const struct timespec t2)
  {
    if(t1.tv_sec == t2.tv_sec)
    {
      result.tv_sec = 0;
      result.tv_nsec = t1.tv_nsec - t2.tv_nsec;
      if(result.tv_nsec < 0)
      {
        result.tv_sec--;
        result.tv_nsec = 1000000000 + result.tv_nsec;
        return false;
      }
      return true;
    }
    else if(t1.tv_sec < t2.tv_sec)
    {
      result.tv_sec = t1.tv_sec - t2.tv_sec + 1;
      result.tv_nsec = -1 * (1000000000 - t1.tv_nsec + t2.tv_nsec);
      if(result.tv_nsec <= -1000000000)
      {
        result.tv_sec--;
        result.tv_nsec += 1000000000;
      }
      if(result.tv_nsec < 0)
      {
        result.tv_sec--;
        result.tv_nsec = 1000000000 + result.tv_nsec;
      }
      return false;
    }

    //else t1.tv_sec > t2.tv_sec
    result.tv_sec = t1.tv_sec - t2.tv_sec - 1;
    result.tv_nsec = t1.tv_nsec + 1000000000 - t2.tv_nsec;
    if(result.tv_nsec >= 1000000000)
    { 
      result.tv_sec++;
      result.tv_nsec -= 1000000000;
    }
    return true;
  }
}; // namespace onld
