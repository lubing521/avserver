
#ifndef _HI3518C_P2P_h
#define _HI3518C_P2P_h

// ************************ Platform Selection ************************
//#define IOTC_ARC_HOPE312
#define IOTC_Linux
//#define IOTC_Win32

// ************************ OS Selection ******************************
//#define OS_ANDROID

// ************************ Compilier Option **************************
//#define _ARC_COMPILER	 // for Arc compilier

// ************************ Debug Option **************************
//#define OUTPUT_DEBUG_MESSAGE			// Define this to enable debug output.
// #ifdef OS_ANDROID
// 	#define LOG_FILE	"/sdcard/log_iotcapi.txt"	// Define this to redirect debug output into specified log file.
// #else
// 	#define LOG_FILE	"log_iotcapi.txt"			// Define this to redirect debug output into specified log file.
// #endif

// ************************ AVAPI Debug Option ***************************
//#define OUTPUT_DEBUG_MESSAGE_AV			// Define this to enable av module debug output.

//#ifdef OS_ANDROID
//	#define LOG_FILE_AVAPI	"/sdcard/log_avapi.txt"
//#else
//	#define LOG_FILE_AVAPI	"log_avapi.txt"
//#endif
int send_frame_p2p(char *buf, int size, int flag);
void init_p2p();
void quit_p2p();

#endif //_IOTCAPIs_charlie_H_Platform_Config_h
