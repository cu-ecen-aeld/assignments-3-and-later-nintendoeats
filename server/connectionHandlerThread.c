#include "aesdsocket.h"
#include <sys/time.h>
#include "../aesd-char-driver/aesd_ioctl.h"


void ProcessBuffer(char* bufferPtr, size_t bufferLen, int fileFD, int connectionFD, char* residualPtr, size_t* residualSize)
    {
    size_t segmentLen = 0;
    char* segmentPtr = bufferPtr;

    for(size_t rd = 0; rd < bufferLen; ++rd)
        {
        ++segmentLen;

        if(bufferPtr[rd] == '\n')
            {
            pthread_mutex_lock(&FileMutex);

            if(segmentLen == 23
             && memcmp("AESDCHAR_IOCSEEKTO:", segmentPtr, segmentLen) == 0
             && segmentPtr[20] == ','
             )
                {
                struct aesd_seekto seek;
                seek.write_cmd = segmentPtr[19] - '0';
                seek.write_cmd_offset = segmentPtr[21] - '0';
                ioctl(fileFD, AESDCHAR_IOCSEEKTO, seek);
                }
            else
                {
                const int written = write(fileFD, segmentPtr, segmentLen);
                if (written != segmentLen)
                    {fail("Error writing to file.", -1);}
                }

#if USE_AESD_CHAR_DEVICE != 0

            char buf[4096]; //shush
            ssize_t totalOffset = 0;
            ssize_t numRead = 0;
            while ((numRead = pread(fileFD, buf, 4096, totalOffset)) > 0)
                {
                totalOffset += numRead;
                if(connectionFD == 0)
                    {syslog(LOG_DEBUG,"Connection closed while responding");}
                send(connectionFD, buf, numRead, 0);
                }
#else
            struct stat sb;
            fstat(fileFD, &sb);

            char* fileMap = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fileFD, 0);
            if(connectionFD == 0)
                {syslog(LOG_DEBUG,"Connection closed while responding");}
            else
                {send(connectionFD, fileMap, sb.st_size, 0);}
            munmap(fileMap, sb.st_size);
#endif

            fsync(fileFD);

            pthread_mutex_unlock(&FileMutex);

            segmentPtr += segmentLen;
            segmentLen = 0;
            }
        }

    if (segmentLen != 0)
        {memcpy(residualPtr,segmentPtr, segmentLen);}

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

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    setsockopt(connectionFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    bool lastCall = false;

    char buffer[200];
    char residual[200];
    size_t residualSize = 0;

    while(true)
    {
        ssize_t writtenLen = 0;
        writtenLen = recv(connectionFD, buffer, sizeof(buffer) - residualSize, 0);

        //Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
        if(writtenLen <= 0)
        {
            if(!(errno == EAGAIN || errno == EWOULDBLOCK) || writtenLen == 0)
            {
                syslog(LOG_DEBUG,"closed connection from %s", peerString);
                break;
            }

            if(closeRequested)
                {lastCall = true;}

            continue;
        }

        int fileFD = 0;

#if USE_AESD_CHAR_DEVICE != 0
        fileFD = open(aesdfile, O_RDWR, 0666);
#else
        fileFD = open(aesdfile, O_RDWR | O_CREAT | O_TRUNC, 0666);
#endif
        ProcessBuffer(buffer, writtenLen + residualSize, fileFD, connectionFD, residual, &residualSize);
        close(fileFD);

        if(residualSize > 0)
            {memcpy(residual, buffer, residualSize);}


        if(lastCall)
            {break;}
    }

    close(connectionFD);
    return thread_param;
}
