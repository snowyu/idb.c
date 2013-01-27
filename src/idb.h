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

#ifndef idb__h
 #define idb__h

 #include <stdbool.h>
 #include <stdio.h>
 #include <stdarg.h>

 /*use the redis C dynamic strings library: sds.h*/
 #include "sds.h"
 #include "isdk_xattr.h"
 #include "isdk_utils.h"

 #include "config.h"
 #include "idb_helpers.h"

#ifdef __GNUC__
    #define _ANONYMOUS_STRUCT __extension__
    #define __C89_NAMELESS __extension__
    #define __C89_NAMELESSSTRUCTNAME
#endif

//the simple types:
#define IDB_STR_TYPE        0x00
#define IDB_NUM_TYPE        0x01
//the complex types:
#define IDB_DICT_TYPE       0x20
#define IDB_LIST_TYPE       0x21

 #ifdef __cplusplus
 extern "C"
  {
 #endif

struct __iBaseItem {
  char *path;                   //the iDB item path.
  bool loadOnDemand;
  bool storeInXattr;            //enabled the xattr storage.
  bool storeInFile;             //enabled the file storage.
  bool raiseOnTypeMismatch;     //raise error if Type Mismatch when true
};
typedef struct __iBaseItem iBaseItem;

struct __iItem {
  //inheritance from iBaseItem:
  #ifdef _ANONYMOUS_STRUCT
    struct __iBaseItem;  //the anonymous-structs, only avaiable on C11: gcc -fms-extensions to enable it.
  #else
    //--------------------------------------------------------------------
    char *path;                   //the iDB item path.
    bool loadOnDemand;
    bool storeInXattr;            //enabled the xattr storage.
    bool storeInFile;             //enabled the file storage.
    bool raiseOnTypeMismatch;     //raise error if Type Mismatch when true
    //--------------------------------------------------------------------
  #endif
    int  type;             //the Item type.
    char *value;            //the Item value.
};
typedef struct __iItem iItem;


struct __iDB {
    //inheritance from iBaseItem:
  #ifdef _ANONYMOUS_STRUCT
    struct __iBaseItem;  //the anonymous-structs, only avaiable on C11: gcc -fms-extensions to enable it.
  #else
    //--------------------------------------------------------------------
    char *path;                   //the iDB item path.
    bool loadOnDemand;
    bool storeInXattr;            //enabled the xattr storage.
    bool storeInFile;             //enabled the file storage.
    bool raiseOnTypeMismatch;     //raise error if Type Mismatch when true
    //--------------------------------------------------------------------
  #endif
    bool opened;                  //whether the internal database is opened
};
typedef struct __iDB iDB;

//create a database object.
void* iDB_New(void);
//free a iDB Database object.
void  iDB_Free(void *aDB);
//
//  Open the Specified Database
//  *name: the database folder name
//
//  If successful, the return value is true, else, it is false.
bool  iDB_Open(void *aDB, sds aDBPath);
bool  iDB_Close(void *aDB);


//low-level operations:
//Get the type of a key
void* iDB_GetType(void *aDB, const void *aKey, int aKeySize, int *aTypeSize);
// Get the size of the type of a record.
// If successful, the return value is the size of the value of the corresponding record, else,
// it is -1.
int   iDB_GetTypeSize(void *aDB, const void *aKey, int aKeySize);

// Get the value of the key.
void* iDB_GetValue(void *aDB, const void *aKey, int aKeySize, int *aValueSize);
// Get the size of the value of a record.
// If successful, the return value is the size of the value of the corresponding record, else,
// it is -1.
int   iDB_GetValueSize(void *aDB, const void *aKey, int aKeySize);
bool  iDB_Delete(void *aDB, const void *aKey, int aKeySize);
//Insert or update a record(key/calue) 
bool  iDB_Put(void *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const void *aType, int aTypeSize);
bool  iDB_Update(iDB *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const void *aType, int aTypeSize);
bool  iDB_Insert(iDB *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const void *aType, int aTypeSize);
//----------------------------
// Add an integer to a record in a database object.
// `aDB' specifies the abstract database object.
// `aKey' specifies the pointer to the region of the key.
// `aKeySize' specifies the size of the region of the key.
// `aNumber' specifies the additional value.
// If successful, the return value is the summation value, else, it is `INT_MIN'.
// If the corresponding record exists, the value is treated as an integer and is added to.  If no
// record corresponds, a new record of the additional value is stored. 
//
int    iDB_AddInt(void *aDB, const void *aKey, int aKeySize, int aNumber);
double iDB_AddDouble(void *aDB, const char *aKey, int aKeySize, double aNumber);


 #ifdef __cplusplus
 }
 #endif

#endif
