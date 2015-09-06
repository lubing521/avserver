
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
#include <sys/sysinfo.h>

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

#define MAX_LIST_FD		10
void av_server(void)
{
	int ch_num, stream, i;
	int server_fd, client_fd, arrClient_fd[MAX_LIST_FD];
	int max_socket_fd = -1;
	struct sockaddr_un client_addr;
	int readlen = -1;
	int	ret = -1;

	char *pMsg_buf;
	char *pRet_buf;
	fd_set	readfset;
	int len = 0;
	//char log[256];

	pMsg_buf = (char *)malloc(MAX_CMD_LEN*sizeof(char));
	if(NULL==pMsg_buf){
		return;
	}
	pRet_buf = (char *)malloc(MAX_CMD_LEN*sizeof(char));
	if(NULL==pRet_buf){
		free(pMsg_buf);
		return;
	}

	server_fd = create_server(UN_AVSERVER_DOMAIN);
	if(server_fd<0){
		printf("avserver: error create server_fd.\n");
		system("echo \"avserver: error create server_fd.\" >> /mnt/nfs/system.log");
		free(pMsg_buf);
		return;
	}

	if(listen(server_fd, MAX_LIST_FD-1)<0)
	{
		goto error_avserver;
	}
	for(i=0; i<MAX_LIST_FD; i++){
		arrClient_fd[i] = 0;
	}
	arrClient_fd[0] = server_fd;

	sleep(2);	

#if 1
	g_stRec_cfg_file.enable = 1;
	g_stRec_cfg_file.time_len = 5;
	g_stRec_cfg_file.sch = 0xffffff;
#endif
	rec_cfg_set();
	
	set_audio_play_callback(put_audio_data);
	//========liz================
	cmd_receiver_thread_start();
	usleep(100);
	if (g_stCloud_cfg_file.enable) {
		YL_Start(g_stNet_file.macaddr, 
			g_stCloud_cfg_file.addr, 
			g_stCloud_cfg_file.port, 
			g_stCloud_cfg_file.id, 
			g_stCloud_cfg_file.pw,
			g_stCloud_cfg_file.name);
	}
	//===========================

	printf("start...\n");
	while(1){
		FD_ZERO(&readfset);
		FD_SET(arrClient_fd[0], &readfset);
		max_socket_fd = arrClient_fd[0];
		for(i=1; i<MAX_LIST_FD; i++){
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

			for(i=1; i<MAX_LIST_FD; i++){
				if(arrClient_fd[i]<=0){
					arrClient_fd[i] = client_fd;
					break;
				}
			}
			if(i==MAX_LIST_FD){
				close(client_fd);
				continue;			
			}

			if(--ret==0)
				continue;
		}
		for(i=1; i<MAX_LIST_FD; i++){
			if(FD_ISSET(arrClient_fd[i], &readfset)){
				
				client_fd = arrClient_fd[i];
				memset(pMsg_buf, 0, MAX_CMD_LEN*sizeof(char));
				readlen = read(client_fd, (void *)pMsg_buf, MAX_CMD_LEN);
//				printf("read len is %d\n",readlen);
				if(readlen<=3){
					FD_CLR(client_fd, &readfset);
					close(client_fd);
					arrClient_fd[i] = 0;
					continue;
				}
				
				fprintf(stdout, "get len is %d, data: %s.\n", readlen, pMsg_buf);
				//sprintf(log, "echo \"%s\" >> /mnt/mtd/time.log", pMsg_buf);
				//system(log);
				memset(pRet_buf, 0, MAX_CMD_LEN*sizeof(char));
				len = processMsg(pMsg_buf, strlen(pMsg_buf), pRet_buf);
				//fprintf(stdout, "return len: %d.\n", len);
				if(len==0)	continue;
				fprintf(stdout, "return data: %s.\n", pRet_buf);
				if(write(client_fd, pRet_buf, /*strlen(pRet_buf)*/MAX_CMD_LEN)<0){
//					fprintf(stdout, "write error.\n");
					if (errno == EBADF) {
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

	if(server_fd>0)
		close(server_fd);

	return;
}


int boot_other()
{
/*
	//onvifserver启动必须在avserver之后
	system("/usr/bin/onvifserver.out &");
	sleep(2);

	system("/usr/bin/wis-streamer &");
	sleep(2);
*/
	return 0;
}


