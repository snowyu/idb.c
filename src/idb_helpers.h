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
 #include "sds.h"
 #include "isdk_xattr.h"
 #include "isdk_utils.h"

 #define IDB_KEY_TYPE_NAME ".type"
 #define IDB_VALUE_NAME ".value"
 #define XATTR_PREFIX  "user."

 //the store types:
 #define STORE_IN_FILE  1
 #define STORE_IN_XATTR 2
 #define STORE_IN_FILE_BIT  0 //the 0 bit
 #define STORE_IN_XATTR_BIT 1 //the 1 bit

 #ifdef __cplusplus
 extern "C"
  {
 #endif

 //Low-Level functions
 //-1 means err, 0 means false, 1 means true.
 int IsDirValueExists(const sds aDir, const sds aAttribute);
 /* Get the value of the a Key(aDir) if successful, else return NULL
  * U must free the result after using it.
  */
 sds GetDirValue(const sds aDir, const sds aAttribute);
 int SetDirValue(const sds aDir, const sds aValue, const sds aAttribute);


 //the atrribute operations:
 //if aAttribute is NULL means the default attribute: .value
 int iIsExists(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType);
 sds iGet(const sds aDir, const char* aKey, const int aKeyLen, const sds aAttribute, const int aStoreType);
 int iPut(const sds aDir, const char* aKey, const int aKeyLen, const sds aValue, const sds aAttribute, const int aStoreType);
//int symlink(const char *srcPath, const char *destPath); //make symbolic link(destPath) to a srcPath

 //the key operations:
 //Make a new aAlias of the aKey
 //return: Upon successful completion, a zero value is returned.  else the error code returned.
 int iAlias(const sds aDir, const char* aKey, const int aKeyLen, const char* aAlias, const int aAliasLen);
 //delete the key includes the subkeys.
 int iDelete(const sds aDir, const char* aKey, const int aKeyLen);
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
 dStringArray* iSubkeys(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern, const int aSkipCount, const int aCount);
 size_t iSubkeyCount(const sds aDir, const char* aKey, const int aKeyLen, const char* aPattern);

 #ifdef __cplusplus
 }
 #endif

#endif
