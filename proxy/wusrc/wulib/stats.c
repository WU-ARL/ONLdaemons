/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/stats.c,v $
 * $Author: fredk $
 * $Date: 2007/06/06 21:06:38 $
 * $Revision: 1.12 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington UNiversity in St. Louis
 * */
/* Standard include files */
#include <wulib/kern.h>

#include <math.h>
#include <sys/time.h>

#include <wulib/wulog.h>
#include <wulib/stats.h>
#include <wulib/timer.h>

static FILE *open_fp(char *name, char *ext);

/* Returns File pointer on success or NULL on error */
static FILE *
open_fp(char *fname, char *ext)
{
  char filenm[FILENAME_MAX];
  FILE *fp;

  if (fname == NULL)
    return NULL;

  if (ext)
    sprintf(filenm, "%s.%s", fname, ext);
  else 
    sprintf(filenm, "%s", fname);

  if ((fp = fopen(filenm, "w")) == NULL) {
    wulog(wulogStat, wulogError, "open_fp: Unable to open dist file %s!\n", filenm);
    return (FILE *)NULL;
  }

  return fp;
}

/* Hold Measurement Parameters and Stats */
void
print_stats (stats_t *stats)
{
  int64_t tmp;
  int i;
  FILE *sumfp=0, *cdffp=0, *freqfp=0, *sampfp=0;
  char *units;

  if (stats == NULL)
    return;

  if (stats->scale == STATS_NSEC_SCALE)
    units = STATS_NSEC_TXT;
  else if (stats->scale == STATS_USEC_SCALE)
    units = STATS_USEC_TXT;
  else if (stats->scale == STATS_MSEC_SCALE)
    units = STATS_MSEC_TXT;
  else if (stats->scale == STATS_SEC_SCALE)
    units = STATS_SEC_TXT;
  else
    units = NULL;

  /* sumfp -- summary data, 
   * freqfp distribution data (for histogram), 
   * sampfp sample data 
   * */

  printf("\nMeasurement Results for %i samples, units are %s: \
        \n\tSample Mean = %.2f, Measurement Overhead = %lld \
        \n\tMax Value = %lld, Max sample number = %d, \
        \n\tMin Value = %lld, Min sample number = %d, \
        \n\tStandard Deviation = %.2f, \
        \n\tStandard Error = %.2f, \
        \n\tStdDev as a percent of Mean (StdDev/Mean * 100) = %.2f%%\n\n",
        stats->indx, (units ? units : "Unknown"),
        stats->mean, stats->ohead,
        stats->max_value, stats->max_index,
        stats->min_value, stats->min_index,
        stats->sdev,
        stats->serr,
        (stats->sdev/stats->mean)*100);

  if (stats->datafile) {
    if ((freqfp = open_fp(stats->datafile, STATS_FREQ_EXT))  == NULL ||
        (sumfp  = open_fp(stats->datafile, STATS_SUM_EXT))   == NULL ||
        (cdffp  = open_fp(stats->datafile, STATS_CDF_EXT))   == NULL ||
        (sampfp = open_fp(stats->datafile, STATS_SAMPLE_EXT)) == NULL) {
      free (stats->datafile);
      if (freqfp) fclose (freqfp);
      if (sumfp)  fclose (sumfp);
      if (cdffp)  fclose (cdffp);
      if (sampfp) fclose (sampfp);
      return;
    }

    tmp = stats->min_value + stats->window/2;
    printf("Ndist = %d\n", stats->ndist);
    for (i = 0 ; i < stats->ndist ; i++) {
      fprintf(freqfp, "%lld\t%d\n", tmp, stats->Dist[i]);
      tmp += stats->window;
    }

    tmp = stats->min_value - 1;
    for (i = 0; i < stats->ncdf ; i++) {
      fprintf(cdffp, "%lld\t%.4f\n", tmp, stats->cdf[i]);
      tmp += stats->window;
    }

    if (stats->tstamps) {
      for (i = 0; i < stats->indx; i++)
        fprintf(sampfp, "%lld\t%lld\n",
                stats->tstamps[i], stats->samples[i]);
    } else {
      for (i = 0; i < stats->indx; i++)
        fprintf(sampfp, "%lld\n", stats->samples[i]);
    }

    fprintf(sumfp, "Latency Measurement Record:");

    fprintf(sumfp,"\nMeasurement Results for %i samples (%s): \
          \n\tSample Mean = %.2f, Measurement Overhead = %lld \
          \n\tMax Value = %lld, Max sample number = %d, \
          \n\tMin Value = %lld, Min sample number = %d, \
          \n\tStandard Deviation = %.2f, \
          \n\tStandard Error = %.2f, \
          \n\tStdDev as a percent of Mean (StdDev/Mean * 100) = %.2f%%\n\n",
          stats->indx, (units ? units : "Unknown"),
          stats->mean, stats->ohead,
          stats->max_value, stats->max_index,
          stats->min_value, stats->min_index,
          stats->sdev,
          stats->serr,
          (stats->sdev/stats->mean)*100);
    fclose (freqfp);
    fclose (sumfp);
    fclose (sampfp);
  }
  return;
}


stats_t *
reuse_stats (stats_t *stats)
{
  if (stats == NULL)
    return NULL;

  if (stats->Dist) {
    free(stats->Dist);
    stats->Dist = NULL;
    stats->ndist = 0;
  }

  if (stats->cdf) {
    free(stats->cdf);
    stats->cdf = NULL;
    stats->ncdf = 0;
  }

  stats->indx = 0;
  // stats->ohead = 0;
        /* */
  stats->sum  = 0;
  stats->mean = 0.0;
  stats->sdev = 0.0;
  stats->serr = 0.0;
        /* */
  // stats->window = 0;
        /* */
  stats->max_value = 0;
  stats->max_index = 0;
  stats->min_value = 0;
  stats->min_index = 0;
        /* */
        /* */
  // stats->scale = 0;
  memset((void *)stats, 0, sizeof(stats_t));

  return stats;
}

void
release_stats (stats_t *stats)
{
  if (stats == NULL)
  return;

  /* this free memory */
  reuse_stats (stats);

  if (stats->samples) {
    free(stats->samples);
    stats->samples = NULL;
  }

  if (stats->tstamps) {
    free(stats->tstamps);
    stats->tstamps = NULL;
  }

  if (stats->datafile) {
    free(stats->datafile);
    stats->datafile = NULL;
  }

  free(stats);

  return;
}

/*
 * nsamples - the maximum number of samples that will be collected.
 * window   - the bin size in usecs for histogram data
 * fname    - file prefix used for output data. If null then data is
 *            not written to a file.
 * scale    - 0, STATS_NSEC_SCALE, STATS_USEC_SCALE, STATS_MSEC_SCALE,
 *            STATS_SEC_SCALE
 */
stats_t *
init_mstats (int nsamples, int window, const char *fname, int scale)
{
  stats_t *stats;

#ifdef __i386__
  if (cpuspeed == 0) {
    if (init_timers () < 0) {
      wulog(wulogStat, wulogError, "init_mstats: Unable to initialize timers\n");
      return NULL;
    }
  }
#endif


  if (scale == 0) {
    scale = STATS_NSEC_SCALE;
  } else if (scale != STATS_NSEC_SCALE &&
             scale != STATS_USEC_SCALE &&
             scale != STATS_MSEC_SCALE &&
             scale != STATS_SEC_SCALE) {
    wulog( wulogStat, wulogError, "init_mstats: Invalid scale value (%d)\n", scale);
    return NULL;
  }

  /* First make sure we can allocate a stats struct */
  stats = (stats_t *)malloc(sizeof(stats_t));
  if (stats == NULL) {
    wulog(wulogStat, wulogError, "init_mstats: Unable to allocate memory for stats struct\n");
    return NULL;
  }
  memset((void *)stats, 0, sizeof(stats_t));

  /* Do they want the results written to a file? */
  if (fname != NULL) {
    stats->datafile = (char *)calloc((strlen(fname) + 1), sizeof(char));
    if (stats->datafile == NULL) {
      wulog(wulogStat, wulogError, "init_mstats: Unable to allocate memory file file name\n");
      return NULL;
    }
    strcpy(stats->datafile, fname);
  } else
    stats->datafile = NULL;

  stats->samples = (int64_t *)calloc(nsamples, sizeof(int64_t));
  if (stats->samples == NULL) {
    wulog(wulogStat, wulogError, "init_mstats: Unable to allocate memory for sample array\n");
    if (stats->datafile)
      free(stats->datafile);
    free(stats);
    return NULL;
  }

  /*
   * Note, calloc will zero fill the allocated memory so no need to deal with
   * fields that have default values of zero ... but I do it anyway.
   * */
  stats->nsamples = nsamples;
  stats->indx = 0;
  stats->ohead = 0;
  /* */
  stats->sum  = 0;
  stats->mean = 0.0;
  stats->sdev = 0.0;
  stats->serr = 0.0;
        /* */
  stats->window = window;
        /* */
  stats->max_value = INT64_MIN;
  stats->max_index = 0;
  stats->min_value = INT64_MAX;
  stats->min_index = 0;
        /* */
  stats->Dist = NULL;
  stats->ndist = 0;
  stats->ncdf = 0;
  stats->cdf = NULL;
        /* */
  stats->scale = scale;

  return stats;
}

void
do_figuring (stats_t *stats)
{
  int64_t window = 0;
  int d, i;

  if (stats == NULL || stats->indx <= 0)
    return;

  /* we use index in case there really aren't nsamples samples */
  stats->mean = ((double)stats->sum)/((double)stats->indx);

  if (stats->window) {
    window  = stats->window;
    /* calculate number of buckets:
     *  window = w = size of bin (range of values)
     *
     *  bucket number for sample i (Si) = bi.
     *  w = window size in "units"
     *    if Si = min, bi = 0 (bucket number 0)
     *    if Si = max, bi = n-1, (bucket number n-1, there are n buckets)
     *
     *    Let S = S' + min, then S' = S - min
     *
     *    b = 0,   for     0*w <= S' < 1*w or     0 <= S'/w < 1
     *    b = 1,   for     1*w <= S' < 2*w or     1 <= S'/w < 2
     *    b = 2,   for     2*w <= S' < 3*w or     2 <= S'/w < 3
     *         ...            ...
     *    b = n-1, for (n-1)*w <= S' < n*w or (n-1) <= S'/w < n
     *
     *    But (n-1)w <= (max - min) < nw
     *
     *    This is true for n = floor((max - min)/w) + 1;
     *    let ndist = n, see below
     * */
    stats->ndist = ((int)(stats->max_value - stats->min_value) / window) + 1;
    stats->ncdf  = stats->ndist + 1;

    stats->Dist = (int *)calloc(stats->ndist, sizeof(int));
    stats->cdf  = (double *)calloc(stats->ncdf, sizeof(double));

    if (stats->Dist == NULL || stats->cdf == NULL) {
      wulog(wulogStat, wulogError, "do_figuring: Unable to allocate Dist/CDF array\n");
      if (stats->Dist) free(stats->Dist);
      if (stats->cdf)  free(stats->cdf);
      stats->Dist = NULL;
      stats->cdf = NULL;
      window = 0;
    }

  }

  /* Standard Deviation =
   * SQRT(SUM((sample - mean)**2))/(number_samples - 1)
   * here we get the SUM part.
   *
   * Standard Error =
   *  Standard_Deviation/SQRT(number_samples)
   */
   for (i = 0; i < stats->indx ; i++ ) {
    stats->sdev += ((double)stats->samples[i] - stats->mean) *
                   ((double)stats->samples[i] - stats->mean);

    /* get index into distribution array */
    if (window) {
      d = (int)((stats->samples[i] - stats->min_value)/window);

      if (d < 0 || d > stats->ndist) {
        wulog(wulogStat, wulogError,
              "do_figuring: Error indexing into dist array %d (%d)\n\n", d, stats->ndist);
        window = 0;
        free(stats->Dist);
        free(stats->cdf);
        stats->Dist = NULL;
        stats->cdf  = NULL;
        continue;
      }
      stats->Dist[d] += 1; /* increment bucket count for this sample */
    }
  } /* for all samples */
  if (window) {
    stats->cdf[0] = 0.0; // X <= min_value - 1
    for (i = 1; i < stats->ncdf; i++) {
      // P(X <= min_value + i*window - 1)
      stats->cdf[i] = stats->cdf[i-1] + stats->Dist[i-1];
    }
    for (i = 0; i < stats->ncdf; i++)
      stats->cdf[i] = stats->cdf[i] / (double)stats->indx;
  }

  if (stats->indx > 1)
    stats->sdev = (double) sqrt(stats->sdev/(double)(stats->indx - 1));
  else
    stats->sdev = (double)0;

  stats->serr = (double) stats->sdev/sqrt((double)stats->indx);

  return;
}
