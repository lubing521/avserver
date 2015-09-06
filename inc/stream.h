

#ifndef _STREAM_H_
#define _STREAM_H_

#include <mem_mng.h>
#include <cache_mng.h>
#include <gop_lock.h>
#include <semaphore_util.h>
#include <pthread.h>

enum{
	STREAM_FAILURE   = -1,
	STREAM_SUCCESS   = 0
};


enum{
	STREAM_SEM_MAIN = 0,
	STREAM_SEM_SUB,
	STREAM_SEM_GOP,
	STREAM_SEM_NUM
};

typedef struct _STREAM_PARM{
	MEM_MNG_INFO 	MemInfo;
	int 			MemMngSemId[STREAM_SEM_NUM];
	int 			lockNewFrame[GOP_INDEX_NUM];
	int				IsQuit;
	int				qid;
	pthread_t		threadControl;
}	STREAM_PARM;

STREAM_PARM *stream_get_handle(void);
int stream_init( STREAM_PARM *pParm);
int stream_write(void *pAddr, int size, int frame_type ,int stream_type, unsigned int timestamp , unsigned int temporalId ,STREAM_PARM *pParm);
int stream_end(STREAM_PARM *pParm);

#endif
