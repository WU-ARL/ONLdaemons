/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/timer.h,v $
 * $Author: fredk $
 * $Date: 2008/02/22 16:38:51 $
 * $Revision: 1.5 $
 * $Name:  $
 *
 * File:   timer.h
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  02/16/07 10:33:24 CST
 * 
 * Description:
 */

#ifndef _WUPP_TIMER_H
#define _WUPP_TIMER_H

#include <sys/time.h>

namespace wupp {
  static inline bool nonzero(const struct timeval &);

  static inline unsigned int tval2msec(const struct timeval &);
  static inline int64_t      tval2msec64(const struct timeval &);
  static inline struct timeval &msec2tval(unsigned int, struct timeval &);
  static inline struct timeval &msec2tval(int64_t, struct timeval &);

  static inline unsigned int tval2usec(const struct timeval &);
  static inline int64_t      tval2usec64(const struct timeval &);
  static inline struct timeval &usec2tval(unsigned int, struct timeval &);
  static inline struct timeval &usec2tval(int64_t, struct timeval &);

  static inline struct timeval &gettod(struct timeval &);
  static inline struct timeval &gettod(void);
  static inline int64_t gettod64(void);

  static inline bool nonzero(const struct timeval &tval) { return (tval.tv_sec || tval.tv_usec); }
  /* ********** tval -- msec ********** */
  static inline unsigned int tval2msec(const struct timeval &tval)
  {
    return (unsigned int)tval2msec64(tval);
  }
  static inline int64_t tval2msec64(const struct timeval &tval)
  {
    return (((int64_t)tval.tv_sec * 1000UL) + ((int64_t)tval.tv_usec / 1000UL));
  }
  static inline struct timeval &msec2tval(unsigned int msec, struct timeval &tval)
  {
    tval.tv_sec  = (long)(msec / 1000UL);
    tval.tv_usec = (long)((msec * 1000UL) % 1000000UL);
    return tval;
  }
  static inline struct timeval &msec2tval(int64_t msec, struct timeval &tval)
  {
    tval.tv_sec  = (long)(msec / 1000UL);
    tval.tv_usec = (long)((msec * 1000UL) % 1000000UL);
    return tval;
  }
  /* ********** tval2 -- sec ********** */
  static inline unsigned int tval2usec(const struct timeval &tval)
  {
    return (unsigned int)tval2usec64(tval);
  }
  static inline int64_t tval2usec64(const struct timeval &tval)
  {
    return (((int64_t)tval.tv_sec * (int64_t)1000000UL) + (int64_t)tval.tv_usec);
  }
  static inline struct timeval &usec2tval(unsigned int usec, struct timeval &tval)
  {
    tval.tv_sec  = (long)(usec / 1000000UL);
    tval.tv_usec = (long)(usec % 1000000UL);
    return tval;
  }
  static inline struct timeval &usec2tval(int64_t usec, struct timeval &tval)
  {
    tval.tv_sec  = (long)(usec / 1000000UL);
    tval.tv_usec = (long)(usec % 1000000UL);
    return tval;
  }
  /* ********** XXX ********** */

  static inline struct timeval &gettod(struct timeval &tod)
  {
    if (gettimeofday(&tod, 0) == -1)
      throw wupp::errorBase(wupp::sysError, "gettimeofday failed", errno);
    return tod;
  }
  static inline struct timeval &gettod(void)
  {
    struct timeval tod;
    return gettod(tod);
  }
  static inline int64_t gettod64(void)
  {
    struct timeval tval;
    gettod(tval);
    return tval2usec64(tval);
  }
}; // namespace wupp

static inline struct timeval operator-(const struct timeval &lhs, const struct timeval &rhs);
static inline struct timeval operator+(const struct timeval &lhs, const struct timeval &rhs);

static inline bool operator==(const struct timeval &, const struct timeval &);
static inline bool operator<(const struct timeval &, const struct timeval &);
static inline bool operator<=(const struct timeval &, const struct timeval &);
static inline bool operator>(const struct timeval &, const struct timeval &);
static inline bool operator>=(const struct timeval &, const struct timeval &);
static inline bool operator!(const struct timeval &);

static inline std::ostream &operator<<(std::ostream &os, const struct timeval &);


static inline struct timeval operator-(const struct timeval &lhs, const struct timeval &rhs)
{
  struct timeval diff;

  // assumptions:
  //   0 <= tv_usec < 1,000,000
  //   So this means that lhs.tv_usec and rhs.tv_usec are both non-negative
  //   and less than 1,000000.
  // But it is posible for the seconds to be < 0.
  diff.tv_usec = lhs.tv_usec - rhs.tv_usec;
  diff.tv_sec  = lhs.tv_sec  - rhs.tv_sec;

  if (diff.tv_usec < 0) {
    diff.tv_sec  -= 1;
    diff.tv_usec += 1000000;
  }

  return diff;
}

static inline struct timeval operator+(const struct timeval &lhs, const struct timeval &rhs)
{
  struct timeval sum;

  sum.tv_sec  = lhs.tv_sec  + rhs.tv_sec;
  sum.tv_usec = lhs.tv_usec + rhs.tv_usec;

  if (sum.tv_usec >= 1000000) {
    sum.tv_sec  += 1;
    sum.tv_usec -= 1000000;
  }

  return sum;
}

static inline bool operator==(const struct timeval &lhs, const struct timeval &rhs)
  { return ((lhs.tv_sec == rhs.tv_sec) && (lhs.tv_usec == rhs.tv_usec)); }
static inline bool operator<=(const struct timeval &lhs, const struct timeval &rhs)
  { return !(rhs < lhs); }
static inline bool operator<(const struct timeval &lhs, const struct timeval &rhs)
  { return ((lhs.tv_sec < rhs.tv_sec) || (lhs.tv_sec == rhs.tv_sec && lhs.tv_usec < rhs.tv_usec)); }
static inline bool operator>(const struct timeval &lhs, const struct timeval &rhs)
  { return rhs < lhs; }
static inline bool operator>=(const struct timeval &lhs, const struct timeval &rhs)
  { return !(lhs < rhs); }

static inline bool operator!(const struct timeval &tval)
{
  return ! wupp::nonzero(tval);
}

static inline std::ostream &operator<<(std::ostream &os, const struct timeval &tval)
{
  os << "{" << tval.tv_sec << ", " << tval.tv_usec << "}";
  return os;
}

#endif
