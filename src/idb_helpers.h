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

 #ifdef __cplusplus
 extern "C"
  {
 #endif

 //Low-Level functions
 bool IsDirValueExists(const sds aDir, const sds aAttribute);
 sds GetDirValue(const sds aDir, const sds aAttribute);
 bool SetDirValue(const sds aDir, const sds aValue, const sds aAttribute);

 #ifdef __cplusplus
 }
 #endif

#endif
