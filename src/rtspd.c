/*
 * Copyright (C) 2009 Faraday Technology Corporation.
 * NOTE:
 *  1. librtsp is statically linked into this program.
 *  2. Only one rtsp server thread created. No per-thread variables needed.
 *  3. Write for testing librtsp library, not optimized for performance.
 *  Version	Date		Description
 *  ----------------------------------------------
 *  0.1.0	        	First release.
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "librtsp.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <signal.h>

#include "rtspd.h"

#include "ioctl_isp210.h"
#include "ioctl_gm_ae.h"
#include "ioctl_gm_awb.h"

#include "cmd_type.h"
#include "init_system.h"
#include "cmdreceiver.h"
#include "g711s.h"

#include "GM8126_p2p.h"

#include "mt.h"
//#include "mem_mng.h"

#define IOCTL(a,b,c) ioctl(a,b,&c)
pthread_t		enq_thread_id;
unsigned int	sys_tick = 0;
__time_t		sys_sec;
int				sys_port	= 554;
char			file_path[FILE_PATH_MAX]	= "file";
char			*ipptr = NULL;
char			ipstr[32] = {0};
//static int t_ch_num=CHANNEL_NUM;
static int t_ch_num = 2;
static int rtspd_sysinit=0;
static int rtspd_set_event=0;
static int enable_print_average = 0;

static frame_info_t frame_info[VQ_MAX];
//pthread_mutex_t rtspd_mutex;

int NVR_send_status[CHANNEL_NUM+1][DVR_ENC_REPD_BT_NUM];


av_t enc[CHANNEL_NUM];
fcfg_file_t   *cfg_file;

struct pollfd poll_fds[CHANNEL_NUM];  // poll
struct pollfd audio_poll_fds;
//int bs_buf_snap_offset;
unsigned int snap_start = 0;
static int snap_init_OK = 0;

int rtspd_dvr_fd = 0;

static char *opt_type_def_str[]={  
                "None",
                "RTSP",
                " AVI",
                "H264"};

static char *enc_type_def_str[]={  
                " H264",
                "MPEG4",
                "MJPEG"};

static char *rcCtlTypeString[]={
                "CBR",
				"VBR",
				"ECBR",
				"EVBR"};	

static char *rcInSysString[]={
                "NTSC",
				"PAL",
				"CMOS"};	
						
static int dvr_enc_queue_get_def_table[]={  
                DVR_ENC_QUEUE_GET,		    /* main-bitstream */
                DVR_ENC_QUEUE_GET_SUB1_BS,	/* sub1-bitstream */
                DVR_ENC_QUEUE_GET_SUB2_BS,	/* sub2-bitstream */
                DVR_ENC_QUEUE_GET_SUB3_BS,	/* sub2-bitstream */
                DVR_ENC_QUEUE_GET_SUB4_BS,	/* sub2-bitstream */
                DVR_ENC_QUEUE_GET_SUB5_BS,	/* sub2-bitstream */
                DVR_ENC_QUEUE_GET_SUB6_BS,	/* sub2-bitstream */
                DVR_ENC_QUEUE_GET_SUB7_BS,	/* sub2-bitstream */
                DVR_ENC_QUEUE_GET_SUB8_BS};	/* sub3-bitstream */

static int dvr_enc_queue_offset_def_table[]={  
                0,		                                 /* main-bitstream */
		   DVR_ENC_QUERY_OUTPUT_BUFFER_SUB1_BS_OFFSET,	 /* sub1-bitstream */
                DVR_ENC_QUERY_OUTPUT_BUFFER_SUB2_BS_OFFSET,	 /* sub2-bitstream */
                DVR_ENC_QUERY_OUTPUT_BUFFER_SUB3_BS_OFFSET,	 /* sub2-bitstream */
                DVR_ENC_QUERY_OUTPUT_BUFFER_SUB4_BS_OFFSET,	 /* sub2-bitstream */
                DVR_ENC_QUERY_OUTPUT_BUFFER_SUB5_BS_OFFSET,	 /* sub2-bitstream */
                DVR_ENC_QUERY_OUTPUT_BUFFER_SUB6_BS_OFFSET,	 /* sub2-bitstream */
                DVR_ENC_QUERY_OUTPUT_BUFFER_SUB7_BS_OFFSET,	 /* sub2-bitstream */
                DVR_ENC_QUERY_OUTPUT_BUFFER_SUB8_BS_OFFSET}; /* sub3-bitstream */

dvr_enc_channel_param   main_ch_setting = 
{
    {
		0,
    	ENC_TYPE_FROM_CAPTURE,
        {1280, 720},
        LVFRAME_EVEN_ODD, //LVFRAME_EVEN_ODD, LVFRAME_GM3DI_FORMAT,
		LVFRAME_FRAME_MODE, //LVFRAME_FIELD_MODE, //LVFRAME_FRAME_MODE,
		DMAORDER_PACKET, //DMAORDER_PACKET, DMAORDER_3PLANAR, DMAORDER_2PLANAR
        CAPSCALER_NOT_KEEP_RATIO,
        MCP_VIDEO_NTSC,     // MCP_VIDEO_NTSC
		CAPCOLOR_YUV422,  // CAPCOLOR_YUV420_M0, CAPCOLOR_YUV422
        { FALSE, FALSE, GM3DI_FRAME }    //Denoise off
    },
    {
		DVR_ENC_EBST_ENABLE,  //enabled
		0,	// main-bitstream
		ENC_TYPE_H264,
		FALSE,
		DVR_ENC_EBST_ENABLE, // en_snapshot
		{1280, 720},			// DIM
		{ENC_INPUT_H2642D, 30, 1048576,  30, 25, 51, 1 , FALSE, {0, 0, 320, 240}},	//EncParam
		{SCALE_YUV422, SCALE_YUV422, SCALE_LINEAR, FALSE, FALSE, TRUE, 0 },	//ScalerParam
		{JCS_yuv420, 0, JENC_INPUT_MP42D, 70} //snapshot_param	
    }
};

ReproduceBitStream   sub_ch_setting = 
{
	DVR_ENC_EBST_DISABLE,  //enabled
	1,	// sub1-bitstream
	ENC_TYPE_MPEG, //enc_type, 0: ENC_TYPE_H264, 1:ENC_TYPE_MPEG, 2:ENC_TYPE_MJPEG
	FALSE,  // is_blocked
	DVR_ENC_EBST_DISABLE, // en_snapshot,
	{1280, 720},			// DIM
	{ENC_INPUT_1D422, 5, 262144,  15, 25, 51, 1 , FALSE, {0, 0, 320, 240}},	//EncParam
	{SCALE_YUV422, SCALE_YUV422, SCALE_LINEAR, FALSE, FALSE, TRUE, 0 },	//ScalerParam
	{JCS_yuv420, 0, JENC_INPUT_MP42D, 70 }	//snapshot_param	
};


#define Audio_RTP_HZ    8000
static struct timeval audio_sys_sec_a={-1,-1};
static unsigned int audio_sys_tick = 0;
static unsigned int audio_get_tick(struct timeval *tv)
{
	struct timeval  t, *tval;
	__time_t        tv_sec;

	if(tv == NULL) {
		tval = &t;
		gettimeofday(tval, NULL);
	} else {
		tval = tv;
	}
	if(audio_sys_sec_a.tv_sec == -1) {
		audio_sys_sec_a.tv_sec = tval->tv_sec; //->tv_sec;
		audio_sys_sec_a.tv_usec = tval->tv_usec;

	}
	tv_sec = tval->tv_sec-audio_sys_sec_a.tv_sec;
	//sys_tick += tv_sec*Audio_RTP_HZ;
	if(tv_sec>0)
		audio_sys_tick += ((tval->tv_usec+(1000000-audio_sys_sec_a.tv_usec))
		*(Audio_RTP_HZ/1000)/1000);
	else
		audio_sys_tick += ((tval->tv_usec-audio_sys_sec_a.tv_usec)*(Audio_RTP_HZ/1000)/1000);
	audio_sys_sec_a.tv_sec = tval->tv_sec;
	audio_sys_sec_a.tv_usec = tval->tv_usec;
	//tick = tval->tv_usec*(Audio_RTP_HZ/1000)/1000;
	//tick += sys_tick;
	return audio_sys_tick;
}

#define		AUDIO_SILC_SIZE		(20*8)
#define		ADATA_BUF_LENGTH	4096
#define		NORMAL				0
#define		EMTYE				1
#define		FULL				2
struct Audio_data{
	int		p_in;
	int		p_out;
	int		stat;
	char	buf[ADATA_BUF_LENGTH];
};

unsigned char *audio_data = NULL;
unsigned char *play_data = NULL;
unsigned char *g711_data=NULL; // = {....};

int audio_fd = -1, ablk_size = 0;
int paudio_fd=-1, paudio_init = 0;
#define AUDIO_DEF   "/dev/dsp2"
//FILE *fp_audio;
char *audio_file = "/mnt/mtd/audio.pcm";
struct Audio_data	adata_b[2];

static void audio_stop(void)
{
	close(audio_fd);
	close(paudio_fd);
	audio_fd=-1;
	paudio_fd = -1;
	ablk_size=0;
	free(audio_data);
	free(play_data);
	free(g711_data);

	play_data = NULL;
	audio_data=NULL;
	g711_data=NULL;

	paudio_init = 0;
	audio_sys_tick=0;
	audio_sys_sec_a.tv_sec=-1;
	audio_sys_sec_a.tv_usec=-1;
	//fclose(fp_audio);
	printf("Audio stop!\n");

}

static int audio_out_init(void)
{
	int stereo = 1;
	int samplesize = 16;
	int dsp_speed = 8000;
	char *audio_name=AUDIO_DEF;

	if(audio_fd<0||ablk_size==0){
		return;
	}

	if(paudio_fd == -1){
		printf("Init audio out.\n");
		paudio_fd = open(audio_name, O_WRONLY, 0);
		if(paudio_fd<0){
			printf("open /dev/dsp2 for writing failed.\n");
		}else{
			IOCTL(paudio_fd, SNDCTL_DSP_SAMPLESIZE, samplesize);
			IOCTL(paudio_fd, SNDCTL_DSP_SPEED, dsp_speed);
			IOCTL(paudio_fd, SNDCTL_DSP_STEREO, stereo);

			if(play_data!=NULL){
				free(play_data);
			}
			play_data = (char *)malloc(ablk_size);
			if(play_data==NULL){
				printf("play_data alloc error.\n");
				close(paudio_fd);
				paudio_fd = -1;
				return -1;
			}
		}
	}
	return 0;
}

void put_audio_data(unsigned char *pbuf, unsigned int data_len)
{
//	printf("write audio data.....\n");
	int idx = 0;
	unsigned short *pDst;
	unsigned short *pSrc;

	if(paudio_init!=1){
		if(audio_out_init()==0){
			paudio_init = 1;
		}else{
			return;
		}
	}

//	printf("......write.....audio...ablk_size = %d, data_len = %d.\n", ablk_size, data_len);
	memset(play_data, 0, ablk_size);
	pDst = (unsigned short *)play_data;
	pSrc = (unsigned short *)pbuf;
	for(idx=0; idx<data_len; idx++){
		pDst++;
		*pDst++ = *pSrc++;
	}
/*
	printf("psrc = %x %x %x %x %x %x %x %x %x %x %x %x.\n", 
													*pbuf, *(pbuf+1), *(pbuf+2), *(pbuf+3), 
													*(pbuf+4), *(pbuf+5), *(pbuf+6), *(pbuf+7),
													*(pbuf+8), *(pbuf+9), *(pbuf+10), *(pbuf+11));
	printf("pDst = %x %x %x %x %x %x %x %x %x %x %x %x.\n", 
													*play_data, *(play_data+1), *(play_data+2), *(play_data+3), 
													*(play_data+4), *(play_data+5), *(play_data+6), *(play_data+7),
													*(play_data+8), *(play_data+9), *(play_data+10), *(play_data+11));
*/
	write(paudio_fd, play_data, data_len<<1);
//	write(paudio_fd, pbuf, data_len);
}


static void audio_init(void)
{
	int stereo = 1;
	int samplesize = 16;
	int dsp_speed = 8000;
	char *audio_name=AUDIO_DEF;
	//fp_audio = fopen(audio_file, "w+");
	NVR_send_status[2][0] = 0;
	if( audio_fd == -1 )
	{
		printf("Init Audio data!\n");
		audio_fd = open(audio_name, O_RDONLY, 0);
		if( audio_fd < 0 )
		{
			printf("open(/dev/dsp2) failed. So we disable audio!\n");
		}
		else
		{
			IOCTL(audio_fd, SNDCTL_DSP_SAMPLESIZE, samplesize);
			IOCTL(audio_fd, SNDCTL_DSP_SPEED, dsp_speed);
			IOCTL(audio_fd, SNDCTL_DSP_STEREO, stereo);
			IOCTL(audio_fd, SNDCTL_DSP_GETBLKSIZE, ablk_size);

			if(audio_data)  
				free(audio_data);

			if((audio_data = (char *)malloc(ablk_size))==NULL)
			{
				printf("audio block alloc err!\n");
				close(audio_fd);
				audio_fd = -1;
				return;
			}
			else
			{
				printf("audio block size=%d\n", ablk_size);
			}
#if 1
			if(g711_data)  
				free(g711_data);

			if( (g711_data = (unsigned char *) malloc(ablk_size >> (stereo+1))) == NULL ){
				printf("audio block alloc err!\n");
			}
			else{
				printf("audio G711 block size=%d\n", ablk_size >> (stereo+1));
			}
#endif
			audio_poll_fds.revents = 0;
			audio_poll_fds.events = POLLIN;
			audio_poll_fds.fd = audio_fd;

			adata_b[0].p_in = adata_b[0].p_out = 0;
			adata_b[1].p_in = adata_b[1].p_out = 0;
			adata_b[0].stat = adata_b[1].stat = EMTYE;
		}
	}
}

static void adata_fifo_in(struct Audio_data *padata, char* buf, int len)
{
	int b_len = 0;

	b_len = (padata->p_out>padata->p_in)?(padata->p_out-padata->p_in):(padata->p_out+ADATA_BUF_LENGTH-padata->p_in);
	if(b_len>len){
		padata->stat = NORMAL;
	}else{
		padata->stat = FULL;
	}

	//	printf("padata = 0x%x, b_len = %d, len = %d.\n", (unsigned int)padata, b_len, len);

	if(padata->p_in+len>ADATA_BUF_LENGTH){
		memcpy(&(padata->buf[padata->p_in]), buf, ADATA_BUF_LENGTH-padata->p_in);
		buf += (ADATA_BUF_LENGTH-padata->p_in);
		memcpy(&(padata->buf[0]), buf, padata->p_in+len-ADATA_BUF_LENGTH);
		padata->p_in = (padata->p_in+len-ADATA_BUF_LENGTH)%ADATA_BUF_LENGTH;
	}else{
		memcpy(&(padata->buf[padata->p_in]), buf, len);
		padata->p_in += len;
	}
	padata->p_in %= ADATA_BUF_LENGTH;

	if(padata->stat==FULL){
		//如果空间满，输出指向输入点
		padata->p_out = padata->p_in;
	}
	//	printf("exit adata_fifo_in.\n");

	return;
}

static int adata_fifo_out(struct Audio_data *padata, char* buf, int* plen, int max_len)
{
	int b_len = 0;
	int len = 0;

	if(padata->stat==EMTYE){
		*plen = 0;
		return 0;
	}

	if(padata->p_in>padata->p_out){
		b_len = padata->p_in-padata->p_out;
		if(b_len>max_len){
			b_len = max_len;
		}
		len = (b_len/AUDIO_SILC_SIZE)*AUDIO_SILC_SIZE;
		memcpy(buf, &(padata->buf[padata->p_out]), len);
		padata->p_out += len;
	}else{
		b_len = ADATA_BUF_LENGTH-padata->p_out+padata->p_in;
		if(b_len>max_len){
			b_len = max_len;
		}
		len = (b_len/AUDIO_SILC_SIZE)*AUDIO_SILC_SIZE;
		if((padata->p_out+len)>ADATA_BUF_LENGTH){
			memcpy(buf, &(padata->buf[padata->p_out]), ADATA_BUF_LENGTH-padata->p_out);
			buf += (ADATA_BUF_LENGTH-padata->p_out);
			memcpy(buf, &(padata->buf[0]), padata->p_out+len-ADATA_BUF_LENGTH);
			padata->p_out = (padata->p_out+len)%ADATA_BUF_LENGTH;
		}else{
			memcpy(buf, &(padata->buf[padata->p_out]), len);
			padata->p_out += len;
		}
		padata->p_out %= ADATA_BUF_LENGTH;
	}

	if(padata->p_in==padata->p_out){
		padata->stat = EMTYE;
	}

	//	printf("p_in = %d, p_out = %d, len = %d.\n", padata->p_in, padata->p_out, len);

	*plen = len;
	return len;
}

static int convert_gmss_audio_type(int type)
{
	int media_type;

	switch(type) {
	case 0: 
		media_type=GM_SS_TYPE_G711; 
		break;
	case 1: 
		media_type=GM_SS_TYPE_G726; 
		break;
	case 2: 
		media_type=GM_SS_TYPE_AAC; 
		break;
	default: 
		media_type  = -1; 
		fprintf(stderr, "convert_gmss_media_type: type=%d, error!\n", type); 
		break;
	}
	return media_type;	
}



int g711_encode_frame(unsigned char *frame, void *data, int ByteLength,int channel)
{
	int i;
	short *samples;
	unsigned char *dst;

	dst = frame;
	samples = (short *)data;
	//input 8K 16bits stereo (4096 bytes = 1024 samples * 2 channel * 2 bytes)
	//output 8K 8bits mono  (1024 bytes = 1024 samples * 1 channel * 1 bytes)
	for(i=0; i<ByteLength>>channel; i++) //output bytes
	{
		*dst = G711_linear2ulaw(*samples);  // Faraday-enhanced function in [g711s.s]
		dst++;
		samples+=channel;
	}

	return dst - frame;
}


static int write_rtp_audio(int ch_num, int sub_num, void *data, int data_len)
{
	int ret, media_type;
	avbs_t *b;
	priv_avbs_t *pb;
	gm_ss_entity entity;

	pb = &enc[ch_num].priv_bs[sub_num];
	b = &enc[ch_num].bs[sub_num];

	if(pb->play==0 || (b->event != NONE_BS_EVENT)) {
		ret = 1;
		goto exit_free_as_buf;
	}

	entity.data =(char *)data; // enc[ch_num].v_bs_mbuf_addr + pb->video.offs + q->bs.offset;
	entity.size = data_len;  //q->bs.length;
	entity.timestamp = audio_get_tick(NULL);
	media_type = convert_gmss_audio_type(b->audio.enc_type); //GM_SS_TYPE_PCM; //for PCM
	//fwrite(data, data_len, 1, fp_audio);

	ret = stream_media_enqueue(media_type, pb->audio.qno, &entity);
	if (ret < 0) {
		if (ret == ERR_FULL) {
			pb->congest = 1;
			fprintf(stderr, "enqueue queue ch_num=%d, sub_num=%d full\n", ch_num, sub_num);
		} else if ((ret != ERR_NOTINIT) && (ret != ERR_MUTEX) && (ret != ERR_NOTRUN)) {
			fprintf(stderr, "enqueue queue ch_num=%d, sub_num=%d error %d\n", ch_num, sub_num, ret);
		}
		fprintf(stderr, "enqueue queue ch_num=%d, sub_num=%d error %d\n", ch_num, sub_num, ret);
		goto exit_free_audio_buf;
	}
	return 0;

exit_free_audio_buf:
	//put_video_frame(pb->video.qno, fs);
exit_free_as_buf:
	//free_bs_data(ch_num, sub_num, q);
	return ret;
}

void send_audio_proc(void)
{
	avbs_t *b;
	priv_avbs_t *pb;
	unsigned int ch_num,sub_num;

	char buf[ADATA_BUF_LENGTH];
	int G_len = 0;
	int max_len = AUDIO_SILC_SIZE*2;

	for(ch_num = 0; ch_num < 2; ch_num++){
		for(sub_num = 0; sub_num < 1; sub_num++){
			pb = &enc[ch_num].priv_bs[sub_num];
			b = &enc[ch_num].bs[sub_num];
			if(pb->play == 0 || (b->event != NONE_BS_EVENT)){
				continue;
			}
			if(adata_fifo_out(&adata_b[ch_num], buf, &G_len, max_len)){
				write_rtp_audio(ch_num, sub_num, buf, G_len);
			}
		}
	}

	return;
}


//#define AUDIO_SLICE     200
//#define AUDIO_SLICE_LEN 8*AUDIO_SLICE
#if 0
FILE *fp_audio = NULL;
int ll = 0;
#endif
void do_audio_proc(void)
{
	avbs_t *b;
	priv_avbs_t *pb;
	unsigned int ch_num,sub_num;
	int audio_type = 0;

#if 0
	struct timeval  tval;
	unsigned int tick,timelapse;
	//int ch_num,sub_num;
	static struct timeval last_tval = {0,0};
	static unsigned int audio_slice=0;
#endif
	int ret,i;
	int audio_len=0;
	int g711_len=0;
#if 0
	gettimeofday(&tval, NULL);
	timelapse = 1000000 * ( tval.tv_sec - last_tval.tv_sec ); // + tval.tv_usec - last_tval.tv_usec;
	if( timelapse )
		timelapse += tval.tv_usec + (1000000 - last_tval.tv_usec);
	else
		timelapse = tval.tv_usec - last_tval.tv_usec;

	printf("do_audio_proc: timelapse: %d.\n", timelapse);

	//	if (timelapse >= AUDIO_SLICE*1000 ) // AUDIO_SLICE msec
	//	{
#endif
#if 1
	ret = poll(&audio_poll_fds, 1, 3);
	if( ret == 0 ){
		return;
	}else if( ret < 0 ){
		printf("poll failed!\n");
		return;
	}

	if( audio_poll_fds.revents ){
		audio_len = read(audio_fd, (char *) audio_data, ablk_size);
	}else
		return;

	audio_poll_fds.revents = 0;
	if( audio_len <= 0 )
		return;
#endif
	if(NVR_send_status[2][0]){
			unsigned char saudio[audio_len];
			for(i=0;i<audio_len>>1;i+=2){
				saudio[i] = audio_data[i*2];
				saudio[i+1] = audio_data[i*2+1];
			}
			//printf("send_video_frame... audio_len = %d.\n", audio_len);
			send_video_frame(saudio, 2, 0, audio_len>>1, 0, 0, 0, 0); 
	}

	for(ch_num = 0; ch_num < 2; ch_num++){
		for(sub_num = 0; sub_num < 1; sub_num++){
			pb = &enc[ch_num].priv_bs[sub_num];
			b = &enc[ch_num].bs[sub_num];
			if(pb->play == 0 || (b->event != NONE_BS_EVENT)){
				continue;
			}

			audio_type = convert_gmss_audio_type(b->audio.enc_type);
			if(audio_type == GM_SS_TYPE_G711){
				g711_len = g711_encode_frame(g711_data, audio_data, audio_len,2);
//				printf("do_audio_proc: g711_len = %d.\n", g711_len);
//				adata_fifo_in(&adata_b[ch_num], g711_data, g711_len);

				write_rtp_audio(ch_num, sub_num, g711_data, g711_len);
#if 0
			if(ch_num==0)
			{
				if(fp_audio==NULL&&ll==0)
					fp_audio = fopen("/tmp/test_0.g711", "wb+");
				if(fp_audio==NULL){
					continue;
				}
				fwrite(g711_data, g711_len, 1, fp_audio);
				ll++;
				
				if(ll>100)
					fclose(fp_audio);
			}
#endif
			}
		}
	}

#if 0
	last_tval.tv_sec = tval.tv_sec;
	last_tval.tv_usec = tval.tv_usec;
	audio_slice+=AUDIO_SLICE_LEN;
	if(audio_slice>=AUDIO_SLICE_LEN*(1000/AUDIO_SLICE))
		audio_slice=0;
	//	} //if timelapse
#endif
}


static void pt_list(int qno)
{
	int num;
	frame_slot_t *fs;

	fprintf(stderr, "----------------- avail -------------------------\n");
	for(fs = frame_info[qno].avail, num=0; fs; fs = fs->next, num++) 
		fprintf(stderr, "num=%d, fs->prev=%p, fs=%p, fs->next=%p\n", num, fs->prev, fs, fs->next);
	fprintf(stderr, "----------------- used -------------------------\n");
	for(fs = frame_info[qno].used, num=0; fs; fs = fs->next, num++) 
		fprintf(stderr, "num=%d, fs->prev=%p, fs=%p, fs->next=%p\n", num, fs->prev, fs, fs->next);
}

static void dbg_rtsp(void)
{
	int ch_num, sub_num;
	av_t *e;
	avbs_t *b;
	priv_avbs_t *pb;
	
	/* public data initial */
	for(ch_num=0; ch_num < CHANNEL_NUM; ch_num++) {
		e = &enc[ch_num];
		fprintf(stderr, "----------------------------------------------\n");
		fprintf(stderr, "ch_num=%d, revent=%x, events=%x, fd=%d\n", ch_num, poll_fds[ch_num].revents, poll_fds[ch_num].events, poll_fds[ch_num].fd);
		fprintf(stderr, "----------------------------------------------\n");
		for(sub_num=0; sub_num < DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &e->bs[sub_num];
			pb = &e->priv_bs[sub_num];
			fprintf(stderr, "----------------------------------------------\n");
			fprintf(stderr, "sub_num=%d_%d: \n",ch_num, sub_num);
			fprintf(stderr, "----------------------------------------------\n");
			fprintf(stderr, "\tevent=%d, enable=%x, opt_type=%d\n", b->event, b->enabled, b->opt_type);
			fprintf(stderr, "\tvideo :enabled=%x, enc_type=%d, w=%d, h=%d, fps=%d, bps=%d\n", b->video.enabled, 
															b->video.enc_type, 
															b->video.width,
															b->video.height,
															b->video.fps,
															b->video.bps);
			fprintf(stderr, "\toffs=%x, sdpstr=%s, tick=%d, timed=%d, qno=%d, buf_usage=%d\n", pb->video.offs, 
										pb->video.sdpstr,
										pb->video.tick,
										pb->video.timed,
										pb->video.qno, 
										pb->video.buf_usage);
			fprintf(stderr, "\tplay=%d, congest=%d, sr=%d, name=%s, fd=%p\n", pb->play, 
										pb->congest, 
										pb->sr, 
										pb->name,
										pb->fd);
			if(pb->video.qno>=0) pt_list(pb->video.qno);
		}
	}
}	

static inline unsigned int timediff(unsigned int ptime, unsigned int ctime)
{
	unsigned int diff;

	if(ctime >= ptime ) diff = ctime - ptime;
	else diff = ((unsigned int)0xffffffff - ptime) + ctime;
	
	return diff;
}

static void show_enc_cfg(void)
{
	avbs_t *b;
	priv_avbs_t *pb;
	int ch_num, sub_num, is_enable;

//	printf("------------------------------------------------\n");
//	printf("\rinput_syst=%s\n", rcInSysString[input_system]);
	printf("------------------------------------------------\n");
	for(ch_num=0; ch_num <CHANNEL_NUM; ch_num++) {
    	is_enable = 0;
    	for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &enc[ch_num].bs[sub_num];
        	if(b->opt_type != OPT_NONE) { is_enable = 1; break; }
    	}
    	if(is_enable) printf("\r%02d  : input_system=%s, 3DN=%d, 3DI=%d\n", ch_num, 
    	                                    rcInSysString[enc[ch_num].input_system],
    	                                    enc[ch_num].denoise,
    	                                    enc[ch_num].de_interlace);
    	for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &enc[ch_num].bs[sub_num];
			if(b->opt_type == OPT_NONE) continue;
			pb = &enc[ch_num].priv_bs[sub_num];
			printf("\r%02d_%d: %s(%s):%s %02dfps, %dx%d, gop=%d\n",
							ch_num,
							sub_num,
							enc_type_def_str[b->video.enc_type],
							(b->enabled==DVR_ENC_EBST_ENABLE) ? "ON" : "OFF", 
							opt_type_def_str[b->opt_type],
							b->video.fps,
							b->video.width,
							b->video.height, 
							b->video.ip_interval);
            switch(b->video.enc_type) {
                case ENC_TYPE_H264:
                case ENC_TYPE_MPEG:
                    switch(b->video.rate_ctl_type) {
                        case RATE_CTL_CBR:
                	        printf("      rctype=%s, bs=%dk\n", 
                							rcCtlTypeString[b->video.rate_ctl_type], 
                							b->video.bps/1000);
                	        printf("      maxQ=%d, minQ=%d, iniQ=%d\n", 
                							b->video.max_quant,
                							b->video.min_quant,
                							b->video.init_quant);
                            break;
                        case RATE_CTL_VBR:
                	        printf("      rctype=%s\n", 
                							rcCtlTypeString[b->video.rate_ctl_type]);
                	        printf("      maxQ=%d, minQ=%d, iniQ=%d\n", 
                							b->video.max_quant,
                							b->video.min_quant,
                							b->video.init_quant);
                            break;
                        case RATE_CTL_ECBR:
                	        printf("      rctype=%s, bs=%dk, trm=%dk, rdm=%ds,\n", 
                							rcCtlTypeString[b->video.rate_ctl_type], 
                							b->video.bps/1000,
                							b->video.target_rate_max/1000, 
                							b->video.reaction_delay_max/1000);
                	        printf("      maxQ=%d, minQ=%d, iniQ=%d\n", 
                							b->video.max_quant,
                							b->video.min_quant,
                							b->video.init_quant);
                            break;
                        case RATE_CTL_EVBR:
                	        printf("      rctype=%s, trm=%dk\n", 
                							rcCtlTypeString[b->video.rate_ctl_type], 
                							b->video.target_rate_max/1000);
                	        printf("      maxQ=%d, minQ=%d, iniQ=%d\n", 
                							b->video.max_quant,
                							b->video.min_quant,
                							b->video.init_quant);
                            break;
                    }
                    break;
                case ENC_TYPE_MJPEG:
        	        printf("      mjQ=%d\n", b->video.mjpeg_quality);
                    break;
            }
            if(b->video.enabled_roi == 1) {
    	        printf("      ROI=ON (x=%d, y=%d, w=%d, h=%d)\n", 
    							b->video.roi_x,
    							b->video.roi_y,
    							b->video.roi_w,
    							b->video.roi_h);
			}
    	}
		printf("\n");
	}
}

#if PRINT_ENC_AVERAGE
static void print_enc_average(int tick)
{
	avbs_t *b;
	priv_avbs_t *pb;
	int ch_num, sub_num, diff_tv;
	static int average_tick=0;

	if(enable_print_average==0) return;
	diff_tv = timediff(average_tick, tick);
	if (diff_tv < TIME_INTERVAL) return;
	printf("------------------------------------------------\n");
	for(ch_num=0; ch_num <CHANNEL_NUM; ch_num++) {
    	for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &enc[ch_num].bs[sub_num];
			if(b->opt_type == OPT_NONE || b->video.enabled == DVR_ENC_EBST_DISABLE) continue;
			pb = &enc[ch_num].priv_bs[sub_num];
			printf("\r%02d_%d: %s:%s %2d.%02d_fps, %5d_kbps, %4dx%-4d\n",
							ch_num,
							sub_num,
							enc_type_def_str[b->video.enc_type],
							opt_type_def_str[b->opt_type],
							pb->video.avg_fps/100, pb->video.avg_fps%100,
							pb->video.avg_bps/1024,
							b->video.width,
							b->video.height);
    	}
		printf("\n");
	}
	average_tick = tick;
}
#endif

static char getch(void)
{
	int n = 1;
	unsigned char ch;
	struct timeval tv;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	n = select(1, &rfds, NULL, NULL, &tv);
	if (n > 0) {
		n = read(0, &ch, 1);
		if (n == 1)
			return ch;
		return n;
	}
	return -1;
}

static void set_sdpstr(char *sdpstr, int enc_type)
{
	// Not ready !
	switch(enc_type) {
	    case ENC_TYPE_MPEG:
        	// mpeg4, 352x240
        	strcpy(sdpstr, "000001B001000001B50900000100000001200086C400670C281078518F");
	        break;
	    case ENC_TYPE_MJPEG:
	        // mjpeg, 352x240
        	strcpy(sdpstr, "281e19");
	        break;
	    case ENC_TYPE_H264:
			//1920x1072
			strcpy(sdpstr, "Z0IAKOkA8AQ9cSAA27oAM3+YA2IEJQ==,aM4xUg==");
			break;
			//320x240
//			strcpy(sdpstr, "Z0IAHukCwS1xIADbugAzf5gDYgQl,aM4xUg==");
//			break;
	    default: 
        	// h264, 352x240
        	strcpy(sdpstr, "Z0IAKOkCw/I=,aM44gA==");
	        break;
	}
}

static inline int get_num_of_list(frame_slot_t **list)
{
	int num=0;
	frame_slot_t *fs;
	
	for(fs = *list; fs; fs = fs->next) num++;
	return num;
}

static inline void add_to_list(frame_slot_t *fs, frame_slot_t **list)
{
	fs->prev = NULL;
	fs->next = *list;
	if( fs->next ) fs->next->prev = fs;
	*list = fs;
}

static inline frame_slot_t *remove_from_listhead(frame_slot_t **list)
{
	frame_slot_t *fs;
	
	fs = *list;
	if(fs->next) fs->next->prev = NULL;
	*list = fs->next;
	return fs;
}

static frame_slot_t *search_from_list(int search_key, frame_slot_t **list)
{
	frame_slot_t *fs, *next, *ret_fs=NULL;
	
	for( fs = *list; fs; fs = next ) {
		next = fs->next;
		if( fs->vf->search_key == search_key ) {
			ret_fs = fs;
			break;
		}
	}
	return ret_fs;
}

static void remove_from_list(frame_slot_t *fs, frame_slot_t **list)
{
	if( fs->next ) fs->next->prev = fs->prev;
	if( fs->prev ) fs->prev->next = fs->next;
	else *list = fs->next;
}

static int init_video_frame(int qno)
{
	int i;
	frame_slot_t *fs;

	if(qno>=VQ_MAX || qno<0) {
		fprintf(stderr, "%s: qno=%d err.\n", __func__, qno);
		return -1;
	}
		
	if (pthread_mutex_init(&frame_info[qno].mutex, NULL)) {
		perror("init_video_frame: mutex init failed:");
		return -1;
	}
	frame_info[qno].avail = NULL;
	frame_info[qno].used = NULL;
	for(i=0; i<VIDEO_FRAME_NUMBER; i++) {
		if((fs = (frame_slot_t *) malloc(sizeof(frame_slot_t))) == NULL)
			goto err_exit;
		if((fs->vf = (video_frame_t *)malloc(sizeof(video_frame_t))) == NULL)
			goto err_exit;
		add_to_list(fs, &frame_info[qno].avail);
	}
	return 0;

err_exit: 
	fprintf(stderr, "%s: qno=%d, malloc failed!\n", __func__, qno);
	return -1;
}

static int get_num_of_used_video_frame(int qno)
{
	int num;
	
	pthread_mutex_lock( &frame_info[qno].mutex);
	num = get_num_of_list(&frame_info[qno].used);
	pthread_mutex_unlock(&frame_info[qno].mutex);
	return num;
}

static int free_video_frame(int qno)
{
	frame_slot_t *fs, *next;
	int i, ret=-1;

	pthread_mutex_lock( &frame_info[qno].mutex );
	for( fs = frame_info[qno].avail, i=0; fs; fs = next, i++) {
		next = fs->next;
		if(fs->vf) free(fs->vf);
		if(fs) free(fs);
		if(i >= VIDEO_FRAME_NUMBER) goto err_exit;
	}
	frame_info[qno].avail=NULL;
	frame_info[qno].used=NULL;
	
	if(i != VIDEO_FRAME_NUMBER) ret = -1;
	else ret = 0; 

err_exit:
	pthread_mutex_unlock(&frame_info[qno].mutex );
	pthread_mutex_destroy(&frame_info[qno].mutex);
	if(ret < 0) fprintf(stderr, "%s: qno=%d, i=%d free failed!\n", __func__, qno, i);
	return ret;
}

static frame_slot_t *search_video_frame(int qno, int search_key)
{
	frame_slot_t *fs;
	pthread_mutex_lock( &frame_info[qno].mutex );
	fs = search_from_list(search_key, &frame_info[qno].used);
	pthread_mutex_unlock( &frame_info[qno].mutex );
	return fs;
}

static frame_slot_t *take_video_frame(int qno)
{
	frame_slot_t *fs;
	
	if(qno>=VQ_MAX || qno<0) {
		fprintf(stderr, "%s: qno=%d err\n", __func__, qno);
		return NULL;
	}

	pthread_mutex_lock( &frame_info[qno].mutex );
	if((fs = remove_from_listhead(&frame_info[qno].avail)) == NULL) goto err_exit;
	add_to_list(fs, &frame_info[qno].used);
err_exit:
	pthread_mutex_unlock( &frame_info[qno].mutex );
	if(!fs) fprintf(stderr, "%s: qno=%d, video frame full.\n", __func__, qno);
	return fs;
}

static void put_video_frame(int qno, frame_slot_t *fs)
{
	if(qno>=VQ_MAX || qno<0 || fs==NULL) {
		fprintf(stderr, "%s: qno=%d err\n", __func__, qno);
		return;
	}
	
	pthread_mutex_lock(&frame_info[qno].mutex);
	remove_from_list(fs, &frame_info[qno].used);
	add_to_list(fs, &frame_info[qno].avail);
	pthread_mutex_unlock(&frame_info[qno].mutex);
	return;
}


int set_fps(int channel, int sub_channel, int val)
{
	av_t *e;
	dvr_enc_control	enc_update;
	FuncTag tag;

	CHECK_CHANNUM_AND_SUBNUM(channel, sub_channel);
	if ((val > 30) && (val < 4)) {
		printf("Invalid input value!\n");
		return -1;
	}

	e = &enc[channel];
	memset(&enc_update, 0x0, sizeof(dvr_enc_control));    
	enc_update.stream = sub_channel;
	enc_update.update_parm.stream_enable = 1;
	enc_update.update_parm.frame_rate = val;
	enc_update.update_parm.bit_rate = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.ip_interval = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.dim.width = GMVAL_DO_NOT_CARE;  
	enc_update.update_parm.dim.height = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.di_mode = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.mode = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.scale_indep = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.is_3DI = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.is_denoise = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.init_quant = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.max_quant = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.min_quant = GMVAL_DO_NOT_CARE;

	enc_update.command = ENC_UPDATE;
	ioctl(e->enc_fd, DVR_ENC_CONTROL, &enc_update);
	FN_RESET_TAG(&tag);
	FN_SET_REC_CH(&tag, channel);
	ioctl(rtspd_dvr_fd, DVR_COMMON_APPLY, &tag);

	return 0;
}

int set_ip_int(int channel, int sub_channel, int val)
{
	av_t *e;
	dvr_enc_control	enc_update;
	FuncTag tag;

	CHECK_CHANNUM_AND_SUBNUM(channel, sub_channel);
	if ((val > 150) && (val < 25)) {
		printf("Invalid input value!\n");
		return -1;
	}

	e = &enc[channel];
	memset(&enc_update, 0x0, sizeof(dvr_enc_control));    
	enc_update.stream = sub_channel;
	enc_update.update_parm.stream_enable = 1;
	enc_update.update_parm.frame_rate = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.bit_rate = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.ip_interval = val;
	enc_update.update_parm.dim.width = GMVAL_DO_NOT_CARE;  
	enc_update.update_parm.dim.height = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.di_mode = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.mode = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.scale_indep = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.is_3DI = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.is_denoise = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.init_quant = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.max_quant = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.min_quant = GMVAL_DO_NOT_CARE;

	enc_update.command = ENC_UPDATE;
	ioctl(e->enc_fd, DVR_ENC_CONTROL, &enc_update);
	FN_RESET_TAG(&tag);
	FN_SET_REC_CH(&tag, channel);
	ioctl(rtspd_dvr_fd, DVR_COMMON_APPLY, &tag);

	return 0;
}

//使用固定码率方式修改码率才有意义
int set_bps(int channel, int sub_channel, int val)
{
	av_t *e;
	dvr_enc_control	enc_update;
	FuncTag tag;

	CHECK_CHANNUM_AND_SUBNUM(channel, sub_channel);
	e = &enc[channel];

	memset(&enc_update, 0x0, sizeof(dvr_enc_control));    
	enc_update.stream = sub_channel;
	enc_update.update_parm.stream_enable = 1;
	enc_update.update_parm.frame_rate = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.bit_rate = val;
	enc_update.update_parm.ip_interval = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.dim.width = GMVAL_DO_NOT_CARE;  
	enc_update.update_parm.dim.height = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.di_mode = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.mode = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.scale_indep = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.is_3DI = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.src.is_denoise = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.init_quant = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.max_quant = GMVAL_DO_NOT_CARE;
	enc_update.update_parm.min_quant = GMVAL_DO_NOT_CARE;

	enc_update.command = ENC_UPDATE;
	ioctl(e->enc_fd, DVR_ENC_CONTROL, &enc_update);
	FN_RESET_TAG(&tag);
	FN_SET_REC_CH(&tag, channel);
	ioctl(rtspd_dvr_fd, DVR_COMMON_APPLY, &tag);

	return 0;
}


static int do_queue_alloc(int type)
{
	int	rc;
	do {
		rc = stream_queue_alloc(type);
	} while MUTEX_FAILED(rc);

	return rc;
}


static unsigned int get_tick(struct timeval *tv)
{
	struct timeval	t, *tval;
    __time_t 		tv_sec;
    unsigned int	tick;
/*
	if(tv==NULL){
		tval = &t;
		gettimeofday(tval, NULL);	
	}else{
		tval = tv;
	}

	tval = &t;
	gettimeofday(tval, NULL);
	tick = (unsigned int)(tval->tv_sec*1000.0+tval->tv_usec/1000.0);
	
	return tick;
*/

    if(tv == NULL) {
        tval = &t;
        gettimeofday(tval, NULL);
    } else {
        tval = tv;
    }
	tv_sec = tval->tv_sec-sys_sec;
	sys_tick += tv_sec*RTP_HZ;
	sys_sec = tval->tv_sec;
	tick = tval->tv_usec*(RTP_HZ/1000)/1000;
	tick += sys_tick;
	return tick;

}

static int convert_gmss_media_type(int type)
{
	int media_type;

	switch(type) {
	case ENC_TYPE_H264: media_type=GM_SS_TYPE_H264; break;
	case ENC_TYPE_MPEG: media_type=GM_SS_TYPE_MP4; break;
	case ENC_TYPE_MJPEG: media_type=GM_SS_TYPE_MJPEG; break;
	default: media_type  = -1; fprintf(stderr, "convert_gmss_media_type: type=%d, error!\n", type); break;
	}
	return media_type;	
}

void get_time_str(char *time_str)
{
	long ltime;	
	struct tm *newtime;
	
	time( &ltime );
	newtime = gmtime(&ltime);	
	sprintf(time_str,"%04d%02d%02d%02d%02d%02d",newtime->tm_year+1900,	
												newtime->tm_mon+1,
												newtime->tm_mday,
												newtime->tm_hour,
												newtime->tm_min,							
												newtime->tm_sec);
}

static int env_cfg_check_section(const char *section, fcfg_file_t * cfile)
{
    return(fcfg_check_section( section,  cfile));
}

static int env_cfg_get_value_4_erret(const char *section, const char *keyname, fcfg_file_t * cfile)
{
    int value, ret; 

    ret = fcfg_getint(section, keyname, &value, cfg_file);
    if(ret<=0) 
        value = GMVAL_DO_NOT_CARE;
    return(value);
}

static int env_cfg_get_value(const char *section, const char *keyname, fcfg_file_t * cfile)
{
    int value, ret; 

    ret = fcfg_getint(section, keyname, &value, cfg_file);
    if(ret<=0) {
        printf("<\"%s:%s:%d\" is not exist!>\n", section, keyname, ret);
        printf("Please copy file /dvr/ipc_conf/ipc_xxx.cfg to /mnt/mtd/ipc.cfg\n");
        exit(-1);
    }
    return(value);
}

static void env_cfg_init(void)
{
	int ch_num, sub_num;
	av_t *e;
	avbs_t *b;
	char tmp_str[32];
	FILE *encfile;
	av_t *av_cfg;
	avbs_t *avbs_cfg;
	char *cfg_file_name;

	Av_cfg_t stAv_file;

	cfg_file=fcfg_open(IPC_CFG_FILE , "r" ) ;
	if(cfg_file == NULL) {
		printf("please copy file /product/IPC-XXX/demo/ipc.cfg to /mnt/mtd/ipc.cfg\n");
		exit(-1);
	}

	/* public data initial */
	for(ch_num=0; ch_num < CHANNEL_NUM; ch_num++) {
		/*this is the default value for all paramers.*/
		e = &enc[ch_num];
		snprintf(tmp_str, 32, "enc_%d", ch_num);
		if(env_cfg_check_section(tmp_str, cfg_file) == 0) 
			continue;
		e->denoise=env_cfg_get_value(tmp_str, "temporal_denoise", cfg_file); 
		e->de_interlace=env_cfg_get_value(tmp_str, "de_interlace", cfg_file); 
		e->input_system = env_cfg_get_value(tmp_str, "input_system", cfg_file);
		for(sub_num=0; sub_num < DVR_ENC_REPD_BT_NUM; sub_num++) {
			NVR_send_status[ch_num][sub_num] = 0;//NVR发送标志清零
			
			b = &e->bs[sub_num];
			snprintf(tmp_str, 32, "enc_%d_%d", ch_num, sub_num);
			if(env_cfg_check_section(tmp_str, cfg_file) == 0) {
				b->opt_type = 0;
				b->enabled = DVR_ENC_EBST_DISABLE;
				b->video.enabled = DVR_ENC_EBST_DISABLE;
				b->audio.enabled = DVR_ENC_EBST_DISABLE;
				continue;
			}
			b->audio.bps=8000*8;
			b->opt_type=env_cfg_get_value(tmp_str, "opt_type", cfg_file);
			b->video.enc_type=env_cfg_get_value(tmp_str, "enc_type", cfg_file);
			b->audio.enc_type=(env_cfg_get_value(tmp_str, "audio_enc_type", cfg_file));
			b->video.fps=env_cfg_get_value(tmp_str, "fps", cfg_file);
			b->video.ip_interval= b->video.fps * 1;
			b->video.width=env_cfg_get_value(tmp_str, "width", cfg_file);
			b->video.height=env_cfg_get_value(tmp_str, "height", cfg_file);
			b->video.mjpeg_quality=env_cfg_get_value(tmp_str, "mjpeg_quality", cfg_file);
			b->video.rate_ctl_type=env_cfg_get_value(tmp_str, "rate_ctl_type", cfg_file); 
			b->video.max_quant=env_cfg_get_value(tmp_str, "max_quant", cfg_file); 
			b->video.min_quant=env_cfg_get_value(tmp_str, "min_quant", cfg_file); 
			b->video.init_quant=env_cfg_get_value(tmp_str, "init_quant", cfg_file); 
			b->video.bps=env_cfg_get_value(tmp_str, "kbps", cfg_file)*1000; 
			b->video.target_rate_max=env_cfg_get_value(tmp_str, "target_rate_max", cfg_file)*1000; 
			b->video.reaction_delay_max=env_cfg_get_value(tmp_str, "reaction_delay_max", cfg_file)*1000; 
//			b->video.enabled_snapshot = 1;// env_cfg_get_value(tmp_str, "enabled_snapshot", cfg_file); 
			b->video.enabled_roi=env_cfg_get_value_4_erret(tmp_str, "enabled_roi", cfg_file); 
			b->video.roi_x=env_cfg_get_value_4_erret(tmp_str, "roi_x", cfg_file); 
			b->video.roi_y=env_cfg_get_value_4_erret(tmp_str, "roi_y", cfg_file); 
			b->video.roi_w=env_cfg_get_value_4_erret(tmp_str, "roi_w", cfg_file); 
			b->video.roi_h=env_cfg_get_value_4_erret(tmp_str, "roi_h", cfg_file); 

			fprintf(stdout, ".......[%d]b->video.bps = %d .\n", sub_num, b->video.bps);
			fprintf(stdout, ".......[%d]b->video.fps = %d .\n", sub_num, b->video.fps);
		}

		if(ch_num==0){
			cfg_file_name = ENC_FILE(0);
		}else{
			cfg_file_name = ENC_FILE(1);
		}

		if(access(cfg_file_name, F_OK|W_OK|R_OK)>=0){
			fprintf(stdout, "enc file exist.\n");
			/*there is the config data file*/
			encfile = fopen(cfg_file_name, "r+");
			if(encfile==NULL){
				goto env_cfg_exit;
			}

			fread(&stAv_file, sizeof(Av_cfg_t), 1, encfile);
			fclose(encfile);

			e = &enc[stAv_file.chn];

			e->denoise = stAv_file.denoise;
			e->de_interlace = stAv_file.de_interlace;
			e->input_system = stAv_file.input_system;
			if(e->input_system>CMOS||e->input_system<NTSC){
				e->input_system = CMOS;
			}
			for(sub_num=0; sub_num<MAX_UPDATE_SUB_CHN; sub_num++){
				b = &e->bs[sub_num];
				if(stAv_file.ubs[sub_num].stream_enable){
					b->opt_type = 1;
				} else {
					b->opt_type = 0;
					b->video.enabled = DVR_ENC_EBST_DISABLE;
					b->enabled = DVR_ENC_EBST_DISABLE;
					continue;
				}
				b->audio.bps=8000*8;
				b->audio.enc_type = 0;
				b->video.enc_type = 0;//stAv_file.ubs[sub_num].enc_type;
				b->video.fps = (stAv_file.ubs[sub_num].frame_rate>25)?(25):(stAv_file.ubs[sub_num].frame_rate);
				b->video.bps = stAv_file.ubs[sub_num].bit_rate;
				b->video.ip_interval = (stAv_file.ubs[sub_num].ip_interval>(2*b->video.fps))?(2*b->video.fps):(stAv_file.ubs[sub_num].ip_interval);
				b->video.width = stAv_file.ubs[sub_num].width;
				b->video.height = stAv_file.ubs[sub_num].height;
				b->video.rate_ctl_type = (stAv_file.ubs[sub_num].rate_ctl_type==0)?(0):(3);//CBR OR EVBR
				b->video.target_rate_max = stAv_file.ubs[sub_num].target_rate_max;
				b->video.reaction_delay_max = stAv_file.ubs[sub_num].reaction_delay_max;
				b->video.init_quant = stAv_file.ubs[sub_num].init_quant;
				b->video.max_quant = stAv_file.ubs[sub_num].max_quant;
				b->video.min_quant = stAv_file.ubs[sub_num].min_quant;
//				b->video.enabled_snapshot = 1;
				b->video.mjpeg_quality = stAv_file.ubs[sub_num].mjpeg_quality;
				b->video.enabled_roi = stAv_file.ubs[sub_num].enabled_roi;
				b->video.roi_x = stAv_file.ubs[sub_num].roi_x;
				b->video.roi_y = stAv_file.ubs[sub_num].roi_y;
				b->video.roi_w = stAv_file.ubs[sub_num].roi_w;
				b->video.roi_h = stAv_file.ubs[sub_num].roi_h;
			}

		}else{
			fprintf(stdout, "enc file not exist.\n");
			stAv_file.chn = ch_num;
			stAv_file.denoise = e->denoise;
			stAv_file.de_interlace = e->de_interlace;
			stAv_file.input_system = e->input_system;
			for(sub_num=0; sub_num<MAX_UPDATE_SUB_CHN; sub_num++){
				b = &e->bs[sub_num];
				fprintf(stdout, "sub_num=%d.\n", sub_num);
				if(b->opt_type!=0){
					fprintf(stdout, "enable[%d].\n", sub_num);
					stAv_file.ubs[sub_num].stream_enable = 1;
				}else{
					fprintf(stdout, "disable[%d].\n", sub_num);
					stAv_file.ubs[sub_num].stream_enable = 0;
					continue;
				}
				
				stAv_file.ubs[sub_num].enc_type = b->video.enc_type;
				stAv_file.ubs[sub_num].frame_rate = b->video.fps;
				stAv_file.ubs[sub_num].bit_rate = b->video.bps;
				stAv_file.ubs[sub_num].ip_interval = b->video.ip_interval;
				stAv_file.ubs[sub_num].width = b->video.width;
				stAv_file.ubs[sub_num].height = b->video.height;
				stAv_file.ubs[sub_num].rate_ctl_type = b->video.rate_ctl_type;
				stAv_file.ubs[sub_num].target_rate_max = b->video.target_rate_max;
				stAv_file.ubs[sub_num].reaction_delay_max = b->video.reaction_delay_max;
				stAv_file.ubs[sub_num].init_quant = b->video.init_quant;
				stAv_file.ubs[sub_num].max_quant = b->video.max_quant;
				stAv_file.ubs[sub_num].min_quant = b->video.min_quant;
				stAv_file.ubs[sub_num].mjpeg_quality = b->video.mjpeg_quality;
				stAv_file.ubs[sub_num].enabled_roi = b->video.enabled_roi;
				stAv_file.ubs[sub_num].roi_x = b->video.roi_x;
				stAv_file.ubs[sub_num].roi_y = b->video.roi_y;
				stAv_file.ubs[sub_num].roi_w = b->video.roi_w;
				stAv_file.ubs[sub_num].roi_h = b->video.roi_h;

			fprintf(stdout, "[%d]b->video.bps = %d .\n", sub_num, b->video.bps);
			fprintf(stdout, "[%d]b->video.fps = %d .\n", sub_num, b->video.fps);
			fprintf(stdout, "stAv_file_%d.ubs[%d].bit_rate = %d .\n", ch_num, sub_num, stAv_file.ubs[0].bit_rate);
			fprintf(stdout, "stAv_file_%d.ubs[%d].frame_rate = %d .\n", ch_num, sub_num, stAv_file.ubs[0].frame_rate);
			}

			//若系统中不存在配置文件，将默认参数写入配置文件
			encfile = fopen(cfg_file_name, "w+");
			if(NULL==encfile){
				goto env_cfg_exit;
			}
			fwrite(&stAv_file, sizeof(Av_cfg_t), 1, encfile);
			fclose(encfile);
		}
		
		if(ch_num==0){
			fprintf(stdout, "init enc 0 file.\n");
			memcpy(&g_stAv_0_file, &stAv_file, sizeof(Av_cfg_t));		
			fprintf(stdout, "stAv_file_0.ubs[0].bit_rate = %d .\n", stAv_file.ubs[0].bit_rate);
			fprintf(stdout, "stAv_file_0.ubs[0].frame_rate = %d .\n", stAv_file.ubs[0].frame_rate);
			fprintf(stdout, "g_stAv_0_file.ubs[0].bit_rate = %d .\n", g_stAv_0_file.ubs[0].bit_rate);
			fprintf(stdout, "g_stAv_0_file.ubs[0].frame_rate = %d .\n", g_stAv_0_file.ubs[0].frame_rate);
		}else/* if(ch_num==1)*/{
			fprintf(stdout, "init enc 1 file.\n");
			memcpy(&g_stAv_1_file, &stAv_file, sizeof(Av_cfg_t));	
			fprintf(stdout, "stAv_file_1.ubs[0].bit_rate = %d .\n", stAv_file.ubs[0].bit_rate);
			fprintf(stdout, "stAv_file_1.ubs[0].frame_rate = %d .\n", stAv_file.ubs[0].frame_rate);
			fprintf(stdout, "g_stAv_1_file.ubs[0].bit_rate = %d .\n", g_stAv_1_file.ubs[0].bit_rate);
			fprintf(stdout, "g_stAv_1_file.ubs[0].frame_rate = %d .\n", g_stAv_1_file.ubs[0].frame_rate);
		}

	}

env_cfg_exit:
    fcfg_close(cfg_file);
}


static void env_enc_init(void)
{
	int ch_num, sub_num;
	av_t *e;
	avbs_t *b;
	priv_avbs_t *pb;

	memset(enc,0,sizeof(enc));
	e = &enc[0];
	b = &e->bs[0];
    b->opt_type = RTSP_LIVE_STREAMING;
   	b->video.enc_type = ENC_TYPE_H264;
    b->video.bps=2000000;  /* bitrate 2M */
	b->event = NONE_BS_EVENT;
	b->enabled = DVR_ENC_EBST_DISABLE;
	b->video.enabled = DVR_ENC_EBST_DISABLE;
	b->video.width=1280;
	b->video.height=720;
	b->video.fps=30;
	b->video.ip_interval = b->video.fps *1;
	b->video.rate_ctl_type = RATE_CTL_ECBR;
	b->video.target_rate_max = 4*1024*1024;
	b->video.reaction_delay_max = 10000;
	b->video.max_quant = 51;
	b->video.min_quant = 1;
	b->video.init_quant = 30;
	/*add by Aaron.qiu 2012.02.20*/
	b->video.enabled_snapshot = 1;	//enable snapshot
	/*------------------------------*/
	for(ch_num=1; ch_num < CHANNEL_NUM; ch_num++) {
		e = &enc[ch_num];
    		if (pthread_mutex_init(&e->ubs_mutex, NULL)) {
    			perror("env_enc_init: mutex init failed:");
    			exit(-1);
    		}
		for(sub_num=0; sub_num < DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &e->bs[sub_num];
			b->enabled = DVR_ENC_EBST_DISABLE;
        	b->video.enabled = DVR_ENC_EBST_DISABLE;
		}
    }
	/* private data initial */
	for(ch_num=0; ch_num < CHANNEL_NUM; ch_num++) {
		e = &enc[ch_num];
		e->enc_fd = -1;
		for(sub_num=0; sub_num < DVR_ENC_REPD_BT_NUM; sub_num++) {
			pb = &e->priv_bs[sub_num];
			pb->video.qno = -1;
			pb->audio.qno = -1;
			pb->sr = -1;
		}
	}
}

static inline int get_bitrate(int width, int height, int fps)
{
	int bitrate;

	bitrate = (width*height)/256 * MACRO_BLOCK_SIZE * fps;
	return bitrate;
}

static inline int open_channel(int ch_num)
{
	int enc_fd;
	if(enc[ch_num].enabled == DVR_ENC_EBST_ENABLE) {
		fprintf(stderr, "open_channel: ch_num=%d already enabled, error!\n", ch_num);
		return -1;
	}
	
    if((enc_fd=open("/dev/dvr_enc", O_RDWR)) < 0) {
        perror("RTSP open_channel [/dev/dvr_enc] failed:");
        return -1;
    }
	enc[ch_num].enabled = DVR_ENC_EBST_ENABLE;
//	if (rtspd_dvr_fd == 0) {
//	    rtspd_dvr_fd = open("/dev/dvr_common", O_RDWR);   //open_dvr_common
//	}
	return enc_fd;
}

static inline int close_channel(int ch_num)
{
    int i, is_close_dvr_fd=1;
    
	munmap((void*)enc[ch_num].v_bs_mbuf_addr, enc[ch_num].v_bs_mbuf_size);
    close(enc[ch_num].enc_fd);
	enc[ch_num].enc_fd = -1;
	enc[ch_num].enabled = DVR_ENC_EBST_DISABLE;
    for(i=0; i< CHANNEL_NUM; i++ ) {
        if(enc[i].enabled == DVR_ENC_EBST_ENABLE) {
            is_close_dvr_fd = 0;
            break;
        }
    }
    if(is_close_dvr_fd) {
        close(rtspd_dvr_fd);      //close_dvr_common
        rtspd_dvr_fd = 0;
    }
    return 0;
}

static inline int set_bs_res(int ch_num, int sub_num, int width, int height)
{
	av_t *e;

	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	if((width%16) || (height%16)) {
		fprintf(stderr, "set_bs_res: width=%d, height=%d error!\n", width, height);
		return -1;
	}
	e = &enc[ch_num];
	e->bs[sub_num].video.width = width;
	e->bs[sub_num].video.height = height;
	return 0;
}

static inline int set_bs_fps(int ch_num, int sub_num, int fps)
{
	av_t *e;
	
	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	if(fps > 30 || fps <= 0) {
		fprintf(stderr, "set_bs_fps: fps=%d error!\n", fps);
		return -1;
	}
	e = &enc[ch_num];
	e->bs[sub_num].video.fps = fps;
	return 0;
}

#if 0
static int set_bs_rc_evbr(int ch_num, int sub_num, int max_bps, int intQ)
{
	av_t *e;
	
	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	if(intQ <= 0 || intQ > 51) {
		fprintf(stderr, "%s: intQ=%d error!\n", __FUNCTION__, intQ);
		return -1;
	}
	if(max_bps < 0) {
		fprintf(stderr, "%s: bps=%d error!\n", __FUNCTION__, max_bps);
		return -1;
	}
	e = &enc[ch_num];
	e->bs[sub_num].video.target_rate_max = max_bps;
	e->bs[sub_num].video.reaction_delay_max = 0;
	e->bs[sub_num].video.max_quant = intQ;
	e->bs[sub_num].video.min_quant= intQ;
	e->bs[sub_num].video.init_quant= intQ;
	e->bs[sub_num].video.rate_ctl_type = RATE_CTL_EVBR;
	return 0;
}

static int set_bs_rc_ecbr(int ch_num, int sub_num, int bps, int max_bps, int reacDyMax)
{
	av_t *e;
	
	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	if((max_bps < 0) || (bps < 0)) {
		fprintf(stderr, "%s: bps=%d, max_bps=%d error!\n", __FUNCTION__, bps, max_bps);
		return -1;
	}
	if(reacDyMax < 0) {
		fprintf(stderr, "%s: reaction_delay_max=%d error!\n", __FUNCTION__, reacDyMax);
		return -1;
	}
		
	e = &enc[ch_num];
	e->bs[sub_num].video.bps = bps;
	e->bs[sub_num].video.target_rate_max = max_bps;
	e->bs[sub_num].video.reaction_delay_max = reacDyMax;  //unit: milliseconds
	e->bs[sub_num].video.max_quant = 51;
	e->bs[sub_num].video.min_quant= 1;
	e->bs[sub_num].video.init_quant= 25;
	e->bs[sub_num].video.rate_ctl_type = RATE_CTL_ECBR;
	return 0;
}

static int set_bs_rc_vbr(int ch_num, int sub_num, int intQ)
{
	av_t *e;
	
	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	if(intQ <= 0 || intQ > 51) {
		fprintf(stderr, "%s: intQ=%d error!\n", __FUNCTION__, intQ);
		return -1;
	}
	e = &enc[ch_num];
	e->bs[sub_num].video.target_rate_max = 0;
	e->bs[sub_num].video.reaction_delay_max = 0;  //unit: milliseconds
	e->bs[sub_num].video.max_quant = intQ;
	e->bs[sub_num].video.min_quant= intQ;
	e->bs[sub_num].video.init_quant= intQ;
	e->bs[sub_num].video.rate_ctl_type = RATE_CTL_VBR;
	return 0;
}

static int set_bs_rc_cbr(int ch_num, int sub_num, int bps)
{
	av_t *e;
	
	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	if(bps < 0) {
		fprintf(stderr, "%s: bps=%d error!\n", __FUNCTION__, bps);
		return -1;
	}
	e = &enc[ch_num];
	e->bs[sub_num].video.bps = bps;
	e->bs[sub_num].video.target_rate_max = 0;
	e->bs[sub_num].video.reaction_delay_max = 0;  //unit: milliseconds
	e->bs[sub_num].video.max_quant = 51;
	e->bs[sub_num].video.min_quant= 1;
	e->bs[sub_num].video.init_quant= 25;
	e->bs[sub_num].video.rate_ctl_type = RATE_CTL_CBR;
	return 0;
}
#endif

static inline int set_bs_bps(int ch_num, int sub_num, int bps, int rate_ctl_type)
{
	av_t *e;
	
	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	if(bps < 0) {
		fprintf(stderr, "set_bs_bps: bps=%d error!\n", bps);
		return -1;
	}
	e = &enc[ch_num];
	e->bs[sub_num].video.bps = bps;
	e->ubs[sub_num].bit_rate = bps;
	return 0;
}

static inline int set_bs_opt_type(int ch_num, int sub_num, int opt_type)
{
	avbs_t *b;
	
	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	b = &enc[ch_num].bs[sub_num];
	b->opt_type = opt_type;
	return 0;
}

static void set_tag(int ch_num, int sub_num, FuncTag *tag)
{
    FN_RESET_TAG(tag);
	switch(sub_num) {
		case MAIN_BS_NUM: 
			FN_SET_REC_CH(tag, ch_num); 
			break;
		case SUB1_BS_NUM: 
			FN_SET_SUB1_REC_CH(tag, ch_num); 
			break;
		case SUB2_BS_NUM: 
			FN_SET_SUB2_REC_CH(tag, ch_num); 
			break;
		case SUB3_BS_NUM: 
			FN_SET_SUB3_REC_CH(tag, ch_num); 
			break;
		case SUB4_BS_NUM: 
			FN_SET_SUB4_REC_CH(tag, ch_num); 
			break;
		case SUB5_BS_NUM: 
			FN_SET_SUB5_REC_CH(tag, ch_num); 
			break;
		case SUB6_BS_NUM: 
			FN_SET_SUB6_REC_CH(tag, ch_num); 
			break;
		case SUB7_BS_NUM: 
			FN_SET_SUB7_REC_CH(tag, ch_num); 
			break;
		case SUB8_BS_NUM: 
			FN_SET_SUB8_REC_CH(tag, ch_num); 
			break;
		default: 
			printf("%s: ch_num=%d, sub_num %d failed!\n", __FUNCTION__, ch_num, sub_num); 
	}
}

static inline void cfgud_init(int ch_num, int sub_num)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->stream_enable = GMVAL_DO_NOT_CARE; 
    ubs->enc_type = GMVAL_DO_NOT_CARE; 
    ubs->frame_rate = GMVAL_DO_NOT_CARE; 
    ubs->bit_rate = GMVAL_DO_NOT_CARE; 
    ubs->ip_interval = GMVAL_DO_NOT_CARE; 
    ubs->width = GMVAL_DO_NOT_CARE; 
    ubs->height = GMVAL_DO_NOT_CARE; 
    ubs->rate_ctl_type = GMVAL_DO_NOT_CARE; 
    ubs->target_rate_max = GMVAL_DO_NOT_CARE; 
    ubs->reaction_delay_max = GMVAL_DO_NOT_CARE; 
    ubs->init_quant = GMVAL_DO_NOT_CARE; 
    ubs->max_quant = GMVAL_DO_NOT_CARE; 
    ubs->min_quant = GMVAL_DO_NOT_CARE; 
    ubs->mjpeg_quality = GMVAL_DO_NOT_CARE; 
    ubs->enabled_roi = GMVAL_DO_NOT_CARE; 
    ubs->roi_y = GMVAL_DO_NOT_CARE; 
    ubs->roi_x = GMVAL_DO_NOT_CARE; 
    ubs->roi_w = GMVAL_DO_NOT_CARE; 
    ubs->roi_h = GMVAL_DO_NOT_CARE; 
}

static inline void cfgud_stream_enable(int ch_num, int sub_num, int stream_enable)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->stream_enable = stream_enable;
}

static inline void cfgud_mjpeg_quality(int ch_num, int sub_num, int mjpeg_quality)
{
    update_avbs_t *ubs;

	if(mjpeg_quality<1 || mjpeg_quality >100) {
	    printf("mjpeg quality fail!\n");
	    printf("mjpeg quality val 1~100, input val=%d\n", mjpeg_quality);
	}
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->mjpeg_quality = mjpeg_quality; 
}

static inline void cfgud_rate_ctl_type(int ch_num, int sub_num, int rate_ctl_type)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->rate_ctl_type = rate_ctl_type;
}

static inline void cfgud_bps(int ch_num, int sub_num, int bit_rate)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->bit_rate = bit_rate;
}

static inline void cfgud_trm(int ch_num, int sub_num, int target_rate_max)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->target_rate_max = target_rate_max;
}

static inline void cfgud_rdm(int ch_num, int sub_num, int reaction_delay_max)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->reaction_delay_max = reaction_delay_max;
}

static inline int cfgud_enc_type(int ch_num, int sub_num, int enc_type)
{
    update_avbs_t *ubs;

	if(enc_type < ENC_TYPE_H264 || enc_type >= ENC_TYPE_YUV422) {
	    printf("%s: ch=%d sub=%d, Input codec (%d) is illegal. (0:H264, 1:MPEG4, 2:MJPEG)\n",
	                            __FUNCTION__,
	                            ch_num,
	                            sub_num,
	                            enc_type);
        goto err_exit;
    }
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->enc_type = enc_type;
    return 0;

err_exit: 
    return -1;
}

static inline void cfgud_iniQ(int ch_num, int sub_num, int enc_type, int init_quant)
{
    update_avbs_t *ubs;
	switch (enc_type) {
		case ENC_TYPE_H264:
        	if ((init_quant > 51) || (init_quant < 1)) {
        	    printf("%s: ch=%d sub=%d, Input init quant (%d) is illegal. (h264 max:51,min:1)\n",
        	                            __FUNCTION__,
        	                            ch_num,
        	                            sub_num,
        	                            init_quant);
        	    goto err_exit;
            }
            break;
		case ENC_TYPE_MPEG:
        	if ((init_quant > 31) || (init_quant < 1)) {
        	    printf("%s: ch=%d sub=%d, Input init quant (%d) is illegal. (mpeg4 max:31,min:1)\n",
        	                            __FUNCTION__,
        	                            ch_num,
        	                            sub_num,
        	                            init_quant);
        	    goto err_exit;
            }
            break;
		case ENC_TYPE_MJPEG:
        default:
    	    printf("%s: ch=%d sub=%d, Codec (%d) is illegal.)\n",
    	                            __FUNCTION__,
    	                            ch_num,
    	                            sub_num,
    	                            enc_type);
            goto err_exit;
    }	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->init_quant = init_quant;
    return;

err_exit:
    return;
}

static inline void cfgud_maxQ(int ch_num, int sub_num, int enc_type, int max_quant)
{
    update_avbs_t *ubs;
	switch (enc_type) {
		case ENC_TYPE_H264:
        	if ((max_quant > 51) || (max_quant < 1)) {
        	    printf("%s: ch=%d sub=%d, Input max quant (%d) is illegal. (h264 max:51,min:1)\n",
        	                            __FUNCTION__,
        	                            ch_num,
        	                            sub_num,
        	                            max_quant);
        	    goto err_exit;
            }
            break;
		case ENC_TYPE_MPEG:
        	if ((max_quant > 31) || (max_quant < 1)) {
        	    printf("%s: ch=%d sub=%d, Input max quant (%d) is illegal. (mpeg4 max:31,min:1)\n",
        	                            __FUNCTION__,
        	                            ch_num,
        	                            sub_num,
        	                            max_quant);
        	    goto err_exit;
            }
            break;
		case ENC_TYPE_MJPEG:
        default:
    	    printf("%s: ch=%d sub=%d, Codec (%d) is illegal.)\n",
    	                            __FUNCTION__,
    	                            ch_num,
    	                            sub_num,
    	                            enc_type);
            goto err_exit;
    }	
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->max_quant = max_quant;
    return;

err_exit:
    return;
}

static inline void cfgud_minQ(int ch_num, int sub_num, int enc_type, int min_quant)
{
    update_avbs_t *ubs;
	switch (enc_type) {
		case ENC_TYPE_H264:
        	if ((min_quant > 51) || (min_quant < 1)) {
        	    printf("%s: ch=%d sub=%d, Input min quant (%d) is illegal. (h264 max:51,min:1)\n",
        	                            __FUNCTION__,
        	                            ch_num,
        	                            sub_num,
        	                            min_quant);
        	    goto err_exit;
            }
            break;
		case ENC_TYPE_MPEG:
        	if ((min_quant > 31) || (min_quant < 1)) {
        	    printf("%s: ch=%d sub=%d, Input min quant (%d) is illegal. (mpeg4 max:31,min:1)\n",
        	                            __FUNCTION__,
        	                            ch_num,
        	                            sub_num,
        	                            min_quant);
        	    goto err_exit;
            }
            break;
		case ENC_TYPE_MJPEG:
        default:
    	    printf("%s: ch=%d sub=%d, Codec (%d) is illegal.)\n",
    	                            __FUNCTION__,
    	                            ch_num,
    	                            sub_num,
    	                            enc_type);
            goto err_exit;
    }	
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->min_quant = min_quant;
    return;

err_exit:
    return;
}

static inline int cfgud_res(int ch_num, int sub_num, int width, int height)
{
    update_avbs_t *ubs;
	
	if((width%16) || (height%16)) {
		fprintf(stderr, "set_bs_res: width=%d, height=%d error!\n", width, height);
		return -1;
	}

	ubs = &enc[ch_num].ubs[sub_num];
    ubs->width= width;
    ubs->height = height;
    return 0;
}

static inline void cfgud_fps(int ch_num, int sub_num, int fps)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->frame_rate = fps;
}

static inline void cfgud_enabled_roi(int ch_num, int sub_num, int enabled_roi)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->enabled_roi= enabled_roi;
}

static inline void cfgud_roi_x(int ch_num, int sub_num, int roi_x)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->roi_x= roi_x;
}

static inline void cfgud_roi_y(int ch_num, int sub_num, int roi_y)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->roi_y= roi_y;
}

static inline void cfgud_roi_w(int ch_num, int sub_num, int roi_w)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->roi_w= roi_w;
}

static inline void cfgud_roi_h(int ch_num, int sub_num, int roi_h)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->roi_h= roi_h;
}

static inline void cfgud_ip_interval(int ch_num, int sub_num, int ip_interval)
{
    update_avbs_t *ubs;
	
	ubs = &enc[ch_num].ubs[sub_num];
    ubs->ip_interval = ip_interval;
}

#define NEW_ROI_FUNC
static int cfgud_chk_parm(int ch_num, int sub_num)
{
    update_avbs_t *ubs;
	avbs_t *b;
	
	ubs = &enc[ch_num].ubs[sub_num];
	b = &enc[ch_num].bs[sub_num];

	if(b->opt_type == OPT_NONE) {
	    printf("This channel %d:%d is disable that cannot be change parameter.\n", ch_num, sub_num);
	    goto err_exit;
	}

	if(ubs->stream_enable != GMVAL_DO_NOT_CARE) {
	    if(ubs->stream_enable) {
    	    b->video.enabled = DVR_ENC_EBST_ENABLE;
    	    b->enabled = DVR_ENC_EBST_ENABLE;
    	} else {
    	    b->video.enabled = DVR_ENC_EBST_DISABLE;
    	    b->enabled = DVR_ENC_EBST_DISABLE;
    	}
	}
	if(ubs->enc_type != GMVAL_DO_NOT_CARE) {
	    b->video.enc_type = ubs->enc_type;
	}
	if(ubs->frame_rate != GMVAL_DO_NOT_CARE) {
	    b->video.fps = ubs->frame_rate;
	}
	if(ubs->bit_rate != GMVAL_DO_NOT_CARE) {
	    b->video.bps = ubs->bit_rate;
	}
	if(ubs->ip_interval != GMVAL_DO_NOT_CARE) {
	    b->video.ip_interval = ubs->ip_interval;
	}
	if(ubs->width != GMVAL_DO_NOT_CARE) {
	    b->video.width = ubs->width;
	}
	if(ubs->height != GMVAL_DO_NOT_CARE) {
	    b->video.height = ubs->height;
	}
	if(ubs->rate_ctl_type != GMVAL_DO_NOT_CARE) {
	    b->video.rate_ctl_type = ubs->rate_ctl_type;
	}
	if(ubs->target_rate_max != GMVAL_DO_NOT_CARE) {
	    b->video.target_rate_max = ubs->target_rate_max;
	}
	if(ubs->reaction_delay_max != GMVAL_DO_NOT_CARE) {
	    b->video.reaction_delay_max = ubs->reaction_delay_max;
	}
	if(ubs->init_quant != GMVAL_DO_NOT_CARE) {
	    b->video.init_quant = ubs->init_quant;
	}
	if(ubs->max_quant != GMVAL_DO_NOT_CARE) {
	    b->video.max_quant = ubs->max_quant;
	}
	if(ubs->min_quant != GMVAL_DO_NOT_CARE) {
	    b->video.min_quant = ubs->min_quant;
	}
	if(ubs->mjpeg_quality != GMVAL_DO_NOT_CARE) {
	    b->video.mjpeg_quality = ubs->mjpeg_quality;
	}
#ifdef NEW_ROI_FUNC
	if(ubs->enabled_roi != GMVAL_DO_NOT_CARE) {
	    b->video.enabled_roi = ubs->enabled_roi;
	}
	if(ubs->roi_w != GMVAL_DO_NOT_CARE) {
	    b->video.roi_w = ubs->roi_w;
	}
	if(ubs->roi_h != GMVAL_DO_NOT_CARE) {
	    b->video.roi_h = ubs->roi_h;
	}
#endif
	if(ubs->roi_x != GMVAL_DO_NOT_CARE) {
	    b->video.roi_x = ubs->roi_x;
	}
	if(ubs->roi_y != GMVAL_DO_NOT_CARE) {
	    b->video.roi_y = ubs->roi_y;
	}

	return 0;
err_exit: 
    return -1; 
}


static int set_bs_intra_frame(int ch_num, int sub_num)
{
	av_t *e;
	
	e = &enc[ch_num];

	if(ioctl(e->enc_fd, DVR_ENC_RESET_INTRA, &sub_num)) {
	    perror("set_bs_intra_frame : error to use DVR_ENC_RESET_INTRA\n");
        return -1;
	}
	return 0;
}

int set_I_frame(int ch_num)
{
	if(ch_num<0||ch_num>1){
		return -1;
	}

	if(set_bs_intra_frame(ch_num, 0)){
		return -2;
	}

	return 0;
}

static int apply_bs_ensnap(int ch_num, int Num)
{
	av_t *e;
	dvr_enc_control enc_ctrl;
	int N = Num;

	e = &enc[ch_num];
	enc_ctrl.command = ENC_SNAP;
	enc_ctrl.output.count = (int *)N;
	ioctl(e->enc_fd, DVR_ENC_CONTROL, &enc_ctrl);

}

static int apply_bs(int ch_num, int sub_num, int command)
{
	av_t *e;
    FuncTag tag;
    dvr_enc_control enc_ctrl;
	
	enc_ctrl.command = command;
	enc_ctrl.stream = sub_num;
	e = &enc[ch_num];
	if(ioctl(e->enc_fd, DVR_ENC_CONTROL, &enc_ctrl)<0) {
	    perror("apply_bs : error to use DVR_ENC_CONTROL\n");
        return -1;
	}
    set_tag(ch_num, sub_num, &tag);
	if(ioctl(rtspd_dvr_fd, DVR_COMMON_APPLY, &tag)<0) {
	    perror("apply_bs: Error to use DVR_COMMON_APPLY\n");
        return -1;
	}
	return 0;
}


int is_bs_all_disable(void)
{
	av_t *e;
    int ch_num, sub_num;

    for(ch_num=0; ch_num < CHANNEL_NUM; ch_num++) {
	    e = &enc[ch_num];
        for(sub_num=0; sub_num < DVR_ENC_REPD_BT_NUM; sub_num++) {
        	if(e->bs[sub_num].enabled == DVR_ENC_EBST_ENABLE) return 0;  /* already enabled */
        }
    }
    return 1;
}

static inline int check_bs_param(int ch_num, int sub_num)
{
	av_t *e;
	int fps, width, height, enc_type, opt_type, bps;
	
	if((ch_num >= CHANNEL_NUM || ch_num < 0) || 
		(sub_num >= DVR_ENC_REPD_BT_NUM || sub_num < 0)) {
		fprintf(stderr, "check_bs_param: ch_num=%d, sub_num=%d error!\n", ch_num, sub_num);
		return -1;
	}
	
	e = &enc[ch_num];
	fps = e->bs[sub_num].video.fps;
	bps = e->bs[sub_num].video.bps;
	width = e->bs[sub_num].video.width;
	height = e->bs[sub_num].video.height;
	enc_type = e->bs[sub_num].video.enc_type;
	opt_type = e->bs[sub_num].opt_type;

	if((opt_type < RTSP_LIVE_STREAMING) || (opt_type>=OPT_END)) {
		fprintf(stderr, "check_bs_param: ch_num=%d, sub_num=%d, opt_type=%d error!\n", ch_num, sub_num, opt_type);
		return -1;
	}
	if((enc_type < ENC_TYPE_H264) || (enc_type > ENC_TYPE_MJPEG)) {
		fprintf(stderr, "check_bs_param: ch_num=%d, sub_num=%d, enc_type=%d error!\n", ch_num, sub_num, enc_type);
		return -1;
	}
	if(fps > 30 || fps <= 0) {
		fprintf(stderr, "check_bs_param: ch_num=%d, sub_num=%d, fps=%d error!\n", ch_num, sub_num, fps);
		return -1;
	}
	if(bps < 0) {
		fprintf(stderr, "check_bs_param: ch_num=%d, sub_num=%d, bps=%d error!\n", ch_num, sub_num, bps);
		return -1;
	}
//++ fullhd
#if 0
	if((width%16) || (height%16) || (width<=0) || (height<=0)) {
		fprintf(stderr, "check_bs_param: ch_num=%d, sub_num=%d, width=%d, height=%d error!\n", ch_num, sub_num, width, height);
		return -1;
	}
#endif
//-- fullhd
	return 0;
}


static int set_bs_buf(int ch_num)
{
	av_t *e;
	int i;

	e = &enc[ch_num];
	if (ioctl(e->enc_fd, DVR_ENC_QUERY_OUTPUT_BUFFER_SIZE, &e->v_bs_mbuf_size)<0) {
		perror("open_main_bs query output buffer error");
        return -1;
	}

	if(ioctl(e->enc_fd, DVR_ENC_QUERY_OUTPUT_BUFFER_SNAP_OFFSET, &e->bs_buf_snap_offset)<0){
		snap_init_OK = 0;
		printf("%s:%d <DVR_ENC_QUERY_OUTPUT_BUFFER_SNAP_OFFSET failed.\n>", __FUNCTION__, __LINE__);
	}else{
		printf("bs_buf_snap_offset[%d]  = %d.\n", ch_num, e->bs_buf_snap_offset);
		snap_init_OK = 1;
	}

	e->priv_bs[0].video.offs=0;
    for(i=1; i<DVR_ENC_REPD_BT_NUM; i++) {
    	if(ioctl(e->enc_fd, dvr_enc_queue_offset_def_table[i], &e->priv_bs[i].video.offs)<0) {
    	    printf("%s:%d <open_main_bs sub%d error>\n",__FUNCTION__,__LINE__, i);
            return -1;
    	}
    }
	e->v_bs_mbuf_addr = (unsigned char*) mmap(NULL, e->v_bs_mbuf_size, PROT_READ|PROT_WRITE, MAP_SHARED, e->enc_fd, 0);
    if(e->v_bs_mbuf_addr==MAP_FAILED) {
		perror("Enc mmap failed");
        return -1;
	}
    return 0;
}

static void set_main_3di(int ch_num, dvr_enc_channel_param *ch_param)
{
	av_t *e;
    
	e = &enc[ch_num];

	if(e->input_system == CMOS) {
    	ch_param->src.vp_param.is_3DI = FALSE;
    	ch_param->src.vp_param.denoise_mode = GM3DI_FRAME;
        if(e->denoise == 1) 
            ch_param->src.vp_param.is_denoise = TRUE;
        else
            ch_param->src.vp_param.is_denoise = FALSE;

    } else {  /* PAL, NTSC system */
       	ch_param->src.dma_order = DMAORDER_PACKET;
      	ch_param->src.scale_indep = CAPSCALER_NOT_KEEP_RATIO;
      	ch_param->src.color_mode = CAPCOLOR_YUV422;
	    ch_param->src.vp_param.denoise_mode = GM3DI_FIELD;
    	if(ch_param->src.dim.width == 720) {   /* D1 */
        	ch_param->src.di_mode = LVFRAME_GM3DI_FORMAT;
        	ch_param->src.mode = LVFRAME_FRAME_MODE;
        	if(e->de_interlace == 1) {
            	ch_param->src.vp_param.is_denoise = TRUE;
            	ch_param->src.vp_param.is_3DI = TRUE;
        	} else if(e->denoise == 1) {
            	ch_param->src.vp_param.is_denoise = TRUE;
            	ch_param->src.vp_param.is_3DI = FALSE;
        	} else {
            	ch_param->src.vp_param.is_denoise = FALSE;
            	ch_param->src.vp_param.is_3DI = FALSE;
        	}
    	} else {     /* cif, qcif */
          	ch_param->src.di_mode = LVFRAME_EVEN_ODD;
          	ch_param->src.mode = LVFRAME_FIELD_MODE;
    	    if(e->denoise == 1) 
               	ch_param->src.vp_param.is_denoise = TRUE;
            else 
               	ch_param->src.vp_param.is_denoise = FALSE;
           	ch_param->src.vp_param.is_3DI = FALSE;
    	}
    }
}

static int set_main_bs(int ch_num)
{
    dvr_enc_channel_param ch_param;
	int width, height;
	av_t *e;
    EncParam_Ext3 enc_param_ext = {0};

	if(check_bs_param(ch_num, MAIN_BS_NUM)<0) return -1;

	e = &enc[ch_num];
	if(e->bs[0].enabled == DVR_ENC_EBST_ENABLE) return -1;  /* already enabled */
	if(e->enc_fd > 0) return -1;
	if((e->enc_fd=open_channel(ch_num)) < 0) return -1;
	width = e->bs[0].video.width;
	height = e->bs[0].video.height;
	memcpy(&ch_param, &main_ch_setting, sizeof(ch_param));
    switch(e->input_system) {
        case PAL: 
        	ch_param.src.input_system = MCP_VIDEO_PAL;
//        	init_isp_pw_frequency(50); 
        	break;
        case NTSC:
        case CMOS: 
        default: 
        	ch_param.src.input_system = MCP_VIDEO_NTSC; 
//        	init_isp_pw_frequency(60);
        	break;
    }
    ch_param.src.channel = ch_num;
	ch_param.src.dim.width = ch_param.main_bs.dim.width = width;
	ch_param.src.dim.height = ch_param.main_bs.dim.height = height;
	e->bs[0].video.fps = ((ch_param.src.input_system == MCP_VIDEO_PAL)&&(e->bs[0].video.fps > 25)) ? 25 : e->bs[0].video.fps;
	ch_param.main_bs.enc_type = e->bs[0].video.enc_type;
	ch_param.main_bs.enc.frame_rate = e->bs[0].video.fps;
	ch_param.main_bs.enc.ip_interval= e->bs[0].video.ip_interval;  // 2 seconds 
	ch_param.main_bs.enc.bit_rate = (e->bs[0].video.bps <= 0) ? get_bitrate(width, height, e->bs[0].video.fps) : e->bs[0].video.bps;
    if(e->bs[0].video.enabled_roi == 1) {
        ch_param.main_bs.enc.is_use_ROI = TRUE;
        ch_param.main_bs.enc.ROI_win.x = e->bs[0].video.roi_x;
        ch_param.main_bs.enc.ROI_win.y = e->bs[0].video.roi_y;
        ch_param.main_bs.enc.ROI_win.width = e->bs[0].video.roi_w;
        ch_param.main_bs.enc.ROI_win.height = e->bs[0].video.roi_h;
    } else {
        ch_param.main_bs.enc.is_use_ROI = FALSE;
    }
    ch_param.main_bs.enc.ext_size = DVR_ENC_MAGIC_ADD_VAL(sizeof(enc_param_ext));
    ch_param.main_bs.enc.pext_data = &enc_param_ext;
    enc_param_ext.feature_enable = 0;

	switch (ch_param.main_bs.enc_type) {
		case ENC_TYPE_MJPEG: 
	        enc_param_ext.feature_enable |= DVR_ENC_MJPEG_FUNCTION;
			enc_param_ext.MJ_quality = e->bs[0].video.mjpeg_quality;
			break;
		case ENC_TYPE_H264:
		case ENC_TYPE_MPEG:
			enc_param_ext.feature_enable &= ~DVR_ENC_MJPEG_FUNCTION;
			break;
		default: 
            printf(" Encoder not support! (%d)\n", ch_param.main_bs.enc_type);
			ch_param.main_bs.enc_type = ENC_TYPE_H264;
			break;
	}
	switch(e->bs[0].video.rate_ctl_type) {
		case RATE_CTL_ECBR:
	        ch_param.main_bs.enc.bit_rate = (e->bs[0].video.bps <= 0) ? get_bitrate(width, height, e->bs[0].video.fps) : e->bs[0].video.bps;
	        enc_param_ext.feature_enable |= DVR_ENC_ENHANCE_H264_RATECONTROL;
	        enc_param_ext.target_rate_max = e->bs[0].video.target_rate_max;
	        enc_param_ext.reaction_delay_max = e->bs[0].video.reaction_delay_max;
	        ch_param.main_bs.enc.max_quant = e->bs[0].video.max_quant;
	        ch_param.main_bs.enc.min_quant = e->bs[0].video.min_quant;
			ch_param.main_bs.enc.init_quant = e->bs[0].video.init_quant; 
			break;
		case RATE_CTL_EVBR:
	        enc_param_ext.feature_enable |= DVR_ENC_ENHANCE_H264_RATECONTROL;
	        ch_param.main_bs.enc.bit_rate = 0;
	        enc_param_ext.target_rate_max = e->bs[0].video.target_rate_max;
	        enc_param_ext.reaction_delay_max = 0;
	        ch_param.main_bs.enc.max_quant = e->bs[0].video.max_quant; 
	        ch_param.main_bs.enc.min_quant = e->bs[0].video.min_quant; 
	        ch_param.main_bs.enc.init_quant = e->bs[0].video.init_quant; 
	        break;
		case RATE_CTL_VBR:
	        ch_param.main_bs.enc.max_quant = e->bs[0].video.init_quant; 
	        ch_param.main_bs.enc.min_quant = e->bs[0].video.init_quant; 
	        ch_param.main_bs.enc.init_quant = e->bs[0].video.init_quant; 
	        break;
		case RATE_CTL_CBR:
		default: 
	        ch_param.main_bs.enc.bit_rate = (e->bs[0].video.bps <= 0) ? get_bitrate(width, height, e->bs[0].video.fps) : e->bs[0].video.bps;
	        ch_param.main_bs.enc.max_quant = e->bs[0].video.max_quant;
	        ch_param.main_bs.enc.min_quant = e->bs[0].video.min_quant;
			ch_param.main_bs.enc.init_quant = e->bs[0].video.init_quant; 
			break;
	}
	e->bs[0].enabled = DVR_ENC_EBST_ENABLE;
	e->bs[0].video.enabled = DVR_ENC_EBST_ENABLE;
    set_main_3di(ch_num, &ch_param);

	if(ch_num==0){
		ch_param.main_bs.en_snapshot = DVR_ENC_EBST_ENABLE;
	}
	if(ch_param.main_bs.en_snapshot==DVR_ENC_EBST_ENABLE){
		ch_param.main_bs.snap.quality = 70;
		ch_param.main_bs.snap.RestartInterval = SCALE_YUV422;
		ch_param.main_bs.snap.sample = SCALE_YUV422;
		ch_param.main_bs.snap.u82D = SCALE_LINEAR;
	}

	printf("set main bs channel %d.....\n", ch_num);
	printf("ch_param.src.dim.width = %d.\n", ch_param.src.dim.width);
	printf("ch_param.src.dim.height = %d.\n", ch_param.src.dim.height);
	printf("ch_param.main_bs.enc_type = %d.\n", ch_param.main_bs.enc_type);
	printf("ch_param.main_bs.enc.frame_rate = %d.\n", ch_param.main_bs.enc.frame_rate);
	printf("ch_param.main_bs.enc.ip_interval = %d.\n", ch_param.main_bs.enc.ip_interval);
	printf("ch_param.main_bs.enc.bit_rate = %d.\n", ch_param.main_bs.enc.bit_rate);
	if(ioctl(e->enc_fd, DVR_ENC_SET_CHANNEL_PARAM, &ch_param)<0) {
		perror("set_main_bs: set channel param error");
        return -1;
	}
	if(set_bs_buf(ch_num)<0) return -1;
    return 0;
}

static int set_sub_bs(int ch_num, int sub_num)
{
	ReproduceBitStream sub_bs;
	int width, height, fps;
	av_t *e;
    EncParam_Ext3 enc_param_ext = {0};

	if(check_bs_param(ch_num, sub_num)<0) return -1;
	
	e = &enc[ch_num];
	if(e->bs[sub_num].video.enabled == DVR_ENC_EBST_ENABLE) return 0;
    memcpy(&sub_bs, &sub_ch_setting, sizeof(ReproduceBitStream));
	sub_bs.out_bs = sub_num;
	sub_bs.enabled = DVR_ENC_EBST_ENABLE;
	sub_bs.dim.width = width = e->bs[sub_num].video.width;
	sub_bs.dim.height = height = e->bs[sub_num].video.height;
	sub_bs.enc.frame_rate = fps = e->bs[sub_num].video.fps;
	sub_bs.enc.ip_interval= e->bs[sub_num].video.ip_interval;
//	sub_bs.enc.bit_rate = (e->bs[sub_num].video.bps <= 0) ? get_bitrate(width, height, fps) : e->bs[sub_num].video.bps;
	sub_bs.enc_type = e->bs[sub_num].video.enc_type;

    if(e->bs[sub_num].video.enabled_roi == 1) {
        sub_bs.enc.is_use_ROI = TRUE;
        sub_bs.enc.ROI_win.x = e->bs[sub_num].video.roi_x;
        sub_bs.enc.ROI_win.y = e->bs[sub_num].video.roi_y;
        sub_bs.enc.ROI_win.width = e->bs[sub_num].video.roi_w;
        sub_bs.enc.ROI_win.height = e->bs[sub_num].video.roi_h;
    } else {
        sub_bs.enc.is_use_ROI = FALSE;
    }

    sub_bs.enc.ext_size = DVR_ENC_MAGIC_ADD_VAL(sizeof(enc_param_ext));
    sub_bs.enc.pext_data = &enc_param_ext;
    enc_param_ext.feature_enable = 0;
	switch (sub_bs.enc_type) {
		case ENC_TYPE_MJPEG: 
	        enc_param_ext.feature_enable |= DVR_ENC_MJPEG_FUNCTION;
			enc_param_ext.MJ_quality = e->bs[sub_num].video.mjpeg_quality;
			break;
		case ENC_TYPE_H264:
		case ENC_TYPE_MPEG:
			enc_param_ext.feature_enable &= ~DVR_ENC_MJPEG_FUNCTION;
			break;
		default: 
            printf(" Encoder not support! (%d)\n", sub_bs.enc_type);
			sub_bs.enc_type = ENC_TYPE_H264;
			break;
	}

	switch(e->bs[sub_num].video.rate_ctl_type) {
		case RATE_CTL_ECBR:
            sub_bs.enc.bit_rate = (e->bs[sub_num].video.bps <= 0) ? get_bitrate(width, height, fps) : e->bs[sub_num].video.bps;
	        enc_param_ext.feature_enable |= DVR_ENC_ENHANCE_H264_RATECONTROL;
	        enc_param_ext.target_rate_max = e->bs[sub_num].video.target_rate_max;
	        enc_param_ext.reaction_delay_max = e->bs[sub_num].video.reaction_delay_max;
	        sub_bs.enc.max_quant = e->bs[sub_num].video.max_quant;
	        sub_bs.enc.min_quant = e->bs[sub_num].video.min_quant;
			sub_bs.enc.init_quant = e->bs[sub_num].video.init_quant; 
			break;
		case RATE_CTL_EVBR:
	        enc_param_ext.feature_enable |= DVR_ENC_ENHANCE_H264_RATECONTROL;
	        sub_bs.enc.bit_rate = 0;
	        enc_param_ext.target_rate_max = e->bs[sub_num].video.target_rate_max;
	        enc_param_ext.reaction_delay_max = 0;
	        sub_bs.enc.max_quant = e->bs[sub_num].video.max_quant; 
	        sub_bs.enc.min_quant = e->bs[sub_num].video.min_quant; 
	        sub_bs.enc.init_quant = e->bs[sub_num].video.init_quant; 
	        break;
		case RATE_CTL_VBR:
	        sub_bs.enc.max_quant = e->bs[sub_num].video.init_quant; 
	        sub_bs.enc.min_quant = e->bs[sub_num].video.init_quant; 
	        sub_bs.enc.init_quant = e->bs[sub_num].video.init_quant; 
	        break;
		case RATE_CTL_CBR:
		default: 
            sub_bs.enc.bit_rate = (e->bs[sub_num].video.bps <= 0) ? get_bitrate(width, height, fps) : e->bs[sub_num].video.bps;
	        sub_bs.enc.max_quant = e->bs[sub_num].video.max_quant;
	        sub_bs.enc.min_quant = e->bs[sub_num].video.min_quant;
			sub_bs.enc.init_quant = e->bs[sub_num].video.init_quant; 
			break;
	}
	if(ioctl(e->enc_fd, DVR_ENC_SET_SUB_BS_PARAM, &sub_bs)<0) {
		perror("set_sub_bs");
        return -1;
	}
	e->bs[sub_num].enabled = DVR_ENC_EBST_ENABLE;
	e->bs[sub_num].video.enabled = DVR_ENC_EBST_ENABLE;

	return 0;
}


static int get_bs_data(int ch_num, int sub_num, dvr_enc_queue_get *data)
{
	av_t *e;
	int ret=0, diff_tv, time_c;
	priv_avbs_t	*pb;

	if((ch_num >= CHANNEL_NUM || ch_num < 0) || 
		(sub_num >= DVR_ENC_REPD_BT_NUM || sub_num < 0)) {
		fprintf(stderr, "get_bs_data: ch_num=%d, sub_num=%d error!\n", ch_num, sub_num);
		return -1;
	}
	e = &enc[ch_num];
    	ret = ioctl(e->enc_fd, dvr_enc_queue_get_def_table[sub_num], data);

	if(ret >= 0) {
 		pb = &enc[ch_num].priv_bs[sub_num];
		pb->video.buf_usage++; 
	#if PRINT_ENC_AVERAGE
		pb->video.itvl_fps++;
		pb->video.itvl_bps +=  data->bs.length;
		time_c = get_tick(NULL);
		if(pb->video.timeval == 0) {
			pb->video.timeval = time_c;
		} else {
			diff_tv = timediff(pb->video.timeval, time_c);

			if (diff_tv >= TIME_INTERVAL) {
				pb->video.avg_fps = pb->video.itvl_fps * 9000000 / diff_tv;
				pb->video.avg_bps = pb->video.itvl_bps * 80 / (diff_tv / 9000);
				print_enc_average(time_c);
				pb->video.itvl_fps = 0;
				pb->video.itvl_bps = 0;
				pb->video.timeval = time_c;
			}
		}
	#endif
	} else {
		printf("get data fail.\n");	
	}
	return ret;
}

int free_bs_data(int ch_num, int sub_num, dvr_enc_queue_get *data)
{
	av_t *e;
	int ret;
	
	if((ch_num >= CHANNEL_NUM || ch_num < 0) || 
		(sub_num >= DVR_ENC_REPD_BT_NUM || sub_num < 0)) {
		fprintf(stderr, "free_bs_data: ch_num=%d, sub_num=%d error!\n", ch_num, sub_num);
		return -1;
	}
	e = &enc[ch_num];

	ret = ioctl(e->enc_fd, DVR_ENC_QUEUE_PUT, data);
	if(ret<0) 
		fprintf(stderr, "free_bs_data sub%d_bitstream failed...\n", sub_num);
	else
		e->priv_bs[sub_num].video.buf_usage--;
	
	return ret;
}

void set_IDR_BR(int ch_num, int sub_num)
{
	priv_avbs_t *pb;

	pb = &enc[ch_num].priv_bs[sub_num];
	pb->reset_intra_frame = 1;
}

void set_nvr_send_status(int ch_num, int sub_num, int opt)
{
//	fprintf(stdout, "NVR status: %d.\n", opt);
//	fprintf(stdout, "channal is: %d:%d.\n", ch_num, sub_num);
	if(ch_num>2) return;

	NVR_send_status[ch_num][sub_num] = opt;
	if(ch_num!=2)
		set_IDR_BR(ch_num, sub_num);
}

static int open_live_streaming(int ch_num, int sub_num)
{

	int media_type;
	avbs_t *b;
	priv_avbs_t *pb;
	char livename[64];

	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	b = &enc[ch_num].bs[sub_num];
 	pb = &enc[ch_num].priv_bs[sub_num];

	media_type = convert_gmss_media_type(b->video.enc_type);
	pb->video.qno = do_queue_alloc(media_type);
	pb->audio.qno = do_queue_alloc(convert_gmss_audio_type(b->audio.enc_type));
	printf("video.qno = %d. audio.qno = %d\n", pb->video.qno, pb->audio.qno);

//	sprintf(livename, "live/ch%02d_%d", ch_num, sub_num);
	sprintf(livename, "%d", (ch_num==0)?(0):(4));
	pb->sr = stream_reg(livename, pb->video.qno, pb->video.sdpstr, pb->audio.qno, pb->audio.sdpstr, 1, 0, 0);
	if (pb->sr < 0) {
		fprintf(stderr, "open_live_streaming: ch_num=%d, sub_num=%d setup error, pb->sr = %d\n", ch_num, sub_num, pb->sr);
		return -1;
	}
	init_video_frame(pb->video.qno);
	strcpy(pb->name, livename);

	return 0;
}

static int H264GetNALType(char *pBSBuf, int nBSLen)
{
	unsigned char* pBS = (unsigned char *)pBSBuf;
	int nType;
	int i = 0;

	if ( nBSLen < 5 ) // 不完整的NAL单元
		return H264NT_NAL;

	while(i<nBSLen-4){
		if((pBS[i]==0)&&(pBS[i+1]==0)&&(pBS[i+2]==0)&&(pBS[i+3]==0x01)){
			nType = pBS[4] & 0x1F;
			if(nType <= H264NT_PPS){
				return nType;
			}
		}
		i++;
	}

	return 0;
}

static int write_rtp_frame(int ch_num, int sub_num, void *data)
{
	int ret, media_type;
	avbs_t *b;
	priv_avbs_t *pb;
	unsigned int tick;
	dvr_enc_queue_get *q= (dvr_enc_queue_get *) data;
	gm_ss_entity entity;
	frame_slot_t *fs;
	int frame_type;
	int stream_type;
	static char f_index[2] = {0,0};	

	static int i = 0;
	FILE *fp_d;	
	char *file_name;
	static int pre_time_sec = 0;
	static int pre_time_usec = 0;

	Mem_head stMem_head_to_NVR;
	Mem_head stMem_head_old;

 	pb = &enc[ch_num].priv_bs[sub_num];
	b = &enc[ch_num].bs[sub_num];

	if(/*pb->play==0 || */(b->event != NONE_BS_EVENT)) {
		ret = 1;
		goto exit_free_buf;
	}
	tick = get_tick(&q->bs.timestamp);
	if(pb->video.timed==0) {
		pb->congest = 0;
		pb->video.tick = tick;
		pb->video.timed = 1;
	}
  	if(pb->reset_intra_frame == 1) {
		set_bs_intra_frame(ch_num, sub_num);
		pb->reset_intra_frame = 0;
    	}

	entity.data =(char *) enc[ch_num].v_bs_mbuf_addr + pb->video.offs + q->bs.offset;
	entity.size = q->bs.length;
	entity.timestamp = tick;
	frame_type = H264GetNALType(entity.data, entity.size);
	if(NVR_send_status[ch_num][sub_num]){
//		fprintf(stdout, "send  channel %d data to NVR.\n", ch_num);
		stMem_head_to_NVR.channel = ch_num;
		stMem_head_to_NVR.frame_type = frame_type;
//		if(frame_type==H264NT_SLICE_IDR||frame_type==H264NT_SPS){
		if(f_index[ch_num]==201){
			f_index[ch_num] = 0;		
		}else{
			f_index[ch_num]++;		
		}
		stMem_head_to_NVR.frame_index = f_index[ch_num];
		stMem_head_to_NVR.height = b->video.height;
		stMem_head_to_NVR.width = b->video.width;
		stMem_head_to_NVR.size = entity.size;
		stMem_head_to_NVR.statue = 1;
	
//		fprintf(stdout, "frame index %d, type %d.\n", stMem_head_to_NVR.frame_index, stMem_head_to_NVR.frame_type);
//		fprintf(stdout, "=================================send NVR data start.\n");
		if((entity.size>(500*1024))&&((b->video.rate_ctl_type==0)||(b->video.rate_ctl_type==2))) {
			char log[256];
			set_bs_intra_frame(ch_num, sub_num);
//			sprintf(log, "echo timestamp = %08d.%06d, size = %d. >> /mnt/log.txt\n", \
					q->bs.timestamp.tv_sec, q->bs.timestamp.tv_usec, entity.size);
//			system(log);
//			printf("%s.", log);
		} else {
			if (ch_num == 0) {
			unsigned int diff = 0;
			if(pre_time_sec != 0) {
				if(q->bs.timestamp.tv_usec < pre_time_usec)
					diff = 1000000 + q->bs.timestamp.tv_usec - pre_time_usec;
				else
					diff = q->bs.timestamp.tv_usec - pre_time_usec;
			}	
			
			if (diff > 200000)	
				printf("timestamp gap = %d..now = %d, pre = %d.\n", diff, q->bs.timestamp.tv_usec, pre_time_usec);
			pre_time_sec = q->bs.timestamp.tv_sec;
			pre_time_usec = q->bs.timestamp.tv_usec;
			}
			//printf("send video %d data %d.\n", ch_num, entity.size);
			send_video_frame(entity.data, 
				(!(ch_num==0)),
				(stMem_head_to_NVR.frame_type==H264NT_SPS)||(stMem_head_to_NVR.frame_type==H264NT_SLICE_IDR),
				entity.size,
				stMem_head_to_NVR.height,
				stMem_head_to_NVR.width,
				q->bs.timestamp.tv_sec,
				q->bs.timestamp.tv_usec);
		}
//		fprintf(stdout, "=================================send NVR data OK.\n");

	}

//		if(ch_num==1)
//			send_frame_p2p(entity.data, entity.size, (frame_type==H264NT_SPS)?1:0);

	if (pb->play == 0) {
		ret = 1;
		goto exit_free_buf;		
	}

	media_type = convert_gmss_media_type(b->video.enc_type);
	fs = take_video_frame(pb->video.qno);
	fs->vf->search_key =(int) entity.data;
	fs->vf->ch_num = ch_num;
	fs->vf->sub_num = sub_num;
	memcpy(&fs->vf->queue, data, sizeof(dvr_enc_queue_get));
	ret = stream_media_enqueue(media_type, pb->video.qno, &entity);
	if (ret < 0) {
		if (ret == ERR_FULL) {
			pb->congest = 1;
			fprintf(stderr, "enqueue queue ch_num=%d, sub_num=%d full\n", ch_num, sub_num);
		} else if ((ret != ERR_NOTINIT) && (ret != ERR_MUTEX) && (ret != ERR_NOTRUN)) {
			fprintf(stderr, "enqueue queue ch_num=%d, sub_num=%d error %d\n", ch_num, sub_num, ret);
		}
		fprintf(stderr, "enqueue queue ch_num=%d, sub_num=%d error %d\n", ch_num, sub_num, ret);
		goto exit_free_video_buf;
	}

	stream_type = (ch_num == 0) ? (0) : (1);
	if((frame_type==H264NT_SPS)||(frame_type==H264NT_SLICE_IDR)){
//		fprintf(stdout, "I Frame .......stream_type = %d\n", stream_type);
		frame_type = 1;//I_FRAME;
	}else{
//		fprintf(stdout, "P Frame .......stream_type = %d\n", stream_type);
		frame_type = 2;//P_FRAME;
	}
	mt_write(entity.data, entity.size, frame_type, stream_type, entity.timestamp, 0,b->video.height,b->video.width);

	return 0;

exit_free_video_buf: 
	put_video_frame(pb->video.qno, fs);
exit_free_buf: 
	free_bs_data(ch_num, sub_num, q);
	return ret;
}

static int write_jpg(int chn, unsigned int jpg_id, void *data)
{
	int ret;
	av_t *e;
	gm_ss_entity entity;
	dvr_enc_queue_get *q;

	e = &enc[chn];
	ret = ioctl(e->enc_fd, DVR_ENC_QUEUE_GET_SNAP, data);
//	printf("write_jpg:.....ret = %d.\n",ret);
	if(ret>=0){
		q = (dvr_enc_queue_get *)data;
		//printf("bs_buf_snap_offset[%d] = %d, q->bs.offset = %d.\n", chn, e->bs_buf_snap_offset, q->bs.offset);

		entity.data = (char *)e->v_bs_mbuf_addr+e->bs_buf_snap_offset+q->bs.offset;
		entity.size = q->bs.length;
		write_jpg_data(entity.data, entity.size, jpg_id);
#if 0
		{
			FILE *fp;
			char filename[64];

			sprintf(filename, "/tmp/test_%d.jpg", jpg_id);
			fp = fopen(filename, "wb+");
			if(fp==NULL){
				printf("debug: open jpg file failed.\n");
			}else{
				fwrite(entity.data, 1, entity.size, fp);
				fclose(fp);
			}
		}
#endif
	}else{
		return -1;
	}

	ioctl(e->enc_fd, DVR_ENC_QUEUE_PUT, data);
	return 0;
}


static int close_live_streaming(int ch_num, int sub_num)
{
	avbs_t *b;
	priv_avbs_t *pb;
	int ret = 0;

	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	b = &enc[ch_num].bs[sub_num];
 	pb = &enc[ch_num].priv_bs[sub_num];
	free_video_frame(pb->video.qno);
	if(pb->sr >= 0) {
		ret = stream_dereg(pb->sr, 1);
		if (ret < 0) goto err_exit;
		pb->sr = -1; 
		pb->video.qno = -1;
	}

err_exit: 
	if(ret < 0) 
		fprintf(stderr, "%s: stream_dereg(%d) err %d\n", __func__, pb->sr, ret);
	return ret;
}


int open_bs(int ch_num, int sub_num)
{
	//av_t *e;
	avbs_t *b;
	priv_avbs_t *pb;
	int ret;

	if((ch_num >= CHANNEL_NUM || ch_num < 0) || 
		(sub_num >= DVR_ENC_REPD_BT_NUM || sub_num < 0)) {
		fprintf(stderr, "open_bs: ch_num=%d, sub_num=%d error!\n", ch_num, sub_num);
		return -1;
	}
	pb = &enc[ch_num].priv_bs[sub_num];
	b = &enc[ch_num].bs[sub_num];

    if(sub_num == MAIN_BS_NUM) {
        if((ret = set_main_bs(ch_num))<0) 
            goto err_exit;
    } else {
        if((ret = set_sub_bs(ch_num, sub_num))<0) 
            goto err_exit; 
    }
	if((ret = apply_bs(ch_num, sub_num, ENC_START))<0) goto err_exit;
	switch(b->opt_type) {
		case RTSP_LIVE_STREAMING:
			if(b->video.fps <= 0) 
				fprintf(stderr, "open_bs: ch_num=%d, sub_num=%d, fps=%d error!\n", ch_num, sub_num, b->video.fps);
			pb->video.tinc = 90000/b->video.fps;
			pb->video.buf_usage = 0;
			set_sdpstr(pb->video.sdpstr, b->video.enc_type);
			pb->open = open_live_streaming;
			pb->close = close_live_streaming;
			pb->read = get_bs_data;
			pb->free = NULL;
			pb->write = write_rtp_frame;
			break;
		default: 
			break;
	}
err_exit:
	return ret;
}

int close_bs(int ch_num, int sub_num)
{
	av_t *e;
	priv_avbs_t *pb;
	int ret=0, sub, is_close_channel=1;

	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	e = &enc[ch_num];
	pb = &e->priv_bs[sub_num];

	if(e->bs[sub_num].video.enabled == DVR_ENC_EBST_ENABLE) {
		pb->video.timeval = 0;
		pb->video.itvl_fps = 0;
		pb->video.itvl_bps = 0;
		pb->video.avg_fps = 0;
		pb->video.avg_bps = 0;
		if((ret = apply_bs(ch_num, sub_num, ENC_STOP))<0) goto err_exit;
		e->bs[sub_num].video.enabled = DVR_ENC_EBST_DISABLE;
		e->bs[sub_num].enabled = DVR_ENC_EBST_DISABLE;
		for(sub=0; sub < DVR_ENC_REPD_BT_NUM; sub++) {
		    if(e->bs[sub].video.enabled == DVR_ENC_EBST_ENABLE) {
		        is_close_channel = 0;
		        break;
		    }
		}
		if(is_close_channel == 1) close_channel(ch_num);
	} else {
		e->bs[sub_num].video.enabled = DVR_ENC_EBST_DISABLE;
		e->bs[sub_num].enabled = DVR_ENC_EBST_DISABLE;
	}
	return ret;

err_exit:
	fprintf(stderr, "close_bs: ch_num=%d, sub_num=%d error!\n", ch_num, sub_num);
	return ret;
}

static int bs_check_event(void)
{
	int ch_num, sub_num, ret=0;
	avbs_t *b;
	
	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++) {
		for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &enc[ch_num].bs[sub_num];
			if(b->event != NONE_BS_EVENT) {
				ret = 1;
				break;
			}
		}
	}
	return ret;
}

static int bs_check_event_type(int bs_event)
{
	int ch_num, sub_num, ret=0;
	avbs_t *b;
	
	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++) {
		for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &enc[ch_num].bs[sub_num];
			if(b->event == bs_event) { 
				ret = 1;
				break;
			}
		}
	}
	return ret;
}

void bs_new_event(void)
{
	int ch_num, sub_num;
	avbs_t *b;
	priv_avbs_t *pb;
	static int bs_event=START_BS_EVENT;
	//int sub_num_b, sub_num_e, i, exitflag=0;
	int sub_num_b, sub_num_e, i;

	if(bs_check_event() == 0) {
		bs_event=START_BS_EVENT;
		rtspd_set_event = 0;
		return;
	}
	
	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++) {
		pthread_mutex_lock(&enc[ch_num].ubs_mutex);
		for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &enc[ch_num].bs[sub_num];
			pb = &enc[ch_num].priv_bs[sub_num];
			if(b->event != bs_event) continue;
			//exitflag = 1;
			switch(b->event){
				case START_BS_EVENT: 
					open_bs(ch_num, sub_num);
					if(pb->open) pb->open(ch_num, sub_num);
					b->event = NONE_BS_EVENT;
					break;
				case UPDATE_BS_EVENT:
					if(pb->video.qno >= 0) {  // for live streaming 
						if(get_num_of_used_video_frame(pb->video.qno)==0) {
							if(pb->close) pb->close(ch_num, sub_num);  /* close RTSP Streaming */
					        if(pb->open) pb->open(ch_num, sub_num); /* re-open RTSP streaming */
							b->event = NONE_BS_EVENT;
						}
					}
					break;
				case STOP_BS_EVENT:
					if(sub_num == MAIN_BS_NUM) {
						sub_num_b = DVR_ENC_REPD_BT_NUM-1;
						sub_num_e = 0;
					} else {
						sub_num_b = sub_num_e = sub_num;
					}
					for(i=sub_num_b; i>=sub_num_e; i--) {
					 	pb = &enc[ch_num].priv_bs[i];
						b = &enc[ch_num].bs[i];
						pb->open = NULL;
						pb->read = NULL;
						pb->free = NULL;
						pb->write = NULL;
						if(pb->video.qno >= 0) {  // for live streaming 
							if(get_num_of_used_video_frame(pb->video.qno)==0) {
								if(pb->close) pb->close(ch_num, i);
								pb->close = NULL;
								close_bs(ch_num, i);
								b->event = NONE_BS_EVENT;
							} else {
							    break;
							}
						} else if(pb->close) {  /* for recording */
							pb->close(ch_num, i);
							pb->close = NULL;
							close_bs(ch_num, i);
							b->event = NONE_BS_EVENT;
						} else {
							b->event = NONE_BS_EVENT;
						}
					}
					break;
			}
		}
		//if(exitflag == 1) break;
		pthread_mutex_unlock(&enc[ch_num].ubs_mutex);
	}
	switch(bs_event) {
		case START_BS_EVENT: 
			if(bs_check_event_type(START_BS_EVENT) == 0) 
				bs_event=STOP_BS_EVENT; 
			break;
		case STOP_BS_EVENT: 
			if(bs_check_event_type(STOP_BS_EVENT) == 0) 
				bs_event=UPDATE_BS_EVENT; 
			break;
		case UPDATE_BS_EVENT: 
			if(bs_check_event_type(UPDATE_BS_EVENT) == 0) 
				bs_event=START_BS_EVENT; 
			break;
	}
}

int env_set_bs_new_event(int ch_num, int sub_num, int event)
{
	avbs_t *b;
	int ret = 0;

	CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
	b = &enc[ch_num].bs[sub_num];
	switch(event){
		case START_BS_EVENT: 
			if(b->opt_type == OPT_NONE) goto err_exit;
			if(b->enabled == DVR_ENC_EBST_ENABLE) {
				fprintf(stderr, "Already enabled: ch_num=%d, sub_num=%d\n", ch_num, sub_num);
				ret = -1;
				goto err_exit;
			}
			break;
		case UPDATE_BS_EVENT:
			break;
		case STOP_BS_EVENT:
		    if(sub_num == 0) { /* for main, and disable all sub */
		        if(is_bs_all_disable()) {
    				fprintf(stderr, "Already disabled. ch_num=%d\n", ch_num);
    				goto err_exit;
		        } else {
		            break;
		        }
		    } else {
    			if(b->enabled != DVR_ENC_EBST_ENABLE) {
    				fprintf(stderr, "Already disabled: ch_num=%d, sub_num=%d\n", ch_num, sub_num);
    				ret = -1;
    				goto err_exit;
    			}
		    }
			break;
		default: 
			fprintf(stderr, "env_set_bs_new_event: ch_num=%d, sub_num=%d, event=%d, error\n", ch_num, sub_num, event);
			ret = -1;
			goto err_exit;
	}
	b->event = event;
	rtspd_set_event = 1;

err_exit:
	return ret;
}

int set_poll_event(void)
{
	int ch_num, sub_num, ret = -1;
	av_t *e;
	avbs_t *b;

	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++) {
		poll_fds[ch_num].revents = 0;
		poll_fds[ch_num].events = 0;
		poll_fds[ch_num].fd = -1;
		e = &enc[ch_num];
		if(e->enabled != DVR_ENC_EBST_ENABLE) continue;
		poll_fds[ch_num].fd = e->enc_fd;
		if(e->bs[ch_num].video.enabled_snapshot&&snap_init_OK&&snap_start){
			poll_fds[ch_num].events |= POLLIN_SNAP_BS;
			apply_bs_ensnap(ch_num, 1);
		}
		for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			b = &e->bs[sub_num];
			if(b->video.enabled == DVR_ENC_EBST_ENABLE) {
				poll_fds[ch_num].events |= (POLLIN_MAIN_BS << sub_num);
				ret = 0;
			}
		}
	}
	return ret;
}

void restart()
{
	system("/sbin/reboot");
}

#define POLL_WAIT_TIME 19500 /* microseconds */

void do_poll_event(void)
{
	int ch_num, sub_num, ret;
	priv_avbs_t *pb;
	dvr_enc_queue_get data;
	static struct timeval prev;
	struct timeval cur, tout;
	static int timeval_init = 0;	
	int diff;
	static int num_time = 0;

	gettimeofday(&cur, NULL);
	if(timeval_init==0) {
		timeval_init=1;
		tout.tv_sec=0;
		tout.tv_usec=POLL_WAIT_TIME;
	} else {
		diff= (cur.tv_usec < prev.tv_usec) ? (cur.tv_usec+1000000-prev.tv_usec) : (cur.tv_usec-prev.tv_usec);
		tout.tv_usec = (diff > POLL_WAIT_TIME) ? (tout.tv_usec=0) : (POLL_WAIT_TIME-diff);
//		printf("cur.tv_sec = %d, cut.tv_usec = %d, diff = %d.\n", cur.tv_sec, cur.tv_usec, diff);
	}
	usleep(tout.tv_usec);
	gettimeofday(&prev, NULL);
	ret =poll(poll_fds, CHANNEL_NUM, 500);
	if(ret < 0) {
	    printf("poll:");
	    printf("%s:%d <ret=%d>\n",__FUNCTION__,__LINE__, ret);
			num_time++;
			if(num_time>3000){
				restart();
			}
	    return;
	}
	num_time = 0;
	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++) {
		if(poll_fds[ch_num].revents == 0) continue;
		for(sub_num=0; sub_num < 1/*DVR_ENC_REPD_BT_NUM*/; sub_num++){
			if(poll_fds[ch_num].revents & (POLLIN_MAIN_BS << sub_num)) {
				pb = &enc[ch_num].priv_bs[sub_num];
				if(pb->read) ret=pb->read(ch_num, sub_num, &data);
				if(ret<0) 
					continue;
				if(pb->write) pb->write(ch_num, sub_num, &data);
				if(pb->free) pb->free(ch_num, sub_num, &data);
			}
//			printf("snap_init_OK = %d, snap_start = %d, poll_fds[ch_num].events&POLLIN_SNAP_BS = %d.\n", 
//									snap_init_OK, snap_start, (poll_fds[ch_num].events&POLLIN_SNAP_BS));
			if(snap_init_OK&&snap_start&&(poll_fds[ch_num].events&POLLIN_SNAP_BS)){
				ret = write_jpg(ch_num, snap_start, &data);
				if(ret==0){
//					snap_start--;
					snap_start = 0;
				}
			}
		}
	}
}

static int init_resources(void)
{
	struct timeval	tval;
	
	srand((unsigned int) time(NULL));
	gettimeofday(&tval, NULL);
	sys_sec = tval.tv_sec;
//	memset(frame_info, 0, sizeof(frame_info));
    return 0;
}

static void	env_release_resources(void)
{
	int ret, ch_num;
	av_t *e;
/*
	if ((ret = stream_server_stop())) 
		fprintf(stderr, "stream_server_stop() error %d\n", ret);
*/
	for(ch_num=0; ch_num < CHANNEL_NUM; ch_num++) {
		e = &enc[ch_num];
        pthread_mutex_destroy(&e->ubs_mutex);
    }
}

static int frm_cb(int type, int qno, gm_ss_entity *entity)
{
	frame_slot_t *fs;

	if ((GM_SS_VIDEO_MIN <= type) && (type <= GM_SS_VIDEO_MAX)) {
		fs = search_video_frame(qno, (int)entity->data);
		CHECK_CHANNUM_AND_SUBNUM(fs->vf->ch_num, fs->vf->sub_num);
		free_bs_data(fs->vf->ch_num, fs->vf->sub_num, &fs->vf->queue);
		put_video_frame(qno, fs);
	}
	return 0;
}

static int sent_ptz_cmd(char *cmdline)
{
	int sock_fd = -1;
	struct sockaddr_un addr;
	int ret = -1;
	int fd = -1;
	char cmd[1024];

	sock_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if(sock_fd<0){
		fprintf(stderr, "socket create error.\n");
		return -1;
	}

	memset(cmd, 0, 1024);
	memcpy(cmd, cmdline, strlen(cmdline));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, COM_UDP_DMOMAIN, sizeof(addr.sun_path)-1);
	ret = sendto(sock_fd, cmdline, 1024, 0, (struct socketaddr *)&addr, sizeof(addr));
	if(ret<0){
		fprintf(stderr, "PTZ cmd sendro error.\n");
		close(sock_fd);
		return -1;
	}

//	fprintf(stdout, "send PTZ cmd, size: %d.\n", ret);
	close(sock_fd);
	return 0;
}

int mctp_cmd_cb(enum MCTP_CMD_TYPE cmd_type, char *cmdline)
{
	if(cmdline==NULL){
		fprintf(stderr, "cmd line if NULL.\n");
		return -1;
	}

	switch(cmd_type){
	case ALIVE:
		fprintf(stdout, "ALIVE.\n");
		break;
	case PTZ:
		fprintf(stdout, "PTZ:%s.\n", cmdline);
		sent_ptz_cmd(cmdline);
		break;
	case VCTRL:
		fprintf(stdout, "VCTRL.\n");
		break;
	default:
		fprintf(stderr, "mctp cmd not support.\n");
		break;
	}

	return 0;
}

priv_avbs_t *find_file_sr(char *name, int srno)
{
	int	ch_num, sub_num, hit=0;
	priv_avbs_t	*pb;

	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++) {
		for(sub_num=0; sub_num<DVR_ENC_REPD_BT_NUM; sub_num++) {
			pb = &enc[ch_num].priv_bs[sub_num];
			if ((pb->sr == srno) && (pb->name) && (strcmp(pb->name, name) == 0)) {
				hit = 1;	
				pb->reset_intra_frame = 1;  
				break;
			}
		}
		if(hit) break;
	}
	return (hit ? pb : NULL); 
}

static int cmd_cb(char *name, int sno, int cmd, void *p)
{
	int	ret = -1;
	priv_avbs_t		*pb;

	switch (cmd) {
	case GM_STREAM_CMD_OPEN:
		printf("%s:%d <GM_STREAM_CMD_OPEN>\n",__FUNCTION__,__LINE__);
		ERR_GOTO(-10, cmd_cb_err);
		break;
	case GM_STREAM_CMD_PLAY:
		if ((pb = find_file_sr(name, sno)) == NULL) goto cmd_cb_err;
		if(pb->video.qno >= 0) pb->play = 1;
		ret = 0;
		break;
	case GM_STREAM_CMD_PAUSE:
		printf("%s:%d <GM_STREAM_CMD_PAUSE>\n",__FUNCTION__,__LINE__);
		ret = 0;
		break;
	case GM_STREAM_CMD_TEARDOWN:
		if ((pb = find_file_sr(name, sno)) == NULL) goto cmd_cb_err;
		pb->play = 0;
		pb->video.timed = 0;
		ret = 0;
		break;
	default:
		fprintf(stderr, "%s: not support cmd %d\n", __func__, cmd);
		ret = -1;
	}

cmd_cb_err:
	if (ret < 0) {
		fprintf(stderr, "%s: cmd %d error %d\n", __func__, cmd, ret);
	}
	return ret;
}


void *enq_thread(void *ptr)
{
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	printf("enq_thread.....\n");
	while (1) {
		if(rtspd_sysinit == 0) 
			break;

		if(rtspd_set_event){
			bs_new_event();
			audio_init();
		}
		if(set_poll_event() < 0) {
			sleep(1);
			continue;
		}
		do_audio_proc();  //for audio
//		send_audio_proc();
		do_poll_event();
	}

	audio_stop();
	env_release_resources();
	pthread_exit(NULL);
	return NULL;
}

int env_resources(void)
{
	int	ret;
	
	if ((ret = init_resources()) < 0) {
		fprintf(stderr, "init_resources, ret %d\n", ret);
	}
	return ret;
}

int env_server_init(void)
{
	int	ret = 0;
	
	if ((ret = stream_server_init(ipptr, (int) sys_port, 256, SR_MAX, VQ_MAX, VQ_LEN, AQ_MAX, AQ_LEN, frm_cb, cmd_cb)) < 0) {
		fprintf(stderr, "stream_server_init, ret %d\n", ret);
	}
	return ret;
}


int env_file_init(void)
{
	int	ret = 0;

	if ((ret = stream_server_file_init(file_path)) < 0) {
		fprintf(stderr, "stream_server_file_init, ret %d\n", ret);
	}

	return ret;
}

int env_server_start(void)
{
	int	ret = 0;

	if ((ret = stream_server_start()) < 0) {
		fprintf(stderr, "stream_server_start, ret %d\n", ret);
	}
	return ret;
}

int env_init(void)
{
	int	ret;

	env_enc_init();	//初始化所有参数。
	env_cfg_init();
	if ((ret = env_resources()) < 0) return ret;
	if ((ret = env_server_init()) < 0) return ret;
	if ((ret = env_file_init()) < 0) return ret;
	if ((ret = env_server_start()) < 0) return ret;
	return 0;
}




void env_bs_start(void)
{
	int ch_num, stream;

	for(ch_num=0; ch_num<t_ch_num; ch_num++){
		pthread_mutex_lock(&enc[ch_num].ubs_mutex);
		for(stream=0; stream<DVR_ENC_REPD_BT_NUM; stream++) {
			env_set_bs_new_event(ch_num, stream, START_BS_EVENT);
		}
		pthread_mutex_unlock(&enc[ch_num].ubs_mutex);
	}
}

void env_bs_stop()
{
	int ch_num;
	int i;

	for(ch_num=0; ch_num < CHANNEL_NUM; ch_num++) {
		pthread_mutex_lock(&enc[ch_num].ubs_mutex);
		env_set_bs_new_event(ch_num, 0, STOP_BS_EVENT);  /* disable main + sub */
		pthread_mutex_unlock(&enc[ch_num].ubs_mutex);

	}
	for(i=0; i<10; i++) {
		sleep(1);
		if(is_bs_all_disable()) break;
	}
}

/*
int recv_data(int fd, char *buf, int len)
{
	int lread = 0;
	int reallen = 0;
	char *pbuf = buf;
	
	if(fd<0)
		return -1;
		
	while(reallen<len){
		lread = read(fd, pbuf, len);
		if(lread<0){
			return -1;
		}
		reallen += lread;
		pbuf += lread;
	}
	
	return reallen;
}
*/

int init_drv_common()
{
	if (rtspd_dvr_fd == 0) {
		rtspd_dvr_fd = open("/dev/dvr_common", O_RDWR);   //open_dvr_common
		if(rtspd_dvr_fd < 0){
			return -1;
		}
	}
	
	return 0;
}

int rtspd_start()		//port = 554
{
	int				ret;
	pthread_attr_t	attr;
	int port = g_stPort_file.rtspport;	

	if(rtspd_sysinit == 1) {
		return -1;
	}

	if ((0 < port) && (port < 0x10000)) sys_port = port;

	mt_init(NULL);

	if ((ret = env_init()) < 0) return ret;
	rtspd_sysinit = 1;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&enq_thread_id, &attr, &enq_thread, NULL);
	pthread_attr_destroy(&attr);

	usleep(200);
	
	init_p2p();

	/*start rtsp stream*/
	env_bs_start();
	
	return 0;
}





