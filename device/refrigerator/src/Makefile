EXEC=refrigerator
CC=gcc
CPP=g++
LIVE_PATH = ../live
LIVE_INC = -I${LIVE_PATH}/UsageEnvironment/include -I${LIVE_PATH}/groupsock/include -I${LIVE_PATH}/liveMedia/include -I${LIVE_PATH}/BasicUsageEnvironment/include

DIR_INC=.
DIR_SRC=.

DIR_OBJ=../obj

SRC=$(wildcard ${DIR_SRC}/*.cpp)
OBJ=$(patsubst %.cpp, ${DIR_OBJ}/%.o, $(notdir ${SRC}))
INCLUDES = -I. ${LIVE_INC}

CFLAGS=-Wall -g
LFLAGS=-lpthread -ldl -lrt -lm -lsqlite3 -lssl -lcrypto

USAGE_ENVIRONMENT_DIR = ${LIVE_PATH}/UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.a
BASIC_USAGE_ENVIRONMENT_DIR = ${LIVE_PATH}/BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.a
LIVEMEDIA_DIR = ${LIVE_PATH}/liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.a
GROUPSOCK_DIR = ${LIVE_PATH}/groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.a
LIBS =  $(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) $(BASIC_USAGE_ENVIRONMENT_LIB) $(USAGE_ENVIRONMENT_LIB)

$(EXEC):$(OBJ)
	$(CPP) $^ -o $@ $(CFLAGS) $(LIBS) $(LFLAGS)

$(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(CPP) -c $< -o $@ $(CFLAGS) $(LFLAGS) $(INCLUDES)

clean:
	rm $(EXEC) *.o
