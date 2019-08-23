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

#ifndef _UTIL_H
#define _UTIL_H

namespace onld 
{
  std::string int2str(unsigned int i);
  void write_stderr(std::string msg, int errnum=0);
  void write_stdout(std::string msg);


  class log_exception : public std::runtime_error
  {
    public:
      log_exception(const std::string& e) : std::runtime_error("Log exception: " + e) {}
  };

  class log_file
  {
    public:
      log_file(std::string filename) throw(log_exception);
      ~log_file() throw();
      void write(std::string msg, int errnum=0) throw();
  
    private:
      int fd;
      pthread_mutex_t write_lock;
  };

  extern log_file* usage_log;
  extern log_file* log;

  void write_log(std::string msg, int errnum=0);
  void write_usage_log(std::string msg, int errnum=0);

  // result = t1 + t2;
  void timespec_add(struct timespec& result, const struct timespec t1, const struct timespec t2);
  // result = t1 - t2, if t1<t2, returns false, o/w returns true
  bool timespec_diff(struct timespec& result, const struct timespec t1, const struct timespec t2);

  class autoRdLock
  {
    private:
      pthread_rwlock_t &lock_;
      bool locked_;

    public:
      autoRdLock(pthread_rwlock_t &l) : lock_(l), locked_(false) {lock();}
      ~autoRdLock() { unlock(); }
      int unlock()
      {
        int res = 0;
        if (locked_ == true) {
          if ((res = pthread_rwlock_unlock(&lock_)) != 0 && res != EDEADLK)
            write_stderr("pthread_rwlock_unlock failed!", errno);
          else
            locked_ = false;
        }
        return res;
      }
      int lock()
      {
        int res = 0;
        if (locked_ == false) {
          if ((res = pthread_rwlock_rdlock(&lock_)) != 0)
            write_stderr("pthread_rwlock_rdlock failed!", errno);
          else
            locked_ = true;
        }
        return res;
      }

      bool locked() {return locked_;}
      bool operator!() {return !locked();}
  }; 

  class autoWrLock
  {
    private:
      pthread_rwlock_t &lock_;
      bool locked_;

    public:
      autoWrLock(pthread_rwlock_t &l) : lock_(l), locked_(false) {lock();}
      ~autoWrLock() { unlock(); }
      int unlock()
      {
        int res = 0;
        if (locked_ == true) {
          if ((res = pthread_rwlock_unlock(&lock_)) != 0 && res != EDEADLK)
            write_stderr("pthread_rwlock_unlock failed!", errno);
          else
            locked_ = false;
        }
        return res;
      }
      int lock()
      {
        int res = 0;
        if (locked_ == false) {
          if ((res = pthread_rwlock_wrlock(&lock_)) != 0)
            write_stderr("pthread_rwlock_rdlock failed!", errno);
          else
            locked_ = true;
        }
        return res;
      }

      bool locked() {return locked_;}
      bool operator!() {return !locked();}
  }; 

class autoLock
  {
    private:
      pthread_mutex_t &lock_;
      bool locked_;

    public:
      autoLock(pthread_mutex_t &l) : lock_(l), locked_(false) {lock();}
      ~autoLock() { unlock(); }
      int unlock()
      {
        int res = 0;
        if (locked_ == true) {
          if ((res = pthread_mutex_unlock(&lock_)) != 0 && res != EDEADLK)
            write_stderr("pthread_mutex_unlock failed!", errno);
          else
            locked_ = false;
        }
        return res;
      }
      int lock()
      {
        int res = 0;
        if (locked_ == false) {
          if ((res = pthread_mutex_lock(&lock_)) != 0)
            write_stderr("pthread_mutex_lock failed!", errno);
          else
            locked_ = true;
        }
        return res;
      }

      bool locked() {return locked_;}
      bool operator!() {return !locked();}
  };

  class autoLockDebug
  {
    private:
      pthread_mutex_t &lock_;
      bool locked_;
      std::string name_;

    public:
      autoLockDebug(pthread_mutex_t &l, std::string name) : lock_(l), locked_(false), name_(name) {lock();}
      ~autoLockDebug() { if(locked_ == true) { unlock(); } }
      int unlock()
      {
        int res = 0;
        //write_log(name_ + " release lock");
        if(locked_ == true)
        {
          if((res = pthread_mutex_unlock(&lock_)) != 0 && res != EDEADLK)
          {
            write_log(name_ + " pthread_mutex_unlock failed");
          }
          else
          {
            locked_ = false;
          //  write_log(name_ + " released lock");
          }
        }
        else
        {
          write_log(name_ + " did not have lock");
        }
        return res;
      }
      int lock()
      {
        int res = 0;
        //write_log(name_ + " get lock");
        if(locked_ == false)
        {
          if((res = pthread_mutex_lock(&lock_)) != 0)
          {
            write_log(name_ + " pthread_mutex_lock failed");
          }
          else
          {
            locked_ = true;
         //   write_log(name_ + " got lock");
          }
        }
        else
        { 
          write_log(name_ + " already had lock");
        }
        return res;
      }

      bool locked() {return locked_;}
      bool operator!() {return !locked();}
  };
};

#endif // _UTIL_H
