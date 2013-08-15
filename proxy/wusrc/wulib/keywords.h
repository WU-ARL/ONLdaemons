/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/keywords.h,v $
 * $Author: fredk $
 * $Date: 2007/08/02 23:14:39 $
 * $Revision: 1.15 $
 * $Name:  $
 *
 * File:
 * Author:       Fred Kuhns
 * Organization  Applied Research Laboratory,
 *               Washington University in St. Louis
 * Description:·
 *
 *
 * */

#ifndef _WUKEYWORDS_H
#define _WUKEYWORDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wulib/wulog.h>

#define WULIB_MAXNAME      32
#define WULIB_MAXDESC     128

/* The idea is that a flag value of all ones (-1) will never be valid.
 * I'd actually prefer to raise an exception, I could pass in a pointer to the
 * flag but that is too cumbersome. 
 * */
#define WULIB_FLAG_INVALID    0xffffffff
#define WULIB_KEY_DEFSTR       "Unknown"

typedef uint32_t wuKeyFlag_t;

typedef struct KeyInfo_t {
  wuKeyFlag_t  flag;
  const char  *name;
  const char  *desc;
} KeyInfo_t;
#define WULIB_KEYINFO_NULL_ENTRY {WULIB_FLAG_INVALID, NULL, NULL}

#define wukey_str2indx  str2indx
#define wukey_str2flag  str2flag
#define wukey_flag2str  flag2str
#define wukey_flag2indx flag2indx
#define wukey_print_info print_info
static inline int         wukey_str2indx (const char *str, const KeyInfo_t *info);
static inline wuKeyFlag_t wukey_str2flag (const char *str, const KeyInfo_t *info);
static inline const char *wukey_flag2str_safe (wuKeyFlag_t flag, const KeyInfo_t *info);
static inline const char *wukey_flag2str (wuKeyFlag_t flag, const KeyInfo_t *info);
static inline int         wukey_flag2indx (wuKeyFlag_t flag, const KeyInfo_t *info);
static inline void        wukey_print_info (const char *pre, const KeyInfo_t *info);

static inline int
wukey_str2indx (const char *str, const KeyInfo_t *info)
{
  int i;

  WUASSERT(str && info);

  for (i = 0; info[i].name; ++i) {
    if (strlen(str) == strlen(info[i].name) && strcmp(str, info[i].name) == 0)
      return i;
  }

  return -1;
}

/* str2flag() is a utility function used for locating an entry
 * in a KeyInfo_t structure.  this returns the flag (i.e. cmd)
 * value.
 * */
static inline wuKeyFlag_t
wukey_str2flag (const char *str, const KeyInfo_t *info)
{
  int i;

  WUASSERT(str && info);

  for (i = 0; info[i].name ; ++i) {
    if (str && strlen(str) == strlen(info[i].name) && strcmp(str, info[i].name) == 0)
      break;
  }

  return info[i].flag;
}

static inline const char *
wukey_flag2str_safe(const wuKeyFlag_t flag, const KeyInfo_t *info)
{
  const char *str = wukey_flag2str(flag, info);
  return (str ? str : WULIB_KEY_DEFSTR);
}

static inline const char *
wukey_flag2str (const wuKeyFlag_t flag, const KeyInfo_t *info)
{
  int i;

  WUASSERT(info);

  for (i = 0; info[i].name; ++i) {
    if (info[i].flag == flag) break;
  }

  return info[i].name;
}

static inline int
wukey_flag2indx (const wuKeyFlag_t flag, const KeyInfo_t *info)
{
  int i;

  WUASSERT(info);

  for (i = 0; info[i].name; ++i) {
    if (info[i].flag == flag) return i;
  }

  return -1;
}

static inline void
wukey_print_info (const char *pre, const KeyInfo_t *info)
{
  int i;

  WUASSERT(info);

  printf("%s\n", pre ? pre : "Keyword: ");
  for (i = 0;info[i].name; ++i)
    printf("\tKey = %s, Flag = 0x%08x, Description = %s\n",
        info[i].name, (unsigned int)info[i].flag, info[i].desc);

  return;
}

#ifdef __cplusplus
}
#endif
#endif
