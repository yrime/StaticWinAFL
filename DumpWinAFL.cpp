// DumpWinAFL.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include "windows.h"

#include <atlstr.h>
#include <regex>
#include <fstream>


//#define RELEASE

#define env_size 100
#define MAP_SIZE 65536

typedef wchar_t TCHAR;

//#ifdef UEFI
	#define MAP_SIZE 65536
	#define BUFSIZE 1024
	#define SHA256LEN 32

	unsigned int prev = 0;

	char* __afl_area_ptr;
	PVOID areaPtr = NULL;
//#endif // UEFI

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

//#ifdef UEFI
BYTE* getFileHash(char* name) {
	HANDLE hFile = NULL;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BOOL bResult = FALSE;
	BYTE rgbFile[BUFSIZE];
	BYTE rgbHash[SHA256LEN];
	DWORD cbRead = 0;
	DWORD cbHash = 0;
	DWORD dwStatus = 0;
	LPCWSTR filename = CA2CT(name);//CA2CT(name);

	hFile = CreateFile(filename,
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		dwStatus = GetLastError();
		return NULL;
	}
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		dwStatus = GetLastError();
		CloseHandle(hFile);
		return NULL;
	}
	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
		dwStatus = GetLastError();
		printf("CryptAcquireContext %d", dwStatus);
		CloseHandle(hFile);
		CryptReleaseContext(hProv, 0);
		return NULL;
	}
	while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
	{
		if (0 == cbRead) {
			break;
		}
		if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
			dwStatus = GetLastError();
			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			CloseHandle(hFile);
			return NULL;
		}
	}
	if (!bResult) {
		dwStatus = GetLastError();
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		CloseHandle(hFile);
		return NULL;
	}
	cbHash = SHA256LEN;
	CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);

	CryptReleaseContext(hProv, 0);
	CryptDestroyHash(hHash);
	CloseHandle(hFile);
	return rgbHash;
}
//#endif

int main(int argc, char* argv[])
{
	std::wofstream outfile("testlogfork2.txt", std::ios::app);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	HANDLE hfile = CreateFile(L"cmdOutput.txt", GENERIC_WRITE,//FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	STARTUPINFO si = { sizeof(si) };
	g_hChildStd_OUT_Wr = hfile;
	si.hStdInput = NULL;
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdError = g_hChildStd_OUT_Wr;
	si.hStdOutput = g_hChildStd_OUT_Wr;
	PROCESS_INFORMATION pi;
	WCHAR* prog_name;
	if (argc == 4) {
		int sit = (int)argv[3] - (int)'0';
		const char* sit_str;
		std::ifstream ifs("OUT\\.cur_input", std::ios::binary);
		std::string content;
		//std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
		if (ifs.is_open()) {
			ifs >> content;
			outfile << "cur_input " << content.c_str() << std::endl;
		}
		else {
			outfile << "dont open cur input" << std::endl;
		}
	//	content.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	//	std::streambuf* rrbuf = ifs.rdbuf();
	//	char* block = new char[1024];
	//	rrbuf->sgetn(block, 1024); 
		//delete[] block;
	//	std::string content((std::istreambuf_iterator<char>(rrbuf)), (std::istreambuf_iterator<char>()));
		std::string argvstr(argv[1]);
		std::string reg("OUT\\.cur_input");
		argvstr.replace(argvstr.find(reg), reg.length(), content);
	//	char* test = NULL;
	//	test = (char*)argvstr.c_str();
	//	argv[1] = (char*)test;
		outfile << "argv[1] " << argv[1] << std::endl;
		prog_name = StrDupW((std::wstring(argvstr.begin(), argvstr.end())).c_str());
		outfile << "4arg " << prog_name << " --- "; // << prog_arg << " !!!";
	}
	else {
		//TCHAR* prog_name = GetWC_(argv[1]);
		prog_name = (WCHAR*)malloc((strlen(argv[1]) + 1) * sizeof(WCHAR));
		prog_name[0] = L'\0';
		wcscat(prog_name, GetWC_(argv[1]));
		//TCHAR prog_arg[env_size];
	//	TCHAR* prog_name = GetWC_("\"D:/ПРОЕКТЫ/java-fuzz/winafl/winafl-master/build/bin/Debug/test.exe .cur_input \"");
	}
	outfile << "67 " << prog_name << " --- "; // << prog_arg << " !!!";
	outfile << "kk " << std::endl;
	//SetEnvironmentVariable(TEXT("PREMIERE_AFL"), L"1");
	GetEnvironmentVariable(TEXT("AFL_STATIC_CONFIG"), envbuff, env_size);
	if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
		outfile << "afl env static not found\n" << std::endl;
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
		
		#ifdef UEFI
	//	outfile << "77 \n"; // << prog_arg << std::endl;

		HANDLE mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, shm);
		areaPtr = MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (areaPtr == NULL) {
			outfile << "shm value failed " << shm << std::endl;
			outfile.close();
		}
		__afl_area_ptr = (char*)areaPtr;

		BYTE* hhash = getFileHash(argv[2]);
		for (int y = 0; y < SHA256LEN / 4; y++) {
			outfile << "hash --- " << ((unsigned int*)hhash)[y] << std::endl;
		}
		unsigned int j = 0;
		prev = 0;
		//(int*)hhash[i]
		//printf(" %X", (int) * (hhash + i * 4));
		__afl_area_ptr[14567345 % MAP_SIZE]++;
		__afl_area_ptr[19999999 % MAP_SIZE]++;
		__afl_area_ptr[14823715 % MAP_SIZE]++;
		__afl_area_ptr[14569014 % MAP_SIZE]++;
		__afl_area_ptr[14567354 % MAP_SIZE]++;

		for (int index = 0; index < SHA256LEN / 4; ++index) {
			outfile << "hash " << ((unsigned int*)hhash)[index] << std::endl;
			j = (prev ^ ((unsigned int*)hhash)[index]);
			j %= MAP_SIZE;
			__afl_area_ptr[j]++;
			prev = ((unsigned int*)hhash)[index] >> 1;
			outfile << "j " << j << "prev" << prev << std::endl;
		}
	/*	for (int l = 0; l < rand() % 15; ++l) {
			j = rand() % MAP_SIZE;
			j %= MAP_SIZE;
			__afl_area_ptr[prev ^ j]++;
			prev = j >> 1;
		}*/
		#endif // UEFI

		res = WaitForSingleObject(pi.hProcess, INFINITE);
		//outfile << "output from process: " << (TCHAR*)(g_hChildStd_OUT_Wr) << std::endl;
		DWORD result = -1;
		if (!GetExitCodeProcess(pi.hProcess, (LPDWORD)& result))
		{
			outfile <<"GetExitCodeProcess() failed " << GetLastError() << std::endl;
		}
		else
			outfile << "The exit code for " << (LPTSTR)(prog_name) << " " << result << std::endl;
		// Close process and thread handles. 
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(hfile);
		outfile << "wait " << result << std::endl;
		outfile << "argv[2] " << argv[2] << std::endl;
		//outfile << "strcat " << strcat(argv[2], ".cov");

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
		DWORD res2 = res & 0xFF000000;
		outfile << "res & " << res2 << std::endl;
		if (res2 == 0) {
			outfile << "pipe out" << " k " << res << std::endl;
			WriteFile(hpipe, "K", 1, &Dummy, NULL);
		}
		else {
			outfile << "pipe out" << " C " << res << std::endl;
			DWORD errcode = (DWORD)res;
			WriteFile(hpipe, "C", 1, &Dummy, NULL);
			WriteFile(hpipe, &errcode, sizeof(DWORD), &Dummy, NULL);
		}

		//		break;
	//} while (calibrate <= 40); //40 c'est magic. sans question!
	return 0;
}
