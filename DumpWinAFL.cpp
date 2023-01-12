// DumpWinAFL.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include "windows.h"

#include <atlstr.h>
#include <sstream>

//#define RELEASE

#define env_size 100
#define MAP_SIZE 65536

typedef wchar_t TCHAR;

TCHAR envbuff[env_size];
//TCHAR envbuff[] = L"sdf:sdf";
TCHAR* fuzzid;// [] = L"234";
TCHAR *b;

TCHAR pipe_name[env_size];
TCHAR shm[env_size];

HANDLE hpipe;
HANDLE g_hChildStd_OUT_Wr = NULL;


TCHAR* GetWC_(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

int pipe_check(DWORD res, std::wofstream& outfile);

int main(int argc, char* argv[])
{
	//data for cmdOutput log file
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	HANDLE hfile = CreateFile(L"cmdOutput.txt", FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	STARTUPINFO si = { sizeof(si) };
	g_hChildStd_OUT_Wr = hfile;
	si.hStdInput = NULL;
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdError = g_hChildStd_OUT_Wr;
	si.hStdOutput = g_hChildStd_OUT_Wr;
	PROCESS_INFORMATION pi;

	//open file for DumpWinAfl log
	std::wofstream outfile("testlogfork.txt", std::ios::app);

	//get name fuzzing programm from argv
	WCHAR* prog_name;// = (WCHAR*)malloc((strlen(argv[1]) + 1) * sizeof(WCHAR));
	//check if use cmd args
	if (argc == 3) {
		std::ifstream ifs("OUT\\.cur_input", std::ios::binary);
		std::string content;
		std::ostringstream ss;
		if (ifs.is_open()) {
			ss << ifs.rdbuf();
			content = ss.str();
			outfile << "OUT\\cur_input: " << content.c_str() << std::endl;
		}
		else {
			outfile << "OUT\\cur_input dont open" << std::endl;
		}
		std::string argvstr(argv[1]);
		std::string reg("OUT\\.cur_input");
		argvstr.replace(argvstr.find(reg), reg.length(), content);
		prog_name = StrDupW((std::wstring(argvstr.begin(), argvstr.end())).c_str());
		outfile << "argument: " << prog_name << " --- " << std::endl;
	}
	else {
		prog_name = (WCHAR*)malloc((strlen(argv[1]) + 1) * sizeof(WCHAR));
		prog_name[0] = L'\0';
		wcscat(prog_name, GetWC_(argv[1]));
	}
	
	outfile << "DumpWinAfl: " << prog_name << " --- " << std::endl;

	//get environment varible winafl
	GetEnvironmentVariable(TEXT("AFL_STATIC_CONFIG"), envbuff, env_size);
	if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
		printf("afl env static not found\n");
		return -2;
	}
	outfile << "afl enironment variable: "<<envbuff << std::endl;
	//afl env parse
	fuzzid = wcstok(envbuff, L":", &b);

	wcscpy_s(pipe_name, sizeof(L"\\\\.\\pipe\\afl_pipe_"), L"\\\\.\\pipe\\afl_pipe_");
	wcscpy_s(shm, sizeof(L"afl_shm_"), L"afl_shm_");
	
	wcscat_s(pipe_name, fuzzid);
	wcscat_s(shm, fuzzid);
	outfile << "pipe name: " << pipe_name << std::endl;
	//open afl pipe
	hpipe = CreateFile(
		pipe_name,   // pipe name
		GENERIC_READ | GENERIC_WRITE,  0,  NULL,  OPEN_EXISTING,  0,  NULL);          // no template file

	if (hpipe == INVALID_HANDLE_VALUE) {
		printf("winafl pipe not found");
		return -1;
	}
	//winafl calibrate phase
	DWORD res = 0;
	int calibrate = 0;
	do {
		++calibrate;
		//running fizzing programm
		if (!CreateProcess(NULL, prog_name,
			NULL, NULL,	TRUE, CREATE_NO_WINDOW, NULL,	NULL, &si, &pi))
		{
			printf("get lasterr %d\n", GetLastError());
			outfile << prog_name << " wait create programm " << GetLastError() << std::endl;
			return -1;
		}
		outfile << "prgramm: " << prog_name << " --- is started " << std::endl;
		
		res = WaitForSingleObject(pi.hProcess, INFINITE);
		//outfile << "output from process: " << (TCHAR*)(g_hChildStd_OUT_Wr) << std::endl;
		DWORD result = -1;
		if (!GetExitCodeProcess(pi.hProcess, (LPDWORD)& result))
		{
			outfile <<"GetExitCodeProcess() failed " << GetLastError() << std::endl;
		}
		else
			outfile << "The exit code for " << (LPTSTR)(prog_name) << " " << result << std::endl;
		std::cout << result;
		// Close process and thread handles. 
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(hfile);
		outfile << "wait " << result << std::endl;

		pipe_check(result, outfile);
	} while (calibrate <= 40); //40 c'est magic. sans question!
	outfile.close();

	return 0;
}
// il y a se passer un processus
int pipe_check(DWORD res, std::wofstream& outfile) {
	//int calibrate = 0;
	bool bb = false;
	DWORD Dummy;
	CHAR Command = 0;
	//do {
		//++calibrate;
		if (hpipe == INVALID_HANDLE_VALUE) {
			printf("winafl pipe not found");
			return -1;
		}
		if (bb != true) {
			char c = 'P';
			WriteFile(hpipe, &c, 1, &Dummy, NULL);
		}
		else {
			char c = 'K';
			WriteFile(hpipe, &c, 1, &Dummy, NULL);
		}
		ReadFile(hpipe, &Command, 1, &Dummy, NULL);
		outfile << Command << " " << res<< std::endl;

		if (Command != 'F') {
			if (Command == 'Q') {
				printf("Received the quit signal, exiting. \n");
				outfile.close();
				return 0;
				//break;
			}
			else {
				printf("Received an unknow command from afl-fuzz, exiting (%.2x)\n", Command);
			}
			outfile.close();
			return -1;
		}
		if (res == 0 || res == 1 || res == 2) {
			WriteFile(hpipe, "K", 1, &Dummy, NULL);
		}
		else {
			DWORD errcode = (DWORD)res;
			WriteFile(hpipe, "C", 1, &Dummy, NULL);
			WriteFile(hpipe, &errcode, sizeof(DWORD), &Dummy, NULL);
		}

		//		break;
	//} while (calibrate <= 40); //40 c'est magic. sans question!
	return 0;
}
