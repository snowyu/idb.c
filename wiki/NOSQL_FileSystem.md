# 大道至简 -- 之iDB NoSQL数据库开发随笔

Structed-Column Oriented Hierarchy Database

iDB 是结构化的面向列(Structured-Column Oriented),并基于目录(Directory-Based)
的层级式(Hierarchy)NoSQL数据存储系统，鉴于iDB完全有别于传统数据库基于文件的存
储形式。故我把它视作新一代数据库系统(A new way to database engine)的核心。


所谓面向列(Column Oriented)是和面向行(Row Oriented)相对而言的，绝大多数的关系
型数据库就是面向行存储的，而iDB则是面向列基于Key/Value的存储系统，所谓结构化
(Structured)是指值(Value)可以是带结构数据的复杂类型，而层级则是指Key可以有不同
的层级关系，基于目录树的形式。

基于目录(Directory-Based)也是相对于基于文件(File-Based)而言，对于传统的数据库系
统几乎都是通过创建少量的随机文件，然后把大量的数据存放在里面，而iDB则完全不同，
也可以说是恰恰相反，它是创建大量的目录和文件，把数据分散在目录中，你可以认为这是
一种创新。

## Background (设计背景)

iDB NoSQL数据库引擎的设计初衷是希望能更为合理的设计组织数据库引擎的层次架构，
以及看到现代文件系统设计发展的变化有感而发，两者融合就促使我萌发设计iDB的念
头。

### KISS 原则

大家都应该听说KISS(Keep It Simple, Stupid)原则吧，保持架构简单而愚蠢的原则，
因为简单，所以看上去足够的蠢。而将架构设计得足够的简单是架构师的梦寐以求，
大道至简，说着看是容易，实际做起来，其实真难，知易行难是也。

我希望iDB的架构设计做到了足够简单以及足够的愚蠢，这也是我从事架构设计中最接
近KISS原则的一次架构体验，我很乐于与大家分享我这设计过程的经验和错误。

### 文件系统与数据库

前面我提到现代文件系统的发展和变化，这和NoSQL数据库的设计存在什么样的联系？
数据库引擎的设计需要考虑文件系统的特性么？我们可以做一个列表来对比，文件系
统和数据库的功能特性：

#### 从操作对象和操作方式上看

* 数据
  * 文件系统: 文件名/文件内容
  * NoSQL   : Key/Value
* 操作
  * 文件系统: 读、写、删除、列举、文件
    * 管理操作: 磁盘系统检查(fsck)，磁盘卷热备
  * NoSQL   : Get, Set. Delete, List Key
    * 管理操作: 数据库校验, 数据库热备

#### 从内部存储结构上看

内部存储结构上，绝大多数数据库系统都是使用Btree树或者其变体来存储数据(也有
用hash表实现的)。而目前越来越多的现代文件系统正在用Btree树或者其变体来存储
目录或文件。比如:

* ext3/ext4 使用Htree(Btree的变体)来索引目录，不过这ext3中需要手动启用dir_index
  功能，ext4默认是启用的。
* btrfs 使用btree,又被称为 "Btree FS"。
* ZFS 使用 Extensible hash table
* Reiser4 使用 Dancing B\*-tree
* XFS B+tree

可以看到在最基本数据存储和访问上，数据库和文件系统有惊人的相似性。为了最大限度
的优化数据库的性能，开发者做了许多与文件系统相同的重复工作，磁头位置预估，扇面
连续，defragment等等，并针对SSD做优化。但是这些相同的工作文件系统也都在做。

#### 不要重复发明轮子(DRTW)

那么不同的地方在哪里？数据库更复杂，除了基本的存储和访问需求外，它还需要对结
构化的数据进行检索和处理。但是因为老的数据库思路是申请一个文件，也即是磁盘空
间的一个区域在这上面进行数据存储和访问，这样，文件系统的成果自然无法用上。

So，根据懒人法则第一条，能躺绝坐，能坐绝不站。站在巨人的肩膀上，才能看得更远，
走得更远。那么怎么样对数据存储才能利用上文件系统的成果呢？

让我们跳出来文件这个限制框框思考，为啥数据库的数据一定要保存在文件中？为啥不能
直接用文件的目录系统直接作为NoSQL数据库的存储呢？当我们跳出这个限制：数据必须保
存到文件中。我们就会欣然发现，只要我们正视文件系统的一些限制，其实目录也可以存
数据。既然文件系统都已经充分考虑并已经成熟(N多程序在通过不同方式使用着文件系统)，
为啥要重复发明轮子？作为数据库引擎应该更专注于数据处理才对，而不是数据存储。

### KISS IT

iDB就是这样的一次尝试。它试图充分而彻底的利用文件系统本身来进行数据存储，你甚至
可以利用文件系统自己去浏览管理数据，看上去，我所做的一切似乎仅仅只是制定了数据
存储的规范和一系列的约定。其中，最重要，最核心的约定是文件系统的目录名称为Key名，
该目录下的“.value”文件内容为值的内容，“.type”文件内容为该Key的类型说明。

听上去够简单吧。也许简单到能让你发笑。

古语有云:

    上士闻道，勤而行之；中士闻道，若存若亡；下士闻道，大笑之。不笑不足以为道。

就这么个简单东东，我从去年7月份开始设计，断断续续直到现在，逐步理清思路，才实施
了一个能实用的东西。中间走了些弯路，单是规范修改了好几次。而且这一切仅仅还只是
一个开端。如此，是不是够蠢。

    Stay simple ,stay foolish...

如上所述就是开发iDB的设计背景，下面从哪里谈起呢?想写的东西太多，就先从iDB规范讲起吧。
(未完待续)

## iDB 架构与规范

草拟规范是架构设计中尤为重要的一环，架构没有规范，如同建筑师没有设计蓝图就开始
修建房屋。无论是自顶向下还是自底向上设计，每一步都离不开规范文档。从自底向上看
描述数据在文件系统中如何存储的iDB存储规范是基石，没有它，一切都是空谈。

起初iDB规范只有一个，后来逐渐分为多个，自底向上看:

* iDB存储规范：用于描述数据在文件系统中是如何存储的。它是与文件系统的接口规范
* iDB数据库规范：用于描述iDB数据的类型、索引规范以及其它相关约定
  * iDB Key/Value 数据库规范
  * iDB 关系型数据库规范
* iDB 存储服务规范：用于描述iDB存储服务的功能规范，分为三层:
  * 读写分离的异步通讯IO层
  * Memcache 内部缓存层
  * 实际iDB存储层
* iDB云数据库服务规范：以下规范实际上与iDB存储无关，只是用于描述一种分布式NoSQL
  数据库的规范方案，我称之为iDB云数据库规范。
  * iDB 节点控制器: 控制和监控数据库，作为与各种数据库的中间代理层
  * iDB 集群控制器: 管理iDB节点控制器,数据定位
  * iDB 云控制器: 是暴露在互联网上顶层组件。为外部世界提供RESTful API以及Web接口

目前只实现iDB存储规范中的内容，文档还很乱，写得比较糟糕，但是具体思路已经敲定了，
只剩下文字增订的事情，iDB规范依然还在草拟中。

### iDB存储规范

iDB存储规范是iDB最基础的规范约定，描述了iDB基础数据是如何被存储和访问。

我们大概已经知道了，目录名为Key名，目录名下的".value"文件内容为键值。
我们将".value"称之为key的一个属性，".value"是其中最重要的一个属性。一个
key(键)可以有多个属性，对于一般使用者来说，是无法直接看见这些属性的，这些
属性为iDB规范服务的。

#### 基础概念

小结，从以上的描述，我们可以总结出以下的概念

* Key Name: 键名，同于目录名
* Attribute: 键属性, 同于文件名，其文件内容为键属性值。
  * .value: 键值属性
  * .type: 键值类型属性

btw, 这里约定所有的字符串编码统一为utf-8编码格式。其中包括key name.
值内容为纯字符串的形式表示。"redis"类型例外，如果类型为"redis"，则值
内容为redisIO的raw格式。

路径为iDB层次性的体现，因此键路径出现了:

* Key Path: 键路径，同于目录路径

然后就有了子健:

* Subkey: 子健，属于键路径下的某个键，称为该键路径下的子健。

随着对子健遍历需求的出现，又诞生了对键的分页(分区)的需要。

##### 分区/分页(Partition Key)

对iDB数据库根目录下的子健的遍历就是对整个数据库的key的遍历。
如果数据库中数据巨大，那么一次遍历传回所有key数据，可能消耗
大量的时间和资源，在这种情况下可能需要分页(分区)的形式来组织key列表。

那么分区(分页)是否是必要的？
（待续）

##### 为啥要分区

有了遍历，分页的出现就很自然了，两个理由需要分页：

* 分页实质是对key的内部“分区”，从而使得客户端不会在每次遍历中
  通过索取大量的数据，而占用太多服务资源。这可以当作对每次任务
  的分片操作。
* 文件系统中同一目录下的子目录数量是有限制的，具体限制数量随着
  文件系统的不同而不同。
  * ext3 限制为最多32,000个子目录
  * ext4 默认限制为64,000个子目录，可以取消限制
  * zfs     2^48
  * btrfs   2^64
  * Reiser4 1,200,000

对了，说道这里，顺道提下key name 的长度限制，大多数文件系统把文
件名的长度限制为255个字符，除了Reiser4的文件名可以是3976个字符外。

接着说"partional"key，当子键数量达到IDBMaxPageCount限制后，就需要对
该子健进行分页存放了，不能直接存放在该目录下了。

注：IDBMaxPageCount 小于等于0的时候为不分页。

##### 分区方案1

分页方案，想了好久，最开始本想用一致性hashing，有两个原因让我放弃了：

1. 丧失了key的直观可读性（理解性）
2. 对同一数据库内部的分区来说太重

后来采用的是通过分割Key进行分区的方案。最开始的分割办法是这样的，设有
键名jack：

* 当子目录满后，分割键名的第一个字母j, 得到j/ack，也就是说存放在j目录下，
* 如果j目录也满了，那么就继续分割，得到j/a/ck
* 如此类推,直到j/a/c/k都满了，报告分区full，无法存放


###### 存在问题

这种分割方式要求IDBMaxPageCount必须至少大于等于你的key所使用字符集的数量。
这个在新的文件系统上一般问题不大，对于ext3，如果key在utf-8中文字符集中，
那么所用的字符个数如果超过32,000会是个问题。这种分割方式的好处在于非常
直观。一眼就能看明白。后来发现的一个问题，解决之后又发现另一个问题，而这个
问题使得我放弃了该方案，或者说改进了该方案。这个问题使得存储和访问变得异常
的复杂，从stay simple 变成了stay complex。那么问题再哪里？

Think it over，且听下回分解。


###### 分割Key分区方案

上回说到，初次构想的分割key分区方案存在问题，回顾下分割办法，设有键名jack：

* 当子目录满后，分割键名的第一个字母j, 得到j/ack，也就是说存放在j目录下，
* 如果j目录也满了，那么就继续分割，得到j/a/ck
* 如此类推,直到j/a/c/k都满了，报告分区full，无法存放

那么这样分割会把事情弄复杂的关键出在哪里？反过来思考下键"j/a/c/k"，它的真实
的键一定会是"jack"？不，它还可能是"j/ack"，"j/a/ck"，以及"j/a/c/k"。那么该如
何区分它们？补充方案，可以引入属性后缀的形式：

* attr值在 ".[attr].jack"文件中   if the real key is "jack"
* attr值在 ".[attr].ack"文件中    if the real key is "j\ack"
* attr值在 ".[attr].ck"文件中     if the real key is "j\a\ck"
* attr值在 ".[attr]"文件中        if the real key is "j\a\c\k"

引入上述的补充方案就可以解决该分割方案中键名混淆的问题，但是这样一来，就使得
同一目录会有多个键的属性存在，使得键的操作复杂化。然后导致开发也变复杂。这个
方案上我犯了错误，那么错误的根本是在哪里？为什么会犯这样的错误？

错误的根本在于，我将一个目录一个键名，慢慢变成了允许同一目录下有多个键存在。
当一对一变成一对多的时候，事情也就一下从简单变复杂了。基本定义必须保证简单明
确，理清概念的内涵和外延万分重要。因此，在这里重申关于键名的定义如下：

* 键名(Key Name)等同于目录名，一目录对应一键名，而且仅仅只对应一个键名。

然后因为我将键的值存放在属性中，因此引出了属性的定义：

* 属性名(attribute Name) ，一个键可以有多个属性，其中必须存在最基本的属
  性是“值”属性。当属性存储为文件时，它于文件名有一一对应关系，文件名等于
  以前缀"."打头的属性名。

为了表述键之间的层级关系，因此引出了键路径的定义：

* 键路径(Key Path): 等同于目录路径，用以描述键的树状层级关系。

通过这三者的组合，将能形成各种复杂的数据库系统，这个容我以后慢慢道来：

    道生一，一生二，而生三，三生万物

在遵循上述基本概念"一键一目录"下，key该如何进行分区？

##### 新的分割Key分区方案

鉴于键(Key)总是遵循"一键一目录"的前提下，为了分区，我引入了新的键类型：
分区键(Partition Key)，用来与正常的键(Normal Key)以示区分。

* Partition Key(分区键): 为分区而存在的特殊键类型，它总是以前缀"."打头的一个字符。

分区办法如下，设对键名"jack"进行分区 ：

* 当子目录满后，分割键名的第一个字母j, 得到“.j/ack”，也就是说存放在“.j”子键(分区键)下，
* 如果“.j"分区键目录也满了，那么就继续第二个字母分割，得到".j/.a/ck"
* 如此类推,直到".j/.a/.c/"都满了，报告分区full，无法存放

如此，键名不会在被混淆，".j/.a/.c/k" 仅仅只表示 "jack"，而不会表示其它。上文
分割方案中的大麻烦终于解决了。但是，随着旧的问题解决，接着新的问题又产生了。
不过，还好这是个小问题。这是一个关于键重复(Duplication Key)的问题，下述四键：

* "jack" (没有分区)
* ".j/ack"
* ".j/.a/ck"
* ".j/.a/.c/k"

我们可以看到，上述四键都是指的同一键"jack"，如果它们都在数据库中存在，那么该
以何键为准？为了解决这个小问题，因此这里作了一个关于访问规则的约定：层级小的
键总是优于层级大的键访问：

    "jack" > ".j/ack" > ".j/.a/ck" > ".j/.a/.c/k"

* "jack"  没有分区 = 分区 0 层
* ".j/ack"         = 分区 1 层
* ".j/.a/ck"       = 分区 2 层
* ".j/.a/.c/k"     = 分区 3 层

也就是说，当它们四者同时存在于数据库的时候，会以"jack"键的数据为准，忽略其它
数据，而当"jack"被删掉后，则又会以".j/ack"的数据为准。

该方案还会有其它问题发生么？暂时我还没有发现，期待你的发现，如有任何发现，殷
切盼望告知于我，万分感谢。

如果某个属性占用了怎么办，它就是这么贱只用1个字！，设定一个特定的属性（系统抢先占用）:
“.分区/”。

#### 操作数据(Operations)

根据上述数据，我们自然就可以得知与数据相关的操作如下：

* 读写操作：
  * Get/Put 键属性(iGet/iPut)
  * Delete 删除键(iKeyDelete)，删除键属性(iDelete)
  * 键别名操作: iKeyAlias
* 访问操作：
  * 判断键是否存在(iKeyIsExists)，属性是否存在(iIsExists)
  * 遍历操作：
    * 枚举子键(iSubkeys)
    * 枚举属性(iAttrs)
    * 统计子键个数(iSubkeyTotal)
    * 统计正常子键个数(iSubkeysCount): 不包括分区的数据。
    * 统计属性个数(iAttrCount)


### Subkeys 索引和Subkeys Count

费时，需要索引和Count

以前是可以任意路径，如果路径中有不存在的，自动创建子目录

这样导致很麻烦，在存储key中，得必须为每一个不存在的目录，
也要去建立索引和count+1.

两个办法
1、始终在DB的根目录上索引和统计
2、禁止父目录不存在的情况下存储key。

选择方案1，在DB根目录下索引统计。
使用一个简单的key/value数据库作为索引解决方案。
分库方案：当到达单库限制的时候,需要分库

DB根目录下建立.index 目录作为索引统计目录

* .index/.count: 内容为一个DB
  * count= 所有的总数
  * [key].count = 该[key]下的总数
  * [key] = .k/ey  等到[key]的真实位置
* .index/.keys: 内容为DB,把所有的索引放置在一个数据库中？还是拆分开？
  * [key].index/abc = .k/ey
* .index/[key]/.value: 内容为该key的索引DB
  * index/abc = .k/ey
* .index/.type: 表示用DB的类型： sqlite, levelDB, lmdb...

## iDB数据库规范

任何一个目录都有可能成为iDB数据库或者叫iDB数据库项.每一个目录都是一个key.
在该目录下的子目录称为该目录的subkey。每个key可以有多个属性，多个subkeys.
key中最重要的属性就是value.

Path, Key, Value, Attribute

注意：属性可以是直接的值(File)，也可以是Subkey.

最重要的属性：

* value
* type


最底层函数被用来存储管理属性和Subkeys。

更上层的函数利用属性类型构造存放数据类型。

--------------------------
在redis-iDB的过程中，发现一些有趣的问题，
首先是异步写入引发的，
1. 当我删key后，后台还没有异步写入的情况，这个时候后面保存该key，会加入老数据
   因为存入操作是首先去lookupWrite，这就成了bug！
2. aof 使用dirty 判断是否需要调用追加，这使得同步写入如果直接dirty--,aof就不工作。

是我的错么？如果有20个并发再写同一个key，一个写的ziplist, 还有一个写的是linkedlist
类型，那么自然会报告WRONGTYPE Operation against a key holding the wrong kind of value.


Redis集成测试：它的持久化存储只是cache备份，不存在从磁盘上找数据的。
所以它可以不关心清空数据库，这集成测试IDB中，引发随机问题。

问题依然存在，如果始终大概是在 deleteKeyOnIDB的时候，dictDelete()
然后报告 试图decrRefCount 小于等于0 ！
我已经仔细看过了，加入dirtyKeys的时候已经增加了incrRefCount!!
没折了，准备把Key换回 sds实验下
这个原因是因为内存中用的hash类型是：dbDictType, 它要求的key是sds类型，而不是robj.

这一起测试中，会并发开启多个测试客户端进行测试，我发现
它始终用一些同样的 x 键作为名称，如果以前在内存中，咩有
问题，但是这磁盘上，那么就可能会有问题！！如果某个测试
写的x正好被后台写到磁盘，那么如果另一个测试也是用的相同
目录，那么就不正确了。所以单独测试ok，一旦一起测试，就
有一定机率出错。我的办法是改键名。

Case 1:
  set a key (dict no and disk is yes)
  lookup the key, find on disk, add to db.dict
  dbAdd(dict)
  setKeyOnIDB: add to dirtyKeys(if no bgsaving) or to dirtyQueue(bgsaving)

  del the key
    del from db.dict
    when bgsaving:
        add/update to dirtyQueue




1、缘起
2、存储的基本原理（一堆的名词解释）与设计理念（KISS）
3、自顶向下的架构简介
4、架构设计中的决策考虑
5、技术方案的spec
6、性能预估与评测方案
7、评测结果
8、iDB的适用场景
9、其他

iDB数据库它简单到什么样的程度？简单到也许就能令你发笑，它没有B+tree, 没有LRU,
没有Cache,只关注数据本身。那么它是如何存储数据的呢？充分而彻底的利用文件系统本
身来进行数据存储，你甚至可以利用文件系统本身去浏览管理数据，看上去，我所做的一
切似乎仅仅只是制定了数据存储的规范和一系列的约定。其中，最重要，最核心的约定是
文件系统的目录名称为Key名，该目录下的“.value”文件内容为值的内容，“.type”文件内
容为该Key的类型说明。

是什么让我做出如此简单愚蠢可笑的基于文件系统的iDB数据库架构设计呢？

让我们回顾下数据库系统和文件系统的功能。


数据库的数据存储最终需要保存到磁盘上。数据库的访问和文件系统的访问是类似的：读写、搜索。在我看来要想提升数据库
的性能不如深度利用文件系统本身的特性。当然为了挖掘数据库系统底层性能的还有一类例如Oracle，走上另一条路：饶过文
件系统，从磁头，柱面，扇道开始开发，直接磁盘当raw盘用。我认为这不可取，这使得数据库开发者又需要去专精文件磁盘
系统的专业知识：磁头位置预估，扇面连续，defragment等等，这个诸多文件系统的开发者已经累积了n年的经验，并且在不断的
发展，术业有专攻，不好好去做数据库引擎自身的开发，转而去研究文件系统，吃饱了撑的。



The iDB is a Key/Value Data Store, the Key can be a Hierarchy level. the Value can be a structured/object value.
iDB is an open source, advanced hierarchy key-value store. It is often referred to as a data structure server since keys can contain strings, hashes and hierarchy key supported.


参数配置：

根据文件系统的不同而不同

MAX_FILENAME_LEN =
MAX_DIR_NUM =

## The Key

The Directory Name

    目录监视 http://inotify.aiken.cz/?section=inotify&page=why&lang=en

Key: Can be the Parent/Child Level
Key: 是具有父子（层级）关系的索引值，在文件系统中表现为目录.

Limits:

  the same level key maxmium count is the sub-directory maxmium count.

ZFS:  子目录的限制为 2^48
      ~14k files per directory is a reasonable maximum for maintaining good performance.
ext4：默认子目录数量的限制为64,000，using the "dir_nlink" feature (but the parent's link count will not increase).
      取消限制：tune2fs -o user_xattr -O dir_nlink,dir_index [device]
ext3: 子目录数量只有32,000, 只能在源代码中修改参数:
      /usr/src/linux-2.4.19/include/linux/ext3_fs.h has define EXT3_LINK_MAX 32000.
btrfs: 2^64 (Max number of files)
Reiser4: 1 200 000

max filename length:
btrfs: 255 chars
zfs: 255 chars
ext3/ext4: 255 chars
Reiser4: 3976 chars

http://en.wikipedia.org/wiki/Comparison_of_file_systems

How many bytes per inodes?
I need to create a very high number of files which are not very large (like 4kb,8kb). It's not possible on my computer cause it takes all inodes up to 100% and I cannot create more files :

-bash-4.0$ df -i /dev/sda5
Filesystem            Inodes   IUsed   IFree IUse% Mounted on
/dev/sda5            54362112 36381206 17980906   67% /scratch
(I started deleting files, it's why it's now 67%)

The bytes-per-nodes are of 256 on my filesystem (ext4)

-bash-4.0$ sudo tune2fs -l /dev/sda5 | grep Inode
Inode count:              54362112
Inodes per group:         8192
Inode blocks per group:   512
Inode size:           256
I wonder if it's possible to set this value very low even below 128(during reformating). If yes,what value should I use? Thx

Answer:
The default bytes per inode is approximately 16384. If you're running out of inodes, you might try for example:

mkfs.ext4 -i 8192 /dev/mapper/main-var2
Also, you can not change the number of inodes in a ext3 or ext4 filesystem without re-creating it. Reiser filesystems are dynamic so you'll never have an issue with them.


The inode-size (-I) is something different than the bytes-per-inode (-i) setting. The inode-size determines the size of a single inode, larger inodes can contain more pointers to blocks, reducing the need for indirect blocks at the cost or increased disk usage. The bytes-per-inode setting sets a ratio that will be used to determine the maximum number of inodes. None of these two values can be changed after the filesystem had been created.


Ext4 XATTRs upto 4kb.

主意：在Linux下，xattr 只能使用 "user."打头的属性，

Currently on Linux there are four namespaces for extended file attributes:

user
trusted
security
system

This article will focus on the “user” namespace since it has no restrictions with regard to naming or contents. However, the “system” namespace could be used for adding metadata controlled by root.


http://www.linux-mag.com/id/8741/

## The Value

The File name Or File Body Or Extended Attributes(xattr)

Value: 以文件以及扩展属性(xattr)体现。

## The Types


现在我似乎把它们混在一起，还是先描述，以后再看怎么整理。
每一个Key有一个“类型”属性，用来指明该key的类型。
类型可以为两类“单值”类型和“列表”类型，
“列表”类型可以看作是dict(字典)类型，数组则是key为数字的特殊字典。
“列表”类型均可以有索引，以数据表举例，设想有“(字)典”这样的key。
它是一个数据表（字典），存放所有的字(词)。
这里的数据表的字段必须存在一个PrimeKey(PK),需要属性"Identifier"，
来描述PrimeKey的构成，属性"Identifier"的内容是以逗号分隔的字段名称。

这样是不对的！对于Key的(父子)层次，以前是这样定义的,Key的层次表示继承派生关系,
子层次的Key会继承所有的父层属性和方法。这有点错误，这个是高层KB数据库的定义，
而在底层的key存储中，应该是由数据类型来决定。key的分层只是目录存储，便于隔离
和查找。我脑子有点搅了，一会儿数据库，一会儿知识库。

这里实际上是把索引和key混为一谈了。这里的字是按照语言、词性索引的。

* /典/[字]/[语言]/[词性]/
  * /典.type=Dict
  * /典.Identifier=字,语言,词性
  * /典.Keys=字,语言,词性,释义 (默认已知字段，但是可以不限于此)
    * /典.Keys.type=Dict
    * /典.Keys/字=/人/文/语言/字
    * /典.Keys/语言=/人/文/语言
    * /典.Keys/释义=/人/文/语言/释义
  * /典.Indexes/

注：为了兼容一般的Key/Value数据存储，Key的形式有些变化：
不过这些应该在底层，上层看不到的。

* 在k/v系统中: /典.Keys.type
* iDB: /典/.Keys/.type

对于上层而言，这些是无法感知的？
不是的，还是会反应到上面的！！
如果以Key的形式来获取属性。
决定采用k/v的构造形式，在iDB的时候翻译下，再传到下面。
关于索引的引发的性能问题，如果每一个key都可能有索引，
那么在写之前必须至少读取一次key，用于判断以及取出索引项，才能写入。
提示性能：
* 建立Schema(metadata)Cache,缓冲MetaData(元数据).



* Simple(简单类型)
  * String(字符串)
  * Number(数值)
  * Boolean
  * Identifier: Ref (引用) [NOT Published]
* Complex(复杂类型)
  * Array(数组)
  * Class(类)
  * Blob
    * MIME

### Simple Types

The Simple Types includes the Boolean, Number , String and Identifier Type.

#### Boolean

=true/false, yes/no

#### Number & String

I use file name with a "=" prefix to treat as the value of number or string.

The number is always 10 base or 16 base(prefix $).
The String must be quoted before.(maybe automatically?)

The String Max Length is the file name max length.

suppose we have a key "age" and the value is 23; a key name and the value "Mike jans" in my FS_DB:

    $ls -l age
    -rw-r--r--  1 staff  staff     0 Jul 20 15:31 =23
    $ls -l name
    -rw-r--r--  1 staff  staff     0 Jul 20 15:31 ='Mike jans'

Escape char: \
the slash character can not be escaped,  The slash separates the filenames that form a pathname.


#### Identifier

只能是字母数字“\_"组成，必须以字母打头。

I use the directory name with a '=' prefix as the value of the Identifier.

### Complex Types

## The Operations

### Get/Put

Key:  users/mike/age
Key:  users/mike/name

### List

Key: users/

the users is a list of user object.


Create a new directory base on the first char of the object id when the users is get the limits of the files.

Such as: Jaons, Mike

Keys: users/j/Jaons, users/m/Mike


如何(在哪里)表示 users 的类型？单独一个metadata目录？还是混合在一起？

混合方式:
users/
.type/=list   标示该项的类型
.count/=2     记录列表的总数
.level/=1     记录的当前级数（0表示没有拆分子目录,都在同一目录下，1表示拆分有1级目录)
.case_sensitive/=false

users/j
#如果metadata项与上一层相同，就不出现
.count/=1     记录列表的总数
.level/=0     记录的当前级数（0表示没有拆分子目录,都在同一目录下)

users/j/Jaons
.type=Object/User


————————————————————————————————

删除目录使用attr会快很多

ext4/btrfs 差不多(attr):
benchmark the add key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 2.05 2.02 2.1 1000 loops, best of 3: 2.02 msec per loop
running time(H M S): 00 00 06
2. raw times: 1.99 2.44 2.05 1000 loops, best of 3: 1.99 msec per loop
running time(H M S): 00 00 07
3. raw times: 2.43 2.11 2.29 1000 loops, best of 3: 2.11 msec per loop
running time(H M S): 00 00 08
total running time(H M S): 00 00 21
root@vagrant-ubuntu-precise:/vagrant/iDB/benchmark# ./testGet.sh  testiDB
benchmark the get key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 0.565 0.515 0.493 1000 loops, best of 3: 493 usec per loop
running time(H M S): 00 00 02
2. raw times: 0.539 0.492 0.488 1000 loops, best of 3: 488 usec per loop
running time(H M S): 00 00 02
3. raw times: 0.596 0.54 0.509 1000 loops, best of 3: 509 usec per loop
running time(H M S): 00 00 02
total running time(H M S): 00 00 06

ext4(dir):
benchmark the add key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 2.11 2.3 2.37 1000 loops, best of 3: 2.11 msec per loop
running time(H M S): 00 00 07
2. raw times: 2.16 2.6 2.21 1000 loops, best of 3: 2.16 msec per loop
running time(H M S): 00 00 07
3. raw times: 2.55 2.39 2.88 1000 loops, best of 3: 2.39 msec per loop
running time(H M S): 00 00 09
total running time(H M S): 00 00 23

benchmark the get key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 0.496 0.457 0.503 1000 loops, best of 3: 457 usec per loop
running time(H M S): 00 00 02
2. raw times: 0.555 0.488 0.513 1000 loops, best of 3: 488 usec per loop
running time(H M S): 00 00 02
3. raw times: 0.45 0.433 0.476 1000 loops, best of 3: 433 usec per loop
running time(H M S): 00 00 02
total running time(H M S): 00 00 06

btrfs(dir):
benchmark the add key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 2.57 2.8 2.52 1000 loops, best of 3: 2.52 msec per loop
running time(H M S): 00 00 09
2. raw times: 2.51 2.42 2.42 1000 loops, best of 3: 2.42 msec per loop
running time(H M S): 00 00 08
3. raw times: 2.41 2.45 2.36 1000 loops, best of 3: 2.36 msec per loop
running time(H M S): 00 00 07
total running time(H M S): 00 00 24

benchmark the get key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 0.469 0.454 0.455 1000 loops, best of 3: 454 usec per loop
running time(H M S): 00 00 01
2. raw times: 0.459 0.459 0.458 1000 loops, best of 3: 458 usec per loop
running time(H M S): 00 00 02
3. raw times: 0.464 0.423 0.427 1000 loops, best of 3: 423 usec per loop
running time(H M S): 00 00 02
total running time(H M S): 00 00 05

ZFS:

dir attr:
benchmark the add key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 2.99 3.25 3.34 1000 loops, best of 3: 2.99 msec per loop
running time(H M S): 00 00 10
2. raw times: 3.43 3.59 3.73 1000 loops, best of 3: 3.43 msec per loop
running time(H M S): 00 00 11
3. raw times: 3.42 3.86 3.71 1000 loops, best of 3: 3.42 msec per loop
running time(H M S): 00 00 12
total running time(H M S): 00 00 33
root@vagrant-ubuntu-precise:/vagrant/iDB/benchmark# ./testGet.sh testiDB
benchmark the get key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 0.602 0.595 0.525 1000 loops, best of 3: 525 usec per loop
running time(H M S): 00 00 03
2. raw times: 0.541 0.494 0.491 1000 loops, best of 3: 491 usec per loop
running time(H M S): 00 00 02
3. raw times: 0.565 0.508 0.517 1000 loops, best of 3: 508 usec per loop
running time(H M S): 00 00 02
total running time(H M S): 00 00 07

sa attr:

benchmark the add key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 2.89 2.98 2.91 1000 loops, best of 3: 2.89 msec per loop
running time(H M S): 00 00 09
2. raw times: 3.13 3.47 4.35 1000 loops, best of 3: 3.13 msec per loop
running time(H M S): 00 00 12
3. raw times: 3.94 3.59 3.82 1000 loops, best of 3: 3.59 msec per loop
running time(H M S): 00 00 12
total running time(H M S): 00 00 33
root@vagrant-ubuntu-precise:/vagrant/iDB/benchmark# ./testGet.sh testiDB
benchmark the get key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 0.531 0.501 0.493 1000 loops, best of 3: 493 usec per loop
running time(H M S): 00 00 02
2. raw times: 0.632 0.557 0.558 1000 loops, best of 3: 557 usec per loop
running time(H M S): 00 00 02
3. raw times: 0.654 0.524 0.553 1000 loops, best of 3: 524 usec per loop
running time(H M S): 00 00 03
total running time(H M S): 00 00 07


zfs(dir):
benchmark the add key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 2.7 2.94 2.79 1000 loops, best of 3: 2.7 msec per loop
running time(H M S): 00 00 09
2. raw times: 2.81 2.93 2.92 1000 loops, best of 3: 2.81 msec per loop
running time(H M S): 00 00 10
3. raw times: 2.7 2.82 3.15 1000 loops, best of 3: 2.7 msec per loop
running time(H M S): 00 00 09
total running time(H M S): 00 00 28

benchmark the get key/value, run testiDB loop 1000, repeat 3, total 3
1. raw times: 0.479 0.451 0.454 1000 loops, best of 3: 451 usec per loop
running time(H M S): 00 00 02
2. raw times: 0.511 0.468 0.449 1000 loops, best of 3: 449 usec per loop
running time(H M S): 00 00 02
3. raw times: 0.503 0.485 0.485 1000 loops, best of 3: 485 usec per loop
running time(H M S): 00 00 02
total running time(H M S): 00 00 06

