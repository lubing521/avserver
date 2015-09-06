/*
*2012.02.21 by Aaron 
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "interface.h"
#include "rtspd.h"
#include "ipnc.h"
#include "cmdpares.h"
#include "cmd_type.h"
#include "init_system.h"

#include "avi_save.h"
//#include "mt.h"

#define DH_SERVER 0

void set_httpport(int port)
{
	char *cmd;

	cmd = malloc(128);
	if(cmd==NULL){
		return;
	}

	sprintf(cmd, "killall /usr/bin/thttpd");
	system(cmd);

	sprintf(cmd, "/bin/sh /usr/bin/thttpd -C /web/html/thttpd.conf -p %d", port);
	system(cmd);

	return;
}

void set_imageconfig(Command_e cmd, Img_cfg_t img_data)
{
	int ret;
	Img_cfg_t stImage_file;

	ret = read_cfg_file(IMAGE_ATTR_FILE, &stImage_file, sizeof(Img_cfg_t));
	if(ret!=READ_CFG_FILE_OK){
		return;
	}

	switch(cmd){
	case cmd_imgattr:
		if(stImage_file.brightness!=img_data.brightness){
			stImage_file.brightness = img_data.brightness;
			adjust_brightness(stImage_file.brightness);
		}
		if(stImage_file.contrast!=img_data.contrast){
			stImage_file.contrast = img_data.contrast;
			adjust_contrast(stImage_file.contrast);
		}
		if(stImage_file.hue!=img_data.hue){
			stImage_file.hue = img_data.hue;
			adjust_hue(stImage_file.hue);
		}
		if(stImage_file.saturation!=img_data.saturation){
			stImage_file.saturation = img_data.saturation;
//			adjust_saturation(stImage_file.saturation);
			set_Img_saturation(img_data.saturation);
		}
		if(stImage_file.scene!=img_data.scene){
			stImage_file.scene = img_data.scene;
			if(stImage_file.scene==3){
				stImage_file.wb[0] = img_data.wb[0];
				stImage_file.wb[1] = img_data.wb[1];
				stImage_file.wb[2] = img_data.wb[2];
				set_isp_rgb_gain(0, stImage_file.wb[0]);
				set_isp_rgb_gain(1, stImage_file.wb[1]);
				set_isp_rgb_gain(2, stImage_file.wb[2]);
			}else{
				adjust_awb_scene_mode(stImage_file.scene);
			}
		}
		if(stImage_file.flip!=img_data.flip){
			stImage_file.flip = img_data.flip;
			set_flip(stImage_file.flip);
		}
		if(stImage_file.mirror!=img_data.mirror){
			stImage_file.mirror = img_data.mirror;
			set_mirror(stImage_file.mirror);
		}
		stImage_file.imgctow[0] = img_data.imgctow[0];
		stImage_file.imgctow[1] = img_data.imgctow[1];
		
		set_imgCtoW(img_data.imgctow[0]);
		set_imgWtoC(img_data.imgctow[1]);

		break;
	default:
		break;
	}

	ret = write_cfg_file(IMAGE_ATTR_FILE, &stImage_file, sizeof(stImage_file));

	return;
}

void set_infraredconfig(Command_e cmd, Infrared_cfg_t infrared_data)
{
	int ret;
	Infrared_cfg_t stInfrared_file;

	ret = read_cfg_file(INFRARED_CFG_FILE, &stInfrared_file, sizeof(Infrared_cfg_t));
	if(ret!=READ_CFG_FILE_OK){
		return;
	}

	stInfrared_file.stat = infrared_data.stat;
	//process the infrared infomation.
	set_infradstatus(infrared_data.stat);

	ret = write_cfg_file(INFRARED_CFG_FILE, &stInfrared_file, sizeof(Infrared_cfg_t));

	return;
}

void set_netconfig(Command_e cmd, Net_cfg_t net_data)
{
	int ret;
	Net_cfg_t stNet_file;
	char str[128];

	ret = read_cfg_file(NET_CFG_FILE, &stNet_file, sizeof(Net_cfg_t));
	if(ret!=READ_CFG_FILE_OK){
		return;
	}

	strcpy(&stNet_file, &net_data);

	sprintf(str, "netconf set"
				 "-ipaddr %s"
				 "-netmask %s"
				 "-gateway %s"
				 "-dhcp %s"
				 "-fdnsip %s"
				 "-sdnsip %s"
				 "-dnsstat %d"
				 "-hwaddr %s",
				 stNet_file.ip,
				 stNet_file.netmask,
				 stNet_file.gateway,
				 stNet_file.dhcpflag==0?"off":"on",
				 stNet_file.fdnsip,
				 stNet_file.sdnsip,
				 stNet_file.dnsstat,
				 stNet_file.macaddr
				 );
	system(str);

	return;
}

#define MAC_FILE		FILE_ROOT"if.cfg"
#define MAC_FILE_BCKUP	FILE_ROOT"if_bck.cfg"
void gen_if_cfg(char *ip, char *mac)
{
	FILE *fp;
	char cmd[64];
	
	fp = fopen("if.tmp", "w");
	if(fp==NULL){
		return;
	}
	fprintf(fp, "ip %s\n", ip);
	fprintf(fp, "mac %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	fflush(fp);
	fclose(fp);

	sprintf(cmd, "mv if.tmp %s > /dev/null", MAC_FILE);
	system(cmd);
	sprintf(cmd, "cp %s %s > /dev/null", MAC_FILE, MAC_FILE_BCKUP);
	system(cmd);

	return;
}


void set_portconfig(Command_e cmd, Port_cfg_t port_data)
{
	int ret;
	Port_cfg_t stPort_file;
	char str[32];

	ret = read_cfg_file(PORT_CFG_FILE, &stPort_file, sizeof(port_data));
	if(ret!=READ_CFG_FILE_OK){
		return;
	}
		
	system("killall -9 thttpd");

	sprintf(str, "thttpd -p %d -C thttpd.conf", stPort_file.httpport);
	system(str);

	//rtsp port暂时不能修改

	write_cfg_file(PORT_CFG_FILE, &stPort_file, sizeof(port_data));

	return;
}

void set_timerconfig(Command_e cmd, Time_cfg_t time_data)
{
	int ret;
	Time_cfg_t stTime_file;
	char str[32];
//	char log[64];

	ret = read_cfg_file(SYS_TIMER_FILE, &stTime_file, sizeof(Time_cfg_t));
	if(ret!=READ_CFG_FILE_OK){
		return;
	}

	sprintf(str, "date -s \"%s\" -u", stTime_file.sys_time);
//	sprintf(log, "echo %s >> /mnt/mtd/time.log", str);
//	system(log);
	system(str);
	
	if(stTime_file.ntpenable){
		system("BR_ntp_client");
	}else{
		system("killall -9 BR_ntp_client");
	}

	return;
}

static int process_ID(char *p)
{
	char cmd[128];
	char *file = "/tmp/process_ID.tmp";
	FILE *fp;
	int ret = -1;
	char *str;

	if(p==NULL)
		return -1;

	sprintf(cmd, "ps | grep \"%s\" > %s", p, file);
	system(cmd);

//	printf("process_ID_1: cmd = %s.\n", cmd);
	fp = fopen(file, "r");
	if(fp==NULL)
		return -1;

	memset(cmd, 0, sizeof(cmd));
	fgets(cmd, sizeof(cmd), fp);

	fclose(fp);
	unlink(file);

//	printf("process_ID_2: cmd = %s.\n", cmd);
	str = strtok(cmd, " ");
//	printf("process_ID_3: str = %s.\n", str);
	if(str!=NULL)
		ret = atoi(str);

	return ret;
}

int start_ntp()
{
	char cmd[256];
	int pid = -1;
	char *ntp_process = "/mnt/mtd/ntpclient";

	pid = process_ID(ntp_process);
	if(pid<0){
		return -1;
	}

	sprintf(cmd, "kill -9 %d", pid);
	system(cmd);

	if(g_stTimecfg_file.ntpenable){
		sprintf(cmd, "/mnt/mtd/ntpclient -h %s -s -i %d -c 0 &", g_stTimecfg_file.ntpserver, g_stTimecfg_file.ntpinterval*60);
		system(cmd);
	}

	return 0;
}

void kill_thttpd()
{
	char cmd[256];
	int pid = -1;
	char *thttpd_process = "/usr/sbin/thttpd -D -C /web/html/thttpd.conf";

//	printf("kill_thttpd: enter...\n");
	pid = process_ID(thttpd_process);
	if(pid<0){
		return;
	}

	sprintf(cmd, "kill -9 %d", pid);
	printf("kill_thttpd: %s.\n", cmd);
	system(cmd);
}

static int get_curProtocol()
{
	char *pfilename = "/mnt/nfs/curprotocol.cfg";
	char cbuf;
	FILE *fp = NULL;

	fp = fopen(pfilename, "r");
	if (NULL == fp)
		return 1;
	
	fread(&cbuf, 1, 1, fp);
	fclose(fp);

	if(cbuf == '0')
		return 0;
	else
		return 1;
}

static void set_Protocol(int p)
{
	char *pfilename = "/mnt/nfs/curprotocol.cfg";
	char *tmp_pfilename = "/tmp/pro.tmp";
	char cmd[256];
	FILE *fp = NULL;

	fp = fopen(tmp_pfilename, "w");
	if (NULL == fp)
		return;
	
	if (p == 0)
		fwrite("0", 1, 1, fp);
	else
		fwrite("1", 1, 1, fp);

	fclose(fp);
	fflush(fp);

	sprintf(cmd, "mv %s %s", tmp_pfilename, pfilename);
	system(cmd);
}

void rec_cfg_set(void)
{
	char cmd[128];
	int fd_sock = -1;

	sprintf(cmd, "$%d=%d&%d=%d&%d=%d#",
		e_avi_enable, g_stRec_cfg_file.enable,
		e_avi_TimeL, g_stRec_cfg_file.time_len,
		e_avi_schd, g_stRec_cfg_file.sch);

	fd_sock = connect_local(UN_AVI_DOMAIN);
	if (fd_sock < 0) {
		printf("rec_cfg_set: open avisave domain failed\n");
		return;
	}

	if (send_local(fd_sock, (char *)cmd, sizeof(cmd)) <= 0)
		printf("rec_cfg_set: send cmd failed\n");

	close_local(fd_sock);
	return;
}


enum{
	e_ENC_0_FILE = 0,
	e_ENC_1_FILE,
	e_ENC_FW_FILE,
	e_IMAGE_ATTR_FILE,
	e_OVERLAY_CFG_FILE,
	e_INFRARED_CFG_FILE,
	e_NET_CFG_FILE,
	e_PORT_CFG_FILE,
	e_UPNP_CFG_FILE,
	e_WF_CFG_FILE,
	e_DDNS_CFG_FILE,
	e_PTZCOM_CFG_FILE,
	e_MD_CFG_FILE,
	e_USER_FILE,
	e_SNAPTIMER_CFG_FILE,
	e_FTP_CFG_FILE,
	e_SMTP_CFG_FILE,
	e_VIDMASK_CFG_FILE,
	e_SYS_CFG_FILE,
	e_SYS_TIMER_FILE,
	e_PTZ_PRESET_FILE,
	e_CLOUD_CFG_FILE,
	e_REC_CFG_FILE,
	e_END_FILE
};

struct _tagFile_array{
	int index;
	char *filename;
	void *buf;
	int buf_len;
}File_array[] = {
	{e_ENC_0_FILE, ENC_FILE(0), &g_stAv_0_file, sizeof(g_stAv_0_file)},
	{e_ENC_1_FILE, ENC_FILE(1), &g_stAv_1_file, sizeof(g_stAv_1_file)},
	{e_ENC_FW_FILE, ENC_FW_FILE, &g_iFw, sizeof(g_iFw)},
	{e_IMAGE_ATTR_FILE, IMAGE_ATTR_FILE, &g_stImg_file, sizeof(g_stImg_file)},
	{e_OVERLAY_CFG_FILE, OVERLAY_CFG_FILE, g_arrstOsd_file, REGION_NUM*sizeof(g_arrstOsd_file[0])},
	{e_INFRARED_CFG_FILE, INFRARED_CFG_FILE, &g_stInfrad_file, sizeof(g_stInfrad_file)},
	{e_NET_CFG_FILE, NET_CFG_FILE, &g_stNet_file, sizeof(g_stNet_file)},
	{e_PORT_CFG_FILE, PORT_CFG_FILE, &g_stPort_file, sizeof(g_stPort_file)},
	{e_UPNP_CFG_FILE, UPNP_CFG_FILE, &g_stUpnp_file, sizeof(g_stUpnp_file)},
	{e_WF_CFG_FILE, WF_CFG_FILE, &g_stwfcfg_file, sizeof(g_stwfcfg_file)},
	{e_DDNS_CFG_FILE, DDNS_CFG_FILE, &g_stDDNS_file, sizeof(g_stDDNS_file)},
	{e_PTZCOM_CFG_FILE, PTZCOM_CFG_FILE, &g_stPtzcfg_file, sizeof(g_stPtzcfg_file)},
	{e_MD_CFG_FILE, MD_CFG_FILE, &g_stMdcfg_file, sizeof(g_stMdcfg_file)},
	{e_USER_FILE, USER_FILE, &g_stUsr_file, sizeof(g_stUsr_file)},
	{e_SNAPTIMER_CFG_FILE, SNAPTIMER_CFG_FILE, &g_stSnaptimercfg_file, sizeof(g_stSnaptimercfg_file)},
	{e_FTP_CFG_FILE, FTP_CFG_FILE, &g_stFtcfg_file, sizeof(g_stFtcfg_file)},
	{e_SMTP_CFG_FILE, SMTP_CFG_FILE, &g_stSmtpcfg_file, sizeof(g_stSmtpcfg_file)},
	{e_VIDMASK_CFG_FILE, VIDMASK_CFG_FILE, &g_stVidMaskcfg_file, sizeof(g_stVidMaskcfg_file)},
	{e_SYS_CFG_FILE, SYS_CFG_FILE, &g_stSyscfg_file, sizeof(g_stSyscfg_file)},
	{e_SYS_TIMER_FILE, SYS_TIMER_FILE, &g_stTimecfg_file, sizeof(g_stTimecfg_file)},
	{e_PTZ_PRESET_FILE, PTZ_PRESET_CFG_FILE, &g_stPtzpreset_file, sizeof(g_stPtzpreset_file)},
	{e_CLOUD_CFG_FILE, CLOUD_CFG_FILE, &g_stCloud_cfg_file, sizeof(g_stCloud_cfg_file)},
	{e_REC_CFG_FILE, REC_CFG_FILE, &g_stRec_cfg_file, sizeof(g_stRec_cfg_file)}
};

#define SET_FILE_CHANGED(flag, file_NO)	(flag |= (0x01 << (file_NO)))
#define CLR_FILE_CHANGED(flag, file_NO)	(flag &= (~(0x01 << (file_NO))))
#define IS_SET_FILE_CHANGED(flag, file_NO)	((flag) & (0x01 << (file_NO)))

/*
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
*/
#define DELT		0.00001

/*
fvx: 横向速度，正为左，负为右
fvy: 纵向速度，正为上，负为下
*/
void set_ptz(float fvx, float fvy, char* ptzcmd)
{
	int ptz_direct = -1;
	int ptz_speed = 0;
	int vx, vy;

	if(NULL==ptzcmd){
		return;
	}

	if(fabs(fvx)>1||fabs(fvy)>1){
		vx = (int)(fvx);
		vy = (int)(fvy);		
	}else{
		vx = (int)(fvx*64);
		vy = (int)(fvy*64);
	}

	if(0==vx&&0==vy){
		return;
	}else if(vx!=0&&0==vy){
		if(vx>0){
			ptz_direct = PAN_RIGHT;
			ptz_speed = vx;
		}else{
			ptz_direct = PAN_LEFT;
			ptz_speed = abs(vx);
		}
	}else if(vx==0&&vy!=0){
		if(vy>0){
			ptz_direct = TILT_UP;
			ptz_speed = vy;
		}else{
			ptz_direct = TILT_DOWN;
			ptz_speed = abs(vy);
		}
	}else{
		if(vx>0&&vy>0){
//			ptz_direct = PT_RIGHT_UP;
			ptz_direct = FOCUS_NEAR;
		}else if(vx>0&&vy<0){
//			ptz_direct = PT_RIGHT_DOWN;
			ptz_direct = IRIS_OPEN;
		}else if(vx<0&&vy>0){
//			ptz_direct = PT_LEFT_UP;
			ptz_direct = FOUCE_FAR;
		}else if(vx<0&&vy<0){
//			ptz_direct = PT_LEFT_DOWN;
			ptz_direct = IRIS_CLOSE;
		}
		ptz_speed = sqrt(vx*vx+vy*vy);	
	}

	sprintf(ptzcmd, "@%d@%d@%d\n", 0, ptz_direct, ptz_speed);
	
}

static char *get_prenamestring(char *name)
{
	int i = 0;

	while(name[i]!='\0'){
		i++;
	}

	i--;
	while((i>=0)&&(name[i]>='0')&&(name[i]<='9')){
		name[i] = '\0';
		i--;
	}

	return name;
}

static int get_presetIndex(char *name)
{
	int ret = 0;
	int i = 0;

	while(name[i]!='\0'){
		i++;
	}

	i--;
	while((i>=0)&&(name[i]>='0')&&(name[i]<='9')){
		i--;
	}

	ret = atoi(&name[i+1]);

	return ret;
}

#define SPE_PRESET_START	56
#define SPE_PRESET_END 64
int spe_preset_index[SPE_PRESET_END-SPE_PRESET_START] = {95, 101, 102, 103, 125, 126, 127, 128};
static int deal_ptz_preset(int cmd_type, int index, char *name, char *pRet, int *pfile_flag)
{
	char ptzcmd[128];
	char cmd_tmp[128];
	int i = 0;

//	printf("index = %d......\n");

	if(index<0||index>PTZ_PRESET_ALLNUM){
		sprintf(cmd_tmp, "&%d=%d&%d=%d", 
			e_ptz_preset, index, e_error, ERROR_PTZ_INDEX_OUT_RANGE);
		strcat(pRet, cmd_tmp);
		return -1;
	}

	if(cmd_type==T_Set){
		if(name[0]=='\0'){
			sprintf(cmd_tmp, "&%d=%d&%d=%s&%d=%d", 
				e_ptz_preset, index, e_ptz_presetname, name, e_error, ERROR_PTZ_PRESETNAME_EMPTY);
			strcat(pRet, cmd_tmp);
			return -1;
		}else{
			int tmp_index = -1;
			/*
			for(i=0; i<PTZ_PRESET_ALLNUM; i++){
				if(g_stPtzpreset_file.preset[i].isSet){
					if(!strcmp(name, g_stPtzpreset_file.preset[i].name)){
						sprintf(cmd_tmp, "&%d=%d&%d=%s&%d=%d", 
							e_ptz_preset, index, e_ptz_presetname, name, e_error, ERROR_PTZ_PRESETNAME_REPEAT);
						strcat(pRet, cmd_tmp);
						return -1;
					}
				}else if(tmp_index<0){
					tmp_index = i;
				}
			}
			*/
			tmp_index = get_presetIndex(name);
			tmp_index--;

			if(index==0){
				char *p;

				if(tmp_index == PTZ_PRESET_ALLNUM){
					sprintf(cmd_tmp,"&%d=%d", e_error, ERROR_PTZ_INDEX_OVER);
					strcat(pRet,cmd_tmp);
				}else{
					sprintf(cmd_tmp,"&%d=%d&%d=%s&%d=%d",
						e_ptz_preset,tmp_index+1, e_ptz_presetname, name, e_error, RESULT_OK);
					strcat(pRet,cmd_tmp);

					sprintf(ptzcmd, "@%d@%d@%d\n", 0, SET_PRESET, tmp_index+1);
					mctp_cmd_cb(PTZ, ptzcmd);
					printf("set %s\n",ptzcmd);

					g_stPtzpreset_file.preset[tmp_index].isSet = 1;
					strncpy(g_stPtzpreset_file.preset[tmp_index].name, name, sizeof(g_stPtzpreset_file.preset[tmp_index].name));

					p=get_prenamestring(name);
					for(i=SPE_PRESET_START; i<SPE_PRESET_END; i++){
//						printf("spe_preset_index[i-SPE_PRESET_START]-1 = %d.\n", spe_preset_index[i-SPE_PRESET_START]-1);
						if(g_stPtzpreset_file.preset[spe_preset_index[i-SPE_PRESET_START]-1].isSet)	continue;

						g_stPtzpreset_file.preset[spe_preset_index[i-SPE_PRESET_START]-1].isSet = 1;
						sprintf(g_stPtzpreset_file.preset[spe_preset_index[i-SPE_PRESET_START]-1].name, "%s%02d", p, spe_preset_index[i-SPE_PRESET_START]);
					}

					SET_FILE_CHANGED(*pfile_flag, e_PTZ_PRESET_FILE);
				}
			}else{
				char *p;

				sprintf(cmd_tmp,"&%d=%d&%d=%s&%d=%d",
					e_ptz_preset, index, e_ptz_presetname, name, e_error, RESULT_OK);
				strcat(pRet,cmd_tmp);

				g_stPtzpreset_file.preset[index-1].isSet = 1;
				strncpy(g_stPtzpreset_file.preset[index-1].name, name, sizeof(g_stPtzpreset_file.preset[index-1].name));

				p=get_prenamestring(name);
				for(i=SPE_PRESET_START; i<SPE_PRESET_END; i++){
//						printf("spe_preset_index[i-SPE_PRESET_START]-1 = %d.\n", spe_preset_index[i-SPE_PRESET_START]-1);
						if(g_stPtzpreset_file.preset[spe_preset_index[i-SPE_PRESET_START]-1].isSet)	continue;

						g_stPtzpreset_file.preset[spe_preset_index[i-SPE_PRESET_START]-1].isSet = 1;
						sprintf(g_stPtzpreset_file.preset[spe_preset_index[i-SPE_PRESET_START]-1].name, "%s%02d", p, spe_preset_index[i-SPE_PRESET_START]);
				}

				SET_FILE_CHANGED(*pfile_flag, e_PTZ_PRESET_FILE);

				sprintf(ptzcmd, "@%d@%d@%d\n", 0, SET_PRESET, index);
				mctp_cmd_cb(PTZ, ptzcmd);
				printf("set %s\n",ptzcmd);
			}
		}
	}else{
		if(index==0){
			sprintf(cmd_tmp, "&%d=%d&%d=%d", 
				e_ptz_preset, index, e_error, ERROR_PTZ_INDEX_OUT_RANGE);
			strcat(pRet, cmd_tmp);
			return -1;
		}

		if(g_stPtzpreset_file.preset[index-1].isSet){
			sprintf(cmd_tmp,"&%d=%d&%d=%s",
				e_ptz_preset, index, e_ptz_presetname, g_stPtzpreset_file.preset[index-1].name);
			strcat(pRet,cmd_tmp);
		}else{
			sprintf(cmd_tmp,"&%d=%d&%d=%d",
				e_ptz_preset, index, e_error, ERROR_PTZ_INDEX_NOT_EXIST);
			strcat(pRet,cmd_tmp);
		}
	}

	return 0;
}


#define MIN_SET_FPS		4
#define MAX_SET_BPS		5000
int processMsg(void *buf, int len, void *rbuf)
{
	int cmd_type = -1;
	char *pKey;
	int iKey;
	int iValue;
	unsigned char cValue;
	char *pValue;
	char *cmd_tmp;
	char *pInput = buf;
	int parseIndex = 0;
	int i = 0;

	char *pRet = rbuf;
	int ret = 0;
	int file_changed_flag = 0;
	void *p_tmp;

	int channel = -1;
	int sub_channel = 0;
	int witch_file = 0;
	int osd_region = -1;
	int index = -1;
	int index1 = -1;

	int if_cfg = 0;
	int osd_cfg = 0;
	int ntpserver_cfg = 0;

	int fd_socket = -1;
	int send_len = 0;

	float fValue_vx = .0;
	float fValue_vy = .0;
	char ptzcmd[128];
	int ptzpreset_index = 0;
	int ptz_preset_in = 0;
	char ptz_preset_name[32];

	int reboot = 0;
	int yl_set = 0;

	if (buf == NULL || rbuf == NULL)
		return -1;

	cmd_tmp = (char *)malloc(128*sizeof(char));
	if (cmd_tmp == NULL)
		return -2;

	//fprintf(stdout, "enter process.\n");
	do{
		pKey = ParseVars(pInput, &parseIndex);
		pValue = ParseVars(pInput, &parseIndex); 

		if(pKey==NULL || pValue==NULL)
			break;

		iKey = atoi(pKey);
		if(iKey<e_TYPE||iKey>e_END){
			continue;
		}
//		printf("key:%s  value:%s ikey:%d\n",pKey,pValue,iKey);
		switch(iKey){
		case e_TYPE:
			cmd_type = atoi(pValue);
			if (cmd_type > 1) {
				cmd_type = T_Set;
				yl_set = 1;		
			} else {
				yl_set = 0;
			}
			sprintf(cmd_tmp, "$%d=%d", e_TYPE, cmd_type);
			strcpy(pRet, cmd_tmp);
			break;
		case e_Chn:
			channel = atoi(pValue);
			sprintf(cmd_tmp, "&%d=%d", e_Chn, channel);
			strcat(pRet, cmd_tmp);
			break;
		case e_Sub_Chn:
			sub_channel = atoi(pValue);
			sprintf(cmd_tmp, "&%d=%d", e_Sub_Chn, sub_channel);
			strcat(pRet, cmd_tmp);
			break;
		case e_video_chn_num:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_video_chn_num, 2);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_audio_enable:
			if(cmd_type==T_Set){
				
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_audio_enable, 1);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_audio_enc_type:
			if(cmd_type==T_Set){

			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_audio_enc_type, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_audio_bitrate:
			if(cmd_type==T_Set){

			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_audio_bitrate, 128000);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_audio_rtspport:
			if(cmd_type==T_Set){

			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_audio_rtspport, 554);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_audio_samplesize:
			if(cmd_type==T_Set){

			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_audio_samplesize, 16);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_audio_samplerate:
			if(cmd_type==T_Set){

			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_audio_samplerate, 8000);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_video_addr:
			if(cmd_type==T_Get){
				if(channel==0){
					sprintf(cmd_tmp, "&%d=rtsp://%s/0", e_video_addr, g_stNet_file.ip);
				}else{
					sprintf(cmd_tmp, "&%d=rtsp://%s/4", e_video_addr, g_stNet_file.ip);
				}
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_encode_profile:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_encode_profile, 0);//ENC_TYPE_H264
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_profile_levels:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_profile_levels, 1);//ENC_TYPE_H264
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_video_SynchronizationPoint:
			iValue = atoi(pValue);
			if(cmd_type==T_Set&&iValue>=0&&iValue<=1){
				set_I_frame(iValue);
			}
			break;
		case e_FW:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if((iValue!=50)&&(iValue!=60)){
					break;
				}

				if(g_iFw!=iValue){
					g_iFw = iValue;
					init_isp_pw_frequency(iValue);
					SET_FILE_CHANGED(file_changed_flag, e_ENC_FW_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_FW, g_iFw);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_denoise:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->denoise != iValue){
					((Av_cfg_t *)p_tmp)->denoise = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_denoise, ((Av_cfg_t *)p_tmp)->denoise);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_input_system:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->input_system != iValue){
					((Av_cfg_t *)p_tmp)->input_system = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_input_system, ((Av_cfg_t *)p_tmp)->input_system);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_de_interlace:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->de_interlace != iValue){
					((Av_cfg_t *)p_tmp)->de_interlace = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_de_interlace, ((Av_cfg_t *)p_tmp)->de_interlace);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_Stream_enable:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].stream_enable != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].stream_enable = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_Stream_enable, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].stream_enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_enc_type:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].enc_type != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].enc_type = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_enc_type, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].enc_type);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_frame_rate:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(iValue < MIN_SET_FPS){
					iValue = MIN_SET_FPS;
				}
				if (iValue > 150) {
					iValue = 150;
				}

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].frame_rate != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].frame_rate = iValue;
					//............

					set_fps(channel, sub_channel, iValue);
					if (!yl_set)
						SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_frame_rate, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].frame_rate);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_min_frame_rate:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_min_frame_rate, 10);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_max_frame_rate:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_max_frame_rate, 30);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_bit_rate:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(iValue > MAX_SET_BPS){
					iValue = MAX_SET_BPS;
				}
				if (channel == MAIN_STREAM_CHN) {
					if (iValue < 512)
						iValue = 512;
				} else {
					if (iValue < 100)
						iValue = 100;
				}

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].bit_rate != iValue*1000){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].bit_rate = iValue*1000;
					//............
					set_bps(channel, sub_channel, iValue*1000);
//					printf("=======to set bit_rate:%d.\n", ((Av_cfg_t *)p_tmp)->ubs[sub_channel].bit_rate);
//					printf("=======to g_stAv_0_file set bit_rate:%d.\n", g_stAv_0_file.ubs[sub_channel].bit_rate);
					if (!yl_set)
						SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_bit_rate, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].bit_rate/1000);
				strcat(pRet, cmd_tmp);

//				printf("=======to get bit_rate:%d.\n", ((Av_cfg_t *)p_tmp)->ubs[sub_channel].bit_rate/1000);

				ret++;
			}
			break;
		case e_ip_interval:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].ip_interval != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].ip_interval = iValue;
					//............
					set_ip_int(channel, sub_channel, iValue);
					if (!yl_set)
						SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ip_interval, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].ip_interval);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_min_ip_interval:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_min_ip_interval, 25);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_max_ip_interval:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_max_ip_interval, 100);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_width:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
#if 0
				iValue = atoi(pValue);
				if(iValue==1920){
					//切换为1080
					if(g_stAv_0_file.ubs[0].width!=1920){
						if (g_stAv_1_file.ubs[0].width == 352)
							system("/bin/sh /mnt/mtd/switch.sh 1 0");
						else
							system("/bin/sh /mnt/mtd/switch.sh 1 2");
					}

				}else{
					//切换为720
					if(g_stAv_0_file.ubs[0].width!=1280){
						if (g_stAv_1_file.ubs[0].width == 352)
							system("/bin/sh /mnt/mtd/switch.sh 0");
					}
				}
#endif
/*
				iValue = atoi(pValue);

				((Av_cfg_t *)p_tmp)->ubs[sub_channel].width = iValue;
				//............
				SET_FILE_CHANGED(file_changed_flag, witch_file);
*/
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_width, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].width);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_height:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
/*
				iValue = atoi(pValue);

				((Av_cfg_t *)p_tmp)->ubs[sub_channel].height = iValue;
				//............
				SET_FILE_CHANGED(file_changed_flag, witch_file);
*/
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_height, 
							(((Av_cfg_t *)p_tmp)->ubs[sub_channel].height==1072)?1080:((Av_cfg_t *)p_tmp)->ubs[sub_channel].height);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_videoResolutions:
			if(cmd_type==T_Get){
				if(channel==0)
					sprintf(cmd_tmp, "&%d=%d,%d/%d,%d", e_videoResolutions, 1920, 1080, 1280, 720);
				else
					sprintf(cmd_tmp, "&%d=%d,%d", e_videoResolutions, 352, 288);

				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_min_videoEncodingInterval:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_min_videoEncodingInterval, 1);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_max_videoEncodingInterval:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_max_videoEncodingInterval, 1);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_rate_ctl_type:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].rate_ctl_type != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].rate_ctl_type = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_rate_ctl_type, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].rate_ctl_type);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_target_rate_max:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].target_rate_max != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].target_rate_max = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_target_rate_max, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].target_rate_max);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_reaction_delay_max:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].reaction_delay_max != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].reaction_delay_max = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_reaction_delay_max, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].reaction_delay_max);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_init_quant:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].init_quant != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].init_quant = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_init_quant, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].init_quant);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_max_quant:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].max_quant != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].max_quant = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_max_quant, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].max_quant);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_min_quant:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].min_quant != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].min_quant = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_min_quant, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].min_quant);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_mjpeg_quality:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].mjpeg_quality != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].mjpeg_quality = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_mjpeg_quality, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].mjpeg_quality);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_enable_roi:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].enabled_roi != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].enabled_roi = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_enable_roi, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].enabled_roi);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_roi_x:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_x != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_x = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_roi_x, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_x);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_roi_y:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_y != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_y = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_roi_y, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_y);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_roi_w:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_w != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_w = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_roi_w, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_w);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_roi_h:
			if(channel==MAIN_STREAM_CHN){
				p_tmp = (void *)&g_stAv_0_file;
				witch_file = e_ENC_0_FILE;
			}else{
				p_tmp = (void *)&g_stAv_1_file;
				witch_file = e_ENC_1_FILE;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_h != iValue){
					((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_h = iValue;
					//............
					SET_FILE_CHANGED(file_changed_flag, witch_file);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_roi_h, ((Av_cfg_t *)p_tmp)->ubs[sub_channel].roi_h);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_brightness:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.brightness!=iValue){
					g_stImg_file.brightness = iValue;
					adjust_brightness(g_stImg_file.brightness);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_brightness, g_stImg_file.brightness);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_saturation:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.saturation!=iValue){
					g_stImg_file.saturation = iValue;
					adjust_saturation(g_stImg_file.saturation);
					set_Img_saturation(g_stImg_file.saturation);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_saturation, g_stImg_file.saturation);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_contrast:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.contrast!=iValue){
					g_stImg_file.contrast = iValue;
					adjust_contrast(g_stImg_file.contrast);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_contrast, g_stImg_file.contrast);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sharpness:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_sharpness, 50);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_hue:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.hue!=iValue){
					g_stImg_file.hue = iValue;
					adjust_hue(g_stImg_file.hue);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_hue, g_stImg_file.hue);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_scene:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.scene!=iValue){
					g_stImg_file.scene = iValue;
					adjust_awb_scene_mode(g_stImg_file.scene);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_scene, g_stImg_file.scene);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_flip:	//被修改为锐度值
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.flip!=iValue){
					g_stImg_file.flip = iValue;
//					set_flip(g_stImg_file.flip);
					adjust_shapness(g_stImg_file.flip);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_flip, g_stImg_file.flip);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_mirror:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.mirror!=iValue){
					g_stImg_file.mirror = iValue;
					set_mirror(g_stImg_file.mirror);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_mirror, g_stImg_file.mirror);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_imgctow:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.imgctow[0]!=iValue){
					g_stImg_file.imgctow[0] = iValue;
					set_imgCtoW(g_stImg_file.imgctow[0]);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_imgctow, g_stImg_file.imgctow[0]);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_imgwtoc:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.imgctow[1]!=iValue){
					g_stImg_file.imgctow[1] = iValue;
					set_imgWtoC(g_stImg_file.imgctow[1]);
					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_imgwtoc, g_stImg_file.imgctow[1]);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wb_r:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.wb[0]!=iValue){
					g_stImg_file.wb[0] = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wb_r, g_stImg_file.wb[0]);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wb_g:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.wb[1]!=iValue){
					g_stImg_file.wb[1] = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wb_g, g_stImg_file.wb[1]);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wb_b:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);

				if(g_stImg_file.wb[2]!=iValue){
					g_stImg_file.wb[2] = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_IMAGE_ATTR_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wb_b, g_stImg_file.wb[2]);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wb_mode:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wb_mode, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wb_crgain:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wb_crgain, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wb_cbgain:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wb_cbgain, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_backlightcomp_mode:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_backlightcomp_mode, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_backlightcomp_level:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_backlightcomp_level, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wdrange_mode:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wdrange_mode, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wdrange_level:
			if(cmd_type==T_Set){
				//TODO:
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wdrange_level, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_osd_region:
			osd_region = atoi(pValue);
			sprintf(cmd_tmp, "&%d=%d", e_osd_region, osd_region);
			strcat(pRet, cmd_tmp);
			break;
		case e_osd_region_name:
			if(osd_region<0||osd_region>=REGION_NUM){
				break;
			}
			if(cmd_type==T_Set){
				if(strcmp(g_arrstOsd_file[osd_region].name, pValue) != 0){
					strcpy(g_arrstOsd_file[osd_region].name, pValue);

					SET_FILE_CHANGED(file_changed_flag, e_OVERLAY_CFG_FILE);
					osd_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_osd_region_name, g_arrstOsd_file[osd_region].name);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_osd_enable:
			if(osd_region<0||osd_region>=REGION_NUM){
				break;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_arrstOsd_file[osd_region].enable != iValue){
					g_arrstOsd_file[osd_region].enable = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_OVERLAY_CFG_FILE);
					osd_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_osd_enable, g_arrstOsd_file[osd_region].enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_osd_x:
			if(osd_region<0||osd_region>=REGION_NUM){
				break;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_arrstOsd_file[osd_region].x != iValue){
					g_arrstOsd_file[osd_region].x = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_OVERLAY_CFG_FILE);
					osd_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_osd_x, g_arrstOsd_file[osd_region].x);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_osd_y:
			if(osd_region<0||osd_region>=REGION_NUM){
				break;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_arrstOsd_file[osd_region].y != iValue){
					g_arrstOsd_file[osd_region].y = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_OVERLAY_CFG_FILE);
					osd_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_osd_y, g_arrstOsd_file[osd_region].y);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_osd_w:
			if(osd_region<0||osd_region>=REGION_NUM){
				break;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_arrstOsd_file[osd_region].w != iValue){
					g_arrstOsd_file[osd_region].w = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_OVERLAY_CFG_FILE);
					osd_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_osd_w, g_arrstOsd_file[osd_region].w);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_osd_h:
			if(osd_region<0||osd_region>=REGION_NUM){
				break;
			}
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_arrstOsd_file[osd_region].h != iValue){
					g_arrstOsd_file[osd_region].h = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_OVERLAY_CFG_FILE);
					osd_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_osd_h, g_arrstOsd_file[osd_region].h);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_infrad_stat:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stInfrad_file.stat != iValue){
					g_stInfrad_file.stat = iValue;
					set_infradstatus(g_stInfrad_file.stat);
					SET_FILE_CHANGED(file_changed_flag, e_INFRARED_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_infrad_stat, g_stInfrad_file.stat);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_dhcpflag:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stNet_file.dhcpflag != iValue){
					g_stNet_file.dhcpflag = iValue;
/*
					if(g_stNet_file.dhcpflag){
						net_enable_dhcpcd();
					}else{
						net_disable_dhcpcd();
					}
*/
					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_net_dhcpflag, g_stInfrad_file.stat);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_ip:
			if(cmd_type==T_Set){
				if(strcmp(g_stNet_file.ip, pValue)!=0){
					strcpy(g_stNet_file.ip, pValue);

//					net_set_ifaddr("eth0", inet_addr(g_stNet_file.ip));

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
					if_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_net_ip, g_stNet_file.ip);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_netmask:
			if(cmd_type==T_Set){
				if(strcmp(g_stNet_file.netmask, pValue)!=0){
					strcpy(g_stNet_file.netmask, pValue);

					net_set_netmask("eth0", inet_addr(g_stNet_file.netmask));

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_net_netmask, g_stNet_file.netmask);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_gateway:
			if(cmd_type==T_Set){
				if(strcmp(g_stNet_file.gateway, pValue)!=0){
					strcpy(g_stNet_file.gateway, pValue);

//					net_set_gateway(inet_addr(g_stNet_file.gateway));

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_net_gateway, g_stNet_file.gateway);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_dnsstat:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stNet_file.dnsstat!=iValue){
					g_stNet_file.dnsstat = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_net_dnsstat, g_stNet_file.dnsstat);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_fdnsip:
			if(cmd_type==T_Set){
				if(strcmp(g_stNet_file.fdnsip, pValue)!=0){
					strcpy(g_stNet_file.fdnsip, pValue);

//					net_set_dns(g_stNet_file.fdnsip);

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_net_fdnsip, g_stNet_file.fdnsip);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_sdnsip:
			if(cmd_type==T_Set){
				if(strcmp(g_stNet_file.sdnsip, pValue)!=0){
					strcpy(g_stNet_file.sdnsip, pValue);

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_net_sdnsip, g_stNet_file.sdnsip);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_macaddr:
			if(cmd_type==T_Set){
				sprintf(cmd_tmp, "%02x-%02x-%02x-%02x-%02x-%02x",
								g_stNet_file.macaddr[0], g_stNet_file.macaddr[1], g_stNet_file.macaddr[2],
								g_stNet_file.macaddr[3], g_stNet_file.macaddr[4], g_stNet_file.macaddr[5]);
				if(strcmp(cmd_tmp, pValue)!=0){
					sscanf(pValue, "%02x-%02x-%02x-%02x-%02x-%02x",
								&g_stNet_file.macaddr[0], &g_stNet_file.macaddr[1], &g_stNet_file.macaddr[2],
								&g_stNet_file.macaddr[3], &g_stNet_file.macaddr[4], &g_stNet_file.macaddr[5]);

//					sprintf(cmd_tmp, "%02x:%02x:%02x:%02x:%02x:%02x", 
//								g_stNet_file.macaddr[0], g_stNet_file.macaddr[1], g_stNet_file.macaddr[2],
//								g_stNet_file.macaddr[3], g_stNet_file.macaddr[4], g_stNet_file.macaddr[5]);
//					net_set_hwaddr("eth0", cmd_tmp);

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
					if_cfg = 1;
				}
			}else if(cmd_type==T_Get){
				net_get_hwaddr("eth0", g_stNet_file.macaddr);

				sprintf(cmd_tmp, "&%d=%02x-%02x-%02x-%02x-%02x-%02x", e_net_macaddr,
							g_stNet_file.macaddr[0], g_stNet_file.macaddr[1], g_stNet_file.macaddr[2],
							g_stNet_file.macaddr[3], g_stNet_file.macaddr[4], g_stNet_file.macaddr[5]);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_nettype:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stNet_file.nettype!=iValue){
					g_stNet_file.nettype = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_NET_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_net_nettype, g_stNet_file.dnsstat);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_cardname:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_net_cardname, "eth0");
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_net_protocols:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_net_protocols, 0);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_port_httpport:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPort_file.httpport!=iValue){
					g_stPort_file.httpport = iValue;

					set_httpport(g_stPort_file.httpport);

					SET_FILE_CHANGED(file_changed_flag, e_PORT_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_port_httpport, g_stPort_file.httpport);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_port_rtspport:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPort_file.rtspport!=iValue){
					g_stPort_file.rtspport = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_PORT_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_port_rtspport, g_stPort_file.rtspport);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_upnp_upmenable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stUpnp_file.upm_enable!=iValue){
					g_stUpnp_file.upm_enable = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_UPNP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_upnp_upmenable, g_stUpnp_file.upm_enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ddns_d3thenable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stDDNS_file.d3th_enable!=iValue){
					g_stDDNS_file.d3th_enable = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_DDNS_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ddns_d3thenable, g_stDDNS_file.d3th_enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ddns_d3thservice:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stDDNS_file.d3th_service!=iValue){
					g_stDDNS_file.d3th_service = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_DDNS_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ddns_d3thservice, g_stDDNS_file.d3th_service);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ddns_d3thuname:
			if(cmd_type==T_Set){
				if(strcmp(g_stDDNS_file.d3th_uname, pValue)!=0){
					strcpy(g_stDDNS_file.d3th_uname, pValue);

					SET_FILE_CHANGED(file_changed_flag, e_DDNS_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_ddns_d3thuname, g_stDDNS_file.d3th_uname);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ddns_d3thpasswd:
			if(cmd_type==T_Set){
				if(strcmp(g_stDDNS_file.d3th_passwd, pValue)!=0){
					strcpy(g_stDDNS_file.d3th_passwd, pValue);

					SET_FILE_CHANGED(file_changed_flag, e_DDNS_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_ddns_d3thpasswd, g_stDDNS_file.d3th_passwd);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ddns_domain:
			if(cmd_type==T_Set){
				if(strcmp(g_stDDNS_file.d3th_domain, pValue)!=0){
					strcpy(g_stDDNS_file.d3th_domain, pValue);

					SET_FILE_CHANGED(file_changed_flag, e_DDNS_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_ddns_domain, g_stDDNS_file.d3th_domain);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wf_enable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stwfcfg_file.wf_enable !=iValue){
					g_stwfcfg_file.wf_enable = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_WF_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wf_enable, g_stwfcfg_file.wf_enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wf_ssid:
			if(cmd_type==T_Set){
				if(strcmp(g_stwfcfg_file.wf_ssid, pValue)!=0){
					strcpy(g_stwfcfg_file.wf_ssid, pValue);

					SET_FILE_CHANGED(file_changed_flag, e_WF_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_wf_ssid, g_stwfcfg_file.wf_ssid);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wf_auth:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stwfcfg_file.wf_auth !=iValue){
					g_stwfcfg_file.wf_auth = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_WF_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wf_auth, g_stwfcfg_file.wf_auth);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wf_key:
			if(cmd_type==T_Set){
				if(strcmp(g_stwfcfg_file.wf_key, pValue)!=0){
					strcpy(g_stwfcfg_file.wf_key, pValue);

					SET_FILE_CHANGED(file_changed_flag, e_WF_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_wf_key, g_stwfcfg_file.wf_key);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wf_enc:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stwfcfg_file.wf_enc !=iValue){
					g_stwfcfg_file.wf_enc = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_WF_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wf_enc, g_stwfcfg_file.wf_enc);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wf_mode:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stwfcfg_file.wf_mode !=iValue){
					g_stwfcfg_file.wf_mode = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_WF_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wf_mode, g_stwfcfg_file.wf_mode);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wfsearch_accesspoints:
			if(cmd_type==T_Get){
				iValue = atoi(pValue);
				if(iValue>=g_stWf_search_file.waccess_points)	break;
				index = iValue;
				sprintf(cmd_tmp, "&%d=%d", e_wfsearch_accesspoints, index);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wfsearch_channel:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wfsearch_channel, g_stWf_search_file.search_Param[index].wchannel);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wfsearch_rssi:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wfsearch_rssi, g_stWf_search_file.search_Param[index].wrssi);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wfsearch_essid:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_wfsearch_essid, g_stWf_search_file.search_Param[index].wessid);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wfsearch_enc:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wfsearch_enc, g_stWf_search_file.search_Param[index].wenc);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wfsearch_auth:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wfsearch_auth, g_stWf_search_file.search_Param[index].wauth);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_wfsearch_net:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_wfsearch_net, g_stWf_search_file.search_Param[index].wnet);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ptz_protocal:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPtzcfg_file.protocal !=iValue){
					g_stPtzcfg_file.protocal = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_PTZCOM_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_protocal, g_stPtzcfg_file.protocal);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ptz_address:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPtzcfg_file.address !=iValue){
					g_stPtzcfg_file.address = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_PTZCOM_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_address, g_stPtzcfg_file.address);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ptz_baud:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPtzcfg_file.baud !=iValue){
					g_stPtzcfg_file.baud = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_PTZCOM_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_baud, g_stPtzcfg_file.baud);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ptz_databit:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPtzcfg_file.databit !=iValue){
					g_stPtzcfg_file.databit = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_PTZCOM_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_databit, g_stPtzcfg_file.databit);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ptz_stopbit:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPtzcfg_file.stopbit !=iValue){
					g_stPtzcfg_file.stopbit = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_PTZCOM_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_stopbit, g_stPtzcfg_file.stopbit);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ptz_check:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stPtzcfg_file.check !=iValue){
					g_stPtzcfg_file.check = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_PTZCOM_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_check, g_stPtzcfg_file.check);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_io_alarm_enable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.io_alarm_enable !=iValue){
					g_stMdcfg_file.io_alarm_enable = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_io_alarm_enable, g_stMdcfg_file.io_alarm_enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_io_alarm_flag:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.io_alarm_flag !=iValue){
					g_stMdcfg_file.io_alarm_flag = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_io_alarm_flag, g_stMdcfg_file.io_alarm_flag);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_email_switch:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.email_switch !=iValue){
					g_stMdcfg_file.email_switch = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_email_switch, g_stMdcfg_file.email_switch);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_snap_switch:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.snap_switch !=iValue){
					g_stMdcfg_file.snap_switch = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_snap_switch, g_stMdcfg_file.snap_switch);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_record_switch:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.record_switch !=iValue){
					g_stMdcfg_file.record_switch = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_record_switch, g_stMdcfg_file.record_switch);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_ftp_switch:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.ftp_switch !=iValue){
					g_stMdcfg_file.ftp_switch = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_ftp_switch, g_stMdcfg_file.ftp_switch);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_relay_switch:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.relay_switch !=iValue){
					g_stMdcfg_file.relay_switch = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_relay_switch, g_stMdcfg_file.relay_switch);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_relay_time:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.relay_time !=iValue){
					g_stMdcfg_file.relay_time = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_relay_time, g_stMdcfg_file.relay_time);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_type:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.type!=iValue){
					g_stMdcfg_file.schedule.type = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_type, g_stMdcfg_file.schedule.type);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_ename:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.ename!=iValue){
					g_stMdcfg_file.schedule.ename = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_ename, g_stMdcfg_file.schedule.ename);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_etm:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.etm!=iValue){
					g_stMdcfg_file.schedule.etm = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_etm, g_stMdcfg_file.schedule.etm);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_workday_Tend:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.stWorkday.t_end!=iValue){
					g_stMdcfg_file.schedule.stWorkday.t_end = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_workday_Tend, g_stMdcfg_file.schedule.stWorkday.t_end);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_workday_Tstart:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.stWorkday.t_start!=iValue){
					g_stMdcfg_file.schedule.stWorkday.t_start = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_workday_Tstart, g_stMdcfg_file.schedule.stWorkday.t_start);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_workend_Tstart:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.stWeekend.t_start!=iValue){
					g_stMdcfg_file.schedule.stWeekend.t_start = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_workend_Tstart, g_stMdcfg_file.schedule.stWeekend.t_start);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_workend_Tend:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.stWeekend.t_end!=iValue){
					g_stMdcfg_file.schedule.stWeekend.t_end = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_workend_Tend, g_stMdcfg_file.schedule.stWeekend.t_end);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case  e_md_schedule_week_Tstart:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.astWeek[0].t_start!=iValue){
					g_stMdcfg_file.schedule.astWeek[0].t_start = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_week_Tstart, g_stMdcfg_file.schedule.astWeek[0].t_start);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_schedule_week_Tend:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.schedule.astWeek[0].t_end!=iValue){
					g_stMdcfg_file.schedule.astWeek[0].t_end = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_schedule_week_Tend, g_stMdcfg_file.schedule.astWeek[0].t_end);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_area:
			iValue = atoi(pValue);
			index = iValue;
			if(index>=4)	break;
			sprintf(cmd_tmp, "&%d=%d", e_md_area, index);
			strcat(pRet, cmd_tmp);
			ret++;
			break;
		case e_md_area_eable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.area_param[index].enable!=iValue){
					g_stMdcfg_file.area_param[index].enable = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_area_eable, g_stMdcfg_file.area_param[index].enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_area_s:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.area_param[index].s!=iValue){
					g_stMdcfg_file.area_param[index].s = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_area_eable, g_stMdcfg_file.area_param[index].s);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_area_method:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.area_param[index].method!=iValue){
					g_stMdcfg_file.area_param[index].method = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_area_method, g_stMdcfg_file.area_param[index].method);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_area_x:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.area_param[index].x!=iValue){
					g_stMdcfg_file.area_param[index].x = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_area_x, g_stMdcfg_file.area_param[index].x);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_area_y:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.area_param[index].y!=iValue){
					g_stMdcfg_file.area_param[index].y = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_area_y, g_stMdcfg_file.area_param[index].y);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_area_w:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.area_param[index].w!=iValue){
					g_stMdcfg_file.area_param[index].w = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_area_y, g_stMdcfg_file.area_param[index].w);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_md_area_h:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stMdcfg_file.area_param[index].h!=iValue){
					g_stMdcfg_file.area_param[index].h = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_MD_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_md_area_y, g_stMdcfg_file.area_param[index].h);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_snapT_enable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stSnaptimercfg_file.as_enable!=iValue){
					g_stSnaptimercfg_file.as_enable = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_SNAPTIMER_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_snapT_enable, g_stSnaptimercfg_file.as_enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_snapT_interval:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stSnaptimercfg_file.as_interval!=iValue){
					g_stSnaptimercfg_file.as_interval = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_SNAPTIMER_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_snapT_interval, g_stSnaptimercfg_file.as_interval);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_snapT_type:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stSnaptimercfg_file.as_type!=iValue){
					g_stSnaptimercfg_file.as_type = iValue;

					SET_FILE_CHANGED(file_changed_flag, e_SNAPTIMER_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_snapT_type, g_stSnaptimercfg_file.as_type);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_user_number:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_user_number, g_stUsr_file.user_num);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_user_opt_type:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(iValue<0||iValue>MODIFY_USER)	break;
				index = iValue;
			}
			break;
		case e_user_name:
			if(cmd_type==T_Set){
				switch(index){
				case ADD_USER:
					for(i=0; i<USER_MAX; i++){
						if(g_stUsr_file.stUser[i].username[0]=='\0')
							break;
					}
					if(i==USER_MAX)	break;
					strcpy(g_stUsr_file.stUser[i].username, pValue);
					break;
				case DEL_USER:
					for(i=0; i<USER_MAX; i++){
						if(strcmp(g_stUsr_file.stUser[i].username, pValue)==0){
							g_stUsr_file.stUser[i].username[0] = '\0';
							break;
						}
					}
					break;
				case MODIFY_USER:
					for(i=0; i<USER_MAX; i++){
						if(strcmp(g_stUsr_file.stUser[i].username, pValue)==0)
							break;
					}
					break;
				}
			}else if(cmd_type==T_Get){
				for(i=0; i<USER_MAX; i++){
					if(strcmp(g_stUsr_file.stUser[i].username, pValue)==0)
						break;
				}
			}
			index1 = i;
			sprintf(cmd_tmp, "&%d=%s", e_user_name, g_stUsr_file.stUser[index1].username);
			strcat(pRet, cmd_tmp);
			ret++;
			break;
		case e_user_password:
			if(cmd_type==T_Set){
				strcpy(g_stUsr_file.stUser[index1].password, pValue);
				SET_FILE_CHANGED(file_changed_flag, e_USER_FILE);
			}else if(cmd_type==T_Get){

			}
			break;
		case e_user_group:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				g_stUsr_file.stUser[index1].group = iValue;
				SET_FILE_CHANGED(file_changed_flag, e_USER_FILE);
			}else if(cmd_type==T_Get){

			}
			break;
		case e_ft_serverip:
			if(cmd_type==T_Set){
				if(strcmp(g_stFtcfg_file.serverip, pValue)!=0){
					strcpy(g_stFtcfg_file.serverip, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_FTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_ft_serverip, g_stFtcfg_file.serverip);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ft_port:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stFtcfg_file.port!=iValue){
					g_stFtcfg_file.port = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_FTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ft_port, g_stFtcfg_file.port);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ft_username:
			if(cmd_type==T_Set){
				if(strcmp(g_stFtcfg_file.username, pValue)!=0){
					strcpy(g_stFtcfg_file.username, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_FTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_ft_username, g_stFtcfg_file.username);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ft_password:
			if(cmd_type==T_Set){
				if(strcmp(g_stFtcfg_file.password, pValue)!=0){
					strcpy(g_stFtcfg_file.password, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_FTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_ft_password, g_stFtcfg_file.password);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ft_mode:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stFtcfg_file.mode!=iValue){
					g_stFtcfg_file.mode = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_FTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ft_mode, g_stFtcfg_file.mode);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_ft_dirname:
			if(cmd_type==T_Set){
				if(strcmp(g_stFtcfg_file.dirname, pValue)!=0){
					strcpy(g_stFtcfg_file.dirname, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_FTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_ft_dirname, g_stFtcfg_file.dirname);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_serverip:
			if(cmd_type==T_Set){
				if(strcmp(g_stSmtpcfg_file.serverip, pValue)!=0){
					strcpy(g_stSmtpcfg_file.serverip, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_smtp_serverip, g_stSmtpcfg_file.serverip);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_port:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stSmtpcfg_file.port!=iValue){
					g_stSmtpcfg_file.port = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_smtp_port, g_stSmtpcfg_file.port);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_sslflag:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stSmtpcfg_file.ssl_flag!=iValue){
					g_stSmtpcfg_file.ssl_flag = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_smtp_sslflag, g_stSmtpcfg_file.ssl_flag);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_logintype:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stSmtpcfg_file.logintype!=iValue){
					g_stSmtpcfg_file.logintype = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_smtp_logintype, g_stSmtpcfg_file.logintype);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_username:
			if(cmd_type==T_Set){
				if(strcmp(g_stSmtpcfg_file.username, pValue)!=0){
					strcpy(g_stSmtpcfg_file.username, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_smtp_username, g_stSmtpcfg_file.username);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_password:
			if(cmd_type==T_Set){
				if(strcmp(g_stSmtpcfg_file.password, pValue)!=0){
					strcpy(g_stSmtpcfg_file.password, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_smtp_password, g_stSmtpcfg_file.password);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_from:
			if(cmd_type==T_Set){
				if(strcmp(g_stSmtpcfg_file.from, pValue)!=0){
					strcpy(g_stSmtpcfg_file.from, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_smtp_from, g_stSmtpcfg_file.from);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_to:
			if(cmd_type==T_Set){
				if(strcmp(g_stSmtpcfg_file.to, pValue)!=0){
					strcpy(g_stSmtpcfg_file.to, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_smtp_to, g_stSmtpcfg_file.to);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_subject:
			if(cmd_type==T_Set){
				if(strcmp(g_stSmtpcfg_file.subject, pValue)!=0){
					strcpy(g_stSmtpcfg_file.subject, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_smtp_subject, g_stSmtpcfg_file.subject);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_smtp_text:
			if(cmd_type==T_Set){
				if(strcmp(g_stSmtpcfg_file.text, pValue)!=0){
					strcpy(g_stSmtpcfg_file.text, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SMTP_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_smtp_text, g_stSmtpcfg_file.text);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_vdmask_number:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_vdmask_number, g_stVidMaskcfg_file.aeraNum);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_vdmask_NO:
//			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				index = iValue;
//			}
			break;
		case e_vdmask_enable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stVidMaskcfg_file.aera[index].enable!=iValue){
					g_stVidMaskcfg_file.aera[index].enable = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_VIDMASK_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_vdmask_enable, g_stVidMaskcfg_file.aera[index].enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_vdmask_x:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stVidMaskcfg_file.aera[index].x!=iValue){
					g_stVidMaskcfg_file.aera[index].x = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_VIDMASK_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_vdmask_x, g_stVidMaskcfg_file.aera[index].x);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_vdmask_y:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stVidMaskcfg_file.aera[index].y!=iValue){
					g_stVidMaskcfg_file.aera[index].y = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_VIDMASK_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_vdmask_y, g_stVidMaskcfg_file.aera[index].y);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_vdmask_w:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stVidMaskcfg_file.aera[index].w!=iValue){
					g_stVidMaskcfg_file.aera[index].w = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_VIDMASK_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_vdmask_w, g_stVidMaskcfg_file.aera[index].w);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_vdmask_h:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stVidMaskcfg_file.aera[index].h!=iValue){
					g_stVidMaskcfg_file.aera[index].h = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_VIDMASK_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_vdmask_h, g_stVidMaskcfg_file.aera[index].h);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_vdmask_color:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stVidMaskcfg_file.aera[index].color!=iValue){
					g_stVidMaskcfg_file.aera[index].color = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_VIDMASK_CFG_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_vdmask_color, g_stVidMaskcfg_file.aera[index].color);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_time_Zone:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stTimecfg_file.timeZone!=iValue){
					g_stTimecfg_file.timeZone = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_SYS_TIMER_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_time_Zone, g_stTimecfg_file.timeZone);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_time_dstmode:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stTimecfg_file.dstmode!=(char)iValue){
					g_stTimecfg_file.dstmode = (char)iValue;
					SET_FILE_CHANGED(file_changed_flag, e_SYS_TIMER_FILE);
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_time_dstmode, g_stTimecfg_file.dstmode);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_time_ntpenable:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stTimecfg_file.ntpenable!=iValue){
					g_stTimecfg_file.ntpenable = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_SYS_TIMER_FILE);
					ntpserver_cfg++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_time_ntpenable, 0/*g_stTimecfg_file.ntpenable*/);//永远告诉onvif ntp服务关闭
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_time_ntpserver:
			if(cmd_type==T_Set){
				if(strcmp(g_stTimecfg_file.ntpserver, pValue)!=0){
					strcpy(g_stTimecfg_file.ntpserver, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_SYS_TIMER_FILE);
					ntpserver_cfg++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_time_ntpserver, g_stTimecfg_file.ntpserver);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_time_ntpinterval:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				if(g_stTimecfg_file.ntpinterval!=iValue){
					g_stTimecfg_file.ntpinterval = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_SYS_TIMER_FILE);
					ntpserver_cfg++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_time_ntpinterval, g_stTimecfg_file.ntpinterval);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_time_systime:
			if(cmd_type==T_Set){
			//	char log[64];
				if(strcmp(g_stTimecfg_file.sys_time, pValue)!=0){

//					printf("============system: %s.\n", pValue);
					sprintf(cmd_tmp, "date -s \"%s\" -u", pValue);
			//		sprintf(log, "echo %s >> /mnt/mtd/time.log", cmd_tmp);
//					printf(log);
			//		system(log);
					system(cmd_tmp);

					kill_thttpd();//

					strcpy(g_stTimecfg_file.sys_time, pValue);
//					SET_FILE_CHANGED(file_changed_flag, e_SYS_TIMER_FILE);		//当前时间设置不写入flash中
				}
			}else if(cmd_type==T_Get){
				time_t t;
				t = time(&t);
				sprintf(cmd_tmp, "&%d=%s", e_time_systime, ctime(&t)/*g_stTimecfg_file.sys_time*/);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_devtype:
			if(cmd_type==T_Get){
				if(g_stAv_0_file.ubs[0].width==1920){
					g_stSyscfg_file.devtype = 1;
				}else{
					g_stSyscfg_file.devtype = 0;
				}
				sprintf(cmd_tmp, "&%d=%d", e_sys_devtype, g_stSyscfg_file.devtype);//0:720p 1:1080p
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_model:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_model, g_stSyscfg_file.model);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_hdversion:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_hdversion, g_stSyscfg_file.hardVersion);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_swversion:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_swversion, g_stSyscfg_file.softVersion);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_devname:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_devname, g_stSyscfg_file.name);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_startdate:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_startdate, g_stSyscfg_file.startdate);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_runtimes:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_runtimes, g_stSyscfg_file.runtimes);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_sdstatus:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_sys_sdstatus, g_stSyscfg_file.sdstatus);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_sdfreespace:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_sys_sdfreespace, g_stSyscfg_file.sdfreespace);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_sdtotalspace:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_sys_sdtotalspace, g_stSyscfg_file.sdtotalspace);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_hardwareId:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_hardwareId, "2012-04-25");
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_manufacturer:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_manufacturer, "BRVision");
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_Model:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_sys_model, g_stSyscfg_file.model);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_sys_serialNumber:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%02x-%02x-%02x-%02x-%02x-%02x", e_sys_serialNumber, 
								g_stNet_file.macaddr[0], g_stNet_file.macaddr[1], g_stNet_file.macaddr[2],
								g_stNet_file.macaddr[3], g_stNet_file.macaddr[4], g_stNet_file.macaddr[5]);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_nvr_opt:
			if(cmd_type==T_Set){
				iValue = atoi(pValue);
				set_nvr_send_status(channel, sub_channel, iValue);
			}
			break;
		case e_nvr_forIDR:
			if(cmd_type==T_Set){
				set_IDR_BR(channel, sub_channel);
			}
			break;
		case e_nvr_clientID:
			if(cmd_type==T_Get){
				iValue = atoi(pValue);
				sprintf(cmd_tmp, "&%d=%d", e_nvr_clientID, iValue);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_enc_resolution:
			if(cmd_type==T_Get){
				if(g_stAv_0_file.ubs[0].width==1920){
					iValue = 1;
				}else{
					if (g_stAv_1_file.ubs[0].width == 352)
						iValue = 0;
					else
						iValue = 2;
				}

				sprintf(cmd_tmp, "&%d=%d", e_enc_resolution, iValue);
				strcat(pRet, cmd_tmp);

//				printf("==============to get resolution: %d.\n", iValue);

				ret++;
			}else{
				iValue = atoi(pValue);
				if(iValue==1){
					//切换为1080
					if(g_stAv_0_file.ubs[0].width!=1920){
						if (g_stAv_1_file.ubs[0].width == 352) {
							system("/bin/sh /mnt/mtd/switch.sh 1 0");
						} else {
							system("/bin/sh /mnt/mtd/switch.sh 1 2");
						}
					}
				}else if (iValue == 2){
					//720 + 640
					if(g_stAv_1_file.ubs[0].width!=640){
						if (g_stAv_0_file.ubs[0].width == 1280) {
							system("/bin/sh /mnt/mtd/switch.sh 2 0");
						}
						else {
							system("/bin/sh /mnt/mtd/switch.sh 2 1");
						}
					}
				}else{
					//切换为720  720 + 352
					if(g_stAv_0_file.ubs[0].width!=1280){
						system("/bin/sh /mnt/mtd/switch.sh 0 1");
					} else {
						if (g_stAv_1_file.ubs[0].width == 640) {
							system("/bin/sh /mnt/mtd/switch.sh 0 2");
						}				
					}
				}
			}
			break;

		case e_reset:
			if(cmd_type==T_Set){
				//before reset, goto preset 95 to clear all presets on board
				sprintf(ptzcmd, "@%d@%d@%d\n", 0, GOTO_PRESET, 95);
				printf("goto %s\n",ptzcmd);
				mctp_cmd_cb(PTZ, ptzcmd);
				
				do_reset();
				reboot++;
			}
			break;

		case e_reboot:
			if(cmd_type==T_Set){
				reboot++;
			}
			break;

		case e_mctp_ptzstring:
			if(cmd_type==T_Set){
				mctp_cmd_cb(PTZ, pValue);
			}
			break;

		case e_ptz_continue_move_default_timeout:
			if(cmd_type==T_Set){

			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_continue_move_default_timeout, 10000);//default timeout 10000 ms
				strcat(pRet, cmd_tmp);
			}
//			fprintf(stderr, "e_ptz_continue_move_default_timeout: %s\n", pValue);
			break;

		case e_ptz_continue_move_vx:
			if(cmd_type==T_Set){
				fValue_vx= atof(pValue);
			}
			break;

		case e_ptz_continue_move_vy:
			if(cmd_type==T_Set){
				fValue_vy = atof(pValue);
			}			
			break;

		case e_ptz_continue_move_vzoom:
			if(cmd_type==T_Set){
				int flas_tmp = 0;
				if(pValue[0]=='-'){
					flas_tmp = 1;
				}
//				printf("flas_tmp = %d.\n", flas_tmp);
				iValue= (int)(64*atof(pValue));
				if(iValue<0||flas_tmp){
					sprintf(ptzcmd, "@%d@%d@%d\n", 0, Z_ZOOM_OUT, abs(iValue));
					mctp_cmd_cb(PTZ, ptzcmd);
#if DH_SERVER
				}else if(iValue>0){
#else
				}else if(iValue>=0){
#endif
					sprintf(ptzcmd, "@%d@%d@%d\n", 0, Z_ZOOM_IN, iValue);
					mctp_cmd_cb(PTZ, ptzcmd);
				}
			}			
			break;

		case e_ptz_continue_move_timeout:
			fprintf(stderr, "e_ptz_continue_move_default_timeout: %s\n", pValue);
			break;

		case e_ptz_stop_pt:
			if(cmd_type==T_Set){
				sprintf(ptzcmd, "@%d@%d@%d\n", 0, PTZ_STOP, 0);
				mctp_cmd_cb(PTZ, ptzcmd);
			}						
			break;

		case e_ptz_stop_zoom:
			if(cmd_type==T_Set){
				sprintf(ptzcmd, "@%d@%d@%d\n", 0, PTZ_STOP, 0);
				mctp_cmd_cb(PTZ, ptzcmd);
			}
			break;

		case e_ptz_goto_preset:
			if(cmd_type==T_Set){
				cValue=strtol(pValue,NULL,10);
				if((cValue>0)&&(cValue<PTZ_PRESET_ALLNUM+1)){
					if(g_stPtzpreset_file.preset[cValue-1].isSet){
						if(cValue==95){
							//95号预置位，初始化PTZ预置位
							for(i=0; i<PTZ_PRESET_ALLNUM; i++){
								g_stPtzpreset_file.preset[i].isSet = 0;
								g_stPtzpreset_file.preset[i].name[0] = '\0';
							}
							SET_FILE_CHANGED(file_changed_flag, e_PTZ_PRESET_FILE);
						}
						sprintf(ptzcmd, "@%d@%d@%d\n", 0, GOTO_PRESET, cValue);
						printf("goto %s\n",ptzcmd);
						mctp_cmd_cb(PTZ, ptzcmd);

						sprintf(cmd_tmp, "&%d=%d&%d=%d", e_ptz_goto_preset, cValue, e_error, 0);
						strcat(pRet, cmd_tmp);
						ret++;
					}else{
						sprintf(cmd_tmp, "&%d=%d&%d=%d", e_ptz_goto_preset, cValue, e_error, ERROR_PTZ_INDEX_NOT_EXIST);
						strcat(pRet, cmd_tmp);
						ret++;
					}
				}else{
					sprintf(cmd_tmp, "&%d=%d&%d=%d", e_ptz_goto_preset, cValue, e_error, ERROR_PTZ_INDEX_OUT_RANGE);
					strcat(pRet, cmd_tmp);
					ret++;
				}
			}
			break;

		case e_ptz_preset:
			ptzpreset_index=strtol(pValue, NULL, 10);
			ptz_preset_in++;

			break;

		case e_ptz_presetname:
			strncpy(ptz_preset_name, pValue, sizeof(ptz_preset_name));
			ptz_preset_in++;
			break;

		case e_ptz_deletepreset:
			if(cmd_type==T_Set){
				cValue=strtol(pValue,NULL,10);
				if(cValue <= 0) break;
				if(cValue > PTZ_PRESET_ALLNUM) break;
				if((cValue > 0)&&(!g_stPtzpreset_file.preset[cValue-1].isSet)) break;

				g_stPtzpreset_file.preset[cValue-1].isSet = 0;
				g_stPtzpreset_file.preset[cValue-1].name[0] = '\0';
				SET_FILE_CHANGED(file_changed_flag, e_PTZ_PRESET_FILE);

				sprintf(ptzcmd, "@%d@%d@%d\n", 0, CLE_PRESET, cValue);
				printf("clear %s\n",ptzcmd);
				mctp_cmd_cb(PTZ, ptzcmd);
			}
			break;

		case e_ptz_allpresets:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=", e_ptz_allpresets);
				strcat(pRet, cmd_tmp);
				ret++;
				for(i=0; i<PTZ_PRESET_ALLNUM; i++){
					if(g_stPtzpreset_file.preset[i].isSet){
						sprintf(cmd_tmp, "%x/", i+1);
						strcat(pRet, cmd_tmp);
					}
				}
			}
			break;

		case e_ptz_presets_capacity:
			if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_ptz_presets_capacity, PTZ_PRESET_ALLNUM);
				strcat(pRet, cmd_tmp);
				ret++;
			}									
			break;

		case e_sys_protocol:
			iValue = atoi(pValue);
			if(cmd_type==T_Set){
				if(iValue!=get_curProtocol()){
					set_Protocol(iValue);
					reboot++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_sys_protocol, get_curProtocol());
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;

		case e_snapshot_start:
			iValue = atoi(pValue);
//printf("value:%d\n",iValue);
			snap_start = iValue;
			break;
		case e_cld_enable:
			iValue = atoi(pValue);
			if(cmd_type==T_Set){
				if (g_stCloud_cfg_file.enable != (iValue > 0)) {
					g_stCloud_cfg_file.enable = (iValue > 0);
					SET_FILE_CHANGED(file_changed_flag, e_CLOUD_CFG_FILE);
					reboot++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_cld_enable, g_stCloud_cfg_file.enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_cld_id:
			if(cmd_type==T_Set){
				if (strcmp(g_stCloud_cfg_file.id, pValue) != 0) {
					strcpy(g_stCloud_cfg_file.id, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_CLOUD_CFG_FILE);
					reboot++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_cld_id, g_stCloud_cfg_file.id);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;

		case e_cld_pw:
			if(cmd_type==T_Set){
				if (strcmp(g_stCloud_cfg_file.pw, pValue) != 0) {
					strcpy(g_stCloud_cfg_file.pw, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_CLOUD_CFG_FILE);
					reboot++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_cld_pw, g_stCloud_cfg_file.pw);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		case e_cld_srv_prt:
			iValue = atoi(pValue);
			if(cmd_type==T_Set){
				if(iValue != g_stCloud_cfg_file.port){
					g_stCloud_cfg_file.port = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_CLOUD_CFG_FILE);
					reboot++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%d", e_cld_srv_prt, g_stCloud_cfg_file.port);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;

		case e_cld_srv_addr:
			if(cmd_type==T_Set){
				if (strcmp(g_stCloud_cfg_file.addr, pValue) != 0) {
					strcpy(g_stCloud_cfg_file.addr, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_CLOUD_CFG_FILE);
					reboot++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_cld_srv_addr, g_stCloud_cfg_file.addr);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;
		
		case e_cld_dev_name:
			if(cmd_type==T_Set){
				if (strcmp(g_stCloud_cfg_file.name, pValue) != 0) {
					strcpy(g_stCloud_cfg_file.name, pValue);
					SET_FILE_CHANGED(file_changed_flag, e_CLOUD_CFG_FILE);
					reboot++;
				}
			}else if(cmd_type==T_Get){
				sprintf(cmd_tmp, "&%d=%s", e_cld_dev_name, g_stCloud_cfg_file.name);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;

		case e_rec_enable:
			if (cmd_type == T_Set) {
				iValue = atoi(pValue);
				if (g_stRec_cfg_file.enable != iValue) {
					g_stRec_cfg_file.enable = !!(iValue);
					SET_FILE_CHANGED(file_changed_flag, e_REC_CFG_FILE);
				}
			} else {
				sprintf(cmd_tmp, "&%d=%d", e_rec_enable, g_stRec_cfg_file.enable);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;

		case e_rec_len:
			if (cmd_type == T_Set) {
				iValue = atoi(pValue);
				if (g_stRec_cfg_file.time_len != iValue) {
					g_stRec_cfg_file.time_len = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_REC_CFG_FILE);
				}
			} else {
				sprintf(cmd_tmp, "&%d=%d", e_rec_len, g_stRec_cfg_file.time_len);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;

		case e_rec_time:
			if (cmd_type == T_Set) {
				iValue = atoi(pValue);
				if (g_stRec_cfg_file.sch != iValue) {
					g_stRec_cfg_file.sch = iValue;
					SET_FILE_CHANGED(file_changed_flag, e_REC_CFG_FILE);
				}
			} else {
				sprintf(cmd_tmp, "&%d=%d", e_rec_len, g_stRec_cfg_file.sch);
				strcat(pRet, cmd_tmp);
				ret++;
			}
			break;

		default:
			break;
		}

	} while(parseIndex!=-1);

	if(ptz_preset_in){
		deal_ptz_preset(cmd_type, ptzpreset_index, ptz_preset_name, pRet, &file_changed_flag);
		ret++;
	}

	strcat(pRet, "#");

	if ((fabs(fValue_vx) > DELT)
		||(fabs(fValue_vy) > DELT)) {
		set_ptz(fValue_vx, fValue_vy, ptzcmd);
		mctp_cmd_cb(PTZ, ptzcmd);
	}

	if (if_cfg)
		gen_if_cfg(g_stNet_file.ip, g_stNet_file.macaddr);

	if (ntpserver_cfg)
		start_ntp();

	if (IS_SET_FILE_CHANGED(file_changed_flag, e_REC_CFG_FILE))
		rec_cfg_set();

	if (osd_cfg) {
		fd_socket = connect_local(UN_CAPOSD_DOMAIN);
		if (fd_socket >= 0) {
			Pack_Msg Msg;
			Msg.head.emSrc = mine_server;
			Msg.head.emData_type = osd_cfg_data;
			Msg.head.data_len = sizeof(Osd_cfg_t);
			Msg.head.eCmd = cmd_overlay;
			g_arrstOsd_file[osd_region].region = osd_region;
			memcpy(&Msg.Pack_data.osd_data, &(g_arrstOsd_file[osd_region]), sizeof(Osd_cfg_t));
			send_len = send_local(fd_socket, (char *)&Msg, sizeof(Msg));
			if (send_len!=0)
				printf("%s: send osd cmd error.\n", __FUNCTION__);

			close_local(fd_socket);
		}
	}

	for (i=e_ENC_0_FILE; i<=e_END_FILE; i++) {
		if (IS_SET_FILE_CHANGED(file_changed_flag, i)) {
			write_cfg_file(File_array[i].filename, File_array[i].buf, File_array[i].buf_len);
			CLR_FILE_CHANGED(file_changed_flag, i);
		}
	}

	if (fd_socket >= 0)
		close_local(fd_socket);	

	if (cmd_tmp!=NULL)
		free(cmd_tmp);

	if(reboot){
		usleep(100);
		system("/sbin/reboot");
	}

	return ret;
}



