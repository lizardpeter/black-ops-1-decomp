// gfx_d3d/r_init.cpp
//
// Partial reconstruction of codsrc/src/gfx_d3d/r_init.cpp: so far R_StoreWindowSettings, where the
// renderer decides the window's aspect ratio (which cgame's Hor+ FOV then uses, docs/fov.md).
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv.
#define T5_SOURCE "gfx_d3d/r_init.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp"  // __FILE__ in the original build

#include "gfx_d3d/r_local.h"

#include <emmintrin.h>
#include <t5_assert.h>

// float to int, rounding to nearest after adding 2^-30, as the original compiles it (fld, fadd double
// 2^-30, fistp). The same idiom as CL_MouseMoveToInt in client_mp/cl_input_mp.cpp.
static inline int R_RoundBiased(float f)
{
	return _mm_cvtsd_si32(_mm_set_sd((double)f + 9.313225746154785e-10));
}

// 00A4F6D0  r_init.cpp:469
// Records the window's sizes and picks its aspect ratio. Only three aspect ratios exist: with r_aspectRatio
// on auto, 16 * height / width (of the monitor's native mode in fullscreen, else of the window) is rounded
// to a whole number: 10 means 16:10, less than 10 means 16:9 and more means 4:3. Everything wider than 16:9
// (21:9, 32:9) is treated as 16:9, and 5:4 and 3:2 as 4:3 (records/bugs.tsv).
void R_StoreWindowSettings(const GfxWindowParms *wndParms)
{
	int monitorWidth;
	int monitorHeight;
	int aspectHeight;

	iassert_at(476, r_aspectRatio);
	vidConfig.sceneWidth = wndParms->sceneWidth;
	vidConfig.sceneHeight = wndParms->sceneHeight;
	vidConfig.displayWidth = wndParms->displayWidth;
	vidConfig.displayHeight = wndParms->displayHeight;
	vidConfig.displayFrequency = wndParms->hz;
	vidConfig.isFullscreen = wndParms->fullscreen;
	switch (r_aspectRatio->current.integer)
	{
	case ASPECT_RATIO_AUTO:
		if (vidConfig.isFullscreen && dx.adapterNativeIsValid)
		{
			monitorWidth = dx.adapterNativeWidth;
			monitorHeight = dx.adapterNativeHeight;
		}
		else
		{
			monitorWidth = vidConfig.displayWidth;
			monitorHeight = vidConfig.displayHeight;
		}
		aspectHeight = R_RoundBiased((float)monitorHeight * 16.0f / (float)monitorWidth);
		if (aspectHeight == 10)
			vidConfig.aspectRatioWindow = 16.0f / 10.0f;
		else if (aspectHeight < 10)
			vidConfig.aspectRatioWindow = 16.0f / 9.0f;
		else
			vidConfig.aspectRatioWindow = 4.0f / 3.0f;
		break;
	case ASPECT_RATIO_4_3:
		vidConfig.aspectRatioWindow = 4.0f / 3.0f;
		break;
	case ASPECT_RATIO_16_10:
		vidConfig.aspectRatioWindow = 16.0f / 10.0f;
		break;
	case ASPECT_RATIO_16_9:
		vidConfig.aspectRatioWindow = 16.0f / 9.0f;
		break;
	default:
		if (!Assert_MyHandler(T5_ASSERT_FILE, 521, 1, va("unhandled case, aspectRatio = %i\n", r_aspectRatio->current.integer)))
			__debugbreak();
		break;
	}
	iassert_at(537, com_wideScreen);
	Dvar_SetBool(com_wideScreen, vidConfig.aspectRatioWindow != 4.0f / 3.0f);
	// The original computes this one on the x87 stack (it converts the unsigned sizes there) without
	// rounding the product to float: double arithmetic reproduces it.
	vidConfig.aspectRatioScenePixel =
		(float)((double)vidConfig.sceneHeight * (double)vidConfig.aspectRatioWindow / (double)vidConfig.sceneWidth);
	if (!vidConfig.isFullscreen)
		vidConfig.aspectRatioDisplayPixel = 1.0f;
	else
		vidConfig.aspectRatioDisplayPixel =
			(float)dx.adapterFullscreenHeight * vidConfig.aspectRatioWindow / (float)dx.adapterFullscreenWidth;
	vidConfig.isToolMode = false;  // stored twice in the original
	vidConfig.isToolMode = r_reflectionProbeGenerate && r_reflectionProbeGenerate->current.enabled;
}
