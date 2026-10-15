// win32: declarations the reconstruction of win32/win_input.cpp uses.
//
// Layouts from the PDB; signatures fixed by the original decorated names in the MAP. Original header
// locations not yet confirmed (win_local.h, win_input.h, gamepad and client headers in the IW engine family).
#pragma once

#include <windows.h>

#include "qcommon/dvar.h"

// ---- win32/win_main.cpp -------------------------------------------------------------------------------
struct HINSTANCE__ { int unused; };
struct HHOOK__ { int unused; };

// The PDB has two layouts of WinVars_t: PDB type 0xF689 (36 bytes, with lowLevelKeyboardHook) and 0x8322
// (32 bytes, without it), so translation units were compiled with different versions of the header (an ODR
// violation, records/bugs.tsv). win_input.cpp uses the 36-byte one: it reads sysMsgTime at g_wv+0x20, and the
// next global (marker_win_wndproc) starts 36 bytes after g_wv. Ghidra's types.h has only the 32-byte one.
struct WinVars_t
{
	HINSTANCE__ *reflib_library;   // 0x00
	int reflib_active;             // 0x04
	HWND hWnd;                     // 0x08
	HINSTANCE__ *hInstance;        // 0x0C
	int activeApp;                 // 0x10
	int isMinimized;               // 0x14
	int recenterMouse;             // 0x18
	HHOOK__ *lowLevelKeyboardHook; // 0x1C
	unsigned int sysMsgTime;       // 0x20
};
extern WinVars_t g_wv;

// ---- win32/win_input.cpp ------------------------------------------------------------------------------
struct WinMouseVars_t
{
	int oldButtonState;            // 0x00
	tagPOINT oldPos;               // 0x04
	bool mouseActive;              // 0x0C
	bool mouseInitialized;         // 0x0D
};

extern int g_showCursor;
extern const dvar_s *in_mouse;
extern const dvar_s *gpad_present;

void TRACK_win_input();
void IN_StartupGamepads();
void IN_GamepadsMove();
void IN_RecenterMouse();
void IN_DeactivateWin32Mouse();
int IN_IsForegroundWindow();
void IN_ActivateMouse(int force);
void IN_DeactivateMouse();
void IN_StartupMouse();
void IN_MouseEvent(int mstate);
void IN_ClampMouseMove(tagPOINT *curPos);
void IN_MouseMove();
void IN_SetCursorPos(int x, int y);
void IN_ShowSystemCursor(int show);
void IN_Startup();
void IN_Shutdown();
void IN_Init();
void IN_Activate(int active);
void IN_Frame();
void IN_InitWin32Mouse();

// ---- win32/win_gamepad.cpp ----------------------------------------------------------------------------
enum GamePadStick
{
	GPAD_INVALID = 0,
	GPAD_LX = 0x40000000,
	GPAD_LY = 0x40000001,
	GPAD_RX = 0x40000002,
	GPAD_RY = 0x40000003,
};

enum GamePadButton
{
	GPAD_NONE = 0,
	GPAD_L_TRIG = 0x20000000,
	GPAD_R_TRIG = 0x20000001,
};

struct ButtonToCodeMap_t
{
	GamePadButton padButton;
	int code;
};
extern ButtonToCodeMap_t buttonList[16];

void GPad_InitAll();
void GPad_UpdateAll();
bool GPad_IsActive(int portIndex);
float GPad_GetStick(int portIndex, GamePadStick stick);
float GPad_GetButton(int portIndex, GamePadButton button);
bool GPad_IsButtonPressed(int portIndex, GamePadButton button);
bool GPad_ButtonRequiresUpdates(int portIndex, GamePadButton button);
bool GPad_IsButtonReleased(int portIndex, GamePadButton button);

// ---- win32/win_main.cpp: system events ---------------------------------------------------------------
enum sysEventType_t
{
	SE_NONE = 0,
	SE_KEY = 1,
	SE_CHAR = 2,
	SE_CONSOLE = 3,
};
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr);
int Sys_Milliseconds();

// ---- qcommon -------------------------------------------------------------------------------------------
void Com_Printf(int channel, const char *fmt, ...);
extern "C" void track_static_alloc_internal(void *ptr, int size, const char *name, int type);  // qcommon/mem_track.cpp

// ---- client --------------------------------------------------------------------------------------------
int CL_MouseEvent(int x, int y, int dx, int dy);             // client_mp/cl_input_mp.cpp
void CL_GamepadIgnoreMouse(bool ignore);                     // client/cl_gamepad.cpp (empty in this build)
void CL_GamepadEvent(int portIndex, int physicalAxis, int value, int time);
void CL_GamepadButtonEventForPort(int portIndex, int key, int buttonEvent, unsigned int time, int button);

// ---- gfx_d3d -------------------------------------------------------------------------------------------
extern const dvar_s *r_fullscreen;
