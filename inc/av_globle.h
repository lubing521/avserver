
#ifndef _AV_GBL_H
#define _AV_GBL_H

#include <pthread.h>



typedef struct _tagInfrad_ctrl{
	pthread_mutex_t		mutex;
	int					status;
	int					imgCtoW_th;
	int					imgWtoC_th;
	int					img_saturation;
}stInfrad_ctl;

stInfrad_ctl g_stInfrad_ctl;

void init_gbl(int status, int imgCtoW_th, int imgWtoC_th, int saturation);
int getImgCtoW();
int getImgWtoC();
int getInfradStatue();
void set_imgCtoW(int imgCtoW);
void set_imgWtoC(int imgWtoC);
void set_infradstatus(int status);
int getImg_saturation();
void set_Img_saturation(int saturation);

#endif
