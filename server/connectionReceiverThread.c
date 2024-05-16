#include "aesdsocket.h"

void* ConnectionReceiverThread(void* thread_param)
{
    struct ConnectionList connections;
    InitConnectionList(&connections);
    int epollFD = epoll_create1(0);

    struct epoll_event socketEvent;
    socketEvent.events = EPOLLIN;
	socketEvent.data.fd = socketFD;

    struct epoll_event closingEvent;
    closingEvent.events = EPOLLIN;
	closingEvent.data.fd = interthreadSocketFD;

    struct epoll_event events[2];

    epoll_ctl(epollFD, EPOLL_CTL_ADD, socketFD, &socketEvent);
    epoll_ctl(epollFD, EPOLL_CTL_ADD, interthreadSocketFD, &closingEvent);

    //Listens for and accepts a connection
    while (!closeRequested)
    {
        const int foundEvents = epoll_wait(epollFD, events, 2, 1);
        if(foundEvents == 0)
            {continue;}

        if(events->data.fd == interthreadSocketFD)
        {
            syslog(LOG_DEBUG, "She died");
            int message = -1;
            char* messagePtr = (char*)&message;
            size_t needed = sizeof(message);
            while(needed != 0)
                {needed -= recv(interthreadSocketFD, &messagePtr, needed, MSG_DONTWAIT);}

            if(message == 0) //closingEvent
                {break;}
            else
                {CloseConnection(&connections, message);}
        }
        else if(events->data.fd == socketFD)
        {
            int connectionFD = accept(socketFD, NULL, NULL);

            if(connectionFD == -1)
                {continue;}

            // Receives data over the connection and appends to the target file
            pthread_t newThread;
            struct Connection* connection  = AddConnection(&connections, connectionFD, 0);
            connection->m_thread = pthread_create(&newThread, NULL, ConnectionHandlerThread, connection);
        }
    }


    syslog(LOG_DEBUG, "Destroying ConnectionList");
    DestroyConnectionList(&connections);

    close(epollFD);
    syslog(LOG_DEBUG, "GTFOing");
    return thread_param;
}
