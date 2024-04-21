#define _POSIX_C_SOURCE 199309
#include <sys/types.h>
#include <sys/socket.h>
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
  

int socketFD = 0;
int connectionFD = 0;
int fileFD = 0;
atomic_bool closing = false;



void DoClose(int* FD)
    {if(FD > 0) {close(*FD); *FD=0;}}

void CloseAll()
    {
    DoClose(&fileFD);
    DoClose(&socketFD);
    DoClose(&connectionFD);
    }

    
static void sigHandler(int sig)
    {
    syslog(LOG_DEBUG, "Caught signal, exiting");
    CloseAll();
    closing = true;
    exit(0);
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


static void fail(const char* message, int returnCode)
    {
    if(closing)
        {return;}
    syslog(LOG_DEBUG, "%s", message);
    CloseAll();
    exit(returnCode);
    }
    
void ProcessBuffer(char* bufferPtr, size_t bufferLen, int fileFD, int connectionFD)
    {
    size_t segmentLen = 0;
    char* segmentPtr = bufferPtr;
        
    for(size_t read = 0; read < bufferLen; ++read)
        {
        ++segmentLen;
    
        if(bufferPtr[read] == '\n')
            {      
            const int written = write(fileFD, segmentPtr, segmentLen);
            if (errno != 0 || written != segmentLen)
                {fail("Error writing to file.", -1);}
            
            struct stat sb;
            fstat(fileFD, &sb);
            char* fileMap = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fileFD, 0); 
            send(connectionFD, fileMap, sb.st_size, 0);
            munmap(fileMap, sb.st_size);
        
            segmentPtr += segmentLen;
            segmentLen = 0;
            }
        }

    if (segmentLen != 0)
        {write(fileFD, segmentPtr, segmentLen);}

    }

void Daemonize()
    {
    pid_t forkpid = fork();

    if (forkpid == -1) 
        {fail("Could not fork",-1);}
    else if(forkpid != 0)
        {CloseAll();exit(0);}
    else //we are the child
        {
        setsid();
        subscribeToSignals();
        }
    
    //We were the child, now fork again to lose terminal access
    forkpid = fork();
    if (forkpid == -1) 
        {fail("Could not fork",-1);}
    else if(forkpid != 0)
        {CloseAll();exit(0);}
    }

int main(int argc, char* argv[])
    {
    openlog("aesdsocket",LOG_PERROR, LOG_USER);
    
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
            {Daemonize();}
        }
        
    if (listen(socketFD, 10) == -1)
        {fail("Failed to listen.",-1);}
        
          
    fileFD = open("/var/tmp/aesdsocketdata", O_RDWR |O_CREAT | O_TRUNC, 0666);
    if (fileFD == -1)
        {fail("Target file could not be opened.", -1);}

     //Listens for and accepts a connection
      while (true)
        {
        connectionFD = accept(socketFD, NULL, NULL);
      
        if (connectionFD == -1)
            {fail("Accept failed.", -1);}
            
        struct sockaddr sa_peer;
        socklen_t len_peer = 0;
      
      
        if (getpeername(connectionFD , &sa_peer, &len_peer) == -1)
            {fail("Could not get peer name.", -1);}

        //Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connectedclient.
        struct sockaddr_in *sa_peeraddr = (struct sockaddr_in *)&sa_peer;
        char peerString[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa_peeraddr->sin_addr, peerString, sizeof peerString);
        syslog(LOG_DEBUG,"Accepted connection from %s", peerString);
        
        // Receives data over the connection and appends to file /var/tmp/aesdsocketdata


        while(true)
            {
                    
            char buffer[256000];
            ssize_t writtenLen = 0;
            writtenLen = recv(connectionFD, buffer, sizeof(buffer), 0);
            //Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
            if(writtenLen <= 0)
                {syslog(LOG_DEBUG,"closed connection from %s", peerString); break;}
            ProcessBuffer(buffer, writtenLen, fileFD, connectionFD);
                
            }
        connectionFD=0;
        }
      
    return 0;
    }






















