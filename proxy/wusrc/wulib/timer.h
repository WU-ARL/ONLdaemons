/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/timer.h,v $
 * $Author: fredk $
 * $Date: 2007/03/08 18:22:46 $
 * $Revision: 1.19 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington UNiversity in St. Louis
 * */

#ifndef _WUTIMER_H
#define _WUTIMER_H

/* I include this here since we need anyway if this file is included */
#if defined(linux) && defined(__KERNEL__)
#include <linux/timer.h>
#else
#include <sys/time.h>
#include <sys/param.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Hold Measurement Parameters and Stats
 * Note, an array is kept of all samples to simplify the 
 * calculations and to permit the prinitng out of all collected 
 * data into a file for plotting histoggrams
 *
 * Note: Time is assumed to be in nano-seconds. When a scalling
 * factor is used:
 *         scale   units
 *           1     nsec
 *        1000     usec
 *     1000000     msec
 *  1000000000      sec
 * */
#define WUT_MSEC                   1000l
#define WUT_USEC                1000000l
#define WUT_NSEC             1000000000l
#define WUT_MSEC2USEC              1000l
#define WUT_MSEC2NSEC           1000000l
#define WUT_USEC2NSEC              1000l

static inline int     init_timers(void);

/* some utility method for calculating the local tick size */
static inline uint32_t tick2msec(uint32_t ticks);
static inline uint32_t msec2ticks(uint32_t msecs);
static inline uint32_t tick2usec(uint32_t ticks);
static inline uint32_t usec2ticks(uint32_t usecs);

static inline uint32_t tick2msec(uint32_t ticks)  {return (WUT_MSEC*ticks)/HZ;}
static inline uint32_t msec2ticks(uint32_t msecs) {return (msecs*HZ)/WUT_MSEC;}
static inline uint32_t tick2usec(uint32_t ticks)  {return (WUT_USEC*ticks)/HZ;}
static inline uint32_t usec2ticks(uint32_t usecs) {return (usecs*HZ)/WUT_USEC;}

/* struct timeval ops */
static inline struct timeval *gettod (struct timeval *tv);
static inline struct timeval *toddiff(const struct timeval *tv1, const struct timeval *tv2, struct timeval *diff);
static inline struct timeval *todadd (const struct timeval *tv1, const struct timeval *tv2, struct timeval *sum);
static inline struct timeval *todadd1(const struct timeval *tv1, const struct timeval *tv2, struct timeval *sum);
static inline int              todlt (const struct timeval *tv1, const struct timeval *tv2);
static inline int              todeq (const struct timeval *tv1, const struct timeval *tv2);

/* 64 bit version are the standard */
static inline int64_t gettod64(void);
static inline int64_t getnsecs(void);
static inline int64_t getusecs(void);
static inline int64_t getmsecs(void);
static inline int64_t getcycles(void);
static inline int64_t cycl2nsec(int64_t);
static inline int64_t cycl2usec(int64_t);

/* Conversions */
static inline void    usec2tval(int64_t usec, struct timeval *tval);
static inline int64_t tval2usec(const struct timeval *tv);
static inline int64_t tval2nsec(const struct timeval *tv);

/* Also offer 32 bit versions ... Add as I need */
static inline uint32_t tval2usec32(const struct timeval *tv);
static inline uint32_t tval2msec32(const struct timeval *tv);
static inline uint32_t getusecs32(void);
static inline uint32_t getmsecs32(void);
static inline long tval2usecL(const struct timeval *tv);

static inline struct timeval * gettod(struct timeval *tv) {
  if (gettimeofday (tv, 0) < 0) {
    return NULL;
  }
  return tv;
}

static inline int todlt(const struct timeval *tv1, const struct timeval *tv2)
{
  if (!tv1 || !tv2)  return 0;
  return ((tv1->tv_sec < tv2->tv_sec) || (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec < tv2->tv_usec));
}

static inline int todeq(const struct timeval *tv1, const struct timeval *tv2)
{
  if (!tv1 || !tv2)  return 0;
  return (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec == tv2->tv_usec);
}

/* diff = tv2 - tv1 */
static inline struct timeval * toddiff(const struct timeval *tv2, const struct timeval *tv1, struct timeval *diff)
{
  if ((diff == NULL) || (tv2 == NULL && tv1 == NULL))
    return NULL;
  else if (tv2 == NULL) {
    diff->tv_sec  = -1 * tv1->tv_sec;
    diff->tv_usec = -1 * tv1->tv_usec;
  } else if (tv1 == NULL) {
    diff->tv_sec  = tv2->tv_sec;
    diff->tv_usec = tv2->tv_usec;
  } else if (tv2->tv_sec == tv1->tv_sec) {
    /* No wrapping */
    diff->tv_sec = 0;
    diff->tv_usec = tv2->tv_usec - tv1->tv_usec;
  } else {
    /* Wrapped >= one or more times. Since the final usec value is less than
     * the original we only increased time by tv1->tv_sec - tv2->tv_sec - 1
     * seconds.
     * */
    diff->tv_sec  = (tv2->tv_sec - tv1->tv_sec) - 1;
    diff->tv_usec = WUT_USEC - tv1->tv_usec + tv2->tv_usec;
    if (diff->tv_usec >= WUT_USEC) {
      diff->tv_sec++;
      diff->tv_usec -= WUT_USEC;
    }
  }
  if (diff->tv_sec < 0) {
    diff->tv_sec--;
    diff->tv_usec -= WUT_USEC;
  }
  return diff;
}

static inline struct timeval *todadd (const struct timeval *tv1, const struct timeval *tv2, struct timeval *sum) {
  if ((sum == NULL) || (tv1 == NULL && tv2 == NULL))
    return NULL;
  else if (tv1 == NULL || tv2 == NULL) {
    sum->tv_sec  = tv1 ? tv1->tv_sec  : tv2->tv_sec;
    sum->tv_usec = tv1 ? tv1->tv_usec : tv2->tv_usec;
  } else {
    sum->tv_sec  = tv1->tv_sec  + tv2->tv_sec;
    sum->tv_usec = tv1->tv_usec + tv2->tv_usec;
    if (sum->tv_usec >= WUT_USEC) {
      sum->tv_sec++;
      sum->tv_usec = sum->tv_usec - WUT_USEC;
    }
  }
  return sum;
}

static inline struct timeval *todadd1(const struct timeval *tv1, const struct timeval *tv2, struct timeval *sum)
{
  if (!tv1 || !tv2 || !sum)
    return NULL;

  sum->tv_sec  = tv1->tv_sec  + tv2->tv_sec;
  sum->tv_usec = tv1->tv_usec + tv2->tv_usec;
  if (sum->tv_usec >= WUT_USEC) {
    sum->tv_sec++;
    sum->tv_usec = sum->tv_usec - WUT_USEC;
  }

  return sum;
}

static inline int64_t getmsecs(void) {return (getusecs() / WUT_MSEC2USEC);}

static inline int64_t gettod64(void)
{
  struct timeval tv;
  if (gettimeofday (&tv, 0) < 0) {
    return -1;
  }
  return tval2usec(&tv);
}

static inline void msec2tval (int32_t msec, struct timeval *tval)
{
  tval->tv_sec  = (long)(msec / 1000);
  tval->tv_usec = (long)((msec * 1000) % 1000000);
  return;
}

static inline void usec2tval (int64_t usec, struct timeval *tval)
{
  tval->tv_sec  = (long)(usec / WUT_USEC);
  tval->tv_usec = (long)(usec % WUT_USEC);
  return;
}

static inline int64_t tval2usec (const struct timeval *tv)
{
  int64_t sec, usec;
  sec  = (int64_t)tv->tv_sec;
  usec = (int64_t)tv->tv_usec;
  return ((WUT_USEC * sec) + usec);
}

static inline int64_t tval2nsec (const struct timeval *tv)
{
  int64_t sec, usec;
  sec  = (int64_t)tv->tv_sec;
  usec = (int64_t)tv->tv_usec;
  return ((WUT_NSEC * sec) + (WUT_USEC2NSEC * usec));
}

#if defined(__i386__) || defined(__x86_64__) || defined(__i686__)
  int64_t i386_clock64(void);
  int i386_init_timers(void);
  extern int cpuspeed;

  static inline int64_t getnsecs (void)
  {
    return cycl2nsec(getcycles());
  }

  static inline int64_t getusecs (void)
  {
    return cycl2usec(getcycles());
  }

  static inline int init_timers(void)
  {
    return i386_init_timers();
  }

  static inline int64_t getcycles(void)
  {
    return i386_clock64();
  }

  static inline int64_t cycl2nsec(int64_t cycls)
  {
    // return (cpuspeed ? ((cycls * 1000)/cpuspeed) : 0);
    return (cpuspeed ? ((cycls*1000)/cpuspeed) : 0);
  }

  static inline int64_t cycl2usec(int64_t cycls)
  {
    return (cpuspeed ? (cycls / cpuspeed) : 0);
  }

#elif defined(sun)

  static inline int init_timers(void)
  {
    return 0;
  }

  static inline int64_t getusecs (void) {
    return (int64_t)(gethrtime () / WUT_USEC2NSEC);
  }

  static inline int64_t getnsecs (void) { return (int64_t)gethrtime(); }

  static inline int64_t getcycles(void)
  {
    return getnsecs();
  }
  static inline int64_t cycl2nsec(int64_t cycls)
  {
    return cycls;
  }
  static inline int64_t cycl2usec(int64_t cycls)
  {
    return cycls;
  }

#else

  static inline int init_timers(void)
  {
    return 0;
  }

  static inline int64_t getusecs(void)
  {
    return gettod64();
  }

  static inline int64_t getnsecs(void)
  {
    return (getusecs() * 1000);
  }

  static inline int64_t getcycles(void)
  {
    return getnsecs();
  }
  static inline int64_t cycl2nsec(int64_t cycls)
  {
    return cycls;
  }
  static inline int64_t cycl2usec(int64_t cycls)
  {
    return cycls;
  }
#endif

static inline uint32_t tval2usec32(const struct timeval *tv)
{
  uint32_t usec1, usec2;
  usec1 = (uint32_t)tv->tv_sec * WUT_USEC;
  usec2 = (uint32_t)tv->tv_usec;
  return (usec1 + usec2);
}

static inline uint32_t tval2msec32(const struct timeval *tv)
{
  uint32_t msec1, msec2;
  msec1 = (uint32_t)tv->tv_sec * WUT_MSEC;
  msec2 = (uint32_t)tv->tv_usec / WUT_MSEC2USEC;
  return (msec1 + msec2);
}

// signed version
static inline long tval2usecL(const struct timeval *tv)
{
  long sec, usec;
  sec  = (long)tv->tv_sec;
  usec = (long)tv->tv_usec;
  return ((WUT_USEC * sec) + usec);
}

static inline uint32_t getusecs32(void)
{
  struct timeval tv;
  if (gettimeofday (&tv, 0) < 0) {
    return 0;
  }
  return tval2usec32(&tv);
}

static inline uint32_t getmsecs32(void)
{
  struct timeval tv;
  if (gettimeofday (&tv, 0) < 0) {
    return 0;
  }
  return tval2msec32(&tv);
}

/* Some simple functions for keeping track of average values */

// the sample count needs to start at 1 and not 0 since
//   update(avg, S, 1) returns S not (S+avg)/2
static inline int64_t updateAvg(int64_t avg, int64_t sample, int n)
{
  // this is wrong but ultimately what it means is the first sample (n = 0)
  // will be ignored since both updateAvg(avg,S,0) and updateAvg(avg,S,1)
  // just return the sample value S.
  if (n == 0)
    return sample;
  return (avg + ((sample - avg)/n));
}

#ifdef __cplusplus
}
#endif
#endif /* _WUTIMER_H */
