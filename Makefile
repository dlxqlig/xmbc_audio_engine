HOST=i686-cm-linux
CC=gcc
CXX=g++
AR=ar
AR_RC=ar rc
AS=as
LD=ld
M4=m4
NM=nm
RANLIB=ranlib
BISON=bison
STRIP=strip
OBJCOPY=objcopy
OBJDUMP=objdump
STRINGS=strings
CXXCPP=$(CXX) -E

CPP_DEFS =-DHAVE_CONFIG_H
CPP_OPTS =-Wall -O0 -msse3 -mfpmath=sse -mtune=atom -march=atom -ffast-math -funroll-loops

BASE_DIR ?= .

CFLAGS = -I$(BASE_DIR)/include -I$(BUILD_ROOT)/addpack/usr/include -I$(BUILD_ROOT)/fsroot/include -I$(BUILD_ROOT)/fsroot/usr/include -I$(BUILD_DEST)/include -I$(BUILD_DEST)/include/linux_user -I$(BUILD_DEST)/usr/include
LDFLAGS = -L$(BASE_DIR)
CPPFLAGS = $(CFLAGS) 
INSTALL=/usr/bin/install -c

CMN_SRC_DIR = $(BASE_DIR)/aud-cmn/src
THREAD_DIR = $(CMN_SRC_DIR)/threads
UTILS_DIR = $(CMN_SRC_DIR)/utils
RND_SRC_DIR = $(BASE_DIR)/aud-rnd/src
RND_TEST_DIR = $(BASE_DIR)/aud-rnd/test
DEC_SRC_DIR = $(BASE_DIR)/aud-dec/src
DEC_SRC_TEST = $(BASE_DIR)/aud-dec/test
OMX_DEC_SRC_DIR = $(BASE_DIR)/aud-omx-dec/src

CFLAGS += ${CPP_OPTS} ${CPP_DEFS}

THREADS_SRCS = 	$(THREAD_DIR)/Thread.cpp \
				$(THREAD_DIR)/SystemClock.cpp \
				$(THREAD_DIR)/CriticalSection.cpp \
				$(THREAD_DIR)/Event.cpp

UTILS_SRCS = 	$(UTILS_DIR)/Logger.cpp \
				$(UTILS_DIR)/AudBuffer.cpp \
				$(UTILS_DIR)/AudChInfo.cpp \
				$(UTILS_DIR)/AudUtil.cpp \
				$(UTILS_DIR)/AudConvert.cpp \
				$(UTILS_DIR)/AudDevInfo.cpp \
				$(UTILS_DIR)/AudRemap.cpp

RND_SRCS = 	$(RND_SRC_DIR)/AudSinkNULL.cpp \
				$(RND_SRC_DIR)/AudSinkProfiler.cpp \
				$(RND_SRC_DIR)/AudSinkALSA.cpp \
				$(RND_SRC_DIR)/AudStream.cpp \
				$(RND_SRC_DIR)/AudRender.cpp

RND_TEST_SRCS = $(RND_TEST_DIR)/TestPlayer.cpp

DEC_SRCS = 	$(DEC_SRC_DIR)/OMXAudio.cpp \
				$(DEC_SRC_DIR)/OMXCore.cpp \
				$(DEC_SRC_DIR)/OMXProcess.cpp

DEC_TEST_SRCS = $(DEC_SRC_TEST)/omxplayer.cpp


OMX_DEC_SRCS = 	$(OMX_DEC_SRC_DIR)/library_entry_point.c \
				$(OMX_DEC_SRC_DIR)/omx_audiodec_component.c

THREADS_OBJS = $(patsubst %.cpp, %.o, $(THREADS_SRCS))
UTILS_OBJS = $(patsubst %.cpp, %.o, $(UTILS_SRCS))

RND_OBJS = $(patsubst %.cpp, %.o, $(RND_SRCS))
RND_TEST_OBJS = $(patsubst %.cpp, %.o, $(RND_TEST_SRCS))

DEC_OBJS = $(patsubst %.cpp, %.o, $(DEC_SRCS))
DEC_TEST_OBJS = $(patsubst %.cpp, %.o, $(DEC_TEST_SRCS))

OMX_DEC_OBJS = $(patsubst %.c, %.o, $(OMX_DEC_SRCS))

UTILS = libaudUtils.so
RND = libaudRender.so
RND_TEST = rnd_test
DEC = libaudDec.so
DEC_TEST = dec_test
OMX_DEC = libaudOmxDec.so

.PHONY: all install install_dev install_target clean print
all: print $(UTILS) $(RND) $(RND_TEST) $(DEC) $(DEC_TEST) $(OMX_DEC)

print:
	@echo "****************************************************"
	@echo "*** Start building audio utils                   ***"
	@echo "****************************************************"

%.o : %.cpp
	@echo Compiling $<
	@$(CXX) $(CFLAGS) -c -o $@ $<

%.o : %.c
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(UTILS): $(THREADS_OBJS) $(UTILS_OBJS) 
	@echo Building executable files for $(UTILS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -shared -fpic -o $(UTILS) $(THREADS_OBJS) $(UTILS_OBJS)
	@echo "Successfully built $@"

$(RND): $(RND_OBJS) $(UTILS)
	@echo Building executable files for $(RND)
	$(CXX) $(CFLAGS) $(LDFLAGS) -shared -fpic -o $(RND) $(RND_OBJS) -laudUtils -lavformat -lavutil -lavcodec -lasound -lsamplerate
	@echo "Successfully built $@"

$(RND_TEST): $(RND_TEST_OBJS) $(RND)
	@echo Building executable files for $(RND_TEST)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $(RND_TEST) $(RND_TEST_OBJS) -laudUtils -laudRender -lswresample -lavformat -lavutil -lavcodec

$(DEC): $(DEC_OBJS) $(UTILS)
	@echo Building executable files for $(DEC)
	$(CXX) $(CFLAGS) $(LDFLAGS) -shared -fpic -o $(DEC) $(DEC_OBJS) -laudUtils -lavutil -lavcodec -lavformat -lswscale -lrt -lomxil-bellagio -lpthread -laudRender
	@echo "Successfully built $@"

$(DEC_TEST): $(DEC_TEST_OBJS) $(DEC)
	@echo Building executable files for $(DEC_TEST)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $(DEC_TEST) $(DEC_TEST_OBJS) -laudUtils -laudRender -laudDec

$(OMX_DEC): $(OMX_DEC_OBJS)
	@echo Building executable files for $(OMX_DEC)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -fpic -o $(OMX_DEC) $(OMX_DEC_OBJS) -lavcodec -lavformat -lavutil -lswscale -lswresample
	@echo "Successfully built $@"

install: install_dev install_target

install_dev: 

install_target:
	@echo "****************************************************"
	@echo "*** Start installing audio utils                 ***"
	@echo "****************************************************"


	@echo "Installation done."

install_local:
	@echo "****************************************************"
	@echo "*** Start installing audio render               ***"
	@echo "****************************************************"

clean: 
	@echo Cleaning up previous object files
	@rm -f $(THREADS_OBJS) $(UTILS_OBJS) $(UTILS) $(RND_OBJS) $(RND_TEST_OBJS) $(RND) $(RND_TEST) $(DEC) $(DEC_OBJS) $(DEC_TEST_OBJS) $(DEC_TEST) $(OMX_DEC) $(OMX_DEC_OBJS)
