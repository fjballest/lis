/*                               -*- Mode: C -*- 
 * stats.c --- streams statistics
 * Author          : Graham Wheeler
 * Created On      : Tue May 31 22:25:19 1994
 * Last Modified By: David Grothe
 * RCS Id          : $Id: stats.c,v 1.1 1995/12/19 15:50:04 dave Exp $
 * Purpose         : provide some stats for LiS
 * ----------------______________________________________________
 *
 *   Copyright (C) 1995  Graham Wheeler
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
 *    You can reach me by email to
 *    gram@aztec.co.za
 */


/*  -------------------------------------------------------------------  */
/*				 Dependencies                            */

#include <sys/strport.h>
#include <sys/strconfig.h>	/* config definitions */
#include <sys/LiS/share.h>	/* streams shared defs*/
#include <sys/LiS/stats.h>	/* module interface */

/*  -------------------------------------------------------------------  */
/*			   Local functions & macros                      */

/*  -------------------------------------------------------------------  */
/*				  Glob. Vars                             */

/* This are the stats 
 */
unsigned long lis_strstats[STRMAXSTAT][4] ;

/* If in user mode, include a stats dump routine 
 */
#ifdef MEMPRINT

itemname_t lis_itemnames[] =
{
    {MEMALLOCS,		MEMALLOCSSTR},
    {MEMFREES,		MEMFREESSSTR},
    {MEMALLOCD,		MEMALLOCDSTR},
    {MEMLIM,		MEMLIMSTR},
    {HEADERS,		HEADERSSTR},
    {FREEHDRS,		FREEHDRSSTR},
    {DATABS,		DATABSSTR},
    {DBLKMEM,		DBLKMEMSTR},
    {MSGMEMLIM,		MSGMEMLIMSTR},
    {BUFCALLS,		BUFCALLSSTR},
    {MSGSQD,		MSGSQDSTR},
    {MSGQDSTRHD,	MSGQDSTRHDSTR},
    {CANPUTS,		CANPUTSSTR},
    {QUEUES,		QUEUESSTR},
    {QSCHEDS,		QSCHEDSSTR}
};
char *lis_countnames[] =
{
    CURRENTSTR, TOTALSTR, MAXIMUMSTR, FAILURESSTR
};
#endif

/*  -------------------------------------------------------------------  */
/*			Exported functions & macros                      */

/* increment count for one item
 * STATUS: complete, untested
 */
void 
LisUpCounter(int item, int n)
{
    lis_strstats[item][TOTAL] += n;
    lis_strstats[item][CURRENT] += n;
    if (lis_strstats[item][CURRENT] > lis_strstats[item][MAXIMUM])
	lis_strstats[item][MAXIMUM] = lis_strstats[item][CURRENT];
}/*LisUpCounter*/

/*  -------------------------------------------------------------------  */
/* If in USER mode, include a stats dump routine 
 */
#ifdef MEMPRINT
void
LisShowStrStats(unsigned long (*strstats)[STRMAXSTAT][4])
{
    int i;
    int inx;

    /* Print heading */
    printf("%-28s  ", "");
    for (i = 0; i < 4; i++)
	printf("%12s", lis_countnames[i]) ;
    printf("\n");

    /* Print statistics */
    for (i = 0; i < STRMAXSTAT; i++)
    {
	int j;

	if (lis_itemnames[i].name == NULL) continue ;

	inx = lis_itemnames[i].stats_inx ;
	printf("%-28s: ", lis_itemnames[i].name);
	for (j = 0; j < 4; j++)
	    printf("%12lu", (*strstats)[inx][j]);
	printf("\n");
    }
}/*LisShowStrStats*/
#endif /* MEMPRINT */

/*----------------------------------------------------------------------
# Local Variables:      ***
# change-log-default-name: "~/src/prj/streams/src/NOTES" ***
# End: ***
  ----------------------------------------------------------------------*/
