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

 #define IDB_ERR_PART_FULL          -100  //the iDB local partition is full.
 #define IDB_ERR_PART_DUP_KEY       -101  //the Duplication Key occur
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
 typedef ssize_t(*WalkKeyHandler)(size_t aCount, const char* aDir, const char *aKey, const char *aSubkeyPart, const void *aUserPtr);


 //Global IDB Options:
 //<=0 means no limit. the max iDB items count in the same dir(=MaxPageSize).
 //Note: the MaxPageSize MUST greater than ASCII table count.
 int IDBMaxItemCount = -1;//256;
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

 //Low-Level functions
 bool DelDirValue(const sds aDir, const sds aAttribute);
 bool IsDirValueExists(const sds aDir, const sds aAttribute);
 /* Get the value of the a Key(aDir) if successful, else return NULL
  * U must free the result after using it.
  */
 sds GetDirValue(const sds aDir, const sds aAttribute);
 bool SetDirValue(const sds aDir, const sds aValue, const sds aAttribute);


 //the atrribute operations:
 //if aAttribute is NULL means the default attribute: .value
 bool iIsExists(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType);
 sds iGet(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType);
 //result = 0 means ok, ENOEXEC means no operation, -1(PATH_IS_FILE) means the same file name exists error, 
 int iPut(const sds aDir, const char* aKey, const int aKeyLen, const sds aValue, const sds aAttribute, const int aStoreType);
 bool iDeleleAttr(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType);
//int symlink(const char *srcPath, const char *destPath); //make symbolic link(destPath) to a srcPath

 //the key operations:
 //Make a new aAlias of the aKey
 //return: Upon successful completion, a zero value is returned.  else the error code returned.
 int iAlias(const sds aDir, const char* aKey, const int aKeyLen, const char* aAlias, const int aAliasLen);
 //delete the key includes the subkeys.
 bool iDelete(const sds aDir, const char* aKey, const int aKeyLen);
 //is the key exists?
 //-1 means err, 0 means false, 1 means true.
 static inline int iKeyExists(const sds aDir, const char* aKey, const int aKeyLen)
 {
     sds vDir = sdsJoinPathLen(sdsdup(aDir), aKey, aKeyLen);
     int result = DirectoryExists(vDir);
     sdsfree(vDir);
     return result;
 }

 //the subkey operations:
 //aPattern = NULL means match all subkeys
 //return -1 means error, the errno is error code.
 ssize_t iSubkeyWalk(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern,
        size_t aSkipCount, size_t aCount, const WalkKeyHandler aProcessor, const void *aUserPtr);
 dStringArray* iSubkeys(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern, const int aSkipCount, const int aCount);
 ssize_t iSubkeyCount(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern);

 #ifdef __cplusplus
 }
 #endif

#endif
