//////////////////////////////////////////////////////////////////////////////
//
//  Detours Test Program (simple.cpp of simple.dll)
//
//  Microsoft Research Detours Package, Version 3.0.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This DLL will detour the Windows SleepEx API so that TimedSleep function
//  gets called instead.  TimedSleepEx records the before and after times, and
//  calls the real SleepEx API through the TrueSleepEx function pointer.
//
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "detours.h"
#pragma comment(lib, "Ws2_32.lib")

static LONG dwSlept = 0;

static SOCKET (WINAPI * true_accept)(SOCKET s, struct sockaddr *addr, int *addrlen) = accept;
static DWORD (WINAPI * TrueSleepEx)(DWORD dwMilliseconds, BOOL bAlertable) = SleepEx;
//setdll.exe -d:simple32.dll server.exe
SOCKET WINAPI my_accept(SOCKET s, struct sockaddr *addr, int *addrlen)
{
	SOCKET ret = true_accept(s, addr, addrlen);

	struct sockaddr_in* saddr = (struct sockaddr_in*)addr;
    char *connected_ip = inet_ntoa(saddr->sin_addr);
	printf("accept tcp connection from: %s", connected_ip);

	FILE * fp = fopen("c:/clients.txt", "a+");
	fprintf(fp, "%s\n", connected_ip);
	fclose(fp);

	return ret;
}

DWORD WINAPI TimedSleepEx(DWORD dwMilliseconds, BOOL bAlertable)
{
    DWORD dwBeg = GetTickCount();
    DWORD ret = TrueSleepEx(dwMilliseconds, bAlertable);
    DWORD dwEnd = GetTickCount();

    InterlockedExchangeAdd(&dwSlept, dwEnd - dwBeg);

    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
        DetourRestoreAfterWith();

        printf("simple" DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
               " Starting.\n");
        fflush(stdout);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)true_accept, my_accept);
        error = DetourTransactionCommit();

        if (error == NO_ERROR) {
            printf("simple" DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
                   " Detoured SleepEx().\n");
        }
        else {
            printf("simple" DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
                   " Error detouring SleepEx(): %d\n", error);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)true_accept, my_accept);
        error = DetourTransactionCommit();

        printf("simple" DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
               " Removed SleepEx() (result=%d), slept %d ticks.\n", error, dwSlept);
        fflush(stdout);
    }
    return TRUE;
}

//
///////////////////////////////////////////////////////////////// End of File.






