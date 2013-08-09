# Copyright (c) 2009-2013 Charlie Wiseman, Jyoti Parwatikar, John DeHart 
# and Washington University in St. Louis
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
#    limitations under the License.
#

WUARL ?= ../../../..
DNET  ?= $(WUARL)/dnet
WUSRC ?= $(WUARL)/wusrc

include ../Makefile.inc
include ../../Makefile.inc

DEBUG = -g

CXXFLAGS = ${DEBUG} -DMyOS=Linux -DWUDEBUG -UNDEBUG -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual -D_OS_LINUX_USER -DIX_PLATFORM_2800 -DNO_INTEL_LIB_SUPPORT -DINCLUDE_IIPC_QDR -DOS=LINUX ${WUIXP_CXXFLAGS} -DIXP_XSCALE

INCLUDES = -I${IXA_ME_TOOLS}/include -I${IXA_ME_TOOLS}/XSC_CoreLibs ${WUIXP_USRINC} -I${ATCA7010_DIR}/include -I${DNET}/npe -I${WUSRC} -I${DNET}/npe/IDT_IMS -I${DNET}/npe/IDT_IMS/API -I${DNET}/npe/IDT_IMS/include -I${DNET}/npe/IDT_IMS/include/nse_qdr -I${DNET}/npe/IDT_IMS/include/IXP2XXX -I${DNET}/npe/IDT_IMS/services -I. -I${ONL}/daemons/ixp

TESTINCLUDES = ${WUIXP_USRINC} -I. -I${ONL}/daemons/ixp

LIBS = ${DNET}/npe/IDT_IMS/bin/IMSLIB_IIPC_QDR_LUXSCALE_IXP2800/IdtServicesLib.o -L${DNET}/npe/tcamLib/${OBJDIR} -ltcamLib -L${WUSRC}/wulib/${OBJDIR} -lwu -lm -L${WUSRC}/wuPP/${OBJDIR} -lwuPP

EXEC = $(OBJDIR)/tcamd
OBJS = $(OBJDIR)/tcamd_main.o \
       $(OBJDIR)/tcamd_ops.o  \
       $(OBJDIR)/tcamd_util.o  \
       $(OBJDIR)/tcamd_logfile.o

TESTEXEC = $(OBJDIR)/test
TESTOBJS = $(OBJDIR)/test.o

CXX=${PREFIX}g++

all: ${OBJDIR} ${OBJS} ${EXEC} ${TESTOBJS} ${TESTEXEC}

install: ${EXEC} ${TESTEXEC}
	scp ${EXEC} root@onlsrv:/users/onl/ixp/npr_files
	scp ${TESTEXEC} root@onlsrv:/export/ixp_files/ixp/bin/test_utils

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(EXEC) : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${CXXFLAGS} ${OBJS} ${LIBS}
	chmod 0755 $@

$(TESTOBJS) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${TESTINCLUDES} -o $@ -c $<

$(TESTEXEC) : ${OBJDIR}/% : ${TESTOBJS}
	${CXX} -o $@ ${CXXFLAGS} ${TESTOBJS}
	chmod 0755 $@

clean:
	rm -f ${OBJDIR}/*
