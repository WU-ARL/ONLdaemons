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

DEBUG = -g

CXXFLAGS = ${DEBUG} -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual 
INCLUDES = -I. -I../shared

LIBS = -lrt -pthread -L../shared/${OBJDIR} -lONLshared -lrt

EXEC = $(OBJDIR)/onlvpnbased
OBJS = $(OBJDIR)/onlvpnbased_userdata.o \
       $(OBJDIR)/onlvpnbased_responses.o \
       $(OBJDIR)/onlvpnbased_requests.o \
       $(OBJDIR)/onlvpnbased_session.o \
       $(OBJDIR)/onlvpnbased_session_manager.o \
       $(OBJDIR)/onlvpnbased_main.o

all: ${OBJDIR} ${OBJS} ${EXEC}

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.cc
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(EXEC) : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${OBJS} ${LIBS}
	chmod 0755 $@

ifeq ($(OBJDIR), Linux26.32)
install:
	scp ${EXEC} root@onlsrv:/export/keeboot_files/pc2core/etc/ONL/bin
	scp ${EXEC} root@onlsrv:/export/keeboot_files/netfpga/etc/ONL/bin
else
install:
	scp ${EXEC} root@onlsrv:/export/keeboot_files/pc1core/etc/ONL/bin
	scp ${EXEC} root@onlsrv:/export/keeboot_files/pc8core/etc/ONL/bin
endif

clean:
	rm -f ${OBJDIR}/*
