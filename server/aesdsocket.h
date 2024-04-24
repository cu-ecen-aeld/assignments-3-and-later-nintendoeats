#pragma once
#define _POSIX_C_SOURCE 199309
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stddef.h>

#include "connectionlist.h"

extern atomic_int fileFD;
extern atomic_int socketFD;
extern pthread_mutex_t FileMutex;

extern pthread_cond_t closingVar;
extern pthread_mutex_t closingMutex;
extern atomic_bool closeRequested;

void LaunchTimestamper();
void* ConnectionReceiverThread(void* thread_param);
void* ConnectionHandlerThread(void* thread_param);

extern atomic_int interthreadSocketFD;
void InitITS();
void CloseITS();
void SendOnITS(const char* message, size_t length);

static void fail(const char* message, int returnCode)
    {
    syslog(LOG_DEBUG, "%s", message);
    exit(returnCode);
    }
