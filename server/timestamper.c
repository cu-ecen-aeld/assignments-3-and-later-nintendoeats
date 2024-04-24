#include "aesdsocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static void TimeStamper(int sig, siginfo_t* si, void* uc)
{
    char timeStringBase[1024] ;
    memcpy(timeStringBase,"timestamp:",10);
    char* timeString = timeStringBase + 10;

    pthread_mutex_lock(&FileMutex);
    struct timespec theTime;
    clock_gettime(CLOCK_REALTIME, &theTime);
    size_t timeLen = 10 + strftime(timeString, sizeof(timeStringBase) - 10, "%a, %d %b %Y %T %z%n", gmtime(&theTime.tv_sec));
    write(fileFD,timeStringBase, timeLen);
    pthread_mutex_unlock(&FileMutex);
}


void LaunchTimestamper()
{

    int res = 0;
    timer_t timerId = 0;


    struct sigevent sev = { 0 };


    /* specifies the action when receiving a signal */
    struct sigaction sa = { 0 };

    /* specify start delay and interval */
    struct itimerspec its = {   .it_value.tv_sec  = 10,
                                .it_value.tv_nsec = 0,
                                .it_interval.tv_sec  = 10,
                                .it_interval.tv_nsec = 0
                            };


    sev.sigev_notify = SIGEV_SIGNAL; // Linux-specific
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = NULL;

    /* create timer */
    res = timer_create(CLOCK_REALTIME, &sev, &timerId);

    /* specifz signal and handler */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = TimeStamper;

    /* Initialize signal */
    sigemptyset(&sa.sa_mask);

    printf("Establishing handler for signal %d\n", SIGRTMIN);

    /* Register signal handler */
    if (sigaction(SIGRTMIN, &sa, NULL) == -1)
    {
        fprintf(stderr, "Error sigaction: %s\n", strerror(errno));
        exit(-1);
    }

    /* start timer */
    res = timer_settime(timerId, 0, &its, NULL);

    if ( res != 0)
    {
        fprintf(stderr, "Error timer_settime: %s\n", strerror(errno));
        exit(-1);
    }

}
