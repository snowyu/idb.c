/*
@(#)File:           $RCSfile: sysstat.h,v $
@(#)Version:        $Revision: 1.8 $
@(#)Last changed:   $Date: 2008/06/28 03:15:20 $
@(#)Purpose:        POSIX-compliant cover for <sys/stat.h> for Windows
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1995,1997,2001,2005,2008
@(#)Product:        SQLCMD Version 88.00 (2012-07-19)
*/

/*TABSTOP=4*/

#ifndef SYSSTAT_H
#define SYSSTAT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef MAIN_PROGRAM
#ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
const char jlss_id_sysstat_h[] = "@(#)$Id: sysstat.h,v 1.8 2008/06/28 03:15:20 jleffler Exp $";
#endif /* lint */
#endif	/* MAIN_PROGRAM */

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER                 /* MS <sys/stat.h> uses this trigger */
typedef unsigned short mode_t;  /* POSIX defines mode_t in <sys/types.h> */
#endif /* _MSCVER */

#ifndef S_IRWXU

/* POSIX-compliant defines omitted from sys/stat.h. */
/* Assume UNIX Version 7 header, more or less       */

#ifndef S_IREAD
#define S_IREAD     0400
#endif /* S_IREAD */ 
#ifndef S_IWRITE
#define S_IWRITE    0200
#endif /* S_IWRITE */ 
#ifndef S_IEXEC
#define S_IEXEC     0100
#endif /* S_IEXEC */

#define S_IRUSR (S_IREAD)
#define S_IWUSR (S_IWRITE)
#define S_IXUSR (S_IEXEC)
#define S_IRGRP (S_IRUSR >> 3)
#define S_IWGRP (S_IWUSR >> 3)
#define S_IXGRP (S_IXUSR >> 3)
#define S_IROTH (S_IRUSR >> 6)
#define S_IWOTH (S_IWUSR >> 6)
#define S_IXOTH (S_IXUSR >> 6)
#define S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR) 
#define S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP) 
#define S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH) 

#ifdef _MSC_VER                 /* MS <sys/stat.h> uses this trigger */
/* MS Windows does not support SUID, SGID or sticky bit */
#ifndef S_ISUID
#define S_ISUID 0
#endif
#ifndef S_ISGID
#define S_ISGID 0
#endif
#ifndef S_ISVTX
#define S_ISVTX 0
#endif
#else
#ifndef S_ISUID
#define S_ISUID 04000
#endif
#ifndef S_ISGID
#define S_ISGID 02000
#endif
#ifndef S_ISVTX
#define S_ISVTX 01000
#endif
#endif /* _MSCVER */

#endif /* S_IRWXU */

/*
** Assume the S_IFMT, S_IFDIR, S_IFCHR, S_IFREG are defined, which is OK
** for Windows under MS Visual Studio 2008 Express.  Conditional testing
** not perfect: it assumes pragmatically that S_ISFIFO() is undefined if
** S_IFIFO() is undefined, for example.
*/

/* POSIX file type test macros */
#ifndef S_ISDIR
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif /* S_ISDIR */
#ifndef S_ISCHR
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#endif /* S_ISCHR */
#ifndef S_ISREG
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif /* S_ISREG */

/* Not in MS Visual Studio 2008 Express */
#ifndef S_IFBLK
#ifndef S_ISBLK
  #define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#endif /* S_ISBLK */
#define S_IFBLK(m)  (0)
#endif /* S_IFBLK */

/* Not in Version 7 Unix, but we'll define it anyway */
#ifndef S_IFIFO
#ifndef S_ISFIFO
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#endif /* S_ISFIFO */
#define S_IFFIFO(m)	(0)
#endif /* S_IFIFO */

/* Not in POSIX 1003.1-1990, but we'll define it anyway */
#ifndef S_IFLNK
#ifndef S_ISLNK
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#endif /* S_ISLNK */
#define S_IFLNK(m)	(0)
#endif /* S_IFLNK */

/* Not in POSIX 1003.1-1990, but we'll define it anyway */
#ifndef S_IFSOCK
#ifndef S_ISSOCK
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
#endif /* S_ISSOCK */
#define S_IFSOCK(m)	(0)
#endif /* S_IFSOCK */

#endif	/* SYSSTAT_H */
