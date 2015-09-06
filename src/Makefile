
LINUX_SRC=/usr/src/arm-linux-2.6.28/linux-2.6.28-fa
LINUX_INC=$(LINUX_SRC)/include

MODULE_INC=$(LINUX_SRC)/../module/include

EXE = av_server_onvif_rec
LIB = 

ROOT = /home/onvif/ipnc_app


INCLUDE_DIRS = $(ROOT)/../libipnc/inc ../inc /usr/src/arm-linux-2.6.28/module/include/vcap200_v3 $(LINUX_INC) $(MODULE_INC)/dvr $(MODULE_INC)/isp $(MODULE_INC)/isp210_v3 $(MODULE_INC)/sar_adc $(MODULE_INC)/../BR_infrad $(ROOT)/build/TUTK_p2p/Include $(ROOT)/build/TUTK_p2p/Include/P2PCam $(ROOT)/codes/CmdParser/libyl $(ROOT)/codes/CmdParser/libyl/include $(ROOT)/codes/libmt

LINK_LIBS = ipc cmdyl rtsp pthread m AVAPIs IOTCAPIs mt stdc++ ancda_zy

include $(ROOT)/build/make.rule
