/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/timer.c,v $
 * $Author: fredk $
 * $Date: 2008/02/11 16:41:04 $
 * $Revision: 1.11 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington UNiversity in St. Louis
 * */
/* Standard include files */
#include <wulib/kern.h>

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>
/* string manipulation */
#include <string.h>
#include <math.h>
#include <sys/time.h>
#endif

#include <wulib/wulog.h>
#include <wulib/timer.h>

#if defined(__i386__) || defined(__x86_64__)

int cpuspeed = 0; /* in MHz */

int
i386_init_timers(void) {
  uint32_t i;
  int64_t cntr2, cntr1, cycls, diff; // secs, usecs;
  struct timeval tv1, tv2;
  volatile int xx = 0;

#ifdef _KERNEL
  microtime(&tv1);
#else
  if (gettimeofday (&tv1, 0) < 0) {
    fprintf (stderr, "i386_init_timers: gettimeofday ERROR\n");
    return -1;
  }
#endif
  cntr1 = i386_clock64();

  /* run up the cpu_clock counter
   * How it works: assuming that the clock period is between 1 - 10 nsec,
   * there should be roughly 100 - 1000 ticks per usec.  Also, assuming the
   * real-time clock's frequency is around 1 MHz and we want +/- .1% error
   * then shoot for 1 msec of spinning.
   * */
  if (cntr1 == 0)
    i = 1;
  else
    i = 0;
  for (; i < 30000000; i++)
    xx = xx + i;

  if (xx == 0)
    cntr2 = 0;
  else
    cntr2 = i386_clock64();
#ifdef _KERNEL
  microtime(&tv2);
#else
  if (gettimeofday (&tv2, 0) < 0) {
    fprintf (stderr, "i386_init_timers: gettimeofday ERROR\n");
    return -1;
  }
#endif

  cycls = cntr2 - cntr1;
  // diff  = tval2usec(&tv2) - tval2usec(&tv1);
  {
    struct timeval tvDiff = {0, 0};
    if (toddiff(&tv2, &tv1, &tvDiff) == NULL) {
      fprintf(stderr, "i386_init_timers: Error Calculating difference\n");
      cpuspeed = 1;
      return -1;
    }
    diff = tval2usec(&tvDiff);
  }

  /* I'm not worried about rounding errors here, I'll chalk it up
   * to overhead in performing the measurement.
   * */
  if (diff == 0 || cycls == 0) {
    fprintf(stderr, "i386_init_timers: Error, diff = %ud and cycls = %ud\n",
        (unsigned int)diff, (unsigned int)cycls);
    cpuspeed = 1;
    return -1;
  }

  cycls = cycls/diff;
  cpuspeed = (int)cycls;

  wulog(wulogTime, wulogInfo, "i386_init_timers: Clock rate %ud MHz, time diff %lld usec, cycles %lld\n", 
        (unsigned int)cpuspeed, diff, cycls);

  return 0;
}

int64_t i386_clock64(void)
{
  uint32_t hi, lo;
  uint64_t v64;

  /* rdtsc: Loads the time-stamp counter value into %edx:%eax */
  asm volatile ("cpuid; rdtsc; movl %%edx, %0; movl %%eax, %1" /* Read the counter */
      : "=r" (hi), "=r" (lo)                  /* output */
      :                                       /* input (none) */
      : "%edx", "%eax");                      /* Let gcc know whch registers are used */

  v64 = ((uint64_t)hi << 32) + (uint64_t)lo;

  return (int64_t)v64;
}


#endif /* __i386__ */
