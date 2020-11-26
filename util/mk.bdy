# "make all" is the standard thing to do to make the utility programs.
# "make install" will install them in /usr/local/bin
# "make clean" to clean up the object code
#
# Invoking makefile needs to define:
#
# INCLDIR     for STREAMS includes
# OPT         C compiler options
# SRC         where the source code is for utilities being compiled here
# HEAD        where the STREAMS head code is
# BINDIR      where to install the utilities
#
CC	= cc -I$(INCLDIR) $(OPT)

PGMS	= strconf polltst streams


all:	$(PGMS)

install:	$(PGMS)
	cp $(PGMS) $(BINDIR)

clean:	
	rm -f *.o *.a $(PGMS)

#
# A utility to test the ability to poll both STREAMS and non-STREAMS
# files.
#
polltst:	polltst.o
	$(CC) -o polltst polltst.o $(XLIBS)

polltst.o:	$(SRC)/polltst.c
	$(CC) -c $(SRC)/polltst.c

#
# A utility program to process the STREAMS config file.  Used to
# specifiy which drivers are present.
#
strconf:	strconf.o
	$(CC)	-o strconf strconf.o $(XLIBS)

strconf.o:	$(SRC)/strconf.c
	$(CC)	-c $(SRC)/strconf.c

#
# A utility program to print streams statistics, set debug mask, etc
#
streams:	streams.o ustats.o
	$(CC) -o $@ streams.o ustats.o $(XLIBS)

streams.o:	$(SRC)/streams.c 
	$(CC) -c -U__KERNEL__ $(SRC)/streams.c

ustats.o:	$(HEAD)/stats.c 
	-rm -f ustats.c
	cp $(HEAD)/stats.c ustats.c
	$(CC) -c -U__KERNEL__ -DMEMPRINT -g ustats.c
	-rm -f ustats.c
