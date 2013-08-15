/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/barrier.c,v $
 * $Author: fredk $
 * $Date: 2005/10/04 02:42:22 $
 * $Revision: 1.6 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington University in St. Louis
 * */

/* Standard include files */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>

/* string manipulation */
#include <string.h>

/* UNIX specific includes */
#include <sys/time.h> /* gettimeofday */

#include <sys/types.h> /* POSIX fundamental types */
#include <sys/wait.h>  /* POSIX wait and waitpid */
#include <unistd.h>    /* more POSIX macros and functions: exec close, read ... */

#include <errno.h>

#include <wulib/syn.h>
#include <wulib/barrier.h>
#include <wulib/wulog.h>

int
init_barrier(int count, wubarrier_t *barrier, int syncType)
{
  if (syncType == WULIB_SYNC_TYPE_POSIX) {
    barrier->sops = pthread_ops;
    if ((barrier->lock = (void *)calloc(1, sizeof(pthread_mutex_t))) == NULL) {
      wulog(wulogLib, wulogError, "init_barrier: Unable to allocate lock memory\n");
      return -1;
    } else if ((barrier->cv   = (void *)calloc(1, sizeof(pthread_cond_t))) == NULL) {
      wulog(wulogLib, wulogError, "init_barrier: Unable to allocate cv memory\n");
      free(barrier->lock);
      return -1;
    }

  } else if (barrier->sops == NULL) {
    wulog(wulogLib, wulogError, "init_barrier: Must specify a valid sync type!\n");
    return -1;
  }

  if (barrier->sops->mops.lock_init(barrier->lock, NULL) < 0) {
    wulog(wulogLib, wulogError, "init_barrier: Error initializing lock\n");
    free(barrier->lock);
    free(barrier->cv);
    return -1;
  }

  if (barrier->sops->cops.cv_init(barrier->cv, NULL) < 0) {
    wulog(wulogLib, wulogError, "init_barrier: Error initializing CV\n");
    free(barrier->lock);
    free(barrier->cv);
    return -1;
  }

  barrier->nthreads = barrier->count = count;
  barrier->cycle = 0;
  return 0;
}

int
free_barrier(wubarrier_t *barrier)
{
  if (barrier->count != barrier->nthreads) {
    wulog(wulogLib, wulogError, "free_barrier: count != nthreads!\n");
    return -1;
  }

  if (barrier->lock) {
    barrier->sops->mops.lock_destroy(barrier->lock);
    free(barrier->lock);
    barrier->lock = NULL;
  }

  if (barrier->cv) {
    barrier->sops->cops.cv_destroy(barrier->cv);
    free(barrier->cv);
    barrier->cv = NULL;
  }

  return 0;
}

int
barrier_wait (wubarrier_t *barrier)
{
  int mycycl;

  if (barrier->lock == NULL || barrier->cv == NULL) {
    wulog(wulogLib, wulogError, "Barrier Wait: Invalid barrier argument!\n");
    return -1;
  }

  if (barrier->sops->mops.lockit(barrier->lock) != 0) {
    wulog(wulogLib, wulogError, "Barrier Wait: Error locking barrier mutex\n");
    return -1;
  }

  if (--barrier->count == 0) {
#if defined(WUDEBUG)
    wulog(wulogLib, wulogInfo, "Barrier Wait: Last thread %d, barrier cycle %d, count %d\n",
           barrier->sops->tops.mytid(), barrier->cycle, barrier->count);
#endif

    barrier->cycle++;
    barrier->count = barrier->nthreads;

    if (barrier->sops->cops.cvbcast(barrier->cv) != 0) {
      wulog(wulogLib, wulogError, "Barrier Wait: Error broadcasting to barrier\n");
      barrier->sops->mops.unlockit(barrier->lock);
      return -1;
    }
  } else {
    mycycl = barrier->cycle;
    while (mycycl == barrier->cycle && barrier->count != 0) {
#if defined(WUDEBUG)
      wulog(wulogLib, wulogInfo, "Barrier Wait: Thread %d, mycycle %d, barrier cycle %d, count %d\n",
             barrier->sops->tops.mytid(), mycycl, barrier->cycle, barrier->count);
#endif
      if (barrier->sops->cops.cvwait(barrier->cv, barrier->lock) != 0) {
        wulog(wulogLib, wulogError, "Barrier Wait: Error calling cv_wait on barrier\n");
        barrier->sops->mops.unlockit(barrier->lock);
        return -1;
      }
    }
  }

  if (barrier->sops->mops.unlockit(barrier->lock) != 0) {
    wulog(wulogLib, wulogError, "Error unlocking barrier mutex after broadcast error\n");
    return -1;
  }
#if defined(WUDEBUG)
      wulog(wulogLib, wulogInfo, "Barrier Wait: Thread %d leaving barrier\n",
             barrier->sops->tops.mytid());
#endif
  return 0;
}
