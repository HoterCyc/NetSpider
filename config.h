#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <getopt.h>

const int INF = 0x7fffffff;
const int MAXLEN = 500000;
const char* const short_opt = "hn:u:k:t:";
string START_URL = "http://hi.baidu.com/shouzhewei/home";// "http://blog.csdn.net/yuanmeng001"; 
string KEYWORD =  ""; 
//int MAX_URL = INF;
int MAX_URL = 10;
int TIMEOUT = 5;
const struct option long_opt[] = 
{
    {"help", 0, NULL, 'h'},
    {"nurl", 1, NULL, 'n'},
    {"url", 1, NULL, 'u'},
    {"key", 1, NULL, 'k'},
    {"timeout", 1, NULL, 't'},
    {0, 0, 0, 0}
};

/* config.h
 * which contains global static data.
 */
#endif
