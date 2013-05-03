/*
 * =====================================================================================
 *
 *       Filename:  isdk_indexfile.h
 *
 *    Description:  maintain the sorted keys(index) on the disk.
 *
 *      Item: a fixed length data which includes the key and value.
 *          both the key and value are fixed length.
 *      Block: a block contains a group of items. the count of items in the level 0 block means block size.
 *      Block(n): the (n)th block. the block is fixed length now. Size(n) = BlockBase^n
 *              one level can contain how many items, n is level number(0..MaxBlock-1) :
 *                  BlockBase^n*BlockSize
 *              the block(0) is in the memory, and block(1..MaxBlock-1) is on the disk.
 *      MaxBlockCount: the index file can hold how many blocks.
 *      BlockBase: must be 2^n, 2, 4, 8, the default is 4.
 *      IndexFile: Header(IndexDBFileHeader + IndexDBBlockHeader) + Blocks
 *      Change way:
 *        insert sorted block, new a block when block full.
 *        only splinter file, DO NOT splinter block!!
 *
 *        Version:  1.0
 *        Created:  2013/03/29
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Riceball lEE(snowyu.lee@gmail.com) 
 *
 * =====================================================================================
 */
#ifndef __ISDK_INDEXFILE_H
#define __ISDK_INDEXFILE_H

//#include "isdk_config.h"
#include "isdk_utils.h"
#include "deps/darray.h"

#include <pthread.h>
#include <sched.h>

#define IINDEX_EXT_NAME         ".idx"
#define IINDEX_FILE_PATTERN     "*" IINDEX_EXT_NAME
#define IINDEX_MAX_KEY_SIZE     256
//Block size one block can contain how many items:
#define IINDEX_BLOCK_SIZE       4 //1024*1024*4
#define IINDEX_BLOCK_BASE       4
#define IINDEX_MAX_BLOCK_COUNT  7
#define IINDEX_MAX_CACHE_SIZE   4 //1024*4
#define IINDEX_SPLIT_COUNT      3
#define IINDEX_MAX_OPENED_INDEX 4

#define IINDEX_MAGIC_FLAG       (uint64_t *) "iIdxFile"
#define IINDEX_FILE_VER         0                       //the current IndexFile version
#define IINDEX_DELETED_FLAG     1

#define IINDEX_ERR_ALREADY_OPENED   -1
#define IINDEX_ERR_INVALID_FILE     -2
#define IINDEX_ERR_CANT_READ_FILE   -3
#define IINDEX_ERR_MAGIC_FLAG_FILE  -4
#define IINDEX_ERR_BLOCKCOUNT_FILE  -5
#define IINDEX_ERR_FILE_NAME_NONE   -6
#define IINDEX_ERR_FILE_CLOSED      -7
#define IINDEX_ERR_NOT_FOUND        -8
#define IINDEX_ERR_PATH_IS_FILE     -9
#define IINDEX_ERR_DELETED          -10

// the value is 128 bit
struct iIndexDBValue {
    uint32_t partitionLevel;//the partition key level
    uint64_t count;         //the key's subkey count.
    uint32_t reserved;
};
typedef struct iIndexDBValue    IndexDBValue;
typedef IndexDBValue*           PIndexDBValue;

struct iIndexDBItem {
    char                key[IINDEX_MAX_KEY_SIZE];
    IndexDBValue        value;
    char                isDeleted;
    char                reserved;
    uint16_t            reserved2;
};
typedef struct iIndexDBItem IndexDBItem;

typedef darray(IndexDBItem)      DarrayIndexItems;

struct iIndexDBBlockHeader {
    uint32_t    count;              //the current used items count
    uint32_t    id;                 //the real block index id on the disk
    //char        isFull;             //the block is full or not
    //char        reserved;
    //uint16_t    reserved2;
    char        maxKey[IINDEX_MAX_KEY_SIZE]; //which key is the max key of this block
};
typedef struct iIndexDBBlockHeader  IndexDBBlockHeader;
typedef IndexDBBlockHeader*         PIndexDBBlockHeader;

/*
struct iIndexDBBlock {
    //uint32_t            maxCount;                         //the block can hold how many items: the BlockBase^n*BlockSize
    bool                isFull;
    uint32_t            count;                              //the current used items count.
    char                maxKey[IINDEX_MAX_KEY_SIZE];        //which key is the max key of this block
    IndexDBItem         *items;
};
typedef struct iIndexDBBlock    IndexDBBlock;
typedef IndexDBBlock*           PIndexDBBlock;
typedef darray(IndexDBBlock)    DarrayIndexBlocks;
*/

struct iIndexDBFileHeader {
  uint64_t    magicFlag;                              //"iIdxFile"
  uint32_t    version;
  uint32_t    reserved;
  char        isFull;
  char        reserved2;
  uint16_t    reserved3;
  uint32_t    maxBlockCount;                          //this index file can hold how many blocks maximum.
  uint32_t    blockCount;                             //current used count of blocks
  uint32_t    blockSize;                              //a block can hold how many items, the default is IINDEX_BLOCK_SIZE
  char        maxKey[IINDEX_MAX_KEY_SIZE];            //specified which is the max key of the index file.
  //IndexDBBlockHeader[0..maxBlockCount-1];
};
typedef struct iIndexDBFileHeader   IndexDBFileHeader;
typedef IndexDBFileHeader*          PIndexDBFileHeader;

struct iIndexDBFile {
    bool                opened;                                 //the indexFile is opened or not.
    int                 fd;                                     //the file descriptor
    //uint32_t            maxBlockCount;                          //the index file can hold how many blocks.
    IndexDBFileHeader   header;
    uint32_t            headerSize;                             //the header's size of this index file.
    uint32_t            count;                                  //the index file total items count.
    sds                 path;                                   //the index file path.
    char                maxKey[IINDEX_MAX_KEY_SIZE];            //specified which is the max key of the index file.
    //DarrayIndexBlocks   blocks;                                 //hold the blocks in the file.
    PIndexDBBlockHeader  blockHeaders;                          //hold the block headers in the file. blockHeaders[0..MaxBlockCount-1]
};
typedef struct iIndexDBFile     IndexDBFile;
typedef IndexDBFile*            PIndexDBFile;
typedef darray(PIndexDBFile)    DarrayIndexFiles;

struct iIndexDB {
    bool                opened;                         //the indexDB is opened or not.
    sds                 path;                          //the index database path.
    DarrayIndexFiles    indexes;                        //the index files.
    DarrayIndexItems    *cache;                         //first insert here, write to disk when cache full
    DarrayIndexItems    *cacheSaving;                   //swap cacheSaving and cache, save cacheSaving when cache is full
    void                *indexesOpened;                 //the opened indexes.

    pthread_mutex_t     *c_lock;
    pthread_t           *thread_id;
    pthread_attr_t      thread_attr;
};
typedef struct iIndexDB         IndexDB;

struct iIndexDBIter {
    IndexDB     *db;
    int         fd;
    uint32_t    fileId;
    uint32_t    blockId;
    uint32_t    itemId;
    sds         keyPattern; //keyPattern is null match every thing.
};

 #ifdef __cplusplus
 extern "C"
  {
 #endif

IndexDB *IndexDB_New();
//init the IndexDB.
void IndexDB_Init(IndexDB *self);
int IndexDB_Open(IndexDB *self);
//flush/write cache to disk.
void IndexDB_Flush(IndexDB *self);
//if error, see errno
PIndexDBValue IndexDB_Get(IndexDB *self, const char *aKey);
void IndexDB_Put(IndexDB *self, const char *aKey, const size_t aKeySize, const IndexDBValue aValue);
DarrayIndexItems *IndexDB_List(IndexDB *self, const uint32_t aSkipCount, const uint32_t aCount);
void IndexDB_Del(IndexDB *self, const char *aKey, const size_t aKeySize);
void IndexDB_Close(IndexDB *self);
void IndexDB_Free(IndexDB *self);
ssize_t IndexDB_GetIndexFileId(IndexDB *self, const char *aKey);
PIndexDBFile IndexDB_GetIndexFile(IndexDB *self, const char *aKey);

PIndexDBFile IndexDBFile_New();
void IndexDBFile_Init(PIndexDBFile self);
int IndexDBFile_Open(PIndexDBFile self);
void IndexDBFile_Close(PIndexDBFile self);
void IndexDBFile_Free(PIndexDBFile self);
ssize_t IndexDBFile_GetBlockId(PIndexDBFile self, const char *aKey);
PIndexDBValue IndexDBFile_Get(PIndexDBFile self, const char *aKey);

static inline int iIndexBlockSize(int n)
{
    #if IINDEX_BLOCK_BASE == 2
    return ipow2(n)*IINDEX_BLOCK_SIZE;
    #elif IINDEX_BLOCK_BASE == 4
    return ipow2(2*n)*IINDEX_BLOCK_SIZE;
    #else
    return ipow(IINDEX_BLOCK_BASE, n)*IINDEX_BLOCK_SIZE;
    #endif
}

 #ifdef __cplusplus
 }
 #endif

#endif
