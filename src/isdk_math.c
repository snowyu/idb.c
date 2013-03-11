/*
 * =====================================================================================
 *
 *       Filename:  isdk_math.c
 *
 *    Description:  the math funcs
 *
 *        Version:  1.0
 *        Created:  2013/03/08 11时12分31秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Riceball LEE(snowyu.lee@gmail.com)
 *
 * =====================================================================================
 */

#include "isdk_config.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <fcntl.h>
#include <math.h>

#include "isdk_math.h"


#define RAND_DEV_FILE      "/dev/urandom"    // path of the random device file

/* File descriptor of random number generator. */
int gRandomDevFileHandler = -1;

/* Close the random number generator. */
static inline void randomDevFileClose(void){
  close(gRandomDevFileHandler);
}

/* Get a random number as long integer based on uniform distribution. */
unsigned long randLong(void)
{
  static uint32_t cnt = 0;
  static uint64_t seed = 0;
  static uint64_t mask = 0;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  if((cnt & 0xff) == 0 && pthread_mutex_lock(&mutex) == 0){
    if(cnt == 0) seed += time(NULL);
    if(gRandomDevFileHandler == -1 && (gRandomDevFileHandler = open(RAND_DEV_FILE, O_RDONLY, 00644)) != -1)
      atexit(randomDevFileClose);
    if(gRandomDevFileHandler == -1 || read(gRandomDevFileHandler, &mask, sizeof(mask)) != sizeof(mask)){
      double t = now();
      uint64_t tmask;
      memcpy(&tmask, &t, min(sizeof(t), sizeof(tmask)));
      mask = (mask << 8) ^ tmask;
    }
    pthread_mutex_unlock(&mutex);
  }
  seed = seed * 123456789012301LL + 211;
  uint64_t num = (mask ^ cnt++) ^ seed;
  return SWAB64(num);
}

