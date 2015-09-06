
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ipnc.h"
#include "interface.h"



int g_iFw;
Av_cfg_t g_stAv_0_file;
Av_cfg_t g_stAv_1_file;
Img_cfg_t g_stImg_file;
Osd_cfg_t g_arrstOsd_file[REGION_NUM];
Infrared_cfg_t g_stInfrad_file;
Net_cfg_t g_stNet_file;
Port_cfg_t g_stPort_file;
Upnp_cfg_t g_stUpnp_file;
DDNS_cfg_t g_stDDNS_file;
Wf_search g_stWf_search_file;
Wf_cfg_t g_stwfcfg_file;
Ptz_cfg_t g_stPtzcfg_file;
Ptz_ctrl_t g_stPtzctl_file;
Md_cfg_t g_stMdcfg_file;
Snaptimer_cfg_t g_stSnaptimercfg_file;
User_file_t g_stUsr_file;
Ft_cfg_t g_stFtcfg_file;
Smtp_cfg_t g_stSmtpcfg_file;
VidMask_cfg_t g_stVidMaskcfg_file;
Time_cfg_t g_stTimecfg_file;
Sys_cfg_t g_stSyscfg_file;
Ptz_preset_t g_stPtzpreset_file;
Cloud_cfg_t g_stCloud_cfg_file;
Rec_cfg_t g_stRec_cfg_file;

#define DBG_ERR_INIT(x) printf("%s: %s failed.\n", __FUNCTION__, x)

struct _tagInit_table{
	char *filename;
	void *buf;
	int len;
};

struct _tagInit_table Init_table[] = {
	{ENC_FW_FILE, (void *)&g_iFw, sizeof(g_iFw)},
//	{ENC_FILE(0), (void *)&g_stAv_0_file, sizeof(g_stAv_0_file)},
//	{ENC_FILE(1), (void *)&g_stAv_1_file, sizeof(g_stAv_1_file)},
	{IMAGE_ATTR_FILE, (void *)&g_stImg_file, sizeof(g_stImg_file)},
	{OVERLAY_CFG_FILE, (void *)&g_arrstOsd_file[0], sizeof(g_arrstOsd_file)},
	{INFRARED_CFG_FILE, (void *)&g_stInfrad_file, sizeof(g_stInfrad_file)},
	{NET_CFG_FILE, (void *)&g_stNet_file, sizeof(g_stNet_file)},
	{PORT_CFG_FILE, (void *)&g_stPort_file, sizeof(g_stPort_file)},
	{UPNP_CFG_FILE, (void *)&g_stUpnp_file, sizeof(g_stUpnp_file)},
	{WF_CFG_FILE, (void *)&g_stwfcfg_file, sizeof(g_stwfcfg_file)},
	{DDNS_CFG_FILE, (void *)&g_stDDNS_file, sizeof(g_stDDNS_file)},
	{PTZCOM_CFG_FILE, (void *)&g_stPtzcfg_file, sizeof(g_stPtzcfg_file)},
	{MD_CFG_FILE, (void *)&g_stMdcfg_file, sizeof(g_stMdcfg_file)},
	{USER_FILE, (void *)&g_stUsr_file, sizeof(g_stUsr_file)},
	{SNAPTIMER_CFG_FILE, (void *)&g_stSnaptimercfg_file, sizeof(g_stSnaptimercfg_file)},
	{FTP_CFG_FILE, (void *)&g_stFtcfg_file, sizeof(g_stFtcfg_file)},
	{SMTP_CFG_FILE, (void *)&g_stSmtpcfg_file, sizeof(g_stSmtpcfg_file)},
	{VIDMASK_CFG_FILE, (void *)&g_stVidMaskcfg_file, sizeof(g_stVidMaskcfg_file)},
	{SYS_CFG_FILE, (void *)&g_stSyscfg_file, sizeof(g_stSyscfg_file)},
	{PTZ_PRESET_CFG_FILE, (void *)&g_stPtzpreset_file, sizeof(g_stPtzpreset_file)},
	{SYS_TIMER_FILE, (void *)&g_stTimecfg_file, sizeof(g_stTimecfg_file)},
	{CLOUD_CFG_FILE, (void *)&g_stCloud_cfg_file, sizeof(g_stCloud_cfg_file)},
	{REC_CFG_FILE, (void *)&g_stRec_cfg_file, sizeof(g_stRec_cfg_file)},
	{NULL, NULL, 0}
};

#define NET_DEV_NAME		"eth0"
void init_system()
{
	int ret;
	int i;
	FILE *fp;
	char version[16];

	char cmd_tmp[128];
	char *ipaddr;
	int fd_socket = -1;

	libipc_init();

	i=0;
	while(Init_table[i].filename!=NULL){
		ret = read_cfg_file(Init_table[i].filename, Init_table[i].buf, Init_table[i].len);
		if(ret!=READ_CFG_FILE_OK){
			DBG_ERR_INIT(Init_table[i].filename);
			exit(0);
		}
		i++;
	}

	i = 0;
	for(i=0;i<55;i++)
	{
		g_stPtzpreset_file.preset[i].index = i+1;
		g_stPtzpreset_file.preset[i].isSet = 1;
//		sprintf(g_stPtzpreset_file.preset[i].name, "%d", i+1);
	}


	memset(version, 0, 16);
	if((fp=fopen(VERSION_FILE, "r"))!=NULL){
		fread(version, 1, 16, fp);
		fclose(fp);
		memcpy(g_stSyscfg_file.softVersion, version, 16);
	}

	if(g_stNet_file.dhcpflag){
		printf("....111.\n");
		net_enable_dhcpcd();
	}else{
		net_disable_dhcpcd();

		ipaddr = inet_ntoa(net_get_ifaddr(NET_DEV_NAME));
		memset(g_stNet_file.ip, 0, sizeof(g_stNet_file.ip));
		memcpy(g_stNet_file.ip, ipaddr, strlen(ipaddr));

		if(!ipOk(g_stNet_file.ip)){
			ipaddr = inet_ntoa(net_get_ifaddr(NET_DEV_NAME));
			memset(g_stNet_file.ip, 0, sizeof(g_stNet_file.ip));
			memcpy(g_stNet_file.ip, ipaddr, strlen(ipaddr));	
			if(!ipOk(g_stNet_file.ip))	{
				ipaddr = inet_ntoa(net_get_ifaddr(NET_DEV_NAME));
				memset(g_stNet_file.ip, 0, sizeof(g_stNet_file.ip));
				memcpy(g_stNet_file.ip, ipaddr, strlen(ipaddr));	
				if(!ipOk(g_stNet_file.ip)){
//					net_set_ifaddr(NET_DEV_NAME, inet_addr("192.168.1.105"));
//					memcpy(g_stNet_file.ip, "192.168.1.105", sizeof("192.168.1.105"));
						system("echo \"IP read error.\" >> /mnt/nfs/system.log");
						system("/sbin/reboot");
				}
			}
		}

		net_set_netmask(NET_DEV_NAME, inet_addr(g_stNet_file.netmask));
		printf("set netmask: %s.\n", g_stNet_file.netmask);
		net_set_gateway(inet_addr(g_stNet_file.gateway));
		net_set_dns(g_stNet_file.fdnsip);
/*
		sprintf(cmd_tmp, "%02x:%02x:%02x:%02x:%02x:%02x", 
					g_stNet_file.macaddr[0], g_stNet_file.macaddr[1], g_stNet_file.macaddr[2],
					g_stNet_file.macaddr[3], g_stNet_file.macaddr[4], g_stNet_file.macaddr[5]);
		net_set_hwaddr(NET_DEV_NAME, cmd_tmp);
*/
		net_get_hwaddr(NET_DEV_NAME, g_stNet_file.macaddr);
	}

	if(g_stTimecfg_file.ntpenable){
		sprintf(cmd_tmp, "/mnt/mtd/ntpclient -h %s -s -c 1 &", g_stTimecfg_file.ntpserver);
		system(cmd_tmp);
		sleep(3);

		sprintf(cmd_tmp, "/mnt/mtd/ntpclient -h %s -s -c 1 &", g_stTimecfg_file.ntpserver);
		system(cmd_tmp);
		sleep(3);

		sprintf(cmd_tmp, "/mnt/mtd/ntpclient -h %s -s -i %d -c 0 &", g_stTimecfg_file.ntpserver, g_stTimecfg_file.ntpinterval*60);
		system(cmd_tmp);
	}

        fd_socket = connect_local(UN_CAPOSD_DOMAIN);
        if(fd_socket >= 0){
                Pack_Msg Msg;
                Msg.head.emSrc = mine_server;
                Msg.head.emData_type = osd_cfg_data;
                Msg.head.data_len = sizeof(Osd_cfg_t);
                Msg.head.eCmd = cmd_overlay;
                memcpy(&Msg.Pack_data.osd_data, &(g_arrstOsd_file[0]), sizeof(Osd_cfg_t));
                if(send_local(fd_socket, (char *)&Msg, sizeof(Msg))!=0){
                        printf("%s: send osd cmd error.\n", __FUNCTION__);
                }
                close_local(fd_socket);
        }

	system("date -s \"2015-04-30 12:00:00\"");

	return;
}


struct _tagInit_table reset_table[] = {
	{ENC_FILE(0), (void *)&g_stAv_0_file, sizeof(g_stAv_0_file)},
	{ENC_FILE(1), (void *)&g_stAv_1_file, sizeof(g_stAv_1_file)},
	{OVERLAY_CFG_FILE, (void *)&g_arrstOsd_file[0], sizeof(g_arrstOsd_file)},
	{PTZ_PRESET_CFG_FILE, (void *)&g_stPtzpreset_file, sizeof(g_stPtzpreset_file)},
	{CLOUD_CFG_FILE, (void *)&g_stCloud_cfg_file, sizeof(g_stCloud_cfg_file)},
	{NULL, NULL, 0}
};

void do_reset()
{
	int ret;
	int i;

	i = 0;
	while(reset_table[i].filename!=NULL){
		default_setting(reset_table[i].filename);

		ret = read_cfg_file(reset_table[i].filename, reset_table[i].buf, reset_table[i].len);
		if(ret!=READ_CFG_FILE_OK){
			DBG_ERR_INIT(reset_table[i].filename);
			continue;
		}

		i++;
	}
	
	return;
}




