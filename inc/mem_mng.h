
#ifndef __MEM_MNG__
#define __MEM_MNG__


#define MAIN_MEM_SIZE				(0xa00000)
#define SUB_MEM_SIZE				(0x200000)
#define MAIN_EXT_SIZE				(256)
#define SUB_EXT_SIZE				(256)

#define MAIN_BLK_SIZE				(20*1024)
#define SUB_BLK_SIZE				(2*1024)

#define MAIN_CACHE_SIZE				MAIN_MEM_SIZE
#define SUB_CACHE_SIZE				SUB_MEM_SIZE
#define MAIN_EXT_CACHE_SIZE			MAIN_EXT_SIZE
#define SUB_EXT_CACHE_SIZE			SUB_EXT_SIZE

#define MAIN_CACHE_BLK_SIZE			MAIN_BLK_SIZE
#define SUB_CACHE_BLK_SIZE			SUB_BLK_SIZE

#define TOTAL_MEM_SIZE				(MAIN_MEM_SIZE+SUB_MEM_SIZE+MAIN_EXT_SIZE+SUB_EXT_SIZE)
#define TOTAL_CACHE_SIZE			(MAIN_CACHE_SIZE+SUB_CACHE_SIZE+MAIN_EXT_CACHE_SIZE+SUB_EXT_CACHE_SIZE)
#define TOTAL_SIZE					(TOTAL_MEM_SIZE+TOTAL_CACHE_SIZE)

enum{
	DUMMY_FRAME = (-2),
	EMPTY_FRAME = (-1),
	AUDIO_FRAME = 0,
	I_FRAME,
	P_FRAME,
	B_FRAME,
	END_FRAME_TYPE
};

enum{
	VIDEO_INFO_MAIN = 0,
	VIDEO_INFO_SUB,
	VIDEO_INFO_END
};

enum{
	CACHE_PARM_ERR = -100,
	CACHE_NOMEM,
	CACHE_MEM_FAIL,
	CACHE_ERR,
	CACHE_OK = 0,
};

enum{
	EMPTY_CACHE = (-1),
	I_FRAME_CACHE = I_FRAME,
	P_FRAME_CACHE = P_FRAME,
	B_FRAME_CACHE = B_FRAME,
	END_CACHE_TYPE
};


typedef struct _CACHE_DATA_INFO{
	int 			serial;
	int 			fram_type;
	unsigned long 	start_addr;
	unsigned long 	start_phy;
	int				realsize;
	int 			flag;
	unsigned int	timestamp;
	unsigned int	temporalId;
	int				ref_serial[VIDEO_INFO_END];
}CACHE_DATA_INFO;


typedef struct _CACHE_DATA{
	int 	serial;
	int 	fram_type;
	int		blkindex;
	int		blks;
	int		realsize;
	int		cnt;
	int		flag;
	unsigned int timestamp;
	unsigned int temporalId;
	int		ref_serial[VIDEO_INFO_END];
}CACHE_DATA;

typedef struct _CACHE_BLK{
	int IsFree;
	int offset;
	int serial;
}CACHE_BLK;

typedef struct _CACHE_MNG_INFO{
	int 			video_type;
	unsigned long 	start_addr;
	unsigned long 	start_phy;
	int				size;
	int				blk_sz;
	int				blk_num;
	int				cache_num;
	int				extraSize;
	void			*extraData;
	int				hvextra;
	CACHE_DATA		*cache;
	CACHE_BLK		*blk;
} CACHE_MNG;


typedef struct _VIDEO_FRAME
{
	int 	serial;
	int 	fram_type;
	int		blkindex;
	int		blks;
	int		realsize;
	int		flag;
	unsigned int	timestamp;
	unsigned int	temporalId;
	int		ref_serial[VIDEO_INFO_END];
} VIDEO_FRAME;

typedef struct _VIDEO_GOP
{
	int				last_Start;
	int				last_Start_serial;
	int				last_End;
	int				last_End_serial;
	int				lastest_I;
	int				lastest_I_serial;
}VIDEO_GOP;

typedef struct _VIDEO_BLK_INFO
{
	int 			video_type;
	int				width;
	int				height;
	unsigned long	start;
	unsigned long	start_phy;
	unsigned long	size;
	void			*extradata;
	int				extrasize;
	int				hvextra;
	int				IsCache;

	int				blk_sz;
	int				blk_num;
	int				blk_free;
	int				frame_num;
	int				cur_frame;
	int				cur_serial;
	int				cur_blk;
	unsigned int	timestamp;
	unsigned int	temporalId;
	VIDEO_GOP		gop;
	VIDEO_FRAME		*frame;
	CACHE_MNG		*cachemng;
}VIDEO_BLK_INFO;



typedef struct _MEM_INFO{
	unsigned long 	start_addr;
	unsigned long 	start_phyAddr;
	int				freesize;
	int				totalsize;
	int				offset;
	int				mem_layout;
	int				video_info_nums;
	int				otherSize;
	void				*otherData;
	VIDEO_BLK_INFO	*video_info;

} MEM_MNG_INFO;

typedef struct _MEM_SIZE{
	unsigned int cache_size;
	unsigned int cache_blk_size;
	unsigned int mem_size;
	unsigned int mem_blk_size;
	unsigned int ext_size;
}MEM_SIZE;



int MemMng_Video_Write( void *pData, int size, int frame_type, VIDEO_BLK_INFO *pVidInfo );

int MemMng_release( MEM_MNG_INFO *pInfo );
int MemMng_Init( MEM_MNG_INFO *pInfo );

int MemMng_Video_ReadFrame(void *pDest, int *pSize, int *pFrm_type,
			int bufflimit, VIDEO_FRAME *pFrame, VIDEO_BLK_INFO *pVidInfo );
unsigned long GetMemMngPhyBaseAddr(MEM_MNG_INFO *pInfo);
unsigned long GetMemMngTotalMemSize(MEM_MNG_INFO *pInfo);

VIDEO_FRAME * MemMng_GetFrameBySerial( int serial , VIDEO_BLK_INFO *pVidInfo);
int GetCurrentSerialNo(VIDEO_BLK_INFO *video_info);
int GetCurrentOffset(VIDEO_BLK_INFO *video_info);
VIDEO_FRAME * GetCurrentFrame(VIDEO_BLK_INFO *video_info);
VIDEO_FRAME * GetLastI_Frame(VIDEO_BLK_INFO *video_info);
#endif

