/* web.cpp
 * this file supports Web Page operations. such as url operations,
 * analysis and modify web pages.
 */

#include "web.h"
#include "debug.h"

extern set<unsigned int>Set;
extern URL url;
extern queue<URL>que;
extern string keyword;
extern pthread_mutex_t quelock;
extern pthread_mutex_t setlock;
const unsigned int MOD = 0x7fffffff;

/* hash()
 * it creates a unsigned int hash-value of a string.
 */

unsigned int hash(const char *s)
{
    unsigned int h = 0, g;
    while(*s)
    {
        h = (h<<4) + *s++;
        g = h & 0xf0000000;
        if(g) h ^= g>>24;
        h &= ~g;
    }

    return h % MOD;
}

/* SetUrl()
 * this functions will analysis url and return a normalize url.
 */

int SetUrl(URL& url_t, string& str)
{	
	string src(str);

	if(src.length() == 0 || src.find('#') != string::npos) return -1;
	int len = src.length();

	while(src[len-1] == '/') // remove the last chracter '/'s
		src[--len] = '\0';
	
	// check whether src start with "http://"
	if(src.compare(0, 7, "http://") == 0) src = src.assign(src, 7, len);
	

	/* below will set url's host, port*/
	int p = src.find_first_of(':');

	if(p != string::npos) // explicit port number
	{
		url_t.SetHost(string(src, 0, p));
		
		int c = p+1, port = 0, len_src = src.length();

		while(isdigit(src[c]) && c<len_src)
			port = port*10 + src[c++] - '0';
		url_t.SetPort(port);
		src = string(src, c, src.length());
	}
	else // implicit port number
	{
		p = src.find_first_of('/'); 

		if(p != string::npos) 
		{
			url_t.SetHost(string(src, 0, p));
			src = string(src, p, src.length());
		}
		else 
			url_t.SetHost(src);
		url_t.SetPort(80);
	}
	
	/* below will set url's file */

	p = src.find_first_of('/');

	if(p != string::npos) 
		url_t.SetFile(src);
	else 
		url_t.SetFile("/");
	
	return 1;
}

/* tolower() 
 * set string to lowercase
 */

void tolower(string& src)
{
	for(int i=0; src[i]; i++)
		if(src[i]>='A' && src[i]<='Z')
			src[i] = src[i]-'A' + 'a';
}

/* Analyse()
 * analyse the Web Page which has fetched
 * in the function, we will get all fitted url and insert into
 * wait-queue(que).
 */

void Analyse(string& src)
{
	int p=0, p1, p2;
	int flag;
	URL url_t;
	string str;
	
	/* search every url with prefix 'href' */

	while((p = src.find("href", p)) != string::npos) 
	{
        int pp = p+4;
        while(src[pp] == ' ') 
            pp++;

        if(src[pp] != '=')
        {
            p++;
            continue;
        }
        else 
        {
            p = pp;
            p1 = pp+1;
        }

		// url can be surrounded by '\"', '\'', ' '
		while(src[p1] == ' ') 
			p1++;

		if(src[p1] == '\"')
			flag = 0;
		else if(src[p1] == '\'')
			flag = 1;
		else 
			flag = 3;

		switch(flag)
		{
			case 0:
				p1++;
				p2 = src.find_first_of('\"', p1);
				break;
			case 1:
				p1++;
				p2 = src.find_first_of('\'', p1);
				break;
			case 3:
				p2 = p1;
				while(src[p2] != ' ' && src[p2] != '>')
					p2++;
		}

		// judge whether contains keyword
		str = string(src, p1, p2-p1);
		
		if(keyword != "" && str.find(keyword) == string::npos)
			continue;
		else 
		{
			// judge whether contains 'http://'
			if(str.find("http://") == string::npos)
			{
				int pos = 1;

				while(str[pos] == '/')
					pos++;
				str = string(str, pos-1);
				str.insert(0, "http://"+url.GetHost());
			}
			else if(str.find(url.GetHost()) == string::npos)
				continue;

			if(SetUrl(url_t, str) < 0) 
				continue;
		}

		//#ifdef DEBUG
		//	cout<<p<<" "<<p1<<" "<<p2<<" "<<url_t.GetFile()<<endl;
		//#endif
		
		// modify the set(Set)
		unsigned int hashVal = hash(url_t.GetFile().c_str());
		char tmp[31]; 

		sprintf(tmp, "%010u", hashVal);
		// judge whether has fetched
		if(Set.find(hashVal) == Set.end()) 
		{ 
			pthread_mutex_lock(&setlock);
			Set.insert(hashVal);
			pthread_mutex_unlock(&setlock);
			url_t.SetFname(string(tmp)+".html");
			pthread_mutex_lock(&quelock);
#ifdef DEBUG
            char info[120];
            sprintf(info, "Add queue: %s", url_t.GetFile().c_str());
            PRINT(info);
#endif
			que.push(url_t);
			pthread_mutex_unlock(&quelock);
			
		}
		src.insert(p1-1, "\""+string(tmp)+".html\" ");
	}
}

