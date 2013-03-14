/*
 * =====================================================================================
 *
 *       Filename:  isdk_sds.c
 *
 *    Description:  sds wrapper
 *
 *        Version:  1.0
 *        Created:  2013/03/13
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Riceball LEE(snowyu.lee@gmail.com)
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <string.h>
#include "deps/zmalloc.h"
#include "isdk_sds.h"

sds sdsalloc(const void *init, size_t initlen) {
    struct sdshdr *sh;

    if (init) {
        sh = zmalloc(sizeof(struct sdshdr)+initlen+1);
    } else {
        sh = zcalloc(sizeof(struct sdshdr)+initlen+1);
    }
    if (sh == NULL) return NULL;
    if (init) {
        sh->len = initlen;
        sh->free = 0;
    } else {
        sh->len = 0;
        sh->free = initlen;
    }
    if (initlen && init) {
        memcpy(sh->buf, init, initlen);
    }
    sh->buf[sh->len] = '\0';
    return (char*)sh->buf;
}

sds sdsprintf(sds s, const char *fmt, ...) {
    struct sdshdr *sh = (void*) (s-(sizeof(struct sdshdr)));
    sh->free += sh->len;
    sh->len = 0;
    va_list ap;
    char *t;
    va_start(ap, fmt);
    t = sdscatvprintf(s,fmt,ap);
    va_end(ap);
    return t;
}

#ifdef ISDK_SDS_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"

int main(void) {
    {
        struct sdshdr *sh;

        sds x = sdsempty();
        test_cond("sdsempty() should be strlen 0",
            strlen(x) == 0 && sdslen(x) == 0 && memcmp(x,"\0",1) == 0);

        sdsfree(x);
        x = sdsalloc(NULL, 2);
        test_cond("Create a NULL string with reserved space 2 bytes",
            sdslen(x) == 0 && sdsavail(x) == 2);
        sdsfree(x);


    }
    test_report()
    return 0;
}
#endif
