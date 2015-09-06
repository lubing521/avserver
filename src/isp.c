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


int isp_fd=0;


int start_isp()
{
	return(ioctl(isp_fd, ISP_IOC_START, NULL));
}

void close_isp(void)
{
	printf("close isp.\n");
	close(isp_fd);
	isp_fd = -1;
}


int adjust_ae_ev_mode(int val)
{
	int ret;
	int val_old;
	alg_arg_t arg;
	const char *ev_mode_name[3] = {
		"AE_EV_MODE_VIDEO",
		"AE_EV_MODE_CAMERA",
		"AE_EV_MODE_USER"
	};
#if 1
	// get EV mode
	arg.cmd = AE_GET_EV_MODE;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current EV mode = %s\n", ev_mode_name[val_old]);
#endif

	if ((val !=0) && (val != 1) && (val != 2)) {
		printf("Invalid ev mode!\n");
	}

	arg.cmd = AE_SET_EV_MODE;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;

	return ret;
}

int adjust_ae_pwr_freq(int val)
{
	int ret;
	int val_old;
	alg_arg_t arg;
#if 1
	// get EV mode
	arg.cmd = AE_GET_PWR_FREQ;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current pwr_freq = %s\n", (val_old==0) ? "60Hz" : "50Hz");
#endif
	if ((val !=0) && (val != 1)) {
		printf("Invalid power frequency!\n");
	}

	arg.cmd = AE_SET_PWR_FREQ;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;

	return ret;
}

int adjust_ae_min_gain(int val)
{
	int ret;
	u32 val_old;
	alg_arg_t arg;
#if 1
	// get min gain
	arg.cmd = AE_GET_MIN_GAIN;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current min gain = %d\n", val_old);
#endif

	arg.cmd = AE_SET_MIN_GAIN;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0) {
		printf("AE_SET_MIN_GAIN fail\n"); 
		return ret;
	}

	return ret;
}

int adjust_ae_max_gain(int val)
{
	int ret;
	unsigned int val_old;
	alg_arg_t arg;
#if 1
	// get max gain
	arg.cmd = AE_GET_MAX_GAIN;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current max gain = %d\n", val_old);
#endif

	arg.cmd = AE_SET_MAX_GAIN;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0) {
		printf("AE_SET_MAX_GAIN fail\n"); 
		return ret;
	}

	return ret;
}

int adjust_ae_min_exp(int val)
{
	int ret;
	unsigned int val_old;
	alg_arg_t arg;
#if 1
	// get min exposure
	arg.cmd = AE_GET_MIN_EXPOSURE;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current min exposure = %d\n", val_old);
#endif

	arg.cmd = AE_SET_MIN_EXPOSURE;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0) {
		printf("AE_SET_MIN_EXPOSURE fail\n"); 
		return ret;
	}

	return ret;
}

int adjust_ae_max_exp(int val)
{
	int ret;
	unsigned int val_old;
	alg_arg_t arg;
#if 1
	// get max exposure
	arg.cmd = AE_GET_MAX_EXPOSURE;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current max exposure = %d\n", val_old);
#endif

	arg.cmd = AE_SET_MAX_EXPOSURE;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0) {
		printf("AE_SET_MAX_EXPOSURE fail\n"); 
		return ret;
	}

	return ret;
}

int adjust_ae_speed(int val)
{
	int ret;
	unsigned int val_old;
	alg_arg_t arg;
#if 1
	// get max exposure
	arg.cmd = AE_GET_SPEED;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val;
	arg.size = sizeof(val);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Converge speed = %d\n", val);
#endif

	if ((val < 1) && (val >= 512)) {
		printf("Invalid AE speed!\n");
		return -1;
	}

	arg.cmd = AE_SET_SPEED;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0) {
		printf("AE_SET_SPEED fail\n"); 
		return ret;
	}

	return ret;
}

int get_ae_win_weight(void)
{    
	int ret;
	alg_arg_t arg;
	int win_weight[AE_WIN_XNUM*AE_WIN_YNUM];
	int i, j;

	arg.cmd = AE_GET_WIN_WEIGHT;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &win_weight;
	arg.size = sizeof(win_weight);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;

	printf("AE window weight\n");
	for (j=0; j<AE_WIN_YNUM; j++) {
		for (i=0; i<AE_WIN_XNUM; i++)
			printf("%d, ", win_weight[j*AE_WIN_XNUM+i]);
		printf("\n");
	}

	return ret;
}

int set_ae_win_weight(void)
{
	int ret;
	alg_arg_t arg;
	int win_weight[AE_WIN_XNUM*AE_WIN_YNUM] = {
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 1, 1, 1, 1, 3, 3,
		3, 3, 1, 1, 1, 1, 3, 3,
		3, 3, 1, 1, 1, 1, 3, 3,
		3, 3, 1, 1, 1, 1, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
	};

	arg.cmd = AE_SET_WIN_WEIGHT;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &win_weight;
	arg.size = sizeof(win_weight);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0) {
		printf("AE_SET_WIN_WEIGHT fail!\n");
		return ret;
	}

	return ret;
}

int adjust_awb_scene_mode(int val)
{
	int ret;
	int val_old;
	int i;
	alg_arg_t arg;
#if 1
	const char *scene_mode_name[6] = {
		"Auto",
		"Incandescent Light",
		"Cool Light",
		"Sun Light",
		"Cloudy",
		"Sun Shade",
	};

	// get scnen mode
	arg.cmd = AWB_GET_SCENE_MODE;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current scene mode = %s\n", scene_mode_name[val_old]);
#endif

	if ((val < 0) || (val >= 6)) {
		printf("Invalid scene mode!\n");
		return -1;
	}

	arg.cmd = AWB_SET_SCENE_MODE;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0)
		return ret;

	return ret;
}

unsigned int get_isp_rgb_gain(int rgb_type)
{
	unsigned int gain;
	int ret = -1;

	switch (rgb_type) {
	case 0: // R
		ret = ioctl(isp_fd, ISP_IOC_GET_R_GAIN, &gain);
		break;
	case 1: // G
		ret = ioctl(isp_fd, ISP_IOC_GET_G_GAIN, &gain);
		break;
	case 2: // B
		ret = ioctl(isp_fd, ISP_IOC_GET_B_GAIN, &gain);
		break;
	}
	if (ret < 0)
		gain = 0xFFFFFFFF;
	return gain;
}

void set_isp_rgb_gain(int rgb_type, unsigned int gain)
{
	int ret = -1;

	switch (rgb_type) {
	case 0: // R
		ret = ioctl(isp_fd, ISP_IOC_SET_R_GAIN, &gain);
		if (ret < 0)
			printf("ISP_IOC_SET_B_GAIN fail\n");
		break;
	case 1: // G
		ret = ioctl(isp_fd, ISP_IOC_SET_G_GAIN, &gain);
		if (ret < 0)
			printf("ISP_IOC_SET_B_GAIN fail\n");
		break;
	case 2: // B
		ret = ioctl(isp_fd, ISP_IOC_SET_B_GAIN, &gain);
		if (ret < 0)
			printf("ISP_IOC_SET_B_GAIN fail\n");
		break;
	}
}

int adjust_awb_speed(int val)
{
	int ret;
	unsigned int val_old;
	alg_arg_t arg;
#if 1
	arg.cmd = AWB_GET_SPEED;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val;
	arg.size = sizeof(val);

	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Converge speed = %d\n", val);
#endif

	if ((val < 1) && (val >= 1024)) {
		printf("Invalid AWB speed!\n");
		return -1;
	}

	arg.cmd = AWB_SET_SPEED;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_SPEED fail\n"); 
		return ret;
	}

	return ret;
}

int adjust_awb_interval(int val)
{
	int ret;
	unsigned int val_old;
	alg_arg_t arg;
#if 1
	arg.cmd = AWB_GET_INTERVAL;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Interval = %d frames\n", val_old);
#endif

	arg.cmd = AWB_SET_INTERVAL;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_INTERVAL fail\n"); 
		return ret;
	}

	return ret;
}

int adjust_awb_freeze_seg(int val)
{
	int ret;
	unsigned int val_old;
	alg_arg_t arg;
#if 1
	arg.cmd = AWB_GET_FREEZE_SEG;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("freeze segment number = %d\n", val_old);
#endif

	if ((val < 1) && (val > 2304)) {
		printf("Invalid AWB freeze segment number!\n");
		return -1;
	}

	arg.cmd = AWB_SET_FREEZE_SEG;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_FREEZE_SEG fail\n"); 
		return ret;
	}

	return ret;
}

wp_thres_t* get_awb_wp_thres(void)
{
	int ret;
	alg_arg_t arg;
	wp_thres_t *wp_th;

	wp_th = (wp_thres_t *)malloc(sizeof(wp_thres_t));
	if(wp_th==NULL){
		return NULL;
	}

	arg.cmd = AWB_GET_WP_THRESHOLD;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = wp_th;
	arg.size = sizeof(wp_thres_t);

	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0){
		free(wp_th);
		return NULL;
	}

	printf("AWB threshold\n");
	printf("Min (R-B)/G = %d\n", wp_th->MIN_RsB2G);
	printf("Max (R-B)/G = %d\n", wp_th->MAX_RsB2G);
	printf("Min R/G     = %d\n", wp_th->MIN_R2G);
	printf("Max R/G     = %d\n", wp_th->MAX_R2G);
	printf("Min B/G     = %d\n", wp_th->MIN_B2G);
	printf("Max B/G     = %d\n", wp_th->MAX_B2G);
	printf("Min Y       = %d\n", wp_th->MIN_YTH);
	printf("Max Y       = %d\n", wp_th->MAX_YTH);
	printf("Min R/G*B/G = %d\n", wp_th->MIN_RmB2GG);
	printf("Max R/G*B/G = %d\n", wp_th->MAX_RmB2GG);

	return wp_th;
}


//val = min( Min (R-B)/G )>>
int set_awb_MIN_RsB2G(int val)
{
	int ret;
	int error;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MIN_RsB2G;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MIN_RsB2G fail\n"); 
		return ret;
	}

	return ret;
}
//val = max( Min (R-B)/G )>>
int set_awb_MAX_RsB2G(int val)
{
	int ret;
	int error;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MAX_RsB2G;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MAX_RsB2G fail\n"); 
		return ret;
	}

	return ret;
}

//val = min R/G
int set_awb_MIN_R2G(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MIN_R2G;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MIN_R2G fail\n"); 
		return ret;
	}

	return ret;
}
//val = MAX( R/G)
int set_awb_MAX_R2G(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MAX_R2G;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MAX_R2G fail\n"); 
		return ret;
	}

	return ret;
}
//val = Min(B/G)
int set_awb_MIN_B2G(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MIN_B2G;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MIN_B2G fail\n"); 
		return ret;
	}

	return ret;
}
//val = Max(B/G)
int set_awb_MAX_B2G(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MAX_B2G;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MAX_B2G fail\n"); 
		return ret;
	}

	return ret;
}

// val = Min Y
int set_awb_MIN_YTH(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MIN_YTH;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MIN_YTH fail\n"); 
		return ret;
	}

	return ret;
}
//val = Max Y
static int set_awb_MAX_YTH(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MAX_YTH;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MAX_YTH fail\n"); 
		return ret;
	}

	return ret;
}

//val = Min R/G*B/G
int set_awb_MIN_RmB2GG(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MIN_RmB2GG;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MIN_RmB2GG fail\n"); 
		return ret;
	}

	return ret;
}
//val = Max R/G*B/G 
int set_awb_MAX_RmB2GG(int val)
{
	int ret;
	char buf[256];
	alg_arg_t arg;

	arg.cmd = AWB_SET_MAX_RmB2GG;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AWB_IOCTL, &arg);
	if (ret < 0) {
		printf("AWB_SET_MAX_RmB2GG fail\n"); 
		return ret;
	}

	return ret;
}


int init_isp_pw_frequency(int iFW)
{
	int ret = -1;
	char *cmd;
	int fw;

	if((iFW!=50) && (iFW!=60)) {
		printf("Set power frequency %d fail!\n", iFW);
		goto err_exit;
	}
	cmd = (char *)malloc(128*sizeof(char));
	if(cmd==NULL){
		return -1;	
	}

	fw = (iFW==50)?1:0;
	printf("fw = %d\n", fw);
	sprintf(cmd, "echo %d > /proc/isp0/ae/pwr_freq", fw);
	system(cmd);

	free(cmd);
	return 0;

err_exit: 
	return ret; 
}


int adjust_brightness(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_BRIGHTNESS, &val_old);
	if (ret < 0)
		return ret;
	printf("Current brightness = %d\n", val_old);
#endif

	if(val<0||val>255){
		return -1;
	}
	return ioctl(isp_fd, ISP_IOC_SET_BRIGHTNESS, &val);
}

int adjust_contrast(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_CONTRAST, &val_old);
	if (ret < 0)
		return ret;
	printf("Current contrast = %d\n", val_old);
#endif

	if(val<0||val>255){
		return -1;
	}
	return ioctl(isp_fd, ISP_IOC_SET_CONTRAST, &val);
}

int adjust_hue(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_HUE, &val_old);
	if (ret < 0)
		return ret;
	printf("Current hue = %d\n", val_old);
#endif

	if(val<0||val>255){
		return -1;
	}
	return ioctl(isp_fd, ISP_IOC_SET_HUE, &val);
}

int adjust_saturation(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_SATURATION, &val_old);
	if (ret < 0)
		return ret;
	//	printf("Current saturation = %d\n", val_old);
#endif

	if(val<0||val>255){
		return -1;
	}	
	return ioctl(isp_fd, ISP_IOC_SET_SATURATION, &val);
}

int adjust_shapness(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_SHARPNESS, &val_old);
	if (ret < 0)
		return ret;
	printf("Current sharpness = %d\n", val_old);
#endif

	if(val<0||val>255){
		return -1;
	}
	return ioctl(isp_fd, ISP_IOC_SET_SHARPNESS, &val);
}

int adjust_denoise(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_DENOISE, &val_old);
	if (ret < 0)
		return ret;
	printf("Current denoise = %d\n", val_old);
#endif

	return ioctl(isp_fd, ISP_IOC_SET_DENOISE, &val);
}

int adjust_gamma(int val)
{
#if 1
	int ret;
	int val_old;
	int i;

	const char *gamma_name[6] = {
		"GAMMA 1.6",
		"GAMMA 1.8",
		"GAMMA 2.0",
		"GAMMA 2.2",
		"User Defined",
		"GAMMA BT709"
	};

	ret = ioctl(isp_fd, ISP_IOC_GET_GAMMA, &val_old);
	if (ret < 0)
		return ret;
	printf("Current gamma = %s\n", gamma_name[val_old]);
#endif

	if ((val < 0) || (val >= 6)) {
		printf("Invalid gamma!\n");
		return -1;
	}
	return ioctl(isp_fd, ISP_IOC_SET_GAMMA, &val);
}

int set_auto_cs_en(int val)
{
	int ret;
#if 1
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_AUTO_CS_EN, &val_old);
	if (ret < 0)
		return ret;
	printf("Auto CS: %s\n", (val_old) ? "ON" : "OFF" );
#endif

	if ((val !=0) && (val != 1)) {
		printf("Invalid input value!\n");
	}

	ret = ioctl(isp_fd, ISP_IOC_SET_AUTO_CS_EN, &val);
	if (ret < 0) {
		printf("Can't enable Auto Chroma Suppression because resizing!\n");
		ret = 0;
	}

	return ret;
}

int set_auto_cs_th(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_AUTO_CS_TH, &val_old);
	if (ret < 0)
		return ret;
	printf("Current threshold = %d\n", val_old);
#endif

	if (val > 255) {
		printf("Invalid input value!\n");
	}

	return ioctl(isp_fd, ISP_IOC_SET_AUTO_CS_TH, &val);
}

int set_auto_cs_ratio(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_AUTO_CS_GAIN, &val_old);
	if (ret < 0)
		return ret;
	printf("Current suppression ratio = %d\n", val_old);
#endif

	if (val > 1023) {
		printf("Invalid input value!\n");
	}

	return ioctl(isp_fd, ISP_IOC_SET_AUTO_CS_GAIN, &val);
}

int adjust_ce_intensity(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_CE_INTENSITY, &val_old);
	if (ret < 0)
		return ret;
	printf("Current ce intensity = %d\n", val_old);
#endif

	if(val<0){
		return -1;
	}
	return ioctl(isp_fd, ISP_IOC_SET_CE_INTENSITY, &val);
}

int set_mirror(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_MIRROR, &val_old);
	if (ret < 0)
		return ret;
	if (val_old)
		printf("Mirror: ENABLE\n");
	else
		printf("Mirror: DISABLE\n");
#endif

	if ((val !=0) && (val != 1)) {
		printf("Invalid input value!\n");
	}

	return ioctl(isp_fd, ISP_IOC_SET_SENSOR_MIRROR, &val);
}

int set_flip(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_FLIP, &val_old);
	if (ret < 0)
		return ret;
	if (val_old)
		printf("Flip: ENABLE\n");
	else
		printf("Flip: DISABLE\n");
#endif

	if ((val !=0) && (val != 1)) {
		printf("Invalid input value!\n");
	}

	return ioctl(isp_fd, ISP_IOC_SET_SENSOR_FLIP, &val);
}

int set_sensor_ae(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_AE_EN, &val_old);
	if (ret < 0)
		return ret;
	if (val_old)
		printf("Sensor AE: ENABLE\n");
	else
		printf("Sensor AE: DISABLE\n");
#endif

	if ((val !=0) && (val != 1)) {
		printf("Invalid input value!\n");
	}

	return ioctl(isp_fd, ISP_IOC_SET_SENSOR_AE_EN, &val);
}

int adjust_ae_targetY(int val)
{
	int ret;
	alg_arg_t arg;
#if 1
	int val_old;
	// get ae target Y
	arg.cmd = AE_GET_TARGETY;
	arg.direction = DATA_DRIVER_TO_APP;
	arg.data = &val_old;
	arg.size = sizeof(val_old);

	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;
	printf("Current AE target Y = %d\n", val_old);
#endif

	if (val > 255) {
		printf("Invalid target Y\n");
	}

	// set ae target Y
	arg.cmd = AE_SET_TARGETY;
	arg.direction = DATA_APP_TO_DRIVER;
	arg.data = &val;
	arg.size = sizeof(val);
	ret = ioctl(isp_fd, ISP_IOC_AE_IOCTL, &arg);
	if (ret < 0)
		return ret;

	return ret;
}

int set_sensor_awb(int val)
{
#if 1
	int ret;
	int val_old;

	ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_AWB_EN, &val_old);
	if (ret < 0)
		return ret;
	if (val_old)
		printf("Sensor AWB: ENABLE\n");
	else
		printf("Sensor AWB: DISABLE\n");
#endif

	if ((val !=0) && (val != 1)) {
		printf("Invalid input value!\n");
	}

	return ioctl(isp_fd, ISP_IOC_SET_SENSOR_AWB_EN, &val);
}

int set_exp_gain(int exp, int gain)
{
	int ret;
	unsigned int val;

	if(exp>0){
#if 1
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_EXPOSURE, &val);
		if (ret < 0)
			printf("ISP_IOC_GET_SENSOR_EXPOSURE fail\n");
		else
			printf("Current exposure = %d us\n", val);
#endif
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_EXPOSURE, &exp);
		if (ret < 0)
			printf("ISP_IOC_SET_SENSOR_EXPOSURE fail\n");
	}
	if(gain!=0){
#if 1
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_GAIN, &val);
		if (ret < 0)
			printf("ISP_IOC_GET_SENSOR_GAIN fail\n");
		else
			printf("Current sensor gain = %d\n", val);
#endif
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_GAIN, &gain);
		if (ret < 0)
			printf("ISP_IOC_SET_SENSOR_GAIN fail\n");
	}

	return 0;
}


int init_isp(void)
{
	char isp_file[] = "/dev/isp0";//no file /dev/isp, but /dev/isp0
	FILE *fp;
	int iFW;
	int ret;
	Img_cfg_t stImg_file;

	if((access(isp_file, F_OK)) >= 0) {	//access判断文件是否存在
		printf("isp start.\n");
		isp_fd = open(isp_file, O_RDWR);
		if(isp_fd < 0) {
			printf("Open ISP fail\n");
			return -1;
		}
	}
	ret = start_isp();
	if(ret<0){
		printf("start isp device error.\n");
		close_isp();
		return -1;
	}

	init_isp_pw_frequency(g_iFw);

	adjust_brightness(g_stImg_file.brightness);
	adjust_saturation(g_stImg_file.saturation);
	adjust_hue(g_stImg_file.hue);
	adjust_contrast(g_stImg_file.contrast);
	adjust_shapness(g_stImg_file.flip);
	//	adjust_ae_max_exp(3000);

	return 0;
}



