/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ISDK_CONFIG_H
#define __ISDK_CONFIG_H

#define HAVE_UNISTD_H
#define GNUC
#define _ANONYMOUS_STRUCT
#define HAVE_STRUCT_DIRENT_D_NAMLEN
#define HAVE_STRUCT_DIRENT_D_TYPE

//#ifdef __linux__
//  #define __USE_BSD 1
//  #include <sys/types.h>
//  #include <sys/stat.h>
//#endif
#include "deps/sysstat.h"    /* mode_t for Windows - <sys/types.h> for POSIX */
#include "deps/kbit.h"

#ifdef HAVE_STRUCT_DIRENT_D_NAMLEN
  #define DIR_NAME_LEN(aDirent) (aDirent->d_namlen)
#else
  #define DIR_NAME_LEN(aDirent) (strlen(aDirent->d_name))
#endif

#define DREALLOC_FUNC zrealloc
#define DFREE_FUNC zfree
#define CALLOC_FUNC(count, size) zcalloc((count)*(size))

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

/* x=target variable, y=mask */
#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK(x,y) ((x) & (y))

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

/* Define redis_fstat to fstat or fstat64() */
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
#define redis_fstat fstat64
#define redis_stat stat64
#else
#define redis_fstat fstat
#define redis_stat stat
#endif

/* Test for proc filesystem */
#ifdef __linux__
#define HAVE_PROC_STAT 1
#define HAVE_PROC_MAPS 1
#define HAVE_PROC_SMAPS 1
#endif

/* Test for task_info() */
#if defined(__APPLE__)
#define HAVE_TASKINFO 1
#endif

/* Test for backtrace() */
#if defined(__APPLE__) || defined(__linux__) || defined(__sun)
#define HAVE_BACKTRACE 1
#endif

/* Test for polling API */
#ifdef __linux__
#define HAVE_EPOLL 1
#endif

#if (defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#define HAVE_KQUEUE 1
#endif

#ifdef __sun
#include <sys/feature_tests.h>
#ifdef _DTRACE_VERSION
#define HAVE_EVPORT 1
#endif
#endif

/* Define aof_fsync to fdatasync() in Linux and fsync() for all the rest */
#ifdef __linux__
#define aof_fsync fdatasync
#else
#define aof_fsync fsync
#endif

/* Define rdb_fsync_range to sync_file_range() on Linux, otherwise we use
 * the plain fsync() call. */
#ifdef __linux__
#include <linux/version.h>
#include <features.h>
#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#if (LINUX_VERSION_CODE >= 0x020611 && __GLIBC_PREREQ(2, 6))
#define HAVE_SYNC_FILE_RANGE 1
#endif
#else
#if (LINUX_VERSION_CODE >= 0x020611)
#define HAVE_SYNC_FILE_RANGE 1
#endif
#endif
#endif

#ifdef HAVE_SYNC_FILE_RANGE
#define rdb_fsync_range(fd,off,size) sync_file_range(fd,off,size,SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE)
#else
#define rdb_fsync_range(fd,off,size) fsync(fd)
#endif

/* Byte ordering detection */
#include <sys/types.h> /* This will likely define BYTE_ORDER */

#ifndef BYTE_ORDER
#if (BSD >= 199103)
# include <machine/endian.h>
#else
#if defined(linux) || defined(__linux__)
# include <endian.h>
#else
#define	LITTLE_ENDIAN	1234	/* least-significant byte first (vax, pc) */
#define	BIG_ENDIAN	4321	/* most-significant byte first (IBM, net) */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long (pdp)*/

#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || \
   defined(vax) || defined(ns32000) || defined(sun386) || \
   defined(MIPSEL) || defined(_MIPSEL) || defined(BIT_ZERO_ON_RIGHT) || \
   defined(__alpha__) || defined(__alpha)
#define BYTE_ORDER    LITTLE_ENDIAN
#endif

#if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
    defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || \
    defined(MIPSEB) || defined(_MIPSEB) || defined(_IBMR2) || defined(DGUX) ||\
    defined(apollo) || defined(__convex__) || defined(_CRAY) || \
    defined(__hppa) || defined(__hp9000) || \
    defined(__hp9000s300) || defined(__hp9000s700) || \
    defined (BIT_ZERO_ON_LEFT) || defined(m68k) || defined(__sparc)
#define BYTE_ORDER	BIG_ENDIAN
#endif
#endif /* linux */
#endif /* BSD */
#endif /* BYTE_ORDER */

/* Sometimes after including an OS-specific header that defines the
 * endianess we end with __BYTE_ORDER but not with BYTE_ORDER that is what
 * the Redis code uses. In this case let's define everything without the
 * underscores. */
#ifndef BYTE_ORDER
#ifdef __BYTE_ORDER
#if defined(__LITTLE_ENDIAN) && defined(__BIG_ENDIAN)
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#endif
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN
#endif
#endif
#endif
#endif

#if !defined(BYTE_ORDER) || \
    (BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN)
	/* you must determine what the correct bit order is for
	 * your compiler - the next line is an intentional error
	 * which will force your compiles to bomb until you fix
	 * the above macros.
	 */
#error "Undefined or invalid BYTE_ORDER"
#endif

#if (__i386 || __amd64) && __GNUC__
#define GNUC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GNUC_VERSION >= 40100
#define HAVE_ATOMIC
#endif
#endif


#define SWAB16(aNumber) \
  ( \
   ((aNumber & 0x00ffU) << 8) | \
   ((aNumber & 0xff00U) >> 8) \
  )

#define SWAB32(aNumber) \
  ( \
   ((aNumber & 0x000000ffUL) << 24) | \
   ((aNumber & 0x0000ff00UL) << 8) | \
   ((aNumber & 0x00ff0000UL) >> 8) | \
   ((aNumber & 0xff000000UL) >> 24) \
  )

#define SWAB64(aNumber) \
  ( \
   ((aNumber & 0x00000000000000ffULL) << 56) | \
   ((aNumber & 0x000000000000ff00ULL) << 40) | \
   ((aNumber & 0x0000000000ff0000ULL) << 24) | \
   ((aNumber & 0x00000000ff000000ULL) << 8) | \
   ((aNumber & 0x000000ff00000000ULL) >> 8) | \
   ((aNumber & 0x0000ff0000000000ULL) >> 24) | \
   ((aNumber & 0x00ff000000000000ULL) >> 40) | \
   ((aNumber & 0xff00000000000000ULL) >> 56) \
  )

/* variants of the function doing the actual convertion only if the target
 * host is big endian */
#if (BYTE_ORDER == LITTLE_ENDIAN)
#define MemRev16ifbe(p)
#define MemRev32ifbe(p)
#define MemRev64ifbe(p)
#define IntRev16ifbe(v) (v)
#define IntRev32ifbe(v) (v)
#define IntRev64ifbe(v) (v)
#define SetIntRev16IfBe(v)
#define SetIntRev32IfBe(v)
#define SetIntRev64IfBe(v)
#else
#define MemRev16ifbe(p) *(uint16_t *) p = SWAB16(*(uint16_t *) p)
#define MemRev32ifbe(p) *(uint32_t *) p = SWAB32(*(uint32_t *) p)
#define MemRev64ifbe(p) *(uint64_t *) p = SWAB64(*(uint64_t *) p)
#define IntRev16ifbe(v) SWAB16(v)
#define IntRev32ifbe(v) SWAB32(v)
#define IntRev64ifbe(v) SWAB64(v)
#define SetIntRev16IfBe(v) v = SWAB16(v)
#define SetIntRev32IfBe(v) v = SWAB32(v)
#define SetIntRev64IfBe(v) v = SWAB64(v)
#endif

#endif
