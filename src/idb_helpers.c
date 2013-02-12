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
#include <unistd.h> //symlink
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

//the simple version to get relative path.
//get the relative path from aFrom dir
//"myfrom", "mydest" = "mydest"
//"myfrom/1", "myfrom/mydest" = "mydest"
//"myfrom/1/2", "myfrom/mydest" = "../mydest"
//"1/2", "mydest" = "../mydest"
//"1/2", "a/b/c/mydest" = "../a/b/c/mydest"
//"/myfrom", "mydest" = "mydest"
//"myfrom", "/mydest" = "mydest"
//TODO: the ".." is not supported yet.
sds GetRelativePath(const char* aFrom, const int aFromLen, const char* aTo, const int aToLen)
{
    const char* vFromPart = aFrom;
    const char* vToPart   = aTo;
    while (vFromPart[0] == PATH_SEP) vFromPart++;
    while (vToPart[0] == PATH_SEP) vToPart++;
    char* vFromSep = strchr(vFromPart, PATH_SEP);
    char* vToSep   = strchr(vToPart, PATH_SEP);
    while (vFromSep && vToSep) {
        //try to get rid of the same part path.
        int vFromLen = vFromSep-vFromPart;
        int vToLen   = vToSep-vToPart;
        if (vFromLen == vToLen) {
            if (strncmp(vFromPart, vToPart, vFromLen) == 0) {
                vFromPart = vFromSep + 1;
                vToPart   = vToSep + 1;
                vFromSep  = strchr(vFromPart, PATH_SEP);
                vToSep    = strchr(vToPart, PATH_SEP);
                continue;
            }
        }
        break;
    }
    //count the PATH_SEP of the vFromPart
    int vSepCount = 0;
    vFromSep = strchr(vFromPart, PATH_SEP);
    while (vFromSep) {
        vSepCount++;
        vFromSep++;
        vFromSep=strchr(vFromSep, PATH_SEP);
    }
    //if the last char is PATH_SEP
    if (aFrom[aFromLen-1]==PATH_SEP) vSepCount--;
    sds result = sdsnewlen(NULL, aToLen+(vSepCount*3));
    while (vSepCount) {
        result = sdscatlen(result, "..", 2);
        result = sdscatlen(result, PATH_SEP_STR, 1);
        vSepCount--;
    }
    result = sdscat(result, vToPart);
    return result;
}

int iAlias(const sds aDir, const char* aKey, const int aKeyLen, const char* aAlias, const int aAliasLen)
{
    //sds vSrcDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    sds vDestDir = sdsJoinPathLen(sdsdup(aDir), aAlias, aAliasLen);
    sds vSrcDir = GetRelativePath(aAlias, aAliasLen, aKey, aKeyLen);
    //make symbolic link(destPath) to a srcPath
    int result = symlink(vSrcDir, vDestDir);
    if (result != 0) result = errno;
    sdsfree(vSrcDir);
    sdsfree(vDestDir);
    return result;
}

//get the count number from attribute ".count" if any
long iGetCount(const sds aDir, const char* aKey, const int aKeyLen, const int aStoreType)
{
    sds vCountAttr = sdsnew(".count");
    sds s= iGet(aDir, aKey, aKeyLen, vCountAttr, aStoreType);
    sdsfree(vCountAttr);
    long result = 0;
    if (s) {
        char* vEnd = NULL;
        //strtol: convert str to long
        //base 0 means the same syntax used for integer constants in C
        result = strtol(s, &vEnd, 0);
        if (errno)
            result = -errno;
        else if (*vEnd)
            result = -1; //Converted partially.
    }
    return result;
}

size_t iSubkeyCount(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern)
{
    int vOpts = 1 << LIST_DIR | 1<< LIST_SYMBOLIC;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);

    size_t result = CountDir(vDir, aPattern, vOpts);
    sdsfree(vDir);
    return result;
}

/*
struct _SubkeysRec {
        dStringArray* result;
        int skipCount;
};

static int process_matched_dir(int aCount, const FTSENT *aNode, void *aPtr){
        sds s = sdsnew(aNode->fts_path);
        struct _SubkeysRec* vSubkeys = aPtr;
        darray_append(*(vSubkeys->result), s);
        return WALK_ITEM_OK;
}
*/
dStringArray* iSubkeys(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern, const int aSkipCount, const int aCount)
{
    int vOpts = 1 << LIST_DIR | 1<< LIST_SYMBOLIC;
    //BIT_SET(vOpts, LIST_DIR);
    //BIT_SET(vOpts, LIST_SYMBOLIC);
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);

    dStringArray* result = ListDir(vDir, aPattern, vOpts, aSkipCount, aCount);
    sdsfree(vDir);

    return result;
}

#ifdef IDB_HELPER_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"

//Note: sds.* zmalloc.* config.h come from redis src
//gcc --std=c99 -I. -Ideps  -o test -DIDB_HELPER_TEST_MAIN idb_helpers.c isdk_xattr.c isdk_utils.c deps/sds.c deps/zmalloc.c
void test_list(dStringArray* result, dCStrArray expected) {
        test_cond("List SHOULD NOT NULL", result);
        test_cond("List Length Test", darray_size(*result)==darray_size(expected));
        if (darray_size(*result)==darray_size(expected)) {
                sds s;
                char* s1 = "LIST Item:";
            for (int i=0; i< darray_size(*result); i++) {
                    s = sdsnew(s1);
                    s = sdscat(s, darray_item(*result, i));
                    test_cond(s, strcmp(darray_item(*result, i), darray_item(expected, i))==0);
                    sdsfree(s);
            }
        } else {
            printf("expected Length is %lu, but it is %lu in fact.\n", darray_size(expected), darray_size(*result));
        }
}
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
        //
        iPut(x, "myhi/see/u", 10, y, NULL, STORE_IN_FILE);
        iPut(x, "myhi/fa/si", 10, y, NULL, STORE_IN_FILE);
        iPut(x, "mygoo", 5, y, NULL, STORE_IN_FILE);
        iPut(x, "bygoo", 5, y, NULL, STORE_IN_FILE);
        test_cond("iSubkeyCount(x, '', 0, NULL)",iSubkeyCount(x, "", 0, NULL)==5);
        puts("----------------------------");
        puts("iSubkeys(x, \"\", 0, NULL, 0, 0)");
        puts("----------------------------");
        dCStrArray vExpectedList = darray_new();
        darray_appends_t(vExpectedList, const char*, "bygoo", "good", "mygoo", "myhi", "mytestkey");
        dStringArray* vList = iSubkeys(x, "", 0, NULL, 0, 0);
        test_list(vList, vExpectedList);
        dStringArray_free(vList);
        darray_free(vExpectedList);
        puts("----------------------------");
        sds rpath = GetRelativePath("myfrom", 6, "mydest", 6);
        test_cond("GetRelativePath(\"myfrom\", 6, \"mydest\", 6)", strcmp(rpath, "mydest")==0);
        sdsfree(rpath);
        rpath = GetRelativePath("myfrom/1dg/", 11, "myfrom/mydest", 13);
        test_cond("GetRelativePath(\"myfrom/1dg/\", 11, \"myfrom/mydest\", 13)", strcmp(rpath, "mydest")==0);
        sdsfree(rpath);
        rpath = GetRelativePath("myfrom/1/2", 10, "myfrom/mydest", 13);
        test_cond("GetRelativePath(\"myfrom/1/2\", 10, \"myfrom/mydest\", 13)", strcmp(rpath, "../mydest")==0);
        sdsfree(rpath);
        rpath = GetRelativePath("myfrom/1/2", 10, "myfrom/a/b/c/mydest", 19);
        test_cond("GetRelativePath(\"myfrom/1/2\", 10, \"myfrom/a/b/c/mydest\", 19)", strcmp(rpath, "../a/b/c/mydest")==0);
        sdsfree(rpath);
        rpath = GetRelativePath("1/2/", 4, "a/b/c/mydest/", 13);
        test_cond("GetRelativePath(\"1/2/\", 4, \"a/b/c/mydest/\", 13)", strcmp(rpath, "../a/b/c/mydest/")==0);
        sdsfree(rpath);
        rpath = GetRelativePath("/1/2/", 5, "a/b/c/mydest/", 13);
        test_cond("GetRelativePath(\"/1/2/\", 5, \"a/b/c/mydest/\", 13)", strcmp(rpath, "../a/b/c/mydest/")==0);
        sdsfree(rpath);

        iAlias(x, "myhi/see/u", 10, "myhi/see/u1", 11);
        test_cond("iAlias('myhi/see/u', 'myhi/see/u1')", IsDirectory("testdir/myhi/see/u1") == PATH_IS_SYM_DIR);

        iAlias(x, "myhi/see/u", 10, "myhi/fa/u", 11);
        test_cond("iAlias('myhi/see/u', 'myhi/fa/u')", IsDirectory("testdir/myhi/fa/u") == PATH_IS_SYM_DIR);

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
