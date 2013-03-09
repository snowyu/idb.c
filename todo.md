内存泄露检查： 
  valgrind  --track-origins=yes  --dsymutil=yes --leak-check=full --tool=memcheck ./test


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
