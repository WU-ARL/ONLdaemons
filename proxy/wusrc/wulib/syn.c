/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/syn.c,v $
 * $Author: fredk $
 * $Date: 2005/02/07 19:24:09 $
 * $Revision: 1.6 $
 *
 * Author:       Fred Kuhns
 *               fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington University in St. Louis
 * */

/* Standard include files */
#include <wulib/syn.h>

int pthread_int_lock_init(void *lock, void *attr);
int pthread_int_lock_destroy(void *lock);
int pthread_int_lockit(void *lock);
int pthread_int_unlockit(void *lock);
int pthread_int_cv_init(void *cv, void *attr);
int pthread_int_cv_destroy(void *cv);
int pthread_int_cvwait(void * cv, void *lock);
int pthread_int_cvsig(void * cv);
int pthread_int_cvbcast(void *cv);
int pthread_int_self(void);

wusynops_t pthread_ops_int = {
  // tops, mops, cops
  {pthread_int_self},
  {pthread_int_lock_init, pthread_int_lock_destroy, pthread_int_lockit, pthread_int_unlockit}, 
  {pthread_int_cv_init, pthread_int_cv_destroy, pthread_int_cvwait, pthread_int_cvsig, pthread_int_cvbcast},
};
wusynops_t         *pthread_ops     = &pthread_ops_int;
wusynops_thread_t  *pthread_throps  = &pthread_ops_int.tops;
wusynops_mutex_t   *pthread_lockops = &pthread_ops_int.mops;
wusynops_condvar_t *pthread_cvops   = &pthread_ops_int.cops;

int pthread_int_lock_init(void *lock, void *attr)
{
  return pthread_mutex_init((pthread_mutex_t *)lock, (pthread_mutexattr_t *)attr);
}
int pthread_int_lock_destroy(void *lock)
{
  return pthread_mutex_destroy((pthread_mutex_t *)lock);
}
int pthread_int_lockit(void *lock)
{
  return pthread_mutex_lock((pthread_mutex_t *)lock); 
}
int pthread_int_unlockit(void *lock)
{
  return pthread_mutex_unlock((pthread_mutex_t *)lock);
}
int pthread_int_cv_init(void *cv, void *attr)
{
  return pthread_cond_init((pthread_cond_t *)cv, (pthread_condattr_t *)attr);
}
int pthread_int_cv_destroy(void *cv)
{
  return pthread_cond_destroy((pthread_cond_t *)cv);
}
int pthread_int_cvwait(void * cv, void *lock)
{
  return pthread_cond_wait((pthread_cond_t *)cv, (pthread_mutex_t *)lock);
}
int pthread_int_cvsig(void * cv)
{
  return pthread_cond_signal((pthread_cond_t *)cv);
}
int pthread_int_cvbcast(void *cv)
{
  return pthread_cond_broadcast((pthread_cond_t *)cv);
}

int pthread_int_self(void)
{
#if defined(__linux) || defined (__sun)
  return (int)pthread_self();
#else
  return -1;
#endif
}
