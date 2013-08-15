/* 
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/kern.h,v $
 * $Author: fredk $
 * $Date: 2007/03/06 21:30:14 $
 * $Revision: 1.3 $
 * $Name:  $
 *
 * File:         
 * Author:       Fred Kuhns <fredk@arl.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 * Description:
 *
 *
 */

#ifndef _WU_KERN_H
#define _WU_KERN_H

#ifdef __KERNEL__
# include <linux/kernel.h>  /* Needed for KERN_ALERT, printk */
# include <linux/init.h>    /* Needed for the macros */
# include <linux/types.h>   /* Needed for the macros */
# include <linux/config.h>
# include <linux/time.h>    /* struct timeval */
# include <linux/spinlock.h>
# include <linux/string.h>
# define printf printk
# define PRIx8 "x"
# define PRIx16 "x"
# define PRIx32 "x"
# define PRIx64 "llx"
# define PRId32 "d"
#else
# include <stdio.h>
# include <stdlib.h>
# include <inttypes.h> // also includes #include <stdint.h>
# include <string.h>
# include <wulib/wulog.h>
#endif

#endif /* _WU_KERN_H */
