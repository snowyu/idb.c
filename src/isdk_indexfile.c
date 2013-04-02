/*
 * =====================================================================================
 *
 *       Filename:  isdk_indexfile.c
 *
 *    Description:  maintain the index on disk.
 *
 *        Version:  1.0
 *        Created:  2013/03/29
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Riceball LEE(snowyu.lee@gmail.com)
 *        Company:  
 *
 * =====================================================================================
 */


#include "isdk_indexfile.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

//the internal structures should be in isdk_indexfile.c

/*
PIndexDBBlock IndexDBBlock_New()
{
    PIndexDBBlock result = zmalloc(sizeof(IndexDBBlock));
    result->isFull = false;
    result->maxKey[0] = 0;
    result->items = NULL;
    return result;
}

void IndexDBBlock_Free(PIndexDBBlock self) {
    if (self->items) zfree(self->items);
    zfree(self);
}
*/

#define INDEXDB_FILE_BLOCK_SIZE(self) (self->header.blockSize * sizeof(IndexDBItem))
#define INDEXDB_FILE_SIZEOF_HEAD(maxBlockCount) sizeof(struct iIndexDBFileHeader) + maxBlockCount * (sizeof(IndexDBBlockHeader))

uint32_t IndexDBFile_SizeOfHeader(PIndexDBFile self)
{
    if (self->headerSize == 0)
        self->headerSize = INDEXDB_FILE_SIZEOF_HEAD(self->header.maxBlockCount);
    return self->headerSize;
}

//static void darray_iBlock_free_handler(IndexDBBlock aBlock)
//{
//    if (aBlock.items) zfree(aBlock.items);
//}

static inline off_t IndexDBFile_CalcBlockOffset(PIndexDBFile self, uint32_t aBlockId) {
    return IndexDBFile_SizeOfHeader(self) + aBlockId * INDEXDB_FILE_BLOCK_SIZE(self);
}

static inline bool IndexDBFile_IsFull(PIndexDBFile self) {
    return self->count >= self->header.blockCount*self->header.blockSize;
}

void IndexDBFile_Init(PIndexDBFile self) {
    self->fd = -1;
    self->header.magicFlag    = *IINDEX_MAGIC_FLAG;
    self->header.version      = IINDEX_FILE_VER;
    self->header.reserved     = 0;
    self->header.maxBlockCount= IINDEX_MAX_BLOCK_COUNT;
    self->header.blockCount   = 0;
    self->header.blockSize    = IINDEX_BLOCK_SIZE;
    self->header.maxKey[0]    = 0;
    self->headerSize = 0;
    self->count = 0;
    self->path = NULL;
    self->maxKey[0] = 0;
    self->blockHeaders = NULL;
    //darray_init(result->blocks);
    //result->blocks.onFree = (DarrayFreeHandler) darray_iBlock_free_handler;
}

PIndexDBFile IndexDBFile_New()
{
    PIndexDBFile result = zmalloc(sizeof(IndexDBFile));
    IndexDBFile_Init(result);
    return result;
}

void IndexDBFile_Free(PIndexDBFile self)
{
    IndexDBFile_Close(self);
    if (self->blockHeaders) zfree(self->blockHeaders);
    zfree(self);
}

static inline void IndexDBFileHeader_ConvertEndianIfBe(PIndexDBFileHeader self)
{
    SetIntRev32IfBe(self->version);
    SetIntRev32IfBe(self->maxBlockCount);
    SetIntRev32IfBe(self->blockCount);
    SetIntRev32IfBe(self->blockSize);
}
static inline void IndexDBBlockHeader_ConvertEndianIfBe(PIndexDBBlockHeader self)
{
    SetIntRev32IfBe(self->count);
}

static inline void IndexDBFile_BlocksConvertEndianIfBe(PIndexDBFile self)
{
#if (BYTE_ORDER == BIG_ENDIAN)
    uint32_t i;
    for (i = 0; i < self->header.blockCount; i++) {
        IndexDBBlockHeader_ConvertEndianIfBe(self->blockHeaders[i]);
    }
#endif
}

int IndexDBFile_Open(PIndexDBFile self) {
    IndexDBFileHeader vHeader;
    IndexDBBlockHeader *vBlockHeaders;
    off_t vSize, vFileSize;
    uint32_t i, count;
    int result = self->opened;
    if (!self->path) return IINDEX_ERR_FILE_NAME_NONE;
    if (!result) {
        self->fd = open_or_create_file(self->path, O_RDWR, O_RW_RW_R__PERMS);
        if (self->fd >= 0) {
            vFileSize = lseek(self->fd, 0L, SEEK_END);
            if (vFileSize && vFileSize > sizeof(IndexDBFileHeader)) {
                lseek(self->fd, 0L, SEEK_SET);
                vSize = read(self->fd, &self->header, sizeof(self->header));
                if (vSize != sizeof(self->header)) {
                    result = IINDEX_ERR_CANT_READ_FILE;
                    goto OpenFailed;
                }
                if (self->header.magicFlag != *IINDEX_MAGIC_FLAG) {
                    result = IINDEX_ERR_MAGIC_FLAG_FILE;
                    goto OpenFailed;
                }
                IndexDBFileHeader_ConvertEndianIfBe(&self->header);
                if (self->header.blockCount) {
                    count = self->header.blockCount*sizeof(IndexDBBlockHeader);
                    if (vFileSize < count + sizeof(self->header)) {
                        result = IINDEX_ERR_BLOCKCOUNT_FILE;
                        goto OpenFailed;
                    }
                    vBlockHeaders = zcalloc(self->header.maxBlockCount*sizeof(IndexDBBlockHeader));
                    vSize = read(self->fd, vBlockHeaders, count);
                    if (vSize != count) {
                        result = IINDEX_ERR_CANT_READ_FILE;
                        goto OpenFailed;
                    }
                    self->blockHeaders = vBlockHeaders;
                    IndexDBFile_BlocksConvertEndianIfBe(self);
                }
            }
            else {
                self->blockHeaders = zcalloc(self->header.maxBlockCount*sizeof(IndexDBBlockHeader));
                if (vFileSize)
                    warnx("%s is not a valid IndexDBFile, treat it as empty.", self->path);
            }
        }
        else {
            result = errno;
        }
        if (result == 0) self->opened = true;
    }
    else {
        result = IINDEX_ERR_ALREADY_OPENED;
    }
    return result;

OpenFailed:
    if (self->fd >= 0) {
        close(self->fd);
        self->fd = -1;
    }
    self->opened = false;
    return result;
}


void IndexDBFile_Close(PIndexDBFile self) {
    if (self->opened) {
        self->opened = false;
    }

}

ssize_t IndexDBFile_GetBlockId(PIndexDBFile self, const char *aKey)
{
    ssize_t result;
    for (result = 0; result < self->header.blockCount; result++) {
        //aKey <= blocks[i].maxKey
        if (strcmp(aKey, self->blockHeaders[result].maxKey) <= 0)
            return result;
    }
    //aKey > all blocks.maxKey
    result--;
    return result;
}

//internal method
bool IndexDBFile_WriteBlockHeades(PIndexDBFile self)
{
    off_t vOffset, vSize;

    if (self->fd < 0) self->fd = open(self->path, O_RDWR);
    if (self->fd >= 0) {
        vOffset = sizeof(IndexDBFileHeader);
        vSize = lseek(self->fd, vOffset, SEEK_SET);
        if (vSize == vOffset) {
            vSize = self->header.maxBlockCount * sizeof(IndexDBBlockHeader);
            IndexDBFile_BlocksConvertEndianIfBe(self);
            vOffset = write(self->fd, self->blockHeaders, vSize);
            IndexDBFile_BlocksConvertEndianIfBe(self);
            assert(vOffset == vSize);
            if (vOffset == vSize) return true;
        }
    }
    return false;
}
bool IndexDBFile_WriteBlockHeader(PIndexDBFile self, const uint32_t aBlockId)
{
    off_t vOffset, vSize;

    if (self->fd < 0) self->fd = open(self->path, O_RDWR);
    if (self->fd >= 0) {
        vOffset = sizeof(IndexDBFileHeader)+aBlockId*sizeof(IndexDBBlockHeader);
        vSize = lseek(self->fd, vOffset, SEEK_SET);
        if (vSize == vOffset) {
            vSize = sizeof(IndexDBBlockHeader);
            IndexDBBlockHeader_ConvertEndianIfBe(self->blockHeaders[aBlockId]);
            vOffset = write(self->fd, &self->blockHeaders[aBlockId], vSize);
            IndexDBBlockHeader_ConvertEndianIfBe(self->blockHeaders[aBlockId]);
            assert(vOffset == vSize);
            if (vOffset == vSize) return true;
        }
    }
    return false;
}

bool IndexDBFile_WriteBlock(PIndexDBFile self, const uint32_t aBlockId, IndexDBItem *aBlock)
{
    uint32_t vBlockSize;
    off_t vOffset, vSize;

    if (self->fd < 0) self->fd = open(self->path, O_RDWR);
    if (self->fd >= 0) {
        vOffset = IndexDBFile_CalcBlockOffset(self, aBlockId);
        vBlockSize = INDEXDB_FILE_BLOCK_SIZE(self);
        vSize = lseek(self->fd, vOffset, SEEK_SET);
        if (vSize == vOffset) {
            vSize = write(self->fd, aBlock, vBlockSize);
            if (vSize == vBlockSize) {
                assert(vOffset == vSize);
                return IndexDBFile_WriteBlockHeader(self, aBlockId);
            }
        }
    }
    return false;
}

IndexDBItem *IndexDBFile_ReadBlock(PIndexDBFile self, const uint32_t aBlockId)
{
    IndexDBItem *vBlock = NULL;
    uint32_t vBlockSize;
    off_t vOffset, vSize;

    if (self->fd < 0) self->fd = open(self->path, O_RDWR);
    if (self->fd >= 0) {
        vOffset = IndexDBFile_CalcBlockOffset(self, aBlockId);
        //vSize = lseek(self->fd, 0L, SEEK_END);
        vBlockSize = INDEXDB_FILE_BLOCK_SIZE(self);
        //if (vSize >= vOffset + vBlockSize) {
        vSize = lseek(self->fd, vOffset, SEEK_SET);
        if (vSize == vOffset) {
            vBlock = zcalloc(vBlockSize);
            vSize = read(self->fd, vBlock, vBlockSize);
            if (vSize == vBlockSize) {
                return vBlock;
            }
            else {
                zfree(vBlock);
                vBlock = NULL;
            }
        }
    }
    return vBlock;
}
IndexDBValue IndexDBFile_GetOnBlock(PIndexDBFile self, const uint32_t aBlockId, const char *aKey) {
    IndexDBValue result = {0,0,0};
    uint32_t vBlockSize, i;
    off_t vOffset, vSize;
    IndexDBItem *vBlock;

    //if (self->opened) {
        errno = 0;
        vBlock = IndexDBFile_ReadBlock(self, aBlockId);
        if (vBlock) {
            for (i = 0; i < self->header.blockSize; i++) {
                if (strcmp(vBlock[i].key, aKey) == 0) {
                    zfree(vBlock);
                    return vBlock[i].value;
                }
            }
            zfree(vBlock);
            errno = IINDEX_ERR_NOT_FOUND;
        }
        errno = IINDEX_ERR_CANT_READ_FILE;
    //}
    //else
    //    errno = IINDEX_ERR_FILE_CLOSED;

    return result;
}

IndexDBValue IndexDBFile_Get(PIndexDBFile self, const char *aKey)
{
    IndexDBValue result = {0,0,0};
    ssize_t vBlockId;

    if (self->opened) {
        errno = 0;
        vBlockId = IndexDBFile_GetBlockId(self, aKey);
        if (vBlockId != -1)
            result = IndexDBFile_GetOnBlock(self, vBlockId, aKey);
        else
            errno = IINDEX_ERR_NOT_FOUND;
    }
    else
        errno = IINDEX_ERR_FILE_CLOSED;

    return result;
}

ssize_t IndexDBFile_NewBlock(PIndexDBFile self)
{
    ssize_t result = self->header.blockCount;
    if (result < self->header.maxBlockCount) {
        bzero(&self->blockHeaders[result],sizeof(IndexDBBlockHeader));
        self->header.blockCount++;
    }
    else
        result = -1;
    return result;
}

bool IndexDBFile_SaveItem(PIndexDBFile self, IndexDBItem *aItem)
{
    ssize_t vBlockId;
    int cmp;
    IndexDBItem *vBlock;
    int i;

    vBlockId = IndexDBFile_GetBlockId(self, aItem->key);
    if (vBlockId == -1) {
        vBlockId = IndexDBFile_NewBlock(self);
        if (vBlockId == -1) return false;
        vBlock = zcalloc(INDEXDB_FILE_BLOCK_SIZE(self));
    }
    else {
        vBlock = IndexDBFile_ReadBlock(self, vBlockId);
        if (!vBlock) return false;
    }

    for (i = 0; i < self->blockHeaders[vBlockId].count; i++) {
        cmp = strcmp(aItem->key, vBlock[i].key);
        if (cmp == 0) {
            if (aItem->value.flags == IINDEX_DELETED_FLAG) {
                memmove(vBlock+i, vBlock+i+1, (self->header.blockSize-i-1) * sizeof(IndexDBItem));
                self->blockHeaders[vBlockId].count--;
                break;
            }
        }
        else if (cmp < 0) {
            memmove(vBlock+i+1, vBlock+i, (self->header.blockSize-i) * sizeof(IndexDBItem));
            vBlock[i] = *aItem;
            self->blockHeaders[vBlockId].count++;
            break;
        }
    }
    if (cmp > 0) {
        vBlock[i] = *aItem;
        self->blockHeaders[vBlockId].count++;
    }
    if (self->blockHeaders[vBlockId].count >= self->header.blockSize) {
        return IndexDBFile_SplinterBlock(self, vBlockId, vBlock);
    }
    else
        return IndexDBFile_WriteBlock(self, vBlockId, vBlock);
}

static void darray_iIndex_free_handler(PIndexDBFile aPtr)
{
    if (aPtr) IndexDBFile_Free(aPtr);
}

void IndexDB_Init(IndexDB *self)
{
    self->path = NULL;
    self->opened = false;
    darray_empty(self->cache);
    darray_empty(self->cacheSaving);
}

IndexDB *IndexDB_New()
{
    IndexDB *result = zmalloc(sizeof(IndexDB));
    result->c_lock = zmalloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(result->c_lock, NULL);
	pthread_attr_init(&result->thread_attr);
	pthread_attr_setdetachstate(&result->thread_attr, PTHREAD_CREATE_DETACHED);
    result->thread_id = 0;
    darray_init(result->indexes);
    result->indexes.onFree = (DarrayFreeHandler) darray_iIndex_free_handler;
    darray_init(result->cache);
    darray_growalloc(result->cache, IINDEX_BLOCK_SIZE);
    darray_init(result->cacheSaving);
    darray_growalloc(result->cacheSaving, IINDEX_BLOCK_SIZE);
    IndexDB_Init(result);
    return result;
}

void IndexDB_Free(IndexDB *self)
{
    IndexDB_Close(self);
    darray_free_all(self->indexes);
    darray_free(self->cache);
    darray_free(self->cacheSaving);
    zfree(self->c_lock);
    if (self->path) sdsfree(self->path);
    zfree(self);
}

static ssize_t _WalkDir_OnIndexFile(size_t aCount, const char *aDir, const Dirent *aItem, IndexDB *aDB)
{
    return WALK_ITEM_OK;
}

//0=successful.
int IndexDB_Open(IndexDB *self)
{
    int result = self->opened;
    if (!self->path) return IINDEX_ERR_FILE_NAME_NONE;
    if (!result) {
        result = WalkDir(self->path, IINDEX_FILE_PATTERN, LIST_FILES, 0, 0, (WalkDirHandler) _WalkDir_OnIndexFile, self);
        if (result != -1) {
            self->opened = true;
            result = 0;
        }
        else {
            SDSFreeAndNil(self->path);
            result = errno;
        }
    }
    else {
        result = IINDEX_ERR_ALREADY_OPENED;
    }
    return result;
}

void IndexDB_Close(IndexDB *self)
{
    PIndexDBFile *vIndex;
    if (self->opened) {
        //
        self->opened = false; //disable all operations first.
        IndexDB_Flush(self);
        darray_foreach(vIndex, self->indexes) {
            IndexDBFile_Close(*vIndex);
        }
    }
}


static inline sds IndexDB_GenIndexFileName(IndexDB *self)
{
    sds result = sdsalloc(NULL, 10);
    result = sdsprintf(result, "%08zu%s", darray_size(self->indexes), IINDEX_EXT_NAME);
    return result;
}

ssize_t IndexDB_GetIndexFileId(IndexDB *self, const char *aKey)
{
    ssize_t result;
    PIndexDBFile vIndex = NULL;
    for (result = 0; result < darray_size(self->indexes); result++) {
        vIndex = darray_item(self->indexes, result);
        if (strcmp(aKey, vIndex->maxKey) <= 0)
            //aKey <= vIndex.maxKey
            return result;
    }
    result--;
/*
    if (vIndex == NULL) {
        vIndex = IndexDBFile_New();
        vIndex->path = IndexDB_GenIndexFileName(self);
        darray_append(self->indexes, vIndex);
    }
*/
    return result;
}

PIndexDBFile IndexDB_GetIndexFile(IndexDB *self, const char *aKey)
{
    ssize_t i;
    i = IndexDB_GetIndexFileId(self, aKey);
    if (i != -1)
        return darray_item(self->indexes, i);
    else
        return NULL;
}

IndexDBValue IndexDB_Get(IndexDB *self, const char *aKey)
{
    IndexDBValue result = {0,0,0};
    PIndexDBFile vIndex;
    vIndex = IndexDB_GetIndexFile(self, aKey);
    if (vIndex)
        result = IndexDBFile_Get(vIndex, aKey);
    else
        errno = IINDEX_ERR_NOT_FOUND;
    return result;
}

static inline void IndexDBKey_StrCopy(const char *aSrc, size_t aSrcSize, char *aDest) {
    if (aSrcSize > IINDEX_MAX_KEY_SIZE-1) aSrcSize = IINDEX_MAX_KEY_SIZE-1;
    memcpy(aDest, aSrc, aSrcSize);
    aDest[aSrcSize] = '\0';
}
void IndexDB_PutInCache(IndexDB *self, const char *aKey, size_t aKeySize, const IndexDBValue aValue)
{
    IndexDBItem vItem;
    IndexDBKey_StrCopy(aKey, aKeySize, vItem.key);
    vItem.value = aValue;
    darray_append(self->cache, vItem);
}

static inline void IndexDB_SwapCache(IndexDB *self) {
    DarrayIndexItems t;
    t = self->cache;
    self->cache = self->cacheSaving;
    self->cacheSaving = t;
}

ssize_t IndexDB_AddIndexFile(IndexDB *self, PIndexDBFile aIndex)
{
    ssize_t result;
    int cmp;
    for (result = 0; result < darray_size(self->indexes); result++) {
        cmp = strcmp(aIndex->header.maxKey, darray_item(self->indexes, result)->header.maxKey);
        if (cmp < 0) {
            darray_insert(self->indexes, result, aIndex);
            return result;
        } else if (cmp == 0){
            result == -1;
            break;
        }
    }
    return result;
}

PIndexDBFile IndexDB_NewIndexFile(IndexDB *self, const char *aKey)
{
    PIndexDBFile vIndex;
    vIndex = IndexDBFile_New();
    vIndex->path = IndexDB_GenIndexFileName(self);
    IndexDBKey_StrCopy(aKey, IINDEX_MAX_KEY_SIZE, vIndex->header.maxKey);
    IndexDB_AddIndexFile(self, vIndex);
    return vIndex;
}

void IndexDB_SaveItem(IndexDB *self, IndexDBItem *aItem) {
    PIndexDBFile vIndex;
    vIndex = IndexDB_GetIndexFile(self, aItem->key);
    if (vIndex == NULL) vIndex = IndexDB_NewIndexFile(self, aItem->key);
    if (!vIndex->opened) IndexDBFile_Open(vIndex);
    IndexDBFile_SaveItem(vIndex, aItem);
    if (IndexDBFile_IsFull(vIndex))
        IndexDB_SplinterIndexFile(self, vIndex);
}

void IndexDB_SaveItems(IndexDB *self, DarrayIndexItems aCache)
{
    int i;
    for (i = 0; i < darray_size(aCache); i++) {
        IndexDB_SaveItem(self, &darray_item(aCache, i));
    }
    darray_empty(aCache);
}

void *IndexDB_DoSaveOnThread(IndexDB *self)
{
    IndexDB_SaveItems(self, self->cacheSaving);
	pthread_detach(pthread_self());
    pthread_mutex_lock(self->c_lock);
    self->thread_id = 0;
    pthread_mutex_unlock(self->c_lock);
	pthread_exit(NULL);
}

typedef void*(*ThreadProcessor)(void* arg);
void IndexDB_BgSave(IndexDB *self)
{
    if (pthread_create(&self->thread_id, &self->thread_attr, (ThreadProcessor)IndexDB_DoSaveOnThread, self) != 0) {
        //error, can not create thread
        self->thread_id = 0;
    }
}

void IndexDB_Put(IndexDB *self, const char *aKey, const size_t aKeySize, const IndexDBValue aValue)
{
    IndexDB_PutInCache(self, aKey, aKeySize, aValue);
    if (!self->thread_id && darray_size(self->cache) >= IINDEX_MAX_CACHE_SIZE) {
        pthread_mutex_lock(self->c_lock);
        IndexDB_SwapCache(self);
        IndexDB_BgSave(self);
        pthread_mutex_unlock(self->c_lock);
    }
}

void IndexDB_Flush(IndexDB *self)
{
    while (self->thread_id) sleep(10);
    IndexDB_SaveItems(self, self->cache);
}

#ifdef ISDK_INDEXFILE_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"

typedef darray(IndexDBValue)      DarrayIndexValues;
#define c64ifbe(p) *(uint64_t *) p = SWAB64(*(uint64_t *) p)

//gcc --std=c99 -I. -Ideps  -o test -DISDK_INDEXFILE_TEST_MAIN isdk_indexfile.c isdk_utils.c isdk_sds.c deps/sds.c deps/zmalloc.c deps/utf8proc.c
int main(int argc, char **argv)
{
    {
        //test_cond("darray value changed", memcmp(v, &darray_item(vItems, 0), sizeof(v)) == 0);
    }
    test_report();
    return 0;
}
#endif
