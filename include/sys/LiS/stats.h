/*                               -*- Mode: C -*- 
 * stats.h --- streams statistics
 * Author          : Francisco J. Ballesteros, Graham Wheeler
 * Created On      : Tue May 31 21:40:37 1994
 * Last Modified By: David Grothe
 * RCS Id          : $Id: stats.h,v 1.3 1996/01/27 00:40:29 dave Exp $
 * Purpose         : provide statistics for LiS
 * ----------------______________________________________________
 *
 *   Copyright (C) 1995  Francisco J. Ballesteros, Graham Wheeler
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
 *    nemo@ordago.uc3m.es, gram@aztec.co.za
 */


#ifndef _STATS_H
#define _STATS_H 1

/*  -------------------------------------------------------------------  */
/*				 Dependencies                            */


/*  -------------------------------------------------------------------  */
/*				   Symbols                               */

/* accounted items for stats and their names
 */
#define HEADERS		0
#define FREEHDRS	1
#define DATABS		2
#define	MEMALLOCS	3			/* # times allocator called */
#define	MEMFREES	4			/* # times free called */
#define	MEMALLOCD	5			/* amount of memory */
#define	DBLKMEM		6			/* amt of mem allocated dblks */
#define	MSGSQD		7			/* # messages queued */
#define	MSGQDSTRHD	8			/* # msgs qd at strm hd */
#define	QUEUES		9			/* # queues allocated */
#define	CANPUTS		10			/* # canput calls */
#define	QSCHEDS		11			/* # queues scheduled to run */
#define	BUFCALLS	12			/* # bufcalls */
#define	MEMLIM		13			/* lis_max_mem */
#define	MSGMEMLIM	14			/* lis_max_msg_mem */
#define	STRMAXSTAT	15			/* largest slot */
#define HEADERSSTR      "In-Use Message Blocks"
#define FREEHDRSSTR     "Free Message Blocks"
#define DATABSSTR       "Data Blocks"
#define	MEMALLOCSSTR	"Memory Allocator Calls"
#define	MEMFREESSSTR	"Memory Free Calls"
#define	MEMALLOCDSTR	"Bytes Allocated"
#define	DBLKMEMSTR	"Bytes Allocated to D-Blks"
#define	MSGSQDSTR	"Messages Queued"
#define	MSGQDSTRHDSTR	"Messages Queued at Strm Hd"
#define	QUEUESSTR	"Queues Allocated"
#define	CANPUTSSTR	"Canput Calls"
#define	QSCHEDSSTR	"Scheduled Queues"
#define	BUFCALLSSTR	"Bufcalls"
#define	MEMLIMSTR	"Memory Limit"
#define	MSGMEMLIMSTR	"D-Blk Memory Limit"

/* per item statistics & their names
 */
#define CURRENT		0
#define TOTAL		1
#define MAXIMUM		2
#define FAILURES	3
#define CURRENTSTR	"Current"
#define TOTALSTR	"Total"
#define MAXIMUMSTR	"Maximum" 
#define FAILURESSTR	"Failures"


/*  -------------------------------------------------------------------  */
/*				    Types                                */


/*  -------------------------------------------------------------------  */
/*				 Glob. Vars.                             */

extern unsigned long lis_strstats[STRMAXSTAT][4]; /* the stats */

/* If in user mode, include a stats dump routine 
 */
#ifdef MEMPRINT

typedef struct
{
    int         stats_inx ;             /* index into lis_strstats */
    char        *name ;                 /* ASCII name */
} itemname_t ;


extern itemname_t  lis_itemnames[];
extern char       *lis_countnames[];
#endif


/*  -------------------------------------------------------------------  */
/*			Exported functions & macros                      */
#ifdef __KERNEL__
/* increment count for one item
 */
extern void LisUpCounter(int item, int n);

#define	LisUpCount(item)	LisUpCounter(item,1)

/* increment fail count for item
 */
#define LisUpFailCount(item)	lis_strstats[item][FAILURES]++

/* decrement count for item
 */
#define LisDownCount(item)	lis_strstats[item][CURRENT]--
#define LisDownCounter(item,n)	lis_strstats[item][CURRENT] -= (n)

#endif /* __KERNEL__ */

/* If in user mode, include a stats dump routine 
 */
#ifdef MEMPRINT
extern void
LisShowStrStats(unsigned long (*strstats)[STRMAXSTAT][4]);
#endif

/*  -------------------------------------------------------------------  */
#endif /*!_STATS_H*/

/*----------------------------------------------------------------------
# Local Variables:      ***
# change-log-default-name: "~/src/prj/streams/src/NOTES" ***
# End: ***
  ----------------------------------------------------------------------*/
