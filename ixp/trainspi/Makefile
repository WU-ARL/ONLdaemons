#/*****************************************************************************/
#/* RadiSys ATCA-70xx Software Development Kit                                */
#/* All of this software and any documentation provided with this software is */
#/* Copyright 2005 RadiSys Corporation.                                       */
#/*                                                                           */
#/* All source code provided in this distribution is RadiSys Confidential.    */
#/*                                                                           */
#/* RADISYS LICENSES THIS SOFTWARE AND ANY DOCUMENTATION PROVIDED WITH THIS   */
#/* SOFTWARE TO LICENSEE AS-IS, WITHOUT WARRANTY.  RADISYS DOES NOT MAKE, AND */
#/* LICENSEE EXPRESSLY HEREBY WAIVES, ALL WARRANTIES, EXPRESS OR IMPLIED.     */
#/* WITHOUT LIMITING THE FOREGOING, RADISYS DOES NOT MAKE AND EXPRESSLY       */
#/* DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A        */
#/* PARTICULAR PURPOSE, TITLE, NON-INFRINGEMENT, ACCURACY OF DATA, QUIET      */
#/* ENJOYMENT AND FREEDOM FROM DESIGN DEFECT.                                 */
#/*                                                                           */
#/* This software and any documentation provided with this software is        */
#/* subject to United States export control laws and regulations.  Each       */
#/* licensee must comply with all applicable United States export control     */
#/* laws, including, but not limited to, the rules and regulations of the     */
#/* Bureau of Industry and Secruity ("BIS") of the US Department of Commerce  */
#/* and the Office of Foreign Assets Control ("OFAC") of the US Department of */
#/* the Treasury.  Such compliance includes, but is not limited to,           */
#/* licensee's refraining from the export or re-export, either directly or    */
#/* indirectly, of this software and any documentation provided with this     */
#/* software or any product made from or with such software or documentation  */
#/* without first obtaining any required license or other approval from BIS   */
#/* or from any other agency or department of the United States government    */
#/* with jurisdiction over such export or re-export.                          */
#/*                                                                           */
#/* This software and any documentation provided with this software           */
#/* constitute a "commericial item," as that term is defined in 48 C.F.R.     */
#/* 2.101, consisting of "commercial computer software" and "commercial       */
#/* computer software documentation," as such terms are used in 48 C.F.R.     */
#/* 12.212.  Consistent with 48 C.F.R. 12.212 and 48 C.F.R. 227.7202-1        */
#/* through 227.7202-4, all U.S. government users acquire this software and   */
#/* any documentation proivided with this software with only those rights     */
#/* provided in the applicable license agreement between RadiSys Corporation  */
#/* and the licensee.                                                         */
#/*                                                                           */
#/*****************************************************************************/
#/*                                                                           */
#/*    Filename:         sample.mak                                           */
#/*                                                                           */
#/*    Description:      Makefile for the ATCA-70xx sample code               */
#/*                                                                           */
#/*    Version History:                                                       */
#/*                                                                           */
#/*    Date                 Comment                          Author           */
#/*    ====================================================================== */
#/*    02/05/2005           Initial creation                 AES              */
#/*    05/05/2005           Corrections for Remote Proxy     AES              */
#/*    08/10/2005           Adapted for sample code          AES              */
#/*****************************************************************************/
#
# Define generic characteristics for this build.
#
TYPE=app
REMOTE=y
STATIC=y
BUILDBASE = build/
include ../Makefile.inc
major		?= 0
minor 	?=	0
subminor	?=	0

#
# For linking to the API
# Note that the next two must be taken relative to SRCDIR.
#
RSYS_INCLUDE_DIR	= $(ATCA7010_DIR)/include
RSYS_API_DIR		= $(ATCA7010_DIR)/api/arm

#
# Set up the cross compiler.
#
CROSS_COMPILE = ${MVL_CROSS_BIN}/xscale_be-

AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CC) -Xlinker -EB
STRIP = $(CROSS_COMPILE)strip

#
# Common build options.
#
CFLAGS = -O2 -Wall -I. -I$(ATCA7010_DIR) -I$(RSYS_INCLUDE_DIR) -DSTATIC=static -DINLINE="static inline" \
			-D__$(OS) -D_GNU_SOURCE -D_THREAD_SAFE -Wstrict-prototypes \
			-Wno-trigraphs -fno-strict-aliasing -fno-common
CXXFLAGS = $(CFLAGS)

#
# For big endian architectures
#
CFLAGS += -DRSYS_BIG_ENDIAN -mbig-endian

#
# For the chip architectures
#
CFLAGS += -DRSYS_ARCH_ARM
CFLAGS += -DRSYS_ARCH="arch_arm"

#
# For the operating system
#
CFLAGS += -DOS=LINUX -DRSYS_OS_LINUX -D__linux__
CFLAGS += -DRSYS_OS="os_linux"

#
# For debugging.  Invoke the make with "PRE=y" in order to
# generate preprocessor output rather than true compiling.
#
ifeq ($(DEBUG),y)
	CFLAGS += -g -DDEBUG
endif
ifeq ($(PRE),y)
	CFLAGS += -P -E
endif
ifeq ($(PROFILE),y)
	CFLAGS += -g -pg -DPROFILING
	LD += -pg
endif

APIBASE = ppm

TARGET = .
EXENAME = trainspi
OBJS = trainspi.o common_train.o

APISUFFIX = a

EXE = $(BUILDBASE)$(EXENAME)
API = $(RSYS_API_DIR)/lib$(APIBASE).$(APISUFFIX)
OBJS := $(OBJS:%.o=$(BUILDBASE)%.o)

CFLAGS += -I$(RSYS_INCLUDE_DIR)/include
LIBS = -Xlinker -l$(APIBASE) -lpthread
LDOPTS = -Xlinker -L$(RSYS_API_DIR)

###############
# Build rules #
###############

#
# Default rule for building a .c file is given to keep screen from
# getting cluttered with all the build options.
#
$(BUILDBASE)%.o : %.c
ifndef DEBUG
	echo Compiling \($(ARCH) $(OS) $(RSYS_CUST_FLAGS)\) $< ... && $(CC) $(CFLAGS) -o $@ -c $<
else
	$(CC) $(CFLAGS) -o $@ -c $<
endif

all : info install

info: createdir
	@echo Building : $(EXE)
	@echo Version  : $(major).$(minor).$(subminor)

install : $(EXE)
	chmod a+x $(EXE)
	cp -p $(EXE) $(TARGET)

$(EXE) : $(OBJS) $(API)
	$(LD) $(LDOPTS) -o $(EXE) $(OBJS) $(LIBS)

$(OBJS) : Makefile

createdir: 
	mkdir $(BUILDBASE) -p

clean:
	rm -f $(EXE) $(BUILDBASE)*.o $(BUILDBASE)*.so *~ $(BUILDBASE)core

