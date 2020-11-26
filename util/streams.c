#include <sys/types.h>
#undef GCOM_OPEN
#include <sys/stropts.h>
#include <sys/LiS/stats.h>
#ifdef QNX
#include <unix.h>
#define	LOOP_CLONE	"/dev/gcom/loop_clone"
#else
#define	LOOP_CLONE	"/dev/loop_clone"
#endif
#include <sys/ioctl.h>
#include <unistd.h>

unsigned long strstats[STRMAXSTAT][4]; /* the stats */

void print_debug_bits(void)
{
	printf("DEBUG_OPEN		0x00000001\n");
	printf("DEBUG_CLOSE		0x00000002\n");
	printf("DEBUG_READ		0x00000004\n");
	printf("DEBUG_WRITE		0x00000008\n");
	printf("DEBUG_IOCTL		0x00000010\n");
	printf("DEBUG_PUTNEXT		0x00000020\n");
	printf("DEBUG_STRRPUT		0x00000040\n");
	printf("DEBUG_SIG		0x00000080\n");
	printf("DEBUG_PUTMSG		0x00000100\n");
	printf("DEBUG_GETMSG		0x00000200\n");
	printf("DEBUG_POLL		0x00000400\n");
	printf("DEBUG_LINK		0x00000800\n");
	printf("DEBUG_SAFE		0x00010000\n");
	printf("DEBUG_TRCE_MSG		0x00020000\n");
	printf("DEBUG_CLEAN_MSG		0x00040000\n");
	printf("DEBUG_MP_ALLOC		0x00100000\n");
	printf("DEBUG_MP_FREEMSG	0x00200000\n");
	printf("DEBUG_MALLOC		0x00400000\n");
	printf("DEBUG_MONITOR_MEM	0x00800000\n");
	printf("DEBUG_DMP_QUEUE		0x01000000\n");
	printf("DEBUG_DMP_MBLK		0x02000000\n");
	printf("DEBUG_DMP_DBLK		0x04000000\n");
	printf("DEBUG_DMP_STRHD		0x08000000\n");
	printf("DEBUG_ADDRS		0x80000000\n");

} /* print_debug_bits */

void main( int argc, char *argv[])
{
    int		fd ;
    int		rslt ;
    int		c;
    unsigned long debug_mask;
    int		mflag = 0;
    int		dflag = 0;
    int		sflag = 0;
    extern char *optarg;

    while(( c = getopt(argc, argv, "smhHd:")) != -1)
    {
	switch (c)
	{
	    case 'd':
	        dflag = 1;
	        if ( sscanf(optarg, "%x", &debug_mask) != 1 )
	        {
	            printf("need hex argument for -d\n");
	            exit(1);
	        }
		break;

	    case 's':
	        sflag = 1;
	        break;

	    case 'm':
	        mflag = 1;
	        break;

	    case 'h':
	    case 'H':
	    case '?':
	    default:
		printf("%s: command line argument error.\n", argv[0]);
		printf("	-d debug mask (in hex)\n");
		printf("	-s print streams memory stats\n");
		printf("	-m print memory allocation to log file\n");

		if (c == 'H')
		    print_debug_bits() ;

		exit(1);
	}
    }

    if ( !mflag && !dflag && !sflag )
    {
	printf("%s: command line argument error.\n", argv[0]);
	printf("	-d debug mask (in hex)\n");
	printf("	-s print streams memory stats\n");
	printf("	-m print memory allocation to log file\n");
	exit(1);
    }

    fd = open(LOOP_CLONE, 0, 0) ;
    if (fd < 0)
    {
	printf(LOOP_CLONE ": %s\n", strerror(-fd)) ;
	exit(1) ;
    }

    if ( dflag )
    {
	rslt = ioctl(fd, I_LIS_SDBGMSK, debug_mask) ;
	if (rslt < 0)
	{
	    printf(LOOP_CLONE ": I_LIS_SDBGMSK: %s\n", strerror(-rslt)) ;
	    exit(1) ;
	}
    }

    if ( sflag )
    {
	/* printf("sizeof strstats: %d\n", sizeof(strstats)); */
	rslt = ioctl(fd, I_LIS_GETSTATS, strstats) ;
	if (rslt < 0)
	{
	    printf(LOOP_CLONE ": I_LIS_GETSTATS: %s\n", strerror(-rslt)) ;
	    exit(1) ;
	}
	LisShowStrStats(strstats);
    }

    if ( mflag )
    {
	rslt = ioctl(fd, I_LIS_PRNTMEM, 0) ;
	if (rslt < 0)
	{
	    printf(LOOP_CLONE ": I_LIS_PRNTMEM: %s\n", strerror(-rslt)) ;
	    exit(1) ;
	}
#ifdef QNX
	printf("The memory dump is in the /usr/lib/gcom/streams.log file\n");
#endif
    }
    close(fd) ;

} /* set_debug_mask */
