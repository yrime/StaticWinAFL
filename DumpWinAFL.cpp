﻿// DumpWinAFL.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include "windows.h"

#include <atlstr.h>
#include <sstream>

#include "dynamicBase.h"
#include "argParser.h"

//#define RELEASE

#define env_size 100
#define MAP_SIZE 65536

//typedef wchar_t TCHAR;

WCHAR envbuff[env_size];
//TCHAR envbuff[] = L"sdf:sdf";
WCHAR* fuzzid;// [] = L"234";
WCHAR *b;

WCHAR pipe_name[env_size];
WCHAR shm[env_size];

HANDLE hpipe;
HANDLE g_hChildStd_OUT_Wr = NULL;

std::wofstream outfile;

TCHAR* GetWC_(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

WCHAR* check_args() {
	WCHAR* prog_name;
	if (opt.cmdline) {
		std::ifstream ifs("OUT\\.cur_input", std::ios::binary);
		std::string content;
		std::ostringstream ss;
		if (ifs.is_open()) {
			ss << ifs.rdbuf();
			content = ss.str().substr(0, 4000);
			outfile << "OUT\\cur_input: " << content.c_str() << std::endl;
		}
		else {
			outfile << "OUT\\cur_input dont open" << std::endl;
		}
		ifs.close();
		std::string argvstr(opt.command);
		std::string reg("OUT\\.cur_input");
		argvstr.replace(argvstr.find(reg), reg.length(), content);
		prog_name = StrDupW((std::wstring(argvstr.begin(), argvstr.end())).c_str());
		outfile << "argument: " << prog_name << " --- " << std::endl;
	}
	else {
		prog_name = (WCHAR*)malloc((strlen(opt.command) + 1) * sizeof(WCHAR));
		prog_name[0] = L'\0';
		wcscat(prog_name, GetWC_(opt.command));
	}
	return prog_name;
}

int pipe_check(DWORD res, std::wofstream& outfile);

int main(int argc, char* argv[])
{
	
	retOpt(argc, argv);

	//std::cout << opt.cmdline << " " << opt.command << " " << opt.dinamic << std::endl;

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
	outfile.open("testlogfork.txt", std::ios::app);

	//get name fuzzing programm from argv
	WCHAR* prog_name;// = (WCHAR*)malloc((strlen(argv[1]) + 1) * sizeof(WCHAR));

	//get environment varible winafl
	GetEnvironmentVariable(TEXT("AFL_STATIC_CONFIG"), envbuff, env_size);
	if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
		printf("afl env static not found\n");
		return -2;
	}
	//wcscpy_s(envbuff, sizeof(L"2345"), L"2345");
	if (opt.dinamic != NULL) {
		WCHAR * runDyn;
		WCHAR runCmdDin[500];
		std::string argvdin(opt.dinamic);
		runDyn = StrDupW((std::wstring(argvdin.begin(), argvdin.end())).c_str());
		wcscpy_s(runCmdDin, sizeof(L"cmd.exe /c start cmd.exe \"/k set AFL_STATIC_CONFIG="), 
			L"cmd.exe /c start cmd.exe \"/k set AFL_STATIC_CONFIG=");
		wcscat_s(runCmdDin, envbuff);
		wcscat_s(runCmdDin, L"&");
		wcscat_s(runCmdDin, runDyn);
		wcscat_s(runCmdDin, L"\"");
		if (runSubProc(runCmdDin)) {
			if (opt.tsleep) {
				outfile << "on sleep " << std::endl;
				Sleep(opt.tsleep);
			}
			outfile << "on run" << std::endl;
		}
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
		//check if use cmd args
		prog_name = check_args();
		outfile << "DumpWinAfl: " << prog_name << " --- " << std::endl;
		//run fizzing programm
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
//	do {
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
#ifndef UEFI
		if (res == 0) {
#else
		if ((res & 0xFF000000) == 0) {
#endif
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
