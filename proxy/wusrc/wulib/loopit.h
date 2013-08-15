/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/loopit.h,v $
 * $Author: fredk $
 * $Date: 2005/06/16 22:23:24 $
 * $Revision: 1.10 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington University in St. Louis
 * */

#ifndef _WU_LOOPIT_H
#define _WU_LOOPIT_H

#define DefaultLoopCount       10000

#include <wulib/timer.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t runloopsfn(uint32_t loops, void (*fn)(void));
static inline uint32_t calloops64(int64_t nsecs, uint32_t loops, void (*fn)(void));
static inline uint32_t calloops32(uint32_t usecs, uint32_t loops, void (*fn)(void));

/*
 * Input: reqtime is 64 bit integer representing time in nsecs
 *        loops is the number of loops used last time (or returned by
 *        calloops)
 * Output: Value of loops to use in next call.
 * */
static inline uint32_t
calloops64(int64_t nsecs, uint32_t loops, void (*fn)(void))
{
  int64_t start, diff;

  if (nsecs == 0)
    return 0;

  start = getnsecs();
  runloopsfn(loops, fn);
  diff = getnsecs() - start;
  /*
   * loops / (stop - start) == Loops/nsec,
   * 
   * reqtime * [loops / (stop - start)] = # loops to iterate
   * for requested time period.
   * */
  return (nsecs * loops) / diff;
}

static inline uint32_t
runloopsfn(uint32_t loops, void (*fn)(void))
{
  uint32_t i;

  for (i = 0; i < loops; i++) {
    if (fn)
      fn();
  }

  return i;
}

/* 32 bit versions */
static uint32_t
calloops32(uint32_t usecs, uint32_t loops, void (*fn)(void))
{
  unsigned long start, diff;

  if (usecs == 0)
    return 0;

  start = getusecs32();
  runloopsfn(loops, fn);
  diff = getusecs32() - start;
  return (usecs * loops) / diff;
}

/* Legacy code */

static inline uint32_t runloops(int64_t reqtime, uint32_t loops);
static inline uint32_t calloops(int64_t reqtime);

/*
 * Input: reqtime is 64 bit integer representing time in nsecs
 *        loops is the number of loops used last time (or returned by
 *        calloops)
 * Output: Value of loops to use in next call.
 * */
static inline uint32_t
runloops(int64_t reqtime, uint32_t loops)
{
  int64_t start, stop;
  int i;
  if (reqtime == 0)
    return(0);

  start = getnsecs();
  for(i=0;(unsigned long)i < loops;i++) ;
  stop = getnsecs();
  /*
   * loops / (stop - start) == Loops/nsec,
   * 
   * reqtime * [loops / (stop - start)] = # loops to iterate
   * for requested time period.
   * */
  loops = (reqtime * loops) / (stop - start);
  return(loops);
}

/* 
 * Input: reqtime (nsecs) 
 * Output: number of loops, pass to runloops
 * */
static inline uint32_t
calloops(int64_t reqtime)
{
  uint32_t loops;

  if (reqtime == 0)
    return 0;

  printf("\nRunning %i test iterations to determine loops/nsec\n", 
         DefaultLoopCount);

  loops = runloops (reqtime, DefaultLoopCount);

  if (loops == 0) {
    printf("calloops: Error calculating loop time!!\n");
    return 0;
  }

  printf("calloops: loop for %u iterations to get required computer time = %lld nsecs\n\n",
          (unsigned int)loops, reqtime);

  return loops;
}

#ifdef __cplusplus
}
#endif
#endif
