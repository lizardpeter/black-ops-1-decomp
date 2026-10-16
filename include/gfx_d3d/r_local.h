// gfx_d3d: declarations the reconstructed view-setup and window-settings code uses.
//
// Layouts from the PDB; signatures fixed by the original decorated names in the MAP. r_globals_t (68 KB) and
// DxGlobals (11 KB) are partial: only the members reconstructed code uses are declared, at their PDB offsets.
// Original header locations not yet confirmed (r_local.h, rb_backend.h, r_init.h).
#pragma once

#include "bgame/bg_public.h"  // T5_OFFSET_CHECK
#include "qcommon/dvar.h"

struct refdef_s;  // cgame/cg_local.h

struct GfxMatrix
{
	float m[4][4];
};

struct GfxViewParms  // sizeof 320
{
	GfxMatrix viewMatrix;                  // 0x000: world -> view (MatrixForViewer)
	GfxMatrix projectionMatrix;            // 0x040: view -> clip (InfinitePerspectiveMatrix)
	GfxMatrix viewProjectionMatrix;        // 0x080
	GfxMatrix inverseViewProjectionMatrix; // 0x0C0
	float origin[4];                       // 0x100
	float axis[3][3];                      // 0x110
	float depthHackNearClip;               // 0x134: near plane of the view model (r_znear_depthhack), negated
	float zNear;                           // 0x138
	float zFar;                            // 0x13C
};
static_assert(sizeof(GfxViewParms) == 320, "GfxViewParms");
T5_OFFSET_CHECK(GfxViewParms, axis, 0x110);
T5_OFFSET_CHECK(GfxViewParms, zNear, 0x138);

struct r_globals_t  // sizeof 68112; partial
{
	char pad_00000[67876];
	int renderHiResShot;      // 0x10924
	int hiResShotTiles;       // 0x10928: a hi-res screenshot is tiles x tiles renders
	float hiResShotRow;       // 0x1092C
	float hiResShotCol;       // 0x10930
	char pad_10934[68112 - 67892];
};
static_assert(sizeof(r_globals_t) == 68112, "r_globals_t");
extern r_globals_t rg;

struct GfxWindowParms  // sizeof 40
{
	void *hwnd;               // 0x00 (HWND)
	int hz;                   // 0x04
	bool fullscreen;          // 0x08
	int x;                    // 0x0C
	int y;                    // 0x10
	int sceneWidth;           // 0x14
	int sceneHeight;          // 0x18
	int displayWidth;         // 0x1C
	int displayHeight;        // 0x20
	int aaSamples;            // 0x24
};
static_assert(sizeof(GfxWindowParms) == 40, "GfxWindowParms");

struct vidConfig_t  // sizeof 56
{
	unsigned int sceneWidth;            // 0x00
	unsigned int sceneHeight;           // 0x04
	unsigned int displayWidth;          // 0x08
	unsigned int displayHeight;         // 0x0C
	unsigned short outputDisplayWidth;  // 0x10
	unsigned short outputDisplayHeight; // 0x12
	unsigned int displayFrequency;      // 0x14
	bool isToolMode;                    // 0x18
	int isFullscreen;                   // 0x1C
	float aspectRatioWindow;            // 0x20: 4:3, 16:10 or 16:9, nothing else (R_StoreWindowSettings)
	float aspectRatioScenePixel;        // 0x24
	float aspectRatioDisplayPixel;      // 0x28
	unsigned int maxTextureSize;        // 0x2C
	unsigned int maxTextureMaps;        // 0x30
	bool deviceSupportsGamma;           // 0x34
};
static_assert(sizeof(vidConfig_t) == 56, "vidConfig_t");
extern vidConfig_t vidConfig;

struct DxGlobals  // sizeof 11520; partial
{
	char pad_0000[20];
	bool adapterNativeIsValid;   // 0x14
	int adapterNativeWidth;      // 0x18
	int adapterNativeHeight;     // 0x1C
	int adapterFullscreenWidth;  // 0x20
	int adapterFullscreenHeight; // 0x24
	char pad_0028[11520 - 40];
};
static_assert(sizeof(DxGlobals) == 11520, "DxGlobals");
extern DxGlobals dx;

enum GfxAspectRatio  // r_aspectRatio
{
	ASPECT_RATIO_AUTO = 0,
	ASPECT_RATIO_4_3 = 1,
	ASPECT_RATIO_16_10 = 2,
	ASPECT_RATIO_16_9 = 3,
};

// ---- dvars ---------------------------------------------------------------------------------------------------
extern const dvar_s *r_znear;                    // float 1..10000, default 4
extern const dvar_s *r_znear_depthhack;          // float 0.001..16, default 0.1
extern const dvar_s *r_aspectRatio;              // enum: auto, standard, wide 16:10, wide 16:9
extern const dvar_s *r_reflectionProbeGenerate;  // bool
extern const dvar_s *com_wideScreen;             // bool (qcommon/common.cpp)

// ---- functions resolved to the original binary by the hybrid link ----------------------------------------
char *va(const char *format, ...);  // qcommon/common.cpp

// ---- reconstructed ---------------------------------------------------------------------------------------
float R_GetDefaultNearClip();                                                     // gfx_d3d/r_scene.cpp
void R_SetupProjection(float tanHalfFovX, float tanHalfFovY, GfxViewParms *viewParms);   // gfx_d3d/r_scene.cpp
void R_SetupViewProjectionMatrices(GfxViewParms *viewParms, bool offsetmatrix);   // gfx_d3d/r_scene.cpp
void R_SetViewParmsForScene(const refdef_s *refdef, GfxViewParms *viewParms);     // gfx_d3d/r_scene.cpp
void R_StoreWindowSettings(const GfxWindowParms *wndParms);                       // gfx_d3d/r_init.cpp
