/*****************************************************************************/
/* RadiSys ATCA-70xx Software Development Kit                                */
/* All of this software and any documentation provided with this software is */
/* Copyright 2005, 2006 RadiSys Corporation.                                 */
/*                                                                           */
/* All source code provided in this distribution is RadiSys Confidential.    */
/*                                                                           */
/* RADISYS LICENSES THIS SOFTWARE AND ANY DOCUMENTATION PROVIDED WITH THIS   */
/* SOFTWARE TO LICENSEE AS-IS, WITHOUT WARRANTY.  RADISYS DOES NOT MAKE, AND */
/* LICENSEE EXPRESSLY HEREBY WAIVES, ALL WARRANTIES, EXPRESS OR IMPLIED.     */
/* WITHOUT LIMITING THE FOREGOING, RADISYS DOES NOT MAKE AND EXPRESSLY       */
/* DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A        */
/* PARTICULAR PURPOSE, TITLE, NON-INFRINGEMENT, ACCURACY OF DATA, QUIET      */
/* ENJOYMENT AND FREEDOM FROM DESIGN DEFECT.                                 */
/*                                                                           */
/* This software and any documentation provided with this software is        */
/* subject to United States export control laws and regulations.  Each       */
/* licensee must comply with all applicable United States export control     */
/* laws, including, but not limited to, the rules and regulations of the     */
/* Bureau of Industry and Secruity ("BIS") of the US Department of Commerce  */
/* and the Office of Foreign Assets Control ("OFAC") of the US Department of */
/* the Treasury.  Such compliance includes, but is not limited to,           */
/* licensee's refraining from the export or re-export, either directly or    */
/* indirectly, of this software and any documentation provided with this     */
/* software or any product made from or with such software or documentation  */
/* without first obtaining any required license or other approval from BIS   */
/* or from any other agency or department of the United States government    */
/* with jurisdiction over such export or re-export.                          */
/*                                                                           */
/* This software and any documentation provided with this software           */
/* constitute a "commericial item," as that term is defined in 48 C.F.R.     */
/* 2.101, consisting of "commercial computer software" and "commercial       */
/* computer software documentation," as such terms are used in 48 C.F.R.     */
/* 12.212.  Consistent with 48 C.F.R. 12.212 and 48 C.F.R. 227.7202-1        */
/* through 227.7202-4, all U.S. government users acquire this software and   */
/* any documentation proivided with this software with only those rights     */
/* provided in the applicable license agreement between RadiSys Corporation  */
/* and the licensee.                                                         */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*    Filename:         common_train.h                                       */
/*                                                                           */
/*    Description:      Common training header file for ATCA-7010            */
/*                                                                           */
/*    Version History:                                                       */
/*                                                                           */
/*    Date                 Comment                          Author           */
/*    ====================================================================== */
/*    05/26/2006           Common training code created     Andrew Watts     */
/*    08/15/2006           Dynamic Device Training          AES              */
/*                                                                           */
/*****************************************************************************/
#ifndef __common_train__
#define __common_train__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include "api.h"

/************/
/* Literals */
/************/
#define SUCCESS   RC_SUCCESS
#define FAILURE   0xFFFFFFFF

// Bitmask for which device(s) to train
#define TRAIN_NPUA   1
#define TRAIN_NPUB   2
#define TRAIN_RTM    4
#define TRAIN_FIC    8

// Literals used in training retry attempts
#define TRAINING_RETRIES        10
#define TRAINING_RETRY_DELAY    2000000 //in us

/************/
/* Typedefs */
/************/
typedef struct
{
   UINT32 rxPort;
   UINT32 rxInterface;
   UINT32 txInterface;
   UINT32 txPort;
} Connection;

/***********/
/* Externs */
/***********/
extern char *InterfaceNames[];
extern Connection SystemFICConnections[];
extern Misc_Device_Info_t DeviceInfo;
extern UINT32 appendCRC;
extern UINT32 ConnectionCount;

extern UINT32 NPUAInterfaceStartedRx;
extern UINT32 NPUAInterfaceStartedTx;
extern UINT32 NPUBInterfaceStartedRx;
extern UINT32 NPUBInterfaceStartedTx;


/**************/
/* Prototypes */
/**************/
UINT32 Train_SPI_NPU(UINT32 Processor, UINT32 CalendarLength);
UINT32 Train_NPU_SPI(UINT32 Processor, UINT32 CalendarLength);
UINT32 Train_NPU_SPI_Calendar(UINT32 Processor, UINT32 CalendarLength, CALENDAR *pCalendar);
UINT32 Train_RTM(UINT32 maxFrameSize);
UINT32 Train_FIC(BOOL32 MACEnable, BOOL32 LoopEnable, UINT32 maxFrameSize);
UINT32 Train(Connection *connections,
             UINT32 connectionCount,
             UINT32 deviceMask,
             UINT32 rxLoWaterMark,
             UINT32 rxHiWaterMark,
             UINT32 rtmClkMult,
             UINT32 ficClkMult,
             UINT32 npuAClkMult,
             UINT32 npuBClkMult,
             UINT32 maxFrameSize,
             UINT32 FICLoopEnable,
             UINT32 useTenGbECalLen);
UINT32 openConnections(Connection connections[], UINT32 connectionCount);
UINT32 numConnectionsToFifoSize(UINT32 numberConnections);
UINT32 countConnections(Connection connections[],
                        UINT32 connectionCount,
                        UINT32 Interface,
                        UINT32 Direction);

#endif //common_training

