#
# Location of Lis stuff on Gcom's server
#
SVR	= /rsys/linux/LiS

#
# Subdirectories in release directory
#
DIRS	= 	util \
		util/linux \
		include \
		include/sys \
		include/sys/LiS \
		head

#
# Source code files that go into the release archive
#
SOURCES	=	util/mk.bdy\
		util/polltst.c\
		util/strtst.c\
		util/timetst.c\
		util/streams.c\
		util/strconf.c\
		util/linux/Makefile\
		include/sys/poll.h \
		include/sys/LiS/poll.h \
		include/sys/LiS/stats.h \
		include/sys/LiS/share.h \
		include/sys/LiS/linux-mdep.h \
		include/sys/strconfig.h \
		include/sys/strport.h \
		include/sys/stropts.h \
		include/sys/LiS/loop.h \
		head/stats.c \
		LSM\
		scope\
		Makefile\
		LICENSE\
		LICENSE.README\
		README\
		README.PATCH\
		README.TEST\
		README.DEBUG\
		README.INCL\
		README.DRVRS\
		VOLUNTEERS

#
# Make streams utility programs and install them in /usr/local/bin
#
# This is the main target of this makefile.  It is used after the
# archive has been un-tarred.  See README for instructions.
#
utils:
	cd util/linux; make all install

##############################################################################
#									     #
# Do not make any targets below here.  These are for putting the release     #
# together and are meaningless on your system.				     #
#									     #
##############################################################################

#
# Do all the things that put a release together
#
all:	dirs_made source_update patches.streams

#
# Make directories for source code to reside in
#
dirs_made:	$(DIRS)
	touch dirs_made

#
# Get sources (except kernel patches) from server
#
source_update:	$(SOURCES)


#
# Get patches from neighboring directory
#
patches.streams:	../linux/patches.streams
	cp $? patches.streams

clean:		
	-rm -f $(SOURCES)
	-rm -r $(DIRS)
	-rm dirs_made patches.streams

#
# Source files
#
util/mk.bdy:		$(SVR)/util/mk.bdy; cp $? $@
util/polltst.c:		$(SVR)/util/polltst.c; cp $? $@
util/timetst.c:		$(SVR)/util/timetst.c; cp $? $@
util/strtst.c:		$(SVR)/util/strtst.c; cp $? $@
util/streams.c:		$(SVR)/util/streams.c; cp $? $@
util/strconf.c:		$(SVR)/util/strconf.c; cp $? $@
util/linux/Makefile:	$(SVR)/util/linux/Makefile; cp $? $@
include/sys/poll.h:	$(SVR)/include/sys/poll.h; cp $? $@
include/sys/stropts.h:	$(SVR)/include/sys/stropts.h; cp $? $@
include/sys/strport.h:	$(SVR)/include/sys/strport.h; cp $? $@
include/sys/LiS/loop.h:	$(SVR)/include/sys/LiS/loop.h; cp $? $@
include/sys/LiS/poll.h:	$(SVR)/include/sys/LiS/poll.h; cp $? $@
include/sys/LiS/stats.h:	$(SVR)/include/sys/LiS/stats.h; cp $? $@
include/sys/LiS/share.h:	$(SVR)/include/sys/LiS/share.h; cp $? $@
include/sys/LiS/linux-mdep.h:	$(SVR)/include/sys/LiS/linux-mdep.h; cp $? $@
include/sys/strconfig.h:	$(SVR)/include/sys/strconfig.h; cp $? $@
head/stats.c:		$(SVR)/head/stats.c; cp $? $@
LSM:			$(SVR)/LSM; cp $? $@
scope:			$(SVR)/scope; cp $? $@
Makefile:		$(SVR)/Makefile; cp $? $@
LICENSE:		$(SVR)/LICENSE; cp $? $@
LICENSE.README:		$(SVR)/LICENSE.README; cp $? $@
README.PATCH:		$(SVR)/README.PATCH; cp $? $@
README.TEST:		$(SVR)/README.TEST; cp $? $@
README.DEBUG:		$(SVR)/README.DEBUG; cp $? $@
README.INCL:		$(SVR)/README.INCL; cp $? $@
README.DRVRS:		$(SVR)/README.DRVRS; cp $? $@
README:			$(SVR)/README; cp $? $@
VOLUNTEERS:		$(SVR)/VOLUNTEERS; cp $? $@

#
# Directories
#
util:
	mkdir $@
util/linux:
	mkdir $@
include:
	mkdir $@
include/sys:
	mkdir $@
include/sys/LiS:
	mkdir $@
head:
	mkdir $@

#
# Make the release archive in the directory above us
#
archive:
	(cd ..;				\
	tar cvf LiS-2.0.24.tar LiS;	\
	gzip -9 LiS-2.0.24.tar)

