/*
  Copyright (c) 2012 Riceball LEE(snowyu.lee@gmail.com)

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
 #include "idb_helpers.h"


 #ifdef __cplusplus
 extern "C"
  {
 #endif

struct _iDB {
  char *path;                   //the iDB database path.
  bool opened;                  //whether the internal database is opened
  bool loadOnDemand;
  bool storeInXattr;            //enabled the xattr storage.
  bool storeInFile;             //enabled the file storage.
  bool raiseOnTypeMismatch;     //raise error if Type Mismatch when true
};

typedef struct _iDB iDB;


//create a database object.
void* iDB_New(void);
//free a iDB Database object.
void  iDB_Free(void *aDB);
/*
  Open the Specified Database
  *name: the database folder name

  If successful, the return value is true, else, it is false.
*/
bool  iDB_Open(void *aDB, const sds aDBPath);
bool  iDB_Close(void *aDB);

//low-level operations:
//Get the type of a key
void* iDB_GetType(void *aDB, const void *aKey, int aKeySize, int *aTypeSize);
/* Get the size of the type of a record.
 If successful, the return value is the size of the value of the corresponding record, else,
 it is -1. */
int   iDB_GetTypeSize(void *aDB, const void *aKey, int aKeySize);

/* Get the value of the key.
 */
void* iDB_GetValue(void *aDB, const void *aKey, int aKeySize, int *aValueSize);
/* Get the size of the value of a record.
 If successful, the return value is the size of the value of the corresponding record, else,
 it is -1. */
int   iDB_GetValueSize(void *aDB, const void *aKey, int aKeySize);
bool  iDB_Delete(void *aDB, const void *aKey, int aKeySize);
//Insert or update a record(key/calue) 
bool  iDB_Put(void *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const void *aType, int aTypeSize);
bool  iDB_Update(iDB *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const void *aType, int aTypeSize);
bool  iDB_Insert(iDB *aDB, const void *aKey, int aKeySize, const void *aValue, int aValueSize, const void *aType, int aTypeSize);
//----------------------------
/* Add an integer to a record in a database object.
 `aDB' specifies the abstract database object.
 `aKey' specifies the pointer to the region of the key.
 `aKeySize' specifies the size of the region of the key.
 `aNumber' specifies the additional value.
 If successful, the return value is the summation value, else, it is `INT_MIN'.
 If the corresponding record exists, the value is treated as an integer and is added to.  If no
 record corresponds, a new record of the additional value is stored. */
int    iDB_AddInt(void *aDB, const void *aKey, int aKeySize, int aNumber);
double iDB_AddDouble(void *aDB, const char *aKey, int aKeySize, double aNumber);

 #ifdef __cplusplus
 }
 #endif

#endif
