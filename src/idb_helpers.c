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
#include <unistd.h> //symlink, unlink
#endif /* HAVE_UNISTD_H */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>          /* fdopen(), etc */
#include <stdlib.h>
#include <fcntl.h>

#include "utf8proc.h"

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


bool DelDirValue(const sds aDir, const sds aAttribute)
{
    sds vFile = _make_attr_file_name(aDir, aAttribute);
    int result = unlink(vFile);
    sdsfree(vFile);
    return result == 0;
}

bool IsDirValueExists(const sds aDir, const sds aAttribute)
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

//return true means ok. false means failed.
bool SetDirValue(const sds aDir, const sds aValue, const sds aAttribute)
{
    int result = false;
    sds vFile = _make_attr_file_name(aDir, aAttribute);
    int fd = open(vFile, O_WRONLY|O_TRUNC|O_CREAT, O_RW_RW_R__PERMS);
    if (fd >= 0) {
        int vSize = write(fd, aValue, sdslen(aValue));
        if (vSize == sdslen(aValue)) result = true;
        close(fd);
    }
    return result;
}

static inline sds GenerateXattrName(const sds aAttribute)
{
    sds s = sdsnew(XATTR_PREFIX);
    if (aAttribute) {
        sds v = aAttribute;
        if (v[0] == IDB_ATTR_PREFIX_CHR) v++;
        s = sdscatsds(s, v);
    }
    else
        s = sdscat(s, IDB_VALUE_NAME+1); //skip the dot.
    return s;
}

static inline int _iPut(const sds aDir, const sds aValue, const sds aAttribute, const int aStoreType)
{
    int result = -2;
    if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
        sds s = GenerateXattrName(aAttribute);
        result = SetXattr(aDir, s, aValue);
        sdsfree(s);
    }
    if (BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = !SetDirValue(aDir, aValue, aAttribute);
    }
    return result;
}

static inline size_t _iKeyCount(const sds aDir, const char* aPattern) {
    int vOpts = 1 << LIST_DIR | 1<< LIST_SYMBOLIC;
    size_t result = CountDir(aDir, aPattern, vOpts);
    return result;
}
static inline size_t _iAttrCount(const sds aDir, const char* aPattern) {
    size_t result = CountDir(aDir, aPattern, LIST_ALL_FILES);
    return result;
}
//this is a splited key dir?
static inline bool _iIsKeyDir(const sds aDir, const sds aKey) {
    sds vAttrPattern = sdsnew(".*.");
    vAttrPattern = sdscatsds(vAttrPattern, aKey);
    bool result = IsFileExistsInDir(aDir, vAttrPattern, LIST_ALL_FILES);
    sdsfree(vAttrPattern);
    return result;
}
static inline bool _DeleleAttr(const sds aDir, const sds aAttribute, const int aStoreType)
{
    int result = -1;
    if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
        sds s = GenerateXattrName(aAttribute);
        result = DelXattr(aDir, s);
        if (result != 0) {
            errno = result;
            result = 0;
        } else
            result = true;
        sdsfree(s);
    }
    if (result == -1 && BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = DelDirValue(aDir, aAttribute);
    }
    if (result == -1) {
        result = 0;
        errno = ENOEXEC;
    }
    return result;
}

bool iDeleleAttr(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    bool result = _DeleleAttr(vDir, aAttribute, aStoreType);
    sdsfree(vDir);
    return result;
}

static bool _AttrIsExists(const sds aDir, const sds aAttribute, const int aStoreType)
{
    int result = -1;
    errno = 0;
    if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
        sds s = GenerateXattrName(aAttribute);
        result = IsXattrExists(aDir, s);
        sdsfree(s);
    }
    if (result == -1 && BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = IsDirValueExists(aDir, aAttribute);
    }
    if (result == -1) {
        result = 0;
        errno = ENOEXEC;
    }
    return result;
}

//get the splited key's attribute name
//the real attr name is: .[aAttribute].[aSubKey]
static inline sds GetAttrNameEx(const sds aAttribute, const char* aKey)
{
    sds vAttr = sdscatlen(aAttribute, IDB_ATTR_PREFIX, strlen(IDB_ATTR_PREFIX));
    const char* s = strrchr(aKey, PATH_SEP); //find the subkey if any.
    s = (s) ? s+1 : aKey;
    vAttr = sdscat(vAttr, s);
    return vAttr;
}

//test the splited key's attr is exists or not.
static bool _AttrExIsExists(const sds aDir, const char* aKey, const sds aAttribute, const int aStoreType)
{
    sds vAttr = GetAttrNameEx(aAttribute, aKey);
    bool result = _AttrIsExists(aDir, vAttr, aStoreType);
    sdsfree(vAttr);
    return result;
}


static sds _TryGetKeyDir(const sds aDir, int* aMaxItemCount)
{
    char* s = strrchr(aDir, PATH_SEP);
    if (s != NULL) {
        s++;
    } else {
        s = aDir;
    }
    const sds vKey = sdsnew(s);
    sds vAttrPattern = sdsnew(".*.");
    vAttrPattern = sdscatsds(vAttrPattern, vKey);
    s = vKey;
    sds vDir = aDir;
    ssize_t vKeySize = sdslen(vKey);
    sdsIncrLen(vDir, -vKeySize); //remove the key from dir.
    size_t vCount = _iKeyCount(vDir, NULL);
    if (vCount >= *aMaxItemCount) { //the dir is exceed the max count limits, so put the item into sub-dir
        sds vUtf8Char = sdsnewlen(NULL, 8); //the maximum size of the utf-8 char is 6. but i need point to the int32.
        do {
            ssize_t vLen = utf8proc_iterate(s, vKeySize, (int32_t *)vUtf8Char);
            if (vLen > 0) {
                //vUtf8Char = sdsSetlen(vUtf8Char, vLen);
                vDir = sdsJoinPathLen(vDir, vUtf8Char, vLen);
                s += vLen;
                vKeySize -= vLen;
                //have any attributes for splited key exists?
                // .*.[s]
                sds vTempDir = sdsJoinPathLen(sdsdup(vDir), s, vKeySize);
                if (IsFileExistsInDir(vTempDir, vAttrPattern, LIST_ALL_FILES)){
                    sdsfree(vTempDir);
                    break;
                }
                sdsfree(vTempDir);
                vCount = _iKeyCount(vDir, NULL);
            } else {
                errno = vLen;
                switch (vLen) {
                    case 0:
                        warnx("No Char found in the key:%s", vKey);
                    case UTF8PROC_ERROR_INVALIDUTF8 :
                        warnx("_iAdjustItemDir:%s UTF8PROC_ERROR_INVALIDUTF8", vKey);
                }
                sdsfree(vUtf8Char);
                sdsfree(vKey);
                sdsfree(aDir);
                sdsfree(vAttrPattern);
                return NULL;
            }
        } while(vCount >= *aMaxItemCount && vKeySize > 0);
        //vDir = sdsJoinPathLen(vDir, vKey, sdslen(vKey));
        vDir = sdsJoinPathLen(vDir, s, vKeySize);
        sdsfree(vAttrPattern);
        sdsfree(vKey);
        sdsfree(vUtf8Char);
    }
    else {
        //vDir = sdsJoinPathLen(vDir, vKey, sdslen(vKey)); //restore the vDir value
        *aMaxItemCount = 0;
    }

    return vDir;
}

bool iIsExists(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    bool result = _AttrIsExists(vDir, aAttribute, aStoreType);
    if (result == false && iDBMaxItemCount > 0) {
        int vAdjusted = iDBMaxItemCount;
        vDir = _TryGetKeyDir(vDir, &vAdjusted);
        if (vDir == NULL) return false; //some error occur.
        if (vAdjusted) {
            sds vAttr = GetAttrNameEx(sdsdup(aAttribute), aKey);
            result = _AttrIsExists(vDir, vAttr, aStoreType);
            sdsfree(vAttr);
        }
    }
    sdsfree(vDir);
    return result;
}

sds iGet(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType){
    sds result = NULL;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
        sds s = GenerateXattrName(aAttribute);
        result = GetXattr(vDir, s);
        sdsfree(s);
    }
    if (!result && BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = GetDirValue(vDir, aAttribute);
    }
    sdsfree(vDir);
    return result;
}

//result = 0 means ok, -2 means no operation, -1 means the same file name exists error, 
int iPut(const sds aDir, const char* aKey, const int aKeyLen, const sds aValue, const sds aAttribute, const int aStoreType){
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    int vAdjusted = 0;
    if (aKey && iDBMaxItemCount > 0) {
        vAdjusted = iDBMaxItemCount;
        vDir = _TryGetKeyDir(vDir, &vAdjusted);
        if (vDir == NULL) return errno;
    }
    int result = DirectoryExists(vDir);
    if (result == 0) { //Dir not Exists:
        ForceDirectories(vDir, O_RWXRWXR_XPERMS);
    }
    else if (result == -1) {//File Already Exists Error:
        return result;
    }
    //if (result == 1) //Dir Exists:
    if (vAdjusted == 0) {
        result = _iPut(vDir, aValue, aAttribute, aStoreType);
    }
    else {
        //the real attr name is: .[aAttribute].[aSubKey]
        //GetAttrNameEx(const sds aAttribute, const char* aKey)
        sds vAttr = GetAttrNameEx(sdsdup(aAttribute), aKey);
        result = _iPut(vDir, aValue, vAttr, aStoreType);
        sdsfree(vAttr);
    }
    
    sdsfree(vDir);
    return result;
}

//the delete will be more complex.
//just delete the key's all attributes only.
//only delete the dir if key's empty dir.
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

//delete the key's all matched subkeys.
//aPattern = NULL means all subkeys.
//if sucessful return the deleted subkeys' count. or -1 return if failed.
ssize_t iDeleteSubkeys(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern)
{
}

size_t iSubkeyCount(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    size_t result = _iKeyCount(vDir, aPattern);
    sdsfree(vDir);
    return result;
}

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
//gcc --std=c99 -I. -Ideps  -o test -DIDB_HELPER_TEST_MAIN idb_helpers.c isdk_xattr.c isdk_utils.c deps/sds.c deps/zmalloc.c deps/utf8proc.c
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
        sds x = sdsnew("testdir"), key=sdsnew("mytestkey"), y = sdsnew("hi world"), xa_value_name=GenerateXattrName(NULL);
        test_cond("SetDirValue('testdir', 'hi world')", SetDirValue(x, y, NULL));
        test_cond("IsDirValueExists(testdir, NULL)", IsDirValueExists(x, NULL));
        sds result = GetDirValue(x, NULL);
        test_cond("GetDirValue(testdir)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        test_cond("DelDirValue(testdir, NULL)", DelDirValue(x, NULL));
        test_cond("!IsDirValueExists(testdir, NULL)", !IsDirValueExists(x, NULL));
        test_cond("iPut('testdir','mytestkey', 'hi world', STORE_IN_FILE)", iPut(x, key, sdslen(key), y, NULL, STORE_IN_FILE)==0);
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
        test_cond("iDeleteAttr(testdir, mytestkey, STORE_IN_FILE)", iDeleleAttr(x, key, strlen(key),NULL, STORE_IN_FILE));
        test_cond("IsDirValueExists(testdir/mytestkey, NULL) == false", !IsDirValueExists(path, NULL));
        test_cond("iIsExists(testdir, mytestkey) should not be exists.", !iIsExists(x, key, strlen(key), NULL, STORE_IN_FILE));
        test_cond("iDelete(testdir, mytestkey)", iDelete(x, key, sdslen(key)));
        test_cond("!DirectoryExists('testdir/mytestkey')", DirectoryExists("testdir/mytestkey") == 0);

        test_cond("iPut('testdir','mytestkey', 'hi world', STORE_IN_XATTR)", iPut(x, key, sdslen(key), y, NULL, STORE_IN_XATTR)==0);
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
        iPut(x, "myhi/see/u/s/m", 15, y, NULL, STORE_IN_FILE);
        iPut(x, "myhi/see/d/uack", 15, y, NULL, STORE_IN_FILE);
        iPut(x, "myhi/fa/si", 10, y, NULL, STORE_IN_FILE);
        iPut(x, "mygoo", 5, y, NULL, STORE_IN_FILE);
        iPut(x, "bygoo", 5, y, NULL, STORE_IN_FILE);
        sds adjustedDir= sdsJoinPathLen(sdsdup(x), "myhi/see/usmek", 14);
        int vAdjusted = 1;
        adjustedDir= _TryGetKeyDir(adjustedDir, &vAdjusted);
        test_cond("_TryGetKeyDir('my/hi/see/usmek', 1)", strcmp(adjustedDir, "testdir/myhi/see/u/s/m/ek") == 0);
        sdsfree(adjustedDir);

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

        test_cond("iDeleteAttr(testdir, mytestkey, STORE_IN_XATTR)", iDeleleAttr(x, key, strlen(key),NULL, STORE_IN_XATTR));
        test_cond("!IsXattrExists(testdir/mytestkey, IDB_VALUE_NAME)", !IsXattrExists(path, xa_value_name));
        test_cond("!iIsExists(testdir, mytestkey, STORE_IN_XATTR)", !iIsExists(x, key, strlen(key), NULL, STORE_IN_XATTR));
        test_cond("iDelete(testdir, mytestkey)", iDelete(x, key, sdslen(key)));
        test_cond("!DirectoryExists('testdir/mytestkey')", DirectoryExists("testdir/mytestkey") == 0);
 
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
