/* spider.cpp
 * this file is the main entrance of the program which contains
 * main() function. It also defines global variables. It show us
 * the main frame of the program.
 */

#include "http.h"
#include "config.h"

/* global data definition
 */

struct epoll_event events[31];	// return events which will be dealed with.
struct timeval t_st, t_ed;		// store time value. used to calculate avg-speed.
set<unsigned int>Set;			// the 'hash-table'.
URL url;						// first url we set.
queue<URL>que;					// url wait-queue.
int cnt;						// record how many urls we have fetched.
int sum_byte;					// record how many bytes we have fetched.
int pending;					// record pending urls that wait to deal with 
int epfd;						// record the epoll fd.
bool is_first_url;				// judge whether is the first url.
double time_used;               // record totally time costed.
pthread_mutex_t quelock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t setlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connlock = PTHREAD_MUTEX_INITIALIZER;
string input;
string HtmFile;
string keyword;

/* init()
 * initialize global data
 */

void init()
{
	cnt = 0;
	sum_byte = 0;
	pending = 0;
	is_first_url = true;
	input = START_URL;
	keyword = KEYWORD;
    time_used = 0;
}

/* Usage()
 * used to print the usage of this spider
 */

void Usage()
{
    printf("usage\n");
    printf("-h    --help    print the usage of this spider.                 default    0\n");
    printf("-n    --nurl    set the number of url you want to get.          default    INF\n");
    printf("-u    --url     set the first url you want to start fetch.      default    \"\"\n");
    printf("-k    --key     set the key word of the url to fetch web pages. default    \"\"\n");
	printf("-t    --timeout set timeout for epoll(waitque is empty).        default    20\n");
    printf("\nexample\n");
    printf("./spider -h\n");
    printf("./spider -n 1000 -u http://hi.baidu.com/xxx -k xxx\n");
    printf("./spider --url http://hi.baidu.com/xxx --key xxx\n");
    printf("./spider --url http://hi.baidu.com/xxx -t 15\n");
}

/* Parse()
 * It call getopt_long() function to parse command line arguments.
 */

int Parse(int argc, char **argv) 
{
    int c;
    int ret = 0;
    while((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
    {
        switch(c)
        {
            case 'h':
                return -2;
            case 'n':
                MAX_URL = atoi(optarg);
                break;
            case 'u':
                START_URL = string(optarg);
                break;
            case 'k':
                KEYWORD = string(optarg);
                break;
			case 't':
				TIMEOUT = atoi(optarg);
				break;
            default:
                return -1;
        }
		ret ++;
    }
    return ret;
}

/* main()
 * main entrance. deal with the first url and create new pthread.
 */

int main(int argc, char **argv)
{
	int timeout;

    // argument num error
    if(argc<1) 
    {
        printf("command argument error!\n");
        exit(1);
    }

    // parse argument by call Parse() function.
    int rec = Parse(argc, argv);
    if(rec == -1 || rec == 0) 
    {
        printf("command argument error!\n");
        Usage();
        exit(1);
	} 
	else if(rec == -2) 
	{
		Usage();
		exit(1);
	}

    // initialize global data
	init();
	
	// get normalize of the first url(url).
	if(SetUrl(url, input) < 0) 
	{
		puts("input url error");
	}
	
	// get host by name(do only once in the whole program)
	if(GetHostByName(url.GetHost()) < 0)
	 {
		printf("gethostbyname error\n");
		exit(1);
	}
	
	cout<<"start fetching url: "<<url.GetHost()<<url.GetFile()<<endl;
	
    // prepare for the first url. calculate hash-code for it.
	unsigned int hashVal = hash(url.GetFile().c_str());
	char tmp[31];
	
	sprintf(tmp, "%010u", hashVal);
	url.SetFname(string(tmp)+".html");
	que.push(url);
		
	pthread_mutex_lock(&setlock);
	Set.insert(hashVal);
	pthread_mutex_unlock(&setlock);
	
    // create epoll fd(epfd) with maximum events 50
	epfd = epoll_create(50);

	int n = (que.size()>=30) ? 30 : que.size(); // in this program, n is always 1.
	gettimeofday(&t_st, NULL);

	for(int i=0; i<n; i++) 
	{
		pthread_mutex_lock(&quelock);
		URL url_t = que.front();
		que.pop();
		pthread_mutex_unlock(&quelock);
		
		int sockfd;
		
		// connect to web
		timeout = 0;
		while(ConnectWeb(sockfd) < 0 && timeout < 10) 
		{
			timeout++;
		}
		if(timeout>=10) 
		{
			perror("create socket");
			exit(1);
		}
		
		// setnoblock
		if(SetNoblocking(sockfd) < 0) 
		{
			perror("setnoblock");
			exit(1);
		}
		
		// sendrequest
		if(SendRequest(sockfd, url_t) < 0) 
		{
			perror("sendrequest");
			exit(1);
		}
		
        // initialize argument that the event want to pass to called function
		struct argument *arg=new struct argument;
		struct epoll_event ev;
		arg->url = url_t;
		arg->sockfd = sockfd;
		ev.data.ptr = arg;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

        // enroll event by EPOLL_CTL_ADD mode
		if(epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) 
		{ 
			perror("epoll_ctl_add");
			exit(1);
		}
	}

    /* this dead loop below deal with all events.(to every event, it will create a
     * new thread to execute).
     */

	timeout = 0;
	while(1) 
	{
        // max num of url we want to fetch
        if(cnt>=MAX_URL) 
        {
            break;
        }

        // get n available events to array of events 
		int n = epoll_wait(epfd, events, 30, 2000);
		
        // set timeout
		if(timeout >= TIMEOUT) 
		{
			printf("wait time out for %d times\n", TIMEOUT);
			break;
		}
		if(n<0) 
		{
			perror("epoll_wait_error");
			break;
		}
		else if(n==0 && que.empty()) 
		{
			timeout++;
			continue;
		}
		timeout = 0;
		
		// create pthread for every event and call GetResponse().
		for(int i=0; i<n; i++) 
		{
			struct argument* arg = (struct argument*)events[i].data.ptr;
			
			pthread_attr_t pAttr;
			pthread_t thread;
		
			pthread_attr_init(&pAttr);										// initialize pthread attribute
			pthread_attr_setstacksize(&pAttr, 8 * 1024 * 1024);				// 1M stack size
			pthread_attr_setdetachstate(&pAttr, PTHREAD_CREATE_DETACHED);	// or JOINABLE

			int r = pthread_create(&thread, &pAttr, GetResponse, arg);

			if(r != 0 ) 
			{
				perror("thread create failed");
				continue;
			}

            // destroy pthread attribute
			pthread_attr_destroy(&pAttr);

			struct epoll_event ev;
		    
			// delete used event with sockfd = arg->sockfd.
			if(epoll_ctl(epfd, EPOLL_CTL_DEL, arg->sockfd, &ev) == -1) 
			{
				epoll_ctl(epfd, EPOLL_CTL_MOD, arg->sockfd, &ev);
				perror("epoll_ctl_del");
				continue;
			}
		}
	}
    
    // get current date & time
    struct tm *p;
    time_t ti;
    time(&ti);
    p = gmtime(&ti);
    gettimeofday(&t_ed, NULL);
    time_used = Calc_Time_Sec(t_st, t_ed);

    // print summary infomations
    printf("\n                           STATISTICS\n");
    printf("----------------------------------------------------------------------\n");
    printf("fetch target:           %s\n", input.c_str());
    printf("totally urls fetched:   %d\n", cnt);
    printf("totally byte fetched:   %.2fKB\n", sum_byte/1024.0);
    printf("totally time cost:      %.2fs\n", time_used);
    printf("average download speed: %.2fKB/s\n", sum_byte/1024.0/(time_used-TIMEOUT));
    printf("auther:                 xxx\n");
    printf("date:                   %04d-%02d-%02d\n",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday);  
    printf("time:                   %02d:%02d:%02d\n", (p->tm_hour+8)%24, p->tm_min, p->tm_sec);  
    printf("----------------------------------------------------------------------\n");

	close(epfd);
}
