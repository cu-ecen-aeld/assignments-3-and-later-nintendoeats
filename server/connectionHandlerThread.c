#include "aesdsocket.h"


void ProcessBuffer(char* bufferPtr, size_t bufferLen, int fileFD, int connectionFD)
    {
    size_t segmentLen = 0;
    char* segmentPtr = bufferPtr;

    for(size_t read = 0; read < bufferLen; ++read)
        {
        ++segmentLen;

        if(bufferPtr[read] == '\n')
            {
            pthread_mutex_lock(&FileMutex);

            const int written = write(fileFD, segmentPtr, segmentLen);
            if (written != segmentLen)
                {fail("Error writing to file.", -1);}

            struct stat sb;
            fstat(fileFD, &sb);
            char* fileMap = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fileFD, 0);
            if(connectionFD == 0)
                {syslog(LOG_DEBUG,"Connection closed while responding");}
            else
                {send(connectionFD, fileMap, sb.st_size, 0);}
            munmap(fileMap, sb.st_size);

            fsync(fileFD);

            pthread_mutex_unlock(&FileMutex);

            segmentPtr += segmentLen;
            segmentLen = 0;
            }
        }

    if (segmentLen != 0)
        {write(fileFD, segmentPtr, segmentLen);}

    }

void* ConnectionHandlerThread(void* thread_param)
{
    struct Connection* connection = thread_param;

    int connectionFD = connection->m_connectionFD;


    struct sockaddr sa_peer;
    socklen_t len_peer = 0;


    if (getpeername(connectionFD , &sa_peer, &len_peer) == -1)
        {fail("Could not get peer name.", -1);}

    //Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connectedclient.
    struct sockaddr_in *sa_peeraddr = (struct sockaddr_in *)&sa_peer;
    char peerString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa_peeraddr->sin_addr, peerString, sizeof peerString);
    syslog(LOG_DEBUG,"Accepted a connection from %s", peerString);

    while(true)
    {
        char buffer[200];
        ssize_t writtenLen = 0;
        writtenLen = recv(connectionFD, buffer, sizeof(buffer), MSG_DONTWAIT );

        //Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
        if(writtenLen <= 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                {if(!closeRequested){continue;}}
            syslog(LOG_DEBUG,"closed connection from %s", peerString);
            break;

        }
        ProcessBuffer(buffer, writtenLen, fileFD, connectionFD);
    }

    close(connectionFD);
    return thread_param;
}
