// aim_assist/aim_assist.cpp
//
// Partial reconstruction of codsrc/src/aim_assist/aim_assist.cpp: so far only the mouse entry point.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv.
#define T5_SOURCE "aim_assist/aim_assist.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\aim_assist\\aim_assist.cpp"  // __FILE__ in the original build

#include "aim_assist/aim_assist.h"

#include <t5_assert.h>
#include <t5_compiler.h>
#include <t5_static.h>

T5_STATIC AimAssistGlobals aaGlobArray[1] T5_AT("", "aaGlobArray");

// 00405F20  aim_assist.cpp:2369
// With a mouse, aim assist only does auto-melee; the view angles pass through unchanged. When aim assist
// isn't initialized, output->meleeChargeYaw and meleeChargeDist are left unwritten (records/bugs.tsv).
void AimAssist_UpdateMouseInput(const AimInput *input, AimOutput *output)
{
	AimAssistGlobals *aaGlob;
	int i;

	iassert_at(2378, input);
	iassert_at(2379, output);
	T5_X87_COPY(output->pitch, input->pitch);
	T5_X87_COPY(output->yaw, input->yaw);
	aaGlob = &aaGlobArray[input->localClientNum];
	if (!aaGlob->initialized)
		return;
	AimAssist_UpdateTweakables(input->localClientNum);
	AimAssist_UpdateAdsLerp(input);
	AimAssist_ApplyAutoMelee(input, output);
	for (i = 0; i < 2; i++)
		aaGlob->prev_button_bits.array[i] = input->button_bits.array[i];
}
