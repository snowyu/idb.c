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

#ifndef isdk_xattr__h
 #define isdk_xattr__h

/*use the redis C dynamic strings library: sds.h*/
#include "sds.h"

 #ifdef __cplusplus
 extern "C"
 {
 #endif


//the xattr low level functions:
static ssize_t xattr_getxattr(const char *path, const char *name,
                              void *value, ssize_t size, u_int32_t position, 
                              int options);
static ssize_t xattr_setxattr(const char *path, const char *name,
                              void *value, ssize_t size, u_int32_t position,
                              int options);
static ssize_t xattr_removexattr(const char *path, const char *name,
                                 int options);
static ssize_t xattr_listxattr(const char *path, char *namebuf,
                               size_t size, int options);
static ssize_t xattr_fgetxattr(int fd, const char *name, void *value,
                               ssize_t size, u_int32_t position, int options);
static ssize_t xattr_fsetxattr(int fd, const char *name, void *value,
                               ssize_t size, u_int32_t position, int options);
static ssize_t xattr_fremovexattr(int fd, const char *name, int options);
static ssize_t xattr_flistxattr(int fd, char *namebuf, size_t size, int options);


ssize_t ListXattr(const char *path, char *namebuf, size_t size);
int IsXattrExists(const char* aFile, const char* aKey);
sds GetXattr(const char* aFile, const char* aKey);
//0: ok; others: errcode.
ssize_t SetXattr(const char* aFile, const char* aKey, sds aValue);


 #ifdef __cplusplus
 }
 #endif

#endif
