// Copyright � 1996-2017, Valve Corporation, All rights reserved.

#include "base/include/windows/windows_light.h"
#include "Sys_Utils.h"
#include "EngineInterface.h"
#include "tier0/include/vcrmode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/include/memdbgon.h"

const unsigned int SYS_NO_ERROR = NO_ERROR;
const unsigned int SYS_ERROR_INVALID_HANDLE = ERROR_INVALID_HANDLE;

void Sys_SetLastError(unsigned long error)
{
	::SetLastError(error);
}

unsigned long Sys_GetLastError()
{
	return ::GetLastError();
}


WHANDLE Sys_CreateMutex(const char *mutexName)
{
	return (WHANDLE)::CreateMutex(NULL, FALSE, TEXT(mutexName));
}

void Sys_ReleaseMutex(WHANDLE mutexHandle)
{
	::ReleaseMutex((HANDLE)mutexHandle);
}


const unsigned int SYS_WAIT_OBJECT_0 = WAIT_OBJECT_0;
const unsigned int SYS_WAIT_ABANDONED = WAIT_ABANDONED;

unsigned int Sys_WaitForSingleObject(WHANDLE mutexHandle, int milliseconds)
{
	return VCRHook_WaitForSingleObject((HANDLE)mutexHandle, milliseconds);
}

unsigned int Sys_RegisterWindowMessage(const char *msgName)
{
	return ::RegisterWindowMessage(msgName);
}

WHANDLE Sys_FindWindow(const char *className, const char *windowName)
{
	return (WHANDLE)::FindWindow(className, windowName);
}

void Sys_EnumWindows(void *callbackFunction, int lparam)
{
	::EnumWindows((WNDENUMPROC)callbackFunction, lparam);
}

#ifndef _XBOX
void Sys_GetWindowText(WHANDLE wnd, char *buffer, int bufferSize)
{
	::GetWindowText((HWND)wnd, buffer, bufferSize - 1);
}
#endif

void Sys_PostMessage(WHANDLE wnd, unsigned int msg, unsigned int wParam, unsigned int lParam)
{
	::PostMessageA((HWND)wnd, msg, wParam, lParam);
}

#ifndef _XBOX
void Sys_SetCursorPos(int x, int y)
{
	::SetCursorPos(x, y);
//	engine->SetCursorPos(x,y); // SRC version
}
#endif

#ifndef _XBOX
static ATOM staticWndclassAtom = 0;
static WNDCLASS staticWndclass = { NULL };
#endif

WHANDLE Sys_CreateWindowEx(const char *windowName) { return (WHANDLE)1; }

void Sys_DestroyWindow(WHANDLE wnd) {}

