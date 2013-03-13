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
 #include "isdk_config.h"
 #include <stdbool.h>
 #include <stdio.h>
 #include <stdarg.h>
//#include <fts.h>            /* Traverse a file hierarchy. */

 #include "deps/zmalloc.h"
 /*use the redis C dynamic strings library: sds.h*/
 #include "deps/sds.h"
 #include "deps/darray.h"
 //#include "deps/adlist.h"  //the double-link list
 #include "deps/filename.h"
 #include "deps/utf8proc.h"

 //#define HAVE_STRUCT_DIRENT_D_TYPE
 #ifdef HAVE_STRUCT_DIRENT_D_TYPE
 /* True if the type of the directory entry D is known.  */
 # define DT_IS_KNOWN(d) ((d)->d_type != DT_UNKNOWN)
 /* True if the type of the directory entry D must be T.  */
 # define DT_MUST_BE(d, t) ((d)->d_type == (t))
 # define D_TYPE(d) ((d)->d_type)
 #else
 # define DT_IS_KNOWN(d) false
 # define DT_MUST_BE(d, t) false
 # define D_TYPE(d) DT_UNKNOWN

 # undef DT_UNKNOWN
 # define DT_UNKNOWN 0

 /* Any nonzero values will do here, so long as they're distinct.
    Undef any existing macros out of the way.
 # undef IFTODT
 # undef DTTOIF
 # define IFTODT(mode)    (((mode) & 0170000) >> 12)
 # define DTTOIF(dirtype) ((dirtype) << 12)
 # undef DT_BLK
 # undef DT_CHR
 # undef DT_DIR
 # undef DT_FIFO
 # undef DT_LNK
 # undef DT_REG
 # undef DT_SOCK
 # define DT_BLK IFTODT(S_IFBLK)
 # define DT_CHR IFTODT(S_IFCHR)
 # define DT_DIR IFTODT(S_IFDIR)
 # define DT_FIFO IFTODT(S_IFIFO)
 # define DT_LNK IFTODT(S_IFLNK)
 # define DT_REG IFTODT(S_IFREG)
 # define DT_SOCK IFTODT(S_IFSOCK)
 */
 #endif

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
 #define LIST_HIDDEN_FILE           6  //list hidden files(the file name start with ".") in the aDir.
 #define LIST_NORMAL_FILE           7  //list noraml files(the file name do not start with ".") in the aDir.
 #define LIST_DIRS                  (1 << LIST_DIR | 1 << LIST_NORMAL_FILE)
 #define LIST_FILES                 (1 << LIST_FILE | 1 << LIST_NORMAL_FILE)
 #define LIST_NORMAL_FILES          (LIST_FILES | 1 << LIST_SYMBOLIC)
 #define LIST_HIDDEN_FILES          (1 << LIST_FILE | 1 << LIST_HIDDEN_FILE | ! << LIST_SYMBOLIC)
 #define LIST_NORMAL_DIRS           (LIST_DIRS | 1 << LIST_SYMBOLIC)
 #define LIST_HIDDEN_DIRS           ( 1 << LIST_DIR | 1 << LIST_SYMBOLIC | 1 << LIST_HIDDEN_FILE)


 #define WALK_ITEM_OK               0
 #define WALK_ITEM_SKIP             1
 #define WALK_ITEM_STOP            -1
 #define WALK_ITEM_ERROR           -2  //u should put the error code into errno first.

 // the IsDirectory result constants:
 #define PATH_IS_FILE       -1
 #define PATH_IS_DIR        1
 #define PATH_IS_SYM_DIR    2    //is symlink dir
 #define PATH_IS_SYM_FILE   -999 //is symlink file
 #define PATH_IS_NOT_EXISTS 0

 #define FNM_MATCHED(aPattern, aDir) (!aPattern || fnmatch(aPattern, aDir, FNM_PERIOD) == 0)

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
static void _darray_sds_free_handler(void* aPtr) {
    if (aPtr) {
        sdsfree(aPtr);
        aPtr = NULL;
    }
}
#define dStringArray_init(arr) do {(arr).item=0; (arr).size=0; (arr).alloc=0;(arr).onFree=_darray_sds_free_handler;} while(0)
static inline dStringArray* dStringArray_new() {
    dStringArray* result = zmalloc(sizeof(dStringArray));
    dStringArray_init(*result);
    return result;
}
static inline void dStringArray_free(dStringArray* arr) {
    darray_free_all(*arr);
    zfree(arr);
}
static inline ssize_t dStringArray_indexOf(const dStringArray* arr, sds str) {
    ssize_t result = -1;
    int i;
    for (i = 0; i < (*arr).size; i++) {
        if ((*arr).item[i]) {
            if (strcmp((*arr).item[i], str) == 0) {
                result = i;
                break;
            }
        }
    }
    return result;
}
static inline ssize_t dCStrArray_indexOf(const dCStrArray* arr, const char* str) {
    ssize_t result = -1;
    int i;
    for (i = 0; i < (*arr).size; i++) {
        if ((*arr).item[i]) {
            if (strcmp((*arr).item[i], str) == 0) {
                result = i;
                break;
            }
        }
    }
    return result;
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
typedef struct dirent Dirent;
typedef int(*FTSWalkDirHandler)(int aCount,const void *aNode, void *aPtr);
typedef ssize_t(*WalkDirHandler)(size_t aCount, const char *aDir, const Dirent *aItem, void *aUserPtr);

/* Open named file without truncate or create safely */
static int open_or_create_file(const char *file, int flags, mode_t perms);

//Move aSrc file or dir to aDest
//Must be in a same file system.
//A 0 value is returned if the operation succeeds,
int MoveDir(const char* aSrc, const char* aDest);

//remove file or directories recursive.
//retrun 0 means successful.
//TODO: DONT USE FTS to do so.
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
//whether is aPattern mached file in the aDir exists?
bool IsFileExistsInDir(const char* aDir, const char* aPattern, int aOptions);


//test the filename whether is a directory
//return PATH_IS_FILE = a file, PATH_IS_DIR = a Dir, PATH_IS_SYM_DIR = a symbolic dir, PATH_IS_SYM_FILE = a symbolic file, PATH_IS_NOT_EXISTS = Not Exists, < 0 others means error code.
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
//return the utf8 char size if successful, or return < 0 means error code(see utf8proc)
//sizeof(*aResult) MUST be greater than 6.
static inline ssize_t IterateUtf8Char(const char* aUtf8Str, ssize_t aStrLen, char* aResult)
{
    int32_t vInt32Char;
    ssize_t vLen = utf8proc_iterate((const uint8_t *)aUtf8Str, aStrLen, &vInt32Char);
    if (vLen > 0) {
        vLen = utf8proc_encode_char(vInt32Char, (uint8_t *)aResult);
    }
    return vLen;
}

static inline sds ExtractLastPathName(const sds aDir)
{
    ssize_t vLen = sdslen(aDir);
    while (aDir[vLen-1] == PATH_SEP) {
        vLen--;
        sdsSetlen_(aDir, vLen);
    }
    char* s = strrchr(aDir, PATH_SEP);
    if (s != NULL) {
        s++;
    } else {
        s = aDir;
    }
    vLen = strlen(s);
    sds vLastPath = NULL;
    if (vLen > 0) {
        vLastPath = sdsnewlen(s, vLen);
        //remove the last part path name from aDir.
        sdsIncrLen(aDir, -vLen);
    }
    return vLastPath;
}

 #ifdef __cplusplus
 }
 #endif

#endif
