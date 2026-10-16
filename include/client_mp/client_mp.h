// client_mp: declarations the reconstructed client input functions use.
//
// Layouts from the PDB; signatures fixed by the original decorated names in the MAP. The client structs are
// huge (clientActive_t is 1.7 MB): only the members reconstructed code uses are declared, at their PDB
// offsets, with padding between them. Original header locations not yet confirmed (client_mp.h and
// friends in the IW engine family).
#pragma once

#include "bgame/bg_public.h"
#include "qcommon/dvar.h"

// ---- shared types -----------------------------------------------------------------------------------------
template <int BIT_COUNT>
class bitarray
{
public:
	unsigned int array[(BIT_COUNT + 31) / 32];
};

struct usercmd_s
{
	int serverTime;                        // 0x00
	bitarray<51> button_bits;              // 0x04
	int angles[3];                         // 0x0C
	unsigned short weapon;                 // 0x18
	unsigned short offHandIndex;           // 0x1A
	unsigned short lastWeaponAltModeSwitch;// 0x1C
	signed char forwardmove;               // 0x1E
	signed char rightmove;                 // 0x1F
	signed char upmove;                    // 0x20
	signed char pitchmove;                 // 0x21
	signed char yawmove;                   // 0x22
	float meleeChargeYaw;                  // 0x24
	unsigned char meleeChargeDist;         // 0x28
	float rollmove;                        // 0x2C
	signed char selectedLocation[2];       // 0x30
	unsigned char selectedYaw;             // 0x32
};
static_assert(sizeof(usercmd_s) == 52, "usercmd_s");

// ---- client_mp/cl_input_mp.cpp ----------------------------------------------------------------------------
struct kbutton_t
{
	int down[2];          // 0x00
	unsigned int downtime;// 0x08
	unsigned int msec;    // 0x0C
	bool active;          // 0x10
	bool wasPressed;      // 0x11
	float val;            // 0x14
};
static_assert(sizeof(kbutton_t) == 24, "kbutton_t");

enum kbuttons_t
{
	KB_LEFT = 0, KB_RIGHT = 1, KB_FORWARD = 2, KB_BACK = 3, KB_LOOKUP = 4, KB_LOOKDOWN = 5, KB_MOVELEFT = 6,
	KB_MOVERIGHT = 7, KB_STRAFE = 8, KB_SPEED = 9, KB_UP = 10, KB_DOWN = 11, KB_ANYUP = 12, KB_MLOOK = 13,
	NUM_BUTTONS = 47,
};

extern unsigned int frame_msec;
extern const dvar_s *m_filter;
extern const dvar_s *cl_mouseAccel;
extern const dvar_s *cl_sensitivity;
extern const dvar_s *cl_showMouseRate;
extern const dvar_s *m_yaw;
extern const dvar_s *m_pitch;
extern const dvar_s *m_side;
extern const dvar_s *m_forward;
extern const dvar_s *cl_freelook;

// ---- client structs (partial) ----------------------------------------------------------------------------
struct clSnapshot_t  // sizeof 9948; partial
{
	char pad_0000[32];
	playerState_s ps;       // 0x020
	char pad_26C4[9948 - 32 - 9892];
};
static_assert(sizeof(clSnapshot_t) == 9948, "clSnapshot_t");

struct clientActive_t  // sizeof 1728768; partial
{
	bool usingAds;                       // 0x0000
	int timeoutcount;                    // 0x0004
	clSnapshot_t snap;                   // 0x0008
	char pad_26E4[10068 - 8 - 9948];
	int mouseDx[2];                      // 0x2754
	int mouseDy[2];                      // 0x275C
	int mouseIndex;                      // 0x2764
	char pad_2768[10116 - 10088];
	float cgameFOVSensitivityScale;      // 0x2784
	float cgameMaxPitchSpeed;            // 0x2788
	float cgameMaxYawSpeed;              // 0x278C
	char pad_2790[10248 - 10128];
	float viewangles[3];                 // 0x2808
};
T5_OFFSET_CHECK(clientActive_t, snap.ps.pm_flags, 0x34);
T5_OFFSET_CHECK(clientActive_t, snap.ps.eFlags2, 0x10C);
T5_OFFSET_CHECK(clientActive_t, snap.ps.weaponstate, 0x180);
T5_OFFSET_CHECK(clientActive_t, mouseDx, 0x2754);
T5_OFFSET_CHECK(clientActive_t, cgameFOVSensitivityScale, 0x2784);
T5_OFFSET_CHECK(clientActive_t, viewangles, 0x2808);

struct clientUIActive_t  // partial
{
	char pad_0000[4];
	int keyCatchers;                     // 0x0004
};

struct clientStatic_t  // sizeof 17992312; partial
{
	char pad_0000[288];
	int frametime;                       // 0x0120
};
extern clientStatic_t cls;

#define KEYCATCH_UI 0x10
#define PMF_FROZEN_NO_LOOK 0x800  // name not confirmed: pm_flags bit that stops view movement
#define WEAPON_STATE_NO_LOOK 35   // name not confirmed: weaponstate that stops view movement
#define EF2_NO_AIM_ASSIST 0x40000 // name not confirmed: eFlags2 bit that skips mouse aim assist

// ---- functions resolved to the original binary by the hybrid link ----------------------------------------
clientActive_t *CL_GetLocalClientGlobals(int localClientNum);    // client_mp/cl_main_mp.cpp
clientUIActive_t *CL_GetLocalClientUIGlobals(int localClientNum);// client_mp/cl_main_mp.cpp
void CL_ShowSystemCursor(int show);                              // client_mp/cl_input_mp.cpp
bool DevGui_IsActive();                                          // devgui/devgui.cpp
void DevGui_MouseEvent(int dx, int dy);                          // devgui/devgui_input.cpp
bool UI_KeysBypassMenu(int localClientNum);                      // ui/ui_main.cpp
void UI_MouseEvent(int localClientNum, int x, int y);            // ui/ui_main.cpp
void IN_SetCursorPos(int x, int y);                              // win32/win_input.cpp
void Com_Printf(int channel, const char *fmt, ...);              // qcommon/common.cpp
signed char ClampChar(const int i);                              // universal/com_math.cpp

// ---- reconstructed ---------------------------------------------------------------------------------------
int CL_MouseEvent(int x, int y, int dx, int dy);
void CL_SetCursorPos(int x, int y);
void CL_GetMouseMovement(clientActive_t *cl, float *mx, float *my);
void CL_MouseMove(int localClientNum, usercmd_s *cmd);
void CL_SetFOVSensitivityScale(int localClientNum, float scale);  // client_mp/cl_cgame_mp.cpp
