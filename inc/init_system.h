

#ifndef _INIT_SYSTEM_H
#define _INIT_SYSTEM_H

#include "ipnc.h"

#define SYS_PROTOCOL 1	//0:client 1:onvif

extern int g_iFw;
extern Av_cfg_t g_stAv_0_file;
extern Av_cfg_t g_stAv_1_file;
extern Img_cfg_t g_stImg_file;
extern Osd_cfg_t g_arrstOsd_file[REGION_NUM];
extern Infrared_cfg_t g_stInfrad_file;
extern Net_cfg_t g_stNet_file;
extern Port_cfg_t g_stPort_file;
extern Upnp_cfg_t g_stUpnp_file;
extern DDNS_cfg_t g_stDDNS_file;
extern Wf_search g_stWf_search_file;
extern Wf_cfg_t g_stwfcfg_file;
extern Ptz_cfg_t g_stPtzcfg_file;
extern Ptz_ctrl_t g_stPtzctl_file;
extern Md_cfg_t g_stMdcfg_file;
extern Snaptimer_cfg_t g_stSnaptimercfg_file;
extern User_file_t g_stUsr_file;
extern Ft_cfg_t g_stFtcfg_file;
extern Smtp_cfg_t g_stSmtpcfg_file;
extern VidMask_cfg_t g_stVidMaskcfg_file;
extern Time_cfg_t g_stTimecfg_file;
extern Sys_cfg_t g_stSyscfg_file;
extern Ptz_preset_t g_stPtzpreset_file;
extern Cloud_cfg_t g_stCloud_cfg_file;
extern Rec_cfg_t g_stRec_cfg_file;


void init_system();

#endif
