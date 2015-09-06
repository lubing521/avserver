
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/poll.h>

#include "ipnc.h"
#include "drv_infrad_ioctl.h"
#include "sar_adc_ioctl.h"
#include "rtspd.h"

#include "av_globle.h"
#include "init_system.h"

#include "cmd_type.h"

#define INFRAD_DEV_NAME 	"/dev/infrad"
#define KEY_DEV_NAME    	"/dev/ftsar_adc010_0_key"

pthread_t		infradctrl_id;

int reset_stat = 0;

static void set_to_Color(int fd)
{
	adjust_saturation(getImg_saturation());//将图像还原为原始状态
	ioctl(fd, INFRAD_SET_OFF, 0);
}

static void set_to_White(int fd)
{
	adjust_saturation(0);//将图像转成黑白
	ioctl(fd, INFRAD_SET_ON, 1);
}

static int get_adc_value(int fd)
{
	int value = -1;

	if(ioctl(fd, SAR_ADC_KEY_ADC_DIRECT_READ, &value)!=0){
		printf("read adc value failed.\n");
		return -1;
	}

	return value;
}

static int get_reset_value(int fd)
{
	int value = -1;

	if(ioctl(fd, RESET_GET_STAT, &value)!=0){
		printf("read reset value failed.\n");
		return -1;
	}

	return value;	
}

static void reset_sys()
{
	char *cmd;
	char p_tmp[64];
	int fd;

	cmd = malloc(MAX_CMD_LEN*sizeof(char));
	if(cmd==NULL){
		return;
	}

	sprintf(p_tmp, "$%d=%d", e_TYPE, T_Set);
	strcpy(cmd, p_tmp);
	sprintf(p_tmp, "&%d=%d", e_reset, 1);
	strcat(cmd, p_tmp);
	sprintf(cmd, "#");

	fd = connect_local(UN_AVSERVER_DOMAIN);
	if(fd<0){
		printf("reset_sys fail: connect av_server failed.\n");
		return;
	}

	send_local(fd, cmd, MAX_CMD_LEN);

	close_local(fd);
	return;	
}


void *infrad_ctrl(void *ptr)
{
	int ret;
	int fd_infrad = -1;
	int fd_adc = -1;

	int adc_value = -1;

	fd_infrad = open(INFRAD_DEV_NAME, O_RDWR);
	if(fd_infrad<0){
		printf("infrad_ctrl: open device %s failed.\n", INFRAD_DEV_NAME);	
		goto exit_infrad_ctrl;
	}

	fd_adc = open(KEY_DEV_NAME, O_RDONLY|O_NONBLOCK);
	if(fd_adc<0){
		printf("infrad_ctrl: open device %s failed.\n", KEY_DEV_NAME);
		goto exit_infrad_ctrl;
	}

	printf("infrad_ctrl control start....\n");
	set_to_Color(fd_infrad);//开始强制关闭

	while(1){
		switch(getInfradStatue()){
		case 1://强制开
			set_to_Color(fd_infrad);
			break;
		case 2://强制关
			set_to_White(fd_infrad);
			break;
		case 0://自动
		default:
			adc_value = get_adc_value(fd_adc);
			if(adc_value<0){
				break;
			}
	
			if(adc_value>getImgWtoC()){
				set_to_Color(fd_infrad);
			}else if(adc_value<getImgCtoW()){
				set_to_White(fd_infrad);
			}
			break;
		}
		
		if(0==get_reset_value(fd_infrad)){
			reset_stat++;
		}else{
			reset_stat = 0;
		}
		if(reset_stat>2){
			reset_sys();
			reset_stat = 0;
		}

		sleep(2);
	}

exit_infrad_ctrl:
	if(fd_infrad>0){
		close(fd_infrad);
		fd_infrad = -1;
	}
	if(fd_adc>0){
		close(fd_adc);
		fd_adc = -1;
	}

	return;
}


void init_infrad()
{
	pthread_attr_t	attr;
	int ret;

	init_gbl(g_stInfrad_file.stat, g_stImg_file.imgctow[0], g_stImg_file.imgctow[1], g_stImg_file.saturation);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&infradctrl_id, &attr, &infrad_ctrl, NULL);
	pthread_attr_destroy(&attr);

	return;
}


