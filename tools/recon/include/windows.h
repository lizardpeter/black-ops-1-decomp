// The part of <windows.h> the reconstruction uses.
//
// Build environment, not game source. Declarations match the Win32 SDK, so calls go through the import
// table as in the original (__imp__Name@N). In the hybrid build those import slots are the original
// binary's, and the differential harness points them at its window model (tests/differential/harness.py).
#pragma once

typedef int BOOL;
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef long LONG;
typedef unsigned long DWORD;

struct HWND__ { int unused; };
typedef struct HWND__ *HWND;

typedef struct tagPOINT
{
	LONG x;
	LONG y;
} POINT, *LPPOINT;

typedef struct tagRECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *LPRECT;

#define WINAPI __stdcall
#define WINUSERAPI extern "C" __declspec(dllimport)

#define WM_LBUTTONDOWN 0x0201
#define MK_LBUTTON 0x0001

WINUSERAPI BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect);
WINUSERAPI BOOL WINAPI GetCursorPos(LPPOINT lpPoint);
WINUSERAPI BOOL WINAPI SetCursorPos(int X, int Y);
WINUSERAPI HWND WINAPI GetForegroundWindow(void);
WINUSERAPI BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint);
WINUSERAPI BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint);
WINUSERAPI int WINAPI ShowCursor(BOOL bShow);
WINUSERAPI BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
