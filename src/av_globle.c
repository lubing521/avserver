

#include "av_globle.h"

void init_gbl(int status, int imgCtoW_th, int imgWtoC_th, int saturation)
{
	pthread_mutex_init(&g_stInfrad_ctl.mutex, NULL);

	g_stInfrad_ctl.imgCtoW_th = imgCtoW_th;
	g_stInfrad_ctl.imgWtoC_th = imgWtoC_th;
	g_stInfrad_ctl.status = status;
	g_stInfrad_ctl.img_saturation = saturation;

	return;
}

int getImgCtoW()
{
	int ret;

	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	ret = g_stInfrad_ctl.imgCtoW_th;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);

	return ret;
}

int getImgWtoC()
{
	int ret;

	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	ret = g_stInfrad_ctl.imgWtoC_th;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);

	return ret;
}

int getInfradStatue()
{
	int ret;

	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	ret = g_stInfrad_ctl.status;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);

	return ret;
}

int getImg_saturation()
{
	int ret;

	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	ret = g_stInfrad_ctl.img_saturation;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);

	return ret;
}

void set_imgCtoW(int imgCtoW)
{
	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	g_stInfrad_ctl.imgCtoW_th = imgCtoW;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);
}

void set_imgWtoC(int imgWtoC)
{
	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	g_stInfrad_ctl.imgWtoC_th = imgWtoC;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);
}


void set_infradstatus(int status)
{
	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	g_stInfrad_ctl.status = status;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);
}

void set_Img_saturation(int saturation)
{
	pthread_mutex_lock(&g_stInfrad_ctl.mutex);
	g_stInfrad_ctl.img_saturation = saturation;
	pthread_mutex_unlock(&g_stInfrad_ctl.mutex);
}
