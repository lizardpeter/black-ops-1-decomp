// client_mp/cl_input_mp.cpp
//
// Partial reconstruction of codsrc/src/client_mp/cl_input_mp.cpp: so far only the mouse path, in original
// line order. Each function is tagged with its address in build CoDMPServer and its first line in the
// original file. Status of every function lives in records/functions.tsv.
//
// How mouse movement becomes view angles:
//   IN_MouseMove (win32/win_input.cpp) -> CL_MouseEvent: adds the cursor delta to cl->mouseDx/Dy[mouseIndex]
//   CL_CreateCmd -> CL_MouseMove, once per frame:
//     CL_GetMouseMovement: this frame's counts (or the average of two frames with m_filter), then clears
//       the other slot and switches to it
//     speed = |movement| / frame_msec (counts per millisecond); sensitivity = (speed * cl_mouseAccel +
//       cl_sensitivity) * cgameFOVSensitivityScale (the ADS zoom scale cgame sets)
//     yaw -= m_yaw * mx * sensitivity, pitch += m_pitch * my * sensitivity, each limited per frame by
//       cgameMaxYawSpeed / cgameMaxPitchSpeed (degrees per second) when cgame sets one
//     then mouse aim assist (AimAssist_UpdateMouseInput) adjusts the angles
// Findings (records/bugs.tsv): mouse movement is consumed even on frames where frame_msec is 0 and then
// dropped; acceleration and the speed caps use whole milliseconds; uninitialized melee-charge output is
// copied into the usercmd when aim assist isn't initialized.
#define T5_SOURCE "client_mp/cl_input_mp.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\client_mp\\cl_input_mp.cpp"  // __FILE__ in the original build

#include "client_mp/client_mp.h"
#include "aim_assist/aim_assist.h"
#include "universal/com_math.h"

#include <emmintrin.h>
#include <t5_assert.h>
#include <t5_compiler.h>
#include <t5_static.h>

T5_STATIC kbutton_t playersKb[1][NUM_BUTTONS] T5_AT("", "playersKb");

// The original converts the strafe and forward mouse movement with the x87 FPU: fld the float, fadd the
// double 2^-30, fistp with the default round-to-nearest. The tiny bias tips exact .5 cases upward. SSE2
// does the same: double add, then cvtsd2si (round to nearest), including 0x80000000 for out-of-range values.
static inline int CL_MouseMoveToInt(float f)
{
	return _mm_cvtsd_si32(_mm_set_sd((double)f + 9.313225746154785e-10));
}

// 00551BF0  cl_input_mp.cpp:1741
int CL_MouseEvent(int x, int y, int dx, int dy)
{
	clientUIActive_t *clUI;
	clientActive_t *cl;

	if (DevGui_IsActive())
	{
		DevGui_MouseEvent(dx, dy);
		return 1;
	}
	clUI = CL_GetLocalClientUIGlobals(0);
	cl = CL_GetLocalClientGlobals(0);
	if (!cl)
		return 0;
	if ((clUI->keyCatchers & KEYCATCH_UI) && !UI_KeysBypassMenu(0))
	{
		UI_MouseEvent(0, x, y);
		return 0;
	}
	CL_ShowSystemCursor(0);
	cl->mouseDx[cl->mouseIndex] += dx;
	cl->mouseDy[cl->mouseIndex] += dy;
	return 1;
}

// 00551CE0  cl_input_mp.cpp:1787
void CL_SetCursorPos(int x, int y)
{
	IN_SetCursorPos(x, y);
}

// 00554630  cl_input_mp.cpp:2147
void CL_GetMouseMovement(clientActive_t *cl, float *mx, float *my)
{
	iassert_at(2148, mx);
	iassert_at(2149, my);
	if (m_filter->current.enabled)
	{
		*mx = (float)(cl->mouseDx[0] + cl->mouseDx[1]) * 0.5f;
		*my = (float)(cl->mouseDy[0] + cl->mouseDy[1]) * 0.5f;
	}
	else
	{
		*mx = (float)cl->mouseDx[cl->mouseIndex];
		*my = (float)cl->mouseDy[cl->mouseIndex];
	}
	cl->mouseIndex ^= 1;
	cl->mouseDx[cl->mouseIndex] = 0;
	cl->mouseDy[cl->mouseIndex] = 0;
}

// 00553F90  cl_input_mp.cpp:2168
// VC90 does the unsigned-to-float conversions of frame_msec on the x87 stack (fild of a zero-extended
// qword) and keeps the arithmetic there until the store, so those expressions are written in double below:
// a double divide or multiply rounded once to float is what the x87 code computes.
void CL_MouseMove(int localClientNum, usercmd_s *cmd)
{
	clientActive_t *cl;
	float mx;
	float my;
	float rate;
	float accelSensitivity;
	int yawMove;
	int pitchMove;
	float moveScale;
	kbutton_t *kb;
	float delta;
	float cap;
	AimInput aimInput;
	AimOutput aimOutput;
	int i;

	cl = CL_GetLocalClientGlobals(localClientNum);
	CL_GetMouseMovement(cl, &mx, &my);
	if (!frame_msec)
		return;
	rate = (float)((double)__builtin_elementwise_sqrt(mx * mx + my * my) / (double)frame_msec);
	accelSensitivity = rate * cl_mouseAccel->current.value + cl_sensitivity->current.value;
	accelSensitivity = accelSensitivity * cl->cgameFOVSensitivityScale;
	if (rate != 0.0f && cl_showMouseRate->current.enabled)
		Com_Printf(14, "%f : %f\n", rate, accelSensitivity);
	if ((cl->snap.ps.pm_flags & PMF_FROZEN_NO_LOOK) || cl->snap.ps.weaponstate == WEAPON_STATE_NO_LOOK)
		return;
	mx = mx * accelSensitivity;
	my = my * accelSensitivity;
	if (mx != 0.0f || my != 0.0f)
	{
		yawMove = 0;
		pitchMove = 0;
		moveScale = 127.0f;
		kb = playersKb[localClientNum];
		if (kb[KB_STRAFE].active)
		{
			cmd->rightmove = ClampChar(cmd->rightmove + CL_MouseMoveToInt(m_side->current.value * mx));
		}
		else
		{
			delta = m_yaw->current.value * mx;
			if (cl->cgameMaxYawSpeed > 0.0f)
			{
				cap = (float)((double)frame_msec * (double)cl->cgameMaxYawSpeed * (double)0.001f);
				delta = I_fmax(-cap, I_fmin(delta, cap));
			}
			cl->viewangles[1] = cl->viewangles[1] - delta;
			yawMove = -(int)I_fmin(moveScale, mx);
		}
		if ((!kb[KB_MLOOK].active && !cl_freelook->current.enabled) || kb[KB_STRAFE].active)
		{
			cmd->forwardmove = ClampChar(cmd->forwardmove - CL_MouseMoveToInt(m_forward->current.value * my));
		}
		else
		{
			delta = m_pitch->current.value * my;
			if (cl->cgameMaxPitchSpeed > 0.0f)
			{
				cap = (float)((double)frame_msec * (double)cl->cgameMaxPitchSpeed * (double)0.001f);
				delta = I_fmax(-cap, I_fmin(delta, cap));
			}
			cl->viewangles[0] = cl->viewangles[0] + delta;
			pitchMove = (int)I_fmin(moveScale, my);
		}
		cmd->pitchmove = ClampChar(cmd->pitchmove + pitchMove);
		cmd->yawmove = ClampChar(cmd->yawmove + yawMove);
	}
	if (cl->snap.ps.eFlags2 & EF2_NO_AIM_ASSIST)
		return;
	for (i = 0; i < 2; i++)
		aimInput.button_bits.array[i] = 0;
	aimInput.deltaTime = (float)cls.frametime * 0.001f;
	aimInput.pitch = cl->viewangles[0];
	aimInput.pitchAxis = 0.0f;
	aimInput.pitchMax = 0.0f;
	aimInput.yaw = cl->viewangles[1];
	aimInput.yawAxis = 0.0f;
	aimInput.yawMax = 0.0f;
	aimInput.forwardAxis = 0.0f;
	aimInput.rightAxis = 0.0f;
	for (i = 0; i < 2; i++)
		aimInput.button_bits.array[i] = cmd->button_bits.array[i];
	aimInput.localClientNum = 0;
	AimAssist_UpdateMouseInput(&aimInput, &aimOutput);
	cl->viewangles[0] = aimOutput.pitch;
	cl->viewangles[1] = aimOutput.yaw;
	cmd->meleeChargeYaw = aimOutput.meleeChargeYaw;
	cmd->meleeChargeDist = aimOutput.meleeChargeDist;
}
