#ifndef __ARG_PARSER__
#define __ARG_PARSER__
#endif

#include <iostream>
void retOpt(int argc, char* argv[]);

struct OPT
{
	char *command;
	char* dinamic;
	int cmdline;
	int tsleep;
};

extern struct OPT opt;
