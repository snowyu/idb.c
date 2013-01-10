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

#include "idb_helpers.h"


static inline sds _make_attr_file_name(const sds aDir, const sds aAttribute)
{
    sds vFile = sdsdup(aDir);
    if (aAttribute != NULL && sdslen(aAttribute) != 0) {
        vFile = sdsJoinPathLen(vFile, aAttribute, sdslen(aAttribute));
    }
    else {
        vFile = sdsJoinPathLen(vFile, IDB_VALUE_NAME, 6);
    }
    return vFile;
}


int IsDirValueExists(const sds aDir, const sds aAttribute)
{
    sds vFile = _make_attr_file_name(aDir, aAttribute);
    int result = DirectoryExists(vFile);
    sdsfree(vFile);
    return result == -1;
}

//http://codewiki.wikidot.com/system-calls
sds GetDirValue(const sds aDir, const sds aAttribute)
{
    sds result = _make_attr_file_name(aDir, aAttribute);
    int fd = open(result, O_RDONLY);
    if (fd < 0) {
        //fprintf(stderr, "Failed:%s\n", result);
        sdsfree(result);
        result = NULL;
    } else {
        sdsclear(result);
        int vSize = lseek(fd, 0L, SEEK_END);
        lseek(fd, 0L, SEEK_SET);
        sdsMakeRoomFor(result, vSize);
        vSize = read(fd, result, vSize);
        sdsIncrLen(result, vSize);
        close(fd);
    }
    return result;
}

int SetDirValue(const sds aDir, const sds aValue, const sds aAttribute)
{
    int result = 0;
    sds vFile = _make_attr_file_name(aDir, aAttribute);
    int fd = open(vFile, O_WRONLY|O_TRUNC|O_CREAT, O_RW_RW_R__PERMS);
    if (fd >= 0) {
        int vSize = write(fd, aValue, sdslen(aValue));
        if (vSize == sdslen(aValue)) result = 1;
        close(fd);
    }
    return result;
}

int iIsExists(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType){
    int result = -1;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
        sds s = sdsnew(XATTR_PREFIX);
        if (aAttribute)
            s = sdscatsds(s, aAttribute);
        else
            s = sdscat(s, IDB_VALUE_NAME);
        result = IsXattrExists(vDir, s);
        sdsfree(s);
    }
    if (result == -1 && BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = IsDirValueExists(vDir, aAttribute);
    }
    sdsfree(vDir);
    return result;
}

sds iGet(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType){
    sds result = NULL;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
        sds s = sdsnew(XATTR_PREFIX);
        if (aAttribute)
            s = sdscatsds(s, aAttribute);
        else
            s = sdscat(s, IDB_VALUE_NAME);
        result = GetXattr(vDir, s);
        sdsfree(s);
    }
    if (!result && BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = GetDirValue(vDir, aAttribute);
    }
    sdsfree(vDir);
    return result;
}

int iPut(const sds aDir, const char* aKey, const int aKeyLen, const sds aValue, const sds aAttribute, const int aStoreType){
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    int result = DirectoryExists(vDir);
    if (result == 0) { //Dir not Exists:
        ForceDirectories(vDir, O_RWXRWXR_XPERMS);
    }
    else if (result == -1) {//File Exists Error:
        return result;
    }
    //if (result == 1) //Dir Exists:
    {
        result = -2;

        if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
            sds s = sdsnew(XATTR_PREFIX);
            if (aAttribute)
                s = sdscatsds(s, aAttribute);
            else
                s = sdscat(s, IDB_VALUE_NAME);
            result = SetXattr(vDir, s, aValue) == 0;
            sdsfree(s);
        }
        if (BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
            result = SetDirValue(vDir, aValue, aAttribute);
        }
    }
    sdsfree(vDir);
    return result;
}

int iDelete(const sds aDir, const char* aKey, const int aKeyLen){
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    int result = DeleteDir(vDir) == 0;
    sdsfree(vDir);
    return result;
}

#ifdef IDB_HELPER_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"

//Note: sds.* zmalloc.* config.h come from redis src
//gcc --std=c99 -I. -Ideps  -o test -DIDB_HELPER_TEST_MAIN idb_helpers.c isdk_xattr.c isdk_utils.c deps/sds.c deps/zmalloc.c
int main(int argc, char **argv) 
{
    {
        ForceDirectories("testdir/good/better/best", O_RWXRWXR_XPERMS);
        test_cond("DirectoryExists('testdir/good/better/best')", DirectoryExists("testdir/good/better/best") == 1);
        sds x = sdsnew("testdir"), key=sdsnew("mytestkey"), y = sdsnew("hi world"), xa_value_name=sdsnew(XATTR_PREFIX);
        xa_value_name=sdscat(xa_value_name, IDB_VALUE_NAME);
        test_cond("SetDirValue('testdir', 'hi world')", SetDirValue(x, y, NULL));
        test_cond("IsDirValueExists(testdir, NULL)", IsDirValueExists(x, NULL));
        sds result = GetDirValue(x, NULL);
        test_cond("GetDirValue(testdir)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        test_cond("iPut('testdir','mytestkey', 'hi world', STORE_IN_FILE)", iPut(x, key, sdslen(key), y, NULL, STORE_IN_FILE));
        sds path=sdsnew("testdir/mytestkey");
        test_cond("IsDirValueExists(testdir/mytestkey, NULL)", IsDirValueExists(path, NULL));
        test_cond("iIsExists(testdir, mytestkey, STORE_IN_FILE) should be exists.", iIsExists(x, key, strlen(key),NULL, STORE_IN_FILE));
        if (result) sdsfree(result);
        result = GetDirValue(path, NULL);
        test_cond("GetDirValue(testdir/mytestkey)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        if (result) sdsfree(result);
        result = iGet(x, key, strlen(key), NULL, STORE_IN_FILE);
        test_cond("iGet(testdir, mytestkey, NULL, STORE_IN_FILE)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        test_cond("iDelete(testdir, mytestkey)", iDelete(x, key, sdslen(key)));
        test_cond("IsDirValueExists(testdir/mytestkey, NULL) == false", !IsDirValueExists(path, NULL));
        test_cond("iIsExists(testdir, mytestkey) should not be exists.", !iIsExists(x, key, strlen(key), NULL, STORE_IN_FILE));

        test_cond("iPut('testdir','mytestkey', 'hi world', STORE_IN_XATTR)", iPut(x, key, sdslen(key), y, NULL, STORE_IN_XATTR));
        test_cond("IsDirValueExists(testdir/mytestkey, NULL)", !IsDirValueExists(path, NULL));
        test_cond("IsXattrExists(testdir/mytestkey, IDB_VALUE_NAME)", IsXattrExists(path, xa_value_name));
        test_cond("iIsExists(testdir, mytestkey, STORE_IN_XATTR) should be exists.", iIsExists(x, key, strlen(key),NULL, STORE_IN_XATTR));
        if (result) sdsfree(result);
        result = GetXattr(path, xa_value_name);
        test_cond("GetXattr(testdir/mytestkey, IDB_VALUE_NAME)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        if (result) sdsfree(result);
        result = iGet(x, key, strlen(key), NULL, STORE_IN_XATTR);
        test_cond("iGet(testdir, mytestkey, NULL, STORE_IN_XATTR)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        test_cond("iDelete(testdir, mytestkey)", iDelete(x, key, sdslen(key)));
        test_cond("IsDirValueExists(testdir/mytestkey, NULL) == false", !IsDirValueExists(path, NULL));
        test_cond("iIsExists(testdir, mytestkey) should not be exists.", !iIsExists(x, key, strlen(key), NULL, STORE_IN_FILE));
 
        sdsfree(path);
        sdsfree(x);
        sdsfree(y);
        if (result) sdsfree(result);
        test_cond("DeleteDir(\"testdir\")", DeleteDir("testdir")==0);
        test_cond("DirectoryExists('testdir') should be not exists", DirectoryExists("testdir") == 0);
    }
    test_report();
    return 0;
}
#endif
