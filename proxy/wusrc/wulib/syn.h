/* 
 * Description:
 *
 * $Author: fredk $
 * $Date: 2005/06/16 22:23:24 $
 * $Revision: 1.9 $
 *
 * Auther: Fred Kuhns
 * Organization: Applied Research Laboratory
 *               Computer Science Department
 *               Washington University in St. Louis
 *
 * */

#ifndef _WULIB_SYN_H
#define _WULIB_SYN_H

#define WULIB_SYNC_TYPE_NONE       0x00
#define WULIB_SYNC_TYPE_POSIX      0x01
#define WULIB_SYNC_TYPE_OTHER      0x02

#include <pthread.h>  /* pthread */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wusynops_thread_t {
  int (*mytid)(void); // returns oue ID as an int, used for debugging!
} wusynops_thread_t;

typedef struct wusynops_mutex_t {
  int (*lock_init)(void *lock, void *attr);
  int (*lock_destroy)(void *lock);
  int (*lockit)(void *lock);
  int (*unlockit)(void *lock);
} wusynops_mutex_t;

typedef struct wusynops_condvar_t {
  int (*cv_init)(void *cv, void *attr);
  int (*cv_destroy)(void *cv);
  int (*cvwait)(void * cv, void *lock);
  int (*cvsig)(void * cv);
  int (*cvbcast)(void *cv);
} wusynops_condvar_t;

typedef struct wusynops_t {
  wusynops_thread_t  tops; // thread OPs
  wusynops_mutex_t   mops; // Mutex OPS
  wusynops_condvar_t cops; // Cv OPS
} wusynops_t;

extern wusynops_thread_t  *pthread_throps;
extern wusynops_mutex_t   *pthread_lockops;
extern wusynops_condvar_t *pthread_cvops;
extern wusynops_t         *pthread_ops;

#ifdef __cplusplus
}
#endif
#endif /* WULIB_SYN_H */
