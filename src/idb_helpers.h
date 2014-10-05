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

#ifndef idb_helpers__h
 #define idb_helpers__h

 #include <stdbool.h>
 #include <stdio.h>
 #include <stdarg.h>

 /*use the redis C dynamic strings library: sds.h*/
 #include "deps/sds.h"
 #include "isdk_xattr.h"
 #include "isdk_utils.h"

 #define IDB_KEY_TYPE_NAME          ".type"
 #define IDB_VALUE_NAME             ".value"
 //#define IDB_SPLIT_ATTR_POSTFIX ".me"                           //the attribute postfix for the split key.
 //#define IDB_SPLIT_KEY_NAME     ".key"IDB_SPLIT_ATTR_POSTFIX   //store the key name in the file for the split key dir.
 #define XATTR_PREFIX               "user."
 #define IDB_ATTR_PREFIX            "."
 #define IDB_ATTR_PREFIX_CHR        IDB_ATTR_PREFIX[0]
 #define IDB_PART_DIR_PREFIX        "."                       //the prefix of the split key partition dir.
 #define IDB_PART_DIR_PREFIX_CHR    IDB_PART_DIR_PREFIX[0]

 #define IDB_ERR_INVALID_UTF8       UTF8PROC_ERROR_INVALIDUTF8
 #define IDB_ERR_PART_FULL          -100  //the iDB local partition is full.
 #define IDB_ERR_PART_DUP_KEY       -101  //the Duplication Key occur
 #define IDB_ERR_DUP_FILE_NAME      PATH_IS_FILE  //can not create the key for the same name file exists.
 #define IDB_ERR_KEY_NOT_EXISTS     -102
 #define IDB_OK                     0

 //the store types:
 #define STORE_IN_FILE  1
 #define STORE_IN_XATTR 2
 #define STORE_IN_FILE_BIT  0 //the 0 bit
 #define STORE_IN_XATTR_BIT 1 //the 1 bit

 #define LIST_ALL_FILES (LIST_FILES | 1<< LIST_SYMBOLIC | 1 << LIST_HIDDEN_FILE)

 #ifdef __cplusplus
 extern "C"
  {
 #endif

 //dkIgnored: do not care of the duplication key, just ignore the left duplication keys.
 //dkFixed: keep one, others remove the duplication key.
 //dkReserved: Keep all the keys includes the illegal duplication keys(with the original partition key path)
 typedef enum {dkIgnored, dkFixed, dkReserved, dkStopped} TIDBProcesses;
 typedef ssize_t(*WalkKeyHandler)(size_t aCount, const char* aDir, const char *aKey, const char *aSubkeyPart, const void *aUserPti, const TIDBProcesses aDuplicationKeyProcess);

 struct __iDBBase {
    char            *path;                  //the iDB item path.
    size_t          maxPageSize;            //the max page count for the subkeys
    int             storeType;
    //bool            storeInXattr;           //enabled the xattr storage.
    //bool            storeInFile;            //enabled the file storage.
    TIDBProcesses   duplicationKeyProcess;
    TIDBProcesses   partitionFullProcess;
 };
 typedef struct __iDBBase TIDBBase;

 //Global IDB Options:
 //<=0 means no limit. the max iDB items count in the same dir(=MaxPageSize).
 //Note: the MaxPageSize MUST greater than ASCII table count.
 extern int IDBMaxPageCount;// = -1;//256;
 //howto process the duplication keys when list subkeys, see iSubkeys
 //dkIgnored: DO NOT add the duplication key into list.
 //dkFixed: remove the left duplication key.
 //dkStopped: stopped and raise error.
 //dkReserved: add the duplication key into list too.
 extern TIDBProcesses IDBDuplicationKeyProcess;// = dkIgnored;
 //the available process way when putting:
 //dkIgnored: force to put the key in prev full dir.
 //dkStopped: stopped and raise error.
 extern TIDBProcesses IDBPartitionFullProcess;//  = dkIgnored;

 const char* idbErrorStr(int aErrno);
 sds GetRelativePath(const char* aFrom, const int aFromLen, const char* aTo, const int aToLen);

 //Low-Level functions
 bool DelDirValue(const char *aDir, const int aDirLen, const char *aAttribute);
 bool IsDirValueExists(const char *aDir, const int aDirLen, const char *aAttribute);
 /* Get the value of the a Key(aDir) if successful, else return NULL
  * U must free the result after using it.
  */
 sds GetDirValue(const char *aDir, const int aDirLen, const char *aAttribute);
 bool SetDirValue(const char *aDir, const int aDirLen, const char *aValue, const size_t aValueSize, const char *aAttribute);
 int64_t IncrByDirValue(const char *aDir, const int aDirLen, const int64_t aValue, const char *aAttribute);

 //return the real dir if such key exists, or return NULL
 //Note: the aKeyPath string space will be reused.
 sds iIsKeyDirExists(sds aKeyPath);
 //Find a proper dir name to save the partition key.
 sds iFindKeyDirToPut(sds aDir, const int aMaxItemCount, const TIDBProcesses aPartitionFullProcess);


 //the atrribute operations:
 //if aAttribute is NULL means the default attribute: .value
 bool iIsExistsInFile(const sds aKeyPath, const char *aAttribute);
 bool iIsExistsInXattr(const sds aKeyPath, const char *aAttribute);
 bool iIsExists(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType); //deprecated
 sds iGetInFile(const sds aKeyPath, const char *aAttribute);
 sds iGetInXattr(const sds aKeyPath, const char *aAttribute);
 sds iGet(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType); //deprecated
 int64_t iGetInt(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType); //deprecated
 int64_t iIncrBy(const sds aDir, const char* aKey, const int aKeyLen, long aValue, const char *aAttribute, const int aStoreType);
 int64_t iDecr(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType);
 int64_t iIncr(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType);
 //iIncrByInFile: the errno = 0 if successful, or error
 int64_t iIncrByInFile(const sds aKeyPath, int64_t aValue, const char *aAttribute, const TIDBProcesses aPartitionFullProcess);
 //result = 0 means ok, ENOEXEC means no operation, IDB_ERR_DUP_FILE_NAME means the same file name exists error, 
 int iPutInFile(const sds aKeyPath, const char *aValue, const size_t aValueSize, const char *aAttribute, const TIDBProcesses aPartitionFullProcess);
 int iPutInXattr(const sds aKeyPath, const char *aValue, const size_t aValueSize, const char *aAttribute, const TIDBProcesses aPartitionFullProcess);
 int iPut(const sds aDir, const char* aKey, const int aKeyLen, const char *aValue, const size_t aValueSize, const char *aAttribute, const int aStoreType); //deprecated
 int iPutInt(const sds aDir, const char* aKey, const int aKeyLen, int64_t aValue, const char *aAttribute, const int aStoreType);
 bool iDeleteInFile(const sds aKeyPath, const char *aAttribute);
 bool iDeleteInXattr(const sds aKeyPath, const char *aAttribute);
 bool iDelete(const sds aDir, const char* aKey, const int aKeyLen, const char *aAttribute, const int aStoreType); //deprecated
 //get attributes count.
 size_t iAttrCountInFile(const sds aKeyPath, const char* aPattern);
 //get the list of attributes
 dStringArray *iAttrsInFile(const sds aKeyPath, const char* aPattern, size_t aSkipCount, size_t aCount);

 //the key operations:
 //Make a new aAlias of the aKey
 //return: Upon successful completion, a zero value is returned.  else the error code returned.
 int iKeyAlias(const sds aDir, const char* aKey, const int aKeyLen, const char* aAliasPath, const int aAliasPathLen, TIDBProcesses aPartitionFullProcess);
 //delete the key includes the subkeys.
 bool iKeyDelete(const sds aDir, const char* aKey, const int aKeyLen);
 bool iKeyPathDelete(const sds aDir);
 //is the key exists?
 //-1 means err, 0 means false, 1 means true.
 int iKeyIsExists(const sds aDir, const char* aKey, const int aKeyLen);
 int iKeyPathIsExists(const sds aDir);

 //the subkey operations:
 //aPattern = NULL means match all subkeys
 //return -1 means error, the errno is error code.
 ssize_t iSubkeyWalk(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern,
        size_t aSkipCount, size_t aCount, const WalkKeyHandler aProcessor, const void *aUserPtr, const TIDBProcesses aDuplicationKeyProcess);
 //return NULL means error.
 dStringArray* iSubkeys(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern, const size_t aSkipCount, const size_t aCount, const TIDBProcesses aDuplicationKeyProcess);
 //all keys count(include partition).
 ssize_t iSubkeyTotal(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern, const TIDBProcesses aDuplicationKeyProcess);
 //current normal keys count(no partition).
 static inline size_t iSubkeyCount(const sds aKeyPath, const char* aPattern)
 {
    size_t result = CountDir(aKeyPath, aPattern, LIST_NORMAL_DIRS);
    return result;
 }

 #ifdef __cplusplus
 }
 #endif

#endif
