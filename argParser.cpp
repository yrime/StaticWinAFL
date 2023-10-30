#include <algorithm>

#include "argParser.h"

struct OPT opt { NULL, 0, NULL };

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
	{
		return *itr;
	}
	return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}



void retOpt(int argc, char* argv[]) {
	
	if (cmdOptionExists(argv, argv + argc, "-r"))
	{
		opt.command = getCmdOption(argv, argv + argc, "-r");
	}
	if (cmdOptionExists(argv, argv + argc, "-c")) {
		opt.cmdline = 1;
	}
	if (cmdOptionExists(argv, argv + argc, "-d")) { 
		opt.dinamic = getCmdOption(argv, argv + argc, "-d"); 
	}
	if (cmdOptionExists(argv, argv + argc, "-t")) {
		opt.tsleep = atoi(getCmdOption(argv, argv + argc, "-t"));
	}
}