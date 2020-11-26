/*                               -*- Mode: C -*- 
 * <stropts> --- STREAMS ops.
 * Author          : gram & nemo
 * Created On      : Mon Oct 17 11:37:12 1994
 * RCS Id          ; $Id: stropts.h,v 1.4 1996/01/29 17:36:02 dave Exp $
 * Last Modified By: David Grothe
 *                 :
 *    Copyright (C) 1995  Graham Wheeler, Francisco J. Ballesteros
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *    You can reach us by email to any of
 *    gram@aztec.co.za, nemo@ordago.uc3m.es
 */

#ifndef _STR_OPTS_H
#define _STR_OPTS_H

/*  *******************************************************************  */
/*                               Dependencies                            */

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif
#ifndef _LIS_CONFIG_H
#include <sys/strconfig.h>
#endif


/*  *******************************************************************  */
/*                                 Symbols                               */

/*
 *  timeout for enernity
 */

#define INFTIM          -1

/*  *******************************************************************  */

/*
 * Write opts.
 */

#define SNDZERO         0x001           /* send a zero length message */
#define SNDPIPE         0x002           /* send SIGPIPE on write and */
                                        /* putmsg if sd_werror is set */

/*
 *  Read opts that can be defined with SO_READOPT (so_readopt values)
 */

#define RNORM           0x0000  /* byte stream */
#define RMSGD           0x0001  /* message discard */
#define RMSGN           0x0002  /* message non-discard */

#define RMODEMASK       0x0003  /* RMODE bits */

/* As Solaris' guys say:
 * "These next three read options are added for the sake of
 * user-level transparency.  RPROTDAT will cause the stream head
 * to treat the contents of M_PROTO and M_PCPROTO message blocks
 * as data.  RPROTDIS will prevent the stream head from failing
 * a read with EBADMSG if an M_PROTO or M_PCPROTO message is on
 * the front of the stream head read queue.  Rather, the protocol
 * blocks will be silently discarded and the data associated with
 * the message (in linked M_DATA blocks), if any, will be delivered
 * to the user.  RPROTNORM sets the default behavior, where read
 * will fail with EBADMSG if an M_PROTO or M_PCPROTO are at the
 * stream head."
 *
 * For compatibility, we use the same bits as SVR4.
 */

#define RPROTDAT        0x0004  /* turn M*PROTO into M_DATA */
#define RPROTDIS        0x0008  /* discard M*PROTO */
#define RPROTNORM       0x0010  /* normal proto */

#define RPROTMASK       0x001C  /* RPROT bits */

/*  *******************************************************************  */

/*
 *  flags for M_FLUSH
 */

#define FLUSHR          0x01    /* flush read side */
#define FLUSHW          0x02    /* flush write side */
#define FLUSHRW         0x03    /* flush both read & write sides */
#define FLUSHBAND       0x10    /* flush msg for band priority */

/*  *******************************************************************  */

/*
 *  events for SIGPOLL to be sent
 */

#define S_INPUT         0x0001          /* any msg but hipri on read Q */
#define S_HIPRI         0x0002          /* high priority msg on read Q */
#define S_OUTPUT        0x0004          /* write Q no longer full */
#define S_MSG           0x0008          /* signal msg at front of read Q */
#define S_ERROR         0x0010          /* error msg arrived at stream head */
#define S_HANGUP        0x0020          /* hangup msg arrived at stream head */
#define S_RDNORM        0x0040          /* normal msg on read Q */
#define S_WRNORM        S_OUTPUT
#define S_RDBAND        0x0080          /* out of band msg on read Q */
#define S_WRBAND        0x0100          /* can write out of band */
#define S_BANDURG       0x0200          /* modifier to S_RDBAND, to generate */
                                        /* SIGURG instead of SIGPOLL */
#define S_ALL		0x03FF		/* every event */

/*  *******************************************************************  */

/*
 * Stream buffer structure for putpmsg and getpmsg system calls
 */

typedef
struct strbuf {
    int     maxlen;                 /* no. of bytes in buffer */
    int     len;                    /* no. of bytes returned */
    char    *buf;                   /* pointer to data */
} strbuf_t;

/*  *******************************************************************  */

/*
 * User level routines for getmsg/putmsg
 */
#ifdef LINUX

# ifndef _LINUX_UNISTD_H
# include <linux/unistd.h>
# endif

static inline _syscall5(int,putpmsg,int,fd,
			void *,ctlptr,void *,dataptr,int,band,int,flags)
static inline _syscall5(int,getpmsg,int,fd,
			void *,ctlptr,void *,dataptr,int *,bandp,int *,flagsp)

#else

int	putpmsg(int fd, struct strbuf *ctlptr,
			struct strbuf *dataptr,
			int band,
			int flags) ;
int	getpmsg(int fd, struct strbuf *ctlptr,
			struct strbuf *dataptr,
			int *bandp,
			int *flagsp) ;

#endif

#define	putmsg(f,c,d,g)	putpmsg(f,c,d,-1,g)
#define	getmsg(f,c,d,g)	getpmsg(f,c,d,NULL,g)

#ifdef QNX
#ifndef GCOM_OPEN
#define open gcom_open
#endif
extern int gcom_open( const char *__path, int __oflag, ... );
#endif
/*
 * Flags for getmsg() and putmsg() syscall arguments.
 * A value of zero will cause getmsg() to return
 * the first message on the stream head read queue and putpmsg() to send
 * a normal priority message.
 */
#define RS_HIPRI        0x01    /* high priority message */
#define RS_NORM         0x00    /* normal message */

/*  *******************************************************************  */

/* Flags for getpmsg() and putpmsg() syscall arguments.
 * These are settable by the user and will be set on return
 * to indicate the priority of message received.
 */
#define MSG_HIPRI       0x01            /* send/recv high priority message */
#define MSG_ANY         0x02            /* recv any messages */
#define MSG_BAND        0x04            /* recv messages from specified band */

/*  *******************************************************************  */

/* Flags returned as value of getmsg() and getpmsg() syscall.
 */
#define MORECTL         1               /* more ctl info is left in message */
#define MOREDATA        2               /* more data is left in message */


/*  *******************************************************************  */

/*
 * Define to indicate that all multiplexors beneath a stream should
 * be unlinked.
 */

#define MUXID_ALL       (-1)

/*  *******************************************************************  */

/*
 *  Flag definitions for the I_ATMARK ioctl.
 */

#define ANYMARK         0x01
#define LASTMARK        0x02

/*  *******************************************************************  */

/*
 *   STREAMS Ioctls
 */

#define STRIOC          ('S'<<8)
#define STR		STRIOC
#define I_NREAD         (STRIOC | 1)
#define I_PUSH          (STRIOC | 2)
#define I_POP           (STRIOC | 3)
#define I_LOOK          (STRIOC | 4)
#define I_FLUSH         (STRIOC | 5)
#define I_SRDOPT        (STRIOC | 6)
#define I_GRDOPT        (STRIOC | 7)
#define I_STR           (STRIOC | 8)
#define I_SETSIG        (STRIOC | 9)
#define I_GETSIG        (STRIOC | 10)
#define I_FIND          (STRIOC | 11)
#define I_LINK          (STRIOC | 12)
#define I_UNLINK        (STRIOC | 13)
#define I_PEEK          (STRIOC | 14)
#define I_FDINSERT      (STRIOC | 15)
#define I_SENDFD        (STRIOC | 16)
#define I_RECVFD        (STRIOC | 17)
#define I_SWROPT        (STRIOC | 18)
#define I_GWROPT        (STRIOC | 19)
#define I_LIST          (STRIOC | 20)
#define I_PLINK         (STRIOC | 21)
#define I_PUNLINK       (STRIOC | 22)
#define I_SETEV         (STRIOC | 23)
#define I_GETEV         (STRIOC | 24)
#define I_STREV         (STRIOC | 25)
#define I_UNSTREV       (STRIOC | 26)
#define I_FLUSHBAND     (STRIOC | 27)
#define I_CKBAND        (STRIOC | 28)
#define I_GETBAND       (STRIOC | 29)
#define I_ATMARK        (STRIOC | 30)
#define I_SETCLTIME     (STRIOC | 31)
#define I_GETCLTIME     (STRIOC | 32)
#define I_CANPUT        (STRIOC | 33)
/*
 * The following ioctls are specific to this STREAMS implementation.
 */
#define I_LIS_GET_MAXMSGMEM (STRIOC | 248)
#define I_LIS_SET_MAXMSGMEM (STRIOC | 249)
#define I_LIS_GET_MAXMEM (STRIOC | 250)
#define I_LIS_SET_MAXMEM (STRIOC | 251)
#define I_LIS_GETSTATS  (STRIOC | 252)	/* see include/sys/LiS/stats.h */
#define I_LIS_PRNTSTRM  (STRIOC | 253)
#define	I_LIS_PRNTMEM	(STRIOC | 254)
#define I_LIS_SDBGMSK   (STRIOC | 255)

/*  -------------------------------------------------------------------  */
/*                                  Types                                */

/*
 * I_STR ioctl user data
 */

typedef
struct strioctl {
    int     ic_cmd;                 /* command */
    int     ic_timout;              /* timeout value */
    int     ic_len;                 /* length of data */
    char    *ic_dp;                 /* pointer to data */
} strioctl_t;

/*  *******************************************************************  */

/*
 *  I_PEEK ioctl
 */

typedef
struct strpeek {
    struct strbuf   ctlbuf;
    struct strbuf   databuf;
    long            flags;
} strpeek_t;
/*  *******************************************************************  */

/*
 * Stream I_FDINSERT ioctl format
 */

typedef
struct strfdinsert {
    struct strbuf   ctlbuf;
    struct strbuf   databuf;
    long            flags;
    int             fildes;
    int             offset;
} strfdinsert_t;

/*
 * Receive file descriptor structure
 */

typedef
struct strrecvfd {
    int fd;
    uid_t uid;
    gid_t gid;
    char fill[8];
} strrecvfd_t;

/*  *******************************************************************  */

/*
 *  I_LIST ioctl.
 */

typedef
struct str_mlist {
    char l_name[FMNAMESZ+1];
} str_mlist_t;

typedef
struct str_list {
    int               sl_nmods;
    struct str_mlist *sl_modlist;
} str_list_t;

/*  *******************************************************************  */

/*
 * For I_FLUSHBAND ioctl.  Describes the priority
 * band for which the operation applies.
 */

typedef
struct bandinfo {
    unsigned char   bi_pri;
    int             bi_flag;
} bandinfo_t;

/*  *******************************************************************  */
/*                         Shared global variables                       */



/*  *******************************************************************  */

#endif /*!_STR_OPTS_H*/

/*----------------------------------------------------------------------
# Local Variables:      ***
# change-log-default-name: "~/src/prj/streams/src/NOTES" ***
# End: ***
  ----------------------------------------------------------------------*/
