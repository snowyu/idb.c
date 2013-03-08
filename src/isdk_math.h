/*
 * =====================================================================================
 *
 *       Filename:  isdk_math.h
 *
 *    Description:  the math funcs
 *
 *        Version:  0.1
 *        Created:  2013/03/08 10时52分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Riceball LEE(snowyu.lee@gmail.com) 
 *
 * =====================================================================================
 */
#ifndef __ISDK_MATH_H
#define __ISDK_MATH_H

#include <assert.h>
#include <sys/time.h>
#include <limits.h>

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

/* Get the larger value of two integers. */
static inline long max(long a, long b){
  return (a > b) ? a : b;
}


/* Get the lesser value of two integers. */
static inline long min(long a, long b){
  return (a < b) ? a : b;
}

/* Get the time of day in seconds. */
static inline double now(void){
  struct timeval tv;
  if(gettimeofday(&tv, NULL) == -1) return 0.0;
  return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

/* Get a random number as long integer based on uniform distribution. */
unsigned long randLong(void);


/* Get a random number as double decimal based on uniform distribution. */
static inline double randDouble(void){
  double val = randLong() / (double)ULONG_MAX;
  return val < 1.0 ? val : 0.0;
}


/* Get a random number as double decimal based on normal distribution. */
static inline double randDoubleNd(double avg, double sd){
  assert(sd >= 0.0);
  return sqrt(-2.0 * log(randDouble())) * cos(2 * 3.141592653589793 * randDouble()) * sd + avg;
}

#endif
