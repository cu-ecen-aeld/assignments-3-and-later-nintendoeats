#pragma once
#include <pthread.h>

struct Connection
    {
    int m_connectionFD;
    pthread_t m_thread;
    };
    
struct ConnectionListItem
    {
    struct Connection m_connection;
    struct ConnectionListItem* m_nextConnectionPtr;
    };
    
struct ConnectionList
    {
    pthread_mutex_t m_mutex;
    struct ConnectionListItem* m_head;
    };
    
struct Connection* AddConnection(struct ConnectionList* list, int connectionFD, pthread_t thread);
int CloseConnection(struct ConnectionList* list, int connectionFD);
void DestroyConnectionList(struct ConnectionList* list);
void InitConnectionList(struct ConnectionList* list);
