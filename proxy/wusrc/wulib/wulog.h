/* 
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/wulog.h,v $
 * $Author: fredk $
 * $Date: 2007/06/06 21:24:37 $
 * $Revision: 1.31 $
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

#ifndef _WULOG_H
#define _WULOG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
/* Also putting debug stuff in here */
#if defined(WUDEBUG)
#  include <assert.h>
#  define STATIC    /* Nothing */
#  define INLINE    /* Nothing */
#  define WUASSERT(cond)     assert((cond))
#else
#  define STATIC    static
#  define INLINE    inline
#  define WUASSERT(cond)           /* Nothing */
#endif

// define some convenience macros for dealing with long options
#define OPT_NOARG   0 /* no argument required */
#define OPT_REQARG  1 /* required argument */
#define OPT_OPTARG  2 /* argument is optional */

#define WULOG_MSG_MAXLEN           256
#define WULOG_MSG_MIDLEN            64
#define WULOG_NAME_MAXLEN  FILENAME_MAX
// WULOG_STRING_MAXLEN used for prefix and other short string specifications
#define WULOG_STRING_MAXLEN         32

typedef enum wulogMod_t {
  wulogInvMod  = -1,
  // don't really need to specify these values
  wulogApp,
  wulogLib,     // core library routines
  wulogNet,     // network code
  wulogMem,     // memory and buffer management code
  wulogBuf,
  wulogMsg,
  wulogTime,    // Time
  wulogStat,    // stats
  wulogQ,       // queueing stuff
  // wuthreads logging
  wulogWth,     // Core wth library
  wulogInt,     // interrupt, asynch processing
  wulogSync,    // wth synchronization mechanisms
  wulogSched,   // generic wth scheduling
  wulogSMod,    // wth scheduling modules
  wulogClock,   // wth periodic and timer processing
  // Network servers, command processors
  wulogSession, // client-server connection (Session)
  wulogCmd,     // command processing
  wulogServ,    // Core server processing
  wulogClient,  // Core client side processing
  wulogStats,   // Statistics and Monitoring functions
  // Interpreter
  wulogExpr,    // Tree and Expression analyzer
  wulogNamed,   // Named object, used for command interpreter
  wulogSymbol,  // symbol manipulation for interpreter
  wulogValue,   // value objects, represent symbol's value
  wulogToken,   // tokenizer
  wulogScanner, // input scanner
  wulogShell,   // command interpreter
  // Simplifies things for user/app code
  wulogMNet,
#define wulogUserStart 128
  wulogUser    = wulogUserStart,
  wulogModCnt  = 256,
} wulogMod_t;

typedef enum wulogLvl_t {
  wulogInvLvl = -1,
  wulogFatal,
  wulogError,
  wulogWarn,
  wulogNote,
  wulogInfo,
  wulogVerb,
  wulogLoud,
  wulogLvlCnt,
} wulogLvl_t;

typedef unsigned int wulogFlags_t;
extern const wulogFlags_t wulog_useNone;
extern const wulogFlags_t wulog_useDefault;
extern const wulogFlags_t wulog_useSyslog;
extern const wulogFlags_t wulog_useLocal;
extern const wulogFlags_t wulog_useBoth;
extern const wulogFlags_t wulog_useTStamp;

typedef struct wulogInfo_t {
  wulogMod_t  mod; // Module ID
  wulogLvl_t  lvl; // reporting level
  char      *name; // name used when reporting
  char      *desc; // description
} wulogInfo_t;

#ifndef MyLogModule
#define MyLogModule  wulogApp
#endif

static inline char *wustrncpy(char *dest, const char *src, size_t n);
static inline char *wustrncpy(char *dest, const char *src, size_t n)
{
  dest[n-1] = '\0';
  return strncpy(dest, src, n-1);
}

#define WarnLog(str, code) \
  do { \
    char WarnStr[WULOG_MSG_MAXLEN];\
    if ((code) == 0 || strerror_r((code), (char *)WarnStr, (size_t)WULOG_MSG_MAXLEN)) \
      strcpy((char *)WarnStr, "No error code"); \
    wulog(MyLogModule, wulogError, "%s", (str)); \
    fprintf(stderr, "%s:%s:%d:\n\t%s\n\tCode = %s\n",\
            __FILE__, __func__, __LINE__, (str), WarnStr); \
  } while (0)

//    if ((code) == 0 || strerror_r((code), (char *)WarnStr, (size_t)WULOG_MSG_MAXLEN) == NULL)
#define debugLog(mod, lvl, str, code) \
  do { \
    char *dlog_WarnStr_ = "No error code supplied", *dlog_estr_;\
    if ((code) > 0) \
      dlog_estr_ = strerror((code));\
    else\
      dlog_estr_ = dlog_WarnStr_;\
    wulog((mod), (lvl), "(%s:%s:%d):\n\t%s\n\tErrno (%d) = %s\n",\
            __FILE__, __func__, __LINE__, (str), (code), dlog_estr_); \
  } while (0)


/*
 * May want to add a call to sync_with_stdio(true/false).
 * */
void       wulogPrintMods(void);
void       wulogPrintLvls(void);
void       wulogPrintMods2(const char *pre);
void       wulogPrintLvls2(const char *pre);
wulogLvl_t wulogName2Lvl(const char* name);
const char *wulogLvl2Name(wulogLvl_t lvl);
const char *wulogMod2Name(wulogMod_t mod);
wulogMod_t wulogName2Mod(const char* name);
void       wulogInit(const wulogInfo_t *mdesc, int cnt, int lvl, const char *prog, wulogFlags_t flags);
void       wulogSetLvl(wulogMod_t mod, wulogLvl_t lvl);
int        wulogGetLvl(wulogMod_t mod);
void       wulogSetDef(wulogLvl_t lvl);
void       wulogSetMode(wulogFlags_t flags);
void       wulogAddPrefix(const char *pre);
void       wulogSetPrefix(const char *pre);
void       wulog(wulogMod_t mod, wulogLvl_t lvl, char *fmt, ...);

#ifdef WUDEBUG
#define WULOG(ARGLIST)     wulog ARGLIST
#else
#define WULOG(ARGLIST)   // ARGLIST
#endif

#define ATTRIB_UNUSED __attribute__((__unused__))

/* And finally some string utility functions */
static inline size_t wustrncat(char *dest, const char *src, size_t len);

static inline size_t wustrncat(char *dest, const char *src, size_t len)
{
  size_t n;
  if (dest == NULL || src == NULL) return 0;
  n = strlen(src)+1;
  strncat(dest, src, len-1);
  return ((n <= len) ? len - n : 0);
}

#ifdef __cplusplus
}
#endif
#endif /* _WU_LOG_H */
