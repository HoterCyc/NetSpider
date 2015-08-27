#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <sys/stat.h>
#include <pthread.h>
#include "web.h"
extern const int MAXLEN;//#include "config.h"

/* struct argument
 * used for pass argument at pthread
 */

struct argument {
    URL url;
    int sockfd;
    argument() {}
};

//typedef unsigned int uint
//"Baiduspider" "Wget/1.10.2"//"iaskspider/2.0" //"Sogou Push Spider/3.0" //"msnbot/1.0" //"Sogou web spider/3.0"// "Mozilla/5.0" //"googlebot/2.1"
#define UAGENT "Mozilla/5.0" 
#define ACCEPT "*/*"
//#define CONN "keep-alive"
#define CONN "Close"

#define DEBUG

int GetHostByName(const string&);
int SetNoblocking(const int&);
int ConnectWeb(int&);
int SendRequest(int, URL&);
double Calc_Time_Sec(struct timeval st, struct timeval ed);
void* GetResponse(void *);

#endif
