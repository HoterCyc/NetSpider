/* http.cpp
 * this file supports TCP socket operations. such as gethostbyname()
 * create socket etc.
 */

#include "http.h"

extern set<unsigned int>Set;
extern URL url; 
extern queue<URL>que;
extern struct epoll_event events[31];
extern struct timeval t_st, t_ed;
extern int epfd;
extern int cnt;
extern int sum_byte;
extern int pending;
extern int MAX_URL;
extern bool is_first_url;
extern double time_used;
extern pthread_mutex_t quelock;
extern pthread_mutex_t connlock;
struct hostent *Host;

/* GetHostByName()
 * by call gethostbyname(), we can get host infomation
 * through domain
 */

int GetHostByName(const string& hname)
{
	if((Host = gethostbyname(hname.c_str())) == NULL)
	{
		return -1;
	}
	return 1;
}

/* SetNoblocking()
 * set nonblocking IO model
 */

int SetNoblocking(const int& sockfd)
{
	int opts = fcntl(sockfd, F_GETFL);
	
	if (opts < 0) 
	{
		return -1;
	}
	
	// set nonblocking
	opts |= O_NONBLOCK;
	
	if (fcntl(sockfd, F_SETFL, opts) < 0) 
	{
		return -1;
	}
}

/* ConnectWeb()
 * this function used to create a new socket fd and
 * then connect to host
 */

int ConnectWeb(int& sockfd)
{
	struct sockaddr_in server_addr;

	// create socket
	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
	{
		return -1;
	}
	#ifdef DEBUG
		puts("create socket ok");
	#endif
	
	// initialize server_addr
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr = *((struct in_addr *)Host->h_addr);
	
	// connect to host
	if(connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1)
	{
		perror("connect error");
		return -1;
	}
	#ifdef DEBUG
		puts("connect ok");
	#endif
	
	pthread_mutex_lock(&connlock);
	pending++;
	pthread_mutex_unlock(&connlock);
}

/* SendRequest()
 * this function used to send request to host. tell host
 * what you want to do.
 */

int SendRequest(int sockfd, URL& url_t)
{
	// initialize request
	string request;
	string Uagent = UAGENT, Conn = CONN, Accept = ACCEPT;
	request = "GET /" + url_t.GetFile() + " HTTP/1.1\r\nHost: " + url_t.GetHost() + "\r\nUser-Agent: " + 
			  Uagent + "\r\nAccept: " + Accept + "\r\nConnection: " + Conn + "\r\n\r\n";
  
	// write(send request)
	int d, total = request.length(), send = 0;

	while(send < total)
	{
		if((d = write(sockfd, request.c_str()+send, total-send)) < 0) 
		{
			return -1;
		}
		send += d;
	}
	#ifdef DEBUG
		puts("write in socket ok");
	#endif
}

/* Calc_Time_Sec()
 * this function used to calculate the diffrent time between
 * two time. the time is based on struct timeval:
 *	struct timeval
 *	{
 *		__time_t tv_sec;        // Seconds.
 *		__suseconds_t tv_usec;    // Microseconds. 
 *	};
 */

double Calc_Time_Sec(struct timeval st, struct timeval ed)
{
	double sec = ed.tv_sec - st.tv_sec;
	double usec = ed.tv_usec - st.tv_usec;
	
	return sec + usec/1000000;
}

/* GetResponse()
 * receive the data from host. which will return page information.
 * if ok(get the Web page successfully), it will analyse the page.
 * at last, the function will start new pthread.
 */

void* GetResponse(void *argument)
{
	struct argument *ptr = (struct argument *)argument; // receive the peramter
	struct stat buf;                                    // record the file information
	int timeout;
	int flen;
	int sockfd = ptr->sockfd;
	char ch[2];
	char buffer[1024];                                  // record read buffer from every read() operation
	char headbuffer[2048];                              // record headbuffer from Web Page
	char tmp[MAXLEN];                                   // record the tmp Web Page
	URL url_t = ptr->url;

	*tmp = 0;
	
	// read headbuffer
	int i=0,j=0, n;

	while(1) 
	{
		n = read(sockfd, ch, 1);

		if(n < 0) 
		{
			if(errno == EAGAIN) 
			{
				sleep(1);
				continue;
			}
		}
		if(*ch == '>') 
		{
			headbuffer[j++] = *ch;
			break;
		}
	}
	headbuffer[j] = 0;
	
	// read page context
	int need = sizeof(buffer) - 1;
	timeout = 0;

	while(1) 
	{
		n = read(sockfd, buffer, need);

        // we think that when read() return 0, the Web Page has fetched over
		if(!n) break;
		if(n < 0) 
		{
			if(errno == EAGAIN) 
			{
				timeout++;
				
				if(timeout>10)
				{
				 	//printf("complete or net-speed is not ok. we will stop here\n");
				 	break;
				}
				sleep(1);
				continue;
			}
			else 
			{
				perror("read");
				return NULL;
			}
		}
		else 
		{
			sum_byte += n;
			timeout = 0;
			strncat(tmp, buffer, n);
		}
	}
	close(sockfd); // attention: do not forget close sockfd.
	
	string HtmFile(tmp);
	Analyse(HtmFile);
	flen = HtmFile.length();
	
	chdir("Pages");
	int fd = open(url_t.GetFname().c_str(), O_CREAT|O_EXCL|O_RDWR, 00770);

    /* check whether needs re-fetch */

	if(fd < 0)
	{
		if(errno == EEXIST)
		{
			stat(url_t.GetFname().c_str(), &buf);
			int len = buf.st_size;

			if(len >= flen)
			{
				goto NEXT;
			}
			else
			{
				fd = open(url_t.GetFname().c_str(), O_RDWR|O_TRUNC, 00770);

                if(fd < 0) 
                {
                    perror("file open error");
                    goto NEXT;
                }
			}
		}
		else
		{
			perror("file open error");
			goto NEXT;
		}
	}
	write(fd, HtmFile.c_str(), HtmFile.length());

NEXT:
	
	close(fd); // do not forget to close the fd	
	pthread_mutex_lock(&connlock);
	pending--;
	cnt++;
	
    // calculate how many time has been used
	gettimeofday(&t_ed, NULL);
	time_used = Calc_Time_Sec(t_st, t_ed);

    // print debug infomation
    printf("S:%-6.2fKB  I:%-5dP:%-5dC:%-5d", flen/1024.0, que.size(), pending, cnt);
    printf("[re-]fetch:[%s]->[%s]\n", url_t.GetFile().c_str(), url_t.GetFname().c_str());

	//cout<<"Get:["<<url_t.GetFile()<<"]"<<" --> ["<<url_t.GetFname()<<"]  ";
	//cout<<"size:"<<flen/1024.0<<"KB"<<"  "<<"inq:"<<que.size()<<"  "<<"pending:"<<pending<<"  "<<"completed:"<<cnt+1<<endl;
	//cout<<"  "<<"avg_rate:"<<sum_byte/1024.0/time_used<<"KB/s"<<"  "<<"timec:"<<time_used<<"s"<<endl;
	
	pthread_mutex_unlock(&connlock);

	// judge how many new url in wait-queue can pop()
	if (pending <= 30)
	{
		n = 5;
	}
	else 
	{
		n = 1;	
	}

	for(int i=0; i<n; i++) 
	{
		if(que.empty()) 
		{
            // we think the first url is more import, so give it more time to fetch
			if(is_first_url) 
			{
				is_first_url = false;
				sleep(1);
				continue;
			}
			else 
			{
				break;
			}
		}
		pthread_mutex_lock(&quelock);
		URL url_t = que.front();
		que.pop();
		pthread_mutex_unlock(&quelock);
		
		int sockfd;
		timeout = 0;

        // connect to web 
		while(ConnectWeb(sockfd) < 0 && timeout < 10) 
		{
			timeout++;
		}
        if(timeout>=10) 
        {
            perror("create socket");
            return NULL;
        }
		
        // setnoblock
		if(SetNoblocking(sockfd) < 0) 
		{
			perror("setnoblock");
			return NULL;
		}

        // sendrequest
		if(SendRequest(sockfd, url_t) < 0) 
		{
			perror("sendrequest");
			return NULL;
		}
		
		struct argument *arg=new struct argument;
		struct epoll_event ev;
		arg->url = url_t;
		arg->sockfd = sockfd;
		ev.data.ptr = arg;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
		
		epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
	}
	pthread_exit(NULL);
}
