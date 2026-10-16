// cgame: declarations the reconstructed FOV code uses.
//
// Layouts from the PDB; signatures fixed by the original decorated names in the MAP. cg_s is 465 KB and
// cgs_t 12 KB: only the members reconstructed code uses are declared, at their PDB offsets, with padding
// between them. Original header locations not yet confirmed (cg_local.h, cgame_mp/cg_local_mp.h).
#pragma once

#include "bgame/bg_public.h"
#include "qcommon/dvar.h"

struct refdef_s  // sizeof 99600; partial
{
	unsigned int x;         // 0x00
	unsigned int y;         // 0x04
	unsigned int width;     // 0x08
	unsigned int height;    // 0x0C
	float tanHalfFovX;      // 0x10
	float tanHalfFovY;      // 0x14
	float fov_x;            // 0x18
	float vieworg[3];       // 0x1C
	float yaw;              // 0x28
	float viewaxis[3][3];   // 0x2C
	int time;               // 0x50
	float zNear;            // 0x54
	float zFar;             // 0x58
	char pad_005C[99600 - 92];
};
static_assert(sizeof(refdef_s) == 99600, "refdef_s");

struct playerEntity_t
{
	float fLastWeaponPosFrac;  // 0x00
	int bPositionToADS;        // 0x04
	float vPositionLastOrg[3]; // 0x08
	float fLastIdleFactor;     // 0x14
	float baseMoveOrigin[3];   // 0x18
	float baseMoveAngles[3];   // 0x24
};
static_assert(sizeof(playerEntity_t) == 48, "playerEntity_t");

struct Camera  // sizeof 172; partial
{
	char pad_0000[24];
	float lastFOV;          // 0x18
	char pad_001C[72 - 28];
	float tweenStartFOV;    // 0x48
	int tweenStartTime;     // 0x4C
	float tweenDuration;    // 0x50
	char pad_0054[172 - 84];
};
static_assert(sizeof(Camera) == 172, "Camera");

enum thirdPersonType
{
	TP_OFF = 0,
	TP_FOR_MODEL = 1,
	TP_FOR_SHADOW = 2,
};

struct cg_s  // sizeof 465792; partial
{
	int clientNum;                          // 0x00000
	int localClientNum;                     // 0x00004
	char pad_00008[20 - 8];
	int cameraMode;                         // 0x00014
	char pad_00018[263304 - 24];
	int time;                               // 0x40488
	char pad_4048C[263320 - 263308];
	thirdPersonType renderingThirdPerson;   // 0x40498
	playerState_s predictedPlayerState;     // 0x4049C
	char pad_42B40[274028 - 263324 - 9892];
	playerEntity_t playerEntity;            // 0x42E6C
	char pad_42E9C[274112 - 274028 - 48];
	refdef_s refdef;                        // 0x42EC0
	char pad_5B0D0[373724 - 274112 - 99600];
	Camera cameraData;                      // 0x5B3DC
	char pad_5B488[386332 - 373724 - 172];
	float zoomSensitivity;                  // 0x5E51C
	char pad_5E520[465792 - 386336];
};
static_assert(sizeof(cg_s) == 465792, "cg_s");
T5_OFFSET_CHECK(cg_s, predictedPlayerState, 0x4049C);
T5_OFFSET_CHECK(cg_s, playerEntity, 0x42E6C);
T5_OFFSET_CHECK(cg_s, refdef, 0x42EC0);
T5_OFFSET_CHECK(cg_s, cameraData, 0x5B3DC);
T5_OFFSET_CHECK(cg_s, zoomSensitivity, 0x5E51C);

struct cgs_t  // sizeof 12680; partial
{
	int viewX;              // 0x00
	int viewY;              // 0x04
	int viewWidth;          // 0x08
	int viewHeight;         // 0x0C
	float viewAspect;       // 0x10: the window's aspect ratio as the renderer chose it (R_StoreWindowSettings)
	char pad_0014[12680 - 20];
};
static_assert(sizeof(cgs_t) == 12680, "cgs_t");

struct centity_s  // sizeof 808; partial
{
	char pad_0000[624];
	unsigned char nextVehicleInfoIndex;  // 0x270: nextState.un2.vehicleState.vehicleInfoIndex
	char pad_0271[804 - 625];
	unsigned int applyLeftHandIK : 1;    // 0x324 bit 0
	unsigned int nextValid : 1;          // 0x324 bit 1
	unsigned int otherFlagBits : 30;     // 0x324 bits 2..31 (bMuzzleFlash, bTrailMade, ...)
};
static_assert(sizeof(centity_s) == 808, "centity_s");

// ---- dvars (cgame_mp/cg_main_mp.cpp) ---------------------------------------------------------------------
extern const dvar_s *cg_fov;                      // float 1..160, default 65: horizontal FOV at 4:3
extern const dvar_s *cg_fov_default;              // float 65..80: the player's FOV setting
extern const dvar_s *cg_fov_default_thirdperson;  // float 1..160, default 40
extern const dvar_s *cg_fovScale;                 // float 0.2..2
extern const dvar_s *cg_fovMin;                   // float 1..160, default 10
extern const dvar_s *cg_adsZoomToggleStyle;       // int 0..1
extern const dvar_s *cg_thirdPerson;              // int 0..2

// ---- functions resolved to the original binary by the hybrid link ----------------------------------------
cg_s *CG_GetLocalClientGlobals(int localClientNum);               // cgame_mp/cg_local_mp.h
cgs_t *CG_GetLocalClientStaticGlobals(int localClientNum);        // cgame_mp/cg_local_mp.h
centity_s *CG_GetEntity(int localClientNum, int entityIndex);     // cgame_mp/cg_local_mp.h
int CG_GetPlayerWeapon(const playerState_s *ps, int localClientNum);  // cgame/cg_weapons.cpp
const vehicle_info_t *CG_GetVehicleInfo(int vehicleInfoIndex);    // cgame_mp/cg_vehicles_mp.cpp
bool Demo_IsThirdPersonCamera();                                  // demo/demo_playback.cpp
bool Demo_IsMovieCamera();                                        // demo/demo_playback.cpp
int Sys_Milliseconds();                                           // win32/win_shared.cpp
void Dvar_SetFloat(const dvar_s *dvar, float value);              // universal/dvar.cpp
void Dvar_SetInt(const dvar_s *dvar, int value);                  // universal/dvar.cpp
void Dvar_SetBoolByName(const char *dvarName, bool value);        // universal/dvar.cpp

// ---- reconstructed ---------------------------------------------------------------------------------------
float CG_UpdateCameraTweenFOV(int localClientNum, float fov);     // cgame/cg_camera.cpp
float CG_GetViewFov(int localClientNum);                          // cgame/cg_camera.cpp
bool CG_ShouldDefaultViewFov(int localClientNum);                 // cgame/cg_camera.cpp
void CG_CalcFov(int localClientNum, float fov_x);                 // cgame/cg_camera.cpp
float CG_GetDefaultFovForView();                                  // cgame/cg_main.cpp
void CG_UpdateFov(float value);                                   // cgame/cg_main.cpp
void CG_SetThirdPerson(int value);                                // cgame/cg_main.cpp
