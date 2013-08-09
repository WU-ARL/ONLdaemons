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

include ../Makefile.inc
include ../../Makefile.inc

DEBUG = -g

CXXFLAGS = ${DEBUG} -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual ${WUIXP_CXXFLAGS} -DWU_SRAM_RING_PUT -DWU_RING_OP_BASE

INCLUDES = -I. -I.. -I../../shared ${WUIXP_USRINC} -I${IXA_ME_TOOLS}/include -I${IXA_ME_TOOLS}/XSC_CoreLibs -I${ATCA7010_DIR}/include -I${ONL}/daemons/ixp

LIBS = ${WUIXP_LDFLAGS} -lrt -pthread -L../../shared/${OBJDIR} -lONLshared -L${ATCA7010_DIR}/api/arm -lppm ${IXA_ME_TOOLS}/lib_linux_be/halMev2.a ${IXA_ME_TOOLS}/lib_linux_be/libossl.a ${IXA_ME_TOOLS}/lib_linux_be/uclo.a ${IXA_ME_TOOLS}/lib_linux_be/utils.a

EXEC = $(OBJDIR)/nprd
OBJS = $(OBJDIR)/nprd_arp.o \
       $(OBJDIR)/nprd_ipv4.o \
       $(OBJDIR)/nprd_plugincontrol.o \
       $(OBJDIR)/nprd_datapathconn.o \
       $(OBJDIR)/nprd_datapathmessage.o \
       $(OBJDIR)/nprd_pluginconn.o \
       $(OBJDIR)/nprd_pluginmessage.o \
       $(OBJDIR)/nprd_monitor.o \
       $(OBJDIR)/nprd_configuration.o \
       $(OBJDIR)/nprd_responses.o \
       $(OBJDIR)/nprd_requests.o \
       $(OBJDIR)/nprd_main.o

all: ${OBJDIR} ${OBJS} ${EXEC}

install: ${EXEC}
	scp ${EXEC} root@onlsrv:/users/onl/ixp/npr_files

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.cc
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(EXEC) : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${OBJS} ${LIBS}
	chmod 0755 $@

clean:
	rm -f ${OBJDIR}/*
