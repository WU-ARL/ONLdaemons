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

include ../../Makefile.inc

CFLAGS = -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual
INCLUDES = -I/usr/include/mysql -I../include 
#LIBS = -lpthread -L/usr/lib64/mysql -L ../lib/$(OBJDIR) -lonldb -lmysqlclient_r -lmysqlpp -lssl
#LIBS = /usr/lib64/librt.a -lpthread -L../lib/${OBJDIR} -lonldb -L/usr/lib64/mysql -lmysqlclient_r -lmysqlpp /usr/lib64/libssl.a /usr/lib64/libcrypto.a -lz -lkrb5 -lk5crypto -ldl /usr/lib64/libstdc++.so.6
LIBS = -lpthread -L../lib/${OBJDIR} -lonldb -L/usr/lib -lmysqlclient -lmysqlpp  -lstdc++
#LIBS = -lpthread -L../lib/${OBJDIR} -lonldb -L/usr/lib64/mysql -lmysqlclient_r -lmysqlpp  -lstdc++

DEBUG = -g
#CC     = g++

DBCMDSSRC = get_state.cc set_state.cc authenticate_user.cc make_reservation.cc assign_resources.cc put_in_testing.cc remove_from_testing.cc extend_repair.cc extend_testing.cc reserve_all.cc get_switch_list.cc
DBCMDSOBJS = $(addprefix $(OBJDIR)/, $(patsubst %.cc,%.o,$(DBCMDSSRC)))
DBCMDS = $(addprefix $(OBJDIR)/, $(basename $(DBCMDSSRC)))

all: $(OBJDIR) $(DBCMDS)

$(DBCMDSOBJS) : $(OBJDIR)/%.o : %.cc
	${CC} ${DEBUG} ${CFLAGS} ${INCLUDES} -o $@ -c $<

$(DBCMDS) : $(OBJDIR)/% : $(ONLDBLIB) $(ONLTEMPDBLIB) $(OBJDIR)/%.o
	$(CC) -o $@ $@.o $(LIBS)

$(OBJDIR) :
	mkdir $(OBJDIR)

install: $(DBCMDS)
	mkdir -p ../bin/$(OBJDIR)
	cp $(DBCMDS) ../bin/$(OBJDIR)

onlsrv: $(OBJDIR)/set_state $(OBJDIR)/put_in_testing $(OBJDIR)/remove_from_testing $(OBJDIR)/extend_testing $(OBJDIR)/extend_repair $(OBJDIR)/reserve_all
	scp $(OBJDIR)/set_state $(OBJDIR)/put_in_testing $(OBJDIR)/remove_from_testing $(OBJDIR)/extend_testing $(OBJDIR)/extend_repair $(OBJDIR)/reserve_all root@onlsrv:/usr/testbed/www/scripts

clean:
	/bin/rm -f $(DBCMDS) $(DBCMDSOBJS)
