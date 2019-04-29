#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <tchar.h>

int main(int argc, char **argv)
{
	int my_pid = GetCurrentProcessId();
	if (argc != 1) {
		printf("I am child %d\n", my_pid);
		return 100;
	}
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));
	char cmd[128];
	sprintf(cmd, "%s child", argv[0]);

	if (CreateProcess(NULL, cmd, NULL, NULL, false, 0, NULL, NULL,
			  &si, &pi) == 0) {
		printf("error = %d\n", (int) GetLastError());
		exit(-1);
	}
	printf("I am parent %d, child's pid is %d\n", my_pid,
	       (int) pi.dwProcessId);
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exit_code;
	GetExitCodeProcess(pi.hProcess, &exit_code);
	printf("Child exit code %d\n", (int) exit_code);
	return 0;
}
