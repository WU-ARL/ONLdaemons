/*****************************************************************************/
/* RadiSys ATCA-70xx Software Development Kit                                */
/* All of this software and any documentation provided with this software is */
/* Copyright 2005 RadiSys Corporation.                                       */
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
/*    Filename:         trainspi.h                                           */
/*                                                                           */
/*    Description:      Include file for the trainspi sample code            */
/*                                                                           */
/*    Version History:                                                       */
/*                                                                           */
/*    Date                 Comment                          Author           */
/*    ====================================================================== */
/*    02/04/2005           Initial creation                 AES              */
/*    04/29/2005           Alpha version                    HS               */
/*    08/10/2005           Adapted for sample code          AES              */
/*****************************************************************************/
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
#include "common_train.h"

/************/
/* Literals */
/************/
#define SUCCESS   RC_SUCCESS
#define FAILURE   0xFFFFFFFF
#define MAX_USER_INPUT 5
