#define	inline				/* make disappear */

#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/LiS/loop.h>		/* an odd place for this file */

#include <unistd.h>
#include <time.h>
#ifdef LINUX
#include <stdio.h>
#include <fcntl.h>
#include "linuxio.h"
#else
#include "usrio.h"
#endif

/************************************************************************
*                      File Names                                       *
************************************************************************/
#ifdef LINUX
#define	LOOP_1		"/dev/loop.1"
#define	LOOP_2		"/dev/loop.2"
#define LOOP_CLONE	"/dev/loop_clone"
#else
#define	LOOP_1		"loop.1"
#define	LOOP_2		"loop.2"
#define LOOP_CLONE	"loop_clone"
#endif


/************************************************************************
*                           Storage                                     *
************************************************************************/

char		buf[1000] ;		/* general purpose */
char		rdbuf[1000] ;		/* for reading */
long		iter_cnt = 100000 ;	/* default iteration count */

extern void make_nodes(void) ;

/************************************************************************
*                           register_drivers                            *
*************************************************************************
*									*
* Register the drivers that we are going to use in the test with	*
* streams.								*
*									*
************************************************************************/
void	register_drivers(void)
{
#ifndef LINUX
    port_init() ;			/* stream head init routine */
#endif

} /* register_drivers */


/************************************************************************
*                           timing_test                                 *
*************************************************************************
*									*
* Time reading and writing messages through streams.			*
*									*
************************************************************************/
void	timing_test(void)
{
    time_t		time_on ;
    time_t		et ;
    long		i ;
    int			fd1 ;
    int			fd2 ;
    int			rslt ;
    int			lgth ;
    int			arg ;
    struct strioctl	ioc ;

    printf("\nBegin timing test\n") ;

    fd1 = user_open(LOOP_1, O_RDWR, 0) ;
    if (fd1 < 0)
    {
	printf("loop.1: %s\n", strerror(-fd1)) ;
	return ;
    }

    fd2 = user_open(LOOP_2, O_RDWR, 0) ;
    if (fd2 < 0)
    {
	printf("loop.2: %s\n", strerror(-fd2)) ;
	return ;
    }

    ioc.ic_cmd 	  = LOOP_SET ;
    ioc.ic_timout = 10 ;
    ioc.ic_len	  = sizeof(int) ;
    ioc.ic_dp	  = (char *) &arg ;

    arg = 2 ;
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	printf("loop.1: ioctl LOOP_SET: %s\n", strerror(-rslt)) ;
    }

    strcpy(buf, "Data to send down the file") ;
    lgth = strlen(buf) ;

    printf("Time loopback driver:  write %d bytes, read and service queue: ",
    		lgth) ;
    fflush(stdout) ;

    sync() ;				/* do file sync now rather than
    					 * in the middle of the test.
    					 */
    time_on = time(NULL) ;

    for (i = 0; i < iter_cnt; i++)
    {
	rslt = user_write(fd1, buf, lgth) ;

	if (rslt != lgth)
	{
	    if (rslt < 0)
		printf("loop.1: write: %s\n", strerror(-rslt)) ;
	    else
		printf("loop.1:  write returned %d, expected %d\n", rslt, lgth) ;

	    break ;
	}

	rslt = user_read(fd2, rdbuf, lgth);

	if (rslt != lgth)
	{
	    if (rslt < 0)
		printf("loop.2: read: %s\n", strerror(-rslt)) ;
	    else
		printf("loop.2:  read returned %d, expected %d\n", rslt, lgth) ;
	    break ;
	}
    }

    et = (time(NULL) - time_on) * 1000000 ;	/* time in usecs */

    printf("%ld micro-secs\n", et/iter_cnt) ;



    ioc.ic_cmd 	  = LOOP_PUTNXT ;		/* use putnxt rather then svcq */
    ioc.ic_len	  = 0 ;
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	printf("loop.1: ioctl LOOP_PUTNXT: %s\n", strerror(-rslt)) ;
    }

    printf("Time loopback driver:  write %d bytes, read w/o service queue: ",
    		lgth) ;
    fflush(stdout) ;
    sync() ;
    time_on = time(NULL) ;

    for (i = 0; i < iter_cnt; i++)
    {
	rslt = user_write(fd1, buf, lgth) ;

	if (rslt != lgth)
	{
	    if (rslt < 0)
		printf("loop.1: write: %s\n", strerror(-rslt)) ;
	    else
		printf("loop.1:  write returned %d, expected %d\n", rslt, lgth) ;

	    break ;
	}

	rslt = user_read(fd2, rdbuf, lgth);

	if (rslt != lgth)
	{
	    if (rslt < 0)
		printf("loop.2: read: %s\n", strerror(-rslt)) ;
	    else
		printf("loop.2:  read returned %d, expected %d\n", rslt, lgth) ;
	    break ;
	}
    }

    et = (time(NULL) - time_on) * 1000000 ;	/* time in usecs */

    printf("%ld micro-secs\n", et/iter_cnt) ;



    user_close(fd1) ;
    user_close(fd2) ;

} /* timing_test */

/************************************************************************
*                          set_debug_mask                               *
*************************************************************************
*									*
* Use stream ioctl to set the debug mask for streams.			*
*									*
************************************************************************/
void	set_debug_mask(long msk)
{
    int		fd ;
    int		rslt ;

    fd = user_open(LOOP_1, 0, 0) ;
    if (fd < 0)
    {
	printf("loop.1: %s\n", strerror(-fd)) ;
	return ;
    }

    rslt = user_ioctl(fd, I_LIS_SDBGMSK, msk) ;
    if (rslt < 0)
    {
	printf("loop.1: I_LIS_SDBGMSK: %s\n", strerror(-rslt)) ;
	return ;
    }

    printf("\nStreams debug mask set to 0x%08lx\n", msk) ;

    user_close(fd) ;

} /* set_debug_mask */

/************************************************************************
*                              main                                     *
************************************************************************/
void main(int argc, char **argv)
{
    if (argc > 1)
	sscanf(argv[1], "%ld", &iter_cnt) ;

#ifndef LINUX
    register_drivers() ;
    make_nodes() ;
#endif

    printf("Using safe constructs and message tracing\n") ;
    set_debug_mask(0x30000L) ;
    /* set_debug_mask(0x0FFFFFFF) ; */
    timing_test() ;

    printf("Not using safe constructs or message tracing\n") ;
    set_debug_mask(0L) ;
    timing_test() ;
}
