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

include ../Makefile.inc

DEBUG = -g

CXXFLAGS = ${DEBUG} -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual 

LOCAL_LIB = $(OBJDIR)/libONLshared.a

OBJS =  $(OBJDIR)/util.o \
        $(OBJDIR)/byte_buffer.o \
        $(OBJDIR)/rendezvous.o \
        $(OBJDIR)/message.o \
        $(OBJDIR)/dispatcher.o \
        $(OBJDIR)/nccp_connection.o \
        $(OBJDIR)/onl_types.o \
        $(OBJDIR)/onl_requests.o \
        $(OBJDIR)/onl_responses.o \
        $(OBJDIR)/switch_types.o \
        $(OBJDIR)/switch_requests.o \
        $(OBJDIR)/switch_responses.o

all: ${OBJDIR} ${OBJS} ${LOCAL_LIB} 

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.cc
	${CXX} ${CXXFLAGS} -o $@ -c $<
 
$(LOCAL_LIB) : ${OBJDIR}/% : ${OBJS}
	$(AR) rc $@ ${OBJS}
	$(RANLIB) $@

clean:
	rm -f ${OBJDIR}/*
