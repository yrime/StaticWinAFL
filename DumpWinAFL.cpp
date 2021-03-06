// DumpWinAFL.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include "windows.h"

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

DWORD webStart(std::wofstream& outfile) {
	TCHAR envbuff[1];

//	GetEnvironmentVariable(TEXT("PREMIERE_AFL"), envbuff, 1);
//	if (GetLastError() != ERROR_ENVVAR_NOT_FOUND) {
//		printf("PREMIERE_AFL env static not found\n");
//		return 0;
//	}
//	Sleep(10000);
//	SetEnvironmentVariable(TEXT("PREMIERE_AFL"), L"1");
	STARTUPINFO si_c = { sizeof(si_c) };
	//g_hChildStd_OUT_Wr = hfile;
	si_c.hStdInput = NULL;
	//si_c.dwFlags |= STARTF_USESTDHANDLES;
	//si_c.hStdError = g_hChildStd_OUT_Wr;
	//si_c.hStdOutput = g_hChildStd_OUT_Wr;
	PROCESS_INFORMATION pi_c;
	outfile << " in supprocess\n" << GetLastError() << std::endl;
	WCHAR* prog_name = (WCHAR*)malloc((strlen("java -javaagent:jAsmAgent-1.0-jar-with-dependencies.jar -jar web.jar") + 1) * sizeof(WCHAR));
	if (!CreateProcess(NULL, prog_name,
		NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si_c, &pi_c))
	{
		printf("get lasterr %d\n", GetLastError());
		outfile << "supprocess err\n" << GetLastError() << std::endl;
		return -1;
	}
	outfile << "supprocess start\n" << GetLastError() << std::endl;
	
	return 0;
}

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

	//TCHAR* prog_name = GetWC_(argv[1]);
	WCHAR* prog_name = (WCHAR*)malloc((strlen(argv[1]) + 1) * sizeof(WCHAR));
	prog_name[0] = L'\0';
	wcscat(prog_name, GetWC_(argv[1]));
	TCHAR prog_arg[env_size];

//	TCHAR* prog_name = GetWC_("\"D:/ПРОЕКТЫ/java-fuzz/winafl/winafl-master/build/bin/Debug/test.exe .cur_input \"");

	std::wofstream outfile("testlogfork.txt", std::ios::app);
	outfile << "55 " << prog_name << " --- " << prog_arg<< std::endl;
    std::cout << "Hello!\n";
	//SetEnvironmentVariable(TEXT("PREMIERE_AFL"), L"1");
	GetEnvironmentVariable(TEXT("AFL_STATIC_CONFIG"), envbuff, env_size);
	if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
		printf("afl env static not found\n");
		return -2;
	}
	outfile << envbuff << std::endl;
	fuzzid = wcstok(envbuff, L":", &b);

	wcscpy_s(pipe_name, sizeof(L"\\\\.\\pipe\\afl_pipe_"), L"\\\\.\\pipe\\afl_pipe_");
	wcscpy_s(shm, sizeof(L"afl_shm_"), L"afl_shm_");
	outfile << "release" << std::endl;
	wcscat_s(pipe_name, fuzzid);
	wcscat_s(shm, fuzzid);
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
	int calibrate = 0;
	do {
		++calibrate;
		if (!CreateProcess(NULL, prog_name,
			NULL, NULL,	TRUE, CREATE_NO_WINDOW, NULL,	NULL, &si, &pi))
		{
			printf("get lasterr %d\n", GetLastError());
			outfile << prog_name << " wait create pr " << GetLastError() << std::endl;
			return -1;
		}
		outfile << prog_name << " work " << std::endl;
		

		res = WaitForSingleObject(pi.hProcess, INFINITE);
		webStart(outfile);
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
