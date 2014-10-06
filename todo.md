内存泄露检查： 
  valgrind --log-file=mm.txt --track-origins=yes  --dsymutil=yes --leak-check=full --tool=memcheck ./test

* !+ Cache Mechanism
  * implement the meta attributes for speedup.
    * .count: the normal subkeys count of the current dir only.
  * !+ Cache the key/value in the memory
* !+ Mongrel2 Restful API


# 2014-10-06

+ Build nodejs binding:
    cd nodejs
    npm install
    node-gyp rebuild [-d] #-d for building debug
    npm test

* these functions and constants would be binding to nodejs:
  * Put/Get a Key/attribute Sync/Async
  * IsExists a Key/attribute Sync/Async
  * Delete a Key/attribute Sync/Async
  * incrBy Sync/Async
  * createKeyAlas Sync
  * getAttrCountInFile Sync
  * getAttrsInFile Sync
  * getSubkeyCount Sync
  * getSubkeys Sync
  * string errorStr(errno)
  * bool setMaxPageSize(aMaxPageSize)
  * And some iDB constants:
    * IDB_KEY_TYPE_NAME
    * IDB_VALUE_NAME
    * dkStopped
    * dkIgnored
    * IDB_OK
    * IDB_ERR_PART_FULL
    * IDB_ERR_PART_DUP_KEY
    * IDB_ERR_INVALID_UTF8
    * IDB_ERR_DUP_FILE_NAME
    * IDB_ERR_KEY_NOT_EXISTS


# 2013-07-22

## IndexFileDB Disk Storage Internal Mechanism

The data of the IndexFileDB is stored in a specified directory. It has a configuration file
and many index files in it. There are many index blocks in a index file.


# 2013-03-10

* reconstruct idb_helpers funcs
  * keep IDBMaxPageCount as global options, others put into function.
  * the the IDBDuplicationKeyProcess is still global option.
  * seperate the Xattr and File storage
    * iAttribute:
      * iPutInFile/iPutInXattr(const sds aKeyPath, const sds aAttribute, const TIDBProcesses aPartitionFullProcess)
      * iGetInFile/iGetInXattr(const sds aKeyPath, const sds aAttribute)
      * iDeleteInFile/iDeleteInXattr(sds aKeyPath, aAttribute)
      * iIsExistsInFile/iIsExistsInXattr(sds aKeyPath, aAttribute)
      * iPut/iGet/iDelete/iIsExists(..., aStorageType) -- deprecated
    * iKey: (const sds aKeyPath, const char* aKey, const int aKeyLen)
      * iKeyAlias(..., const char* aAliasPath, const int aAliasPathLen)
      * iKeyDelete(...)
      * iKeyIsExists()
    * iSubkey
      * iSubkeyWalk
      * iSubkeys: List iSubkeys with pattern.
      * iSubkeyCount: current normal keys count(no partition).
      * iSubkeyTotal: all keys count.


## benchmark


### On Ramdisk

    ➜  src git:(master) ✗ ./benchmark write /dev/shm/mydb 100000<Writing Test>
      seed=7788  path=/dev/shm/mydb  count=100000

    record number: 100000
    time: 5.319
    ok

    ➜  src git:(master) ✗ ./benchmark write /dev/shm/mydb 100000
    <Writing Test>
      seed=7788  path=/dev/shm/mydb  count=100000

    record number: 100000
    time: 3.203
    ok

    ➜  src git:(master) ✗ ./benchmark read /dev/shm/mydb 100000
    <Reading Test>
      seed=7788  path=/dev/shm/mydb  rnum=100000

    record number: 100000
    time: 1.702
    ok

    ➜  src git:(master) ✗ ./benchmark read /dev/shm/mydb 100000
    <Reading Test>
      seed=7788  path=/dev/shm/mydb  rnum=100000

    record number: 100000
    time: 1.589
    ok

### On ZFS

➜  src git:(master) ✗ IDBPAGESIZE=100 ./benchmark write /share/test.idb/mydb 100000
<Writing Test>
  pagesize=100 seed=7788  path=/share/test.idb/mydb  count=100000

......................... (00010000)
......................... (00020000)
......................... (00030000)
......................... (00040000)
......................... (00050000)
......................... (00060000)
......................... (00070000)
......................... (00080000)
......................... (00090000)
......................... (00100000)
record number: 100000
time: 266.962
ok

➜  src git:(master) ✗ IDBPAGESIZE=100 ./benchmark write /share/test.idb/mydb 100000
<Writing Test>
  pagesize=100 seed=7788  path=/share/test.idb/mydb  count=100000

......................... (00010000)
......................... (00020000)
..............^C
➜  src git:(master) ✗ IDBPAGESIZE=1000 ./benchmark write /share/test.idb/mydb 100000
➜  src git:(master) ✗ nano clearzfstest.sh
➜  src git:(master) ✗ chmod +x clearzfstest.sh
➜  src git:(master) ✗ ./clearzfstest.sh
➜  src git:(master) ✗ IDBPAGESIZE=10 ./benchmark write /share/test.idb/mydb 10000
<Writing Test>
  pagesize=10 seed=7788  path=/share/test.idb/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 7.517
ok

➜  src git:(master) ✗ IDBPAGESIZE=10 ./benchmark write /share/test.idb/mydb 10000
<Writing Test>
  pagesize=10 seed=7788  path=/share/test.idb/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 4.579
ok

➜  src git:(master) ✗ IDBPAGESIZE=10 ./benchmark read /share/test.idb/mydb 10000
<Reading Test>
  pagesize=10  seed=7788  path=/share/test.idb/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 4.370
ok

➜  src git:(master) ✗ IDBPAGESIZE=10 ./benchmark read /share/test.idb/mydb 10000
<Reading Test>
  pagesize=10  seed=7788  path=/share/test.idb/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 0.629
ok

➜  src git:(master) ✗ ./clearzfstest.sh
➜  src git:(master) ✗ IDBPAGESIZE=100 ./benchmark write /share/test.idb/mydb 10000
<Writing Test>
  pagesize=100 seed=7788  path=/share/test.idb/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 17.294
ok

➜  src git:(master) ✗ IDBPAGESIZE=100 ./benchmark write /share/test.idb/mydb 10000
<Writing Test>
  pagesize=100 seed=7788  path=/share/test.idb/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 12.131
ok

➜  src git:(master) ✗ IDBPAGESIZE=100 ./benchmark read /share/test.idb/mydb 10000
<Reading Test>
  pagesize=100  seed=7788  path=/share/test.idb/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 4.257
ok

➜  src git:(master) ✗ IDBPAGESIZE=100 ./benchmark read /share/test.idb/mydb 10000
<Reading Test>
  pagesize=100  seed=7788  path=/share/test.idb/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 0.538
ok

➜  src git:(master) ✗ ./clearzfstest.sh
➜  src git:(master) ✗ IDBPAGESIZE=1000 ./benchmark write /share/test.idb/mydb 10000
<Writing Test>
  pagesize=1000 seed=7788  path=/share/test.idb/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 77.467
ok

➜  src git:(master) ✗ IDBPAGESIZE=1000 ./benchmark write /share/test.idb/mydb 10000
<Writing Test>
  pagesize=1000 seed=7788  path=/share/test.idb/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 48.229
ok

➜  src git:(master) ✗ IDBPAGESIZE=1000 ./benchmark read /share/test.idb/mydb 10000
<Reading Test>
  pagesize=1000  seed=7788  path=/share/test.idb/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 4.204
ok

➜  src git:(master) ✗ IDBPAGESIZE=1000 ./benchmark read /share/test.idb/mydb 10000
<Reading Test>
  pagesize=1000  seed=7788  path=/share/test.idb/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 0.450
ok

    ➜  src git:(master) ✗ ./benchmark write /share/mydb 100000
    <Writing Test>
      seed=7788  path=/share/mydb  count=100000
    record number: 100000
    time: 131.356
    ok
    ➜  src git:(master) ✗ ./benchmark read /share/mydb 100000
    <Reading Test>
      seed=7788  path=/share/mydb  rnum=100000
    record number: 0
    time: 3.001
    ok

奇怪在ZFS下无法遍历目录？找到了这是bug，某些文件系统下，readdir
无法返回正确的d_type,总是返回0。


当新建一个后：

    zfs create storage/share/test.idb

    ➜  src git:(master) ✗ ./benchmark write /share/test.idb/mydb 10000
    <Writing Test>
      seed=7788  path=/share/test.idb/mydb  count=10000

    record number: 10000
    time: 2.835
    ok
    ➜  src git:(master) ✗ ./benchmark write /share/test.idb/mydb 10000
    <Writing Test>
      seed=7788  path=/share/test.idb/mydb  count=10000

    record number: 10000
    time: 1.760
    ok
    ➜  src git:(master) ✗ ./benchmark read /share/test.idb/mydb 10000
    <Reading Test>
      seed=7788  path=/share/test.idb/mydb  rnum=10000

    record number: 0
    time: 3.252
    ok
    ➜  src git:(master) ✗ ./benchmark read /share/test.idb/mydb 10000
    <Reading Test>
      seed=7788  path=/share/test.idb/mydb  rnum=10000

    record number: 0
    time: 0.298
    ok
    ➜  src git:(master) ✗ ./benchmark read /share/test.idb/mydb 10000
    <Reading Test>
      seed=7788  path=/share/test.idb/mydb  rnum=10000

    record number: 0
    time: 0.304
    ok

### On ext4

  src git:(master) ✗ IDBPAGESIZE=256 ./benchmark write /mnt/home/riceball/mydb 10000
<Writing Test>
  pagesize=256 seed=7788  path=/mnt/home/riceball/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 8.551
ok

➜  src git:(master) ✗ IDBPAGESIZE=256 ./benchmark write /mnt/home/riceball/mydb 10000
<Writing Test>
  pagesize=256 seed=7788  path=/mnt/home/riceball/mydb  count=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 10000
time: 1.038
ok

➜  src git:(master) ✗ IDBPAGESIZE=256 ./benchmark read /mnt/home/riceball/mydb 10000
<Reading Test>
  pagesize=256  seed=7788  path=/mnt/home/riceball/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 100002
time: 0.186
ok

➜  src git:(master) ✗ IDBPAGESIZE=256 ./benchmark read /mnt/home/riceball/mydb 10000
<Reading Test>
  pagesize=256  seed=7788  path=/mnt/home/riceball/mydb  rnum=10000

......................... (00001000)
......................... (00002000)
......................... (00003000)
......................... (00004000)
......................... (00005000)
......................... (00006000)
......................... (00007000)
......................... (00008000)
......................... (00009000)
......................... (00010000)
record number: 100002
time: 0.136
ok

➜  src git:(master) ✗ IDBPAGESIZE=256 ./benchmark write /mnt/home/riceball/mydb 100000
<Writing Test>
  pagesize=256 seed=7788  path=/mnt/home/riceball/mydb  count=100000

......................... (00010000)
......................... (00020000)
......................... (00030000)
......................... (00040000)
......................... (00050000)
......................... (00060000)
......................... (00070000)
......................... (00080000)
......................... (00090000)
......................... (00100000)
record number: 100000
time: 48.222
ok

➜  src git:(master) ✗ IDBPAGESIZE=256 ./benchmark write /mnt/home/riceball/mydb 100000
<Writing Test>
  pagesize=256 seed=7788  path=/mnt/home/riceball/mydb  count=100000

......................... (00010000)
......................... (00020000)
......................... (00030000)
......................... (00040000)
......................... (00050000)
......................... (00060000)
......................... (00070000)
......................... (00080000)
......................... (00090000)
......................... (00100000)
record number: 100000
time: 10.755
ok

➜  src git:(master) ✗ IDBPAGESIZE=256 ./benchmark read /mnt/home/riceball/mydb 100000
<Reading Test>
  pagesize=256  seed=7788  path=/mnt/home/riceball/mydb  rnum=100000

......................... (00010000)
......................... (00020000)
......................... (00030000)
......................... (00040000)
......................... (00050000)
......................... (00060000)
......................... (00070000)
......................... (00080000)
......................... (00090000)
......................... (00100000)
record number: 100002
time: 1.774
ok

    ./benchmark write /mnt/home/riceball/mydb 100000
    <Writing Test>
      seed=7788  path=/mnt/home/riceball/mydb  count=100000

    record number: 100000
    time: 31.869

    ➜  src git:(master) ✗ ./benchmark read /mnt/home/riceball/mydb 100000
    <Reading Test>
      seed=7788  path=/mnt/home/riceball/mydb  rnum=100000
    record number: 100002
    time: 2.601
    ok

    ➜  src git:(master) ✗ ./benchmark write /mnt/home/riceball/test.idb 10000
    <Writing Test>
      seed=7788  path=/mnt/home/riceball/test.idb  count=10000

    record number: 10000
    time: 2.017
    ok

    ➜  src git:(master) ✗ ./benchmark write /mnt/home/riceball/test.idb 10000
    <Writing Test>
      seed=7788  path=/mnt/home/riceball/test.idb  count=10000

    record number: 10000
    time: 0.916
    ok

    ➜  src git:(master) ✗ ./benchmark read /mnt/home/riceball/test.idb 10000
    <Reading Test>
      seed=7788  path=/mnt/home/riceball/test.idb  rnum=10000

    record number: 10000
    time: 0.190
    ok

    ➜  src git:(master) ✗ ./benchmark read /mnt/home/riceball/test.idb 10000
    <Reading Test>
      seed=7788  path=/mnt/home/riceball/test.idb  rnum=10000

    record number: 10000
    time: 0.141
    ok

# 2013-03-08

The math library must be linked in when building the executable. How to do this varies by
environment, but in Linux/Unix, just add -lm to the command:

    gcc --std=gnu99 -I. -Ideps -o benchmark benchmark.c idb_helpers.c isdk_xattr.c isdk_utils.c deps/sds.c deps/zmalloc.c deps/utf8proc.c -lm

The functions in stdlib.h and stdio.h have implementations in libc.so (or .a static linking),
which is linked into your executable by default (as if -lc were specified). GCC can be instructed
to avoid this automatic link with the -nostdlib or -nodefaultlibs options.

The math functions in math.h have implementations in libm.so (or .a for static linking), and libm
is not linked in by default. There are hysterical raisins historical reasons for this libm/libc
split, none of them very convincing.

Interestingly, the C++ runtime libstdc++ requres libm, so if you compile a C++ program with GCC (g++),
you will automatically get libm linked in.





# 2013-02-13

Now, I must use utf-8 lib to split the string.

    See idb_helper.c#_GetKeyDir



# 2013-02-12

the path of 'mkdir' func max length is about 1024.
So I have to use "cd" working dir. I must use the
processes only(DO NOT USE threads).

In the same processes. if u must use thread, then
the current working dir must the iDB root dir.
And the key max length is always 1024.

the best way is no thread!!

ccan/antithread is a reference:

    The antithread module provides memory-sharing infrastructure: the programmer
    indicates the size of the memory to share, and then creates subprocesses
    which share the memory.  Pipes are used to hand pointers between the
    main process and the children: usually pointers into the shared memory.

想多了，不再这一层考虑，我是准备用mongrel2作为上层的啊，要看看它的是线程模型
还是多进程模型。
