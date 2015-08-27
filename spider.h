#ifndef SPIDER_H
#define SPIDER_H

/* global data definition
 */

struct epoll_event events[31];  // return events which will be dealed with.
set<unsigned int>Set;           // the 'hash-table'.
URL url;                        // first url we set.
queue<URL>que;                  // url wait-queue(from each thread).
int cnt;                        // record how many urls we have fetched.
int sum_byte;                   // record how many bytes we have fetched.
int pending;                    // record pending urls that wait to deal with 
int epfd;                       // record the epoll fd.
bool is_first_url;              // judge whether is the first url.
double time_used;               // record totally time costed.
pthread_mutex_t quelock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t setlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connlock = PTHREAD_MUTEX_INITIALIZER;
string input;
string HtmFile;
string keyword;

#endif
