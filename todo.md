# 2013-02-13



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
