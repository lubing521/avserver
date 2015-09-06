/*
 *avi_save.c
*/


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/dir.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "sd_config.h"

#include "cmd_type.h"
#include "mt.h"
#include "interface.h"

#include "gmavi.h"
#include "gmavi_api.h"
#include "gmtypes.h"
#include "avi_save.h"
/*
#define ERR(...)    \
    do{                 \
    printf("error: %s.%s.%d:", __FILE__, __FUNCTION__, __LINE__);   \
    printf(__VA_ARGS__);    \
    }while(0)
*/

#define MAX_REC_TIME_LEN	5
#define MIN_REC_TIME_LEN	1

static int g_rec_enable = 0;
static int g_rec_schdule = 0;
static int g_rec_status = 0; //0:release  1:busy
static int g_rec_TimeL = 5 * 60; //record time length, default 15s and max 900s, add 10s for each start sigle

static pthread_t m_Avi_id;
static int m_Quit = 0;

#define RESERVE_SPACE   16*1024 //16M
int prepare_free_space(char *path, unsigned int space)
{
    long long int free = 0;
    char line[128], file[128];
    char old_dir[128];
    int ret = 0;

    free = GetCardfreeSpace(path);
    printf("free %d, need %d\n", free, space);
    while(free < space){
	  ret = find_oldest_dir_from_sdroot(old_dir);	
	  if (ret)	
	      del_dir(old_dir);
	  else
	    break;

        free = GetCardfreeSpace(path);
    }

    if(free < space){
        return -1;
    }

    return 0;
}

struct _tagAvi_stream_status{
    int streamFlag;  //0:no new stream 1:new stream
    int SerialBook;
    int SerialLock;
    int check; //how long to check, -1: do not check
}AVI_stream_status;
#define AVI_TO_CHECK    3

int av_field[2][7] = {

	{AV_OP_LOCK_MP4_VOL,
	AV_OP_UNLOCK_MP4_VOL,
	AV_OP_LOCK_MP4,
	AV_OP_LOCK_MP4_IFRAME,
	AV_OP_UNLOCK_MP4,
	AV_OP_GET_MPEG4_SERIAL,
	AV_OP_WAIT_NEW_MPEG4_SERIAL},

	{AV_OP_LOCK_MP4_CIF_VOL,
	AV_OP_UNLOCK_MP4_CIF_VOL,
	AV_OP_LOCK_MP4_CIF,
	AV_OP_LOCK_MP4_CIF_IFRAME,
	AV_OP_UNLOCK_MP4_CIF,
	AV_OP_GET_MPEG4_CIF_SERIAL,
	AV_OP_WAIT_NEW_MPEG4_CIF_SERIAL},
};

#define AV_LOCK_MP4_VOL	0
#define AV_UNLOCK_MP4_VOL 1
#define AV_LOCK_MP4		2
#define AV_LOCK_MP4_IFRAME	3
#define AV_UNLOCK_MP4	4
#define AV_GET_MPEG4_SERIAL 5
#define AV_WAIT_NEW_MPEG4_SERIAL 6


static int GetVideoInfo(int channel, int *w, int *h, int *fps, int *bps)
{
    int ret = 0;
    int fd;
    char cmd[MAX_CMD_LEN];
    char tmp[64];

	char *pKey;
	int iKey;
	char *pValue;
	char *pInput;
	int parseIndex = 0;

    fd = connect_local(UN_AVSERVER_DOMAIN);
    if (fd < 0)
        return -1;

    sprintf(cmd, "$%d=%d", e_TYPE, T_Get);
    sprintf(tmp, "&%d=%d", e_Chn, channel);
    strcat(cmd, tmp);
    sprintf(tmp, "&%d=%d", e_Sub_Chn, 0);
    strcat(cmd, tmp);
    sprintf(tmp, "&%d=%d", e_width, 0);
    strcat(cmd, tmp);
    sprintf(tmp, "&%d=%d", e_height, 0);
    strcat(cmd, tmp);
    sprintf(tmp, "&%d=%d", e_frame_rate, 0);
    strcat(cmd, tmp);
    sprintf(tmp, "&%d=%d", e_bit_rate, 0);
    strcat(cmd, tmp);
    strcat(cmd, "#");

    send_local(fd, cmd, MAX_CMD_LEN);

    ret = recv_local(fd, cmd, MAX_CMD_LEN);
    if(ret<=0){
        usleep(200000);
        ret = recv_local(fd, cmd, MAX_CMD_LEN);
        if(ret<=0){
            return -2;
        }
    }

    //$e_TYPE=0&e_Chn=0&e_Sub_Chn=0&e_width=?&e_height=?&e_frame_rate=?&e_bit_rate=?#
//    sscanf(cmd, "$%*d=%*d&%*d=%*d&%*d=%*d&%*d=%d&%*d=%d&%*d=%d&%*d=%d#", w, h, fps, bps);

	pInput = cmd;
	do{
		pKey = ParseVars(pInput, &parseIndex);
		pValue = ParseVars(pInput, &parseIndex);

		if(pKey == NULL || pValue == NULL)
			break;

		printf("pKey = %s, pValue = %s\n", pKey, pValue);
		iKey = atoi(pKey);
		switch (iKey) {
		case e_width:
			*w = atoi(pValue);
			break;
		case e_height:
			*h = atoi(pValue);
			break;
		case e_frame_rate:
			*fps = atoi(pValue);
			break;
		case e_bit_rate:
			*bps = atoi(pValue);
			break;
		default:
			break;
		}
	} while (parseIndex != -1);

    return 0;
}

static int GenerateAviFileName(char *strFileName)
{
    time_t tCurrentTime;
    struct tm *tmnow;
	char *pre_dir;

    time(&tCurrentTime);
    tmnow = localtime(&tCurrentTime);
	pre_dir = mkdir_t(2);
	if (pre_dir == NULL)
		return -1;

    sprintf(strFileName, "%s/%04d%02d%02d%02d%02d%02d.avi", pre_dir,
                   tmnow->tm_year+1900, tmnow->tm_mon+1, tmnow->tm_mday, tmnow->tm_hour,
                   tmnow->tm_min, tmnow->tm_sec);

	free(pre_dir);

	return 0;
}

static int save_schedule(int schd)
{
	time_t tCurrentTime;
	struct tm *tmnow;

	time(&tCurrentTime);
	tmnow = localtime(&tCurrentTime);

	return (schd & (0x1 << tmnow->tm_hour));
}

void *Avi_save(void *ptr)
{
    int ret = 0;
    int status = 0;
    char avi_file_name[128];

    int channel = 0;
    int width = 1280;
    int height = 720;
    int framerate = 30;
    int bitsrate = 3*1024*1024;
	unsigned int need_free = 0;
    int totelFrames = 0;
    int rec_frames = 0;
    int avi_type;
    int avi_str_id;

    HANDLE avi_file;
    AviMainHeader main_header;
    AviStreamHeader stream_header;
    GmAviStreamFormat stream_format;
    int avi_finish = 0;

    AV_DATA av_data;
    int *cmd;
	char *sd_path = get_sdpath();

    if(!GetSdStatus()){
        status = CheckSDCard();
        if(status<0){
            printf("Check SD Card fail.\n");
            goto avi_save_exit_2;
        }
    }
	g_rec_status = 1;

	while (g_rec_enable) {
		if (!save_schedule(g_rec_schdule)) {
			sleep(60);
			continue;
		}

		ret = GetVideoInfo(channel, &width, &height, &framerate, &bitsrate);
		if(ret<0){
			printf("Avi_save: GetVideoInfo failed[%d], use default.\n", ret);
		}

		if (framerate < 8)
			framerate = 9;
		else if (framerate > 30)
			framerate = 25;

		if (bitsrate < 512)
			bitsrate = 512;
		else if (bitsrate > 4000)
			bitsrate = 4000;

		printf("g_rec_TimeL = %d, bitsrate = %d, framerate = %d\n", g_rec_TimeL, bitsrate, framerate);
		need_free = g_rec_TimeL * bitsrate * 2 / 8;//(KB)
		printf("need = %d\n", need_free);
		if(prepare_free_space(sd_path, need_free) < 0){
			printf("no enough space for avi file in sdcard.\n");
			goto avi_save_exit_2;
		}

		GenerateAviFileName(avi_file_name);
		printf("save avi file: %s\n", avi_file_name);
		avi_file = GMAVIOpen(avi_file_name, GMAVI_FILEMODE_CREATE, 0);
		if (avi_file == NULL) {
			printf("Avi_save: GMAVIOpen failed.\n");
			goto avi_save_exit_2;
		}

		totelFrames = g_rec_TimeL * framerate;
		ret = GMAVIFillAviMainHeaderValues(&main_header,
										   width,
										   height,
										   framerate,
										   bitsrate,
										   totelFrames);
		if(ret<0){
			printf("Avi_save: GMAVIFillAviMainHeaderValues failed.\n");
			goto avi_save_exit_1;
		}

		avi_type = GMAVI_TYPE_H264;
		ret = GMAVIFillVideoStreamHeaderValues(&stream_header,
											   &stream_format,
											   avi_type,
											   width,
											   height,
											   framerate,
											   bitsrate,
											   totelFrames);
		if(ret<0){
			printf("Avi_save: GMAVIFillVideoStreamHeaderValues failed.\n");
			goto avi_save_exit_1;
		}

		ret = GMAVISetAviMainHeader(avi_file, &main_header);
		if(ret<0){
			printf("Avi_save: GMAVISetAviMainHeader failed.\n");
			goto avi_save_exit_1;
		}

		ret = GMAVISetStreamHeader(avi_file, &stream_header, &stream_format, &avi_str_id);
		if(ret<0){
			printf("Avi_save: GMAVISetStreamHeader failed.\n");
			goto avi_save_exit_1;
		}

		cmd = av_field[0];
		AVI_stream_status.streamFlag = 1;
		AVI_stream_status.SerialBook = -1;
		AVI_stream_status.SerialLock = 0;
		AVI_stream_status.check = AVI_TO_CHECK;
		while(!m_Quit){
			if(AVI_stream_status.streamFlag){
				ret = mt_get(cmd[AV_LOCK_MP4_IFRAME], -1, &av_data);
				AVI_stream_status.SerialBook = av_data.serial;
				AVI_stream_status.streamFlag = 0;
			}else{
				ret = mt_get(cmd[AV_LOCK_MP4], AVI_stream_status.SerialBook, &av_data);
				if(AVI_stream_status.SerialBook>av_data.serial){
					usleep(5000);
					continue;
				}
				AVI_stream_status.SerialBook = av_data.serial;
				if(av_data.ptr==NULL){
					usleep(5000);
					continue;
				}
			}

			if((ret==RET_SUCCESS)&&(AVI_stream_status.check>=0)){
				if ((av_data.flags == AV_FLAGS_MP4_I_FRAME) &&
						(AVI_stream_status.check == 0)){
					int serial_now;
					mt_get(cmd[AV_GET_MPEG4_SERIAL], -1, &av_data );
					serial_now = av_data.serial;
					AVI_stream_status.check = AVI_TO_CHECK;
					if( serial_now - AVI_stream_status.SerialBook > 25 ){
						mt_get(cmd[AV_UNLOCK_MP4], AVI_stream_status.SerialBook, &av_data);
						AVI_stream_status.streamFlag = 1;
						usleep(5000);
						continue;
					}
				} else {
					AVI_stream_status.check--;
				}
			}

			if(ret==RET_SUCCESS){
				GMAVISetStreamDataAndIndex(avi_file,
										   avi_str_id,
										   av_data.ptr,
										   av_data.size,
										   (av_data.flags==AV_FLAGS_MP4_I_FRAME),
										   NULL,
										   0);
				rec_frames++;
				if(AVI_stream_status.SerialLock>0){
					mt_get(cmd[AV_UNLOCK_MP4], AVI_stream_status.SerialLock, &av_data);
				}
				AVI_stream_status.SerialLock = AVI_stream_status.SerialBook;

				AVI_stream_status.SerialBook++;
			}else{
				AVI_stream_status.streamFlag = 1;
				AVI_stream_status.SerialBook = -1;
				AVI_stream_status.check = AVI_TO_CHECK;
				//printf("sleep 3..........\n");
				usleep(5000);
			}

			if(rec_frames >= totelFrames){
				char idx_file[84];

				GMAVIClose(avi_file);
				sprintf(idx_file, "%s_idx", avi_file_name);
				unlink(idx_file);
				avi_finish = 1;

				break;
			}
		}

		if(avi_finish != 1){
			char idx_file[128];

			GMAVIClose(avi_file);
			sprintf(idx_file, "%s_idx", avi_file_name);
			unlink(idx_file);
		}
	}

    g_rec_status = 0;
    return NULL;

avi_save_exit_1:
    fclose(((GmAviFile *)avi_file)->file);
    unlink(avi_file_name);
avi_save_exit_2:
	g_rec_status = 0;
    return NULL;

}


int processMsg_avi(void *buf, int len, void *rbuf)
{
    //    int cmd_type = -1;
    char *pKey;
    int iKey;
    int iValue;
    char *pValue;
    char *pInput = buf;
    int parseIndex = 0;
    //    int i = 0;

    if(buf==NULL||rbuf==NULL){
        return -1;
    }

    do{
        pKey = ParseVars(pInput, &parseIndex);
        pValue = ParseVars(pInput, &parseIndex);

        if(pKey==NULL||pValue==NULL){
            break;
        }

        iKey = atoi(pKey);
        if(iKey < e_avi_enable||iKey > e_avi_END){
            printf("key error.\n");
            continue;
        }
        printf("key:%s  value:%s ikey:%d\n",pKey,pValue,iKey);
        switch(iKey){
        case e_avi_enable:
			g_rec_enable = atoi(pValue);
            if (g_rec_enable && (!g_rec_status)) {
                //start to record thread
                pthread_t record_id;
                pthread_attr_t attr;

                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                if(pthread_create(&record_id, &attr, &Avi_save, NULL) == 0)
                    printf("start to avi save...\n");
                pthread_attr_destroy(&attr);
            }
            break;

        case e_avi_TimeL:
            iValue = atoi(pValue);
			if (iValue > MAX_REC_TIME_LEN)
				iValue = MAX_REC_TIME_LEN;
			else if (iValue < MIN_REC_TIME_LEN)
				iValue = MIN_REC_TIME_LEN;

            g_rec_TimeL = iValue * 60;
            break;
		case e_avi_schd:
			g_rec_schdule = atoi(pValue);
			break;
        default:
            break;
        }
    }while(parseIndex!=-1);

    return 0;
}


#define AVI_LISTEN      5
void *Avi_Ctrl(void *ptr)
{
    int i;
    int server_fd, client_fd, arrClient_fd[AVI_LISTEN];
    int max_socket_fd = -1;
    struct sockaddr_un client_addr;
    int readlen = -1;
    int	ret = -1;

    char *pMsg_buf;
    char *pRet_buf;
    fd_set	readfset;
    int len = 0;

    pMsg_buf = (char *)malloc(MAX_CMD_LEN * sizeof(char));
    if(NULL == pMsg_buf)
        return NULL;
    pRet_buf = (char *)malloc(MAX_CMD_LEN * sizeof(char));
    if(NULL == pRet_buf) {
        free(pMsg_buf);
        return NULL;
    }

    server_fd = create_server(UN_AVI_DOMAIN);
    if(server_fd<0){
        printf("Avi_Ctrl: error create server_fd.\n");
        system("echo \"Avi_Ctrl: error create server_fd.\" >> /mnt/nfs/system.log");
        free(pMsg_buf);
        return NULL;
    }

    if(listen(server_fd, AVI_LISTEN-1)<0){
        goto error_avserver;
    }
    for(i = 0; i < AVI_LISTEN; i++){
        arrClient_fd[i] = 0;
    }
    arrClient_fd[0] = server_fd;

    sleep(1);

    while(!m_Quit){
        FD_ZERO(&readfset);
        FD_SET(arrClient_fd[0], &readfset);
        max_socket_fd = arrClient_fd[0];
        for(i=1; i<AVI_LISTEN; i++){
            if(arrClient_fd[i]>0){
                FD_SET(arrClient_fd[i], &readfset);
                max_socket_fd = (max_socket_fd<arrClient_fd[i])?(arrClient_fd[i]):(max_socket_fd);
            }
        }

        ret = select(max_socket_fd+1, &readfset, NULL, NULL, NULL);
        if(ret<=0){
            continue;
        }
        if(FD_ISSET(server_fd, &readfset)){
            len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
            if(client_fd<0){
                continue;
            }

            for(i=1; i<AVI_LISTEN; i++){
                if(arrClient_fd[i]<=0){
                    arrClient_fd[i] = client_fd;
                    break;
                }
            }
            if(i==AVI_LISTEN){
                close(client_fd);
                continue;
            }

            if(--ret==0)
                continue;
        }
        for(i = 1; i < AVI_LISTEN; i++){
            if(FD_ISSET(arrClient_fd[i], &readfset)){
                client_fd = arrClient_fd[i];
                memset(pMsg_buf, 0, MAX_CMD_LEN*sizeof(char));
                readlen = read(client_fd, (void *)pMsg_buf, MAX_CMD_LEN);
                printf("read len is %d\n",readlen);
                if(readlen<=3){
                    FD_CLR(client_fd, &readfset);
                    close(client_fd);
                    arrClient_fd[i] = 0;
                    continue;
                }

                fprintf(stdout, "get len is %d, data: %s.\n", readlen, pMsg_buf);
                memset(pRet_buf, 0, MAX_CMD_LEN*sizeof(char));
                len = processMsg_avi(pMsg_buf, strlen(pMsg_buf), pRet_buf);
                fprintf(stdout, "return len: %d.\n", len);
                if(len==0)	continue;
                fprintf(stdout, "return data: %s.\n", pRet_buf);
                if(write(client_fd, pRet_buf, /*strlen(pRet_buf)*/MAX_CMD_LEN)<0){
                    //					fprintf(stdout, "write error.\n");
                    if(errno==EBADF){
                        FD_CLR(client_fd, &readfset);
                        close(client_fd);
                        arrClient_fd[i] = 0;
                    }
                }
            }
        }
    }

error_avserver:
    if(pMsg_buf!=NULL)
        free(pMsg_buf);
    if(pRet_buf!=NULL)
        free(pRet_buf);

    if(server_fd>0){
        close(server_fd);
    }

    return NULL;

}

void Avi_start()
{
    pthread_create(&m_Avi_id, NULL, &Avi_Ctrl, NULL);

    return;
}

int Avi_stop()
{
    printf("AVI_Ctrl stop.....\n");
    m_Quit = 1;
    pthread_join(m_Avi_id, 0);

}


