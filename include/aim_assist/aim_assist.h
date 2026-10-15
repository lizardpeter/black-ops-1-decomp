// aim_assist: declarations the reconstruction uses.
//
// Layouts from the PDB; signatures fixed by the original decorated names in the MAP. Original header location
// not yet confirmed (aim_assist.h).
#pragma once

#include "client_mp/client_mp.h"

struct AimInput
{
	float deltaTime;          // 0x00
	float pitch;              // 0x04
	float pitchAxis;          // 0x08
	float pitchMax;           // 0x0C
	float yaw;                // 0x10
	float yawAxis;            // 0x14
	float yawMax;             // 0x18
	float forwardAxis;        // 0x1C
	float rightAxis;          // 0x20
	bitarray<51> button_bits; // 0x24
	int localClientNum;       // 0x2C
};
static_assert(sizeof(AimInput) == 48, "AimInput");

struct AimOutput
{
	float pitch;                   // 0x00
	float yaw;                     // 0x04
	float meleeChargeYaw;          // 0x08
	unsigned char meleeChargeDist; // 0x0C
};
static_assert(sizeof(AimOutput) == 16, "AimOutput");

struct AimAssistGlobals  // sizeof 3760; partial
{
	char pad_0000[76];
	bool initialized;              // 0x004C
	char pad_004D[3728 - 77];
	bitarray<51> prev_button_bits; // 0x0E90
	char pad_0E98[3760 - 3736];
};
static_assert(sizeof(AimAssistGlobals) == 3760, "AimAssistGlobals");
T5_OFFSET_CHECK(AimAssistGlobals, prev_button_bits, 0xE90);

// ---- resolved to the original binary by the hybrid link --------------------------------------------------
void AimAssist_UpdateTweakables(int localClientNum);
void AimAssist_UpdateAdsLerp(const AimInput *input);
void AimAssist_ApplyAutoMelee(const AimInput *input, AimOutput *output);

// ---- reconstructed ---------------------------------------------------------------------------------------
void AimAssist_UpdateMouseInput(const AimInput *input, AimOutput *output);
