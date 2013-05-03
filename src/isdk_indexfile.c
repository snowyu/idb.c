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
#include "deps/klist.h"

//the single-linked list for opened inex files.
#define __none_free(x)
KLIST_INIT(IndexFile, PIndexDBFile, __none_free)

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

static inline void IndexDBKey_StrCopy(const char *aSrc, size_t aSrcSize, char *aDest) {
    if (aSrcSize > IINDEX_MAX_KEY_SIZE-1) aSrcSize = IINDEX_MAX_KEY_SIZE-1;
    memcpy(aDest, aSrc, aSrcSize);
    aDest[aSrcSize] = '\0';
}

static size_t IndexDBFile_SizeOfBlock(PIndexDBFile self, const uint32_t aBlockId)
{
    return self->blockHeaders[aBlockId].count * sizeof(IndexDBItem);
}

uint32_t IndexDBFile_SizeOfHeader(PIndexDBFile self)
{
    if (self->headerSize == 0)
        self->headerSize = INDEXDB_FILE_SIZEOF_HEAD(self->header.maxBlockCount);
    return self->headerSize;
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
        IndexDBBlockHeader_ConvertEndianIfBe(&self->blockHeaders[i]);
    }
#endif
}

bool IndexDBFile_WriteBlockHeaders(PIndexDBFile self)
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
            IndexDBBlockHeader_ConvertEndianIfBe(&self->blockHeaders[aBlockId]);
            vOffset = write(self->fd, &self->blockHeaders[aBlockId], vSize);
            IndexDBBlockHeader_ConvertEndianIfBe(&self->blockHeaders[aBlockId]);
            //printf("IndexDBFile_WriteBlockHeader blockId=%u, count=%d\n", aBlockId, self->blockHeaders[aBlockId].count);
            assert(vOffset == vSize);
            if (vOffset == vSize) return true;
        }
    }
    return false;
}

bool IndexDBFile_WriteFileHeader(PIndexDBFile self)
{
    off_t vOffset, vSize;

    if (self->fd < 0) self->fd = open(self->path, O_RDWR);
    if (self->fd >= 0) {
        lseek(self->fd, 0L, SEEK_SET);
        vSize = sizeof(IndexDBFileHeader);
        IndexDBFileHeader_ConvertEndianIfBe(&self->header);
        vOffset = write(self->fd, &self->header, vSize);
        IndexDBFileHeader_ConvertEndianIfBe(&self->header);
        return vOffset == vSize;
    }
    return false;
}
bool IndexDBFile_WriteHeader(PIndexDBFile self)
{
    bool result = IndexDBFile_WriteFileHeader(self);
    if (result) result = IndexDBFile_WriteBlockHeaders(self);
}
//static void darray_iBlock_free_handler(IndexDBBlock aBlock)
//{
//    if (aBlock.items) zfree(aBlock.items);
//}

static inline off_t IndexDBFile_CalcBlockOffset(PIndexDBFile self, uint32_t aBlockId) {
    return IndexDBFile_SizeOfHeader(self) + self->blockHeaders[aBlockId].id * INDEXDB_FILE_BLOCK_SIZE(self);
}

static inline bool IndexDBFile_IsFull(PIndexDBFile self) {
    return self->header.isFull;
    //return self->count >= self->header.blockCount*self->header.blockSize;
}

size_t IndexDBFile_GetCount(PIndexDBFile self)
{
    size_t result = 0;
    int i;

    for (i = 0; i < self->header.blockCount; i++)
    {
        result += self->blockHeaders[i].count;
    }
    return result;
}

void IndexDBFile_Init(PIndexDBFile self) {
    self->fd = -1;
    self->header.isFull = false;
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
    if (self->path) sdsfree(self->path);
    //if (self->blockHeaders) zfree(self->blockHeaders);
    zfree(self);
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
                    //printf("openfile(%s): blockCount=%d, count=%ld\n", self->path, self->header.blockCount, IndexDBFile_GetCount(self));
                }
                else {
                    self->blockHeaders = zcalloc(self->header.maxBlockCount*sizeof(IndexDBBlockHeader));
                }
            }
            else {
                self->blockHeaders = zcalloc(self->header.maxBlockCount*sizeof(IndexDBBlockHeader));
                IndexDBFile_WriteHeader(self);
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
    warnx("open failed:%s", self->path);
    if (self->fd >= 0) {
        close(self->fd);
        self->fd = -1;
    }
    self->opened = false;
    return result;
}

static inline void IndexDBFile_CloseFile(PIndexDBFile self)
{
    if (self->fd >= 0) {
        close(self->fd);
        self->fd = -1;
    }
}

void IndexDBFile_Close(PIndexDBFile self) {
    if (self->opened) {
        IndexDBFile_WriteHeader(self);
        //printf("CloseFile, blockCount=%d, count=%zu\n", self->header.blockCount, IndexDBFile_GetCount(self));
        self->opened = false;
    }
    IndexDBFile_CloseFile(self);
    if (self->blockHeaders) {
        zfree(self->blockHeaders);
        self->blockHeaders = NULL;
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

bool IndexDBFile_WriteBlock(PIndexDBFile self, const uint32_t aBlockId, DarrayIndexItems aBlock)
{
    uint32_t vBlockSize;
    off_t vOffset, vSize;

    if (self->fd < 0) self->fd = open(self->path, O_RDWR);
    if (self->fd >= 0) {
        vOffset = IndexDBFile_CalcBlockOffset(self, aBlockId);
        vBlockSize = IndexDBFile_SizeOfBlock(self, aBlockId); //INDEXDB_FILE_BLOCK_SIZE(self);
        vSize = lseek(self->fd, vOffset, SEEK_SET);
        if (vSize == vOffset) {
            vSize = write(self->fd, aBlock.item, vBlockSize);
            if (vSize == vBlockSize) {
                return IndexDBFile_WriteBlockHeader(self, aBlockId);
            }
        }
    }
    return false;
}

DarrayIndexItems IndexDBFile_ReadBlock(PIndexDBFile self, const uint32_t aBlockId)
{
    DarrayIndexItems vBlock;
    uint32_t vBlockSize;
    off_t vOffset, vSize;

    darray_init(vBlock);
    if (self->fd < 0) self->fd = open(self->path, O_RDWR);
    if (self->fd >= 0) {
        vOffset = IndexDBFile_CalcBlockOffset(self, aBlockId);
        //vSize = lseek(self->fd, 0L, SEEK_END);
        vBlockSize = IndexDBFile_SizeOfBlock(self, aBlockId);//INDEXDB_FILE_BLOCK_SIZE(self);
        //if (vSize >= vOffset + vBlockSize) {
        vSize = lseek(self->fd, vOffset, SEEK_SET);
        if (vSize == vOffset) {
            darray_resize(vBlock, self->blockHeaders[aBlockId].count);
            //vBlock = zmalloc(vBlockSize);
            vSize = read(self->fd, vBlock.item, vBlockSize);
            if (vSize == vBlockSize) {
                return vBlock;
            }
            else {
                darray_free(vBlock);
                warnx("expected count=%d, infact count=%d", self->blockHeaders[aBlockId].count, vSize / sizeof(IndexDBItem));
            }
        }
    }
    return vBlock;
}
PIndexDBValue IndexDBFile_GetOnBlock(PIndexDBFile self, const uint32_t aBlockId, const char *aKey) {
    PIndexDBValue result = NULL;
    uint32_t vBlockSize, i;
    off_t vOffset, vSize;
    DarrayIndexItems vBlock;

    //if (self->opened) {
        errno   = 0;
        vBlock  = IndexDBFile_ReadBlock(self, aBlockId);
        if (darray_size(vBlock)) {
            for (i = 0; i < self->header.blockSize; i++) {
                if (strcmp(vBlock.item[i].key, aKey) == 0) {
                    result = zmalloc(sizeof(IndexDBValue));
                    memcpy(result, &vBlock.item[i].value, sizeof(IndexDBValue));
                    //darray_free(vBlock);
                    //return result;
                }
            }
            darray_free(vBlock);
            if (!result)
                errno = IINDEX_ERR_NOT_FOUND;
        }
        else
            errno = IINDEX_ERR_CANT_READ_FILE;
    //}
    //else
    //    errno = IINDEX_ERR_FILE_CLOSED;

    return result;
}

PIndexDBValue IndexDBFile_Get(PIndexDBFile self, const char *aKey)
{
    PIndexDBValue result = NULL;
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

ssize_t IndexDBFile_NewBlock(PIndexDBFile self, const char *aKey)
{
    ssize_t result = self->header.blockCount;
    if (result < self->header.maxBlockCount) {
        //bzero(&self->blockHeaders[result],sizeof(IndexDBBlockHeader));
        result = IndexDBFile_GetBlockId(self, aKey);
        if (result == -1) result = 0;
        if (self->header.blockCount != 0)
            //insert here, so move others if any.
            memmove(self->blockHeaders+result+1, self->blockHeaders+result, (self->header.blockCount-result)*INDEXDB_FILE_BLOCK_SIZE(self));
        self->blockHeaders[result].id = self->header.blockCount;
        self->blockHeaders[result].count = 0;
        strcpy(self->blockHeaders[result].maxKey, aKey);
        self->header.blockCount++;
    }
    else
        result = -1;
    return result;
}

bool IndexDBFile_SplinterBlock(PIndexDBFile self, uint32_t aBlockId, DarrayIndexItems aBlock)
{
    bool result;
    int vSplit, vMod, i, vBlockId;//, vOldBlockCount;
    DarrayIndexItems vBlock;
    IndexDBBlockHeader  vBlockHeader;

    if (self->header.blockCount + IINDEX_SPLIT_COUNT-1 <= self->header.maxBlockCount)
    {
        vSplit  = self->blockHeaders[aBlockId].count / IINDEX_SPLIT_COUNT;
        vMod    = self->blockHeaders[aBlockId].count % IINDEX_SPLIT_COUNT;
        darray_init(vBlock);
        darray_resize(vBlock, vSplit);
        //vBlock  = zcalloc(INDEXDB_FILE_BLOCK_SIZE(self));
        //vOldBlockCount = self->header.blockCount;
        //maybe I should use the link-list in memory.
        memmove(self->blockHeaders+aBlockId+IINDEX_SPLIT_COUNT-1, self->blockHeaders+result, (self->header.blockCount-result)*INDEXDB_FILE_BLOCK_SIZE(self));

        //strcpy(self->blockHeaders[aBlockId].maxKey, aBlock[vSplit+vMod-1].key);
        for (i = 1; i < IINDEX_SPLIT_COUNT; i++) {
            vBlockId = aBlockId + i;
            self->blockHeaders[vBlockId].id = self->header.blockCount+i-1;
            self->blockHeaders[vBlockId].count = vSplit;
            strcpy(self->blockHeaders[vBlockId].maxKey, darray_item(aBlock, vSplit*(i+1)+vMod-1).key);
            memcpy(vBlock.item, aBlock.item+vSplit*i+vMod, vSplit*sizeof(IndexDBItem));
            result = IndexDBFile_WriteBlock(self, vBlockId, vBlock);
            if (!result) break;
        }
        darray_free(vBlock);
        assert(result);
        if (result) {
            self->header.blockCount += IINDEX_SPLIT_COUNT - 1;
            self->blockHeaders[aBlockId].count = vSplit + vMod;
            strcpy(self->blockHeaders[aBlockId].maxKey, aBlock.item[vSplit+vMod-1].key);
            result = IndexDBFile_WriteBlockHeader(self, aBlockId);
        }
        //else {
        //    self->header.blockCount = vOldBlockCount;
        //}
    }
    else {
        result = false;
    }
    if (self->header.blockCount + IINDEX_SPLIT_COUNT -1 > self->header.maxBlockCount)
        self->header.isFull = true;
    printf("SplinterBlock(%s):%d isFull=%d\n", self->path, result, self->header.isFull);
    return result;

}

bool IndexDBFile_SaveItem(PIndexDBFile self, IndexDBItem *aItem)
{
    bool result;
    ssize_t vBlockId;
    int cmp = 1;
    DarrayIndexItems vBlock;
    int i;

    vBlockId = IndexDBFile_GetBlockId(self, aItem->key);
    printf("%s:\n", self->path);
    printf("save key=%s, value=%s, BlockId=%zd, blockCount=%d\n", aItem->key, (char*)&aItem->value, vBlockId, self->header.blockCount);
    if (vBlockId == -1) {
        vBlockId = IndexDBFile_NewBlock(self, aItem->key);
        if (vBlockId == -1) return false;
        darray_init(vBlock);
        //darray_resize(vBlock, 1);
        //vBlock = zcalloc(sizeof(IndexDBItem));
    }
    else {
        vBlock = IndexDBFile_ReadBlock(self, vBlockId);
        assert(darray_size(vBlock) != 0 || self->blockHeaders[vBlockId].count == 0) ;
    }
    //printf("key=%s, isDeleted=%d, BlockId=%zd, id=%d, count=%d, blockSize=%d\n", aItem->key, aItem->isDeleted, vBlockId, self->blockHeaders[vBlockId].id, self->blockHeaders[vBlockId].count, self->header.blockSize);

    for (i = 0; i < self->blockHeaders[vBlockId].count; i++) {
        cmp = strcmp(aItem->key, darray_item(vBlock, i).key);
        //printf("%s cmp %s=%d\n", aItem->key, darray_item(vBlock,i).key, cmp);
        if (cmp == 0) {
            if (aItem->isDeleted) {
                darray_del(vBlock, i);
                self->blockHeaders[vBlockId].count--;
                break;
            }
            else {
                darray_item(vBlock, i).value = aItem->value;
            }
        }
        else if (cmp < 0 && !aItem->isDeleted) {
            //vBlock = zrealloc(vBlock, (self->blockHeaders[vBlockId].count+1)*sizeof(IndexDBItem));
            darray_insert(vBlock, i, *aItem);
            //memmove(vBlock+i+1, vBlock+i, (self->header.blockSize-i) * sizeof(IndexDBItem));
            //vBlock[i] = *aItem;
            //memcpy(vBlock+i, aItem, sizeof(IndexDBItem));
            self->blockHeaders[vBlockId].count++;
            break;
        }
    }
    if (cmp > 0 && !aItem->isDeleted) {
        //vBlock[i] = *aItem;
        //vBlock = zrealloc(vBlock, (self->blockHeaders[vBlockId].count+1)*sizeof(IndexDBItem));
        darray_append(vBlock, *aItem);
        //printf("append %s\n", darray_item(vBlock, darray_size(vBlock)-1).key);
        //memcpy(vBlock+i, aItem, sizeof(IndexDBItem));
        self->blockHeaders[vBlockId].count++;
        strcpy(self->blockHeaders[vBlockId].maxKey, aItem->key);
    }
    printf("     key=%s, isDeleted=%d, BlockId=%zd, id=%d, count=%d, blockSize=%d\n", aItem->key, aItem->isDeleted, vBlockId, self->blockHeaders[vBlockId].id, self->blockHeaders[vBlockId].count, self->header.blockSize);
    assert(self->header.blockSize != 0);
    if (!aItem->isDeleted && (strcmp(aItem->key, self->header.maxKey) > 0)) {
        IndexDBKey_StrCopy(aItem->key, strlen(aItem->key), self->header.maxKey);
        printf("header.maxKey=%s\n", self->header.maxKey);
    }
    //for (i=0; i<darray_size(vBlock); i++)
    //    printf("  key=%s\n", darray_item(vBlock, i).key);

    if (self->blockHeaders[vBlockId].count > self->header.blockSize) {
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
    darray_empty(*self->cache);
    darray_empty(*self->cacheSaving);
}

IndexDB *IndexDB_New()
{
    IndexDB *result = zmalloc(sizeof(IndexDB));
    result->c_lock = zmalloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(result->c_lock, NULL);
	pthread_attr_init(&result->thread_attr);
	pthread_attr_setdetachstate(&result->thread_attr, PTHREAD_CREATE_DETACHED);
    result->thread_id = NULL;
    darray_init(result->indexes);
    result->indexes.onFree = (DarrayFreeHandler) darray_iIndex_free_handler;
    result->indexesOpened = kl_init(IndexFile);
    result->cache = zcalloc(sizeof(DarrayIndexItems));
    //darray_init(result->cache);
    darray_growalloc(*result->cache, IINDEX_BLOCK_SIZE);
    result->cacheSaving = zcalloc(sizeof(DarrayIndexItems));
    //darray_init(result->cacheSaving);
    darray_growalloc(*result->cacheSaving, IINDEX_BLOCK_SIZE);
    IndexDB_Init(result);
    return result;
}

void IndexDB_Free(IndexDB *self)
{
    IndexDB_Close(self);
    darray_free_all(self->indexes);
    kl_destroy(IndexFile, self->indexesOpened);
    darray_free(*self->cache);
    darray_free(*self->cacheSaving);
    zfree(self->cache);
    zfree(self->cacheSaving);
    zfree(self->c_lock);
    if (self->path) sdsfree(self->path);
    zfree(self);
}

static ssize_t _WalkDir_OnIndexFile(size_t aCount, const char *aDir, const Dirent *aItem, IndexDB *aDB)
{
    return WALK_ITEM_OK;
}

static int __klist_can_del(PIndexDBFile aIndex) {
    return aIndex->fd < 0;
}
static inline void cleanClosedIndexes(klist_t(IndexFile) *aOpenedFiles) {
    kl_delex(IndexFile, aOpenedFiles, __klist_can_del);
}
static inline void IndexDB_OpenIndexFile(IndexDB *self, PIndexDBFile aIndex)
{
    PIndexDBFile vIndex;
    klist_t(IndexFile) *vOpenedFiles;
    vOpenedFiles = self->indexesOpened;

    while (kl_size(vOpenedFiles) >= IINDEX_MAX_OPENED_INDEX) {
        kl_shift(IndexFile, vOpenedFiles, &vIndex);
        if (aIndex != vIndex) IndexDBFile_CloseFile(vIndex);
        cleanClosedIndexes(vOpenedFiles);
    }
    IndexDBFile_Open(aIndex);
    kl_del(IndexFile, vOpenedFiles, aIndex);
    *kl_pushp(IndexFile, vOpenedFiles) = aIndex;
}

//0=successful.
int IndexDB_Open(IndexDB *self)
{
    int result = self->opened;
    if (!self->path) return IINDEX_ERR_FILE_NAME_NONE;
    if (!result) {
        result = DirectoryExists(self->path);
        if (result == PATH_IS_DIR) {
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
        else if (result == PATH_IS_NOT_EXISTS) {
            ForceDirectories(self->path, O_RWXRWXR_XPERMS);
            self->opened = true;
            result = 0;
        }
        else //PATH_IS_FILE
            return IINDEX_ERR_PATH_IS_FILE;
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
    sds result = sdsJoinPathLen(sdsdup(self->path), NULL, 0);
    result = sdscatprintf(result, "%08zu%s", darray_size(self->indexes), IINDEX_EXT_NAME);
    return result;
}

ssize_t IndexDB_GetIndexFileId(IndexDB *self, const char *aKey)
{
    ssize_t result;
    PIndexDBFile vIndex = NULL;
    for (result = 0; result < darray_size(self->indexes); result++) {
        vIndex = darray_item(self->indexes, result);
        printf("%s maxKey=%s\n", vIndex->path, vIndex->header.maxKey);
        if (strcmp(aKey, vIndex->header.maxKey) <= 0)
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

ssize_t IndexDB_GetIdInCache(IndexDB *self, const char *aKey, const DarrayIndexItems *aCache)
{
    ssize_t result;

    for (result = 0; result < darray_size(*aCache); result++) {
        if (strcmp(aKey, darray_item(*aCache, result).key) == 0) {
            return result;
        }
    }
    return -1;
}

PIndexDBValue IndexDB_GetInCache(IndexDB *self, const char *aKey)
{
    PIndexDBValue result = NULL;
    errno = 0;
    DarrayIndexItems *vCache = self->cache;

    ssize_t i = IndexDB_GetIdInCache(self, aKey, vCache);
    if (i != -1) {
        if (!darray_item(*vCache, i).isDeleted) {
            result = zmalloc(sizeof(IndexDBValue));
            memcpy(result, &darray_item(*vCache, i).value, sizeof(IndexDBValue));
            return result;
        }
        else
            errno = IINDEX_ERR_DELETED;
    }
    else if (self->thread_id) {
        vCache = self->cacheSaving;
        pthread_mutex_lock(self->c_lock);
        i = IndexDB_GetIdInCache(self, aKey, vCache);
        if (i != -1) {
            //printf("found value=%s\n", (char*) &darray_item(*vCache, i).value);
            if (!darray_item(*vCache, i).isDeleted) {
                result = zmalloc(sizeof(IndexDBValue));
                memcpy(result, &darray_item(*vCache, i).value, sizeof(IndexDBValue));
            }
            else
                errno = IINDEX_ERR_DELETED;
        }
        else
            errno = IINDEX_ERR_NOT_FOUND;
        pthread_mutex_unlock(self->c_lock);
    }
    else
        errno = IINDEX_ERR_NOT_FOUND;
    return result;
}

PIndexDBValue IndexDB_Get(IndexDB *self, const char *aKey)
{
    PIndexDBValue result = NULL;
    PIndexDBFile vIndex;
    errno = 0;
    result = IndexDB_GetInCache(self, aKey);
    if (!result && errno == IINDEX_ERR_NOT_FOUND) {
        vIndex = IndexDB_GetIndexFile(self, aKey);
        if (vIndex) {
            printf("finding %s, the maxKey=%s\n", vIndex->path, vIndex->header.maxKey);
            IndexDB_OpenIndexFile(self, vIndex);
            result = IndexDBFile_Get(vIndex, aKey);
        }
        else
            errno = IINDEX_ERR_NOT_FOUND;
    }

    return result;
}

void IndexDB_PutInCache(IndexDB *self, const char *aKey, size_t aKeySize,
        const IndexDBValue aValue, const char aIsDeleted)
{
    IndexDBItem vItem;
    ssize_t i = IndexDB_GetIdInCache(self, aKey, self->cache);
    if (i == -1) {
        IndexDBKey_StrCopy(aKey, aKeySize, vItem.key);
        memcpy(&vItem.value,  &aValue, sizeof(aValue));
        vItem.isDeleted = aIsDeleted;
        darray_append(*self->cache, vItem);
        assert(memcmp(&darray_item(*self->cache, darray_size(*self->cache)-1), &vItem, sizeof(vItem))==0);
        //printf("put %s\n", (char *)&darray_item(self->cache, darray_size(self->cache)-1).value);
        //printf("put %s isDeleted=%d\n", vItem.key, vItem.isDeleted);
    }
    else {
        if (aIsDeleted)
            darray_item(*self->cache, i).isDeleted = aIsDeleted;
        else
            darray_item(*self->cache, i).value = aValue;
    }
}

static inline void IndexDB_SwapCache(IndexDB *self) {
    DarrayIndexItems *t;
    t = self->cache;
    self->cache = self->cacheSaving;
    self->cacheSaving = t;
}

ssize_t IndexDB_AddIndexFile(IndexDB *self, PIndexDBFile aIndex)
{
    ssize_t result;
    int cmp = 1;
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
    if (cmp > 0) darray_append(self->indexes, aIndex);
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

void IndexDB_SplinterIndexFile(IndexDB *self, PIndexDBFile aIndex)
{
    uint32_t vSplit, vMod, i, j, vBlockId, vNewBlockId;
    PIndexDBFile vIndex;
    DarrayIndexItems vBlock;

    vSplit  = aIndex->header.blockCount / IINDEX_SPLIT_COUNT;
    vMod    = aIndex->header.blockCount % IINDEX_SPLIT_COUNT;
    printf("blockCount=%d, split=%d, mod=%d\n", aIndex->header.blockCount, vSplit, vMod);
    IndexDBKey_StrCopy(aIndex->blockHeaders[vSplit+vMod-1].maxKey, IINDEX_MAX_KEY_SIZE, aIndex->header.maxKey);
    printf("%s maxkey =%s\n", aIndex->path, aIndex->header.maxKey);
    for (i = 1; i < IINDEX_SPLIT_COUNT; i++) {
        //the last block for the new index file.
        vBlockId = vSplit*(i+1)+vMod-1;
        vIndex = IndexDB_NewIndexFile(self, aIndex->blockHeaders[vBlockId].maxKey);
        assert(vIndex);
        printf("%s lastBlock=%d, maxKey=%s\n", vIndex->path, vBlockId, aIndex->blockHeaders[vBlockId].maxKey);
        IndexDB_OpenIndexFile(self, vIndex);
        //back to the first block
        vBlockId = vBlockId-vSplit+1;
        for (j = 0; j < vSplit; j++) {
            printf("blockId=%d\n", vBlockId);
            vBlock  = IndexDBFile_ReadBlock(aIndex, vBlockId);
            assert(darray_size(vBlock));
            printf("    maxKey=%s\n", aIndex->blockHeaders[vBlockId].maxKey);
            vNewBlockId = IndexDBFile_NewBlock(vIndex, aIndex->blockHeaders[vBlockId].maxKey);
            vIndex->blockHeaders[vNewBlockId].count = vSplit;
            assert(IndexDBFile_WriteBlock(vIndex, vNewBlockId, vBlock));
            vBlockId++;
        }
        IndexDBFile_WriteFileHeader(vIndex);
    }
    aIndex->header.blockCount = vSplit + vMod;
    aIndex->header.isFull = false;
    IndexDBFile_WriteFileHeader(aIndex);
}

void IndexDB_SaveItem(IndexDB *self, IndexDBItem *aItem) {
    PIndexDBFile vIndex;
    vIndex = IndexDB_GetIndexFile(self, aItem->key);
    if (vIndex == NULL) vIndex = IndexDB_NewIndexFile(self, aItem->key);
    if (!vIndex->opened) {
        IndexDB_OpenIndexFile(self, vIndex);
    }
    IndexDBFile_SaveItem(vIndex, aItem);
    if (IndexDBFile_IsFull(vIndex))
        IndexDB_SplinterIndexFile(self, vIndex);
    //IndexDBFile_Close(vIndex);
}

void IndexDB_SaveItems(IndexDB *self, DarrayIndexItems *aCache)
{
    int i;
    printf("saveItems count:%lu\n", darray_size(*aCache));
    for (i = 0; i < darray_size(*aCache); i++) {
        IndexDB_SaveItem(self, &darray_item(*aCache, i));
    }
}

void *IndexDB_DoSaveOnThread(IndexDB *self)
{
    IndexDB_SaveItems(self, self->cacheSaving);
	pthread_detach(pthread_self());
    pthread_mutex_lock(self->c_lock);
    darray_clear(*self->cacheSaving);
    if (self->thread_id) {
        zfree(self->thread_id);
        self->thread_id = NULL;
    }
    pthread_mutex_unlock(self->c_lock);
    printf("bgSaving done.\n");
	pthread_exit(NULL);
}

typedef void*(*ThreadProcessor)(void* arg);
void IndexDB_BgSave(IndexDB *self)
{
    printf("bgSaving...\n");
    if (self->thread_id == NULL) {
        self->thread_id = zmalloc(sizeof(pthread_t));
        if (pthread_create(self->thread_id, &self->thread_attr, (ThreadProcessor)IndexDB_DoSaveOnThread, self) != 0) {
            //error, can not create thread, So saving directly
            warnx("can not create bg thread, saving directly");
            IndexDB_SaveItems(self, self->cacheSaving);
            zfree(self->thread_id);
            self->thread_id = NULL;
        }
    }
}

void IndexDB_Put(IndexDB *self, const char *aKey, const size_t aKeySize, const IndexDBValue aValue)
{
    IndexDB_PutInCache(self, aKey, aKeySize, aValue, false);
    //printf("put %lu, %d\n", darray_size(self->cache), IINDEX_MAX_CACHE_SIZE);
    if (!self->thread_id && darray_size(*self->cache) >= IINDEX_MAX_CACHE_SIZE) {
        pthread_mutex_lock(self->c_lock);
        IndexDB_SwapCache(self);
        pthread_mutex_unlock(self->c_lock);
        IndexDB_BgSave(self);
    }
}

void IndexDB_Del(IndexDB *self, const char *aKey, const size_t aKeySize)
{
    IndexDBValue v;
    IndexDB_PutInCache(self, aKey, aKeySize, v, true);
    if (!self->thread_id && darray_size(*self->cache) >= IINDEX_MAX_CACHE_SIZE) {
        pthread_mutex_lock(self->c_lock);
        IndexDB_SwapCache(self);
        pthread_mutex_unlock(self->c_lock);
        IndexDB_BgSave(self);
    }
}

ssize_t IndexDB_GetCount(IndexDB *self)
{
    if (self->opened) {
        //
    }
}

void IndexDB_Flush(IndexDB *self)
{
    DarrayIndexItems *vCache;
    //printf("%lu\n", self->thread_id);
    while (self->thread_id) sched_yield();
    vCache = self->cache;
    if (darray_size(*vCache)) {
        IndexDB_SaveItems(self, vCache);
        pthread_mutex_lock(self->c_lock);
        darray_clear(*vCache);
        pthread_mutex_unlock(self->c_lock);
    }
}

#ifdef ISDK_INDEXFILE_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"

//#undef IINDEX_BLOCK_SIZE
//#undef IINDEX_MAX_BLOCK_COUNT
//#undef IINDEX_MAX_CACHE_SIZE
//#undef IINDEX_SPLIT_COUNT
//#define IINDEX_BLOCK_SIZE       512
//#define IINDEX_MAX_BLOCK_COUNT  64
//#define IINDEX_MAX_CACHE_SIZE   256
//#define IINDEX_SPLIT_COUNT      3

//gcc --std=c99 -I. -Ideps  -o test -DISDK_INDEXFILE_TEST_MAIN isdk_indexfile.c isdk_utils.c isdk_sds.c deps/sds.c deps/zmalloc.c deps/utf8proc.c

bool _test_put(IndexDB *aDB, char *aKey, char aValue[sizeof(IndexDBValue)])
{
    IndexDBValue val, v;
    PIndexDBValue pv;
    int vSize;
    bool vResult;
    vSize = IMIN(sizeof(val), strlen(aValue));

    //bzero(&v, sizeof(IndexDBValue));
    //printf("vSize=%d, DBValue=%lu\n", vSize, sizeof(val));
    //memcpy(&v, aValue, vSize);
    //IndexDB_Put(aDB, aKey, strlen(aKey), v);
    IndexDB_Put(aDB, aKey, strlen(aKey), *(IndexDBValue *) aValue);
    pv = IndexDB_Get(aDB, aKey);
    //val = *v2;
    //assert(errno == 0);
    vResult = pv != NULL;
    if (vResult) vResult = strncmp((char *)pv, aValue, vSize)== 0;
    if (!vResult) printf("put %s key expected: %s; infact: %s\n", aKey, aValue, (char *) pv);
    if (pv) zfree(pv);
    return vResult;
}

void test_put(IndexDB *aDB, char *aKey, char aValue[sizeof(IndexDBValue)])
{
    sds s;
    bool vResult = _test_put(aDB, aKey, aValue);
    s = sdsalloc(NULL, 10);
    s = sdsprintf(s, "put '%s' and get from cache", aKey);
    test_cond(s, vResult);
    sdsfree(s);    
}

int _test_get(IndexDB *aDB, char *aKey, char aValue[sizeof(IndexDBValue)], int aResult)
{
    PIndexDBValue val;
    int vErrno;
    int vSize;
    vSize = IMIN(sizeof(val), strlen(aValue));

    //memcpy(&val, aValue, sizeof(val));
    val = IndexDB_Get(aDB, aKey);
    vErrno = errno;
    if (aResult == 0) {
        //assert(vErrno == aResult);
        aResult = val != NULL;
        if (aResult) {
            aResult = strncmp((char*)val, aValue, vSize)== 0;
            if (!aResult) vErrno = -10000;
        }
    }
    if (val) zfree(val);
    return vErrno;
}

void test_get(IndexDB *aDB, char *aKey, char aValue[sizeof(IndexDBValue)], int aResult)
{
    sds s;
    int result;
    s = sdsalloc(NULL, 10);
    s = sdsprintf(s, "get '%s' should be %d", aKey, aResult);
    result = _test_get(aDB, aKey, aValue, aResult);
    if (result != aResult)
       s = sdscatprintf(s, ", but it is %d", result);
    test_cond(s, result == aResult);
    sdsfree(s);
}

int main(int argc, char **argv)
{
    {

        IndexDB *vDB;
        int i, c;
        sds s = sdsempty();
        sds v = sdsempty();
        vDB = IndexDB_New();
        vDB->path = sdsnew("indexDBTest/1");
/*
        DeleteDir("indexDBTest/1");
        IndexDB_Open(vDB);
        test_put(vDB, "hiworld", "vasf");
        test_put(vDB, "secondlife", "morevaluetest");
        IndexDB_Close(vDB);
        IndexDB_Open(vDB);
        test_get(vDB, "hiworld", "vasf", 0);
        test_get(vDB, "secondlife", "morevaluetest", 0);
        printf("delete hiworld\n");
        IndexDB_Del(vDB, "hiworld", 7);
        test_cond("delete hiworld and cache should be marked deleted", _test_get(vDB, "hiworld", "", IINDEX_ERR_DELETED) == IINDEX_ERR_DELETED);
        IndexDB_Close(vDB);
        IndexDB_Open(vDB);
        test_cond("check hiword should be deleted on disk", _test_get(vDB, "hiworld", "", IINDEX_ERR_NOT_FOUND) == IINDEX_ERR_NOT_FOUND);
        IndexDB_Close(vDB);

        DeleteDir("indexDBTest/1");
        printf("test cache block\n");
        printf("---------------\n");
        IndexDB_Open(vDB);
        c = 0;
        for (i = 0; i < IINDEX_MAX_CACHE_SIZE; i++) {
            s = sdsprintf(s, "Key%08d", i);
            v = sdsprintf(v, "%08d", i);
            if (_test_put(vDB, s, v)) c++;
        }
        test_cond("put a whole cache block", c = i);
        test_cond("bgthread should start", vDB->thread_id);
        IndexDB_Close(vDB);
        IndexDB_Open(vDB);
        c = 0;
        for (i = 0; i < IINDEX_MAX_CACHE_SIZE; i++) {
            s = sdsprintf(s, "Key%08d", i);
            v = sdsprintf(v, "%08d", i);
            if (_test_get(vDB, s, v, 0)==0) c++;
        }
        test_cond("get the whole block keys", c == i);
        IndexDB_Close(vDB);

        DeleteDir("indexDBTest/1");
        IndexDB_Open(vDB);
        printf("test splinter block\n");
        printf("---------------\n");
        c = 0;
        for (i = 0; i < IINDEX_BLOCK_SIZE; i++) {
            s = sdsprintf(s, "Key%08d", i);
            v = sdsprintf(v, "%08d", i);
            if (_test_put(vDB, s, v)) c++;
        }
        test_cond("put a whole block", c = i);
        test_put(vDB, "onanotherblock", "thisAnTherB");
        test_cond("splinter block", vDB->indexes.item[0]->header.blockCount == IINDEX_SPLIT_COUNT);
        IndexDB_Close(vDB);
        IndexDB_Open(vDB);
        c = 0;
        for (i = 0; i < IINDEX_BLOCK_SIZE; i++) {
            s = sdsprintf(s, "Key%08d", i);
            v = sdsprintf(v, "%08d", i);
            if (_test_get(vDB, s, v, 0)==0) c++;
        }
        test_cond("get the splinter block keys", c == i);
        test_get(vDB, "onanotherblock", "thisAnTherB", 0);
        IndexDB_Close(vDB);
//*/
        DeleteDir("indexDBTest/1");
        IndexDB_Open(vDB);
        printf("test splinter file\n");
        printf("---------------\n");
        c = 0;
        for (i = 0; i < IINDEX_BLOCK_SIZE*IINDEX_MAX_BLOCK_COUNT; i++) {
            s = sdsprintf(s, "Key%08d", i);
            v = sdsprintf(v, "%08d", i);
            if (_test_put(vDB, s, v)) c++;
        }
        test_cond("put a whole file", c = i);
        test_put(vDB, "onanotherfile", "thisAnTherB");
        //test_cond("splinter block", vDB->indexes.item[0]->header.blockCount == IINDEX_SPLIT_COUNT);
        IndexDB_Close(vDB);
        IndexDB_Open(vDB);
        printf("indexes=%zd\n", darray_size(vDB->indexes));
        c = 0;
        for (i = 0; i < IINDEX_BLOCK_SIZE*IINDEX_MAX_BLOCK_COUNT; i++) {
            s = sdsprintf(s, "Key%08d", i);
            v = sdsprintf(v, "%08d", i);
            test_get(vDB, s, v, 0);
            //if (_test_get(vDB, s, v, 0)==0) c++;
        }
        //test_cond("get the splinter file keys", c == i);
        test_get(vDB, "onanotherfile", "thisAnTherB", 0);
//*/
        IndexDB_Free(vDB);
        if (s) sdsfree(s);
        if (v) sdsfree(v);
    }
    test_report();
    return 0;
}
#endif
