#include "connectionlist.h"
#include <stdlib.h>
#include <syslog.h>


int FindConnection_nolock(
    struct ConnectionList* list,
    int connectionFD, 
    struct ConnectionListItem** itemOut,
    struct ConnectionListItem** itemBeforeOut)
{
    struct ConnectionListItem* next = list->m_head;
    struct ConnectionListItem* prev = NULL;
        
    while(next != NULL)
        {
        if(next->m_connection.m_connectionFD == connectionFD)
            {
            *itemOut = next;
            if(itemBeforeOut != NULL)
                {*itemBeforeOut = prev;}
            return 0;
            }
        prev = next;
        next = next->m_nextConnectionPtr;
        }
    return -1;
}

struct ConnectionListItem* FindEnd_nolock(struct ConnectionList* list)
{
    if(list->m_head == NULL)
        {return NULL;}

    struct ConnectionListItem* next = list->m_head;

    while(next->m_nextConnectionPtr != NULL)
        {next = next->m_nextConnectionPtr;}

    return next;
}

struct Connection* AddConnection(struct ConnectionList* list, int connectionFD, pthread_t thread)
{
    pthread_mutex_lock(&(list->m_mutex));

    struct ConnectionListItem* newItemPtr = malloc(sizeof(struct ConnectionListItem));
    newItemPtr->m_connection.m_connectionFD = connectionFD;
    newItemPtr->m_connection.m_thread = thread;
    newItemPtr->m_nextConnectionPtr = NULL;

    struct ConnectionListItem* end = FindEnd_nolock(list);
    if(end == NULL)
        {list->m_head = newItemPtr;}
    else
        {end->m_nextConnectionPtr = newItemPtr;}
    
    pthread_mutex_unlock(&(list->m_mutex));

    return &newItemPtr->m_connection;
}
    
int CloseConnection(struct ConnectionList* list, int connectionFD)
{
    pthread_mutex_lock(&(list->m_mutex));
    
    syslog(LOG_DEBUG, "Trying to close ConnectionFD %d\n", connectionFD);

    if(list->m_head == NULL)
        {return 1;}
        
    int status = 0;
        
    struct ConnectionListItem* next;
    struct ConnectionListItem* prev;

    if(0 == FindConnection_nolock(list, connectionFD, &next, &prev ))
    {
        if(prev == NULL)
            {list->m_head = next->m_nextConnectionPtr;}
        else
            {prev->m_nextConnectionPtr = next->m_nextConnectionPtr;}

        syslog(LOG_DEBUG, "Closing ConnectionFD %d\n", connectionFD);
        pthread_join(next->m_connection.m_thread, NULL);
        free(next);
        syslog(LOG_DEBUG, "Closed ConnectionFD %d\n", connectionFD);
    }
    else
    {
        syslog(LOG_DEBUG, "ConnectionFD %d not found!\n", connectionFD);
        status = -1;
    }
    
    pthread_mutex_unlock(&(list->m_mutex));
    return status;
}
    
void InitConnectionList(struct ConnectionList* list)
{
    pthread_mutex_init(&(list->m_mutex), NULL);
    list->m_head = NULL;
}

void DestroyConnectionList(struct ConnectionList* list)
{
    struct ConnectionListItem* next = list->m_head;
    
    while(next != NULL)
    {
        struct ConnectionListItem* tofree = next;
        next = next->m_nextConnectionPtr;
        CloseConnection(list, tofree->m_connection.m_connectionFD);
    }
         
    syslog(LOG_DEBUG, "Destroyed all connections");
    pthread_mutex_destroy(&(list->m_mutex));
}
 
 
 
 
