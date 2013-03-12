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

#include "isdk_config.h"
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
#include <assert.h>

#include "deps/utf8proc.h"

#include "idb_helpers.h"

 //Global IDB Options:
 //<=0 means no limit. the max iDB items count in the same dir(=MaxPageSize).
 //Note: the MaxPageSize MUST greater than ASCII table count.
 int IDBMaxPageCount = -1;//256;
 //howto process the duplication keys when list subkeys, see iSubkeys
 //dkIgnored: DO NOT add the duplication key into list.
 //dkFixed: remove the left duplication key.
 //dkStopped: stopped and raise error.
 //dkReserved: add the duplication key into list too.
 TIDBProcesses IDBDuplicationKeyProcess = dkIgnored;
 //the available process way when putting:
 //dkIgnored: force to put the key in prev full dir.
 //dkStopped: stopped and raise error.
 TIDBProcesses IDBPartitionFullProcess  = dkIgnored;

static inline sds _make_attr_file_name(const sds aDir, const char *aAttribute)
{
    sds vFile = sdsdup(aDir);
    if (aAttribute != NULL && *aAttribute != '\0') {
        vFile = sdsJoinPathLen(vFile, aAttribute, strlen(aAttribute));
    }
    else {
        vFile = sdsJoinPathLen(vFile, IDB_VALUE_NAME, 6);
    }
    return vFile;
}


bool DelDirValue(const sds aDir, const char *aAttribute)
{
    sds vFile = _make_attr_file_name(aDir, aAttribute);
    int result = unlink(vFile);
    sdsfree(vFile);
    return result == 0;
}

bool IsDirValueExists(const sds aDir, const char *aAttribute)
{
    sds vFile = _make_attr_file_name(aDir, aAttribute);
    int result = DirectoryExists(vFile);
    sdsfree(vFile);
    return result == PATH_IS_FILE;
}

//http://codewiki.wikidot.com/system-calls
sds GetDirValue(const sds aDir, const char *aAttribute)
{
    sds result = _make_attr_file_name(aDir, aAttribute);
    int fd = open(result, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed:%s\n", result);
        sdsfree(result);
        result = NULL;
    } else {
        sdsclear(result);
        int vSize = lseek(fd, 0L, SEEK_END);
        lseek(fd, 0L, SEEK_SET);
        result = sdsMakeRoomFor(result, vSize);
        vSize = read(fd, result, vSize);
        sdsIncrLen(result, vSize);
        close(fd);
    }
    return result;
}

//return true means ok. false means failed.
bool SetDirValue(const sds aDir, const sds aValue, const char *aAttribute)
{
    int result = false;
    sds vFile = _make_attr_file_name(aDir, aAttribute);
    int fd = open(vFile, O_WRONLY|O_TRUNC|O_CREAT, O_RW_RW_R__PERMS);
    if (fd >= 0) {
        int vSize = write(fd, aValue, sdslen(aValue));
        if (vSize == sdslen(aValue)) result = true;
        close(fd);
    }
    sdsfree(vFile);
    return result;
}

static inline sds GenerateXattrName(const char *aAttribute)
{
    sds s = sdsnew(XATTR_PREFIX);
    if (aAttribute) {
        //sds v = aAttribute;
        if (aAttribute[0] == IDB_ATTR_PREFIX_CHR) aAttribute++;
        s = sdscat(s, aAttribute);
    }
    else
        s = sdscat(s, IDB_VALUE_NAME+1); //skip the dot.
    return s;
}

static inline sds _iGet(const sds aDir, const char *aAttribute, const int aStoreType)
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

static inline int _iPut(const sds aDir, const sds aValue, const char *aAttribute, const int aStoreType)
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

static inline size_t _iAttrCount(const sds aDir, const char* aPattern) {
    size_t result = CountDir(aDir, aPattern, LIST_ALL_FILES);
    return result;
}

static inline bool _DeleleAttr(const sds aDir, const char *aAttribute, const int aStoreType)
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

static bool _AttrIsExists(const sds aDir, const char *aAttribute, const int aStoreType)
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
    //  size_t vCount = iSubkeyCount(vDir, NULL);
    //  if vCount < aMaxItemCount then begin
    //    i--;
    //    break;
    //  end;
    //end;
static sds _GetKeyDir(const sds aDir, const int aMaxItemCount, const TIDBProcesses aPartitionFullProcess)
{
    int vDirExists = DirectoryExists(aDir);
    if (vDirExists == PATH_IS_DIR) {
        //the Key Directory is exists
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
    errno = 0;
    const sds vKey = ExtractLastPathName(aDir);
    ssize_t vKeySize = sdslen(vKey);
    char *s = vKey;
    sds vDir = aDir;
    if (DirectoryExists(vDir) == PATH_IS_DIR && iSubkeyCount(vDir, NULL) >= aMaxItemCount) {
        sds vUtf8Char = sdsalloc(NULL, 6); //the maximum size of the utf-8 char is 6.
        do {
            //vLen means character byte length.
            ssize_t vLen = IterateUtf8Char(s, vKeySize, vUtf8Char);
            if (vLen > 0) {
                sdsSetlen_(vUtf8Char, vLen);
                s += vLen;
                vKeySize -= vLen;
                if (vKeySize <= 0) break;
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
                    if (iSubkeyCount(vDir, NULL) < aMaxItemCount) {
                        break;
                    }
                }
                else {
                    break;
                }
            } else {
                errno = vLen;
                switch (vLen) {
                    case 0:
                        //errno = IDB_ERR_PART_FULL;
                        warnx("_GetKeyDir: the partition is full at %s dir", vDir);
                        break;
                    case UTF8PROC_ERROR_INVALIDUTF8 :
                        warnx("_GetKeyDir:%s UTF8PROC_ERROR_INVALIDUTF8", vKey);
                        break;
                }
                sdsfree(vUtf8Char);
                if (vKey) sdsfree(vKey);
                sdsfree(aDir);
                return NULL;
            }
        } while(vKeySize > 0);
        if (vKeySize <= 0) {
            if (aPartitionFullProcess == dkIgnored) {
                vDir = sdsJoinPathLen(vDir, vUtf8Char, sdslen(vUtf8Char));
            }
            else 
            if (aPartitionFullProcess == dkStopped) {
                errno = IDB_ERR_PART_FULL;
                sdsfree(vDir);
                vDir = NULL;

            }
        }
        sdsfree(vUtf8Char);
    }
    if (vKeySize > 0) {
        vDir = sdsJoinPathLen(vDir, s, vKeySize);
    }
    if (vKey) sdsfree(vKey);

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
    errno = 0;
    const sds vKey = ExtractLastPathName(aDir);
    if (vKey == NULL) return NULL;
    char *s = vKey;
    sds vDir = aDir;
    ssize_t vKeySize = sdslen(vKey);
    sds vUtf8Char = sdsalloc(NULL, 6); //the maximum size of the utf-8 char is 6.
    do {
        //vLen means character byte length.
        ssize_t vLen = IterateUtf8Char(s, vKeySize, vUtf8Char);
        if (vLen > 0)  {
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

bool iIsExistsInFile(const sds aKeyPath, const char *aAttribute)
{
    bool result = IsDirValueExists(aKeyPath, aAttribute);
    if (result == false && IDBMaxPageCount > 0) {
        sds vKeyPath = _IsKeyDirExists(sdsdup(aKeyPath));
        result = (vKeyPath != NULL);
        if (result) {
            result = IsDirValueExists(vKeyPath, aAttribute);
            sdsfree(vKeyPath);
        }
    }
    return result;
}

bool iIsExistsInXattr(const sds aKeyPath, const char *aAttribute)
{
    sds s = GenerateXattrName(aAttribute);
    bool result = IsXattrExists(aKeyPath, s);
    if (result == false && IDBMaxPageCount > 0) {
        sds vKeyPath = _IsKeyDirExists(sdsdup(aKeyPath));
        result = (vKeyPath != NULL);
        if (result) {
            result = IsXattrExists(vKeyPath, s);
            sdsfree(vKeyPath);
        }
    }
    sdsfree(s);
    return result;
}

bool iIsExists(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    bool result = _AttrIsExists(vDir, aAttribute, aStoreType);
    if (result == false && IDBMaxPageCount > 0) {
        vDir = _IsKeyDirExists(vDir);
        result = (vDir != NULL);
        if (result) result = _AttrIsExists(vDir, aAttribute, aStoreType);
        //if (vDir == NULL) return false; //No such key or some error occur(see errno).
    }
    if (vDir) sdsfree(vDir);
    return result;
}

sds iGetInFile(const sds aKeyPath, const char *aAttribute)
{
    sds result = GetDirValue(aKeyPath, aAttribute);
    if (result == NULL && IDBMaxPageCount > 0) {
        sds vDir = _IsKeyDirExists(sdsdup(aKeyPath));
        if (vDir != NULL) {
            result = GetDirValue(vDir, aAttribute);
            sdsfree(vDir);
        }
    }
    return result;
}

sds iGetInXattr(const sds aKeyPath, const char *aAttribute)
{
    sds s = GenerateXattrName(aAttribute);
    sds result = GetXattr(aKeyPath, s);
    if (result == NULL && IDBMaxPageCount > 0) {
        sds vDir = _IsKeyDirExists(sdsdup(aKeyPath));
        if (vDir != NULL) {
            result = GetXattr(vDir, s);
            sdsfree(vDir);
        }
    }
    sdsfree(s);
    return result;
}

sds iGet(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType){
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    sds result = _iGet(vDir, aAttribute, aStoreType);
    if (result == NULL && IDBMaxPageCount > 0) {
        vDir = _IsKeyDirExists(vDir);
        if (vDir != NULL) result = _iGet(vDir, aAttribute, aStoreType);
    }
    if (vDir) sdsfree(vDir);
    return result;
}

int iPutInFile(const sds aKeyPath, const sds aValue, const char *aAttribute, const TIDBProcesses aPartitionFullProcess)
{
    sds vDir = NULL;
    int vAdjusted = 0;
    if (vDir && IDBMaxPageCount > 0) {
        vDir = _GetKeyDir(vDir, IDBMaxPageCount, aPartitionFullProcess);
        if (vDir == NULL) return errno;
    }
    int result = DirectoryExists(vDir);
    if (result == PATH_IS_NOT_EXISTS) { //Dir not Exists: it is a new key.
        ForceDirectories(vDir, O_RWXRWXR_XPERMS);
    }
    else if (result == PATH_IS_FILE) {//File Already Exists Error:
        return result;
    }
    if(vDir) {
        result = !SetDirValue(vDir, aValue, aAttribute);
        sdsfree(vDir);
    }
    else {
        result = !SetDirValue(aKeyPath, aValue, aAttribute);
    }
    
    return result;
}

int iPutInXattr(const sds aKeyPath, const sds aValue, const char *aAttribute, const TIDBProcesses aPartitionFullProcess)
{
    sds vDir = NULL;
    int vAdjusted = 0;
    if (vDir && IDBMaxPageCount > 0) {
        vDir = _GetKeyDir(vDir, IDBMaxPageCount, aPartitionFullProcess);
        if (vDir == NULL) return errno;
    }
    int result = DirectoryExists(vDir);
    if (result == PATH_IS_NOT_EXISTS) { //Dir not Exists: it is a new key.
        ForceDirectories(vDir, O_RWXRWXR_XPERMS);
    }
    else if (result == PATH_IS_FILE) {//File Already Exists Error:
        return result;
    }
    sds s = GenerateXattrName(aAttribute);
    if(vDir) {
        result = SetXattr(vDir, s, aValue);
        sdsfree(vDir);
    }
    else {
        result = SetXattr(aKeyPath, s, aValue);
    }
    sdsfree(s);
    
    return result;
}

//result = 0 means ok, ENOEXEC means no operation, -1(PATH_IS_FILE) means the same file name exists error, 
int iPut(const sds aDir, const char* aKey, const int aKeyLen, const sds aValue, const char *aAttribute, const int aStoreType)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    int vAdjusted = 0;
    if (aKey && IDBMaxPageCount > 0) {
        vDir = _GetKeyDir(vDir, IDBMaxPageCount, IDBPartitionFullProcess);
        if (vDir == NULL) return errno;
    }
    int result = DirectoryExists(vDir);
    if (result == PATH_IS_NOT_EXISTS) { //Dir not Exists: it is a new key.
        ForceDirectories(vDir, O_RWXRWXR_XPERMS);
    }
    else if (result == PATH_IS_FILE) {//File Already Exists Error:
        return result;
    }
    result = _iPut(vDir, aValue, aAttribute, aStoreType);
    
    sdsfree(vDir);
    return result;
}

bool iDeleleInFile(const sds aKeyPath, const char *aAttribute)
{
    bool result = false;
    sds vDir = sdsdup(aKeyPath);
    if (DirectoryExists(vDir) != PATH_IS_DIR && IDBMaxPageCount > 0){
        vDir = _IsKeyDirExists(vDir);
    }
    if (vDir) {
        result = DelDirValue(vDir, aAttribute);
        sdsfree(vDir);
    }
    return result;
}

bool iDeleleInXattr(const sds aKeyPath, const char *aAttribute)
{
    bool result = false;
    sds vDir = sdsdup(aKeyPath);
    if (DirectoryExists(vDir) != PATH_IS_DIR && IDBMaxPageCount > 0){
        vDir = _IsKeyDirExists(vDir);
    }
    if (vDir) {
        sds s = GenerateXattrName(aAttribute);
        result = DelXattr(vDir, s);
        if (result != 0) {
            errno = result;
            result = false;
        } else
            result = true;
        sdsfree(s);
        sdsfree(vDir);
    }
    return result;
}

bool iDelele(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType)
{
    bool result = false;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    if (DirectoryExists(vDir) != PATH_IS_DIR && IDBMaxPageCount > 0){
        vDir = _IsKeyDirExists(vDir);
    }
    if (vDir) {
      result = _DeleleAttr(vDir, aAttribute, aStoreType);
      sdsfree(vDir);
    }
    return result;
}

bool iKeyDelete(const sds aDir, const char* aKey, const int aKeyLen){
    int result = false;
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    if (DirectoryExists(vDir) != PATH_IS_DIR && IDBMaxPageCount > 0){
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
    sds result = sdsalloc(NULL, aToLen+(vSepCount*3));
    while (vSepCount) {
        result = sdscatlen(result, "..", 2);
        result = sdscatlen(result, PATH_SEP_STR, 1);
        vSepCount--;
    }
    result = sdscat(result, vToPart);
    return result;
}

int iKeyAlias(const sds aDir, const char* aKey, const int aKeyLen, const char* aAliasPath, const int aAliasPathLen)
{
    //sds vSrcDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    sds vDestDir = sdsJoinPathLen(sdsdup(aDir), aAliasPath, aAliasPathLen);
    sds vSrcDir = GetRelativePath(aAliasPath, aAliasPathLen, aKey, aKeyLen);
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
    sds dir;
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
            vKey = sdscatlen(vKey, aItem->d_name, DIR_NAME_LEN(aItem));
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
        vRec.subkeyPart = sdscatlen(sdsdup(aRec->subkeyPart), aItem->d_name+1, DIR_NAME_LEN(aItem)-1);
        ssize_t vCount = _iSubkeyWalk(aDir, aItem->d_name, DIR_NAME_LEN(aItem), &vRec);
        sdsfree(vRec.subkeyPart);
        if (vCount > 0) {
            aRec->count += vCount;
            if (aRec->leftCount > 0 && vCount >= aRec->leftCount)
                result = WALK_ITEM_STOP; //stop walk.
        }
        else if (vCount < 0)
            result = WALK_ITEM_ERROR;
    }
    return result;
}

static ssize_t _iSubkeyWalk(const sds aDir, const char* aKey, const int aKeyLen, TSubkeyWalkerRec *aRec)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    //int vDirLen = sdslen(vDir);
    //char vLastChar = vDir[vDirLen-1];
    ssize_t result = WalkDir(vDir, aRec->pattern, LIST_NORMAL_DIRS, 0, aRec->leftCount, (WalkDirHandler) _SubkeyWalker, (void*) aRec);
    if (result>=0 && (aRec->leftCount <=0 || result < aRec->leftCount)) {
        aRec->leftCount = aRec->leftCount - result;
        sds vSubkeyPart = sdsalloc(NULL, 8);
        //TODO:this should be a repeat loop until pattern[0] == '*' or '?' or NULL
        if (aRec->pattern && aRec->pattern[0] != '\0' && aRec->pattern[0] != '*' && aRec->pattern[0] != '?') {
            sds vIndexKey = sdsnewlen(".", 1);
            vIndexKey = sdsMakeRoomFor(vIndexKey, 8);
            int vLen = IterateUtf8Char(aRec->pattern, 6, vIndexKey+1);
            if (vLen > 0) {
                aRec->subkeyPart = sdscatlen(aRec->subkeyPart, aRec->pattern, vLen);
                vIndexKey = sdscatlen(vIndexKey, aRec->pattern, vLen);
                aRec->pattern += vLen;
                int vCount = _iSubkeyWalk(vDir, vIndexKey, sdslen(vIndexKey), aRec);
                sdsfree(vIndexKey);
                //aRec->count += vCount;
                result += vCount;
            }
            else { //error occur. invalid utf8 string.
                result = -1;
                errno = vLen;
            }
        }
        else { //Walk through the Partition Keys
            TSubkeyWalkerRec vRec = *aRec;
            vRec.count = result;
            result = WalkDir(vDir, NULL, LIST_HIDDEN_DIRS, 0, aRec->leftCount, (WalkDirHandler) _SubkeyPartitionWalker, (void*) &vRec);
            if (result >= 0)
                result = vRec.count;
        }
        sdsfree(vSubkeyPart);

    }
    //sdsSetlen_(vDir, vDirLen);
    //vDir[vDirLen-1] = vLastChar;
    sdsfree(vDir);
    return result;
}
//First Walk through the Normal Keys, then walk through the Partition Keys
//aPattern: only for Normal Keys!
ssize_t iSubkeyWalk(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern,
        size_t aSkipCount, size_t aCount, const WalkKeyHandler aProcessor, const void *aUserPtr)
{
    assert(aDir || aKeyLen > 0);
    sds vDir = aDir;
    sds vSubkeyPart = sdsalloc(NULL, 8);
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
    //sdsfree(vDir);
    return result;
}

ssize_t iSubkeyTotal(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern)
{
    sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
    ssize_t result = iSubkeyWalk(aDir, aKey, aKeyLen, aPattern, 0, 0, NULL, NULL);
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
        case dkStopped:
        case dkFixed: {
            ssize_t i = dStringArray_indexOf(aSubkeys, (sds)aKey);
            if (i < 0) { ////Not Exists in aSubkeys
                vItem = sdsnew(aKey);
            } else {
                if (IDBDuplicationKeyProcess == dkFixed) {
                    if (DeleteDir(aDir) != 0) { //remove duplication failed:
                        warnx("Remove Duplication Key(%s): Delete \"%s\" Failed", aKey, aDir);
                    }
                    result = WALK_ITEM_SKIP;
                }
                else if (IDBDuplicationKeyProcess == dkStopped) {
                    errno = IDB_ERR_PART_DUP_KEY;
                    result = WALK_ITEM_ERROR;
                }
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
        //errno = vCount;
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
            for (int i=0; i< darray_size(*result); i++) {
                    s = sdsnew(s1);
                    s = sdscat(s, darray_item(*result, i));
                    test_cond(s, dCStrArray_indexOf(&expected, darray_item(*result, i))>=0);
                    sdsfree(s);
            }
        } else {
            printf("expected Length is %lu, but it is %lu in fact.\n", darray_size(expected), darray_size(*result));
        }
}
static inline void test_getKey(sds aDir, const char* aKey, const char* aValue, const sds aAttribute, int aStoreType)
{
    sds vMsg = sdsnew("");
    int vValueSize = strlen(aValue);
    sds result = iGet(aDir, aKey, strlen(aKey), aAttribute, aStoreType);
    vMsg = sdscatprintf(vMsg, "iGet('%s', '%s', %s, %d)", aDir, aKey, aAttribute, aStoreType);
    test_cond(vMsg,
              result && sdslen(result) == vValueSize && memcmp(result, aValue, vValueSize+1) == 0
    );
    sdsfree(result);
    sdsfree(vMsg);
}
static inline void test_putKey(sds aDir, const char* aKey, const char* aValue, const sds aAttribute, int aStoreType, int aErrno)
{
    sds vMsg = sdsnew("");
    int vValueSize = strlen(aValue);
    sds vValue = sdsnewlen(aValue, vValueSize);
    vMsg = sdscatprintf(vMsg, "iPut('%s', '%s', '%s', %s, %d)", aDir, aKey, aValue, aAttribute, aStoreType);
    test_cond(vMsg, iPut(aDir, aKey, strlen(aKey), vValue, aAttribute, aStoreType) == aErrno);
    if (aErrno == IDB_OK) test_getKey(aDir, aKey, aValue, aAttribute, aStoreType);
    sdsfree(vMsg);
    sdsfree(vValue);
}
int main(int argc, char **argv)
{
    {
        ForceDirectories("testdir/good/better/best", O_RWXRWXR_XPERMS);
        test_cond("DirectoryExists('testdir/good/better/best')", DirectoryExists("testdir/good/better/best") == PATH_IS_DIR);
        IDBMaxPageCount = -1;
        printf("Testing when Disable MaxPageSize:\n");
        printf("---------------------------------\n");
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
        if (result) SDSFreeAndNil(result);
        result = iGet(x, key, strlen(key), NULL, STORE_IN_FILE);
        test_cond("iGet(testdir, mytestkey, NULL, STORE_IN_FILE)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        test_cond("iDeleteAttr(testdir, mytestkey, STORE_IN_FILE)", iDelele(x, key, strlen(key),NULL, STORE_IN_FILE));
        test_cond("IsDirValueExists(testdir/mytestkey, NULL) == false", !IsDirValueExists(path, NULL));
        test_cond("iIsExists(testdir, mytestkey) should not be exists.", !iIsExists(x, key, strlen(key), NULL, STORE_IN_FILE));
        test_cond("iKeyDelete(testdir, mytestkey)", iKeyDelete(x, key, sdslen(key)));
        test_cond("!DirectoryExists('testdir/mytestkey')", DirectoryExists("testdir/mytestkey") == PATH_IS_NOT_EXISTS);

        test_cond("iPut('testdir','mytestkey', 'hi world', STORE_IN_XATTR)", iPut(x, key, sdslen(key), y, NULL, STORE_IN_XATTR)==0);
        test_cond("IsDirValueExists(testdir/mytestkey, NULL)", !IsDirValueExists(path, NULL));
        test_cond("IsXattrExists(testdir/mytestkey, IDB_VALUE_NAME)", IsXattrExists(path, xa_value_name));
        test_cond("iIsExists(testdir, mytestkey, STORE_IN_XATTR) should be exists.", iIsExists(x, key, strlen(key),NULL, STORE_IN_XATTR));
        if (result) SDSFreeAndNil(result);
        result = GetXattr(path, xa_value_name);
        test_cond("GetXattr(testdir/mytestkey, IDB_VALUE_NAME)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        if (result) SDSFreeAndNil(result);
        result = iGet(x, key, strlen(key), NULL, STORE_IN_XATTR);
        test_cond("iGet(testdir, mytestkey, NULL, STORE_IN_XATTR)",
                result && sdslen(result) == 8 && memcmp(result, "hi world\0", 9) == 0
        );
        if (result) SDSFreeAndNil(result);
        //
        test_putKey(x, "myhi/see/u", y, NULL, STORE_IN_FILE, IDB_OK);
        test_putKey(x, "myhi/see/.u/s", y, NULL, STORE_IN_FILE, IDB_OK);
        test_putKey(x, "myhi/see/.u/.s/c", y, NULL, STORE_IN_FILE, IDB_OK);
        test_putKey(x, "myhi/see/d/uack", y, NULL, STORE_IN_FILE, IDB_OK);
        test_putKey(x, "myhi/fa/si", y, NULL, STORE_IN_FILE, IDB_OK);
        test_putKey(x, "mygoo", y, NULL, STORE_IN_FILE, IDB_OK);
        test_putKey(x, "bygoo", y, NULL, STORE_IN_FILE, IDB_OK);
        sds adjustedDir= sdsJoinPathLen(sdsdup(x), "myhi/see/usmek", 14);
        adjustedDir= _GetKeyDir(adjustedDir, 1, IDBPartitionFullProcess);
        test_cond("_GetKeyDir('my/hi/see/usmek', 1)", strcmp(adjustedDir, "testdir/myhi/see/.u/.s/.m/ek") == 0);
        sdsfree(adjustedDir);

        test_cond("iSubkeyTotal(x, '', 0, NULL) == 5",iSubkeyTotal(x, "", 0, NULL)==5);
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

        printf("---------------------------------------");
        printf("Alias testing.....");
        printf("---------------------------------------");
        iKeyAlias(x, "myhi/see/u", 10, "myhi/see/u1", 11);
        test_cond("iKeyAlias('myhi/see/u', 'myhi/see/u1')", IsDirectory("testdir/myhi/see/u1") == PATH_IS_SYM_DIR);

        iKeyAlias(x, "myhi/see/u", 10, "myhi/fa/u", 11);
        test_cond("iKeyAlias('myhi/see/u', 'myhi/fa/u')", IsDirectory("testdir/myhi/fa/u") == PATH_IS_SYM_DIR);
        test_putKey(x, "god4u", "turn on value", NULL, STORE_IN_FILE, IDB_OK);
        iKeyAlias(x, "god4u", 5, "myhi/see/u2", 11);
        test_getKey(x, "myhi/see/u2", "turn on value", NULL, STORE_IN_FILE);
        printf("---------------------------------------");

        test_cond("iDelele(testdir, mytestkey, STORE_IN_XATTR)", iDelele(x, key, strlen(key),NULL, STORE_IN_XATTR));
        test_cond("!IsXattrExists(testdir/mytestkey, IDB_VALUE_NAME)", !IsXattrExists(path, xa_value_name));
        test_cond("!iIsExists(testdir, mytestkey, STORE_IN_XATTR)", !iIsExists(x, key, strlen(key), NULL, STORE_IN_XATTR));
        test_cond("iKeyDelete(testdir, mytestkey)", iKeyDelete(x, key, sdslen(key)));
        test_cond("!DirectoryExists('testdir/mytestkey')", DirectoryExists("testdir/mytestkey") == PATH_IS_NOT_EXISTS);
        IDBMaxPageCount = 3;
        IDBDuplicationKeyProcess = dkIgnored;
        printf("-------------------------------------\n");
        printf("Testing when Enable MaxPageSize to 3:\n");
        printf("-------------------------------------\n");
        test_cond("DeleteDir(\"testdir\")", DeleteDir("testdir")==0);
        test_cond("!DirectoryExists('testdir')", DirectoryExists("testdir") == PATH_IS_NOT_EXISTS);
        test_putKey(x, "mygoo", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("DirectoryExists('testdir/mygoo')", DirectoryExists("testdir/mygoo") == PATH_IS_DIR);
        test_putKey(x, "bygoo", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("DirectoryExists('testdir/bygoo')", DirectoryExists("testdir/bygoo") == PATH_IS_DIR);
        test_putKey(x, "aygoo", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("DirectoryExists('testdir/aygoo')", DirectoryExists("testdir/aygoo") == PATH_IS_DIR);
        test_putKey(x, "cygoo", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/cygoo')", DirectoryExists("testdir/cygoo") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.c/ygoo')", DirectoryExists("testdir/.c/ygoo") == PATH_IS_DIR);
        test_putKey(x, "cm", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/cm')", DirectoryExists("testdir/cm") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.c/m')", DirectoryExists("testdir/.c/m") == PATH_IS_DIR);
        y = sdscat(y, "3");
        test_putKey(x, "c3", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/c3')", DirectoryExists("testdir/c3") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.c/3')", DirectoryExists("testdir/.c/3") == PATH_IS_DIR);
        puts("-----------------------------------");
        puts("Test raise partition full as error:");
        puts("-----------------------------------");
        IDBPartitionFullProcess = dkStopped;
        test_putKey(x, "c4", y, NULL, STORE_IN_FILE, IDB_ERR_PART_FULL);
        test_cond("!DirectoryExists('testdir/c4')", DirectoryExists("testdir/c4") == PATH_IS_NOT_EXISTS);
        test_cond("!DirectoryExists('testdir/.c/4')", DirectoryExists("testdir/.c/4") == PATH_IS_NOT_EXISTS);
        puts("----------------------------");
        puts("Test Ignore partition full:");
        puts("----------------------------");
        IDBPartitionFullProcess = dkIgnored;
        test_putKey(x, "c4", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/c4')", DirectoryExists("testdir/c4") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.c/4')", DirectoryExists("testdir/.c/4") == PATH_IS_DIR);
        test_cond("iKeyDelete(testdir, c4)", iKeyDelete(x, "c4", 2));
        puts("----------------------------");
        test_putKey(x, "cygo1", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/cygo1')", DirectoryExists("testdir/cygo1") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.c/.y/go1')", DirectoryExists("testdir/.c/.y/go1") == PATH_IS_DIR);
        test_putKey(x, "中国", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/中国')", DirectoryExists("testdir/中国") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.中/国')", DirectoryExists("testdir/.中/国") == PATH_IS_DIR);
        test_putKey(x, "中华", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/中华')", DirectoryExists("testdir/中华") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.中/华')", DirectoryExists("testdir/.中/华") == PATH_IS_DIR);
        test_putKey(x, "中间", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/中间')", DirectoryExists("testdir/中间") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.中/间')", DirectoryExists("testdir/.中/间") == PATH_IS_DIR);
        test_putKey(x, "中华人", y, NULL, STORE_IN_FILE, IDB_OK);
        test_cond("!DirectoryExists('testdir/中华人')", DirectoryExists("testdir/中华人") == PATH_IS_NOT_EXISTS);
        test_cond("DirectoryExists('testdir/.中/.华/人')", DirectoryExists("testdir/.中/.华/人") == PATH_IS_DIR);
        test_cond("iSubkeyTotal(x, '', 0, NULL) == 11",iSubkeyTotal(x, "", 0, NULL)==11);
        puts("----------------------------");
        puts("iSubkeys(x, \"\", 0, NULL, 0, 0)");
        puts("----------------------------");
        darray_init(vExpectedList);
        darray_appends_t(vExpectedList, const char*, "aygoo", "bygoo", "cygoo", "mygoo", "cm", "c3", "cygo1", "中国", "中华", "中间", "中华人");
        vList = iSubkeys(x, "", 0, NULL, 0, 0);
        test_list(vList, vExpectedList);
        dStringArray_free(vList);
        darray_free(vExpectedList);
        puts("----------------------------");
 
        sdsfree(path);
        sdsfree(x);
        sdsfree(y);
        SDSFreeAndNil(key);
        sdsfree(xa_value_name);
        if (result) SDSFreeAndNil(result);
        test_cond("DeleteDir(\"testdir\")", DeleteDir("testdir")==0);
        test_cond("DirectoryExists('testdir') should be not exists", DirectoryExists("testdir") == PATH_IS_NOT_EXISTS);
    }
    test_report();
    return 0;
}
#endif
