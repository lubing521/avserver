#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "IOTCAPIs.h"
#include "AVAPIs.h"
#include "AVFRAMEINFO.h"
#include "AVIOCTRLDEFs.h"

#include "GM8126_p2p.h"

#include "rtspd.h"

#define SERVTYPE_STREAM_SERVER 16
#define MAX_CLIENT_NUMBER	128
#define MAX_SIZE_IOCTRL_BUF		1024
#define MAX_AV_CHANNEL_NUMBER	16

#define VIDEO_BUF_SIZE	(1024 * 300)
#define AUDIO_BUF_SIZE	1024

typedef struct _AV_Client
{
    int avIndex;
    unsigned char bEnableAudio;
    unsigned char bEnableVideo;
    unsigned char bEnableSpeaker;
    unsigned char bStopPlayBack;
    unsigned char bPausePlayBack;
    int speakerCh;
    int playBackCh;
    SMsgAVIoctrlPlayRecord playRecord;
    pthread_rwlock_t sLock;
}AV_Client;
static AV_Client gClientInfo[MAX_CLIENT_NUMBER];

static int gOnlineNum = 0;
static int gbSearchEvent = 0;
SMsgAVIoctrlListEventResp *gEventList;
static char gUID[32];

static int m_Quit = 0;
static pthread_t m_p2p_id;

static int p2p_send_status = 0;

void PrintErrHandling (int nErr)
{
    switch (nErr)
    {
    case IOTC_ER_SERVER_NOT_RESPONSE :
        //-1 IOTC_ER_SERVER_NOT_RESPONSE
        printf ("[Error code : %d]\n", IOTC_ER_SERVER_NOT_RESPONSE );
        printf ("Master doesn't respond.\n");
        printf ("Please check the network wheather it could connect to the Internet.\n");
        break;
    case IOTC_ER_FAIL_RESOLVE_HOSTNAME :
        //-2 IOTC_ER_FAIL_RESOLVE_HOSTNAME
        printf ("[Error code : %d]\n", IOTC_ER_FAIL_RESOLVE_HOSTNAME);
        printf ("Can't resolve hostname.\n");
        break;
    case IOTC_ER_ALREADY_INITIALIZED :
        //-3 IOTC_ER_ALREADY_INITIALIZED
        printf ("[Error code : %d]\n", IOTC_ER_ALREADY_INITIALIZED);
        printf ("Already initialized.\n");
        break;
    case IOTC_ER_FAIL_CREATE_MUTEX :
        //-4 IOTC_ER_FAIL_CREATE_MUTEX
        printf ("[Error code : %d]\n", IOTC_ER_FAIL_CREATE_MUTEX);
        printf ("Can't create mutex.\n");
        break;
    case IOTC_ER_FAIL_CREATE_THREAD :
        //-5 IOTC_ER_FAIL_CREATE_THREAD
        printf ("[Error code : %d]\n", IOTC_ER_FAIL_CREATE_THREAD);
        printf ("Can't create thread.\n");
        break;
    case IOTC_ER_UNLICENSE :
        //-10 IOTC_ER_UNLICENSE
        printf ("[Error code : %d]\n", IOTC_ER_UNLICENSE);
        printf ("This UID is unlicense.\n");
        printf ("Check your UID.\n");
        break;
    case IOTC_ER_NOT_INITIALIZED :
        //-12 IOTC_ER_NOT_INITIALIZED
        printf ("[Error code : %d]\n", IOTC_ER_NOT_INITIALIZED);
        printf ("Please initialize the IOTCAPI first.\n");
        break;
    case IOTC_ER_TIMEOUT :
        //-13 IOTC_ER_TIMEOUT
        break;
    case IOTC_ER_INVALID_SID :
        //-14 IOTC_ER_INVALID_SID
        printf ("[Error code : %d]\n", IOTC_ER_INVALID_SID);
        printf ("This SID is invalid.\n");
        printf ("Please check it again.\n");
        break;
    case IOTC_ER_EXCEED_MAX_SESSION :
        //-18 IOTC_ER_EXCEED_MAX_SESSION
        printf ("[Error code : %d]\n", IOTC_ER_EXCEED_MAX_SESSION);
        printf ("[Warning]\n");
        printf ("The amount of session reach to the maximum.\n");
        printf ("It cannot be connected unless the session is released.\n");
        break;
    case IOTC_ER_CAN_NOT_FIND_DEVICE :
        //-19 IOTC_ER_CAN_NOT_FIND_DEVICE
        printf ("[Error code : %d]\n", IOTC_ER_CAN_NOT_FIND_DEVICE);
        printf ("Device didn't register on server, so we can't find device.\n");
        printf ("Please check the device again.\n");
        printf ("Retry...\n");
        break;
    case IOTC_ER_SESSION_CLOSE_BY_REMOTE :
        //-22 IOTC_ER_SESSION_CLOSE_BY_REMOTE
        printf ("[Error code : %d]\n", IOTC_ER_SESSION_CLOSE_BY_REMOTE);
        printf ("Session is closed by remote so we can't access.\n");
        printf ("Please close it or establish session again.\n");
        break;
    case IOTC_ER_REMOTE_TIMEOUT_DISCONNECT :
        //-23 IOTC_ER_REMOTE_TIMEOUT_DISCONNECT
        printf ("[Error code : %d]\n", IOTC_ER_REMOTE_TIMEOUT_DISCONNECT);
        printf ("We can't receive an acknowledgement character within a TIMEOUT.\n");
        printf ("It might that the session is disconnected by remote.\n");
        printf ("Please check the network wheather it is busy or not.\n");
        printf ("And check the device and user equipment work well.\n");
        break;
    case IOTC_ER_DEVICE_NOT_LISTENING :
        //-24 IOTC_ER_DEVICE_NOT_LISTENING
        printf ("[Error code : %d]\n", IOTC_ER_DEVICE_NOT_LISTENING);
        printf ("Device doesn't listen or the sessions of device reach to maximum.\n");
        printf ("Please release the session and check the device wheather it listen or not.\n");
        break;
    case IOTC_ER_CH_NOT_ON :
        //-26 IOTC_ER_CH_NOT_ON
        printf ("[Error code : %d]\n", IOTC_ER_CH_NOT_ON);
        printf ("Channel isn't on.\n");
        printf ("Please open it by IOTC_Session_Channel_ON() or IOTC_Session_Get_Free_Channel()\n");
        printf ("Retry...\n");
        break;
    case IOTC_ER_SESSION_NO_FREE_CHANNEL :
        //-31 IOTC_ER_SESSION_NO_FREE_CHANNEL
        printf ("[Error code : %d]\n", IOTC_ER_SESSION_NO_FREE_CHANNEL);
        printf ("All channels are occupied.\n");
        printf ("Please release some channel.\n");
        break;
    case IOTC_ER_TCP_TRAVEL_FAILED :
        //-32 IOTC_ER_TCP_TRAVEL_FAILED
        printf ("[Error code : %d]\n", IOTC_ER_TCP_TRAVEL_FAILED);
        printf ("Device can't connect to Master.\n");
        printf ("Don't let device use proxy.\n");
        printf ("Close firewall of device.\n");
        printf ("Or open device's TCP port 80, 443, 8080, 8000, 21047.\n");
        break;
    case IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED :
        //-33 IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED
        printf ("[Error code : %d]\n", IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED);
        printf ("Device can't connect to server by TCP.\n");
        printf ("Don't let server use proxy.\n");
        printf ("Close firewall of server.\n");
        printf ("Or open server's TCP port 80, 443, 8080, 8000, 21047.\n");
        printf ("Retry...\n");
        break;
    case IOTC_ER_NO_PERMISSION :
        //-40 IOTC_ER_NO_PERMISSION
        printf ("[Error code : %d]\n", IOTC_ER_NO_PERMISSION);
        printf ("This UID's license doesn't support TCP.\n");
        break;
    case IOTC_ER_NETWORK_UNREACHABLE :
        //-41 IOTC_ER_NETWORK_UNREACHABLE
        printf ("[Error code : %d]\n", IOTC_ER_NETWORK_UNREACHABLE);
        printf ("Network is unreachable.\n");
        printf ("Please check your network.\n");
        printf ("Retry...\n");
        break;
    case IOTC_ER_FAIL_SETUP_RELAY :
        //-42 IOTC_ER_FAIL_SETUP_RELAY
        printf ("[Error code : %d]\n", IOTC_ER_FAIL_SETUP_RELAY);
        printf ("Client can't connect to a device via Lan, P2P, and Relay mode\n");
        break;
    case IOTC_ER_NOT_SUPPORT_RELAY :
        //-43 IOTC_ER_NOT_SUPPORT_RELAY
        printf ("[Error code : %d]\n", IOTC_ER_NOT_SUPPORT_RELAY);
        printf ("Server doesn't support UDP relay mode.\n");
        printf ("So client can't use UDP relay to connect to a device.\n");
        break;

    default :
        break;
    }
}


void InitAVInfo()
{
    int i;
    for(i=0;i<MAX_CLIENT_NUMBER;i++)
    {
        memset(&gClientInfo[i], 0, sizeof(AV_Client));
        gClientInfo[i].avIndex = -1;
        pthread_rwlock_init(&(gClientInfo[i].sLock), NULL);
    }
}

void DeInitAVInfo()
{
    int i;
    for(i=0;i<MAX_CLIENT_NUMBER;i++)
    {
        memset(&gClientInfo[i], 0, sizeof(AV_Client));
        gClientInfo[i].avIndex = -1;
        pthread_rwlock_destroy(&gClientInfo[i].sLock);
    }
}

int AuthCallBackFn(char *viewAcc,char *viewPwd)
{
    if(strcmp(viewAcc, "admin") == 0 && strcmp(viewPwd, "888888") == 0)
        return 1;

    return 0;
}

void regedit_client_to_video(int SID, int avIndex)
{
    AV_Client *p = &gClientInfo[SID];
    p->avIndex = avIndex;
    p->bEnableVideo = 1;

    p2p_send_status++;
}

void unregedit_client_from_video(int SID)
{
    AV_Client *p = &gClientInfo[SID];
    p->bEnableVideo = 0;

    if(p2p_send_status>0){
        p2p_send_status--;
    }
}

void P2P_do_ptz(unsigned char ctl, unsigned char spd, unsigned char pot)
{
	int act = 0;
	int control = (int)ctl;
	int speed = 16;//(int)(spd);
	int point = (int)pot;
	unsigned char ptzcmd[128];

	switch(control){
		case AVIOCTRL_PTZ_STOP://0
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, PTZ_STOP, 0);
			act++;
			break;

		case AVIOCTRL_PTZ_UP://1
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, TILT_UP, speed);
			act++;
			break;

		case AVIOCTRL_PTZ_DOWN://2
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, TILT_DOWN, speed);
			act++;
			break;

		case AVIOCTRL_PTZ_LEFT://3
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, PAN_LEFT, speed);
			act++;
			break;

		case AVIOCTRL_PTZ_LEFT_UP://4
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, PT_LEFT_UP, speed);
			act++;
			break;

		case AVIOCTRL_PTZ_LEFT_DOWN://5
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, PT_LEFT_DOWN, speed);
			act++;
			break;

		case AVIOCTRL_PTZ_RIGHT://6
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, PAN_RIGHT, speed);
			act++;
			break;

		case AVIOCTRL_PTZ_RIGHT_UP://7
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, PT_RIGHT_UP, speed);
			act++;
			break;	

		case AVIOCTRL_PTZ_RIGHT_DOWN://8
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, PT_RIGHT_DOWN, speed);
			act++;
			break;	

		case AVIOCTRL_PTZ_AUTO://9
			
			break;

		case AVIOCTRL_PTZ_SET_POINT://10
			break;

		case AVIOCTRL_PTZ_CLEAR_POINT://11
			break;

		case AVIOCTRL_PTZ_GOTO_POINT://12
			break;

		case AVIOCTRL_PTZ_SET_MODE_START://13
			break;

		case AVIOCTRL_PTZ_SET_MODE_STOP://14
			break;

		case AVIOCTRL_PTZ_MODE_RUN://15
			break;

		case AVIOCTRL_PTZ_MENU_OPEN://16
			break;			

		case AVIOCTRL_PTZ_MENU_EXIT://17
			break;

		case AVIOCTRL_PTZ_MENU_ENTER://18
			break;

		case AVIOCTRL_PTZ_FLIP://19
			break;

		case AVIOCTRL_PTZ_START://20
			break;

		case AVIOCTRL_LENS_APERTURE_OPEN://21
			break;

		case AVIOCTRL_LENS_APERTURE_CLOSE://22
			break;

		case AVIOCTRL_LENS_ZOOM_IN://23
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, Z_ZOOM_IN, speed);
			act++;
			break;

		case AVIOCTRL_LENS_ZOOM_OUT://24
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, Z_ZOOM_OUT, speed);
			act++;
			break;	

		case AVIOCTRL_LENS_FOCAL_NEAR://25
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, FOCUS_NEAR, speed);
			act++;
			break;

		case AVIOCTRL_LENS_FOCAL_FAR://26
			sprintf(ptzcmd, "@%d@%d@%d\n", 0, FOUCE_FAR, speed);
			act++;
			break;

		case AVIOCTRL_AUTO_PAN_SPEED://27
			break;

		case AVIOCTRL_AUTO_PAN_LIMIT://28
			break;

		case AVIOCTRL_AUTO_PAN_START://29
			break;

		case AVIOCTRL_PATTERN_START://30
			break;

		case AVIOCTRL_PATTERN_STOP://31
			break;

		case AVIOCTRL_PATTERN_RUN://32
			break;

		case AVIOCTRL_SET_AUX://33
			break;	

		case AVIOCTRL_CLEAR_AUX://34
			break;	

		case AVIOCTRL_MOTOR_RESET_POSITION://35
			break;	

		default:
			break;
	}
	
	if(act){
		mctp_cmd_cb(PTZ, ptzcmd);
	}

	return;
}


void Handle_IOCTRL_Cmd(int SID, int avIndex, char *buf, int type)
{
    switch(type)
    {
    case IOTYPE_USER_IPCAM_START:
    {
        SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *)buf;
        printf("IOTYPE_USER_IPCAM_START[%d:%d]\n", p->channel, avIndex);
        //get writer lock
        int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
        regedit_client_to_video(SID, avIndex);
        //release lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);

        printf("regedit_client_to_video OK\n");
    }
        break;
    case IOTYPE_USER_IPCAM_STOP:
    {
        SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *)buf;
        printf("IOTYPE_USER_IPCAM_STOP[%d:%d]\n", p->channel, avIndex);
        //get writer lock
        int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
        unregedit_client_from_video(SID);
        //release lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
        printf("unregedit_client_from_video OK\n");
    }
        break;
    case IOTYPE_USER_IPCAM_AUDIOSTART:
    {
        SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *)buf;
        printf("IOTYPE_USER_IPCAM_AUDIOSTART[%d:%d]\n", p->channel, avIndex);
        //get writer lock
        int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
//        regedit_client_to_audio(SID, avIndex);
        //release lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
        printf("regedit_client_to_audio OK\n");
    }
        break;
    case IOTYPE_USER_IPCAM_AUDIOSTOP:
    {
        SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *)buf;
        printf("IOTYPE_USER_IPCAM_AUDIOSTOP[%d:%d]\n", p->channel, avIndex);
        //get writer lock
        int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
//        unregedit_client_from_audio(SID);
        //release lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
        printf("unregedit_client_from_audio OK\n");
    }
        break;
    case IOTYPE_USER_IPCAM_SPEAKERSTART:
    {
        SMsgAVIoctrlAVStream *p = (SMsgAVIoctrlAVStream *)buf;
        printf("IOTYPE_USER_IPCAM_SPEAKERSTART[%d:%d]\n", p->channel, avIndex);
        //get writer lock
        int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
        gClientInfo[SID].speakerCh = p->channel;
        gClientInfo[SID].bEnableSpeaker = 1;
        //release lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
        // use which channel decided by client
/*
        int *sid = (int *)malloc(sizeof(int));
        *sid = SID;
        pthread_t Thread_ID;
        int ret;
        if((ret = pthread_create(&Thread_ID, NULL, &thread_ReceiveAudio, (void *)sid)))
        {
            printf("pthread_create ret=%d\n", ret);
            exit(-1);
        }
        pthread_detach(Thread_ID);
*/
    }
        break;
    case IOTYPE_USER_IPCAM_SPEAKERSTOP:
    {
        printf("IOTYPE_USER_IPCAM_SPEAKERSTOP\n");
        //get writer lock
        int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
        gClientInfo[SID].bEnableSpeaker = 0;
        //release lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
        if(lock_ret)
            printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
    }
        break;
    case IOTYPE_USER_IPCAM_LISTEVENT_REQ:
    {
        printf("IOTYPE_USER_IPCAM_LISTEVENT_REQ\n");
        /*
                SMsgAVIoctrlListEventReq *p = (SMsgAVIoctrlListEventReq *)buf;
                if(p->event == AVIOCTRL_EVENT_MOTIONDECT)
                Handle search event(motion) list
                from p->stStartTime to p->stEndTime
                and get list to respond to App
            */
#if 1
        //SendPushMessage(AVIOCTRL_EVENT_MOTIONDECT);

        if(gbSearchEvent == 0) //sample code just do search event list once, actually must renew when got this cmd
        {
            gEventList = (SMsgAVIoctrlListEventResp *)malloc(sizeof(SMsgAVIoctrlListEventResp)+sizeof(SAvEvent)*3);
            memset(gEventList, 0, sizeof(SMsgAVIoctrlListEventResp));
            gEventList->total = 1;
            gEventList->index = 0;
            gEventList->endflag = 1;
            gEventList->count = 3;
            int i;
            for(i=0;i<gEventList->count;i++)
            {
                gEventList->stEvent[i].stTime.year = 2012;
                gEventList->stEvent[i].stTime.month = 6;
                gEventList->stEvent[i].stTime.day = 20;
                gEventList->stEvent[i].stTime.wday = 5;
                gEventList->stEvent[i].stTime.hour = 11;
                gEventList->stEvent[i].stTime.minute = i;
                gEventList->stEvent[i].stTime.second = 0;
                gEventList->stEvent[i].event = AVIOCTRL_EVENT_MOTIONDECT;
                gEventList->stEvent[i].status = 0;
            }
            gbSearchEvent = 1;
        }
        avSendIOCtrl(avIndex, IOTYPE_USER_IPCAM_LISTEVENT_RESP, (char *)gEventList, sizeof(SMsgAVIoctrlListEventResp)+sizeof(SAvEvent)*gEventList->count);
#endif
    }
        break;
    case IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL:
    {
        SMsgAVIoctrlPlayRecord *p = (SMsgAVIoctrlPlayRecord *)buf;
        SMsgAVIoctrlPlayRecordResp res;
        printf("IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL cmd[%d]\n", p->command);
        if(p->command == AVIOCTRL_RECORD_PLAY_START)
        {
            memcpy(&gClientInfo[SID].playRecord, p, sizeof(SMsgAVIoctrlPlayRecord));
            //get writer lock
            int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
            if(lock_ret)
                printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
            gClientInfo[SID].bPausePlayBack = 0;
            gClientInfo[SID].bStopPlayBack = 0;
            gClientInfo[SID].playBackCh = IOTC_Session_Get_Free_Channel(SID);
            res.command = AVIOCTRL_RECORD_PLAY_START;
            res.result = gClientInfo[SID].playBackCh;

            //release lock
            lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
            if(lock_ret)
                printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
            int i;
            for(i=0;i<gEventList->count;i++)
            {
                if(p->stTimeDay.minute == gEventList->stEvent[i].stTime.minute)
                    gEventList->stEvent[i].status = 1;
            }
/*
            if(avSendIOCtrl(avIndex, IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL_RESP, (char *)&res, sizeof(SMsgAVIoctrlPlayRecordResp)) < 0)
                break;
            int *sid = (int *)malloc(sizeof(int));
            *sid = SID;
            pthread_t ThreadID;
            int ret;
            if((ret = pthread_create(&ThreadID, NULL, &thread_PlayBack, (void *)sid)))
            {
                printf("pthread_create ret=%d\n", ret);
                exit(-1);
            }
            pthread_detach(ThreadID);
*/
        }
        else if(p->command == AVIOCTRL_RECORD_PLAY_PAUSE)
        {
            res.command = AVIOCTRL_RECORD_PLAY_PAUSE;
            res.result = 0;
            if(avSendIOCtrl(avIndex, IOTYPE_USER_IPCAM_RECORD_PLAYCONTROL_RESP, (char *)&res, sizeof(SMsgAVIoctrlPlayRecordResp)) < 0)
                break;
            //get writer lock
            int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
            if(lock_ret)
                printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
            gClientInfo[SID].bPausePlayBack = !gClientInfo[SID].bPausePlayBack;
            //release lock
            lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
            if(lock_ret)
                printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
        }
        else if(p->command == AVIOCTRL_RECORD_PLAY_STOP)
        {
            //get writer lock
            int lock_ret = pthread_rwlock_wrlock(&gClientInfo[SID].sLock);
            if(lock_ret)
                printf("Acquire SID %d rwlock error, ret = %d\n", SID, lock_ret);
            gClientInfo[SID].bStopPlayBack = 1;
            //release lock
            lock_ret = pthread_rwlock_unlock(&gClientInfo[SID].sLock);
            if(lock_ret)
                printf("Release SID %d rwlock error, ret = %d\n", SID, lock_ret);
        }
    }
        break;

		case IOTYPE_USER_IPCAM_PTZ_COMMAND:
				printf("we get PTZ command.\n");

				SMsgAVIoctrlPtzCmd *ptzcmd = (SMsgAVIoctrlPtzCmd *)buf;
				P2P_do_ptz(ptzcmd->control, ptzcmd->speed, ptzcmd->point);
				
				break;

    default:
        printf("avIndex %d: non-handle type[%X]\n", avIndex, type);
        break;
    }
}

void *thread_ForAVServerStart(void *arg)
{
    int SID = *(int *)arg;
    free(arg);
    int ret;
    unsigned int ioType;
    char ioCtrlBuf[MAX_SIZE_IOCTRL_BUF];
    struct st_SInfo Sinfo;
    if(IOTC_Session_Check(SID, &Sinfo) == IOTC_ER_NoERROR)
    {
        char *mode[3] = {"P2P", "RLY", "LAN"};
        // print session information(not a must)
        printf("Client is from[%s:%d] Mode[%s] VPG[%d:%d:%d] VER[%lX] NAT[%d] AES[%d]\n", Sinfo.RemoteIP, Sinfo.RemotePort, mode[(int)Sinfo.Mode], Sinfo.VID, Sinfo.PID, Sinfo.GID, Sinfo.IOTCVersion, Sinfo.NatType, Sinfo.isSecure);
    }
    printf("thread_ForAVServerStart SID[%d]\n", SID);

    int nResend=-1;
    int avIndex = avServStart3(SID, AuthCallBackFn, 0, SERVTYPE_STREAM_SERVER, 0, &nResend);
    //int avIndex = avServStart2(SID, AuthCallBackFn, 0, SERVTYPE_STREAM_SERVER, 0);
    //int avIndex = avServStart(SID, "admin", "888888", 0, SERVTYPE_STREAM_SERVER, 0);
    if(avIndex < 0)
    {
        printf("avServStart3 failed SID[%d] code[%d]!!!\n", SID, avIndex);
        printf("[thread_ForAVServerStart] exit index[%d]....\n", avIndex);
        IOTC_Session_Close(SID);
        gOnlineNum--;
        pthread_exit(0);
    }
    printf("avServStart3 OK[%d], Resend[%d]\n", avIndex, nResend);

    while(1)
    {
        ret = avRecvIOCtrl(avIndex, &ioType, (char *)&ioCtrlBuf, MAX_SIZE_IOCTRL_BUF, 1000);
        if(ret >= 0)
        {
            Handle_IOCTRL_Cmd(SID, avIndex, ioCtrlBuf, ioType);
        }
        else if(ret != AV_ER_TIMEOUT)
        {
            printf("avRecvIOCtrl error, code[%d]\n", ret);
            break;
        }
    }

    //printf("avServStop SID[%d] calling..............\n", SID);
    unregedit_client_from_video(SID);
//    unregedit_client_from_audio(SID);
    avServStop(avIndex);
    printf("[thread_ForAVServerStart] exit, SID=[%d], avIndex=[%d]....\n", SID, avIndex);

    IOTC_Session_Close(SID);
    gOnlineNum--;

    pthread_exit(0);
}

unsigned int getTimeStamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec*1000 + tv.tv_usec/1000);
}

#define ENC_STREAM_NUM 2
static int m_NVRserver_quit = 0;
struct _tagNVR_stream_status{
    int streamFlag;  //0:no new stream 1:new stream
    int SerialBook;
    int SerialLock;
    int check; //how long to check, -1: do not check
}p2p_stream_status[ENC_STREAM_NUM];
#define P2P_TO_CHECK    3

#if 0
static int av_field[ENC_STREAM_NUM][7] = {

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

static int GetVideoSerial( int vType )
{
    AV_DATA av_data;
    int *cmd = av_field[vType];

    mt_get(cmd[AV_GET_MPEG4_SERIAL], -1, &av_data );

    return av_data.serial;
}
#endif

int send_frame_p2p(char *buf, int size, int flag)
{
    int i = 0;
    int ret;
    FRAMEINFO_t frameInfo;
    memset(&frameInfo, 0, sizeof(FRAMEINFO_t));
    frameInfo.codec_id = MEDIA_CODEC_VIDEO_H264;
    frameInfo.flags = flag;

		if(!p2p_send_status){
			return 1;
		}	

    for(i=0; i<MAX_CLIENT_NUMBER; i++){
        int lock_ret = pthread_rwlock_rdlock(&gClientInfo[i].sLock);
        if(lock_ret)
            printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);
        if(gClientInfo[i].avIndex < 0 || gClientInfo[i].bEnableVideo == 0)
        {
            //release reader lock
            lock_ret = pthread_rwlock_unlock(&gClientInfo[i].sLock);
            if(lock_ret)
                printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);
            continue;
        }

        frameInfo.onlineNum = gOnlineNum;
        ret = avSendFrameData(gClientInfo[i].avIndex, buf, size, &frameInfo, sizeof(FRAMEINFO_t));
        //release reader lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[i].sLock);
        if(lock_ret)
            printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);

        if(ret == AV_ER_EXCEED_MAX_SIZE) // means data not write to queue, send too slow, I want to skip it
        {
            //printf("AV_ER_EXCEED_MAX_SIZE[%d]\n", gClientInfo[i].avIndex);
            usleep(10000);
            continue;
        }
        else if(ret == AV_ER_SESSION_CLOSE_BY_REMOTE)
        {
            printf("thread_VideoFrameData AV_ER_SESSION_CLOSE_BY_REMOTE SID[%d]\n", i);
            unregedit_client_from_video(i);
            continue;
        }
        else if(ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT)
        {
            printf("thread_VideoFrameData AV_ER_REMOTE_TIMEOUT_DISCONNECT SID[%d]\n", i);
            unregedit_client_from_video(i);
            continue;
        }
        else if(ret == IOTC_ER_INVALID_SID)
        {
            printf("Session cant be used anymore\n");
            unregedit_client_from_video(i);
            continue;
        }
        else if(ret < 0)
            printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    }

    return 1;
}
#if 0
void *thread_VideoFrameData(void *arg)
{
    AV_DATA av_data;
    int ret;
    //    struct timeval tp_now;

    int i = 1;
    int *cmd;

    while(!m_Quit){
        cmd = av_field[i];
        if(p2p_send_status){
            if(p2p_stream_status[i].streamFlag){
                ret = mt_get(cmd[AV_LOCK_MP4_IFRAME], -1, &av_data);
                p2p_stream_status[i].SerialBook = av_data.serial;
                p2p_stream_status[i].streamFlag = 0;
            }else{
                ret = mt_get(cmd[AV_LOCK_MP4], p2p_stream_status[i].SerialBook, &av_data);
                if(p2p_stream_status[i].SerialBook>av_data.serial){
                    //                        printf("sleep 1........\n");
                    usleep(5000);
                    continue;
                }
                p2p_stream_status[i].SerialBook = av_data.serial;
                if(av_data.ptr==NULL){
                    //                        printf("sleep 2.......\n");
                    usleep(5000);
                    continue;
                }
            }

            if((ret==RET_SUCCESS)&&(p2p_stream_status[i].check>=0)){
                if((av_data.flags == AV_FLAGS_MP4_I_FRAME)&&
                        (p2p_stream_status[i].check==0)){
                    int serial_now;
                    serial_now = GetVideoSerial(i);
                    p2p_stream_status[i].check = P2P_TO_CHECK;
                    if( serial_now - p2p_stream_status[i].SerialBook > 25 ){
                        mt_get(cmd[AV_UNLOCK_MP4], p2p_stream_status[i].SerialBook, &av_data);
                        p2p_stream_status[i].streamFlag = 1;
                        usleep(5000);
                        continue;
                    }
                }else{
                    p2p_stream_status[i].check--;
                }
            }

            if(ret==RET_SUCCESS){
                send_frame_p2p(av_data.ptr, av_data.size,
                               (av_data.flags==I_FRAME)?IPC_FRAME_FLAG_IFRAME:IPC_FRAME_FLAG_PBFRAME);

                if(p2p_stream_status[i].SerialLock>0){
                    mt_get(cmd[AV_UNLOCK_MP4], p2p_stream_status[i].SerialLock, &av_data);
                }
                p2p_stream_status[i].SerialLock = p2p_stream_status[i].SerialBook;

                p2p_stream_status[i].SerialBook++;
            }else{
                p2p_stream_status[i].streamFlag = 1;
                p2p_stream_status[i].SerialBook = -1;
                p2p_stream_status[i].check = P2P_TO_CHECK;
                //                    printf("sleep 3..........\n");
                usleep(5000);
            }

        }else{
            p2p_stream_status[i].streamFlag = 1;
            p2p_stream_status[i].SerialBook = -1;
            p2p_stream_status[i].check = P2P_TO_CHECK;
            //                printf("sleep 5................\n");
            sleep(1);
        }
    }

    pthread_exit(0);

}
#endif
void LoginInfoCB(unsigned long nLoginInfo)
{
    if((nLoginInfo & 0x04))
        printf("I can be connected via Internet\n");
    else if((nLoginInfo & 0x08))
        printf("I am be banned by IOTC Server because UID multi-login\n");
}

void *thread_Login(void *arg)
{
    int ret;

    while(!m_Quit)
    {
        ret = IOTC_Device_Login((char *)arg, NULL, NULL);
        printf("IOTC_Device_Login() ret = %d\n", ret);
        if(ret == IOTC_ER_NoERROR)
        {
            break;
        }
        else
        {
            PrintErrHandling (ret);
            sleep(30);
        }
    }

    pthread_exit(0);
}

#if 0
void create_streamout_thread()
{
    int ret;
    pthread_t ThreadVideoFrameData_ID;

    if((ret = pthread_create(&ThreadVideoFrameData_ID, NULL, &thread_VideoFrameData, NULL)))
    {
        printf("pthread_create ret=%d\n", ret);
        exit(-1);
    }
    pthread_detach(ThreadVideoFrameData_ID);
}
#endif


#define UID_FILE "/mnt/nfs/uid.cfg"
int get_uid()
{
	FILE *fp;
	int n;

	fp = fopen(UID_FILE, "r");
	if(fp==NULL){
		printf("open %s failed.\n", UID_FILE);
		return -1;
	}

	memset(gUID, 0x0, sizeof(gUID));
	n = fread(gUID, 1, sizeof(gUID), fp);
	if(n<=0){
		printf("P2P thread fread UID file error.\n");
		fclose(fp);
		return -1;	
	}

	fclose(fp);
	
	return 0;
}

void *p2p_start(void *ptr)
{
    int ret , SID;
    pthread_t ThreadLogin_ID;

    InitAVInfo();

//    create_streamout_thread();

    IOTC_Set_Max_Session_Number(MAX_CLIENT_NUMBER);
    ret = IOTC_Initialize2(0);
    //printf("IOTC_Initialize2() ret = %d\n", ret);
    if(ret != IOTC_ER_NoERROR)
    {
        printf("  [] IOTC_Initialize2(), ret=[%d]\n", ret);
        PrintErrHandling (ret);
        //printf("IOTCAPIs_Device exit...!!\n");
        DeInitAVInfo();
        return 0;
    }

    // Versoin of IOTCAPIs & AVAPIs
    unsigned long iotcVer;
    IOTC_Get_Version(&iotcVer);
    int avVer = avGetAVApiVer();
    unsigned char *p = (unsigned char *)&iotcVer;
    unsigned char *p2 = (unsigned char *)&avVer;
    char szIOTCVer[16], szAVVer[16];
    sprintf(szIOTCVer, "%d.%d.%d.%d", p[3], p[2], p[1], p[0]);
    sprintf(szAVVer, "%d.%d.%d.%d", p2[3], p2[2], p2[1], p2[0]);
    printf("IOTCAPI version[%s] AVAPI version[%s]\n", szIOTCVer, szAVVer);

    IOTC_Get_Login_Info_ByCallBackFn(LoginInfoCB);
    avInitialize(MAX_CLIENT_NUMBER*3);

//    strcpy(gUID, "CL4991REDYYFAHPPSZWJ");
//    strcpy(gUID, "E34981RP52E78GPPWZM1");
		if(get_uid()){
			printf("P2P thread get uid failed.\n");
			goto exit_p2p;
		}

    if((ret = pthread_create(&ThreadLogin_ID, NULL, &thread_Login, (void *)gUID)))
    {
        printf("pthread_create(ThreadLogin_ID), ret=[%d]\n", ret);
        exit(-1);
    }
    pthread_detach(ThreadLogin_ID);


    while(!m_Quit)
    {
        // Accept connection only when IOTC_Listen() calling
        SID = IOTC_Listen(0);

        if(SID > -1)
        {
            int *sid = (int *)malloc(sizeof(int));
            *sid = SID;
            pthread_t Thread_ID;
            int ret = pthread_create(&Thread_ID, NULL, &thread_ForAVServerStart, (void *)sid);
            if(ret < 0) printf("pthread_create failed ret[%d]\n", ret);
            else
            {
                pthread_detach(Thread_ID);
                gOnlineNum++;
            }
        }
        else
        {
            PrintErrHandling (SID);
            if(SID == IOTC_ER_EXCEED_MAX_SESSION)
            {
                sleep(5);
            }
        }
    }


exit_p2p:
    DeInitAVInfo();
    avDeInitialize();
    IOTC_DeInitialize();

		while(!m_Quit){
			sleep(2);
		}

    return 0;
}



void init_p2p()
{

    pthread_create(&m_p2p_id, NULL, &p2p_start, NULL);

    return;
}

void quit_p2p()
{
    m_Quit = 1;
    pthread_join(m_p2p_id, 0);

    return;
}




