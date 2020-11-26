/*                               -*- Mode: C -*- 
 * <strport.h> --- Linux STREAMS portability declarations. 
 * Author          : gram & nemo
 * Created On      : Fri Mar 24 2:40:21 1995
 * RCS Id          ; $Id: strport.h,v 1.4 1996/01/20 17:00:53 dave Exp $
 * Last Modified By: David Grothe
 * Restrictions    : SHAREd items can be read/writen by usr
 *                 : EXPORTed items can only be read by usr
 *                 : PRIVATEd items cannot be read nor writen
 * Purpose         : All system dependent stuff goes here. The idea
 *                 : is that different versions of this file can be
 *                 : used to port STREAMS to other operating systems
 *                 : as well as providing a user-space testbed environment.
 *
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

#ifndef _STRPORT_H
#define _STRPORT_H

/*  *******************************************************************  */
/*                               Dependencies                            */

#if	defined( __MSDOS__)
#include <sys/LiS/dos-mdep.h>
#elif defined(LINUX)
#include <sys/LiS/linux-mdep.h>
#elif defined(USER)
#include <sys/LiS/user-mdep.h>
#elif defined (QNX)
#include <sys/LiS/qnx-mdep.h>
#elif defined(SYS_SCO)
#include <sys/LiS/sco-mdep.h>
#elif defined(SYS_54)
#include <sys/LiS/sys54-mdep.h>
#elif defined(PORTABLE)
#include <sys/LiS/port-mdep.h>
#endif /* !__MSDOS__ */

#ifndef OPENFAIL
#define OPENFAIL	(-1)
#endif
#ifndef INFPSZ
#define INFPSZ		(-1)
#endif

#ifdef __KERNEL__
extern char	*lis_errmsg(int lvl) ;
extern void	*lis_malloc(int nbytes, int class, char *file_name,int line_nr);
extern void	 lis_free(void *ptr, char *file_name,int line_nr);
#endif				/* __KERNEL__ */

#endif /* _STRPORT_H */


/*----------------------------------------------------------------------
# Local Variables:      ***
# change-log-default-name: "~/src/prj/streams/src/NOTES" ***
# End: ***
  ----------------------------------------------------------------------*/
