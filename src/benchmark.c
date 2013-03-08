//The math library must be linked in when building the executable. How to do this varies by environment, but in Linux/Unix, just add -lm to the command:
/*
The functions in stdlib.h and stdio.h have implementations in libc.so (or .a static linking), which is linked into your executable by default (as if -lc were specified). GCC can be instructed to avoid this automatic link with the -nostdlib or -nodefaultlibs options.

The math functions in math.h have implementations in libm.so (or .a for static linking), and libm is not linked in by default. There are hysterical raisins historical reasons for this libm/libc split, none of them very convincing.

Interestingly, the C++ runtime libstdc++ requres libm, so if you compile a C++ program with GCC (g++), you will automatically get libm linked in.
*/
//gcc --std=gnu99 -I. -Ideps -o benchmark benchmark.c idb_helpers.c isdk_xattr.c isdk_utils.c isdk_math.c isdk_string.c deps/sds.c deps/zmalloc.c deps/utf8proc.c -lm


#include "deps/config.h"
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>


//#include "tcutil.h"
//#include "deps/zmalloc.h"
#include "isdk_math.h"
#include "isdk_string.h"
#include "idb_helpers.h"

#define RECBUFSIZ      48                // buffer for records


/* global variables */
const char *g_progname;                  // program name
unsigned int g_randseed;                 // random seed
int g_dbgfd;                             // debugging output
int g_store_type = STORE_IN_FILE;

const TIDBBase GIDBConfig = {
    "testdir",
    1024,
    STORE_IN_FILE,
    dkIgnored,
    dkIgnored
};



/* Duplicate a region on memory. */
void *tcmemdup(const void *ptr, size_t size){
  assert(ptr && size >= 0);
  char *p = zmalloc(size + 1);
  memcpy(p, ptr, size);
  p[size] = '\0';
  return p;
}

darray_int gRandoms = darray_new();

/* get a random number */
static inline int _myrand(int range) {
  if(range < 2) return 0;
  int high, low;
  high = (unsigned int)rand() >> 4;
  low = range * (rand() / (RAND_MAX + 1.0));
  low &= (unsigned int)INT_MAX >> 4;
  return (high + low) % range;
}
static inline bool isInIntArray(darray_int arr, int i) {
  bool vIsExists = false;
  int *item;
  darray_foreach(item, arr) {
      if (*item == i) {
          vIsExists = true;
          break;
      }
  }
  return vIsExists;
}
static int myrand(int range){
  if(range < 2) return 0;
  int result;
  do {
    result = _myrand(range);
  } while (isInIntArray(gRandoms, result));
  darray_push(gRandoms, result);
  return result;
}


/* function prototypes */
int main(int argc, char **argv);
static void usage(void);
static void iprintf(const char *format, ...);
static void iputchar(int c);
static void sysprint(void);
static int myrand(int range);
static void *pdprocfunc(const void *vbuf, int vsiz, int *sp, void *op);
static bool iterfunc(const void *kbuf, int ksiz, const void *vbuf, int vsiz, void *op);
static int runwrite(int argc, char **argv);
static int runread(int argc, char **argv);
static int runremove(int argc, char **argv);
static int procwrite(const char *path, int rnum, bool rnd);
static int procread(const char *path, int rnum, bool rnd);
static int procremove(const char *path, bool mt, int rcnum, int lcnum, int ncnum,
                      int xmsiz, int dfunit, int omode, bool rnd);


/* main routine */
int main(int argc, char **argv){
  g_progname = argv[0];
  err_set_progname(g_progname);

  const char *ebuf = getenv("RNDSEED");
  g_randseed = ebuf ? strToInt64x(ebuf) : 7788;//now() * 1000;
  srand(g_randseed);
  ebuf = getenv("IDBGFD");
  g_dbgfd = ebuf ? strToInt64x(ebuf) : UINT16_MAX;
  ebuf = getenv("IDBPAGESIZE");
  IDBMaxPageCount = ebuf ? strToInt64x(ebuf) : IDBMaxPageCount;
  if(argc < 2) usage();
  int rv = 0;
  if(!strcmp(argv[1], "write")){
    rv = runwrite(argc, argv);
  } else if(!strcmp(argv[1], "read")){
    rv = runread(argc, argv);
  } else {
    usage();
  }
  if(rv != 0){
    printf("FAILED: RNDSEED=%u PID=%d", g_randseed, (int)getpid());
    for(int i = 0; i < argc; i++){
      printf(" %s", argv[i]);
    }
    printf("\n\n");
  }
  darray_free(gRandoms);
  return rv;
}


/* print the usage and exit */
static void usage(void){
  fprintf(stderr, "%s: Benchmark of the iDB database API\n", g_progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "U can set the ENV Variable RNDSEED and IDBPAGESIZE\n");
  fprintf(stderr, "  %s write [-rnd]"
          " path recordCount\n", g_progname);
  fprintf(stderr, "  %s read "
          " path recordCount\n", g_progname);
  fprintf(stderr, "\n");
  exit(1);
}


/* print formatted information string and flush the buffer */
static void iprintf(const char *format, ...){
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  fflush(stdout);
  va_end(ap);
}


/* print a character and flush the buffer */
static void iputchar(int c){
  putchar(c);
  fflush(stdout);
}


/* parse arguments of write command */
static int runwrite(int argc, char **argv){
  char *path = NULL;
  char *rstr = NULL;
  char *bstr = NULL;
  char *astr = NULL;
  char *fstr = NULL;
  bool mt = false;
  int opts = 0;
  int rcnum = 0;
  int lcnum = 0;
  int ncnum = 0;
  int xmsiz = -1;
  int dfunit = 0;
  int iflags = 0;
  int omode = 0;
  bool rnd = false;
  for(int i = 2; i < argc; i++){
    if(!path && argv[i][0] == '-'){
      if(!strcmp(argv[i], "-mt")){
        mt = true;
      } else if(!strcmp(argv[i], "-rnd")){
        rnd = true;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else if(!rstr){
      rstr = argv[i];
    } else {
      usage();
    }
  }
  if(!path || !rstr) usage();
  int rnum = strToInt64x(rstr);
  if(rnum < 1) usage();
  int rv = procwrite(path, rnum, rnd);
  return rv;
}


/* parse arguments of read command */
static int runread(int argc, char **argv){
  char *path = NULL;
  char *rstr = NULL;
  bool mt = false;
  int rcnum = 0;
  int lcnum = 0;
  int ncnum = 0;
  int xmsiz = -1;
  int dfunit = 0;
  int omode = 0;
  bool rnd = false;
  for(int i = 2; i < argc; i++){
    if(!path && argv[i][0] == '-'){
      if(!strcmp(argv[i], "-mt")){
        mt = true;
      } else if(!strcmp(argv[i], "-rc")){
        if(++i >= argc) usage();
        rcnum = strToInt64x(argv[i]);
      } else if(!strcmp(argv[i], "-lc")){
        if(++i >= argc) usage();
        lcnum = strToInt64x(argv[i]);
      } else if(!strcmp(argv[i], "-nc")){
        if(++i >= argc) usage();
        ncnum = strToInt64x(argv[i]);
      } else if(!strcmp(argv[i], "-xm")){
        if(++i >= argc) usage();
        xmsiz = strToInt64x(argv[i]);
      } else if(!strcmp(argv[i], "-df")){
        if(++i >= argc) usage();
        dfunit = strToInt64x(argv[i]);
      } else if(!strcmp(argv[i], "-rnd")){
        rnd = true;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else if(!rstr){
      rstr = argv[i];
    } else {
      usage();
    }
  }
  if(!path) usage();
  int rnum = strToInt64x(rstr);
  int rv = procread(path, rnum, rnd);
  return rv;
}


/* perform write command */
static int procwrite(const char *path, int rnum, bool rnd){
  iprintf("<Writing Test>\n  seed=%u  path=%s  count=%d\n\n",
          g_randseed, path, rnum);
  bool err = false;
  double stime = now();
  TIDBBase vDB;
  //vDB.path = path;
  sds vKey = sdsnewlen(NULL, 8), vValue = sdsnewlen(NULL, 8);
  sds vPath = sdsnew(path);
  for(int i = 1; i <= rnum; i++){
      int id = (rnd) ? myrand(rnum) + 1 : i;
      //sdsclear(vKey);
      //sdsclear(vValue);
      vKey = sdsprintf(vKey, "%d", id);
      //if (rnd && iKeyExists(vPath, vKey, sdslen(vKey))) continue;
      vValue = sdsprintf(vValue, "store this %d", id *100);
      int vResult = iPut(vPath, vKey, sdslen(vKey), vValue, NULL, g_store_type);
      if (vResult != 0) {
          warnx("error: iPut:%d",vResult);
        err = true;
        break;
      }
    if(rnum > 250 && i % (rnum / 250) == 0){
      iputchar('.');
      if(i == rnum || i % (rnum / 10) == 0) iprintf(" (%08d)\n", i);
    }
  }
  double vRunTime = now() - stime;
  iprintf("record number: %llu\n", (unsigned long long)(rnum));

  //iprintf("record number: %llu\n", (unsigned long long)TIDBBasernum(tdb));
  //iprintf("size: %llu\n", (unsigned long long)TIDBBasefsiz(tdb));
  //sysprint();
  //if (DeleteDir(path) != 0) {
  //    eprint(&vDB, __LINE__, "DeleteDir");
  //    err = true;
  //}

  sdsfree(vKey);
  sdsfree(vValue);
  sdsfree(vPath);
  iprintf("time: %.3f\n", vRunTime);
  iprintf("%s\n\n", err ? "error" : "ok");
  return err ? 1 : 0;
}


/* perform read command */
static int procread(const char *path, int rnum, bool rnd){
  iprintf("<Reading Test>\n  seed=%u  path=%s  rnum=%d\n\n",
          g_randseed, path, rnum);
  bool err = false;
  double stime = now();
  sds vKey = sdsnewlen(NULL, 8), vValue = sdsnewlen(NULL, 8);
  sds vPath = sdsnew(path);
  for(int i = 1; i <= rnum; i++){
    int id = (rnd) ? myrand(rnum) + 1 : i;
    vKey = sdsprintf(vKey, "%d", id);
    //vValue = sdsprintf(vValue, "%.8f", (myrand(i * 100) + 1) / 100.0);
    vValue = sdsprintf(vValue, "store this %d", id*100);
    sds vResult = iGet(vPath, vKey, sdslen(vKey), NULL, g_store_type);
    if(!vResult ){
      warnx("iGet(%s)=%s, expectd=%s, error:%d", vKey, vResult, vValue, errno);
      err = true;
      break;
    }
    else if (strcmp(vResult, vValue) != 0) {
        warnx("the %s's value %s is not expectd this: %s", vKey, vResult, vValue);
    }
    if(rnum > 250 && i % (rnum / 250) == 0){
      iputchar('.');
      if(i == rnum || i % (rnum / 10) == 0) iprintf(" (%08d)\n", i);
    }
  }
  double vRunTime = now() - stime;
  iprintf("record number: %lu\n", iSubkeyCount(vPath, NULL, 0, NULL));
//  iprintf("size: %llu\n", (unsigned long long)TIDBBasefsiz(tdb));
//  sysprint();
  iprintf("time: %.3f\n", vRunTime);
  iprintf("%s\n\n", err ? "error" : "ok");
  sdsfree(vKey);
  sdsfree(vValue);
  sdsfree(vPath);
  return err ? 1 : 0;
}


// END OF FILE
