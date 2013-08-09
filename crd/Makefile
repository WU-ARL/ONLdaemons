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
INCLUDES = -I. -I../shared -I/usr/include/mysql -I../database/include -I/opt/gurobi300/linux64/include

#LIBS = -lrt -lpthread -L../shared/${OBJDIR} -lONLshared -L../database/lib/${OBJDIR} -lonldb -L/usr/lib64/mysql -lmysqlclient_r -lmysqlpp -L/opt/gurobi300/linux64/lib -lgurobi_c++ -lgurobi30 -lssl
#LIBS = -lrt -lpthread -L../shared/${OBJDIR} -lONLshared -L../database/lib/${OBJDIR} -lonldb /usr/lib64/mysql/libmysqlclient_r.so /usr/lib64/libmysqlpp.so -L/opt/gurobi300/linux64/lib -lgurobi_c++ -lgurobi30 -lssl
#LIBS = /usr/lib64/librt.a -lpthread -L../shared/${OBJDIR} -lONLshared -L../database/lib/${OBJDIR} -lonldb -L/usr/lib64/mysql -lmysqlclient_r -lmysqlpp -L/opt/gurobi300/linux64/lib -lgurobi_c++ -lgurobi30 /usr/lib64/libssl.a /usr/lib64/libcrypto.a -lz -lkrb5 -lk5crypto -ldl
LIBS = -lrt -lpthread -L../shared/${OBJDIR} -lONLshared -L../database/lib/${OBJDIR} -lonldb -lmysqlclient_r -lmysqlpp -lssl -lz -lkrb5 -lk5crypto -ldl

EXEC = $(OBJDIR)/onlcrd
OBJS = $(OBJDIR)/onlcrd_component.o \
       $(OBJDIR)/onlcrd_reservation.o \
       $(OBJDIR)/onlcrd_session_sharing.o \
       $(OBJDIR)/onlcrd_session.o \
       $(OBJDIR)/onlcrd_session_manager.o \
       $(OBJDIR)/onlcrd_responses.o \
       $(OBJDIR)/onlcrd_requests.o \
       $(OBJDIR)/onlcrd_main.o

all: ${OBJDIR} ${OBJS} ${EXEC}

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.cc
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(EXEC) : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${OBJS} ${LIBS}
	chmod 0755 $@

install:
	scp ${EXEC} root@onlsrv:/etc/ONL/bin/onlcrd.move_to_something_meaningful

clean:
	rm -f ${OBJDIR}/*
