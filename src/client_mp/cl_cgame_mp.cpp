// client_mp/cl_cgame_mp.cpp
//
// Partial reconstruction of codsrc/src/client_mp/cl_cgame_mp.cpp: so far only what the mouse path uses.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv.
#define T5_SOURCE "client_mp/cl_cgame_mp.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\client_mp\\cl_cgame_mp.cpp"  // __FILE__ in the original build

#include "client_mp/client_mp.h"

// 00541420  cl_cgame_mp.cpp:384
// cgame's mouse sensitivity scale (lower while aiming down sights); CL_MouseMove multiplies it in.
void CL_SetFOVSensitivityScale(int localClientNum, float scale)
{
	clientActive_t *cl;

	cl = CL_GetLocalClientGlobals(localClientNum);
	cl->cgameFOVSensitivityScale = scale;
}
