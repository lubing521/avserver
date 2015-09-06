/* ===========================================================================
* @path $(IPNCPATH)\interface\inc\
*
* @desc
* .
* Copyright (c) Appro Photoelectron Inc.  2008
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied
*
* =========================================================================== */
/**
* @file share_mem.h
* @brief  API to shared memory
*/
#ifndef __SHARE_MEM_H__
#define __SHARE_MEM_H__

#define PROC_MEM_SIZE 1024*1024*1
#define MAX_SHARE_PROC	1
/**
* @ingroup UTILITY API
* @defgroup SHARED_MEM_DRV Shard memory API

* This is the communication interface of shared memory manager. 
* @{
*/
int ShareMemInit(int key); ///< Initial shared memory.
int pShareMemInit(int key); ///< Initial shared memory for private use .
void ShareMemRead(int mid, int offset,void *buf, int length); ///< Read shared memory.
void ShareMemWrite(int mid, int offset,void *buf, int length); ///< Write shared memory.
void ShareMemDel(int mid);//delete the shared memory

char *shareMat(int mid);
void shareMdt(char *pSrc);

void shareMemWrite_BR(char *pDst, int offset, void *buf, int length);

void shareMemRead_BR(char *pSrc, int offset, void *buf, int length);

int SemCreate(int key);
int SemGet(int key);
int Sem_P(int id);
int Sem_V(int id);
void DeleteSem(int id);

/** @} */
#endif
/**
* @page SHARED_MEM_DRV_HOW How to use shared memory API?
* 1. Call the API ShareMemInit(int key) (or pShareMemInit(int key))  with the key you added to initialize shared memory.\n
* 2. Now you can use any shared memory API functions as you want.\n
* @section SHARED_MEM_DRV_HOW_EX Example
* @code
#include <share_mem.h>
int main()
{ 
         int mid;
	mid = ShareMemInit(key);
	if(mid < 0){			
		return -1;
	}
	// more shared memory API
	return 0;
}
* @endcode
*/
