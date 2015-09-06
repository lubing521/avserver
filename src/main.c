/*
*2012.02.19 	by Aaron
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dvr_common_api.h"
#include "dvr_disp_api.h"
#include "file_cfg.h"

#include "ipnc.h"

//#include "stream.h"

//#include "wis-streamer.hh"
//#include "mt.h"
#include "rtspd.h"
int main(int argc, char *argv[])
{
	init_system();

	init_drv_common();

	init_isp();

	#if DEMO_DEF	

	printf("open infrad thread....??????\n");
	init_infrad();
	#endif

	rtspd_start();

	boot_other();
	Avi_start();	

	av_server();

	return 0;
}






