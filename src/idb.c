/*
  Copyright (c) 2012 Riceball LEE(snowyu.lee@gmail.com)

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

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>          /* fdopen(), etc */
#include <stdlib.h>
#include <fcntl.h>

#include "zmalloc.h"
#include "idb.h"

//create a database object.
iDB* iDB_New(void){
    iDB *result;
    result = zmalloc(sizeof(iDB));
    if (result != NULL) {
        result->opened = false;
        result->path   = NULL;
    }
    return result;
};

//free a iDB Database object.
void iDB_Free(iDB *aDB){
    if (aDB){
        if (aDB->opened) iDB_Close(aDB);
        zfree(aDB);
    }
};

bool  iDB_Open(iDB *aDB, const sds aDBPath){
    assert(aDB && aDBPath);
    if (!aDB->opened) {
        aDB->path   = sdsdup(aDBPath);
        aDB->opened = true;
        return true;
    }
    else
        return false;
};
bool  iDB_Close(iDB *aDB){
    if (aDB && aDB->opened){
        aDB->opened = false;
        if (aDB->path) zfree(aDB->path);
        return true;
    }
    else
        return false;
};



#ifdef IDB_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"

//Note: sds.* zmalloc.* config.h come from redis src
//gcc --std=c99 -I.  -o test -DHAVE_UNISTD_H -DIDB_HELPER_TEST_MAIN idb_helpers.c isdk_xattr.c isdk_utils.c sds.c zmalloc.c
int main(int argc, char **argv)
{
    {
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
    }
    test_report();
    return 0;
}
#endif
