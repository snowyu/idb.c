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

#include "idb.h"
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

#include "deps/zmalloc.h"


//create a database object.
void* iDB_New(void){
    iDB *result;
    result = zmalloc(sizeof(iDB));
    if (result != NULL) {
        result->opened       = false;
        result->loadOnDemand = true;
        result->maxPageSize  = 0;
        /*
        result->storeInFile  = true;
        result->storeInXattr = false;
        */
        result->path         = NULL;
    }
    return result;
};

//free a iDB Database object.
void iDB_Free(void *aDB){
    register iDB* vDB = aDB;
    if (vDB){
        if (vDB->opened) iDB_Close(aDB);
        zfree(aDB);
    }
};

void iDB_SetOpened(void *aDB, bool aOpened)
{
    register iDB* vDB = aDB;
    vDB->opened = aOpened && vDB->path;
    if (vDB->opened) {
        #ifdef IDB_WORKING_DIR_SUPPORT
        vDB->opened = chdir(vDB->path) == 0;
        #else
        vDB->opened = true;
        #endif
        if (vDB->maxPageSize > IDBMaxPageCount) vDB->maxPageSize = IDBMaxPageCount;
    }
}

bool  iDB_Open(void *aDB, const sds aDBPath){
    assert(aDB && aDBPath);
    register iDB* vDB = aDB;
    if (!vDB->opened) {
        vDB->path         = sdsdup(aDBPath);
        iDB_SetOpened(vDB, vDB->opened);
        return   vDB->opened;
    }
    else
        return false;
};

bool  iDB_Close(void *aDB){
    register iDB* vDB = aDB;
    if (aDB && vDB->opened){
        vDB->opened = false;
        if (vDB->path) {
            sdsfree(vDB->path);
            vDB->path = NULL;
        }
        return true;
    }
    else
        return false;
};

static inline sds _iDBAttr_GetString(iDB *aDB, const void *aKey, int aKeySize, const char *aAttribute)
{
    sds vKeyPath, result = NULL;
    #ifdef IDB_WORKING_DIR_SUPPORT
    vKeyPath = sdsnewlen(aKey, aKeySize);
    #else
    vKeyPath = sdsJoinPathLen(sdsdup(aDB->path), aKey, aKeySize);
    #endif
    result = iGetInFile(vKeyPath, aAttribute);
    sdsfree(vKeyPath);
    return result;
};

sds iDBAttr_GetString(void *aDB, const void *aKey, int aKeySize, const char *aAttribute)
{
    sds result = NULL;
    register iDB* vDB = aDB;
    errno = 0;
    if (vDB->opened) {
        result = _iDBAttr_GetString(vDB, aKey, aKeySize, aAttribute);
    }
    else
        errno = ERR_IDB_CLOSED;
    return result;
};

static inline int64_t _iDBAttr_IncrBy(iDB *aDB, const void *aKey, int aKeySize, int64_t aValue, const char *aAttribute)
{
    int64_t result = ERR_IDB_CLOSED;
    sds vKeyPath;
    #ifdef IDB_WORKING_DIR_SUPPORT
    vKeyPath = sdsnewlen(aKey, aKeySize);
    #else
    vKeyPath = sdsJoinPathLen(sdsdup(aDB->path), aKey, aKeySize);
    #endif
    result = iIncrByInFile(vKeyPath, aValue, aAttribute, aDB->partitionFullProcess);
    sdsfree(vKeyPath);
    return result;
}

int64_t iDBAttr_IncrBy(void *aDB, const void *aKey, int aKeySize, int64_t aValue, const char *aAttribute)
{
    register iDB* vDB = aDB;
    int64_t result = ERR_IDB_CLOSED;
    errno = 0;
    sds vKeyPath;
    if (vDB->opened) {
        result = _iDBAttr_IncrBy(vDB, aKey, aKeySize, aValue, aAttribute);
    }
    else
        errno = ERR_IDB_CLOSED;
    return result;
}

static inline int _iDBAttr_PutString(iDB *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const char *aAttribute)
{
    sds vDir, s;
    int result, vLevel=0;
    #ifdef IDB_WORKING_DIR_SUPPORT
    vDir = sdsnewlen(aKey, aKeySize);
    if (sdslen(vDir) == 0) vDir = sdscpylen(vDir, ".", 1);
    #else
    vDir = sdsJoinPathLen(sdsdup(vDB->path), aKey, aKeySize);
    #endif
    result = DirectoryExists(vDir); //a/b/c/d
    if (result == PATH_IS_NOT_EXISTS) {
        //prepare for increment subkeys' count
        s = sdsdup(vDir);
        RemoveLastPathName(s); //the parent key /a/b/c
        #ifdef IDB_WORKING_DIR_SUPPORT
        result = 0;
        #else
        result = sdslen(aDB->path);
        #endif
        while (sdslen(s) > result && DirectoryExists(s) == PATH_IS_NOT_EXISTS) {
            vLevel++;
            RemoveLastPathName(s);
        }
        sdsfree(s);
    }
    result = iPutInFile(vDir, aValue, aValueSize, aAttribute, aDB->partitionFullProcess);
    if (result == 0) {
            //successful, now increment the subkeys' count
            RemoveLastPathName(vDir); //get the parent
            while (vLevel > 0) {
                iIncrByInFile(vDir, 1, IDB_ATTR_COUNT_NAME, aDB->partitionFullProcess);
                //and put it to index
                RemoveLastPathName(vDir);
                vLevel--;
            }
            #ifdef IDB_WORKING_DIR_SUPPORT
            if (sdslen(vDir)==0) vDir = sdscpylen(vDir, ".", 1);
            #endif
            iIncrByInFile(vDir, 1, IDB_ATTR_COUNT_NAME, aDB->partitionFullProcess);
    }
    if (vDir) sdsfree(vDir);
    
    return result;

}

int iDBAttr_PutString(void *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const char *aAttribute)
{
}

sds  iDB_GetString(void *aDB, const void *aKey, int aKeySize)
{
    return iDBAttr_GetString(aDB, aKey, aKeySize, NULL);
}

int iDB_PutString(void *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize)
{
    return iDBAttr_PutString(aDB, aKey, aKeySize, aValue, aValueSize, NULL);
}

dStringArray *iDB_Subkeys(void *aDB, const void *aKey, int aKeySize, const char* aPattern, const size_t aPageNo, size_t aPageSize)
{
    register iDB* vDB = aDB;
    dStringArray *result = NULL;
    errno = 0;
    sds vKeyPath;
    if (vDB->opened) {
        #ifdef IDB_WORKING_DIR_SUPPORT
        vKeyPath = sdsnewlen(".", 1);
        #else
        vKeyPath = vDB->path;
        #endif
        if (aPageSize > vDB->maxPageSize) aPageSize = vDB->maxPageSize;
        result = iSubkeys(vKeyPath, aKey, aKeySize, aPattern, aPageNo * aPageSize, aPageSize, vDB->partitionFullProcess);
        #ifdef IDB_WORKING_DIR_SUPPORT
        sdsfree(vKeyPath);
        #endif
    }
    else
        errno = ERR_IDB_CLOSED;
    return result;
}

#ifdef IDB_TEST_MAIN
#include "testhelp.h"


//Note: sds.* zmalloc.* config.h come from redis src
//gcc -fms-extensions --std=c99 -I. -Ideps -o test -DIDB_TEST_MAIN idb.c idb_helpers.c isdk_xattr.c isdk_utils.c isdk_sds.c deps/sds.c deps/zmalloc.c deps/utf8proc.c deps/filename.c deps/adlist.c
int main(int argc, char **argv)
{
    {
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
