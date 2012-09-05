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

#ifndef isdk_utils__h
 #define isdk_utils__h

 #include <stdbool.h>
 #include <stdio.h>
 #include <stdarg.h>
 #include "sysstat.h"    /* mode_t for Windows - <sys/types.h> for POSIX */
 /*use the redis C dynamic strings library: sds.h*/
 #include "sds.h"
 #include "filename.h"

 #define O_RW_RW_R__PERMS    (S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IROTH)
 #define O_RWXRWXR_XPERMS    (S_IWUSR|S_IXUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
 #define O_EXCL_CREAT        (O_EXCL|O_CREAT)

 #ifdef __cplusplus
 extern "C"
 {
 #endif

typedef struct stat Stat;

/* Open named file without truncate or create safely */
static int open_or_create_file(const char *file, int flags, mode_t perms);

//Move aSrc file or dir to aDest
//Must be in a same file system.
//A 0 value is returned if the operation succeeds,
int MoveDir(const char* aSrc, const char* aDest);

//remove file or directories recursive.
//retrun 0 means successful.
int DeleteDir(const char* aDir);

/*
  return
     -1: file exits.
      1: dir exits.
      0: no dir exits.
*/
static inline int DirectoryExists(const char *aFolderPath){
    Stat vFileinfo;

    if (stat(aFolderPath, &vFileinfo) == -1){
        return 0;
    }
    else if (S_ISDIR(vFileinfo.st_mode)){
        return 1;
    }
    else{
        return -1; //the file exits.
    }
}

bool IsDirValueExists(const sds aDir, const sds aAttribute);
int ForceDirectories(const char* aFolderPath, mode_t aMode);

//Join aPath and aPath2
//result = aPath + '\' + aPath2
//@len: is the length of aPath2 
sds sdsJoinPathLen(const sds aPath, const void *aPath2, const size_t len);
//only esacpe the '%' char and control-chars if aUnSafeChars is NULL 
static char *UrlEncode(char *s, const char *aUnSafeChars);
static int UrlDecode(char *vStr, int len);

 #ifdef __cplusplus
 }
 #endif

#endif
