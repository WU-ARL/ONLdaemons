/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/period.c,v $
 * $Author: fredk $
 * $Date: 2006/11/15 19:48:59 $
 * $Revision: 1.11 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington University in St. Louis
 * */

#include <wulib/kern.h>

#include <sys/time.h>
#include <unistd.h>

#include <wulib/wulog.h>
#include <wulib/period.h>
#include <wulib/timer.h>

static pstats_t pstats;

void  wup_init(long tout, pstats_t *pstats)
{
  if (pstats == NULL) return;
  /* tout is the number of milliseconds for the period
   * convert to usecs and don't overrun */
  pstats->period       = (int64_t)tout * 1000;
  pstats->actperiod    = 0;
  pstats->initial      = 0;
  pstats->start        = 0;
  pstats->finish       = 0;
  pstats->tot_run      = 0;
  pstats->tot_sleep    = 0;
  pstats->tot_actsleep = 0;
  pstats->num_run      = 0;
  pstats->exceeded     = 0;
  return;
}

void init_Pstats(long T)
{
  wup_init(T, &pstats);
}

void wup_start(pstats_t *pstats)
{
  if (pstats == NULL) return;
  if (pstats->period != 0) {
    pstats->initial = gettod64();
    pstats->start   = pstats->initial;
  }
  return;
}

void start_Pclock(void)
{
  wup_start(&pstats);
  return;
}

void wup_yield(pstats_t *pstats)
{
  int64_t start, sleep, stop, run;
  struct timeval timeout;

  if (pstats == NULL) return;
  if (pstats->period == 0)
    return;

  stop              = gettod64();
  run               = stop - pstats->start;
  pstats->num_run++;
  pstats->tot_run   += run;
  sleep             = pstats->period - run;
  pstats->tot_sleep += sleep;

  if (sleep <= 0) {
    pstats->exceeded++;
  } else {
    /* OK, now go to sleep ... fist build the timeval struct
     * sleep is in nanoseconds, so convert to seconds and usecs 
     */
    usec2tval(sleep, &timeout);

    /* XXX Should verify we wake for the right reasons */
    select(0, 0, 0, 0, &timeout);
  }

  start                 = gettod64();
  pstats->actperiod    += start - pstats->start;
  pstats->start         = start;
  pstats->tot_actsleep += pstats->start - stop;
}

void Pyield(void)
{
  wup_yield(&pstats);
  return;
}

void wup_finish(pstats_t *pstats)
{
  if (pstats == NULL) return;
  if (pstats->period != 0)
    pstats->finish = gettod64();
}
void Pfinish(void)
{
  wup_finish(&pstats);
  return;
}

void wup_report(pstats_t *pstats, FILE *fp)
{
  if (pstats == NULL) return;
  if (pstats->period == 0)
    return;

  if (fp == 0)
    fp = stdout;
  /* run 1 more time than yielded and slept same number */
  if (pstats->period == 0) {
    printf("\n\nNo period specified.\n");
    return;
  }
  if (pstats->num_run == 0) pstats->num_run = 1;
  fprintf(fp, "\n\nP Statistics for this thread: \
        \n\tPeriod = %lld (msec), \
        \n\tActual Period = %.2f (msec), \
        \n\tTotal Run time = %.2f (msec) \
        \n\tAverage Run time = %.2f (msec) \
        \n\tAverage Requested Sleep Time = %.2f (msec)\
        \n\tAverage Actual Sleep Time = %.2f (msec)\
        \n\tNumber of Times Scheduled = %u \
        \n\tNumber of Times Exceeded Period = %u\n",
         pstats->period/1000, 
         (float)(pstats->actperiod/pstats->num_run)/1000.0, 
         (float)pstats->tot_run/1000.0,
         (float)(pstats->tot_run/(pstats->num_run + 1.0))/1000.0, 
         (float)(pstats->tot_sleep/pstats->num_run)/1000.0,   
         (float)(pstats->tot_actsleep/pstats->num_run)/1000.0,
         (unsigned int)pstats->num_run,
         (unsigned int)pstats->exceeded);
}
void report_Pstats(FILE *fp)
{
  wup_report(&pstats, fp);
  return;
}
