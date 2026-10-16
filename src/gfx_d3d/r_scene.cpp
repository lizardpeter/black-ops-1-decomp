// gfx_d3d/r_scene.cpp
//
// Partial reconstruction of codsrc/src/gfx_d3d/r_scene.cpp: so far the view setup that turns a refdef
// (position, axes, view extents) into the view and projection matrices.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv; how FOV becomes a projection is in docs/fov.md.
#define T5_SOURCE "gfx_d3d/r_scene.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_scene.cpp"  // __FILE__ in the original build

#include "gfx_d3d/r_local.h"
#include "cgame/cg_local.h"
#include "universal/com_math.h"

#include <string.h>
#include <t5_compiler.h>

static const float MINIMUM_Z_NEAR = 1.0f;  // 00D6C924 in the original (PDB static of r_scene.cpp)

// Vec3Copy as this file's code has it inlined: three float copies through the x87 stack.
static inline void Vec3Copy_x87(const float from[3], float to[3])
{
	T5_X87_COPY(to[0], from[0]);
	T5_X87_COPY(to[1], from[1]);
	T5_X87_COPY(to[2], from[2]);
}

// 00A5D560  r_scene.cpp:916
float R_GetDefaultNearClip()
{
	return I_fmax(MINIMUM_Z_NEAR, r_znear->current.value);
}

// 00A61B60  r_scene.cpp:921
// The projection: a perspective matrix with the far plane at infinity (InfinitePerspectiveMatrix).
void R_SetupProjection(float tanHalfFovX, float tanHalfFovY, GfxViewParms *viewParms)
{
	InfinitePerspectiveMatrix(tanHalfFovX, tanHalfFovY, viewParms->zNear, viewParms->projectionMatrix.m);
	viewParms->depthHackNearClip = -r_znear_depthhack->current.value;
}

// 00A5D5B0  r_scene.cpp:934
// viewProjection = view * projection and its inverse. For a tiled hi-res screenshot (offsetmatrix and a
// shot in progress) the projection first gets narrowed to the current tile.
void R_SetupViewProjectionMatrices(GfxViewParms *viewParms, bool offsetmatrix)
{
	if (rg.renderHiResShot && rg.hiResShotTiles > 0 && offsetmatrix)
	{
		viewParms->projectionMatrix.m[0][0] = (float)rg.hiResShotTiles * viewParms->projectionMatrix.m[0][0];
		viewParms->projectionMatrix.m[1][1] = (float)rg.hiResShotTiles * viewParms->projectionMatrix.m[1][1];
		viewParms->projectionMatrix.m[2][0] =
			(rg.hiResShotRow / (float)rg.hiResShotTiles + (rg.hiResShotRow + 1.0f) / (float)rg.hiResShotTiles)
			/ (rg.hiResShotRow / (float)rg.hiResShotTiles - (rg.hiResShotRow + 1.0f) / (float)rg.hiResShotTiles);
		viewParms->projectionMatrix.m[2][1] =
			(rg.hiResShotCol / (float)rg.hiResShotTiles + (rg.hiResShotCol + 1.0f) / (float)rg.hiResShotTiles)
			/ (-rg.hiResShotCol / (float)rg.hiResShotTiles + (rg.hiResShotCol + 1.0f) / (float)rg.hiResShotTiles);
		viewParms->projectionMatrix.m[2][0] = (float)rg.hiResShotTiles + viewParms->projectionMatrix.m[2][0];
		viewParms->projectionMatrix.m[2][1] = viewParms->projectionMatrix.m[2][1] - (float)rg.hiResShotTiles;
	}
	MatrixMultiply44(viewParms->viewMatrix.m, viewParms->projectionMatrix.m, viewParms->viewProjectionMatrix.m);
	MatrixInverse44(&viewParms->viewProjectionMatrix.m[0][0], &viewParms->inverseViewProjectionMatrix.m[0][0]);
}

// 00A619A0  r_scene.cpp:6759
// The view parameters for a scene: position and axes from the refdef, the view matrix, a near plane of at
// least MINIMUM_Z_NEAR (the refdef's, or r_znear), and the projection from the refdef's view extents.
void R_SetViewParmsForScene(const refdef_s *refdef, GfxViewParms *viewParms)
{
	float zNear;

	memset(viewParms, 0, sizeof(*viewParms));
	Vec3Copy_x87(refdef->vieworg, viewParms->origin);
	viewParms->origin[3] = 1.0f;
	Vec3Copy_x87(refdef->viewaxis[0], viewParms->axis[0]);
	Vec3Copy_x87(refdef->viewaxis[1], viewParms->axis[1]);
	Vec3Copy_x87(refdef->viewaxis[2], viewParms->axis[2]);
	MatrixForViewer(viewParms->origin, viewParms->axis, viewParms->viewMatrix.m);
	zNear = refdef->zNear > 0.0f ? refdef->zNear : R_GetDefaultNearClip();
	viewParms->zNear = I_fmax(MINIMUM_Z_NEAR, zNear);
	R_SetupProjection(refdef->tanHalfFovX, refdef->tanHalfFovY, viewParms);
	R_SetupViewProjectionMatrices(viewParms, true);
}
