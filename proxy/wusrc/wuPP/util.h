/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/util.h,v $
 * $Author: fredk $
 * $Date: 2006/03/24 17:34:58 $
 * $Revision: 1.1 $
 * $Name:  $
 *
 * File:   util.h
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  03/24/06 11:20:28 CST
 * 
 * Description:  
 */

#ifndef _WUPP_UTIL_H
#define _WUPP_UTIL_H

// the mutex must be properly initialized before passing to this object
class autoLock {
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
          wulog(wulogLib, wulogError, "autoLock: Unable to release lock, err = %d\n", res);
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
          wulog(wulogLib, wulogError, "autoLock: Unable to acquire lock, err = %d\n", res);
        else
          locked_ = true;
      }
      return res;
    }

    bool locked() {return locked_;}
    bool operator!() {return !locked();}
};




#endif
