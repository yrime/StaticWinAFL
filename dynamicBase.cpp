#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <tlhelp32.h>

struct param_enum
{
	unsigned long ulPID;
	HWND hWnd_out;
};
BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	param_enum& param_data = *(param_enum*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (param_data.ulPID != process_id)
	{
		return TRUE;
	}
	param_data.hWnd_out = handle;

	return FALSE;
}
HWND find_specific_window(unsigned long process_id)
{
	param_enum param_data;
	param_data.ulPID = process_id;
	param_data.hWnd_out = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&param_data);
	//get_window_title(process_id, param_data.hWnd_out);
	return param_data.hWnd_out;
}
bool findProc(PWCHAR proc)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	HANDLE hModuleSnap;
	HANDLE hExeName;
	DWORD processID;
	MODULEENTRY32 me32;
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	while (hRes)
	{
		if (NULL != _tcsstr(pEntry.szExeFile, (PWCHAR)L"cmd.exe"))
		{
			hExeName = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pEntry.th32ProcessID);
			processID = pEntry.th32ProcessID;
			wchar_t* caption = new wchar_t[MAX_PATH * 2];
			HWND h = find_specific_window(processID);
			GetWindowTextW(h, caption, wcslen(proc)+1);
			if (wcscmp(caption, proc) == 0)
			{
				return true;
			}
			printf("%ls", caption);
		}
		
		hRes = Process32Next(hSnapShot, &pEntry);
		
	}
	CloseHandle(hSnapShot);
	return false;
}
bool runSubProc(WCHAR *argv)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));


	// Start the child process.
	if (findProc((PWCHAR)L"C:\\Windows\\system32\\cmd.exe")) {
		return false;
	}
	if (!CreateProcess(NULL,   // No module name (use command line)
		argv,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		CREATE_NEW_PROCESS_GROUP,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return false;
	}
	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}