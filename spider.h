#ifndef SPIDER_H
#define SPIDER_H

/* global data definition
 */

struct epoll_event g_events[31]; // return events which will be dealed with.
set<unsigned int> g_Set;         // the 'hash-table' indicate whether the url has been fetched.
URL g_url;                       // first url we set.
queue<URL> g_que;                // url wait-queue(from each thread).
int g_cnt;                       // record how many urls we have fetched.
int g_sum_byte;                  // record how many bytes we have fetched.
int g_pending;                   // record pending urls that wait to deal with 
int g_epfd;                      // record the epoll fd.
bool g_is_first_url;             // judge whether is the first url.
double g_time_used;              // record totally time costed.
string g_input;                  // the first url input
string g_keyword;                // keyword in host
int g_nthread;                   // how many threads work in parellel

pthread_mutex_t quelock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t setlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connlock = PTHREAD_MUTEX_INITIALIZER;

#endif
