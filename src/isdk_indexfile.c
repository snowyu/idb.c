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
#include <stdio.h>

//the internal structures should be in isdk_indexfile.c
struct iIndexFileHeader {
	uint32_t    count[IINDEX_MAX_BLOCK];                //keep the each level block's count
	char        isFull[IINDEX_MAX_BLOCK];               //specified whether the level block's full.
	char        maxKey[IINDEX_MAX_KEY_SIZE];            //specified which is the max key of the index file.
};

//manage all index files here.
struct iIndexMeta {

};

