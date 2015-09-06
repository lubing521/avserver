

#ifndef __RTSPD_H
#define __RTSPD_H

#include "dvr_enc_api.h"
#include "dvr_common_api.h"
#include "gmavi_api.h"
#include "ioctl_isp.h"
#include "file_cfg.h"

#include "interface.h"
#include "ipnc.h"


#define IPC_CFG_FILE       "/mnt/mtd/ipc.cfg"
#define SDPSTR_MAX		128
#define FILE_PATH_MAX	128
#define SR_MAX			64
#define	VQ_MAX			(SR_MAX)
#define	VQ_LEN			5
#define AQ_MAX			64			/* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AQ_LEN			2			/* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AV_NAME_MAX		127

#define RTP_HZ				90000

#define ERR_GOTO(x, y)			do { ret = x; goto y; } while(0)
#define MUTEX_FAILED(x)			(x == ERR_MUTEX)

#define TIME_INTERVAL 4*RTP_HZ 
#define MACRO_BLOCK_SIZE 100
#define PRINT_ENC_AVERAGE 1
#define VIDEO_FRAME_NUMBER VQ_LEN+1

#define MAIN_BS_NUM	0
#define SUB1_BS_NUM	1
#define SUB2_BS_NUM	2
#define SUB3_BS_NUM	3
#define SUB4_BS_NUM	4
#define SUB5_BS_NUM	5
#define SUB6_BS_NUM	6
#define SUB7_BS_NUM	7
#define SUB8_BS_NUM	8
//#define ALL_BS_NUM	3

#define NONE_BS_EVENT	0
#define START_BS_EVENT	1
#define UPDATE_BS_EVENT	2
#define STOP_BS_EVENT	3

#define RATE_CTL_CBR	0
#define RATE_CTL_VBR	1
#define RATE_CTL_ECBR	2
#define RATE_CTL_EVBR	3

#define NTSC  0   ///< video mode : NTSC
#define PAL   1   ///< video mode : PAL
#define CMOS   2   ///< video mode : VGA


#define CHANNEL_NUM		2

#define CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num)	\
	do {	\
	if((ch_num >= CHANNEL_NUM || ch_num < 0) || \
	(sub_num >= DVR_ENC_REPD_BT_NUM || sub_num < 0)) {	\
	fprintf(stderr, "%s: ch_num=%d, sub_num=%d error!\n",__FUNCTION__, ch_num, sub_num);	\
	return -1; \
		}	\
	} while(0)	\


typedef int (*open_container_fn)(int ch_num, int sub_num);
typedef int (*close_container_fn)(int ch_num, int sub_num);
typedef int (*read_bs_fn)(int ch_num, int sub_num, dvr_enc_queue_get *data);
typedef int (*write_bs_fn)(int ch_num, int sub_num, void *data);
typedef int (*free_bs_fn)(int ch_num, int sub_num, dvr_enc_queue_get *data);

typedef enum st_opt_type {
	OPT_NONE=0,
	RTSP_LIVE_STREAMING,
	FILE_AVI_RECORDING,
	FILE_H264_RECORDING,
	H264_TO_NVR,
	OPT_END
} opt_type_t;

typedef struct st_video_frame {
	int ch_num;
	int sub_num;
	dvr_enc_queue_get queue;	
	int search_key;
} video_frame_t;

typedef struct st_frame_slot {
	struct st_frame_slot *next;
	struct st_frame_slot *prev;
	video_frame_t *vf;
} frame_slot_t;

typedef struct st_frame_info {
	frame_slot_t *avail;
	frame_slot_t *used;
	pthread_mutex_t mutex;
} frame_info_t;

typedef struct st_vbs {
	int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	int enc_type;	// 0:ENC_TYPE_H264, 1:ENC_TYPE_MPEG, 2:ENC_TYPE_MJPEG
	int width;
	int height;
	int fps;
	int ip_interval;
	int bps;	// if bps = 0, for default bps, generate by get_bitrate()
	int rate_ctl_type;	// 0:cbr, 1:vbr, 2:ecbr, 3:evbr
	int target_rate_max;
	int reaction_delay_max;
	int max_quant;
	int min_quant;
	int init_quant;
	int mjpeg_quality;
	int enabled_snapshot; //1: enabled, 0: disabled, not ready
	int enabled_roi;      //1: enabled, 0: disabled, not ready
	int roi_x;            //roi x position
	int roi_y;            //roi y position
	int roi_w;            //roi width in pixel
	int roi_h;            //roi height in pixel
} vbs_t;

typedef struct st_priv_vbs {
	int offs;		// bitstream mmap buffer offset.
	int buf_usage;
	char sdpstr[SDPSTR_MAX];
	unsigned int tick;
	int	tinc;	/* interval, in unit of 1/9KHz. */
	int timed;
	int itvl_fps;
	int avg_fps;
	int itvl_bps;
	int avg_bps;
	unsigned int timeval;
	int qno;
} priv_vbs_t;

typedef struct st_abs {
//	int reserved;
	int enabled;  	// DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	int enc_type;	// 0:mp2, 1:adpcm, 2:amr
	int bps;
} abs_t;

typedef struct st_priv_abs {
//	int reserved;
	int offs;       // bitstream mmap buffer offset.
	int buf_usage;
	char sdpstr[SDPSTR_MAX];
	unsigned int tick;
	int tinc; /* interval, in unit of 1/9KHz. */
	int timed;
	int avg_bps;
	unsigned int timeval;
	int qno;
} priv_abs_t;

typedef struct st_bs {
	int event; // config change please set 1 for enq_thread to config this 
	int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	opt_type_t opt_type;  /* 1:rtsp_live_streaming, 2: file_avi_recording 3:file_h264_recording */
	vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
	abs_t audio;  /* AUDIO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} avbs_t;


typedef struct st_priv_bs {
	//char rtsp_live_name[AV_NAME_MAX];
	int play;
	int congest;
	int sr;
	char name[AV_NAME_MAX];
	void *fd;
	int reset_intra_frame;
	int avi_str_id;
	open_container_fn open;
	close_container_fn close;
	read_bs_fn read;
	write_bs_fn write;
	free_bs_fn free;
	priv_vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
	priv_abs_t audio;  /* AUDIO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} priv_avbs_t;

typedef struct st_av {
	/* public data */
	avbs_t bs[DVR_ENC_REPD_BT_NUM];  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
	/* update date */
	pthread_mutex_t ubs_mutex;
	update_avbs_t ubs[DVR_ENC_REPD_BT_NUM];

	/* per channel data */
	int denoise;  /* 3D-denoise */
	int de_interlace;  /* 3d-deInterlace  */
	int input_system; 

	/* private data */
	int enabled;  	//DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	priv_avbs_t priv_bs[DVR_ENC_REPD_BT_NUM];	
	int enc_fd; 
	unsigned char *v_bs_mbuf_addr;
	int v_bs_mbuf_size;
	unsigned int bs_buf_snap_offset;
} av_t;
/*
typedef struct st_gm_ss_entity {
	unsigned int	timestamp;	// Global across all entities, in 90 KHz.
	int				size;		// Actual size.
	char			*data;		// Point to actual data buffer.
} gm_ss_entity;
*/
enum MCTP_CMD_TYPE{
	ALIVE = 0,
	PTZ,
	VCTRL
};

extern unsigned int snap_start;

extern av_t enc[CHANNEL_NUM];

int init_isp_pw_frequency(int iFW);
int update_enc_param(update_avbs_t *pubs, int channel, int sub_channel);
void *infrad_ctrl(void *ptr);
int adjust_saturation(int val);
int free_bs_data(int ch_num, int sub_num, dvr_enc_queue_get *data);


//PTZ
#define		PTZ_STOP                 0
#define         	TILT_UP                     1
#define         	TILT_DOWN                2
#define         	PAN_LEFT                     3
#define         	PAN_RIGHT                    4
#define         	PT_LEFT_UP                      5
#define         	PT_LEFT_DOWN 6
#define         	PT_RIGHT_UP  7
#define         	PT_RIGHT_DOWN 8
#define         	Z_ZOOM_IN 9
#define		Z_ZOOM_OUT 10
#define  		FOCUS_NEAR 11
#define 		FOUCE_FAR 12
#define 		IRIS_OPEN 13
#define 		IRIS_CLOSE 14
#define 		SET_PRESET 17
#define 		CLE_PRESET 16
#define 		GOTO_PRESET 15
#define 		PAN_AUTO 18
#define 		PAN_AUTO_STOP 19
#define 		SPEED_UP 30
#define 		SPEED_DOWN 31

int mctp_cmd_cb(enum MCTP_CMD_TYPE cmd_type, char *cmdline);

void put_audio_data(unsigned char *pbuf, unsigned int data_len);

#endif
