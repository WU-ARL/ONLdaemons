/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/stats.h,v $
 * $Author: fredk $
 * $Date: 2007/06/06 21:06:39 $
 * $Revision: 1.10 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington UNiversity in St. Louis
 * */

#ifndef _WU_STATS_H
#define _WU_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#define STATS_FREQ_EXT      "freq"
#define STATS_SUM_EXT       "sum"
#define STATS_SAMPLE_EXT    "samp"
#define STATS_CDF_EXT       "cdf"

/* I include this here since we need anyway if this file is included */
#include <sys/time.h>
#include <wulib/timer.h>

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

#define STATS_NSEC_SCALE                  1
#define STATS_NSEC_TXT               "nsec"
#define STATS_USEC_SCALE               1000
#define STATS_USEC_TXT               "usec"
#define STATS_MSEC_SCALE            1000000
#define STATS_MSEC_TXT               "msec"
#define STATS_SEC_SCALE          1000000000
#define STATS_SEC_TXT                 "sec"

#define STATS_USEC2NSEC                1000
#define STATS_MSEC2USEC                1000
#define STATS_SEC2MSEC                 1000
#define STATS_SEC2USEC         (STATS_SEC2MSEC * 1000)
#define STATS_SEC2NSEC         (STATS_SEC2USEC * 1000)

typedef struct {
  int64_t *samples; /* An array of all collected samples. */
  int64_t *tstamps;
  int nsamples;     /* Maximum samples that will be collected */
  int indx;         /* current next index into samples array */
  int64_t ohead;    /* test overhead, subtracted from each sample */
  /* */
  int64_t sum;      /* the running sum */
  double mean;      /* mean calculated at end of run */
  double sdev;      /* standard deviation calculated at end of run */
  double serr;      /* standard error calculated at end of run */
  /* */
  int window;        /* size of window (aka bucket) in nsecs */
  /* */
  int64_t max_value; /* maximum value seen */
  int  max_index; /* index of maximum value in samples */
  int64_t min_value; /* minimum value seen */
  int  min_index; /* index of minimum value in samples */
  /* */
  int *Dist;         /* Array containing number of samples falling within the
                      * given bucket size. The array index corresponds to a
                      * particular bucket (bucket = index * window)
                      */
  double *cdf;
  int ndist;
  int ncdf;
  int scale;       /* MSEC, NSEC, USEC or SEC */

  char *datafile;  /* datafile - file prefix all data is written to */
} stats_t;

stats_t *init_mstats(int nsamples, int window, const char *fname, int scale);
void release_stats(stats_t *stats);
void print_stats(stats_t *stats);
void do_figuring(stats_t *stat);
stats_t *reuse_stats (stats_t *stats);

static inline int add_sample_impl(stats_t *stat, int64_t smpl, int64_t tst);
static inline int add_sample(stats_t *stat, int64_t s);
static inline int add_sample2(stats_t *stat, int64_t smpl, int64_t tst);
static inline void set_ohead(stats_t *stats, int64_t ohead);
static inline int wus_init_tstamps(stats_t *stat);

static inline int wus_init_tstamps(stats_t *stat)
{
  if (stat == NULL) {
    fprintf(stderr, "wus_init_tstamps: Null stats object\n");
    return -1;
  }

  if (stat->nsamples <= 0) {
    fprintf(stderr, "wus_init_tstamps: nsamples <= 0\n");
    return -1;
  }

  if ((stat->tstamps = (int64_t *)calloc(stat->nsamples, sizeof(int64_t))) == NULL) {
    fprintf(stderr, "wus_init_tstamps: Error allocating memory\n");
    return -1;
  }

  return 0;
}

static inline int add_sample_impl(stats_t *stat, int64_t smpl, int64_t tst)
{
  if (stat == NULL)
    return 0;

  smpl = smpl - stat->ohead;
  /* max and min values */
  if (stat->indx == stat->nsamples) {
    return -1;
  }

  if (stat->max_value < smpl) {
    stat->max_value = smpl;
    stat->max_index = stat->indx;
  }
  if (stat->min_value > smpl) {
    stat->min_value = smpl;
    stat->min_index = stat->indx;
  }

  stat->samples[stat->indx] = smpl;
  if (stat->tstamps)
    stat->tstamps[stat->indx] = tst;

  stat->indx += 1;

  stat->sum += smpl;

  return 0;
}

static inline int add_sample(stats_t *stat, int64_t smpl)
  { return add_sample_impl(stat, smpl, 0); }
static inline int add_sample2(stats_t *stat, int64_t smpl, int64_t tst)
  { return add_sample_impl(stat, smpl, tst); }

static inline void set_ohead(stats_t *stats, int64_t ohead)
{
  stats->ohead = ohead;
  return;
}

#ifdef __cplusplus
}
#endif
#endif
