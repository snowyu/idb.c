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

#include "deps/config.h"
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> //symlink, unlink
#endif /* HAVE_UNISTD_H */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>          /* fdopen(), etc */
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <fnmatch.h>

#include "deps/utf8proc.h"

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
    return result == PATH_IS_FILE;
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

static inline sds _iGet(const sds aDir, const sds aAttribute, const int aStoreType)
{
    sds result = NULL;
    if BIT_CHECK(aStoreType, STORE_IN_XATTR_BIT) {
        sds s = GenerateXattrName(aAttribute);
        result = GetXattr(aDir, s);
        sdsfree(s);
    }
    if (!result && BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = GetDirValue(aDir, aAttribute);
    }
    return result;
}

static inline int _iPut(const sds aDir, const sds aValue, const sds aAttribute, const int aStoreType)
{
    int result = ENOEXEC;
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

static inline bool _DeleleAttr(const sds aDir, const sds aAttribute, const int aStoreType)
{
    int result = ENOEXEC;
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
    if (result == ENOEXEC && BIT_CHECK(aStoreType, STORE_IN_FILE_BIT)) {
        result = DelDirValue(aDir, aAttribute);
    }
    if (result == ENOEXEC) {
        result = false;
        errno = ENOEXEC;
    }
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

//Get the proper dir name to save the partition key.
//aMaxItemCount will be chaged to 0 if Do not need adjust key dir.
    //Get the char of the key's base name one by one.
    //for i := 0 to Length(vKeyBaseName) do
    //begin
    //  //vDir := vDir + '/.' + vKeyBaseName[i];
    //  vDir := JoinPath(vDir, vKeyBaseName[i], IDB_PART_DIR_PREFIX);
    //  vDir += vKeyBaseName[i];
    //  if DirectoryExists(vDir) then begin
    //    vKeySize -= 1;
    //    vTempDir := vDir + SubString(vKeyBaseName, i+1, vKeySize);
    //    if DirectoryExists(vTempDir) then return vTempDir;
    //  end;
    //
    //  //whether the dir is exceed the max count limits?
    //  //vTempDir = SubString(vDir, 1, Length(vDir)-3);
    //  vTempDir = SubString(vDir, 1, Length(vDir)-(length(IDB_PART_DIR_PREFIX)+length(PATH_SEP_STR)+1));
    //  size_t vCount = _iKeyCount(vDir, NULL);
    //  if vCount < aMaxItemCount then begin
    //    i--;
    //    break;
    //  end;
    //end;
static sds _GetKeyDir(const sds aDir, const int aMaxItemCount)
{
    int vDirExists = DirectoryExists(aDir);
    if (vDirExists == PATH_IS_DIR || (vDirExists == PATH_IS_NOT_EXISTS && _iKeyCount(aDir, NULL) < aMaxItemCount)) {
        //the Directory is exists
        return aDir;
    }
    /*
    else if (vDirExists == PATH_IS_FILE) {
       //So if comment this, the same file name exists willl be partition
        errno = PATH_IS_FILE;
        sdsfree(aDir);
        return NULL;
    }
    */
    //get the base name of the key(ONLY NAME, no path in it.)
    char* s = strrchr(aDir, PATH_SEP);
    if (s != NULL) {
        s++;
    } else {
        s = aDir;
    }
    const sds vKey = sdsnew(s);
    errno = 0;
    s = vKey;
    sds vDir = aDir;
    ssize_t vKeySize = sdslen(vKey);
    //remove the key from vDir.
    sdsIncrLen(vDir, -vKeySize);
    sds vUtf8Char = sdsnewlen(NULL, 8); //the maximum size of the utf-8 char is 6. but i need point to the int32.
    do {
        //vLen means character byte length.
        ssize_t vLen = utf8proc_iterate(s, vKeySize, (int32_t *)vUtf8Char);
        if (vLen > 0) {
            s += vLen;
            if (s == "") {
                errno = IDB_ERR_PART_FULL;
                sdsfree(vUtf8Char);
                sdsfree(vKey);
                sdsfree(vDir);
                return NULL;
            }
            vKeySize -= vLen;
            int vOldDirLen = sdslen(vDir);
            vDir = sdsJoinPathLen(vDir, IDB_PART_DIR_PREFIX, strlen(IDB_PART_DIR_PREFIX));
            vDir = sdscatlen(vDir, vUtf8Char, vLen);
            if (DirectoryExists(vDir) == PATH_IS_DIR) {
                sds vTempDir = sdsJoinPathLen(sdsdup(vDir), s, vKeySize);
                if (DirectoryExists(vTempDir) == PATH_IS_DIR){ 
                    //Got it
                    sdsfree(vTempDir);
                    break;
                }
                sdsfree(vTempDir);
                //whether the dir is exceed the max count limits?
                if (_iKeyCount(vDir, NULL) < aMaxItemCount) {
                    break;
                }
            }
            else {
                break;
            }
        } else {
            errno = vLen;
            switch (vLen) {
                case UTF8PROC_ERROR_INVALIDUTF8 :
                    warnx("_iAdjustItemDir:%s UTF8PROC_ERROR_INVALIDUTF8", vKey);
            }
            sdsfree(vUtf8Char);
            sdsfree(vKey);
            sdsfree(aDir);
            return NULL;
        }
    } while(vKeySize > 0);
    vDir = sdsJoinPathLen(vDir, s, vKeySize);
    sdsfree(vKey);
    sdsfree(vUtf8Char);

    return vDir;
}

//return the real dir if such key exists, or return NULL.
//try to find the key in the local partition scheme.
//set the errno if failed
    //Get the char of the key's base name one by one.
    //for i := 0 to Length(vKeyBaseName) do
    //begin
    //  //vDir := vDir + '/.' + vKeyBaseName[i];
    //  vDir := JoinPath(vDir, vKeyBaseName[i], IDB_PART_DIR_PREFIX);
    //  vDir += vKeyBaseName[i];
    //  if DirectoryExists(vDir) then begin
    //    vKeySize -= 1;
    //    vTempDir := vDir + SubString(vKeyBaseName, i+1, vKeySize);
    //    if DirectoryExists(vTempDir) then return vTempDir;
    //  end
    //  else begin
    //    return NULL;
    //  end;
    //end;
static sds _IsKeyDirExists(const sds aDir)
{
    //get the base name of the key(ONLY NAME, no path in it.)
    char* s = strrchr(aDir, PATH_SEP);
    if (s != NULL) {
        s++;
    } else {
        s = aDir;
    }
    const sds vKey = sdsnew(s);
    errno = 0;
    s = vKey;
    sds vDir = aDir;
    ssize_t vKeySize = sdslen(vKey);
    //remove the key from vDir.
    sdsIncrLen(vDir, -vKeySize);
    sds vUtf8Char = sdsnewlen(NULL, 8); //the maximum size of the utf-8 char is 6. but i need point to the int32.
    do {
        //vLen means character byte length.
        ssize_t vLen = utf8proc_iterate(s, vKeySize, (int32_t *)vUtf8Char);
        if (vLen > 0) {
            //vUtf8Char = sdsSetlen(vUtf8Char, vLen);
            vDir = sdsJoinPathLen(vDir, IDB_PART_DIR_PREFIX, strlen(IDB_PART_DIR_PREFIX));
            vDir = sdscatlen(vDir, vUtf8Char, vLen);
            if (DirectoryExists(vDir) == PATH_IS_DIR) {
                s += vLen;
                vKeySize -= vLen;
                sds vTempDir = sdsJoinPathLen(sdsdup(vDir), s, vKeySize);
                if (DirectoryExists(vTempDir) == PATH_IS_DIR){
                    //Got it
                    sdsfree(vTempDir);
                    break;
                }
                sdsfree(vTempDir);
            }
            else {
                sdsfree(vUtf8Char);
                sdsfree(vKey);
                sdsfree(aDir);
                return NULL;
            }
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
            return NULL;
        }
    } while(vKeySize > 0);
    vDir = sdsJoinPathLen(vDir, s, vKeySize);
    sdsfree(vKey);
    sdsfree(vUtf8Char);

    return vDir;
}

bool iIsExists(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    bool result = _AttrIsExists(vDir, aAttribute, aStoreType);
    if (result == false && IDBMaxItemCount > 0) {
        vDir = _IsKeyDirExists(vDir);
        result = (vDir != NULL);
        if (result) result = _AttrIsExists(vDir, aAttribute, aStoreType);
        //if (vDir == NULL) return false; //No such key or some error occur(see errno).
    }
    if (vDir) sdsfree(vDir);
    return result;
}

sds iGet(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType){
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    sds result = _iGet(vDir, aAttribute, aStoreType);
    if (result == NULL && IDBMaxItemCount > 0) {
        vDir = _IsKeyDirExists(vDir);
        if (vDir != NULL) result = _iGet(vDir, aAttribute, aStoreType);
    }
    if (vDir) sdsfree(vDir);
    return result;
}

//result = 0 means ok, ENOEXEC means no operation, -1(PATH_IS_FILE) means the same file name exists error, 
int iPut(const sds aDir, const char* aKey, const int aKeyLen, const sds aValue, const sds aAttribute, const int aStoreType){
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    int vAdjusted = 0;
    if (aKey && IDBMaxItemCount > 0) {
        vDir = _GetKeyDir(vDir, IDBMaxItemCount);
        if (vDir == NULL) return errno;
    }
    int result = DirectoryExists(vDir);
    if (result == PATH_IS_NOT_EXISTS) { //Dir not Exists:
        ForceDirectories(vDir, O_RWXRWXR_XPERMS);
    }
    else if (result == PATH_IS_FILE) {//File Already Exists Error:
        return result;
    }
    result = _iPut(vDir, aValue, aAttribute, aStoreType);
    
    sdsfree(vDir);
    return result;
}

bool iDeleleAttr(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType)
{
    bool result = false;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    if (DirectoryExists(vDir) != PATH_IS_DIR && IDBMaxItemCount > 0){
        vDir = _IsKeyDirExists(vDir);
    }
    if (vDir) {
      result = _DeleleAttr(vDir, aAttribute, aStoreType);
      sdsfree(vDir);
    }
    return result;
}

bool iDelete(const sds aDir, const char* aKey, const int aKeyLen){
    int result = false;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    if (DirectoryExists(vDir) != PATH_IS_DIR && IDBMaxItemCount > 0){
        vDir = _IsKeyDirExists(vDir);
    }
    if (vDir) {
      result = DeleteDir(vDir) == 0;
      sdsfree(vDir);
    }
    return result;
}

//the simple version to get relative path.
//get the relative path from aFrom dir
//FromDir       ToDir           Result(RelativeDir)
//"myfrom",     "mydest"        = "mydest"
//"myfrom/1",   "myfrom/mydest" = "mydest"
//"myfrom/1/2", "myfrom/mydest" = "../mydest"
//"1/2",        "mydest"        = "../mydest"
//"1/2",        "a/b/c/mydest"  = "../a/b/c/mydest"
//"/myfrom",    "mydest"        = "mydest"
//"myfrom",     "/mydest"       = "mydest"
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


typedef struct {
    const char* pattern;
    sds subkeyPart;
    size_t count;     //the current level count
    size_t leftCount;
    size_t skipCount;
    WalkKeyHandler processor;
    const void *userPtr;
} TSubkeyWalkerRec;

static ssize_t _iSubkeyWalk(const sds aDir, const char* aKey, const int aKeyLen, TSubkeyWalkerRec *aRec);

static ssize_t _SubkeyWalker(size_t aCount, const char *aDir, const Dirent *aItem, TSubkeyWalkerRec *aRec) {
    ssize_t result = WALK_ITEM_OK;
    if (aRec) {
        sds vKey = NULL;
        bool vHasSubkey = sdslen(aRec->subkeyPart) > 0;
        if (vHasSubkey) {
            if (!FNM_MATCHED(aRec->pattern, aItem->d_name)) {
                return WALK_ITEM_SKIP;
            }
        }
        if (aRec->skipCount > 0) {
            aRec->skipCount--;
            return WALK_ITEM_SKIP;
        }
        if (vHasSubkey) {
            vKey = sdsdup(aRec->subkeyPart);
            vKey = sdscatlen(vKey, aItem->d_name, aItem->d_namlen);
        }
        if (aRec->processor) {
            const char* s = vKey ? vKey : aItem->d_name;
            result = aRec->processor(aRec->count + aCount, aDir, s, aRec->subkeyPart, aRec->userPtr);
        }
        if (vKey) sdsfree(vKey);
        
    }
    return result;
}

static ssize_t _SubkeyPartitionWalker(size_t aCount, sds aDir, const Dirent *aItem, TSubkeyWalkerRec *aRec) {
    ssize_t result = WALK_ITEM_OK;
    if (aRec) {
        TSubkeyWalkerRec vRec = *aRec;
        vRec.subkeyPart = sdscatlen(sdsdup(aRec->subkeyPart), aItem->d_name+1, aItem->d_namlen-1);
        ssize_t vCount = _iSubkeyWalk(aDir, aItem->d_name, aItem->d_namlen, &vRec);
        sdsfree(vRec.subkeyPart);
        if (vCount > 0) {
            aRec->count += vCount;
            if (aRec->leftCount > 0 && vCount >= aRec->leftCount)
                result = WALK_ITEM_STOP; //stop walk.
        }
    }
    return result;
}

static ssize_t _iSubkeyWalk(const sds aDir, const char* aKey, const int aKeyLen, TSubkeyWalkerRec *aRec)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    size_t result = WalkDir(vDir, aRec->pattern, LIST_NORMAL_DIRS, 0, aRec->leftCount, (WalkDirHandler) _SubkeyWalker, (void*) aRec);
    if (result>=0 && (aRec->leftCount <=0 || result < aRec->leftCount)) {
        aRec->leftCount = aRec->leftCount - result;
        sds vSubkeyPart = sdsnewlen(NULL, 8);
        //TODO:this should be a repeat loop until pattern[0] == '*' or '?' or NULL
        if (aRec->pattern && aRec->pattern[0] != '\0' && aRec->pattern[0] != '*' && aRec->pattern[0] != '?') {
            sds vIndexKey = sdsnewlen(".", 1);
            vIndexKey = sdsMakeRoomFor(vIndexKey, 8);
            int vLen = utf8proc_iterate(aRec->pattern, 6, (int32_t *)(vIndexKey+1));
            if (vLen > 0) {
                aRec->subkeyPart = sdscatlen(aRec->subkeyPart, aRec->pattern, vLen);
                vIndexKey = sdscatlen(vIndexKey, aRec->pattern, vLen);
                aRec->pattern += vLen;
                int vCount = _iSubkeyWalk(vDir, vIndexKey, sdslen(vIndexKey), aRec);
                //aRec->count += vCount;
                result += vCount;
            }
            else { //error occur. invalid utf8 string.
                result = vLen;
                //return vLen;
            }
        }
        else { //Walk through the Partition Keys
            TSubkeyWalkerRec vRec = *aRec;
            vRec.count = result;
            WalkDir(vDir, NULL, LIST_HIDDEN_DIRS, 0, aRec->leftCount, (WalkDirHandler) _SubkeyPartitionWalker, (void*) &vRec);
            result = vRec.count;
        }
        sdsfree(vSubkeyPart);

    }
    sdsfree(vDir);
    return result;
}
//First Walk through the Normal Keys, then walk through the Partition Keys
//aPattern: only for Normal Keys!
ssize_t iSubkeyWalk(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern,
        size_t aSkipCount, size_t aCount, const WalkKeyHandler aProcessor, const void *aUserPtr)
{
    sds vDir = aDir;
    sds vSubkeyPart = sdsnewlen(NULL, 8);
    TSubkeyWalkerRec *vRec = zmalloc(sizeof(TSubkeyWalkerRec));
    vRec->pattern = aPattern;
    vRec->count = 0;
    vRec->leftCount = aCount;
    vRec->skipCount = aSkipCount;
    vRec->subkeyPart = vSubkeyPart;
    vRec->processor = aProcessor;
    vRec->userPtr = aUserPtr;
    ssize_t result = _iSubkeyWalk(vDir, aKey, aKeyLen, vRec);
    zfree(vRec);
    sdsfree(vSubkeyPart);
    return result;
/*
    size_t result = _iSubkeyWalkNormalKeys(vDir, vPartKey, aPattern, aSkipCount, aCount, aProcessor, aUserPtr);
    sdsfree(vPartKey);
    if ((result >= 0) && (aCount <= 0 || result < aCount) && (!aPattern || aPattern[0] != '.')) {
        vOpts = 1 << LIST_DIR | 1 << LIST_HIDDEN_FILE | 1<< LIST_SYMBOLIC;
        sds vPart = sdsnewlen(".", 1);
        if (aPattern && aPattern != "") {
            vPart = sdsMakeRoomFor(vPart, 8);
            int vLen = utf8proc_iterate(aPattern, 6, (int32_t *)(vPart+1));
            if (vLen > 0) {
                if (vPart[1] != '*' && vPart[1] != '?') {
                    sdsIncrLen(vPart, vLen);
                    vDir = sdsJoinPathLen(vDir, vPart, sdslen(vPart));
                    vPart = sdscpy(vPart, aPattern+vLen);
                }
                else {
                    vPart = sdscatlen(vPart, "*", 1);
                }
            }
            else { // error
                result = vLen;
            }
            if (result >= 0) {
                dStringArray* vPartKeys = dStringArray_new();
                int vCount = WalkDir(vDir, vPart, vOpts, 0, aCount-result, aProcessor, vPartKeys);
                dStringArray_free(vPartKeys);
            }
            if (vPart) sdsfree(vPart);
        }
        else {
            vPart = sdscatlen(vPart, "*", 1);
        }
    }
    sdsfree(vDir);
    return result;
*/
}

ssize_t iSubkeyCount(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern)
{
    //int vOpts = 1 << LIST_DIR | 1<< LIST_SYMBOLIC;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    ssize_t result = iSubkeyWalk(aDir, aKey, aKeyLen, aPattern, 0, 0, NULL, NULL);//_iKeyCount(vDir, aPattern);
    sdsfree(vDir);
    return result;
}

static ssize_t _iSubkeysWalker(size_t aCount, const char* aDir, const char* aKey, const char *aSubkeyPart, dStringArray *aSubkeys)
{
    ssize_t result = WALK_ITEM_OK;

    sds vItem = NULL;
    switch (IDBDuplicationKeyProcess)
    {
        case dkReserved: {
            int s = 1;
            vItem = sdsnew(aKey);
            break;
        }
        case dkIgnored:
        case dkFixed: {
            ssize_t i = dStringArray_indexOf(aSubkeys, (sds)aKey);
            if (i < 0) { ////Not Exists in aSubkeys
                vItem = sdsnew(aKey);
            } else if (IDBDuplicationKeyProcess == dkFixed) {
                if (DeleteDir(aDir) != 0) { //remove duplication failed:
                    warnx("Remove Duplication Key(%s): Delete \"%s\" Failed", aKey, aDir);
                }
                result = WALK_ITEM_SKIP;
            }
            break;
        } //end dkFixed
    } //end switch
    if (vItem) {
        darray_append(*aSubkeys, vItem);
    }
    return result;
}
dStringArray* iSubkeys(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern, const int aSkipCount, const int aCount)
{
    dStringArray* result = dStringArray_new();
    ssize_t vCount = iSubkeyWalk(aDir, aKey, aKeyLen, aPattern, aSkipCount, aCount, (WalkKeyHandler) _iSubkeysWalker, result);
    if (vCount < 0) {
        errno = vCount;
        warnx("iSubkeys error:%ld", errno);
        dStringArray_free(result);
        result = NULL;
    }

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
            for (int i=0; i< darray_size(expected); i++) {
                    s = sdsnew(s1);
                    s = sdscat(s, darray_item(expected, i));
                    test_cond(s, dStringArray_indexOf(result, (sds)darray_item(expected, i))>=0);
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
        test_cond("DirectoryExists('testdir/good/better/best')", DirectoryExists("testdir/good/better/best") == PATH_IS_DIR);
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
        test_cond("!DirectoryExists('testdir/mytestkey')", DirectoryExists("testdir/mytestkey") == PATH_IS_NOT_EXISTS);

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
        iPut(x, "myhi/see/.u/s", 13, y, NULL, STORE_IN_FILE);
        iPut(x, "myhi/see/.u/.s/c", 16, y, NULL, STORE_IN_FILE);
        iPut(x, "myhi/see/d/uack", 15, y, NULL, STORE_IN_FILE);
        iPut(x, "myhi/fa/si", 10, y, NULL, STORE_IN_FILE);
        iPut(x, "mygoo", 5, y, NULL, STORE_IN_FILE);
        iPut(x, "bygoo", 5, y, NULL, STORE_IN_FILE);
        sds adjustedDir= sdsJoinPathLen(sdsdup(x), "myhi/see/usmek", 14);
        adjustedDir= _GetKeyDir(adjustedDir, 1);
        test_cond("_TryGetKeyDir('my/hi/see/usmek', 1)", strcmp(adjustedDir, "testdir/myhi/see/.u/.s/.m/ek") == 0);
        sdsfree(adjustedDir);

        test_cond("iSubkeyCount(x, '', 0, NULL) == 5",iSubkeyCount(x, "", 0, NULL)==5);
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
        test_cond("!DirectoryExists('testdir/mytestkey')", DirectoryExists("testdir/mytestkey") == PATH_IS_NOT_EXISTS);
 
        sdsfree(path);
        sdsfree(x);
        sdsfree(y);
        if (result) sdsfree(result);
        test_cond("DeleteDir(\"testdir\")", DeleteDir("testdir")==0);
        test_cond("DirectoryExists('testdir') should be not exists", DirectoryExists("testdir") == PATH_IS_NOT_EXISTS);
    }
    test_report();
    return 0;
}
#endif
