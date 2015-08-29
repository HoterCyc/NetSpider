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

int SetUrl(URL& url_t, string& url)
{   
    int p;
    string src(url);

    // invalid url
    if(src.length() == 0 || src.find('#') != string::npos) 
        return -1;

    // rm characters after '?'
    if ((p = src.find_first_of('?')) != string::npos)
        src = src.assign(src, 0, p-1);
    
    // check whether src start with "http://" or "//"
    if ((src.compare(0, 7, "http://") == 0) ||
        (src.compare(0, 2, "//") == 0)) 
    {
        int subpos = (src.compare(0, 2, "//") == 0) ? 2 : 7;

        src = src.assign(src, subpos, string::npos);
        if ((p = src.find_first_of('/')) != string::npos)
        {
            url_t.SetHost(string(src, 0, p));
            url_t.SetFile(string(src, p));
        }
        else
        {
            url_t.SetHost(src);
            url_t.SetFile("/");
        }
    }
    else
    {
        url_t.SetFile(src);
    }
    // url_t.SetPort(80);
       
    return 0;
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

/* get_url()
 * get the first url from the src context
 */

string get_url(string& src, int search_pos, int& insert_start_pos, int& insert_end_pos)
{
    int idx;
    string flag; // should be "href" or "src
    char ch_flag; // should be ' or "

    int p1 = src.find(" href", search_pos);
    int p2 = src.find(" src", search_pos);
    
    // set correct idx
    if ((p1 != string::npos) && (p2 != string::npos))
        idx = (p1 < p2) ? p1 : p2;
    else if ((p1 == string::npos) && (p2 == string::npos))
        return "";
    else if (p1 != string::npos)
        idx = p1;
    else
        idx = p2;

    while (src[idx] != '\'' && src[idx] != '\"') idx++; // url is surrounded by ' or "
    insert_start_pos = idx;
    ch_flag = src[idx];

    idx++;
    while (src[idx] != ch_flag) idx++;
    insert_end_pos = idx;
    
    //printf("%d %d %d %d %d\n", p1, p2, insert_start_pos, insert_end_pos, src.length());
    return string(src, insert_start_pos+1, insert_end_pos-insert_start_pos-1);
}

/* Analyse()
 * analyse the Web Page which has fetched
 * in the function, we will get all fitted url and insert into
 * wait-queue(que).
 */

void Analyse(string& src)
{
    int p = 0, insert_start_pos, insert_end_pos; // p is the anchor of searching
    URL url_t;
    string str = "";
    
    while ((str = get_url(src, p, insert_start_pos, insert_end_pos)) != "") 
    {
        p = insert_end_pos;

#ifdef DEBUG
        //PRINT(str.c_str());
#endif
        // judge whether contains keyword
        if(keyword != "" && str.find(keyword) == string::npos)
            continue;
        else 
        {
            if(SetUrl(url_t, str) < 0) 
                continue;
        }

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
            url_t.SetFname(string(tmp));
            pthread_mutex_lock(&quelock);
#ifdef DEBUG
            char info[250];
            sprintf(info, "%-5dAdd queue: %s", que.size(), url_t.GetFile().c_str());
            PRINT(info);
#endif
            que.push(url_t);
            pthread_mutex_unlock(&quelock);
            
        }
        string insert("\"" + string(tmp) + "\" ");
        src.insert(insert_start_pos, insert);

        p = insert_end_pos + insert.length();
    }
    return;
}

