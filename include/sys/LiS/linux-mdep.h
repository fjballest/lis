/*                               -*- Mode: C -*- 
 * mdep.h --- machine (actually kernel) dependencies.
 * Author          : Francisco J. Ballesteros
 * Created On      : Tue May 31 21:40:37 1994
 * Last Modified By: David Grothe
 * RCS Id          : $Id: linux-mdep.h,v 1.15 1996/01/27 00:40:26 dave Exp $
 * Purpose         : provide kernel independence as much as possible
 *                 : This could be also considered to be en embryo for
 *                 : dki stuff,i.e. linux-dki
 * ----------------______________________________________________
 *
 *    Copyright (C) 1995  Francisco J. Ballesteros, Denis Froschauer
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
 *    You can reach su by email to any of
 *    nemo@ordago.uc3m.es, 100741.1151@compuserve.com
 *    dave@gcom.com
 */

#ifndef _LIS_M_DEP_H
#define _LIS_M_DEP_H 1
/*  -------------------------------------------------------------------  */
/*				 Dependencies                            */

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

/* kernel includes go here */
#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>        /* common system types */
#include <linux/kdev_t.h>	/* 1.3.xx needs this */
#include <linux/sched.h>	/* sleep,wake,... */
#include <linux/wait.h>
#include <linux/config.h>
#include <linux/kernel.h>	/* suser,...*/
#include <linux/interrupt.h>
#include <linux/major.h>
#include <linux/fs.h>		/* inodes,... */
#include <linux/fcntl.h>	/* inodes,... */
#include <linux/string.h>	/* memcpy,... */
#include <linux/timer.h>	/* timers */
#include <linux/mm.h>		/* memory manager, pages,... */
#include <linux/malloc.h>	/* memory manager, pages,... */
#include <linux/stat.h>		/* S_ISCHR */
#include <asm/segment.h>	/* memcpy_{to,from}_fs */
#include <asm/system.h>		/* sti,cli */
#include <sys/errno.h>	      /* for errno */
#include <sys/signal.h>	      /* for signal numbers */
#endif /* __KERNEL__ */

/*  -------------------------------------------------------------------  */

/* some missing symbols
 */

#ifdef __KERNEL__
#define SECS_TO(t)	(t)	/* pass secs to system tmout time units */

extern long lis_time_till(long target_time);
extern long lis_target_time(long milli_sec);
#endif				/* __KERNEL__ */

/* some missing generic types 
 */
#undef uid 
#undef gid
typedef int     o_uid_t;
typedef int     o_gid_t;
typedef unsigned   char uchar;
typedef struct cred {
	uid_t	cr_uid;			/* effective user id */
	gid_t	cr_gid;			/* effective group id */
	uid_t	cr_ruid;		/* real user id */
	gid_t	cr_rgid;		/* real group id */
} cred_t;


#ifdef __KERNEL__
#define lis_suser(fp)	suser()
#endif				/* __KERNEL__ */

/*  -------------------------------------------------------------------  */


#ifdef __KERNEL__
#define lis_free_page(cp) free_page((unsigned long)(cp))

#define PRINTK		printk

/* should well-define this...
 */
#define ASSERT(x)

/* disable/enable interrupts
 */
#define SPLSTR(x)	{ save_flags(x) ; cli() ; }	/* save intr state */
#define SPLX(x)		restore_flags(x)		/* restore intr state */
#define	SPL0(x)		sti()				/* ignore arg */

/*
 * lis_up and lis_down (Linux semaphores)
 */
extern int	lis_down(struct semaphore *sem) ;	/* in linux-mdep.c */
#define	lis_up	up			/* same as system 'up' */

/* lock inodes...
 */
#define	LOCK_INO(i)	lis_down(&i->i_sem)
#define	ULOCK_INO(i)	lis_up(&i->i_sem)

/*
 * Access inode field of file structure.
 */
#define STR_FILE_INODE(f)	(f)->f_inode

/*
 * Clone driver support
 */
extern struct inode 	*lis_pick_inode(struct inode *old,
					struct inode *new,
					dev_t         dev);

/*
 * Initialize a semaphore
 */
#ifdef VERSION_2
#define SEM_INIT(sem_addr,cntr)						\
			    {						\
				(sem_addr)->count	= cntr ;	\
				(sem_addr)->waking	= 0 ;		\
				(sem_addr)->lock	= 0 ;		\
				(sem_addr)->wait	= NULL ;	\
			    }
#else
#define SEM_INIT(sem_addr,cntr)						\
			    {						\
				(sem_addr)->count	= cntr ;	\
				(sem_addr)->wait	= NULL ;	\
			    }
#endif
#define	SEM_DESTROY(sem_addr)		/* no such function in Linux */

/* Use Linux system macros for MAJOR and MINOR */
#define	STR_MAJOR		MAJOR
#define	STR_MINOR		MINOR
#endif				/* __KERNEL__ */

/************************************************************************
*                            major/minor                                *
*************************************************************************
*									*
* Macros to extract the major and minor device numbers from a dev_t	*
* variable.								*
*									*
************************************************************************/

/*
 * Major and minor macros come from linux ./include/linux/kdev_t.h
 *
 * If sysmacros.h has been included it defines major and minor in
 * the old way.  We want the new way so we undefine them and redefine
 * them to use the kdev_t style.
 */
#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif
#ifdef makedevice
#undef makedevice
#endif

#ifndef _SYS_SYSMACROS_H
#define _SYS_SYSMACROS_H		/* pretend sysmacros.h included */
#endif

#define	major(dev_t_var)	MAJOR(dev_t_var)
#define	minor(dev_t_var)	MINOR(dev_t_var)
#define makedevice(majornum,minornum)	MKDEV(majornum,minornum)

typedef unsigned long	major_t ;	/* mimics SVR4 */
typedef unsigned long	minor_t ;	/* mimics SVR4 */

/*
 * There is no STREAMS FIFO implemented as yet.  Therefore, we declare
 * its major number here as an impossible value.
 */
#define	LIS_FIFO	(-1)



#ifdef __KERNEL__

#ifndef VOID
#define VOID	void
#endif

#define UID(fp)	  current->uid
#define GID(fp)	  current->gid
#define EUID(fp)  current->euid
#define EGID(fp)  current->egid
#define PGRP(fp)  current->pgrp
#define PID(fp)	  current->pid

#define OPENFILES()	current->files->count
#define SESSION()	current->session
#define FILE2INO(fd)	NULL
#define INO_STR(i)	((struct stdata *) ((i)->u.generic_ip))
#define DBLK_ALLOC(n,f,l)	lis_malloc(n,GFP_ATOMIC | GFP_DMA,f,l)
#define ALLOC(n)		lis_malloc(n,GFP_ATOMIC,__FILE__,__LINE__)
#define ALLOCF(n,f)		lis_malloc(n,GFP_ATOMIC, f __FILE__,__LINE__)
#define MALLOC(n)		lis_malloc(n,GFP_ATOMIC,__FILE__,__LINE__)
#define LISALLOC(n,f,l)		lis_malloc(n,GFP_ATOMIC,f,l)
#define FREE(p)			lis_free(p,__FILE__,__LINE__)
#define	KALLOC(n,c)		kmalloc(n,c)
#define	KFREE(p)		kfree(p)
#define MEMCPY(dest, src, len)	memcpy(dest, src, len)
#define PANIC(msg)		panic(msg)

struct stdata	*lis_fd2str(int fd) ;	/* file descr -> stream */

#endif				/* __KERNEL__ */

/*  -------------------------------------------------------------------  */

/* This should be entry points from the kernel into LiS
 * kernel should be fixed to call them when appropriate.
 */

/* some kernel memory has been free'd 
 * tell STREAMS
 */
#ifdef __KERNEL__
extern void
lis_memfree( void );

/* Get avail kernel memory size
 */
#define lis_kmemavail()	((unsigned long)-1) /* lots of mem avail :) */

#endif				/* __KERNEL__ */

/*  -------------------------------------------------------------------  */
/* This will copyin usr string pointed by ustr and return the result  in
 * *kstr. It will stop at  '\0' or max bytes copyed in.
 * caller should call free_page(*kstr) on success.
 * Will return 0 or errno
 */
#ifdef __KERNEL__
int 
lis_copyin_str(struct file *fp, const char *ustr, char **kstr, int max);

/* Just another copy in / out
 */
#define lis_copyin(fp,kbuf,ubuf,len)	memcpy_fromfs(kbuf,ubuf,len)
#define lis_copyout(fp,kbuf,ubuf,len)	memcpy_tofs(ubuf,kbuf,len)

/* check a user memory area
 */
#define lis_check_umem(fp,f,p,l)	verify_area(f,p,l)

#endif				/* __KERNEL__ */

/*  -------------------------------------------------------------------  */

/*
 * The routine 'lis_runqueues' just requests that the queues be run
 * at a later time.  A daemon process runs the queus with the help
 * of a special driver.  This driver has the routine lis_setqsched
 * in it.  See drivers/str/runq.c.
 */
#ifdef __KERNEL__
extern void	lis_setqsched(void) ;
#define	lis_runqueues		lis_setqsched
#endif				/* __KERNEL__ */

/*  -------------------------------------------------------------------  */

/*
 * The routine 'lis_select' handles select calls from the Linux kernel.
 * The structure 'lis_select_t' is embedded in the stdata structure
 * and contains the wait queue head.
 */
#ifdef __KERNEL__

typedef struct lis_select_struct
{
    struct wait_queue	*sel_wait ;

} lis_select_t ;

extern int	lis_select(struct inode *inode, struct file *file,
			   int sel_type, select_table *wait) ;

extern void	lis_select_wakeup(struct stdata *hd) ;

#endif				/* __KERNEL__ */

/*  -------------------------------------------------------------------  */




#endif /*!__LIS_M_DEP_H*/


/*----------------------------------------------------------------------
# Local Variables:      ***
# change-log-default-name: "~/src/prj/streams/src/NOTES" ***
# End: ***
  ----------------------------------------------------------------------*/
