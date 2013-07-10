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
