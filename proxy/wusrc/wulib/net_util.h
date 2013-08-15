/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/net_util.h,v $
 * $Author: fredk $
 * $Date: 2008/02/11 16:41:04 $
 * $Revision: 1.15 $
 * $Name:  $
 *
 * File:         net.h
 * Created:      01/05/2005 04:32:38 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WUNET_UTIL_H
#define _WUNET_UTIL_H

#define makeNetTypeName(base)  wun_##base##_t
#define makeNetName(base)      wun_##base
#define makeNetErrName(base)   wun_et_##base
#define makeNetLvlName(base)   wun_el_##base

#ifdef __cplusplus
extern "C" {
#endif

#define WUNET_MSG_STRLEN    WULOG_MSG_MAXLEN

/****************************************************************************
 *                         Error type declarations                          *
 ****************************************************************************/
typedef enum makeNetTypeName(errLevel) {
              /* Good to know -- operation succeeeded but something unusual occured */
              makeNetLvlName(Info),
              /* Recoverable or noncritical error -- resource state unchanged */
              makeNetLvlName(Warn),
              /* Unrecoverable error -- close resource, throw exception */
              makeNetLvlName(Err),
} makeNetTypeName(errLevel);

typedef enum makeNetTypeName(errType) {
        // Marks start of the NoError status codes
        makeNetErrName(NoError_Start) = 0,
              // value = 0, traditional no error value
              makeNetErrName(noError) = makeNetErrName(NoError_Start),
              makeNetErrName(dataSuccess), // successful operation, may be partial or full
              makeNetErrName(dataPartial), // incomplete message read/write
              makeNetErrName(dataComplete),// successful, data read/written operation
              makeNetErrName(dataNone),    // no data read/written and Async operation
              makeNetErrName(inProgress),  // EINPROGRESS for connect
              makeNetErrName(EndOfFile),   // peer closed connection, no data remains
        makeNetErrName(NoError_End) = makeNetErrName(EndOfFile),
              // Real Errors/Exceptions
        makeNetErrName(Error_Start),
              makeNetErrName(connFailure) = makeNetErrName(Error_Start),
                                           // EPIPE, attempt to write to a closed connection
                                           // From Stevens: do a write after getting a
                                           // reset will cause EPIPE to be returned
                                           // Also returned for a ECONNRESET see timeOut
              makeNetErrName(intrSysCall), // (EINTR) Interrupted system call
              makeNetErrName(protoError),  // unspecified protocol error
              makeNetErrName(progError),   // logic error in program
              makeNetErrName(resourceErr), // EMFILE, ENFILE on open/accept
                                           // or ENOBUFS, ENOMEM: no resources
              makeNetErrName(timeOut),     // ETIMEDOUT: connection timed out can happen if
                                           // a server is halted (not accessable on the
                                           // net). If the server's host is on the net
                                           // but the server program is not listening
                                           // to socket then a reset is sent to client
                                           // Could also mean that select has
                                           // timed out.
                                           // and ECONRESET is returned.
              makeNetErrName(paramError),  // invalid parameter passed
              makeNetErrName(authError),   // requester is not authorized
              makeNetErrName(addrError),   // an invalid target address was specified
              makeNetErrName(dstError),    // unable to connect to target system
              makeNetErrName(badConn),     // referenced connection is not open or bad ID
              makeNetErrName(netError),    // generic network related error
              makeNetErrName(badProto),    // address family/domain or protocol is not
                                           // supported corresponds to EAFNOSUPPORT,
                                           // EPROTONOSUPPORT
              makeNetErrName(badClose),    // connection closed in middle of message or some
                                           // other inappropriate time
              makeNetErrName(fmtError),    // message format error
              makeNetErrName(dupError),    // requested action (create connection etc) already
                                           // performed. thus a 'duplicate' action error.
                                           // Assumed to be a temporary error.
              makeNetErrName(sysError),    // Some unknown/unclassified system error
              makeNetErrName(libError),    // unexpected lib operation or state ... BUG
              makeNetErrName(opError),     // requested operation returned unexpected results
              makeNetErrName(rpcError),    // Error in RPC call
              makeNetErrName(notFound),    // Requested entity/resource not found
              makeNetErrName(Error_End),   // marks the end of error codes
        // Marks end of the list (max range for values)
        makeNetErrName(etCount) = makeNetErrName(Error_End)
} makeNetTypeName(errType);

typedef enum makeNetTypeName(netOp) {
              makeNetName(openOp),   // open/acquire resource
              makeNetName(closeOp),  // close/release resource
              makeNetName(readOp),   // any read/recv op
              makeNetName(writeOp),  // any write/send op
              makeNetName(ctlOp),    // getsockopt, fcntl etc
              makeNetName(listenOp), // listen, accept (passive server)
              makeNetName(connectOp),// active client
              makeNetName(syncOp),   // any synchronization operation (lock, unlock, etc)
              makeNetName(noOp)      // no resource manipulation or no error
} makeNetTypeName(netOp);

typedef struct makeNetTypeName(errInfo) {
  makeNetTypeName(errLevel) el_;
  int                      syserr_; // system error number (errno)
  // A BIG Assumption: Strings assigned to this object will not be dynamically
  // allocated. Or atleast this object assumes no responsibility to managing
  // the memory.
  const char*              who_;
  const char*              msg_;
} makeNetTypeName(errInfo);

/****************************************************************************
 *                       inline function declarations                       *
 ****************************************************************************/
static inline const char *makeNetName(errlvl2str)(makeNetTypeName(errLevel) lvl);
static inline       int   makeNetName(isError)(makeNetTypeName(errType) et);
static inline const char *makeNetName(err2str)(makeNetTypeName(errType) et);
static inline void        makeNetName(errPrint)(makeNetTypeName(errInfo) *einfo,
                                                makeNetTypeName(errType) et);
static inline void makeNetName(errInit)(makeNetTypeName(errInfo) *einfo,
                                        makeNetTypeName(errLevel) el,
                                        int         s,
                                        const char* w,
                                        const char* m);
static inline void makeNetName(errReset)(makeNetTypeName(errInfo) *einfo);

/****************************************************************************
 *                       inline function definition                         *
 ****************************************************************************/
static inline const char *makeNetName(errlvl2str)(makeNetTypeName(errLevel) lvl)
{
  switch (lvl) {
    case makeNetLvlName(Info): return "Info"; break;
    case makeNetLvlName(Warn): return "Warn"; break;
    case makeNetLvlName(Err):  return "Error"; break;
  }
  return "BadLvl";
}

static inline void makeNetName(errReset)(makeNetTypeName(errInfo) *einfo)
{
  if (einfo == NULL) return;
  einfo->el_     = makeNetLvlName(Info);
  einfo->syserr_ = 0;
  einfo->who_    = NULL;
  einfo->msg_    = NULL;
}

static inline
void makeNetName(errInit)(makeNetTypeName(errInfo) *einfo,
                          makeNetTypeName(errLevel) el,
                          int         s,
                          const char* w,
                          const char* m)
{
  if (einfo == NULL) return;
  einfo->el_ = el; einfo->syserr_ = s; einfo->who_ = w; einfo->msg_ = m;
  return;
}

static inline void makeNetName(errPrint)(makeNetTypeName(errInfo) *einfo,
                                         makeNetTypeName(errType) et)
{
  wulogLvl_t lvl = wulogVerb;
  if (einfo == NULL) {
    wulog(wulogNet, lvl, "errPrint: Err type %d (%s)\n",
          et, makeNetName(err2str)(et));
      return;
  }
  switch (einfo->el_) {
    case makeNetLvlName(Err): lvl = wulogError; break;
    case makeNetLvlName(Warn): lvl = wulogWarn; break;
    case makeNetLvlName(Info): lvl = wulogInfo; break;
  }

  wulog(wulogNet, lvl, "\n\t%s: %s.\n\terrType %d: %s\n\tsysErr %d: %s\n",
        (einfo->who_ ? einfo->who_ : "errPrint"),
        (einfo->msg_ ? einfo->msg_ : "Reporting error"),
        et, makeNetName(err2str)(et),
        einfo->syserr_,
        (einfo->syserr_ > 0) ? strerror(einfo->syserr_) : "No system error code given");

  return;
}

static inline int makeNetName(isError)(makeNetTypeName(errType) et)
  { return (et > makeNetErrName(Error_Start) && et < makeNetErrName(Error_End)); }

static inline const char *makeNetName(err2str)(makeNetTypeName(errType) et)
{
  switch (et) {
    // case makeNetErrName(NoError_Start): return "NoError Marker"; break;
    case makeNetErrName(noError):       return "noError"; break;
    case makeNetErrName(dataSuccess):   return "dataSuccess"; break;
    case makeNetErrName(dataPartial):   return "dataPartial"; break;
    case makeNetErrName(dataComplete):  return "dataComplete"; break;
    case makeNetErrName(dataNone):      return "dataNone"; break;
    case makeNetErrName(inProgress):    return "inProgress"; break;
    case makeNetErrName(EndOfFile):     return "EndOfFile"; break;
    // case makeNetErrName(NoError_End):   return "NoError_End Marker"; break;
    // case makeNetErrName(Error_Start):   return "Error_Start Marker"; break;
    case makeNetErrName(connFailure):   return "connFailure"; break;
    case makeNetErrName(intrSysCall):   return "intrSysCall"; break;
    case makeNetErrName(protoError):    return "protoError"; break;
    case makeNetErrName(progError):     return "progError"; break;
    case makeNetErrName(resourceErr):   return "resourceErr"; break;
    case makeNetErrName(timeOut):       return "timeOut"; break;
    case makeNetErrName(paramError):    return "paramError"; break;
    case makeNetErrName(authError):     return "authError"; break;
    case makeNetErrName(addrError):     return "addrError"; break;
    case makeNetErrName(dstError):      return "dstError"; break;
    case makeNetErrName(badConn):       return "badConn"; break;
    case makeNetErrName(netError):      return "netError"; break;
    case makeNetErrName(badProto):      return "badProto"; break;
    case makeNetErrName(badClose):      return "badClose"; break;
    case makeNetErrName(fmtError):      return "fmtError"; break;
    case makeNetErrName(dupError):      return "dupError"; break;
    case makeNetErrName(sysError):      return "sysError"; break;
    case makeNetErrName(libError):      return "libError"; break;
    case makeNetErrName(opError):       return "opError"; break;
    case makeNetErrName(rpcError):      return "rpcError"; break;
    //case makeNetErrName(Error_End):     return "Error_End"; break;
    default:                            return "Unknown"; break;
  }
  return "Unknown";
}

#ifdef __cplusplus
};
#endif

#endif
