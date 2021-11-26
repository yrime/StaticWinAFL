// DumpWinAFL.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include "windows.h"

#define env_size 100

typedef wchar_t TCHAR;

TCHAR envbuff[env_size];
//TCHAR envbuff[] = L"sdf:sdf";
TCHAR* fuzzid;// [] = L"234";
TCHAR *b;

TCHAR pipe_name[env_size];

HANDLE hpipe;
HANDLE g_hChildStd_OUT_Wr = NULL;

TCHAR* GetWC_(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

int pipe_check(int res, std::wofstream& outfile);

int main(int argc, char* argv[])
{

	STARTUPINFO si = { sizeof(si) };
	si.hStdError = g_hChildStd_OUT_Wr;
	PROCESS_INFORMATION pi;

	TCHAR* prog_name = GetWC_(argv[1]);
	TCHAR prog_arg[env_size];

//	TCHAR* prog_name = GetWC_("\"D:/ПРОЕКТЫ/java-fuzz/winafl/winafl-master/build/bin/Debug/test.exe .cur_input \"");

	std::wofstream outfile("testlogfork.txt");
	outfile << "55 " << prog_name << " --- " << prog_arg<< std::endl;
    std::cout << "Hello!\n";
	GetEnvironmentVariable(TEXT("AFL_STATIC_CONFIG"), envbuff, env_size);
	if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
		printf("afl env static not found\n");
#ifdef RELEASE
		return -2;
#endif // RELEASE
	}
	outfile << envbuff << std::endl;
	fuzzid = wcstok(envbuff, L":", &b);

	wcscpy_s(pipe_name, sizeof(L"\\\\.\\pipe\\afl_pipe_"), L"\\\\.\\pipe\\afl_pipe_");
#ifdef RELEASE
	wcscat_s(pipe_name, fuzzid);
#endif // RELEASE
	outfile << pipe_name << std::endl;

	hpipe = CreateFile(
		pipe_name,   // pipe name
		GENERIC_READ | GENERIC_WRITE,  0,  NULL,  OPEN_EXISTING,  0,  NULL);          // no template file

	if (hpipe == INVALID_HANDLE_VALUE) {
		printf("winafl pipe not found");
#ifdef RELEASE
		return -1;
#endif // RELEASE
	}

	DWORD res = 0;

	if (!CreateProcess(NULL, prog_name,
		NULL, NULL,	FALSE, 0, NULL,	NULL, &si, &pi))
	{
		printf("get lasterr %d\n", GetLastError());
		outfile << prog_name << " wait create pr " << GetLastError() << std::endl;
		return -1;
	}
	printf("ok");
	outfile << prog_name << " work " << std::endl;
	res = WaitForSingleObject(pi.hProcess, INFINITE);
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
	outfile << "wait " << res << std::endl;


	pipe_check(res, outfile);
	outfile.close();
	return 0;
}

int pipe_check(int res, std::wofstream& outfile) {
	int calibrate = 0;
	bool bb = false;
	DWORD Dummy;
	CHAR Command = 0;
	do {
		++calibrate;
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
		outfile << Command << std::endl;

		if (Command != 'F') {
			if (Command == 'Q') {
				printf("Received the quit signal, exiting. \n");
				outfile.close();

				break;
			}
			else {
				printf("Received an unknow command from afl-fuzz, exiting (%.2x)\n", Command);
			}
			outfile.close();
			return -1;
		}
		if (res == 0) {
			WriteFile(hpipe, "K", 1, &Dummy, NULL);
		}
		else {
			DWORD errcode = (DWORD)res;
			WriteFile(hpipe, "C", 1, &Dummy, NULL);
			WriteFile(hpipe, &errcode, sizeof(DWORD), &Dummy, NULL);
		}

		//		break;
	} while (calibrate <= 8);
	return 0;
}