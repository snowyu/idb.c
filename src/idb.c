/*
  Copyright (c) 2012-2013 Riceball LEE(snowyu.lee@gmail.com)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

//iDB helpers functions...

#include "config.h"
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>          /* fdopen(), etc */
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
//#include <dirname.h> /* last_component */

#include "zmalloc.h"
#include "idb.h"


//create a database object.
void* iDB_New(void){
    iDB *result;
    result = zmalloc(sizeof(iDB));
    if (result != NULL) {
        result->opened       = false;
        result->loadOnDemand = true;
        result->storeInFile  = true;
        result->storeInXattr = false;
        result->path         = NULL;
    }
    return result;
};

//free a iDB Database object.
void iDB_Free(void *aDB){
    if (aDB){
        register iDB* vDB = aDB;
        if (vDB->opened) iDB_Close(aDB);
        zfree(aDB);
    }
};

bool  iDB_Open(void *aDB, const sds aDBPath){
    assert(aDB && aDBPath);
    register iDB* vDB = aDB;
    if (!vDB->opened) {
        vDB->path         = sdsdup(aDBPath);
        vDB->opened       = true;
        return   true;
    }
    else
        return false;
};

bool  iDB_Close(void *aDB){
    register iDB* vDB = aDB;
    if (aDB && vDB->opened){
        vDB->opened = false;
        if (vDB->path) {
            zfree(vDB->path);
            vDB->path = NULL;
        }
        return true;
    }
    else
        return false;
};



#ifdef IDB_TEST_MAIN
#include "testhelp.h"

//Note: sds.* zmalloc.* config.h come from redis src
//gcc --std=c99 -I. -Ideps  -o test -DHAVE_UNISTD_H -DIDB_TEST_MAIN idb.c idb_helpers.c isdk_xattr.c isdk_utils.c deps/sds.c deps/zmalloc.c deps/filename.c
int main(int argc, char **argv)
{
    {
        sds d= sdsnew("mylink");
        struct stat st;
        int result = IsDirectory(d);
        printf("result=%d\n", result);


        sds v= sdsnew("testdir/god/better/best\n");
        char const *b = last_component(v);
        printf("last_component=%s", b);
        sdsfree(v);
/*
        ForceDirectories("testdir/good/better/best", O_RWXRWXR_XPERMS);
        test_cond("DirectoryExists('testdir/good/better/best')", DirectoryExists("testdir/good/better/best") == 1);
        sds x = sdsnew("testdir"), y = sdsnew("hi world");
        test_cond("SetDirValue('testdir')", SetDirValue(x, y, NULL));
        test_cond("IsDirValueExists(testdir, NULL)", IsDirValueExists(x, NULL));
        sds result = GetDirValue(x, NULL);
        test_cond("GetDirValue(testdir)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        sdsfree(x);
        sdsfree(y);
        if (result) sdsfree(result);
*/
    }

    test_report();
    return 0;
}
#endif
