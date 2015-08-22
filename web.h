#ifndef WEB_H
#define WEB_H

#include <iostream>
#include <cstdio>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <queue>
#include <set>
using namespace std;

/* class of URL
 * it contains URL's constructor, variables, functions.
 * variables are private and functions are public. among them:
 * Host: url's domain name.
 * Port: HTTP port. Default 80.
 * File: Web Page's path.
 * Fname: url's new name(hash code) after rename.
 */

class URL {
	public:
		URL() 
		{
			Host = "";
			Port = 0;
			File = "";
			Fname = "";
		}
		void SetHost(const string& host) { Host = host; }
		string GetHost() { return Host; }
		void SetPort(int port) { Port = port; }
		int GetPort() { return Port; }
		void SetFile(const string& file) { File = file; }
		string GetFile() { return File; }
		void SetFname(const string& fname) { Fname = fname; }
		string GetFname() { return Fname; }
		~URL() {}
	private:
		string Host;
		int Port;
		string File;
		string Fname;
};

unsigned int hash(const char *);
int SetUrl(URL&, string&);
void tolower(string& src);
void Analyse(string&);

#endif


