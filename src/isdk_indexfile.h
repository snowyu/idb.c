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
 *      Block(n): a Level of blocks. Size(n) = BlockBase^n
 *              one level can contain how many items, n is level number(0..MaxBlock-1) :
 *                  BlockBase^n*BlockSize
 *              the block(0) is in the memory, and block(1..MaxBlock-1) is on the disk.
 *      MaxBlock: the index can hold how many blocks.
 *      BlockBase: must be 2^n, 2, 4, 8, the default is 4.
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
//#include <stddef.h>

#define IINDEX_EXT_NAME ".idx"
#define IINDEX_MAX_KEY_SIZE 256
//Block size one block can contain how many items:
#define IINDEX_BLOCK_SIZE 4*256
#define IINDEX_BLOCK_BASE 4
#define IINDEX_MAX_BLOCK  4


// the value is 128 bit
struct iIndexValue {
    uint32_t partitionLevel; //the partition key level
    uint64_t count;          //the key's subkey count.
    uint32_t reserved;
};

struct iIndexItem {
    char                key[IINDEX_MAX_KEY_SIZE];
    struct iIndexValue  value;
};
typedef darray(struct iIndexItem)      DarrayIndexItems;

struct iIndexBlock {
    int                 isFull;
    uint32_t            maxCount;                       //the block can hold how many items: the BlockBase^n*BlockSize
    char                maxKey[IINDEX_MAX_KEY_SIZE];    //which key is the max key of this level
    DarrayIndexItems    *items;
};
typedef darray(struct iIndexBlock)      DarrayIndexBlocks;

struct iIndexFile {
    int                 fd;                                     //the file descriptor
    uint32_t            count;                                  //the index file total items count.
    char                *path;                                  //the index file path.
    char                maxKey[IINDEX_MAX_KEY_SIZE];            //specified which is the max key of the index file.
    DarrayIndexBlocks   blocks;                                 //the block0 is always in memory.
};
typedef struct iIndexFile IndexFile;

typedef darray(IndexFile)           DarrayIndexFiles;

struct iIndexDB {
    char                *path;                          //the index database path.
    DarrayIndexFiles    Indexes;                        //the index files.
    DarrayIndexItems    cache;                          //first insert here, write to disk when cache full
    DarrayIndexItems    cacheSaving;                    //swap cacheSaving and cache, save cacheSaving when cache is full

};
typedef struct iIndexDB IndexDB;

 #ifdef __cplusplus
 extern "C"
  {
 #endif

IndexDB *IndexDB_Open(char *aPath);
//flush/write cache to disk.
void IndexDB_Flush(IndexDB *aDB);
//if error, see errno
int64_t IndexDB_Get(IndexDB *aDB, const char *aKey, const uint8_t aKeySize);
int IndexDB_Set(IndexDB *aDB, const char *aKey, const uint8_t aKeySize, const uint64_t aValue);
DarrayIndexItems *IndexDB_List(IndexDB *aDB, const uint32_t aSkipCount, const uint32_t aCount);
void IndexDB_Close(IndexDB *aDB);
void IndexDB_Free(IndexDB *aDB);


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
