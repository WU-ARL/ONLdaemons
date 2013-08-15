/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/period.h,v $
 * $Author: fredk $
 * $Date: 2007/01/14 17:02:06 $
 * $Revision: 1.6 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington University in St. Louis
 * */

#ifndef _WU_PERIOD_H
#define _WU_PERIOD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PStats {
  int64_t  period;
  int64_t  actperiod;
  int64_t  initial;
  int64_t  start;
  int64_t  finish;
  int64_t  tot_run;
  int64_t  tot_sleep;
  int64_t  tot_actsleep;
  unsigned int num_run;
  unsigned int exceeded;
} pstats_t;

void wup_init(long tout, pstats_t *pstats);
void wup_start(pstats_t *pstats);
void wup_yield(pstats_t *pstats);
void wup_finish(pstats_t *pstats);
void wup_report(pstats_t *pstats, FILE *fp);

void init_Pstats(long T);
void start_Pclock(void);
void Pyield(void);
void Pfinish(void);
void report_Pstats(FILE *fp);

#ifdef __cplusplus
}
#endif
#endif
