// cgame/cg_main.cpp
//
// Partial reconstruction of codsrc/src/cgame/cg_main.cpp: so far the default-FOV functions.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv.
#define T5_SOURCE "cgame/cg_main.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\cgame\\cg_main.cpp"  // __FILE__ in the original build

#include "cgame/cg_local.h"

// 004BFF00  cg_main.cpp:233
// The player's FOV setting for the current view: cg_fov_default (65..80) in first person,
// cg_fov_default_thirdperson in third person.
float CG_GetDefaultFovForView()
{
	if (cg_thirdPerson->current.integer)
		return cg_fov_default_thirdperson->current.value;
	return cg_fov_default->current.value;
}

// 004BFF30  cg_main.cpp:245
void CG_UpdateFov(float value)
{
	if (cg_fov->current.value != value)
		Dvar_SetFloat(cg_fov, value);
}

// 004BFF60  cg_main.cpp:253
void CG_SetThirdPerson(int value)
{
	if (cg_thirdPerson->current.integer != value)
	{
		Dvar_SetInt(cg_thirdPerson, value);
		Dvar_SetBoolByName("cg_spectateThirdPerson", value > 0);
	}
	CG_UpdateFov(CG_GetDefaultFovForView());
}
