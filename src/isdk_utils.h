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

#ifndef isdk_utils__h
 #define isdk_utils__h

 #include <stdbool.h>
 #include <stdio.h>
 #include <stdarg.h>
 #ifdef __linux__
  #define __USE_BSD 1
  #include <sys/types.h>
  #include <sys/stat.h>
 #endif
 #include <fts.h>            /* Traverse a file hierarchy. */

 #include "zmalloc.h"
 #include "sysstat.h"    /* mode_t for Windows - <sys/types.h> for POSIX */
 /*use the redis C dynamic strings library: sds.h*/
 #include "sds.h"
 #include "darray.h"
 //#include "adlist.h"  //the double-link list
 #include "filename.h"

 #define O_RW_RW_R__PERMS    (S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IROTH)
 #define O_RWXRWXR_XPERMS    (S_IWUSR|S_IXUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
 #define O_EXCL_CREAT        (O_EXCL|O_CREAT)
 // the FTSListDir Options:
 #define LIST_DESCENDING            0  //the list dir order
 #define LIST_PHYSICAL              1  //list physical files or logical files(following the symbolic files).
 #define LIST_DIR                   2  //list directories in the aDir
 #define LIST_FILE                  3  //list files in the aDir
 #define LIST_SYMBOLIC              4  //list symbolic links in the aDir
 #define LIST_SYMBOLIC_NONE         5  //list symbolic links with a non-existent target in the aDir
 #define LIST_HIDDEN_FILE           6  //list hidden files in the aDir

 #define WALK_ITEM_OK               0
 #define WALK_ITEM_SKIP             1
 #define WALK_ITEM_STOP            -1

 // the IsDirectory result constants:
 #define PATH_IS_FILE       0
 #define PATH_IS_DIR        1
 #define PATH_IS_SYM_DIR    2    //is symlink dir
 #define PATH_IS_SYM_FILE   -999 //is symlink file
 #define PATH_IS_NOT_EXISTS -2

 #ifdef __cplusplus
 extern "C"
 {
 #endif

/* 
 *     dStringArray a = dStringArray_new();
 *     dStringArray_free(a);
 *
 *
 *
 * */

typedef darray(const char*)     dCStrArray;
typedef darray(sds)             dStringArray;
static void _darray_sds_free_handler(void* aPtr)
{
    if (aPtr) {
        sdsfree(aPtr);
        aPtr = NULL;
    }
}
#define dStringArray_init(arr) do {(arr).item=0; (arr).size=0; (arr).alloc=0;(arr).onFree=_darray_sds_free_handler;} while(0)
static inline dStringArray* dStringArray_new()
{
    dStringArray* result = zmalloc(sizeof(dStringArray));
    dStringArray_init(*result);
    return result;
}
static inline void dStringArray_free(dStringArray* arr) {
    darray_free_all(*arr);
    zfree(arr);
}


/**
 * err_set_progname - set the program name - extract from ccan/err
 * @name: the name to use for err, errx, warn and warnx
 *
 * The BSD err.h calls know the program name, unfortunately there's no
 * portable way for the CCAN replacements to do that on other systems.
 *
 * If you don't call this with argv[0], it will be "??".
 *
 * Example:
 *	err_set_progname(argv[0]);
 */
void err_set_progname(const char *name);

/**
 * warn - print a message to stderr based on format and errno. - extract from ccan/err
 * @eval: the exit code
 * @fmt: the printf-style format string
 *
 * The format string is printed to stderr like so:
 *	<executable name>: <format>: <strerror(errno)>\n
 *
 * Example:
 *	char *p = strdup("hello");
 *	if (!p)
 *		warn("Failed to strdup 'hello'");
 */
void warn(const char *fmt, ...);

/**
 * warnx - print a message to stderr based on format. - extract from ccan/err
 * @eval: the exit code
 * @fmt: the printf-style format string
 *
 * The format string is printed to stderr like so:
 *	<executable name>: <format>\n
 *
 * Example:
 *	if (argc != 1)
 *		warnx("I don't expect any arguments (ignoring)");
 */
void warnx(const char *fmt, ...);

typedef struct stat Stat;
typedef int(*FTSWalkDirHandler)(int aCount,const FTSENT *aNode, void *aPtr);
typedef ssize_t(*WalkDirHandler)(size_t aCount, const struct dirent *aItem, void *aUserPtr);

/* Open named file without truncate or create safely */
static int open_or_create_file(const char *file, int flags, mode_t perms);

//Move aSrc file or dir to aDest
//Must be in a same file system.
//A 0 value is returned if the operation succeeds,
int MoveDir(const char* aSrc, const char* aDest);

//remove file or directories recursive.
//retrun 0 means successful.
int DeleteDir(const char* aDir);

//Walk through files or directories in the aDir.
//aOptions: the list dir options set:
//  * LIST_DESCENDING(0Bit): the list dir order
//  * LIST_PHYSICAL(1Bit): list physical files or logical files(following the symbolic files).
//  * LIST_DIR(2Bit): list directories in the aDir
//  * LIST_FILE(3Bit): list files in the aDir
//  * LIST_SYMBOLIC(4Bit): list symbolic links in the aDir
//  * LIST_SYMBOLIC_NONE(5Bit): list symbolic links with a non-existent target in the aDir
//aProcessor: the processor for matched item
// retrun matched count if successful, or, means errno(<0).
int FTSWalkDir(const char* aDir, const char* aPattern, int aOptions, FTSWalkDirHandler aProcessor, void *aPtr);
//Count files or directories in the aDir.
//aOptions: the list dir options set:
//  * LIST_DESCENDING(0Bit): the list dir order
//  * LIST_PHYSICAL(1Bit): list physical files or logical files(following the symbolic files).
//  * LIST_DIR(2Bit): list directories in the aDir
//  * LIST_FILE(3Bit): list files in the aDir
//  * LIST_SYMBOLIC(4Bit): list symbolic links in the aDir
//  * LIST_SYMBOLIC_NONE(5Bit): list symbolic links with a non-existent target in the aDir
//retrun <0 means failed errno.
int FTSCountDir(const char* aDir, const char* aPattern, int aOptions);
//List files or directories in the aDir.
//aOptions: the list dir options set:
//  * LIST_DESCENDING(0Bit): the list dir order
//  * LIST_PHYSICAL(1Bit): list physical files or logical files(following the symbolic files).
//  * LIST_DIR(2Bit): list directories in the aDir
//  * LIST_FILE(3Bit): list files in the aDir
//  * LIST_SYMBOLIC(4Bit): list symbolic links in the aDir
//  * LIST_SYMBOLIC_NONE(5Bit): list symbolic links with a non-existent target in the aDir
//retrun 0 means failed, or return the list of the matched directories(the value is sds type).
dStringArray* FTSListDir(const char* aDir, const char* aPattern, int aOptions);

//aOptions:
//  * LIST_DIR(2Bit): list directories in the aDir
//  * LIST_FILE(3Bit): list files in the aDir
//  * LIST_SYMBOLIC(4Bit): list symbolic links in the aDir (MUST BE WITH LIST_DIR or LIST_FILE)
//  * LIST_SYMBOLIC_NONE(5Bit): list symbolic links with a non-existent target in the aDir (THE SAME ABOVE)
//  * LIST_HIDDEN_FILE(6Bit): list hidden files in the aDir
//aCount: return max count of mached pathes
//  * 0 means all matched pathes
//Return:
//  * -1 if failed, the error code in errno
//  * or the count of matched dirs.
ssize_t WalkDir(const char* aDir, const char* aPattern, int aOptions, size_t aSkipCount, size_t aCount, WalkDirHandler aProcessor, void *aUserPtr);
ssize_t CountDir(const char* aDir, const char* aPattern, int aOptions);
dStringArray* ListDir(const char* aDir, const char* aPattern, int aOptions, size_t aSkipCount, size_t aCount);


//test the filename whether is a directory
//return 0 = a file, 1 = a Dir, 2 = a symbolic dir, -999 = a symbolic file, -2 = Not Exists(ENOENT), < 0 others means error code.
//See Also: DirectoryExists
int IsDirectory(const char* aFileName);

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

//#include <unistd.h>
//int symlink(const char *srcPath, const char *destPath); //make symbolic link(destPath) to a srcPath

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
