#
# $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/Makefile.inc,v $
# $Author: fredk $
# $Date: 2007/02/14 03:10:10 $
# $Revision: 1.23 $
#
# Author: Fred Kuhns
#         fredk@arl.wustl.edu
# Organization: Applied Research Laboratory
#               Washington UNiversity in St. Louis
# 
# Define veriables/macros for using the wusrc libraries (wulib, wuPP and
# wuklib
#
##
ifdef WUARL
include $(WUARL)/Makefile.inc
else
include $(WUSRC)/Makefile.cse
endif

## ----------------------- The c library ---------------------------
WULIB       = wu
WULIBDIR    = ${WUSRC}/wulib
WULIBNAME   = lib$(WULIB).a
WULIBOBJDIR = $(WULIBDIR)/$(OBJDIR)
WULIBOBJ    = $(WULIBOBJDIR)/$(WULIBNAME)
WULIBS      = -L${WULIBOBJDIR} -l${WULIB}
WUCLIBS     = -L${WULIBOBJDIR} -l${WULIB} -lm

## ----------------------- The c++ library ---------------------------
WUPLUS      = wuPP
WUPLUSDIR   = ${WUSRC}/wuPP
WUPLUSNAME  = lib$(WUPLUS).a
WUPLUSOBJ   = $(WUPLUSDIR)/$(OBJDIR)/$(WUPLUSNAME)
WUCXXLIBS   = -L$(WUPLUSDIR)/$(OBJDIR) -l$(WUPLUS)

## -------------------- Kernel version of the c library --------------
WUKLIB      = wuk
WUKLIBNAME  = lib$(WUKLIB).a
WULIBKOBJ   = $(WULIBOBJDIR)/$(WUKLIBNAME)
WUKLIBS     = -L${WULIBOBJDIR} -l${WUKLIB}

## ------------------- Command Lib ----------------------------------
WUCMDLIB    = cmd
WUCMDDIR    = ${WUSRC}/cmd
WUCMDNAME   = lib${WUCMDLIB}.a
WUCMDOBJ    = ${WUCMDDIR}/$(OBJDIR)/${WUCMDNAME}
WUCMDLIBS   = -L${WUCMDDIR}/${OBJDIR} -l${WUCMDLIB}

## pick up altenate spelling for GNUFLAGS from $WUARL/Makefile.inc
GNUFLGS := ${GNUFLAGS}

## Pick up this dorectory for finding include files.
WUINC += -I${WUSRC}

define mk-objdir
  @echo
  @echo " *** Creating Binary Directory $(OBJDIR)"
  @echo
  @mkdir $(OBJDIR)
endef

define mk-cobjs
  @echo -e "\n--- Updating object file $@ (using tool prefix = $(PREFIX)) ---\n"
  ${CC} ${CFLAGS} ${INCLUDES} -o $@ -c $<
endef

define mk-cmd
  @echo -e "\n--- Building command $@ ---\n"
  ${CC} ${CFLAGS} -o $@ $@.o ${LIBS}
  chmod 0755 $@
endef

define mk-depend
  @echo -e "\n--- Defining dependencies ---\n"
  makedepend makedepend -v -m -p$(OBJDIR)/ $(CFLAGS) $(INCLUDES) $(SRCS)
endef

define mk-clean
  @echo -e "\n--- Cleaning Object directory ---\n"
  /bin/rm -f $(CMDS) $(OBJS) core
endef

define mk-realclean
  @echo -e "\n--- Really cleaning up ... ---\n"
  /bin/rm -f *~
  /bin/rm -fR $(OBJDIR)
endef

ifeq (${WUTARGET}, ixp)
  include ${WUSRC}/Makefile.ixp
endif
