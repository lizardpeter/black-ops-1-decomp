// bgame: shared game types the reconstruction uses (client and cgame both see them).
//
// Layouts from the PDB; only the members reconstructed code touches are declared, at their PDB offsets, with
// padding between them. Original header location not yet confirmed (bg_public.h in the IW engine family).
#pragma once

#define T5_OFFSET_CHECK(type, member, offset) static_assert(__builtin_offsetof(type, member) == offset, #type "." #member)

struct playerState_s  // sizeof 9892; partial
{
	char pad_0000[12];
	int pm_flags;           // 0x00C
	char pad_0010[4];
	int otherFlags;         // 0x014
	char pad_0018[60 - 24];
	int weaponTime;         // 0x03C
	char pad_0040[224 - 64];
	int eFlags;             // 0x0E0
	int eFlags2;            // 0x0E4
	char pad_00E8[344 - 232];
	int weaponstate;        // 0x158
	char pad_015C[360 - 348];
	float fWeaponPosFrac;   // 0x168
	char pad_016C[1088 - 364];
	short viewlocked_entNum;// 0x440
	char pad_0442[1368 - 1090];
	int adsZoomSelect;      // 0x558
	char pad_055C[9892 - 1372];
};
static_assert(sizeof(playerState_s) == 9892, "playerState_s");
T5_OFFSET_CHECK(playerState_s, weaponTime, 0x3C);
T5_OFFSET_CHECK(playerState_s, eFlags, 0xE0);
T5_OFFSET_CHECK(playerState_s, fWeaponPosFrac, 0x168);
T5_OFFSET_CHECK(playerState_s, viewlocked_entNum, 0x440);
T5_OFFSET_CHECK(playerState_s, adsZoomSelect, 0x558);

// names not confirmed: the bits and states as the FOV and mouse code test them
#define POF_THIRD_PERSON 0x2          // playerState_s.otherFlags: third-person / spectator view
#define EF_TURRET_MASK 0x300          // playerState_s.eFlags: on a turret (CG_GetPlayerWeapon tests the same bits)
#define EF_VEHICLE_VIEW 0x4000        // playerState_s.eFlags: in a vehicle seat with its own camera
#define WEAPON_DEPLOYING 35           // playerState_s.weaponstate (also stops mouse look, cl_input_mp.cpp)

// bgame/bg_weapons*.cpp
struct WeaponVariantDef  // sizeof 228; partial
{
	char pad_0000[100];
	float fAdsZoomFov1;     // 0x64
	float fAdsZoomFov2;     // 0x68
	float fAdsZoomFov3;     // 0x6C
	float fAdsZoomInFrac;   // 0x70
	float fAdsZoomOutFrac;  // 0x74
	char pad_0078[228 - 120];
};
static_assert(sizeof(WeaponVariantDef) == 228, "WeaponVariantDef");

struct WeaponDef  // sizeof 2056; partial
{
	char pad_0000[1100];
	int deployTime;         // 0x44C
	char pad_0450[2056 - 1104];
};
static_assert(sizeof(WeaponDef) == 2056, "WeaponDef");

struct vehicle_info_t  // sizeof 7640; partial
{
	char pad_0000[172];
	float cameraFOV;        // 0xAC
	char pad_00B0[7640 - 176];
};
static_assert(sizeof(vehicle_info_t) == 7640, "vehicle_info_t");

const WeaponVariantDef *BG_GetWeaponVariantDef(unsigned int weaponIndex);  // bgame/bg_weapons_def.cpp
const WeaponDef *BG_GetWeaponDef(unsigned int weaponIndex);                // bgame/bg_weapons_def.cpp
int BG_IsAimDownSightWeapon(unsigned int weaponIndex);                     // bgame/bg_weapons.cpp
