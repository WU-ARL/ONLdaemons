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

CFLAGS = -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual -Wno-ignored-qualifiers
INCLUDES = -I/usr/include/mysql
DEBUG = -g
CC     = g++
AR     = /usr/bin/ar
RANLIB = /usr/bin/ranlib

ONLDB_EXT_INC = onl_database.h onldb_resp.h topology.h onldb.h onltempdb.h

ONLDBSRC = onldb_resp.cc topology.cc onldb.cc onltempdb.cc
ONLDBOBJS = $(addprefix $(OBJDIR)/, $(patsubst %.cc,%.o,$(ONLDBSRC)))
ONLDBLIB = $(OBJDIR)/libonldb.a

all: $(OBJDIR) $(ONLDBLIB)

$(ONLDBOBJS) : $(OBJDIR)/%.o : %.cc
	${CC} ${DEBUG} ${CFLAGS} ${INCLUDES} -o $@ -c $<

$(ONLDBLIB) : $(OBJDIR)/% : $(ONLDBOBJS)
	$(AR) rc $@ $(ONLDBOBJS)
	$(RANLIB) $@

$(OBJDIR) :
	mkdir $(OBJDIR)

install: $(ONLDBLIB) $(ONLDB_EXT_INC)
	mkdir -p ../lib/$(OBJDIR)
	cp $(ONLDBLIB) ../lib/$(OBJDIR)
	chmod 600 ../lib/$(ONLDBLIB)
	mkdir -p ../include
	cp $(ONLDB_EXT_INC) ../include

clean:
	/bin/rm -f $(ONLDBLIB) $(ONLDBOBJS)
