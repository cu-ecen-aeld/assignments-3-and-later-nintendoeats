
#include "aesdsocket.h"

atomic_int socketFD = 0;
atomic_int closingSignalFD = 0;
pthread_mutex_t FileMutex;


pthread_cond_t closingVar;
pthread_mutex_t closingMutex;
atomic_bool closeRequested = false;

    
static void sigHandler(int sig)
    {
    syslog(LOG_DEBUG, "Caught signal, exiting");
    closeRequested = true;
    pthread_cond_broadcast(&closingVar);
    }
        
static void subscribeToSignals()
    {
    struct sigaction sigterm_action;
    memset(&sigterm_action, 0, sizeof(sigterm_action));
    sigterm_action.sa_handler = &sigHandler;
    
    // Mask other signals from interrupting SIGINT handler
    sigfillset(&sigterm_action.sa_mask);
    // Register SIGINT handler
    sigaction(SIGINT,  &sigterm_action, NULL);
    sigaction(SIGTERM, &sigterm_action, NULL);
    }


void Daemonize()
    {
    pid_t forkpid = fork();

    if (forkpid == -1) 
        {fail("Could not fork",-1);}
    else if(forkpid != 0)
        {exit(0);}
    else //we are the child
        {setsid();}
    
    //We were the child, now fork again to lose terminal access
    forkpid = fork();
    if (forkpid == -1) 
        {fail("Could not fork",-1);}
    else if(forkpid != 0)
        {exit(0);}
    }
    
    
int main(int argc, char* argv[])
    {
    openlog("aesdsocket",LOG_PERROR, LOG_USER);
    pthread_cond_init(&closingVar, NULL);
    
    // Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
    socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (socketFD == -1)
        {fail("Failed to create socket",-1);}
    
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
  
    sa.sin_family = AF_INET;
    sa.sin_port = htons(9000);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(socketFD,(struct sockaddr *)&sa, sizeof sa) == -1)
        {fail("Failed to bind socket.",-1);}
        
        
    for(int i = 0; i < argc; ++i)
        {
        if(strcmp(argv[i], "-d") == 0)
            {Daemonize(); break;}
        }

    subscribeToSignals();
        
    if (listen(socketFD, 10) == -1)
        {fail("Failed to listen.",-1);}


    int fileFD = 0;
#if USE_AESD_CHAR_DEVICE != 0
    fileFD = open(aesdfile, O_RDWR, 0666);
#else
    fileFD = open(aesdfile, O_RDWR | O_CREAT | O_TRUNC, 0666);
#endif
    if (fileFD == -1)
        {fail("Target file could not be opened.", -1);}
    close (fileFD);

    pthread_mutex_init(&FileMutex, NULL);

#if USE_AESD_CHAR_DEVICE == 0
    LaunchTimestamper();
#endif
    pthread_t connectionReceiverThread;
    pthread_create(&connectionReceiverThread, NULL, ConnectionReceiverThread, 0);

    InitITS();

    // Wait for the process to end before cleaning up.
    while(!closeRequested)
        {pthread_cond_wait(&closingVar, &closingMutex);}

    const int zilch = 0;
    SendOnITS((void*)&zilch, sizeof(zilch));
    pthread_join(connectionReceiverThread,NULL);
    close(socketFD);
    pthread_mutex_destroy(&FileMutex);
    pthread_cond_destroy(&closingVar);
    CloseITS();
    syslog(LOG_DEBUG, "Main thread finished.");
    return 0;
    }























