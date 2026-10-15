// win32/win_input.cpp
//
// Reconstruction of codsrc/src/win32/win_input.cpp, in original line order.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv.
//
// Mouse input is the cursor-based kind: every frame IN_MouseMove reads the cursor position, takes the
// difference from the last position as the movement and puts the cursor back in the window centre. No raw
// input, so Windows pointer speed and "enhance pointer precision" apply to aiming (records/bugs.tsv).
// This dedicated-server build compiles some calls out (IN_InitWin32Mouse, CL_GamepadIgnoreMouse are empty,
// and IN_ActivateMouse has no cursor capture); the retail client keeps them.
#define T5_SOURCE "win32/win_input.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\win32\\win_input.cpp"  // __FILE__ in the original build

#include "win32/win_local.h"

#include <t5_assert.h>
#include <t5_compiler.h>
#include <t5_static.h>

T5_STATIC WinMouseVars_t s_wmv T5_AT("", "s_wmv");
T5_STATIC int window_center_x T5_AT("", "window_center_x");
T5_STATIC int window_center_y T5_AT("", "window_center_y");
T5_STATIC int in_appactive T5_AT("", "in_appactive");

// 007D8760  win_input.cpp:43
void TRACK_win_input()
{
	track_static_alloc_internal(&s_wmv, sizeof(s_wmv), "s_wmv", 11);
}

// 007D8780  win_input.cpp:69
void IN_StartupGamepads()
{
	GPad_InitAll();
}

// 007D8B50  win_input.cpp:120
// The stick and trigger values are scaled on the x87 stack and truncated (_ftol2_sse) without being
// rounded to float first.
X87_MATH void IN_GamepadsMove()
{
	bool gpadPresent;
	int controllerIndex;
	unsigned int countButtons;
	int time;
	int lx, ly, rx, ry;
	int leftTrig, rightTrig;
	unsigned int butIndex;

	gpadPresent = false;
	GPad_UpdateAll();
	countButtons = sizeof(buttonList) / sizeof(buttonList[0]);
	time = Sys_Milliseconds();
	for (controllerIndex = 0; controllerIndex < 1; controllerIndex++)
	{
		if (!GPad_IsActive(controllerIndex))
			continue;
		gpadPresent = true;
		lx = (int)(GPad_GetStick(controllerIndex, GPAD_LX) * 65535.0f);
		ly = (int)(GPad_GetStick(controllerIndex, GPAD_LY) * 65535.0f);
		rx = (int)(GPad_GetStick(controllerIndex, GPAD_RX) * 65535.0f);
		ry = (int)(GPad_GetStick(controllerIndex, GPAD_RY) * 65535.0f);
		leftTrig = (int)(GPad_GetButton(controllerIndex, GPAD_L_TRIG) * 65535.0f);
		rightTrig = (int)(GPad_GetButton(controllerIndex, GPAD_R_TRIG) * 65535.0f);
		CL_GamepadEvent(controllerIndex, 0, rx, time);   // GPAD_PHYSAXIS_RSTICK_X
		CL_GamepadEvent(controllerIndex, 1, ry, time);   // GPAD_PHYSAXIS_RSTICK_Y
		CL_GamepadEvent(controllerIndex, 2, lx, time);   // GPAD_PHYSAXIS_LSTICK_X
		CL_GamepadEvent(controllerIndex, 3, ly, time);   // GPAD_PHYSAXIS_LSTICK_Y
		CL_GamepadEvent(controllerIndex, 5, leftTrig, time);   // GPAD_PHYSAXIS_LTRIGGER
		CL_GamepadEvent(controllerIndex, 4, rightTrig, time);  // GPAD_PHYSAXIS_RTRIGGER
		for (butIndex = 0; butIndex < countButtons; butIndex++)
		{
			if (GPad_IsButtonPressed(controllerIndex, buttonList[butIndex].padButton))
				CL_GamepadButtonEventForPort(controllerIndex, buttonList[butIndex].code, 1, time, buttonList[butIndex].padButton);
			else if (GPad_ButtonRequiresUpdates(controllerIndex, buttonList[butIndex].padButton))
				CL_GamepadButtonEventForPort(controllerIndex, buttonList[butIndex].code, 2, time, buttonList[butIndex].padButton);
			else if (GPad_IsButtonReleased(controllerIndex, buttonList[butIndex].padButton))
				CL_GamepadButtonEventForPort(controllerIndex, buttonList[butIndex].code, 0, time, buttonList[butIndex].padButton);
		}
	}
	if (gpad_present && gpad_present->current.enabled != gpadPresent)
		Dvar_SetBool(gpad_present, gpadPresent);
}

// 007D8790  win_input.cpp:218
void IN_RecenterMouse()
{
	RECT window_rect;

	GetWindowRect(g_wv.hWnd, &window_rect);
	window_center_x = (window_rect.right + window_rect.left) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;
	SetCursorPos(window_center_x, window_center_y);
}

// 007D88C0  win_input.cpp:234
void IN_DeactivateWin32Mouse()
{
	IN_ShowSystemCursor(1);
}

// 007D87E0  win_input.cpp:257
int IN_IsForegroundWindow()
{
	return GetForegroundWindow() == g_wv.hWnd;
}

// 007D8800  win_input.cpp:329
void IN_ActivateMouse(int force)
{
	if (!s_wmv.mouseInitialized)
		return;
	iassert_at(332, r_fullscreen);
	if (!in_mouse->current.enabled)
	{
		s_wmv.mouseActive = false;
		return;
	}
	if (!force && s_wmv.mouseActive)
		return;
	if (IN_IsForegroundWindow())
		s_wmv.mouseActive = true;
	else
		s_wmv.mouseActive = false;
}

// 007D8890  win_input.cpp:366
void IN_DeactivateMouse()
{
	if (!s_wmv.mouseInitialized || !s_wmv.mouseActive)
		return;
	s_wmv.mouseActive = false;
	IN_DeactivateWin32Mouse();
}

// 007D88D0  win_input.cpp:383
void IN_StartupMouse()
{
	s_wmv.mouseInitialized = false;
	if (!in_mouse->current.enabled)
	{
		Com_Printf(16, "Mouse control not active.\n");
		return;
	}
	s_wmv.mouseInitialized = true;
	IN_InitWin32Mouse();
}

// 007D8910  win_input.cpp:405
void IN_MouseEvent(int mstate)
{
	int button;
	int diff;

	if (!s_wmv.mouseInitialized)
		return;
	diff = mstate ^ s_wmv.oldButtonState;
	if (!diff)
		return;
	CL_GamepadIgnoreMouse(false);
	for (button = 0; button < 5; button++)
	{
		if ((1 << button) & diff)
			Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, button + 200, ((1 << button) & mstate) != 0, 0, 0);
	}
	s_wmv.oldButtonState = mstate;
}

// 007D8F60  win_input.cpp:430
void IN_ClampMouseMove(tagPOINT *curPos)
{
	RECT rc;
	bool isClamped;

	GetWindowRect(g_wv.hWnd, &rc);
	isClamped = false;
	if (curPos->x < rc.left)
	{
		curPos->x = rc.left;
		isClamped = true;
	}
	else if (curPos->x >= rc.right)
	{
		curPos->x = rc.right - 1;
		isClamped = true;
	}
	if (curPos->y < rc.top)
	{
		curPos->y = rc.top;
		isClamped = true;
	}
	else if (curPos->y >= rc.bottom)
	{
		curPos->y = rc.bottom - 1;
		isClamped = true;
	}
	if (isClamped)
		SetCursorPos(curPos->x, curPos->y);
}

// 007D8E20  win_input.cpp:470
void IN_MouseMove()
{
	POINT curPos;
	int dx;
	int dy;

	iassert_at(475, s_wmv.mouseInitialized);
	iassert_at(476, r_fullscreen);
	if (!IN_IsForegroundWindow())
		return;
	GetCursorPos(&curPos);
	if (r_fullscreen->current.enabled)
		IN_ClampMouseMove(&curPos);
	dx = curPos.x - s_wmv.oldPos.x;
	dy = curPos.y - s_wmv.oldPos.y;
	if (dx || dy)
		CL_GamepadIgnoreMouse(false);
	s_wmv.oldPos.x = curPos.x;
	s_wmv.oldPos.y = curPos.y;
	ScreenToClient(g_wv.hWnd, &curPos);
	g_wv.recenterMouse = CL_MouseEvent(curPos.x, curPos.y, dx, dy);
	if (g_wv.recenterMouse && (dx || dy))
	{
		IN_RecenterMouse();
		s_wmv.oldPos.x = window_center_x;
		s_wmv.oldPos.y = window_center_y;
	}
}

// 007D89B0  win_input.cpp:506
void IN_SetCursorPos(int x, int y)
{
	POINT curPos;

	curPos.x = x;
	curPos.y = y;
	ClientToScreen(g_wv.hWnd, &curPos);
	SetCursorPos(curPos.x, curPos.y);
	s_wmv.oldPos.x = curPos.x;
	s_wmv.oldPos.y = curPos.y;
}

// 007D8A00  win_input.cpp:522
// ShowCursor keeps a display counter: the cursor shows while it is >= 0. Step it until it is exactly 0
// (show) or -1 (hide).
void IN_ShowSystemCursor(int show)
{
	int desiredShow;
	int actualShow;

	g_showCursor = show;
	desiredShow = (show != 0) - 1;
	for (actualShow = ShowCursor(show); actualShow != desiredShow; actualShow = ShowCursor(actualShow < desiredShow))
		;
}

// 007D8A50  win_input.cpp:548
void IN_Startup()
{
	IN_StartupMouse();
	IN_StartupGamepads();
	Dvar_ClearModified(in_mouse);
}

// 007D8A70  win_input.cpp:568
void IN_Shutdown()
{
	IN_DeactivateMouse();
}

// 007D8A80  win_input.cpp:579
void IN_Init()
{
	in_mouse = Dvar_RegisterBool("in_mouse", true, 0x21, "Initialize the mouse drivers");
	IN_Startup();
}

// 007D8AB0  win_input.cpp:597
void IN_Activate(int active)
{
	in_appactive = active;
	if (active)
		IN_ActivateMouse(1);
	else
		IN_DeactivateMouse();
}

// 007D8AE0  win_input.cpp:615
void IN_Frame()
{
	if (Dvar_GetBool("ClickToContinue"))
		PostMessageA(g_wv.hWnd, WM_LBUTTONDOWN, MK_LBUTTON, 0);
	if (!s_wmv.mouseInitialized)
		return;
	if (!in_appactive)
	{
		IN_DeactivateMouse();
		return;
	}
	IN_ActivateMouse(0);
	IN_MouseMove();
	if (IN_IsForegroundWindow())
		IN_GamepadsMove();
}

// 00546E80  win_input.cpp:753
// Empty in this dedicated-server build (identical-code-folded with 293 other empty functions).
void IN_InitWin32Mouse()
{
}
