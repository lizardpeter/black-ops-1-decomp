// cgame/cg_camera.cpp
//
// Partial reconstruction of codsrc/src/cgame/cg_camera.cpp: so far the field-of-view functions.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv; how FOV works is in docs/fov.md.
#define T5_SOURCE "cgame/cg_camera.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\cgame\\cg_camera.cpp"  // __FILE__ in the original build

#include "cgame/cg_local.h"
#include "universal/com_math.h"

#include <math.h>
#include <t5_assert.h>
#include <t5_compiler.h>
#include <t5_static.h>

// 00477D60  cg_camera.cpp:117
// Blends the FOV from the camera tween's start FOV over tweenDuration seconds (script camera cuts), and
// remembers the result as the camera's last FOV.
float CG_UpdateCameraTweenFOV(int localClientNum, float fov)
{
	cg_s *cgameGlob;
	float elapsed;
	float frac;

	cgameGlob = CG_GetLocalClientGlobals(localClientNum);
	if (cgameGlob->cameraData.tweenStartTime > cgameGlob->time)
		cgameGlob->cameraData.tweenStartTime = 0;
	elapsed = (float)(cgameGlob->time - cgameGlob->cameraData.tweenStartTime) * 0.001f;
	if (cgameGlob->cameraData.tweenDuration > elapsed)
	{
		frac = elapsed / cgameGlob->cameraData.tweenDuration;
		fov = (fov - cgameGlob->cameraData.tweenStartFOV) * frac + cgameGlob->cameraData.tweenStartFOV;
	}
	cgameGlob->cameraData.lastFOV = fov;
	return fov;
}

// The function-local static of CG_GetViewFov (`static int lastTime = Sys_Milliseconds();`), with the
// one-time-initialization guard VC90 generates for it ($S1, bit 0), both at their original addresses.
T5_STATIC unsigned int CG_GetViewFov_initGuard T5_AT("", "$S1");
T5_STATIC int CG_GetViewFov_lastTime T5_AT("CG_GetViewFov", "lastTime");

// 00479530  cg_camera.cpp:912
// The first-person FOV for this frame, in degrees (horizontal at 4:3; CG_CalcFov widens it for the real
// aspect ratio): cg_fov, narrowed toward the weapon's zoom FOV while aiming down sights, widened toward 55
// while deploying, replaced on turrets and in vehicles, then scaled by cg_fovScale and floored at cg_fovMin.
float CG_GetViewFov(int localClientNum)
{
	cg_s *cgameGlob;
	playerState_s *ps;
	int weapIndex;
	const WeaponVariantDef *weapVariantDef;
	const WeaponDef *weapDef;
	float viewFov;
	float posFrac;
	float zoomFov;
	float frac;
	playerEntity_t *pe;
	int now;
	float deployFov;
	centity_s *cent;
	const vehicle_info_t *vehInfo;

	cgameGlob = CG_GetLocalClientGlobals(localClientNum);
	ps = &cgameGlob->predictedPlayerState;
	weapIndex = CG_GetPlayerWeapon(ps, localClientNum);
	weapVariantDef = BG_GetWeaponVariantDef(weapIndex);
	weapDef = BG_GetWeaponDef(weapIndex);
	viewFov = cg_fov->current.value;
	vassertpt_at(932, 1, viewFov >= 1 && viewFov <= 160, "(viewFov) = %g", viewFov);
	if (CG_ShouldDefaultViewFov(localClientNum))
		return viewFov;

	if (BG_IsAimDownSightWeapon(weapIndex))
	{
		iassert_at(943, weapVariantDef->fAdsZoomFov1 > 0.01f);
		posFrac = ps->fWeaponPosFrac;
		// adsZoomSelect picks one of the weapon's three zoom levels; the cycle differs with the toggle style
		if (!cg_adsZoomToggleStyle->current.integer)
		{
			switch (ps->adsZoomSelect % 4)
			{
			case 0:
				zoomFov = I_fmin(weapVariantDef->fAdsZoomFov3, viewFov);
				break;
			case 2:
				zoomFov = I_fmin(weapVariantDef->fAdsZoomFov1, viewFov);
				break;
			default:
				zoomFov = I_fmin(weapVariantDef->fAdsZoomFov2, viewFov);
				break;
			}
		}
		else
		{
			switch (ps->adsZoomSelect % 3)
			{
			case 1:
				zoomFov = I_fmin(weapVariantDef->fAdsZoomFov2, viewFov);
				break;
			case 2:
				zoomFov = I_fmin(weapVariantDef->fAdsZoomFov1, viewFov);
				break;
			default:
				zoomFov = I_fmin(weapVariantDef->fAdsZoomFov3, viewFov);
				break;
			}
		}

		pe = &cgameGlob->playerEntity;
		if (!(CG_GetViewFov_initGuard & 1))
		{
			CG_GetViewFov_initGuard |= 1;
			CG_GetViewFov_lastTime = Sys_Milliseconds();
		}
		now = Sys_Milliseconds();
		if (posFrac == 1.0f)
		{
			viewFov = zoomFov;
		}
		else if (posFrac != 0.0f)
		{
			// the zoom happens over the last fAdsZoomInFrac (fAdsZoomOutFrac going out) of the weapon's
			// travel into position, linearly in degrees
			if (pe->bPositionToADS)
			{
				frac = posFrac - (1.0f - weapVariantDef->fAdsZoomInFrac);
				if (frac > 0.0f)
					frac = frac / weapVariantDef->fAdsZoomInFrac;
			}
			else
			{
				frac = posFrac - (1.0f - weapVariantDef->fAdsZoomOutFrac);
				if (frac > 0.0f)
					frac = frac / weapVariantDef->fAdsZoomOutFrac;
			}
			if (frac > 0.0f)
				viewFov = viewFov - (viewFov - zoomFov) * frac;
		}
		CG_GetViewFov_lastTime = now;  // written, never read
	}

	deployFov = 55.0f;
	if (!(ps->otherFlags & POF_THIRD_PERSON) && ps->weaponstate == WEAPON_DEPLOYING && weapDef != BG_GetWeaponDef(0))
	{
		viewFov = cg_fov->current.value;
		iassert_at(1017, weapDef->deployTime != 0);
		viewFov = (deployFov - cg_fov->current.value) * (1.0f - (float)ps->weaponTime / (float)weapDef->deployTime)
			+ viewFov;
	}
	if (ps->eFlags & EF_TURRET_MASK)
		viewFov = deployFov;
	if (ps->eFlags & EF_VEHICLE_VIEW)
	{
		cent = CG_GetEntity(localClientNum, ps->viewlocked_entNum);
		if (cent && cent->nextValid)
		{
			vehInfo = CG_GetVehicleInfo(cent->nextVehicleInfoIndex);
			viewFov = vehInfo->cameraFOV > cg_fovMin->current.value ? vehInfo->cameraFOV : viewFov;
		}
	}
	viewFov = viewFov * cg_fovScale->current.value;
	viewFov = I_fmax(cg_fovMin->current.value, viewFov);
	return viewFov;
}

// 00479B50  cg_camera.cpp:889
// True when the view uses plain cg_fov: demo third-person and movie cameras, script camera modes, and the
// third-person model view. Turret and vehicle views never do.
bool CG_ShouldDefaultViewFov(int localClientNum)
{
	cg_s *cgameGlob;
	playerState_s *ps;

	cgameGlob = CG_GetLocalClientGlobals(localClientNum);
	ps = &cgameGlob->predictedPlayerState;
	if (Demo_IsThirdPersonCamera() || Demo_IsMovieCamera())
		return true;
	if (cgameGlob->cameraMode)
		return true;
	if ((ps->eFlags & EF_TURRET_MASK) || (ps->eFlags & EF_VEHICLE_VIEW))
		return false;
	return (ps->otherFlags & POF_THIRD_PERSON) && cgameGlob->renderingThirdPerson == TP_FOR_MODEL;
}

// 00479BE0  cg_camera.cpp:1092
// Turns the FOV in degrees into the refdef's view extents. fov_x is the horizontal FOV of a 4:3 view:
// the vertical extent is fixed by it (tan(fov/2) * 3/4) and the horizontal extent follows the window's
// aspect ratio ("Hor+"). fov_x <= 0 means the normal first-person FOV (CG_GetViewFov); kill cams pass their
// own. Also sets zoomSensitivity, the mouse sensitivity scale for this FOV relative to cg_fov.
void CG_CalcFov(int localClientNum, float fov_x)
{
	cg_s *cgameGlob;
	cgs_t *cgs;
	float tanHalfFov;
	float tanHalfFovY;
	float tanHalfFovX;

	cgameGlob = CG_GetLocalClientGlobals(localClientNum);
	cgs = CG_GetLocalClientStaticGlobals(localClientNum);
	if (fov_x <= 0.0f)
		fov_x = CG_GetViewFov(localClientNum);
	fov_x = CG_UpdateCameraTweenFOV(localClientNum, fov_x);
	tanHalfFov = tanf(fov_x * 0.017453292f * 0.5f);
	tanHalfFovY = tanHalfFov * 0.75f;
	tanHalfFovX = tanHalfFovY * cgs->viewAspect;
	cgameGlob->refdef.tanHalfFovX = tanHalfFovX;
	cgameGlob->refdef.tanHalfFovY = tanHalfFovY;
	cgameGlob->refdef.fov_x = fov_x;
	cgameGlob->zoomSensitivity = tanHalfFov / tanf(cg_fov->current.value * 0.017453292f * 0.5f);
}
