/*
 * =====================================================================================
 *
 *       Filename:  isdk_sds.h
 *
 *    Description:  sds lib wrapper
 *
 *        Version:  1.0
 *        Created:  2013/03/13
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Riceball LEE (snowyu.lee@gmail.com)
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef ISDK_SDS__H
#define ISDK_SDS__H

#include "deps/sds.h"

#define SDSFreeAndNil(s) do {sdsfree(s); s = NULL;} while (0)

#ifdef __cplusplus
extern "C"
{
#endif

//create a NULL string with initlen space reserved
//sdslen(s) should always be 0.
//NOTE: the sdsnewlen() means creating a string with specific initlen length.
sds sdsalloc(const void *init, size_t initlen);
#ifdef __GNUC__
sds sdsprintf(sds s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else
sds sdsprintf(sds s, const char *fmt, ...);
#endif

static inline sds sdsSetlen(const sds s, int len) {
    sds result = s;
    struct sdshdr *sh = (struct sdshdr*)(s-(sizeof(struct sdshdr)));
    int incr = len - sh->len;
    if (incr > sh->free) result = sdsMakeRoomFor(s, incr);
    sdsIncrLen(s, incr);
    return result;
}

static inline sds sdsSetlen_(const sds s, int len) {
    sds result = s;
    struct sdshdr *sh = (struct sdshdr*)(s-(sizeof(struct sdshdr)));
    if (len != sh->len && len <= (sh->free+sh->len)) {
        //int incr = len - sh->len;
        sh->free -= len - sh->len;
        sh->len = len;
        result[sh->len] = '\0';
    }
    return result;
}

#ifdef __cplusplus
}
#endif

#endif
