/* http.cpp
 * this file supports TCP socket operations. such as gethostbyname()
 * create socket etc.
 */

#include "http.h"
#include "debug.h"

extern set<unsigned int>Set;
extern URL url; 
extern queue<URL>que;
extern struct epoll_event events[31];
extern int epfd;
extern int cnt;
extern int sum_byte;
extern int pending;
extern int MAX_URL;
extern bool is_first_url;
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

	return 0;
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

    return 0;
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
		return -1;

	#ifdef DEBUG
        //PRINT("create socket success");
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
		//PRINT("connect socket success");
	#endif
	
	pthread_mutex_lock(&connlock);
	pending++;
	pthread_mutex_unlock(&connlock);

    return 0;
}

/* SendRequest()
 * this function used to send request to host. tell host
 * what you want to do.
 */

int SendRequest(int sockfd, URL& url_t)
{
	// initialize request
	string request;
    string url = "http://" + url_t.GetHost() + url_t.GetFile();
    //string url = "http://www.baidu.com/more";
	string Uagent = UAGENT;
    string Conn = CONN;
    string Accept = ACCEPT;

    cout << url << endl;

    request = "GET " + url + " HTTP/1.1\r\n" +
              "Host: " + url_t.GetHost() + "\r\n" +
              "User-Agent: " + Uagent + "\r\n" +
              "Accept: " + Accept + "\r\n" +
              "Connection: " + Conn + "\r\n\r\n";

    //char req[250];
    //sprintf(req, "%s %s HTTP/1.1\r\nAccept: %s\r\nHost: %s\r\nConnection: %s\r\n\r\n", 
    //                           "GET", "http://www.baidu.com/more/", "html/text", "www.baidu.com", "Close"); 
    //request = string(req);
	#ifdef DEBUG
        const char *info = request.c_str();
        PRINT(info);
	#endif
  
	// write(send request)
	int d, total = request.length(), send = 0;

	while(send < total)
	{
		if((d = write(sockfd, request.c_str()+send, total-send)) < 0) 
			return -1;
		send += d;
	}

	#ifdef DEBUG
		//PRINT("Req sent success");
	#endif

    return 0;
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

/* read_and_parse_header(sockfd)
 * parse HTML header and return content length
 */

int read_and_parse_header(int sockfd)
{
    char line_buf[255];
    char ch[2];
    int  n;
    int  retrytimes = 0;
    int  content_length = -1;
    bool resp_success = false; 

    while(1)
    {
        int i=0;
        memset(line_buf, 0, sizeof(line_buf));
        // read line
        while(1)
        {
            n=read(sockfd, ch, 1);
            if (n < 0 && errno == EAGAIN)
            {
                PRINT("warning: errno=EAGAIN");
                if (++retrytimes >=5)
                    return -1;
                else
                {
                    sleep(1);
                    continue;
                }
            }

            line_buf[i++] = ch[0];
            if (ch[0] == '\n')
                break;
        }

#ifdef DEBUG
        PRINT(line_buf);
#endif
        if (strcmp(line_buf, "\r\n") == 0)
            break;

        if (strcmp("HTTP/1.1 200 OK\r\n", line_buf) == 0 ||
            strcmp("HTTP/1.1 301 Moved Permanently\r\n", line_buf) == 0)
            resp_success = true;

        if (strncmp("Content-Length", line_buf, 14) == 0)
            sscanf(line_buf, "Content-Length: %d", &content_length);
    }

    return (resp_success == true) ? content_length : -1;
}

/* GetResponse()
 * receive the data from host. which will return page information.
 * if ok(get the Web page successfully), it will analyse the page.
 * at last, the function will start new pthread.
 */

void* GetResponse(void *argument)
{
	struct argument *arg = (struct argument *)argument; // receive the parameter
	struct stat file_stat;                              // record the file information
	int timeout = 0;
	int flen;
	int sockfd = arg->sockfd;
    int n, len, recv_len;
    int fd;
	char buffer[1024];                                  // record read buffer from every read() operation
	char tmp[MAXLEN]={0};                               // record the tmp Web Page

	URL url_t = arg->url;
	int content_length = read_and_parse_header(sockfd);

#ifdef DEBUG
    char info[200];
    sprintf(info, "expected content_length: %d", content_length);
    PRINT(info);
#endif

    // skip read the content since the GET request is failed
    if (content_length < 0)
    {
        close(sockfd);
        goto NEXT;
    }

	// read size of len everytime from page context
	len = sizeof(buffer) - 1;
    recv_len = 0;

	while(1) 
	{
        // 0 indicate read complete
		n = read(sockfd, buffer, len);

        if (n == 0) // (TODO n will not = 0 when complete reading)
            break;

		if(n < 0) 
		{
			if(errno == EAGAIN) 
			{
                PRINT("warning: errno=EAGAIN");
				sleep(1);
				continue;
			}
			else 
			{
				perror("read");
                close(sockfd);
				goto NEXT;
			}
		}
		else 
		{
			sum_byte += n; // for summery
            memcpy(tmp+recv_len, buffer, n);
            recv_len += n;
			//strncat(tmp, buffer, n);

            //printf("xxxx: %d %d %d\n", n, recv_len, strlen(tmp));
            // complete  
            if (recv_len >= content_length)
                break;
		}
	}
	close(sockfd); // attention: do not forget close sockfd.

#ifdef DEBUG
    sprintf(info, "received content_length: %d", recv_len);
    PRINT(info);
#endif

    if (recv_len < content_length)
        goto NEXT;

    if (url_t.GetFileType() == "" || 
        url_t.GetFileType() == ".html" ||
        url_t.GetFileType() == ".htm")
    {
        string html_content = string(tmp);
        Analyse(html_content);
    }

	chdir("Pages");
	fd = open(url_t.GetFname().c_str(), O_CREAT|O_EXCL|O_RDWR, 00770);

	if (fd < 0 && errno != EEXIST)
	{
        //stat(url_t.GetFname().c_str(), &file_stat);
        perror("file open error");
	}
    else 
    {
        write(fd, tmp, content_length);
        close(fd);	
    }

NEXT:

	pthread_mutex_lock(&connlock);
	pending--;
    
    if (content_length > 0)
    {
        cnt++;
        // print debug infomation
        printf("S:%-6.2fKB  I:%-5dP:%-5dC:%-5d", recv_len/1024.0, que.size(), pending, cnt);
        printf("Fetch: [%s] -> [%s]\n", url_t.GetFile().c_str(), url_t.GetFname().c_str());
    }
	
	pthread_mutex_unlock(&connlock);

	// judge how many new url in wait-queue can pop()
	if (pending <= 30)
		n = 5;
	else 
		n = 1;	

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
				break;
		}
		pthread_mutex_lock(&quelock);
		URL url_t = que.front();
		que.pop();
		pthread_mutex_unlock(&quelock);
		
		int sockfd;
		timeout = 0;

        // connect to web 
		while(ConnectWeb(sockfd) < 0 && timeout < 10) 
			timeout++;

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
