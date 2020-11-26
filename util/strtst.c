#define	inline			/* make disappear */


#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>
#ifdef __KERNEL__			/* directly coupled to usrio.h */
#include <sys/stream.h>
#endif
#include <sys/stropts.h>
#include <sys/LiS/loop.h>		/* an odd place for this file */
#include <sys/LiS/minimux.h>		/* an odd place for this file */


#include <string.h>
/* #include <ioctl.h> */
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#ifdef LINUX
#include <stdio.h>
#include <fcntl.h>
#include "linuxio.h"
#else
# if defined(SCO)
# include <prototypes.h>
# endif
#include "usrio.h"
#endif

/************************************************************************
*                      File Names                                       *
************************************************************************/
#ifdef LINUX
#define	LOOP_1		"/dev/loop.1"
#define	LOOP_2		"/dev/loop.2"
#define LOOP_CLONE	"/dev/loop_clone"
#define MUX_CLONE	"/dev/mux_clone"
#define	NPRINTK		"/dev/printk"
#else
#define	LOOP_1		"loop.1"
#define	LOOP_2		"loop.2"
#define LOOP_CLONE	"loop_clone"
#define MUX_CLONE	"mux_clone"
#define	NPRINTK		"printk"
#endif


/************************************************************************
*                           Storage                                     *
************************************************************************/

char		buf[1000] ;		/* general purpose */
char		ctlbuf[1000] ;		/* control messages */
char		rdbuf[1000] ;		/* for reading */
char		rdctlbuf[1000] ;	/* control messages */
int		printk_fd = -1 ;	/* file descr for printk */

/*
 * For I_LIST
 */
struct str_mlist	mod_names[10] ;
struct str_list		mod_list = {10, mod_names} ;

/*
 * For getmsg, putmsg
 */
struct strbuf	wr_ctl = {0, 0, ctlbuf} ;
struct strbuf	wr_dta = {0, 0, buf} ;
struct strbuf	rd_ctl = {0, 0, rdctlbuf} ;
struct strbuf	rd_dta = {0, 0, rdbuf} ;
struct strpeek	pk_str = {
			      {0, 0, rdctlbuf},	/* ctlbuf */
			      {0, 0, rdbuf},	/* databuf */
			      0			/* flags */
			  } ;

extern void make_nodes(void) ;

#ifdef DIRECT_USER		/* tie-in to cmn_err */
typedef      void  (*lis_print_trace_t)   (char *bfrp) ;
extern       lis_print_trace_t      lis_print_trace;
#endif

/************************************************************************
*                           Dummies                                     *
*************************************************************************
*									*
* Dummy routines to satisfy the linker until system calls are		*
* implemented.								*
*									*
************************************************************************/

#ifdef LINUX


long	lis_mem_alloced ;


#endif

/************************************************************************
*                         msg_to_syslog                                 *
*************************************************************************
*									*
* Print the given message to syslog.					*
*									*
************************************************************************/
#if 0				/* superceded by printk mechanism */
void
msg_to_syslog(char *msg)
{
#ifdef LINUX
    static int		initialized ;

    if (!initialized)
    {
	openlog("strtst", LOG_NDELAY, LOG_SYSLOG) ;
	initialized = 1 ;
    }

    syslog(LOG_WARNING, msg) ;

#else
    (void) msg ;
#endif
} /* msg_to_syslog */
#endif
/************************************************************************
*                           print                                       *
*************************************************************************
*									*
* Like printf only it also arranges to print to syslog for Linux.	*
*									*
************************************************************************/
void
print(char *fmt, ...)
{
    char	 bfr[2048];
    extern int	 vsprintf (char *, const char *, va_list);
    va_list	 args;

    va_start (args, fmt);
    vsprintf (bfr, fmt, args);
    va_end (args);

    printf("%s", bfr) ;				/* write to stdout */
    fflush(stdout) ;

    if (printk_fd >= 0)				/* write to kernel */
	user_write(printk_fd, bfr, strlen(bfr)) ;

} /* print */

/************************************************************************
*                             xit                                       *
*************************************************************************
*									*
* This routine is called to exit the program when a test fails.		*
*									*
************************************************************************/
void	xit(void)
{
    print("\n\n\n");
    print("****************************************************\n");
    print("*                  Test Failed                     *\n");
    print("****************************************************\n\n");

    print("Dump of memory areas in use:\n\n") ;

#ifndef LINUX
    port_print_mem() ;
#endif

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing:\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
#endif
    exit(1) ;

} /* xit */

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

    fd = user_open(LOOP_1, O_RDWR, 0) ;
    if (fd < 0)
    {
	print("loop.1: %s\n", strerror(-fd)) ;
	xit() ;
    }

    rslt = user_ioctl(fd, I_LIS_SDBGMSK, msk) ;
    if (rslt < 0)
    {
	print("loop.1: I_LIS_SDBGMSK: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("\nStreams debug mask set to 0x%08lx\n", msk) ;

    user_close(fd) ;

} /* set_debug_mask */

/************************************************************************
*                           file_size                                   *
*************************************************************************
*									*
* Return the number of bytes in the file or -1 if file does not exist.	*
*									*
************************************************************************/
off_t
file_size(char *file_name)
{
    struct stat		b ;

    if (stat(file_name, &b) < 0) return(-1) ;

    return(b.st_size) ;

} /* file_size */

/************************************************************************
*                          wait_for_logfile 				*
*************************************************************************
*									*
* Wait for the log files to settle down.  This is useful in Linux	*
* or other testing environments in which the test program is running	*
* in user space and the STREAMS code is running in the kernel.  The	*
* STREAMS code writes out voluminous messages to the system log and	*
* we have to let it catch up from time to time or we lose messages.	*
*									*
************************************************************************/
void
wait_for_logfile(char *msg)
{
    off_t	messages_size ;
    off_t	syslog_size ;
    off_t	new_messages_size = -1 ;
    off_t	new_syslog_size = -1 ;
    int		n ;

    if (msg != NULL)
    {
	print("Wait for log file to stabilize: %s ", msg) ;
	fflush(stdout) ;
    }

    messages_size = file_size("/usr/adm/messages") ;
    syslog_size = file_size("/usr/adm/syslog") ;

    for (n = 0;;n++)
    {
	sleep(1) ;
	new_messages_size = file_size("/usr/adm/messages") ;
	new_syslog_size = file_size("/usr/adm/syslog") ;
	if (   new_messages_size == messages_size
	    && new_syslog_size   == syslog_size
	   )
	    break ;

	messages_size = new_messages_size ;
	syslog_size   = new_syslog_size ;
    }

    if (n)
	sync() ;			/* ensure log file integrity */

    if (msg != NULL)
	print("\n") ;

} /* wait_for_logfile */

/************************************************************************
*                             print_mem                                 *
*************************************************************************
*									*
* Issue streams ioctl to print out allocated memory.			*
*									*
************************************************************************/
void	print_mem(void)
{
    int		fd ;
    int		rslt ;

    fd = user_open(LOOP_1, O_RDWR, 0) ;
    if (fd < 0)
    {
	print("loop.1: %s\n", strerror(-fd)) ;
	xit() ;
    }

    print("\n\nBegin dump of in-use memory areas\n\n") ;
    rslt = user_ioctl(fd, I_LIS_PRNTMEM, 0) ;
    if (rslt < 0)
    {
	print("loop.1: I_LIS_PRNTMEM: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("\n\nEnd dump of in-use memory areas\n\n") ;
    user_close(fd) ;

} /* print_mem */

/************************************************************************
*                              print_stream                             *
*************************************************************************
*									*
* Issue the print-stream ioctl on the file.				*
*									*
************************************************************************/
void	print_stream(int fd)
{
    int		rslt ;

    rslt = user_ioctl(fd, I_LIS_PRNTSTRM, 0) ;
    if (rslt < 0)
    {
	print("I_LIS_PRNTSTRM: %s\n", strerror(-rslt)) ;
	xit() ;
    }

} /* print_stream */

/************************************************************************
*                         open_close_test                               *
*************************************************************************
*									*
* Repeatedly open and close the same stream.  This ensures that streams	*
* is not accumulating resources.					*
*									*
************************************************************************/
void	open_close_test(void)
{
    int		fd1 ;
    int		fd2 ;
    int		fd3 ;
    int		i ;

    /*
     * Make this loop iteration large to wring out memory
     * leaks connected with open/close.
     */
    for (i = 1; i <= 1; i++)
    {
	print("\nopen_close_test iteration #%d\n", i) ;

	fd1 = user_open(LOOP_1, O_RDWR, 0) ;
	if (fd1 < 0)
	{
	    print("loop.1: %s\n", strerror(-fd1)) ;
	    break ;
	}

	fd2 = user_open(LOOP_2, O_RDWR, 0) ;
	if (fd2 < 0)
	{
	    print("loop.2: %s\n", strerror(-fd2)) ;
	    break ;
	}

	fd3 = user_open(LOOP_1, O_RDWR, 0) ;
	if (fd3 < 0)
	{
	    print("loop.1 (second open): %s\n", strerror(-fd3)) ;
	    break ;
	}

	user_close(fd1) ;
	user_close(fd2) ;
	user_close(fd3) ;
    }

} /* open_close_test */

/************************************************************************
*                            open_files                                 *
*************************************************************************
*									*
* Standard prologue to various tests.  Open the loop driver and		*
* connect the two streams together with an ioctl.			*
*									*
************************************************************************/
int	open_files(int *fd1, int *fd2)
{
    int			arg ;
    int			rslt ;
    struct strioctl	ioc ;

    *fd1 = user_open(LOOP_1, O_RDWR, 0) ;
    if (*fd1 < 0)
    {
	print("loop.1: %s\n", strerror(-*fd1)) ;
	return(*fd1) ;
    }

    *fd2 = user_open(LOOP_2, O_RDWR, 0) ;
    if (*fd2 < 0)
    {
	print("loop.2: %s\n", strerror(-*fd2)) ;
	user_close(*fd1) ;
	return(*fd2) ;
    }

    ioc.ic_cmd 	  = LOOP_SET ;
    ioc.ic_timout = 10 ;
    ioc.ic_len	  = sizeof(int) ;
    ioc.ic_dp	  = (char *) &arg ;

    arg = 2 ;
    rslt = user_ioctl(*fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop.1: ioctl LOOP_SET: %s\n", strerror(-rslt)) ;
	return(rslt) ;
    }

    return(1) ;

} /* open_files */

/************************************************************************
*                            open_clones                                *
*************************************************************************
*									*
* Open the loop driver as clone devices.				*
*									*
************************************************************************/
int	open_clones(int *fd1, int *fd2)
{
    int			arg ;
    int			rslt ;
    struct strioctl	ioc ;

    *fd1 = user_open(LOOP_CLONE, O_RDWR, 0) ;
    if (*fd1 < 0)
    {
	print("loop_clone.1: %s\n", strerror(-*fd1)) ;
	xit() ;
    }

    *fd2 = user_open(LOOP_CLONE, O_RDWR, 0) ;
    if (*fd2 < 0)
    {
	print("loop_clone.2: %s\n", strerror(-*fd2)) ;
	xit() ;
    }

    ioc.ic_timout = 10 ;
    ioc.ic_len	  = sizeof(int) ;
    ioc.ic_dp	  = (char *) &arg ;

    ioc.ic_cmd 	  = LOOP_GET_DEV ;
    arg = -1 ;
    rslt = user_ioctl(*fd2, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop_clone.2: ioctl LOOP_GET_DEV: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (arg < 0)
    {
	print("loop_clone.2: ioctl LOOP_GET_DEV returned %d\n", arg) ;
	xit() ;
    }

    ioc.ic_cmd 	  = LOOP_SET ;
    rslt = user_ioctl(*fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop_clone.1: ioctl LOOP_SET: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    return(1) ;

} /* open_clones */

/************************************************************************
*                           n_read                                      *
*************************************************************************
*									*
* Do an I_NREAD on the file.  Return negative if error, or the byte	*
* count if successful.							*
*									*
************************************************************************/
int	n_read(int fd)
{
    int		rslt ;
    int		arg ;

    rslt = user_ioctl(fd, I_NREAD, &arg) ;
    if (rslt < 0)
    {
	print("I_NREAD: %s\n", strerror(-rslt)) ;
	return(rslt) ;
    }

    return(arg) ;

} /* n_read */

/************************************************************************
*                             write_data                                *
*************************************************************************
*									*
* Write data and check results.						*
*									*
************************************************************************/
int	write_data(int fd, char *bfr, int cnt)
{
    int		rslt ;

    rslt = user_write(fd, bfr, cnt) ;
    if (rslt < 0)
	print("write_data: %s\n", strerror(-rslt)) ;
    else
    if (rslt != cnt)
	print("write_data: write returned %d, expected %d\n", rslt, cnt) ;

    return(rslt) ;

} /* write_data */

/************************************************************************
*                             put_msg                                   *
*************************************************************************
*									*
* An interface routine to putpmsg().  This one checks return conditions.*
*									*
************************************************************************/
int	put_msg(int fd, struct strbuf *ctlptr,
			struct strbuf *dataptr,
			int band,
			int flags)
{
    int		rslt ;

    rslt = user_putpmsg(fd, ctlptr, dataptr, band, flags) ;
    if (rslt < 0)
	print("put_msg: %s\n", strerror(-rslt)) ;
    else
    if (rslt > 0)
	print("put_msg: putpmsg returned %d, expected <= 0\n", rslt) ;

    return(rslt) ;

} /* put_msg */

/************************************************************************
*                           input_sig                                   *
*************************************************************************
*									*
* This is the signal catcher for the S_INPUT ioctl.  SIGPOLL is the	*
* signal that gets caused.						*
*									*
************************************************************************/
void input_sig(int signo)
{
    print("\ninput_sig:  SIGPOLL (%d) caught\n", signo) ;

} /* input_sig */


/************************************************************************
*                             ioctl_test                                *
*************************************************************************
*									*
* Send an ioctl to the "loop" driver to cross-connect the two streams.	*
*									*
************************************************************************/
void	ioctl_test(void)
{
    int			i ;
    int			fd1 ;
    int			fd2 ;
    int			arg ;
    int			rslt ;
    int			lgth ;
    int			lgth2 ;
    struct strioctl	ioc ;
    struct str_mlist	*mlp ;

    /*
     * Make this loop iteration large to wring out memory
     * leaks connected with ioctl
     */
    for (i = 1; i <= 1; i++)
    {
	print("\nioctl_test iteration #%d\n", i) ;

		/********************************
		*           Open Files          * 
		********************************/

	fd1 = user_open(LOOP_1, O_RDWR, 0) ;
	if (fd1 < 0)
	{
	    print("loop.1: %s\n", strerror(-fd1)) ;
	    xit() ;
	}

	fd2 = user_open(LOOP_2, O_RDWR, 0) ;
	if (fd2 < 0)
	{
	    print("loop.2: %s\n", strerror(-fd2)) ;
	    xit() ;
	}

		/********************************
		*             I_STR             * 
		********************************/

	ioc.ic_cmd 	  = LOOP_SET ;
	ioc.ic_timout 	  = 10 ;
	ioc.ic_len	  = sizeof(int) ;
	ioc.ic_dp	  = (char *) &arg ;

	arg = 2 ;
	rslt = user_ioctl(fd1, I_STR, &ioc) ;
	if (rslt < 0)
	{
	    print("loop.1: ioctl LOOP_SET: %s\n", strerror(-rslt)) ;
	}

	/*
	 * This next one will fail with device busy.  It is rejected
	 * by the loop driver because the previous one set up both
	 * sides of the loopback connection.
	 */
	arg = 1 ;
	rslt = user_ioctl(fd2, I_STR, &ioc) ;
	if (rslt < 0)
	{
	    print("loop.2: ioctl: %s\n", strerror(-rslt)) ;
	}

	user_close(fd1) ;
	user_close(fd2) ;


	fd1 = user_open(LOOP_1, O_RDWR, 0) ;
	if (fd1 < 0)
	{
	    print("loop.1: %s\n", strerror(-fd1)) ;
	    xit() ;
	}

		/********************************
		*             I_PUSH            * 
		********************************/

	print("\nTesting I_PUSH (relay)\n") ;
	rslt = user_ioctl(fd1, I_PUSH, "relay") ;
	if (rslt < 0)
	{
	    print("loop.1: I_PUSH (relay): %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	print_mem() ;

		/********************************
		*             I_LOOK            * 
		********************************/

	print("\nTesting I_LOOK\n") ;
	strcpy(buf,"Nothing at all") ;
	rslt = user_ioctl(fd1, I_LOOK, buf) ;
	if (rslt < 0)
	{
	    print("loop.1: I_LOOK: %s\n", strerror(-fd1)) ;
	    xit() ;
	}

	print("I_LOOK returned \"%s\"\n", buf) ;
	print_mem() ;

		/********************************
		*             I_PUSH            * 
		********************************/

	print("\nTesting I_PUSH (relay2)\n") ;
	rslt = user_ioctl(fd1, I_PUSH, "relay2") ;
	if (rslt < 0)
	{
	    print("loop.1: I_PUSH (relay2): %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	print_stream(fd1) ;
	print_mem() ;

		/********************************
		*             I_FIND            * 
		********************************/

	print("\nTesting I_FIND\n") ;
	rslt = user_ioctl(fd1, I_FIND, "relay") ;
	if (rslt < 0)
	{
	    print("loop.1: I_FIND: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt > 0)	print("Module \"relay\" is present in the stream\n");
	else		print("Module \"relay\" is not present in the stream\n");

	print_mem() ;

		/********************************
		*         Read/Write            * 
		********************************/

	fd2 = user_open(LOOP_2, O_RDWR, 0) ;
	if (fd2 < 0)
	{
	    print("loop.2: %s\n", strerror(-fd2)) ;
	    xit() ;
	}

	ioc.ic_cmd 	  = LOOP_SET ;
	ioc.ic_timout 	  = 10 ;
	ioc.ic_len	  = sizeof(int) ;
	ioc.ic_dp	  = (char *) &arg ;

	arg = 2 ;
	rslt = user_ioctl(fd1, I_STR, &ioc) ;
	if (rslt < 0)
	{
	    print("loop.1: ioctl LOOP_SET: %s\n", strerror(-rslt)) ;
	}

	print("\nTesting read and write\n") ;
	strcpy(buf, "Data to send down the file") ;
	rslt = write_data(fd1, buf, lgth = strlen(buf)) ;
	if (rslt < 0)
	    xit() ;

	/*
	 * Streams read will wait until the entire count is
	 * exhausted as the default.  Later we will test changing
	 * this option.  For now, just read what was written.
	 */
	rdbuf[0] = 0 ;
	rslt = user_read(fd2, rdbuf, lgth);
	if (rslt < 0)
	{
	    print("loop.2: read: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt != lgth)
	{
	    print("loop.2:  read returned %d, expected %d\n", rslt, lgth) ;
	    xit() ;
	}

	if (strcmp(buf, rdbuf))
	{
	    print("loop.2: read: buffer compare error\n") ;
	    print("              wrote \"%s\"\n", buf) ;
	    print("              read  \"%s\"\n", rdbuf) ;
	}
	else
	    print("loop.2: read %d bytes: buffer compared OK\n", rslt) ;

	for (i = 1; i <= 20; i++)
	    strcat(buf, " Add more data to make the message longer. ") ;

	lgth = strlen(buf) ;
	print("Write %d bytes and read back\n", lgth) ;
	rslt = write_data(fd1, buf, lgth) ;
	if (rslt < 0)
	    xit() ;

	/*
	 * Streams read will wait until the entire count is
	 * exhausted as the default.  Later we will test changing
	 * this option.  For now, just read what was written.
	 */
	rdbuf[0] = 0 ;
	rslt = user_read(fd2, rdbuf, lgth);
	if (rslt < 0)
	{
	    print("loop.2: read: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt != lgth)
	{
	    print("loop.2:  read returned %d, expected %d\n", rslt, lgth) ;
	    xit() ;
	}

	if (strcmp(buf, rdbuf))
	{
	    print("loop.2: read: buffer compare error\n") ;
	    print("              wrote \"%s\"\n", buf) ;
	    print("              read  \"%s\"\n", rdbuf) ;
	}
	else
	    print("loop.2: read %d bytes: buffer compared OK\n", rslt) ;


		/********************************
		*             I_NREAD           * 
		********************************/

	print("\nTesting I_NREAD\n") ;
	/*
	 * Write two blocks into the file.
	 */
	strcpy(buf, "Data to send down the file") ;
	rslt = write_data(fd1, buf, lgth = strlen(buf)) ;
	if (rslt < 0)
	    xit() ;

	strcpy(buf, "More data to send down the file") ;
	rslt = write_data(fd1, buf, lgth2 = strlen(buf)) ;
	if (rslt < 0)
	    xit() ;

	rslt = n_read(fd2) ;
	if (rslt < 0)
	{
	    print_mem() ;
	    xit() ;
	}

	if (rslt != lgth)		/* just the 1st  msg */
	{
	    print("loop.2:  I_NREAD returned %d, expected %d\n", rslt, lgth) ;
	    print_mem() ;
	    xit() ;
	}
	else
	    print("loop.2: I_NREAD returned %d, OK\n", rslt) ;

	rslt = user_read(fd2, rdbuf, lgth);
	if (rslt < 0)
	{
	    print("loop.2: read: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt != lgth)
	{
	    print("loop.2:  read returned %d, expected %d\n", rslt, lgth) ;
	    xit() ;
	}


		/********************************
		*             I_FLUSH           * 
		********************************/

	print("\nTesting I_FLUSH\n") ;

	strcpy(buf, "More data to send down the file") ;
	rslt = write_data(fd1, buf, lgth = strlen(buf)) ;
	if (rslt < 0)
	    xit() ;

	rslt = write_data(fd1, buf, lgth) ;
	if (rslt < 0)
	    xit() ;

	rslt = n_read(fd2) ;
	if (rslt < 0) xit() ;
	if (rslt != lgth)
	{
	    print("loop.2: I_NREAD returned %d, expected %d\n", rslt, lgth) ;
	    xit() ;
	}

	rslt = user_ioctl(fd2, I_FLUSH, FLUSHRW) ;
	if (rslt < 0)
	{
	    print("loop.2: I_FLUSH: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	rslt = n_read(fd2) ;
	if (rslt < 0) xit() ;
	if (rslt != 0)
	{
	    print("loop.2: I_NREAD returned %d, expected %d\n", rslt, 0) ;
	    xit() ;
	}

		/********************************
		*           I_CANPUT            * 
		********************************/

	print("\nTesting I_CANPUT\n") ;
	rslt = user_ioctl(fd2, I_CANPUT, 0) ;
	if (rslt < 0)
	{
	    print("loop.2: I_CANPUT: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	print("loop.2: I_CANPUT returned %d\n", rslt) ;

		/********************************
		*           I_SETCLTIME         * 
		********************************/

	print("\nTesting I_SETCLTIME\n") ;
	rslt = user_ioctl(fd2, I_SETCLTIME, 50) ;
	if (rslt < 0)
	{
	    print("loop.2: I_SETCLTIME: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	print("loop.2: I_SETCLTIME returned %d\n", rslt) ;

		/********************************
		*           I_GETCLTIME         * 
		********************************/

	print("\nTesting I_GETCLTIME\n") ;
	arg = 0 ;
	rslt = user_ioctl(fd2, I_GETCLTIME, &arg) ;
	if (rslt < 0)
	{
	    print("loop.2: I_GETCLTIME: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (arg != 50)
	    print("loop.2: I_GETCLTIME returned %d, expected %d\n", arg, 50) ;
	else
	    print("loop.2: I_GETCLTIME returned %d\n", arg) ;

		/********************************
		*             I_LIST            * 
		********************************/

	print("\nTesting I_I_LIST\n") ;
	rslt = user_ioctl(fd1, I_LIST, NULL) ;
	if (rslt < 0)
	{
	    print("loop.1: I_LIST: %s\n", strerror(-rslt)) ;
	    xit() ;
	}
	print("I_LIST(loop.1, NULL) = %d\n", rslt) ;
	lgth = rslt ;			/* length of list */

	rslt = user_ioctl(fd2, I_LIST, NULL) ;
	if (rslt < 0)
	{
	    print("loop.2: I_LIST: %s\n", strerror(-rslt)) ;
	    xit() ;
	}
	print("I_LIST(loop.1, NULL) = %d\n", rslt) ;
	lgth2 = rslt ;			/* length of list */

	rslt = user_ioctl(fd1, I_LIST, &mod_list) ;
	if (rslt < 0)
	{
	    print("loop.1: I_LIST: %s\n", strerror(-rslt)) ;
	    xit() ;
	}
	if (rslt != lgth)
	{
	    print("loop.1: I_LIST returned %d, expected %d \n",
		    rslt, lgth) ;
	    xit() ;
	}

	for (mlp = mod_names; rslt > 0; rslt--, mlp++)
	{
	    print("loop.1 module[%d] is \"%s\"\n", lgth-rslt, mlp->l_name) ;
	}

	rslt = user_ioctl(fd2, I_LIST, &mod_list) ;
	if (rslt < 0)
	{
	    print("loop.2: I_LIST: %s\n", strerror(-rslt)) ;
	    xit() ;
	}
	if (rslt != lgth2)
	{
	    print("loop.2: I_LIST returned %d, expected %d \n",
		    rslt, lgth2) ;
	    xit() ;
	}

	for (mlp = mod_names; rslt > 0; rslt--, mlp++)
	{
	    print("loop.2 module[%d] is \"%s\"\n", lgth2-rslt, mlp->l_name) ;
	}

		/********************************
		*           I_ATMARK            *
		********************************/

	print("\nTesting I_ATMARK\n") ;

	strcpy(buf, "Data to send down the file for testing I_ATMARK") ;
	lgth = strlen(buf) ;

	rslt = user_ioctl(fd2, I_ATMARK, ANYMARK) ;
	print("loop.2: I_ATMARK w/no messages: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	print("OK\n") ;

	rslt = write_data(fd1, buf, lgth) ;
	if (rslt < 0)
	    xit() ;

	rslt = user_ioctl(fd2, I_ATMARK, ANYMARK) ;
	print("loop.2: I_ATMARK w/non-marked message: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	print("OK\n") ;

	rslt = user_ioctl(fd2, I_FLUSH, FLUSHRW) ;
	if (rslt < 0)
	{
	    print("loop.2: I_FLUSH: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	ioc.ic_cmd 	  = LOOP_MARK ;		/* mark the next msg */
	ioc.ic_len	  = 0 ;
	ioc.ic_dp	  = NULL ;

	rslt = user_ioctl(fd1, I_STR, &ioc) ;
	if (rslt < 0)
	{
	    print("loop.1: ioctl LOOP_MARK: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	rslt = write_data(fd1, buf, lgth) ;
	if (rslt < 0)
	    xit() ;

	rslt = user_ioctl(fd2, I_ATMARK, ANYMARK) ;
	print("loop.2: I_ATMARK(ANYMARK) w/marked message: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt == 1)
	    print("OK\n") ;
	else
	{
	    print("returned %d, expected 1\n", rslt) ;
	    xit() ;
	}

	rslt = user_ioctl(fd2, I_ATMARK, LASTMARK) ;
	print("loop.2: I_ATMARK(LASTMARK) w/marked message last: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt == 1)
	    print("OK\n") ;
	else
	{
	    print("returned %d, expected 1\n", rslt) ;
	    xit() ;
	}

	rslt = write_data(fd1, buf, lgth) ;	/* non-marked msg */
	if (rslt < 0)
	    xit() ;

	rslt = user_ioctl(fd2, I_ATMARK, ANYMARK) ;
	print("loop.2: I_ATMARK(ANYMARK) w/marked message: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt == 1)
	    print("OK\n") ;
	else
	{
	    print("returned %d, expected 1\n", rslt) ;
	    xit() ;
	}

	rslt = user_ioctl(fd2, I_ATMARK, LASTMARK) ;
	print("loop.2: I_ATMARK(LASTMARK) w/marked message last: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt == 1)
	    print("OK\n") ;
	else
	{
	    print("returned %d, expected 1\n", rslt) ;
	    xit() ;
	}

	rslt = user_ioctl(fd1, I_STR, &ioc) ;		/* mark nxt msg */
	if (rslt < 0)
	{
	    print("loop.1: ioctl LOOP_MARK: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	rslt = write_data(fd1, buf, lgth) ;
	if (rslt < 0)
	    xit() ;

	rslt = user_ioctl(fd2, I_ATMARK, ANYMARK) ;
	print("loop.2: I_ATMARK(ANYMARK) w/marked message: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt == 1)
	    print("OK\n") ;
	else
	{
	    print("returned %d, expected 1\n", rslt) ;
	    xit() ;
	}

	rslt = user_ioctl(fd2, I_ATMARK, LASTMARK) ;
	print("loop.2: I_ATMARK(LASTMARK) w/marked message not last: ") ;
	if (rslt < 0)
	{
	    print("%s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (rslt == 0)
	    print("OK\n") ;
	else
	{
	    print("returned %d, expected 1\n", rslt) ;
	    xit() ;
	}

	rslt = user_ioctl(fd2, I_FLUSH, FLUSHRW) ;
	if (rslt < 0)
	{
	    print("loop.2: I_FLUSH: %s\n", strerror(-rslt)) ;
	    xit() ;
	}


		/********************************
		*           I_SETSIG            * 
		********************************/

	print("\nTesting I_SETSIG/I_GETSIG\n") ;
	rslt = user_ioctl(fd1, I_SETSIG, S_INPUT) ;
	if (rslt < 0)
	{
	    print("loop.1: I_SETSIG: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	rslt = user_ioctl(fd1, I_GETSIG, &arg) ;
	if (rslt < 0)
	{
	    print("loop.1: I_GETSIG: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	if (arg == S_INPUT)
	    print("loop.1: I_GETSIG returned 0x%x, OK\n", arg) ;
	else
	{
	    print("loop.1: I_GETSIG returned 0x%x, expected 0x%x\n",
	    		arg, S_INPUT) ;
	    xit() ;
	}

	signal(SIGPOLL, input_sig) ;
	rslt = write_data(fd2, buf, lgth) ;	/* produce some input on fd1 */
	if (rslt < 0)
	    xit() ;



		/********************************
		*             I_POP             * 
		********************************/

	print("\nTesting I_POP\n") ;
	rslt = user_ioctl(fd1, I_POP, 0) ;
	if (rslt < 0)
	{
	    print("loop.1: I_POP: %s\n", strerror(-rslt)) ;
	    xit() ;
	}

	print_mem() ;


		/********************************
		*         Close Files           * 
		********************************/

	user_close(fd1) ;
	user_close(fd2) ;
    }

} /* ioctl_test */

/************************************************************************
*                             rdopt_test                                *
*************************************************************************
*									*
* Test various read options (and the associated ioctls).		*
*									*
************************************************************************/
void	rdopt_test(void)
{
    int			i ;
    int			fd1 ;
    int			fd2 ;
    int			arg ;
    int			rslt ;
    int			lgth ;
    int			lgth2 ;

    print("\nRead option test\n") ;

	    /********************************
	    *         Open Files            * 
	    ********************************/

    rslt = open_files(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

	    /********************************
	    *         RNORM Test            * 
	    ********************************/

    print("\nTesting I_SRDOPT/I_GRDOPT(RNORM)\n") ;
    rslt = user_ioctl(fd1, I_SRDOPT, RNORM) ;
    if (rslt < 0)
    {
	print("loop.1: I_SRDOPT(RNORM): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    rslt = user_ioctl(fd1, I_GRDOPT, &arg) ;
    if (rslt < 0)
    {
	print("loop.1: I_GRDOPT(RNORM): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if ((arg & RMODEMASK) == RNORM)
	print("I_SRDOPT(RNORM) == I_GRDOPT\n") ;
    else
	print("I_SRDOPT(RNORM): set opt to %d, read back %d\n",
		    RNORM, arg & RMODEMASK) ;

    /*
     * Demonstrate this mode of operation by writing two messasges
     * downstream and then reading back both of them in one read.
     */
    strcpy(buf, "Test data for I_SRDOPT(RNORM)") ;
    lgth = strlen(buf) ;
    for (i = 1; i <= 3; i++)		/* write it three times */
    {
	if (write_data(fd2, buf, lgth) < 0) xit() ;
    }

    /*
     * Streams read will wait until the entire count is
     * exhausted as the default.  
     */
    memset(rdbuf, 0, sizeof(rdbuf)) ;
    rslt = user_read(fd1, rdbuf, 2*lgth+1);
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != 2*lgth+1)
    {
	print("loop.1:  read returned %d, expected %d\n", rslt, 2*lgth+1) ;
	xit() ;
    }

    rslt = user_read(fd1, &rdbuf[2*lgth+1], lgth-1);
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth-1)
    {
	print("loop.1:  read returned %d, expected %d\n", rslt, lgth-1) ;
	xit() ;
    }

    if (   strncmp(buf, &rdbuf[0],      lgth) == 0
        && strncmp(buf, &rdbuf[lgth],   lgth) == 0
        && strncmp(buf, &rdbuf[2*lgth], lgth) == 0
       )
	   print("Buffers compare OK\n") ;
    else
    {
	print("Data read does not match data written\n") ;
	print("Wrote: %s\n", buf) ;
	print("Read:  %s\n", rdbuf) ;
	xit() ;
    }

	    /********************************
	    *         RMSGD Test            * 
	    ********************************/

    print("\nTesting I_SRDOPT/I_GRDOPT(RMSGD)\n") ;
    rslt = user_ioctl(fd1, I_SRDOPT, RMSGD) ;
    if (rslt < 0)
    {
	print("loop.1: I_SRDOPT(RMSGD): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    rslt = user_ioctl(fd1, I_GRDOPT, &arg) ;
    if (rslt < 0)
    {
	print("loop.1: I_GRDOPT(RMSGD): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if ((arg & RMODEMASK) == RMSGD)
	print("I_SRDOPT(RMSGD) == I_GRDOPT\n") ;
    else
	print("I_SRDOPT(RMSGD): set opt to %d, read back %d\n",
		    RMSGD, arg & RMODEMASK) ;

    /*
     * Demonstrate this mode of operation by writing a message
     * and reading just a part of it back.  I_NREAD will then
     * tell us that there is nothing to be read anymore.
     */
    strcpy(buf, "Test data for I_SRDOPT(RMSGD)") ;
    lgth = strlen(buf) ;
    if (write_data(fd2, buf, lgth) < 0) xit() ;

    /*
     * Streams read will return with whatever is there.  Leftover
     * message fragment will be discarded.
     */
    memset(rdbuf, 0, sizeof(rdbuf)) ;	
    rslt = user_read(fd1, rdbuf, lgth/2);	/* read half the message */
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth/2)
    {
	print("loop.1:  read returned %d, expected %d\n", rslt, lgth/2) ;
	xit() ;
    }

    if (strncmp(buf, rdbuf, lgth/2) == 0)
	print("Buffers compare OK\n") ;
    else
    {
	print("Data read does not match data written\n") ;
	print("Wrote: %s\n", buf) ;
	print("Read:  %s\n", rdbuf) ;
	xit() ;
    }

    rslt = n_read(fd1) ;
    if (rslt < 0) xit() ;
    if (rslt > 0)
	print("%d bytes still waiting to be read, should be zero\n", rslt);
    else
	print("No bytes waiting to be read\n") ;


	    /********************************
	    *         RMSGN Test            * 
	    ********************************/

    print("\nTesting I_SRDOPT/I_GRDOPT(RMSGN)\n") ;
    rslt = user_ioctl(fd1, I_SRDOPT, RMSGN) ;
    if (rslt < 0)
    {
	print("loop.1: I_SRDOPT(RMSGN): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    rslt = user_ioctl(fd1, I_GRDOPT, &arg) ;
    if (rslt < 0)
    {
	print("loop.1: I_GRDOPT(RMSGN): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if ((arg & RMODEMASK) == RMSGN)
	print("I_SRDOPT(RMSGN) == I_GRDOPT\n") ;
    else
	print("I_SRDOPT(RMSGN): set opt to %d, read back %d\n",
		    RMSGN, arg & RMODEMASK) ;

    /*
     * Demonstrate this mode of operation by writing a message
     * and reading just a part of it back.  I_NREAD will then
     * tell us that there are more bytes waiting to be read.
     */
    strcpy(buf, "Test data for I_SRDOPT(RMSGN)") ;
    lgth = strlen(buf) ;
    if (write_data(fd2, buf, lgth) < 0) xit() ;

    /*
     * Streams read will return with whatever is there.  Leftover
     * message fragment will be saved.
     */
    memset(rdbuf, 0, sizeof(rdbuf)) ;	
    rslt = user_read(fd1, rdbuf, lgth/2);	/* read half the message */
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth/2)
    {
	print("loop.1:  read returned %d, expected %d\n", rslt, lgth/2) ;
	xit() ;
    }

    rslt = n_read(fd1) ;			/* see what's left */
    if (rslt < 0) xit() ;
    if (rslt != lgth - lgth/2)
    {
	print("%d bytes still waiting to be read, should be %d\n",
		 rslt, lgth - lgth/2);
	xit() ;
    }

    print("%d bytes waiting to be read, OK\n", rslt) ;

    rslt = user_read(fd1, &rdbuf[lgth/2], lgth) ;	/* read the rest */
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth - lgth/2)
    {
	print("%d bytes on second read, should be %d\n",
		 rslt, lgth - lgth/2);
	xit() ;
    }

    if (strncmp(buf, rdbuf, lgth) == 0)
	print("Buffers compare OK\n") ;
    else
    {
	print("Data read does not match data written\n") ;
	print("Wrote: %s\n", buf) ;
	print("Read:  %s\n", rdbuf) ;
	xit() ;
    }

	    /********************************
	    *           RPROTDAT            * 
	    ********************************/

    print("\nTesting I_SRDOPT/I_GRDOPT(RPROTDAT)\n") ;
    rslt = user_ioctl(fd1, I_SRDOPT, RNORM|RPROTDAT) ;
    if (rslt < 0)
    {
	print("loop.1: I_SRDOPT(RPROTDAT): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    rslt = user_ioctl(fd1, I_GRDOPT, &arg) ;
    if (rslt < 0)
    {
	print("loop.1: I_GRDOPT(RPROTDAT): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (arg == (RNORM|RPROTDAT))
	print("I_SRDOPT(RPROTDAT) == I_GRDOPT\n") ;
    else
	print("I_SRDOPT(RPROTDAT): set opt to %d, read back %d\n",
		    RNORM|RPROTDAT, arg) ;

    /*
     * Use putmsg to send a control message, read it back as normal
     * data.
     */
    print("Control message only...\n") ;
    strcpy(ctlbuf, "Control message for RPROTDAT test") ;
    lgth = strlen(ctlbuf) ;
    wr_ctl.len	= lgth ;
    wr_dta.len	= -1 ;				/* no data part */
    if (put_msg(fd2, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    memset(rdbuf, 0, sizeof(rdbuf)) ;	
    rslt = user_read(fd1, rdbuf, lgth);		/* read the message */
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth)
    {
	print("loop.1:  read returned %d, expected %d\n", rslt, lgth) ;
	xit() ;
    }

    if (strcmp(ctlbuf, rdbuf) == 0)
	print("Buffers compare OK\n") ;
    else
    {
	print("Data read does not match data written\n") ;
	print("Wrote: %s\n", ctlbuf) ;
	print("Read:  %s\n", rdbuf) ;
	xit() ;
    }

    /*
     * Use putmsg to send a control and data message, read it back as normal
     * data.
     */
    print("Control and data message...\n") ;
    strcpy(ctlbuf, "Control message for RPROTDAT test") ;
    strcpy(buf,    "/Data message for RPROTDAT test") ;
    lgth = strlen(ctlbuf) ;
    lgth2 = strlen(buf) ;
    wr_ctl.len	= lgth ;
    wr_dta.len	= lgth2 ;
    if (put_msg(fd2, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    memset(rdbuf, 0, sizeof(rdbuf)) ;	
    lgth = lgth + lgth2 ;			/* combined message lgth */
    rslt = user_read(fd1, rdbuf, lgth);		/* read the message */
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth)
    {
	print("loop.1:  read returned %d, expected %d\n", rslt, lgth) ;
	xit() ;
    }

    strcat(ctlbuf, buf) ;			/* concatenate the msgs */
    if (strcmp(ctlbuf, rdbuf) == 0)
	print("Buffers compare OK\n") ;
    else
    {
	print("Data read does not match data written\n") ;
	print("Wrote: %s\n", ctlbuf) ;
	print("Read:  %s\n", rdbuf) ;
	xit() ;
    }

	    /********************************
	    *           RPROTDIS            * 
	    ********************************/

    print("\nTesting I_SRDOPT/I_GRDOPT(RPROTDIS)\n") ;
    rslt = user_ioctl(fd1, I_SRDOPT, RMSGN|RPROTDIS) ;
    if (rslt < 0)
    {
	print("loop.1: I_SRDOPT(RPROTDIS): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    rslt = user_ioctl(fd1, I_GRDOPT, &arg) ;
    if (rslt < 0)
    {
	print("loop.1: I_GRDOPT(RPROTDIS): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (arg == (RMSGN|RPROTDIS))
	print("I_SRDOPT(RPROTDIS) == I_GRDOPT\n") ;
    else
	print("I_SRDOPT(RPROTDIS): set opt to %d, read back %d\n",
		    RMSGN|RPROTDIS, arg) ;

    /*
     * Use putmsg to send a control message, should leave nothing to
     * be read.
     *
     * It is not clear whether this should result in a zero-length
     * message or no message at all.  For now it is the second.
     */
    print("Control message only...\n") ;
    strcpy(ctlbuf, "Control message for RPROTDIS test") ;
    lgth = strlen(ctlbuf) ;
    wr_ctl.len	= lgth ;
    wr_dta.len	= -1 ;				/* no data part */
    if (put_msg(fd2, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    rslt = n_read(fd1);				/* check how many bytes */
    if (rslt < 0) xit() ;

    if (rslt != 0)				/* should read 0 bytes */
    {
	print("loop.1:  I_NREAD returned %d, expected %d\n", rslt, 0) ;
	xit() ;
    }

    /*
     * Use putmsg to send a control and data message, read it back as normal
     * data.  The control part should be discarded and the data part
     * delivered.
     */
    print("Control and data message...\n") ;
    strcpy(ctlbuf, "Control message for RPROTDIS test") ;
    strcpy(buf,    "/Data message for RPROTDIS test") ;
    lgth = strlen(ctlbuf) ;
    lgth2 = strlen(buf) ;
    wr_ctl.len	= lgth ;
    wr_dta.len	= lgth2 ;
    if (put_msg(fd2, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    memset(rdbuf, 0, sizeof(rdbuf)) ;	
    rslt = user_read(fd1, rdbuf, sizeof(rdbuf));	/* read the message */
    if (rslt < 0)
    {
	print("loop.1: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth2)
    {
	print("loop.1:  read returned %d, expected %d\n", rslt, lgth2) ;
	xit() ;
    }

    if (strcmp(buf, rdbuf) == 0)			/* only the data */
	print("Buffers compare OK\n") ;
    else
    {
	print("Data read does not match data written\n") ;
	print("Wrote: %s\n", buf) ;
	print("Read:  %s\n", rdbuf) ;
	xit() ;
    }

	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nrdopt_test: close files\n") ;
    user_close(fd1) ;
    user_close(fd2) ;

} /* rdopt_test */

/************************************************************************
*                               write_test                              *
*************************************************************************
*									*
* Test various forms of write.						*
*									*
************************************************************************/
void	write_test(void)
{
    int			fd1 ;
    int			fd2 ;
    int			arg ;
    int			rslt ;

    print("\nWrite option test\n") ;

	    /********************************
	    *         Open Files            * 
	    ********************************/

    rslt = open_files(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

	    /********************************
	    *       Zero-lgth Write         * 
	    ********************************/

    print("\nTesting write zero bytes w/o SNDZERO option\n") ;
    rslt = user_write(fd1, buf, 0) ;
#if 1
    if (rslt == 0)
	print("loop.1: *** write zero bytes returned zero.  Should it?\n") ;
    else
	print("loop.1: *** write zero bytes returned %d.  Should it?\n", rslt) ;
#else
    if (rslt < 0)
	print("loop.1: write zero bytes: %s: expected error\n",
		    strerror(-rslt)) ;
    else
    {
	print("loop.1: write zero bytes: returned %d instead of error\n",
		rslt) ;
	xit() ;
    }
#endif

    print("\nTesting write zero bytes with SNDZERO option\n") ;
    rslt = user_ioctl(fd1, I_SWROPT, SNDZERO) ;
    if (rslt < 0)
    {
	print("loop.1: I_SWROPT(SNDZERO): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    rslt = user_ioctl(fd1, I_GWROPT, &arg) ;
    if (rslt < 0)
    {
	print("loop.1: I_SWROPT(SNDZERO): %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (arg == SNDZERO)
	print("I_SWROPT(SNDZERO) == I_GWROPT\n") ;
    else
    {
	print("I_SWROPT(SNDZERO): set opt to %d, read back %d\n",
		    SNDZERO, arg) ;
	xit() ;
    }

    rslt = user_write(fd1, buf, 0) ;
    if (rslt < 0)
    {
	print("loop.1: write zero bytes: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
	print("loop.1: write zero bytes: returned %d\n", rslt) ;

    memset(rdbuf, 0, sizeof(rdbuf)) ;	
    rslt = user_read(fd2, rdbuf, sizeof(rdbuf)) ;
    if (rslt < 0)
    {
	print("loop.2: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != 0)
    {
	print("loop.2:  read returned %d, expected %d\n", rslt, 0) ;
	xit() ;
    }

    print("Read 0 bytes from 0-length write\n") ;


	    /********************************
	    *         Close Files           * 
	    ********************************/

    user_close(fd1) ;
    user_close(fd2) ;

} /* write_test */

/************************************************************************
*                           close_timer_test                            *
*************************************************************************
*									*
* Test the functioning of the close timer.				*
*									*
************************************************************************/
void	close_timer_test(void)
{
    int			fd1 ;
    int			fd2 ;
    int			arg ;
    int			rslt ;
    struct strioctl	ioc ;

    print("\nClose timer test\n") ;

	    /********************************
	    *         Open Files            * 
	    ********************************/

    print("Close and let timer expire\n") ;
    rslt = open_files(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

	    /********************************
	    *         Set 'loop' Optns      *
	    ********************************/


    ioc.ic_cmd 	  = LOOP_MSGLVL ;		/* set queue message level */
    ioc.ic_timout = 10 ;
    ioc.ic_len	  = sizeof(int) ;
    ioc.ic_dp	  = (char *) &arg ;

    arg = 2 ;
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop.1: ioctl LOOP_MSGLVL: %s\n", strerror(-rslt)) ;
	xit() ;
    }

	    /********************************
	    *         Write Message         * 
	    ********************************/

    rslt = write_data(fd1, buf, 20) ;
    if (rslt < 0) xit() ;

    rslt = n_read(fd2) ;
    if (rslt < 0) xit() ;
    if (rslt > 0)
	print("loop.2: I_NREAD returned %d, expected 0\n", rslt) ;


	    /********************************
	    *         Close Files           * 
	    ********************************/

    user_close(fd1) ;
    user_close(fd2) ;


	    /********************************
	    *         Open Files            * 
	    ********************************/

    print("Close and have queue drain before timer expires\n") ;
    rslt = open_files(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

	    /********************************
	    *         Set 'loop' Optns      *
	    ********************************/


    ioc.ic_cmd 	  = LOOP_MSGLVL ;		/* set queue message level */
    ioc.ic_timout = 10 ;
    ioc.ic_len	  = sizeof(int) ;
    ioc.ic_dp	  = (char *) &arg ;

    arg = 2 ;
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop.1: ioctl LOOP_MSGLVL: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    ioc.ic_cmd 	  = LOOP_TIMR ;		/* set timer for queue */
    arg = 10 ;				/* # timer ticks */
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop.1: ioctl LOOP_TIMR: %s\n", strerror(-rslt)) ;
	xit() ;
    }

	    /********************************
	    *         Write Message         * 
	    ********************************/

    rslt = write_data(fd1, buf, 20) ;
    if (rslt < 0) xit() ;

    rslt = n_read(fd2) ;
    if (rslt < 0) xit() ;
    if (rslt > 0)
	print("loop.2: I_NREAD returned %d, expected 0\n", rslt) ;


	    /********************************
	    *         Close Files           * 
	    ********************************/

    user_close(fd1) ;
    user_close(fd2) ;

} /* close_timer_test */

/************************************************************************
*                          check_getmsg_rslts                           *
*************************************************************************
*									*
* Check the results of a getmsg.  We make assumptions about the		*
* locations of the data.  We call xit() if anything goes wrong.		*
*									*
* band is the band the message was sent on, rband is the band it	*
* was received on.							*
*									*
************************************************************************/
void	check_getmsg_rslts(int		  rslt,
			   struct strbuf *rd_ctlp,
			   struct strbuf *rd_dtap,
			   int		 *flags,
			   int		  band,
			   int		  rband)
{
    (void) rslt ;

    if (rd_ctlp->len != wr_ctl.len)
    {
	print("check_getmsg_rslts:  ctl lgth returned %d, expected %d\n",
		rd_ctlp->len, wr_ctl.len) ;
	xit() ;
    }

    if (rd_dtap->len != wr_dta.len)
    {
	print("check_getmsg_rslts:  data lgth returned %d, expected %d\n",
		rd_dtap->len, wr_dta.len) ;
	xit() ;
    }

    print("check_getmsg_rslts: getmsg return flags = 0x%x\n", *flags) ;
    if (rd_dtap->len > 0 && strncmp(buf, rdbuf, rd_dtap->len))
    {
	print("check_getmsg_rslts: read data: buffer compare error\n") ;
	print("              wrote \"%s\"\n", buf) ;
	print("              read  \"%s\"\n", rdbuf) ;
	xit() ;
    }
    else
	print("check_getmsg_rslts: read %d data bytes: buffer compared OK\n",
		rd_dtap->len) ;

    if (rd_ctlp->len > 0 && strncmp(ctlbuf, rdctlbuf, rd_ctlp->len))
    {
	print("check_getmsg_rslts: read control: buffer compare error\n") ;
	print("              wrote \"%s\"\n", ctlbuf) ;
	print("              read  \"%s\"\n", rdctlbuf) ;
	xit() ;
    }
    else
	print("check_getmsg_rslts: read %d ctl bytes: buffer compared OK\n",
		rd_ctlp->len) ;


    if (rband != band)
	print("check_getmsg_rslts: sent on band %d, received on band %d\n",
		band, rband) ;

} /* check_getmsg_rslts */

/************************************************************************
*                             do_get_put                                *
*************************************************************************
*									*
* This is a generalized getmsg/putmsg test routine.  It calls xit()	*
* if anything goes wrong.						*
*									*
* Caller must load up buf (data) and ctlbuf (control) for writing.	*
*									*
************************************************************************/
void	do_get_put(int putfd, int getfd,
		   int ctl_lgth, int data_lgth, int band)
{
    int			flags = MSG_ANY;
    int			rband = 0 ;
    int			rslt ;

    wr_ctl.len	= ctl_lgth ;
    wr_dta.len	= data_lgth ;
    if (put_msg(putfd, &wr_ctl, &wr_dta, band, MSG_BAND) < 0) xit() ;

    rd_ctl.len		= -1 ;
    rd_ctl.maxlen	= sizeof(rdctlbuf) ;
    rd_dta.len		= -1 ;
    rd_dta.maxlen	= sizeof(rdbuf) ;
    flags		= 0 ;

    memset(rdctlbuf, 0, sizeof(rdctlbuf)) ;
    memset(rdbuf, 0, sizeof(rdbuf)) ;

    rslt = user_getpmsg(getfd, &rd_ctl, &rd_dta, &rband, &flags) ;
    check_getmsg_rslts(rslt, &rd_ctl, &rd_dta, &flags, band, rband) ;

} /* do_get_put */

/************************************************************************
*                              do_peek                                  *
*************************************************************************
*									*
* This routine does a putmsg, then an I_PEEK, then a getmsg.  It calls	*
* xit() if anything goes wrong.						*
*									*
* Caller must load up buf (data) and ctlbuf (control) for writing.	*
*									*
************************************************************************/
void	do_peek(int putfd, int getfd,
		   int ctl_lgth, int data_lgth, int band)
{
    int			flags = MSG_ANY;
    int			rband = 0 ;
    int			rslt ;

    wr_ctl.len	= ctl_lgth ;
    wr_dta.len	= data_lgth ;
    if (put_msg(putfd, &wr_ctl, &wr_dta, band, MSG_BAND) < 0) xit() ;

    pk_str.ctlbuf.len		= -1 ;
    pk_str.ctlbuf.maxlen	= sizeof(rdctlbuf) ;
    pk_str.databuf.len		= -1 ;
    pk_str.databuf.maxlen	= sizeof(rdbuf) ;
    pk_str.flags		= MSG_ANY ;

    memset(rdctlbuf, 0, sizeof(rdctlbuf)) ;
    memset(rdbuf, 0, sizeof(rdbuf)) ;

    rslt = user_ioctl(getfd, I_PEEK, &pk_str) ;
    if (rslt < 0)
    {
	print("do_peek: ioctl I_PEEK: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("do_peek: I_PEEK returned %d\n", rslt) ;
    flags = (int) pk_str.flags ;		/* int <-- long */
    check_getmsg_rslts(rslt, &pk_str.ctlbuf, &pk_str.databuf, &flags, 0, 0) ;

    rd_ctl.len		= -1 ;
    if (ctl_lgth >= 0)
	rd_ctl.maxlen	= sizeof(rdctlbuf) ;
    else
	rd_ctl.maxlen	= -1 ;

    rd_dta.len		= -1 ;
    if (data_lgth >= 0)
	rd_dta.maxlen	= sizeof(rdbuf) ;
    else
	rd_dta.maxlen	= -1 ;

    flags		= 0 ;

    memset(rdctlbuf, 0, sizeof(rdctlbuf)) ;
    memset(rdbuf, 0, sizeof(rdbuf)) ;

    flags = MSG_ANY ;
    rslt = user_getpmsg(getfd, &rd_ctl, &rd_dta, &rband, &flags) ;
    check_getmsg_rslts(rslt, &rd_ctl, &rd_dta, &flags, band, rband) ;

} /* do_peek */

/************************************************************************
*                            putmsg_test                                *
*************************************************************************
*									*
* Test putmsg and getmsg.						*
*									*
************************************************************************/
void	putmsg_test(void)
{
    int			fd1 ;
    int			fd2 ;
    int			rslt ;
    int			lgth ;
    int			lgth2 ;
    struct strioctl	ioc ;

    print("\nputmsg/getmsg test\n") ;

	    /********************************
	    *         Open Files            * 
	    ********************************/

    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

	    /********************************
	    *         putmsg/read           *
	    ********************************/

    print("\nUse putmsg to send data, use read to read back data\n") ;
    strcpy(buf, "Test data for putmsg/read") ;
    lgth = strlen(buf) ;
    wr_ctl.len	= -1 ;
    wr_dta.len	= lgth ;
    if (put_msg(fd1, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    rslt = user_read(fd2, rdbuf, lgth);
    if (rslt < 0)
    {
	print("loop_clone.2: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth)
    {
	print("loop_clone.2:  read returned %d, expected %d\n", rslt, lgth) ;
	xit() ;
    }

    if (strcmp(buf, rdbuf))
    {
	print("loop_clone.2: read: buffer compare error\n") ;
	print("              wrote \"%s\"\n", buf) ;
	print("              read  \"%s\"\n", rdbuf) ;
    }
    else
	print("loop_clone.2: read %d bytes: buffer compared OK\n", rslt) ;



    print("\nUse putmsg to send control, read should return error\n") ;
    strcpy(ctlbuf, "Control message") ;
    lgth2 = strlen(ctlbuf) ;
    wr_ctl.len	= lgth2 ;
    wr_dta.len	= lgth ;			/* from before */
    if (put_msg(fd1, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    rslt = user_read(fd2, rdbuf, lgth);
    if (rslt < 0)
	print("loop_clone.2: read returned expected error: %s\n", strerror(-rslt)) ;
    else
    {
	print("loop_clone.2:  read returned %d, expected error\n", rslt) ;
	xit() ;
    }

	    /********************************
	    *         putmsg/getmsg         *
	    *           Data only           *
	    ********************************/

    print("\nUse putmsg to send data, use getmsg to read back data\n") ;

    ioc.ic_timout = 10 ;
    ioc.ic_dp	  = NULL;
    ioc.ic_cmd 	  = LOOP_PUTNXT ;		/* use putnxt rather then svcq */
    ioc.ic_len	  = 0 ;
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop_clone.1: ioctl LOOP_PUTNXT: %s\n", strerror(-rslt)) ;
    }

    rslt = user_ioctl(fd2, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop_clone.2: ioctl LOOP_PUTNXT: %s\n", strerror(-rslt)) ;
    }

    print("Peek with no message present\n") ;
    do_peek(fd1, fd2, -1, -1, 0) ;

    print("Data part only\n") ;
    strcpy(buf, "Test data for putmsg/getmsg") ;
    lgth = strlen(buf) ;
    do_get_put(fd1, fd2, -1, lgth, 0) ;
    do_peek(fd1, fd2, -1, lgth, 0) ;
    do_get_put(fd1, fd2, -1, lgth, 1) ;
    do_peek(fd1, fd2, -1, lgth, 1) ;


	    /********************************
	    *         Ctl and Data          *
	    ********************************/

    print("Control and data parts\n") ;
    do_get_put(fd1, fd2, lgth2, lgth, 0) ;
    do_peek(fd1, fd2, lgth2, lgth, 0) ;
    do_get_put(fd1, fd2, lgth2, lgth, 1) ;
    do_peek(fd1, fd2, lgth2, lgth, 1) ;


	    /********************************
	    *           Ctl only            *
	    ********************************/

    print("Control part only\n") ;
    do_get_put(fd1, fd2, lgth2, -1, 0) ;
    do_peek(fd1, fd2, lgth2, -1, 0) ;
    do_get_put(fd1, fd2, lgth2, -1, 1) ;
    do_peek(fd1, fd2, lgth2, -1, 1) ;


	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nputmsg_test: closing files\n") ;
    user_close(fd1) ;
    user_close(fd2) ;

} /* putmsg_test */

/************************************************************************
*                            poll_test                                  *
*************************************************************************
*									*
* Test the poll function.						*
*									*
************************************************************************/

char	*poll_events(short events)
{
    static char		ascii_events[200] ;

    ascii_events[0] = 0 ;

    if (events & POLLIN) strcat(ascii_events, "POLLIN ") ;
    if (events & POLLRDNORM) strcat(ascii_events, "POLLRDNORM ") ;
    if (events & POLLRDBAND) strcat(ascii_events, "POLLRDBAND ") ;
    if (events & POLLPRI) strcat(ascii_events, "POLLPRI ") ;
    if (events & POLLOUT) strcat(ascii_events, "POLLOUT ") ;
    if (events & POLLWRNORM) strcat(ascii_events, "POLLWRNORM ") ;
    if (events & POLLWRBAND) strcat(ascii_events, "POLLWRBAND ") ;
    if (events & POLLMSG) strcat(ascii_events, "POLLMSG ") ;
    if (events & POLLERR) strcat(ascii_events, "POLLERR ") ;
    if (events & POLLHUP) strcat(ascii_events, "POLLHUP ") ;
    if (events & POLLNVAL) strcat(ascii_events, "POLLNVAL ") ;

    if (ascii_events[0] == 0)
	sprintf(ascii_events, "0x%x", events) ;

    return(ascii_events) ;

} /* poll_events */

void	poll_test(void)
{
    int			fd1 ;
    int			fd2 ;
    int			rslt ;
    int			lgth ;
    int			lgth2 ;
    int			flags ;
    int			rband = 0 ;
    int			arg ;
    struct poll		fds[4] ;
    struct strioctl	ioc ;

    print("\nPoll function test\n") ;

    print("Poll with no descriptors, just timeout\n") ;
    rslt = user_poll(fds, 0, 50) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
	print("Poll returned %d\n", rslt) ;


	    /********************************
	    *         Open Files            * 
	    ********************************/

    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

	    /********************************
	    *         Normal Data           * 
	    ********************************/

    print("\nUse putmsg to send normal data, poll for any input\n") ;
    strcpy(buf, "Test data for putmsg/read") ;
    lgth = strlen(buf) ;
    wr_ctl.len	= -1 ;
    wr_dta.len	= lgth ;
    if (put_msg(fd1, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    fds[0].fd		= fd1 ;		/* writing fd */
    fds[0].events	= 0 ;		/* no events */
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].fd		= fd2 ;		/* reading fd */
    fds[1].events	= POLLIN ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 1)
    {
	print("Poll returned %d, expected 1\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    print("\nPoll for normal data\n") ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLRDNORM ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 1)
    {
	print("Poll returned %d, expected 1\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    print("\nPoll for priority band data\n") ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLRDBAND ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 0)
    {
	print("Poll returned %d, expected 0\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    print("\nPoll for high priority message\n") ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLPRI ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 0)
    {
	print("Poll returned %d, expected 0\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    rslt = user_read(fd2, rdbuf, lgth);
    if (rslt < 0)
    {
	print("loop_clone.2: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth)
    {
	print("loop_clone.2:  read returned %d, expected %d\n", rslt, lgth) ;
	xit() ;
    }

    if (strcmp(buf, rdbuf))
    {
	print("loop_clone.2: read: buffer compare error\n") ;
	print("              wrote \"%s\"\n", buf) ;
	print("              read  \"%s\"\n", rdbuf) ;
    }
    else
	print("loop_clone.2: read %d bytes: buffer compared OK\n", rslt) ;

	    /********************************
	    *         Hi-priority           * 
	    ********************************/

    print("\nUse putmsg to send priority control, poll for any input\n") ;
    strcpy(ctlbuf, "Control message") ;
    lgth2 = strlen(ctlbuf) ;
    wr_ctl.len	= lgth2 ;
    wr_dta.len	= lgth ;			/* from before */
    if (put_msg(fd1, &wr_ctl, &wr_dta, 0, MSG_HIPRI) < 0) xit() ;

    fds[0].fd		= fd1 ;		/* writing fd */
    fds[0].events	= 0 ;		/* no events */
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].fd		= fd2 ;		/* reading fd */
    fds[1].events	= POLLIN ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 0)
    {
	print("Poll returned %d, expected 0\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    print("\nPoll for normal data\n") ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLRDNORM ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 0)
    {
	print("Poll returned %d, expected 0\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    print("\nPoll for priority band data\n") ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLRDBAND ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 0)
    {
	print("Poll returned %d, expected 0\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    print("\nPoll for high priority message\n") ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLPRI ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 1)
    {
	print("Poll returned %d, expected 1\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    print("\nPoll for high priority message and other stream writeable\n") ;
    fds[0].events	= POLLWRNORM ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLPRI ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 2)
    {
	print("Poll returned %d, expected 2\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    rd_ctl.len		= -1 ;
    rd_ctl.maxlen	= sizeof(rdctlbuf) ;
    rd_dta.len		= -1 ;
    rd_dta.maxlen	= sizeof(rdbuf) ;
    flags		= 0 ;

    memset(rdctlbuf, 0, sizeof(rdctlbuf)) ;
    memset(rdbuf, 0, sizeof(rdbuf)) ;

    rslt = user_getpmsg(fd2, &rd_ctl, &rd_dta, &rband, &flags) ;
    check_getmsg_rslts(rslt, &rd_ctl, &rd_dta, &flags, 0, rband) ;



    print("\nPoll for high priority message with delayed delivery\n") ;


    ioc.ic_cmd 	  = LOOP_TIMR ;		/* set timer for queue */
    ioc.ic_timout = 10 ;
    ioc.ic_len	  = sizeof(int) ;
    ioc.ic_dp	  = (char *) &arg ;

    arg = 50 ;				/* # timer ticks */
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop.1: ioctl LOOP_TIMR: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (put_msg(fd1, &wr_ctl, &wr_dta, 0, MSG_HIPRI) < 0) xit() ;

    fds[0].events	= 0 ;
    fds[0].revents	= 0 ;		/* returned events */
    fds[1].events	= POLLPRI ;
    fds[1].revents	= 0 ;		/* returned events */

    rslt = user_poll(fds, 2, 100) ;
    if (rslt < 0)
    {
	print("poll: %s\n", strerror(-rslt)) ;
	xit() ;
    }
    else
    if (rslt != 1)
    {
	print("Poll returned %d, expected 1\n", rslt) ;
	xit() ;
    }
    else
    {
	print("Poll returned %d\n", rslt) ;
	print("fds[0].events  = %s\n", poll_events(fds[0].events)) ;
	print("fds[0].revents = %s\n", poll_events(fds[0].revents)) ;
	print("fds[1].events  = %s\n", poll_events(fds[1].events)) ;
	print("fds[1].revents = %s\n", poll_events(fds[1].revents)) ;
    }

    rd_ctl.len		= -1 ;
    rd_ctl.maxlen	= sizeof(rdctlbuf) ;
    rd_dta.len		= -1 ;
    rd_dta.maxlen	= sizeof(rdbuf) ;
    flags		= 0 ;

    memset(rdctlbuf, 0, sizeof(rdctlbuf)) ;
    memset(rdbuf, 0, sizeof(rdbuf)) ;

    rslt = user_getpmsg(fd2, &rd_ctl, &rd_dta, &rband, &flags) ;
    check_getmsg_rslts(rslt, &rd_ctl, &rd_dta, &flags, 0, rband) ;


	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\npoll_test: closing files\n") ;
    user_close(fd1) ;
    user_close(fd2) ;


} /* poll_test */

/************************************************************************
*                             mux_test                                  *
*************************************************************************
*									*
* Test multiplexors.							*
*									*
************************************************************************/
void	mux_test(void)
{
    int			fd1 ;
    int			fd2 ;
    int			muxfd1 ;
    int			muxfd2 ;
    int			muxfd3 ;
    int			muxfd4 ;
    int			muxid1 ;
    int			muxid2 ;
    int			muxid3 ;
    int			muxid4 ;
    int			rslt ;
    int			lgth ;
    int			arg ;
    struct strioctl	ioc ;

    print("\nSTREAMS multiplexor test\n") ;


	    /********************************
	    *         Open Files            * 
	    ********************************/

    /*
     * Here is what we are building:
     *
     *     	    muxfd1		   muxfd2	fd1  fd2
     *		      |			      |		 |    |
     *		+-----------+		+-----------+    X    X
     *		|  mini-mux |		|  mini-mux |
     *		+-----------+		+-----------+
     *		      | muxid1		      | muxid2
     *		      |			      |
     *		      | (fd1)		      | (fd2)
     *		+-----------+		+-----------+
     *		|    loop   |<--------->|    loop   |
     *		+-----------+		+-----------+
     *
     * muxfd1 and muxfd2 are the control streams for the multiplxor.
     * When they are closed the whole mux is dismantled.
     *
     * fd1 and fd1 can be closed with no impact on the multiplexor.
     */

    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

    print("mux_test: open a mux-clone device\n") ;
    muxfd1 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd1 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd1)) ;
	xit() ;
    }

    print("mux_test: open another mux-clone device\n") ;
    muxfd2 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd2 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd2)) ;
	xit() ;
    }

    print("mux_test: I_LINK loop driver under mux (muxfd1->fd1)\n") ;
    muxid1 = user_ioctl(muxfd1, I_LINK, fd1) ;	/* link fd1 below muxfd1 */
    if (muxid1 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid1)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid1) ;

    print("mux_test: I_LINK loop driver under mux (muxfd2->fd2)\n") ;
    muxid2 = user_ioctl(muxfd2, I_LINK, fd2) ;	/* link fd2 below muxfd2 */
    if (muxid2 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid2)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid2) ;

    print_stream(muxfd1) ;
    print_stream(muxfd2) ;
    print("mux_test: close files to loop driver (now detached)\n") ;
    user_close(fd1) ;
    user_close(fd2) ;

    print("mux_test: send data down through the mux/loop and read back\n") ;
    strcpy(buf, "Test data for sending through the mini-mux") ;
    lgth = strlen(buf) ;
    do_get_put(muxfd1, muxfd2, -1, lgth, 0) ;

	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nmux_test: closing control streams\n") ;
    user_close(muxfd1) ;
    user_close(muxfd2) ;


    print("\nTest cascaded multiplexors\n") ;

	    /********************************
	    *         Open Files            * 
	    ********************************/

    /*
     * Here is what we are building:
     *
     *     	    muxfd3		   muxfd4	fd1  fd2 muxfd1 muxfd2
     *		      |			      |		 |    |     |      |
     *		+-----------+		+-----------+    X    X     X      X
     *		|  mini-mux |		|  mini-mux |
     *		+-----------+		+-----------+
     *		      | muxid3		      | muxid4
     *		      |			      |
     *		      |	(muxfd1)	      | (muxfd2)
     *		+-----------+		+-----------+
     *		|  mini-mux |		|  mini-mux |
     *		+-----------+		+-----------+
     *		      | muxid1		      | muxid2
     *		      |			      |
     *		      | (fd1)		      | (fd2)
     *		+-----------+		+-----------+
     *		|    loop   |<--------->|    loop   |
     *		+-----------+		+-----------+
     *
     * muxfd3 and muxfd4 are the control streams for the upper level of
     * the mux.  muxfd1 and muxfd2 are also control streams but for the
     * lower level of the mux.  When muxfd3 closes (or muxfd4) the
     * whole multiplexor is closed.
     *
     * fd1, fd2, muxfd1 and muxfd2 can all be closed with no loss to
     * the multiplexor topology.
     */

    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

    print("mux_test: open a mux-clone device\n") ;
    muxfd1 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd1 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd1)) ;
	xit() ;
    }

    print("mux_test: open another mux-clone device\n") ;
    muxfd2 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd2 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd2)) ;
	xit() ;
    }

    print("mux_test: I_LINK loop driver under mux (muxfd1->fd1)\n") ;
    muxid1 = user_ioctl(muxfd1, I_LINK, fd1) ;	/* link fd1 below muxfd1 */
    if (muxid1 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid1)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid1) ;

    print("mux_test: I_LINK loop driver under mux (muxfd2->fd2)\n") ;
    muxid2 = user_ioctl(muxfd2, I_LINK, fd2) ;	/* link fd2 below muxfd2 */
    if (muxid2 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid2)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid2) ;

    print("mux_test: open a mux-clone device to cascade\n") ;
    muxfd3 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd3 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd3)) ;
	xit() ;
    }

    print("mux_test: open another mux-clone device to cascade\n") ;
    muxfd4 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd4 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd4)) ;
	xit() ;
    }

    print("mux_test: I_LINK mux driver under mux (muxfd3->muxfd1)\n") ;
    muxid3 = user_ioctl(muxfd3, I_LINK, muxfd1) ; /* link muxfd1 below muxfd3 */
    if (muxid3 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid3)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid3) ;

    print("mux_test: I_LINK mux driver under mux (muxfd4->muxfd2)\n") ;
    muxid4 = user_ioctl(muxfd4, I_LINK, muxfd2) ; /* link muxfd2 below muxfd4 */
    if (muxid4 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid4)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid4) ;

    print_stream(muxfd3) ;
    print_stream(muxfd4) ;
    print("mux_test: close files to loop driver (now detached)\n") ;
    user_close(fd1) ;
    user_close(fd2) ;
    print("mux_test: close files to lower mux driver (now detached)\n") ;
    user_close(muxfd1) ;
    user_close(muxfd2) ;

    print("mux_test: send data down through the mux/mux/loop and read back\n") ;
    strcpy(buf, "Test data for sending through the mini-mux") ;
    lgth = strlen(buf) ;
    do_get_put(muxfd3, muxfd4, -1, lgth, 0) ;

	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nmux_test: closing control streams of cascaded muxs\n") ;
    user_close(muxfd3) ;
    user_close(muxfd4) ;


	    /********************************
	    *         Open Files            * 
	    ********************************/

    /*
     * Here is what we are building:
     *
     *     	    muxfd1		         	fd1  fd2
     *		      |			      		 |    |
     *		+-----------------------------------+    X    X
     *		|               mini-mux            |
     *		+-----------------------------------+
     *		      | muxid1		      | muxid2
     *		      |			      |
     *		      | (fd1)		      | (fd2)
     *		+-----------+		+-----------+
     *		|    loop   |<--------->|    loop   |
     *		+-----------+		+-----------+
     *
     * muxfd1 is the control stream for the multiplxor.
     * When they it is closed the whole mux is dismantled.
     *
     * muxid1 and muxid2 are two lowers under the same mux.  We use
     * ioctls to route downstream data from muxfd1 to muxid1 and
     * to route upstream data from muxid2 to muxfd1.
     *
     * fd1 and fd1 can be closed with no impact on the multiplexor.
     */


    print("\nTest 2 lowers under one control stream\n") ;
    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

    print("mux_test: open a mux-clone device\n") ;
    muxfd1 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd1 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd1)) ;
	xit() ;
    }

    print("mux_test: I_LINK loop driver under mux (muxfd1->fd1)\n") ;
    muxid1 = user_ioctl(muxfd1, I_LINK, fd1) ;	/* link fd1 below muxfd1 */
    if (muxid1 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid1)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid1) ;

    print("mux_test: I_LINK loop driver under mux (muxfd1->fd2)\n") ;
    muxid2 = user_ioctl(muxfd1, I_LINK, fd2) ;	/* link fd2 below muxfd2 */
    if (muxid2 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid2)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid2) ;

    print_stream(muxfd1) ;
    print("mux_test: close files to loop driver (now detached)\n") ;
    user_close(fd1) ;
    user_close(fd2) ;

    ioc.ic_cmd 	  = MINIMUX_DOWN ;	/* set downstream linkage */
    ioc.ic_timout = 10 ;
    ioc.ic_len	  = sizeof(int) ;
    ioc.ic_dp	  = (char *) &arg ;

    arg = muxid1 ;			/* downstream muxfd1 -> muxid1 */
    rslt = user_ioctl(muxfd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("minimux.1: ioctl MINIMUX_DOWN: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    ioc.ic_cmd 	  = MINIMUX_UP ;	/* set upstream linkage */
    arg = muxid2 ;			/* upstream muxfd1 <- muxid2 */
    rslt = user_ioctl(muxfd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("minimux.1: ioctl MINIMUX_UP: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("mux_test: send data down through the mux/loop and read back\n") ;
    strcpy(buf, "Test data for sending through the mini-mux") ;
    lgth = strlen(buf) ;
    do_get_put(muxfd1, muxfd1, -1, lgth, 0) ;

	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nmux_test: closing control streams\n") ;
    user_close(muxfd1) ;


	    /********************************
	    *         Open Files            * 
	    ********************************/

    /*
     * Here is what we are building:
     *
     *     	    muxfd1		   muxfd2	fd1  fd2
     *		      |			      |		 |    |
     *		+-----------+		+-----------+    X    X
     *		|  mini-mux |		|  mini-mux |
     *		+-----------+		+-----------+
     *		      | muxid1		      | muxid2
     *		      |			      |
     *		      | (fd1)		      | (fd2)
     *		+-----------+		+-----------+
     *		|    loop   |<--------->|    loop   |
     *		+-----------+		+-----------+
     *
     * muxfd1 and muxfd2 are the control streams for the multiplxor.
     * When they are closed the whole mux is dismantled.
     *
     * fd1 and fd1 will be held open and reconnected when the multiplexor
     * is unlinked via explicit ioctls.
     */

    print("\nTest reconnecting lower after explicit I_UNLINK\n") ;
    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

    print("mux_test: open a mux-clone device\n") ;
    muxfd1 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd1 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd1)) ;
	xit() ;
    }

    print("mux_test: open another mux-clone device\n") ;
    muxfd2 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd2 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd2)) ;
	xit() ;
    }

    print("mux_test: I_LINK loop driver under mux (muxfd1->fd1)\n") ;
    muxid1 = user_ioctl(muxfd1, I_LINK, fd1) ;	/* link fd1 below muxfd1 */
    if (muxid1 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid1)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid1) ;

    print("mux_test: I_LINK loop driver under mux (muxfd2->fd2)\n") ;
    muxid2 = user_ioctl(muxfd2, I_LINK, fd2) ;	/* link fd2 below muxfd2 */
    if (muxid2 < 0)
    {
	print("mux_clone: I_LINK: %s\n", strerror(-muxid2)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid2) ;

    print_stream(muxfd1) ;
    print_stream(muxfd2) ;
    print("mux_test: leave open files to loop driver\n") ;

    print("mux_test: send data down through the mux/loop and read back\n") ;
    strcpy(buf, "Test data for sending through the mini-mux") ;
    lgth = strlen(buf) ;
    do_get_put(muxfd1, muxfd2, -1, lgth, 0) ;

    print("mux_test: I_UNLINK muxid1 from muxfd1\n") ;
    rslt = user_ioctl(muxfd1, I_UNLINK, muxid1) ;
    if (rslt < 0)
    {
	print("minimux: I_UNLINK: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("mux_test: I_UNLINK muxid2 from muxfd2\n") ;
    rslt = user_ioctl(muxfd2, I_UNLINK, muxid2) ;
    if (rslt < 0)
    {
	print("minimux: I_UNLINK: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("mux_test: send data down through the loop and read back\n") ;
    do_get_put(fd1, fd2, -1, lgth, 0) ;

	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nmux_test: close control streams\n") ;
    user_close(muxfd1) ;
    user_close(muxfd2) ;
    print("mux_test: send data down through the loop and read back\n") ;
    do_get_put(fd1, fd2, -1, lgth, 0) ;
    user_close(fd1) ;
    user_close(fd2) ;


	    /********************************
	    *         Open Files            * 
	    ********************************/

    /*
     * Here is what we are building:
     *
     *     	    muxfd1		   muxfd2	fd1  fd2
     *		      |			      |		 |    |
     *		+-----------+		+-----------+    X    X
     *		|  mini-mux |		|  mini-mux |
     *		+-----------+		+-----------+
     *		      | muxid1		      | muxid2
     *		      |			      |
     *		      | (fd1)		      | (fd2)
     *		+-----------+		+-----------+
     *		|    loop   |<--------->|    loop   |
     *		+-----------+		+-----------+
     *
     * muxfd1 and muxfd2 are the control streams for the multiplxor.
     *
     * fd1 and fd1 can be closed with no impact on the multiplexor.
     * Since we used I_PLINK we can also close muxfd1 and muxfd2
     * and the mux will stay together.  However, when we do that
     * the memory stays allocated and we can't reach the streams
     * anymore due to the clone opens.
     */

    print("\nTest I_PLINK/I_PUNLINK\n") ;
    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

    print("mux_test: open a mux-clone device\n") ;
    muxfd1 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd1 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd1)) ;
	xit() ;
    }

    print("mux_test: open another mux-clone device\n") ;
    muxfd2 = user_open(MUX_CLONE, O_RDWR, 0) ;
    if (muxfd2 < 0)
    {
	print("mux_clone: %s\n", strerror(-muxfd2)) ;
	xit() ;
    }

    print("mux_test: I_PLINK loop driver under mux (muxfd1->fd1)\n") ;
    muxid1 = user_ioctl(muxfd1, I_PLINK, fd1) ;	/* link fd1 below muxfd1 */
    if (muxid1 < 0)
    {
	print("mux_clone: I_PLINK: %s\n", strerror(-muxid1)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid1) ;

    print("mux_test: I_PLINK loop driver under mux (muxfd2->fd2)\n") ;
    muxid2 = user_ioctl(muxfd2, I_PLINK, fd2) ;	/* link fd2 below muxfd2 */
    if (muxid2 < 0)
    {
	print("mux_clone: I_PLINK: %s\n", strerror(-muxid2)) ;
	xit() ;
    }
    else
	print("                   muxid=%d\n", muxid2) ;

    print_stream(muxfd1) ;
    print_stream(muxfd2) ;
    print("mux_test: leave open files to loop driver\n") ;

    print("mux_test: send data down through the mux/loop and read back\n") ;
    strcpy(buf, "Test data for sending through the mini-mux") ;
    lgth = strlen(buf) ;
    do_get_put(muxfd1, muxfd2, -1, lgth, 0) ;

    print("mux_test: I_PUNLINK muxid1 from muxfd1\n") ;
    rslt = user_ioctl(muxfd1, I_PUNLINK, muxid1) ;
    if (rslt < 0)
    {
	print("minimux: I_PUNLINK: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("mux_test: I_PUNLINK muxid2 from muxfd2\n") ;
    rslt = user_ioctl(muxfd2, I_PUNLINK, muxid2) ;
    if (rslt < 0)
    {
	print("minimux: I_PUNLINK: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    print("mux_test: send data down through the loop and read back\n") ;
    do_get_put(fd1, fd2, -1, lgth, 0) ;

	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nmux_test: close control streams\n") ;
    user_close(muxfd1) ;
    user_close(muxfd2) ;
    print("mux_test: send data down through the loop and read back\n") ;
    do_get_put(fd1, fd2, -1, lgth, 0) ;
    user_close(fd1) ;
    user_close(fd2) ;

} /* mux_test */

/************************************************************************
*                           clone_test                                  *
*************************************************************************
*									*
* Test opening and closing clone drivers in which some minors have	*
* visible directory entries and others don't.				*
*									*
************************************************************************/
void	clone_test(void)
{
    int			fd1 ;
    int			fd2 ;
    int			fd3 ;
    int			fd4 ;
    int			fd5 ;
    int			rslt ;

    print("\nclone open test\n") ;

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing at start of test\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
#endif

    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

    rslt = open_clones(&fd3, &fd4) ;
    if (rslt < 0) xit() ;

    fd5 = user_open(LOOP_1, O_RDWR, 0) ;
    if (fd5 < 0) xit() ;

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing after opens\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
    user_print_inodes() ;
#endif

    user_close(fd1) ;
    user_close(fd2) ;
    user_close(fd3) ;
    user_close(fd4) ;
    user_close(fd5) ;

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing after closes\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
    user_print_inodes() ;
#endif

    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

    rslt = open_clones(&fd3, &fd4) ;
    if (rslt < 0) xit() ;

    fd5 = user_open(LOOP_1, O_RDWR, 0) ;
    if (fd5 < 0) xit() ;

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing after 2nd round of opens\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
    user_print_inodes() ;
#endif

    user_close(fd1) ;
    user_close(fd2) ;
    user_close(fd3) ;
    user_close(fd4) ;
    user_close(fd5) ;

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing after 2nd round of closes\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
    user_print_inodes() ;
#endif

} /* clone_test */

/************************************************************************
*                          bufcall_test                                 *
************************************************************************/
void bufcall_test(void)
{
    int			fd1 ;
    int			fd2 ;
    int			rslt ;
    int			lgth ;
    int			lgth2 ;
    struct strioctl	ioc ;

    print("\nbufcall test\n") ;

	    /********************************
	    *         Open Files            * 
	    ********************************/

    rslt = open_clones(&fd1, &fd2) ;
    if (rslt < 0) xit() ;

	    /********************************
	    *         putmsg/read           *
	    ********************************/

    print("\nUse putmsg to send data, use read to read back data\n") ;
    memset(rdctlbuf, 0, sizeof(rdctlbuf)) ;
    memset(rdbuf, 0, sizeof(rdbuf)) ;

    /*
     * Tell the loop driver to use bufcall to drive the service
     * queue enable.
     */
    ioc.ic_timout = 10 ;
    ioc.ic_dp	  = NULL;
    ioc.ic_cmd 	  = LOOP_BUFCALL ;	/* use bufcall to enable svcq */
    ioc.ic_len	  = 0 ;
    rslt = user_ioctl(fd1, I_STR, &ioc) ;
    if (rslt < 0)
    {
	print("loop_clone.1: ioctl LOOP_BUFCALL: %s\n", strerror(-rslt)) ;
    }

    strcpy(buf, "Test data for bufcall putmsg/read") ;
    lgth = strlen(buf) ;
    wr_ctl.len	= -1 ;
    wr_dta.len	= lgth ;
    if (put_msg(fd1, &wr_ctl, &wr_dta, 0, MSG_BAND) < 0) xit() ;

    rslt = user_read(fd2, rdbuf, lgth);
    if (rslt < 0)
    {
	print("loop_clone.2: read: %s\n", strerror(-rslt)) ;
	xit() ;
    }

    if (rslt != lgth)
    {
	print("loop_clone.2:  read returned %d, expected %d\n", rslt, lgth) ;
	xit() ;
    }

    if (strcmp(buf, rdbuf))
    {
	print("loop_clone.2: read: buffer compare error\n") ;
	print("              wrote \"%s\"\n", buf) ;
	print("              read  \"%s\"\n", rdbuf) ;
    }
    else
	print("loop_clone.2: read %d bytes: buffer compared OK\n", rslt) ;




	    /********************************
	    *         Close Files           * 
	    ********************************/

    print("\nbufcall_test: closing files\n") ;
    user_close(fd1) ;
    user_close(fd2) ;

} /* bufcall_test */

/************************************************************************
*                              main                                     *
************************************************************************/
void test(void)
{
    extern long		lis_mem_alloced ;		/* from port.c */

    print("\nBegin STREAMS test\n\n") ;

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing:\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
#endif
    print("Memory allocated = %ld\n", lis_mem_alloced) ;

    set_debug_mask(0x0FFFFFFF) ;

    open_close_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("open/close test") ;

    ioctl_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("ioctl test") ;

    rdopt_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("read options test") ;

    write_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("write test") ;

    close_timer_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("close timer test") ;

    putmsg_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("putmsg test") ;

    poll_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("poll test") ;

    mux_test() ;
    print_mem() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
    wait_for_logfile("multiplexor test") ;

    clone_test() ;
    wait_for_logfile("clone driver test") ;

    bufcall_test() ;
    wait_for_logfile("bufcall test") ;

#ifdef DIRECT_USER
    print("\n\n\nDirectory listing:\n\n") ;
    user_print_dir(NULL, USR_PRNT_INODE) ;
#endif
}

/************************************************************************
*                         tst_print_trace                               *
*************************************************************************
*									*
* This routine is pointed to by the 'lis_print_trace' pointer in the	*
* cmn_err module when the test program is linked directly with STREAMS.	*
*									*
************************************************************************/
void	tst_print_trace (char *bfrp)
{
    print("%s", bfrp) ;

} /* tst_print_trace  */


/************************************************************************
*                           main                                        *
************************************************************************/
void main(void)
{
    extern long		lis_mem_alloced ;		/* from port.c */

#ifdef DIRECT_USER
    lis_print_trace = tst_print_trace ;
#endif

    register_drivers() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;

#ifndef LINUX
    make_nodes() ;
    print("Memory allocated = %ld\n", lis_mem_alloced) ;
#endif

#ifndef DIRECT_USER
    printk_fd = user_open(NPRINTK, O_RDWR, 0) ;
    if (printk_fd < 0)
    {
	printf( NPRINTK ": %s\n", strerror(-printk_fd)) ;
	xit() ;
    }
#endif

    test() ;
/*     test() ; */
}
