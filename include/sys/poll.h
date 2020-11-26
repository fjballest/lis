/*                               -*- Mode: C -*- 
 * <strpoll> --- poll
 * Author          : gram & nemo
 * Created On      : Mon Oct 17 11:37:12 1994
 * RCS Id          ; $Id: poll.h,v 1.3 1996/01/27 00:40:00 dave Exp $
 * Last Modified By: Francisco J. Ballesteros
 * Last Modified On: Tue Sep 26 15:40:02 1995
 * Update Count    : 6
 * Status          : Unknown, Use with caution!
 * Prefix(es)      : 
 * Requirements    :
 * Purpose         :
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

#ifndef _SYS_POLL_H
#define	_SYS_POLL_H

/*  -------------------------------------------------------------------  */
#ifndef _POLL_H
#include <sys/LiS/poll.h>	/* streams module symbols & types */
#endif


/*  -------------------------------------------------------------------  */
/*
 * Structure of file descriptor/event pairs supplied in
 * the poll arrays.
 *
 * Magic Garden calls this "struct pollfd", AT&T STREAMS Programers Guide
 * calls it "struct poll".
 */
typedef struct poll {
	int fd;				/* file desc to poll */
	short events;			/* events of interest on fd */
	short revents;			/* events that occurred on fd */
} pollfd_t;

/* KLUDGE ATTACK FROM MIKEL */
struct pollfd {
	int fd;				/* file desc to poll */
	short events;			/* events of interest on fd */
	short revents;			/* events that occurred on fd */
} ;
/* KLUDGE ATTACK FROM MIKEL */
/*
 * Testable select events
 */
#define	POLLIN		0x0001		/* fd is readable */
#define	POLLPRI		0x0002		/* high priority info at fd */
#define	POLLOUT		0x0004		/* fd is writeable (won't block) */
#define	POLLRDNORM	0x0040		/* normal data is readable */
#define	POLLWRNORM	POLLOUT
#define	POLLRDBAND	0x0080		/* out-of-band data is readable */
#define	POLLWRBAND	0x0100		/* out-of-band data is writeable */
#define	POLLMSG		0x0200		/* M_SIG at head of queue */

#define	POLLNORM	POLLRDNORM

/*
 * Non-testable poll events (may not be specified in events field,
 * but may be returned in revents field).
 */
#define	POLLERR		0x0008		/* fd has error condition */
#define	POLLHUP		0x0010		/* fd has been hung up on */
#define	POLLNVAL	0x0020		/* invalid pollfd entry */


#ifndef poll

# ifdef LINUX

#  ifndef _LINUX_UNISTD_H
#  include <linux/unistd.h>
#  endif

static inline _syscall3(int,poll,void *,pollbuf,long,n,int,timout)

# else

extern int poll(pollfd_t *fds, unsigned long nfds, int timeout);

# endif

#endif

/*  -------------------------------------------------------------------  */

#endif	/* _SYS_POLL_H */
