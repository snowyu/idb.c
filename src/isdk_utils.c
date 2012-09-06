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

#include "config.h"
//utility functions...
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef __linux__
  #define __USE_BSD 1
  #include <sys/types.h>
  #include <sys/stat.h>
#endif
#include <ctype.h>          /*isxdigit, isupper, tolower...*/
#include <string.h>
#include <stdbool.h>
#include <stdio.h>          /* fdopen(), etc */
#include <stdlib.h>
#include <fts.h>            /* Traverse a file hierarchy. */
#include <fcntl.h>
#include "isdk_utils.h"


//Author: J Leffler
static int do_mkdir(const char* path, mode_t mode)
{
    Stat            st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

/*
  create the directory,  and create intermediate directories as required.
*/
int ForceDirectories(const char* aFolderPath, mode_t aMode)
{

    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = strdup(aFolderPath);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            // Neither root nor double slash in path
            *sp = '\0';
            status = do_mkdir(copypath, aMode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(aFolderPath, aMode);
    free(copypath);
    return (status);
}

//Join path2 to aPath
sds sdsJoinPathLen(const sds aPath, const void *aPath2, const size_t len)
{
    sds result = NULL;
    if (aPath == NULL) {
        result = sdsnewlen(aPath2, len);
    }
    else if (sdslen(aPath) == 0) {
        result = sdscatlen(aPath, aPath2, len);
    }
    else {
        result = sdscat(aPath, PATH_SEP_STR);
        sdscatlen(result, aPath2, len);
    }
    return result;
}


/* Open named file without truncate or create safely */
static int open_or_create_file(const char *file, int flags, mode_t perms)
{
    int fd = open(file, flags & ~O_EXCL_CREAT, perms);
    if (fd < 0)
        fd = open(file, flags | O_EXCL_CREAT, perms);
    return(fd);
}
//A 0 value is returned if the operation succeeds,
int MoveDir(const char* aSrc, const char* aDest)
{
    int result = rename(aSrc, aDest);
    if (result) result = errno;
    return result;
}

//remove file or directories recursive.
//retrun 0 means successful.
int DeleteDir(const char* aDir)
{
    const char* paths[] = {aDir, 0};
    int result = 0;
    FTS *tree = fts_open((char**)&paths, FTS_NOCHDIR|FTS_NOSTAT|FTS_PHYSICAL, 0);
    if (tree) {
        FTSENT *node;
        while ((node = fts_read(tree)) && !result) {
            switch (node->fts_info) {
            case FTS_DP: //A directory being visited in post-order.
                result=rmdir(node->fts_path);
                /*printf("%d remove dir named %s at depth %d, "
                    "accessible via %s from the current directory "
                    "or via %s from the original starting directory\n",
                    result,
                    node->fts_name, node->fts_level,
                    node->fts_accpath, node->fts_path); // */
                break;
            case FTS_F: //A regular file.
            case FTS_SL: //A symbolic link.
            case FTS_SLNONE: //A symbolic link with a non-existent target. 
            case FTS_NSOK: // A file for which no stat(2) information was requested. 
                result=unlink(node->fts_path);
                /*printf("%d remove file named %s at depth %d, "
                    "accessible via %s from the current directory "
                    "or via %s from the original starting directory\n",
                    result,
                    node->fts_name, node->fts_level,
                    node->fts_accpath, node->fts_path); // */
                break;
            case FTS_DC: //A directory that causes a cycle in the tree.
                fts_set(tree, node, FTS_SKIP);
                result = EDEADLK;
                break;
            case FTS_DNR: //A directory which cannot be read.  This is an error 
            case FTS_NS:  //A file for which no stat(2) information was available. 
            case FTS_ERR:
                printf("error file name: %s\n", node->fts_path);
                result = node->fts_errno;
                break;
            default:
                break;
            }
        }
        if (errno) result = errno;
        fts_close(tree);
    }

    return result;
}


static int is_printable(char c) {
    return ( 32 <= c && c <= 126 && c != '%');
/*
 return (48 <= c && c <= 57)  ||//0-9(48-57)
        (58 <= c && c <= 64)  ||//:;<=>?@(58-64)
        (65 <= c && c <= 90)  ||//ABC...xyz
        (91 <= c && c <= 96)  ||//[\]^_`
        (97 <= c && c <= 122) ||//abc...xyz
        (123<= c && c <= 126) ||//{|}~
        //c == '~' || c == '!' || c =='*' ||
        c == '(' || c == ')' || c == '\'' || c== '|';
*/
}

static inline int is_safe_char(char c, const char *aUnSafeChars) {
    if (aUnSafeChars && strchr(aUnSafeChars, c)){
        return 0;
    } else
        return is_printable(c);
}

//return a new null-string if successful.
static char *UrlEncode(char *s, const char *aUnSafeChars) {
  char *ret, *c, *ct;
  int i, len;
  //printf("UrlEncode: Encoding '%s'\n", s);
  /* First pass - figure out how long the target string should be */
  len = 0;
  for(c=s; *c; c++) {
    if(is_safe_char(*c, aUnSafeChars)) len++; else len += 3;
  }
  /* Don't forget we need to store terminating zero too */
  len++;
  //printf("Current len: %d, target len: %d\n", strlen(s)+1, len);
  ct = ret = malloc(len);
  /* Second pass - copy/encode */
  if(ct) {
      for(c=s; *c; c++) {
        if(is_safe_char(*c, aUnSafeChars)) {
          *ct++ = *c;
        } else {
          snprintf(ct, 4, "%%%02X", *c);
          ct += 3;
        }
      }
      *ct = 0; /* null-terminate the string */
  }
  //printf("Encoded string: %s\n", ret);
  return ret;
}

/* convert(decode) the str directly.
 * return the new length for decoded string, It is always smaller than the orginal length.
 */
static int UrlDecode(char *vStr, int len)
{
    char *dest = vStr;
    char *data = vStr;

    unsigned int value;
    unsigned int c;

    while (len--) {
        /*
        if (*data == '+') {
        *dest = ' ';
        }
        else */
        if (*data == '%')
        {
            if (len >= 2 && isxdigit((int) *(data + 1))
                 && isxdigit((int) *(data + 2)))
            {

                c = ((unsigned char *)(data+1))[0];
                if (isupper(c))
                    c = tolower(c);
                value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
                c = ((unsigned char *)(data+1))[1];
                if (isupper(c))
                    c = tolower(c);
                value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

                *dest = (char)value ;
                data += 2;
                len -= 2;
            }
            else if (len>=5 && (*(data+1)=='u' || *(data+1)=='U') &&
                    isxdigit((int) *(data + 2)) && isxdigit((int) *(data + 3)) &&
                    isxdigit((int) *(data + 4)) && isxdigit((int) *(data + 5)))
            {
                c = ((unsigned char *)(data+2))[0];
                if (isupper(c))
                    c = tolower(c);
                value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
                c = ((unsigned char *)(data+2))[1];
                if (isupper(c))
                    c = tolower(c);
                value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

                *dest = (char)value ;
                data += 2;
                len -= 2;
                c = ((unsigned char *)(data+4))[0];
                if (isupper(c))
                    c = tolower(c);
                value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
                c = ((unsigned char *)(data+4))[1];
                if (isupper(c))
                    c = tolower(c);
                value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

                *dest = (char)value ;
                data += 2;
                len -= 2;
            }
        } else {
            *dest = *data;
        }
        data++;
        dest++;
    }
    
    *dest = (char) 0;
    return dest - vStr;
}

#ifdef ISDK_UTILS_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"

//Note: sds.* zmalloc.* config.h come from redis src
//gcc --std=c99 -I. -Ideps  -o test -DHAVE_UNISTD_H -DISDK_UTILS_TEST_MAIN isdk_utils.c deps/sds.c deps/zmalloc.c
int main(void) {
    {
        ForceDirectories("testdir/good/better/best?", O_RWXRWXR_XPERMS);
        test_cond("DirectoryExists('testdir/good/better/best?')", DirectoryExists("testdir/good/better/best?") == 1);
        test_cond("MoveDir(\"testdir/good/better\", \"testdir/good/ok\")", MoveDir("testdir/good/better", "testdir/good/ok")==0);
        test_cond("DirectoryExists('testdir/good/ok')", DirectoryExists("testdir/good/ok") == 1);
        test_cond("DeleteDir('testdir')", DeleteDir("testdir") == 0);
        test_cond("DirectoryExists('testdir')", DirectoryExists("testdir") == 0);
        int fd = open_or_create_file("mytestfile", 0, O_RW_RW_R__PERMS);
        test_cond("open_or_create_file(mytestfile):open not exists file", fd != 0);
        close(fd);
        fd = open_or_create_file("mytestfile", 0, O_RW_RW_R__PERMS);
        test_cond("open_or_create_file(mytestfile):open exists file", fd != 0);
        close(fd);
        test_cond("DeleteDir('mytestfile')", DeleteDir("mytestfile") == 0);
        test_cond("DirectoryExists('mytestfile') is false", DirectoryExists("mytestfile") == 0);
        char* s="testing encoding 'it' \"with\" path/haha/it and %& *?|<>.";
        char* r=UrlEncode(s, NULL);
        test_cond("UrlEncode('testing encoding 'it' \"with\" path/haha/it and %& *?|<>.', NULL)", strcmp(r, "testing encoding 'it' \"with\" path/haha/it and %25& *?|<>.")==0);
        char* r1=UrlEncode(s, " &*?|<>.");
        test_cond("UrlEncode('testing encoding 'it' \"with\" path/haha/it and %& *?|<>.',  \" &*?|<>.\")", strcmp(r1, "testing%20encoding%20'it'%20\"with\"%20path/haha/it%20and%20%25%26%20%2A%3F%7C%3C%3E%2E")==0);
        int i=UrlDecode(r, strlen(r));
        test_cond("UrlDecode('testing encoding 'it' \"with\" path/haha/it and *%3F%7C%3C%3E.').Length", i==strlen(s));
        test_cond("UrlDecode('testing encoding 'it' \"with\" path/haha/it and *%3F%7C%3C%3E.')", strcmp(s, r)==0);
        //printf("%s\n",r);
        if(r) free(r);
    }
    test_report();
    return 0;
}
#endif
