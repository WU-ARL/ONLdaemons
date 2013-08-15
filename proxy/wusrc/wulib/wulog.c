/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/wulog.c,v $
 * $Author: fredk $
 * $Date: 2007/10/16 20:26:35 $
 * $Revision: 1.32 $
 * $Name:  $
 *
 * File:         log.c
 * Author:       Fred Kuhns <fredk@arl.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __sun
#include <inttypes.h>
#include <sys/varargs.h>
#else
#include <stdarg.h>
#include <stdint.h>
#endif
#include <syslog.h>
#include <time.h>

#include <wulib/bits.h>
#include <wulib/wulog.h>

const wulogFlags_t wulog_useDefault = 0x02;
const wulogFlags_t wulog_useNone    = 0x00;
const wulogFlags_t wulog_useSyslog  = 0x01;
const wulogFlags_t wulog_useLocal   = 0x02;
const wulogFlags_t wulog_useBoth    = 0x03;
const wulogFlags_t wulog_useTStamp  = 0x04;

const char *wulogInvStr = "BadLogLvl";

wulogFlags_t wulogMode = 0x02;
#define WULOG_PREFIX_STRING   "Log"

#define WULOG_PREFIX_MAXLEN (WULOG_STRING_MAXLEN+3)

char progName[WULOG_NAME_MAXLEN] = {'W', 'L', 'O', 'G', '\0'};
char logPrefix[WULOG_PREFIX_MAXLEN] = WULOG_PREFIX_STRING;

// predefined levels, Note the array index is (must be) equal to the 
// wuLog Level. i.e. Critical is index 0 and has a value of zero.
const wulogInfo_t wulog_LvlArray[] = {
  // {Syslog-Level, WuLog-Level, Name, Description}
  {LOG_CRIT,    wulogFatal, "Fatal","Program can not continue"},
  {LOG_ERR,     wulogError, "Err",  "Operation/calculation can not continue"},
  {LOG_WARNING, wulogWarn,  "Warn", "Operation/calculation continues but results may be suspect"},
  {LOG_NOTICE,  wulogNote,  "Note", "Filtered informational messages"},
  {LOG_INFO,    wulogInfo,  "Info", "Messages a user may find useful"},
  {LOG_DEBUG,   wulogVerb,  "Verb", "Debugging messages"},
  {LOG_DEBUG,   wulogLoud,  "Loud", "Boardering on noise"},
  {0, 0, NULL, NULL},
};

const char *wuNoName = "NA";

static wulogInfo_t wulog_ModsArray[wulogModCnt] = {
  // {moduleID, Level, Name, description}
  {wulogApp,  wulogWarn, "App",  "Default application code"},
  // library
  {wulogLib,   wulogWarn, "Lib",  "Common library code"},
  {wulogNet,   wulogWarn, "Net",  "Network"},
  {wulogMem,   wulogWarn, "Mem",  "Memory management"},
  {wulogBuf,   wulogWarn, "Buf",  "Buffer management"},
  {wulogMsg,   wulogWarn, "Msg",  "Message I/O"},
  {wulogTime,  wulogWarn, "Time", "Time and Stats"},
  {wulogStat,  wulogWarn, "Stat", "Statistics"},
  {wulogQ,     wulogWarn, "Q",    "Queue management"},
  // Threads
  {wulogWth,   wulogWarn, "Wth",   "wuthreads"},
  {wulogInt,   wulogWarn, "Int",   "wth clock and int processing"},
  {wulogSync,  wulogWarn, "Sync",  "wth syncronization"},
  {wulogSched, wulogWarn, "Sched", "wth general scheduling framework"},
  {wulogSMod,  wulogWarn, "SMod",  "wth scheduling modules"},
  {wulogClock, wulogWarn, "Clock", "wth timer and clock handlers"},
  // Network servers, command processors
  {wulogSession, wulogWarn, "Session", "Session manager"},
  {wulogCmd,  wulogWarn, "Cmd",     "command processor"},
  {wulogServ, wulogWarn, "Serv",    "Core server processing"},
  // Interpreter
  {wulogExpr,    wulogWarn, "Expr", "Expression evaluator"},
  {wulogNamed,   wulogWarn, "Named", "Named Object"},
  {wulogSymbol,  wulogWarn, "Symbol", "Symbol table for named Objects"},
  {wulogValue,   wulogWarn, "Value", "Value Object"},
  {wulogToken,   wulogWarn, "Token", "Tokenizer"},
  {wulogScanner, wulogWarn, "Scanner", "Input Scanner"},
  {wulogShell,   wulogWarn, "Shell", "Command Interpreter"},
  {wulogMNet,    wulogWarn, "MNet", "Metnet"},
  // the rest are all zero's
};

// assume we don't need to verify bounds
static inline wulogLvl_t  wumod2lvl(wulogMod_t mod)  {return wulog_ModsArray[mod].lvl;}
static inline const char *wumod2name(wulogMod_t mod) {return wulog_ModsArray[mod].name;}
static inline const char *wumod2desc(wulogMod_t mod) {return wulog_ModsArray[mod].desc;}
static inline int wulogValidMod(wulogMod_t mod) { return wulog_ModsArray[mod].name != NULL; }
static inline const char *wulvl2name(wulogLvl_t lvl);

static inline const char *wulvl2name(wulogLvl_t lvl)
{
  return wulog_LvlArray[(int)lvl].name;
}

const char *wulogMod2Name(wulogMod_t mod)
{
  if (mod > wulogInvMod && mod < wulogModCnt)
    return wumod2name(mod);
  return wulogInvStr;
}

const char *wulogLvl2Name(wulogLvl_t lvl)
{
  if (lvl > wulogInvLvl && lvl < wulogLvlCnt)
    return wulvl2name(lvl);
  return wulogInvStr;
}

void wulogPrintLvls(void)
{
  int indx;
  for (indx=0; indx < wulogLvlCnt; indx++) {
    printf("Token: %s, Level: %d (syslog lvl %d), Desc: %s\n",
          wulog_LvlArray[indx].name, wulog_LvlArray[indx].lvl,
          wulog_LvlArray[indx].mod, wulog_LvlArray[indx].desc);
  }
}

#define wulog_ckstr(str) ((str) ? str : "")
void wulogPrintLvls2(const char *pre)
{
  int indx;
  printf("%sWulog debug levels:\n", wulog_ckstr(pre));
  printf("%s    Tag    Lvl/Slog  Desc\n", wulog_ckstr(pre));
  for (indx=0; indx < wulogLvlCnt; indx++) {
    printf("%s    %-6s %3d/%-3d  %s\n",
          wulog_ckstr(pre),
          wulog_LvlArray[indx].name,
          wulog_LvlArray[indx].lvl,
          wulog_LvlArray[indx].mod,
          wulog_LvlArray[indx].desc);
  }
}

void wulogPrintMods(void)
{
  int indx;
  for (indx=0; indx < wulogModCnt; indx++) {
    if (wulogValidMod(indx))
      printf("Name %s, ModID %d, Level %d, Desc: %s\n",
          wulog_ModsArray[indx].name, wulog_ModsArray[indx].mod,
          wulog_ModsArray[indx].lvl, wulog_ModsArray[indx].desc);
  }
}

void wulogPrintMods2(const char *pre)
{
  int indx;
  printf("%sWulog debug modules:\n", wulog_ckstr(pre));
  printf("%sMod       ID   Lvl    Desc\n", wulog_ckstr(pre));
  for (indx=0; indx < wulogModCnt; indx++) {
    if (wulogValidMod(indx))
      printf("%s%-8s %3d  %-6s  %s\n",
          wulog_ckstr(pre),
          wulog_ModsArray[indx].name,
          wulog_ModsArray[indx].mod,
          wulvl2name(wulog_ModsArray[indx].lvl),
          wulog_ModsArray[indx].desc);
  }
}

wulogMod_t wulogName2Mod(const char* name)
{
  int i;
  if (name == NULL || name[0] == '\0') return wulogInvMod;
  for (i = 0; i < wulogModCnt; i++) {
    if (wulogValidMod(i) && strncmp(name, wulog_ModsArray[i].name, strlen(wulog_ModsArray[i].name)) == 0)
      break;
  }
  return (i < wulogModCnt) ? wulog_ModsArray[i].mod : wulogInvMod;
}

wulogLvl_t wulogName2Lvl(const char* name)
{
  int i;
  if (name == NULL || name[0] == '\0') return wulogInvLvl;
  for (i = 0; i < wulogLvlCnt; i++) {
    if (strncmp(name, wulog_LvlArray[i].name, strlen(wulog_LvlArray[i].name)) == 0)
      break;
  }
  return (i < wulogLvlCnt) ? wulog_LvlArray[i].lvl : wulogInvLvl;
}

void wulogInit(const wulogInfo_t *mdesc, int cnt, int lvl, const char *prog, wulogFlags_t flags)
{
  int i = 0;

  if (prog && prog[0] != '\0')
    wustrncpy(progName, prog, WULOG_NAME_MAXLEN);
  else
    wustrncpy(progName, "wulog", WULOG_NAME_MAXLEN);

  wulogMode = flags;

  if (wubiton(flags, wulog_useSyslog))
    openlog(progName, LOG_CONS, LOG_LOCAL0);

  if (wubiton(flags, wulog_useLocal)) {
#ifdef __cplusplus
    sync_with_stdio(true);
#endif
    setlinebuf(stdout);
  }

  for (i = 0; i < wulogUser; i++)
    wulog_ModsArray[i].lvl = lvl;

  // assume the caller knows what they are doing!
  if (mdesc && cnt > 0) {
    for (i = 0; i < cnt ; i++) {
      if (mdesc[i].mod != wulogInvMod) {
        wulog_ModsArray[mdesc[i].mod].name = mdesc[i].name;
        wulog_ModsArray[mdesc[i].mod].desc = mdesc[i].desc;
        wulog_ModsArray[mdesc[i].mod].lvl  = mdesc[i].lvl;
      }
    }
  }
  return;
}

void wulogSetLvl(wulogMod_t mod, wulogLvl_t lvl)
{
  wulog_ModsArray[mod].lvl = lvl;
  return;
}

int wulogGetLvl(wulogMod_t mod)
{
  return wulog_ModsArray[mod].lvl;
}

void wulogSetDef(wulogLvl_t lvl)
{
  int i;
  for (i = 0; i < wulogModCnt; i++)
    wulog_ModsArray[i].lvl = lvl;
}

void wulogSetMode(wulogFlags_t flags) {wulogMode = flags;}

void wulogAddPrefix(const char *pre)
{
  size_t rem = WULOG_PREFIX_MAXLEN - strlen(WULOG_PREFIX_STRING) - 1;
  *logPrefix = '\0';
  if (pre)
    wustrncpy(logPrefix, pre, rem);
  strcat(logPrefix, WULOG_PREFIX_STRING);
  return;
}

void wulogSetPrefix(const char *pre)
{
  *logPrefix = '\0';
  if (pre)
    wustrncpy(logPrefix, pre, WULOG_PREFIX_MAXLEN-1);
  return;
}

#define TSTR_MAX 80

inline char *getTimeStamp(char *, size_t);
inline char *getTimeStamp(char *buf, size_t n)
{
  const char *tfmt = "%m/%d %H:%M:%S";
  time_t tt = time(NULL);
  struct tm *tmp;

  *buf = '\0';

  if (tt == (time_t)-1)
    return buf;
  
  if ((tmp = localtime(&tt)) == NULL)
    return buf;

  if (strftime(buf, n, tfmt, tmp) == 0)
    *buf = '\0';

  return buf;
}

/* inspired by K&R */
void wulog (wulogMod_t mod, wulogLvl_t lvl, char *fmt, ...)
{
  va_list args;
  char msgbuf[WULOG_MSG_MAXLEN];
  // char *msgbuf, *tstrbuf;
  char tstrbuf[TSTR_MAX];
  int n;

  if (wulogMode == 0 || wumod2lvl(mod) < lvl)
    return;

  // malloc broke on the xscale embedded environment so back to the stack,
  // if ((msgbuf = malloc(WULOG_MSG_MAXLEN)) == NULL) {
  //   fprintf(stderr, "%s: ***** Error ***** "
  //                   "\n\tUnable to allocate memory for text buffer!"
  //                   "\n\tfmt string = %s\n",
  //                   logPrefix, fmt);
  //   return;
  // }
  // if ((tstrbuf = malloc(TSTR_MAX)) == NULL) {
  //   fprintf(stderr, "%s: ***** Error ***** "
  //       "\n\tUnable to allocate memory for text time stamp!",
  //       logPrefix);
  // }

  if (wubiton(wulogMode, wulog_useTStamp) && tstrbuf) {
    n = snprintf((char *)msgbuf, WULOG_MSG_MAXLEN, "%s (%s:%s:%s): %s",
                 logPrefix, wumod2name(mod), wulvl2name(lvl),
                 getTimeStamp(tstrbuf, TSTR_MAX), fmt);
  } else {
    n = snprintf((char *)msgbuf, WULOG_MSG_MAXLEN, "%s (%s:%s): %s",
                 logPrefix, wumod2name(mod), wulvl2name(lvl), fmt);
  }
  if (n >= WULOG_MSG_MAXLEN)
    msgbuf[WULOG_MSG_MAXLEN-1] = '\0';

  va_start(args, fmt);
  if (wubiton(wulogMode, wulog_useSyslog))
    vsyslog(wulog_LvlArray[lvl].mod, msgbuf, args);
  if (wubiton(wulogMode, wulog_useLocal))
    vprintf(msgbuf, args);
  va_end(args);

  // free(msgbuf);
  // free(tstrbuf);
  return;
}
