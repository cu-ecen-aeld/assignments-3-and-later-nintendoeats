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


struct Connection* AddConnection(struct ConnectionList* list, int connectionFD, pthread_t thread)
    {
    pthread_mutex_lock(&(list->m_mutex));
    
    struct ConnectionListItem* newItemPtr = malloc(sizeof(struct ConnectionListItem));
    newItemPtr->m_connection.m_connectionFD = connectionFD;
    newItemPtr->m_connection.m_thread = thread;
    newItemPtr->m_nextConnectionPtr = list->m_head;
    list->m_head = newItemPtr;
    
    pthread_mutex_unlock(&(list->m_mutex));
    return &newItemPtr->m_connection;
    }
    
int CloseConnection(struct ConnectionList* list, int connectionFD)
    {
    pthread_mutex_lock(&(list->m_mutex));
    
    if(list->m_head == NULL)
        {return 1;}
        
        
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
        syslog(LOG_DEBUG, "Closed ConnectionFD %d\n", connectionFD);
        free(next);
        return 0;
        }
    else
        {syslog(LOG_DEBUG, "ConnectionFD %d not found!\n", connectionFD);}
    
    pthread_mutex_unlock(&(list->m_mutex));
    return -1;
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
         
    pthread_mutex_destroy(&(list->m_mutex));
    }
 
 
 
 
