// universal/com_math.cpp
//
// Reconstruction of codsrc/src/universal/com_math.cpp, in original line order.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv.
#define T5_SOURCE "universal/com_math.cpp"
#define T5_ASSERT_FILE "C:\\projects_pc\\cod\\codsrc\\src\\universal\\com_math.cpp"  // __FILE__ in the original build

#include "universal/com_math.h"

#include <math.h>
#include <t5_assert.h>
#include <t5_compiler.h>
#include <t5_static.h>

T5_STATIC unsigned int holdrand T5_AT("", "holdrand");

T5_STATIC float sRGBtoXYZ[4][4] T5_AT("", "sRGBtoXYZ") T5_INIT(= {
	{ 0.4124f, 0.2126f, 0.0193f, 0.0f },
	{ 0.3576f, 0.7152f, 0.1192f, 0.0f },
	{ 0.1805f, 0.0722f, 0.9505f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
});
T5_STATIC float bradfordMA[4][4] T5_AT("", "bradfordMA") T5_INIT(= {
	{ 0.8951f, -0.7502f, 0.0389f, 0.0f },
	{ 0.2664f, 1.7135f, -0.0685f, 0.0f },
	{ -0.1614f, 0.0367f, 1.0296f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
});
T5_STATIC float bradfordMI[4][4] T5_AT("", "bradfordMI") T5_INIT(= {
	{ 0.986993f, 0.432305f, -0.008529f, 0.0f },
	{ -0.147054f, 0.51836f, 0.040043f, 0.0f },
	{ 0.159963f, 0.049291f, 0.968487f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
});
T5_STATIC float XYZtosRGB[4][4] T5_AT("", "XYZtosRGB") T5_INIT(= {
	{ 3.24071f, -0.969258f, 0.0556352f, 0.0f },
	{ -1.53726f, 1.87599f, -0.203996f, 0.0f },
	{ -0.498571f, 0.0415557f, 1.05707f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
});

extern "C" void _CIatan(void);

// 007A7DE0  com_math.cpp:151
void TRACK_com_math()
{
	track_static_alloc_internal(bytedirs, sizeof(bytedirs), "bytedirs", 11);
}

// 007A7E00  com_math.cpp:158
X87_MATH float random()
{
	return (float)rand() / 32768.0f;  // fild / fdiv, returned in ST0
}

// 007A7E20  com_math.cpp:163
X87_MATH float crandom()
{
	return random() * 2.0f - 1.0f;  // on the x87 stack straight from random()'s ST0
}

// 007A7E40  com_math.cpp:168
void GaussianRandom(float *f0, float *f1)
{
	float x;
	float y;
	float w;

	iassert_at(173, f0);
	iassert_at(174, f1);
	do
	{
		x = crandom();
		y = crandom();
		w = x * x + y * y;
	} while (w > 1.0f);
	w = __builtin_elementwise_sqrt(-2.0f * logf(w) / w);
	*f0 = x * w;
	*f1 = y * w;
}

// 007A7F30  com_math.cpp:190
int RandWithSeed(int *seed)
{
	*seed = 1103515245 * *seed + 12345;
	return (unsigned int)(*seed / 65536) % 32768;
}

// 007A7F70  com_math.cpp:197
X87_MATH float randomhash(int value)
{
	return (float)RandWithSeed(&value) / 32768.0f;
}

// 007A7F90  com_math.cpp:204
void PointInCircleFromUniformDeviates(float radiusDeviate, float yawDeviate, float point[2])
{
	float r;
	float rSq;
	float yaw;
	float sinYaw;
	float cosYaw;

	rSq = radiusDeviate;
	r = __builtin_elementwise_sqrt(rSq);
	yaw = 6.2831855f * yawDeviate;
	FastSinCos(yaw, &sinYaw, &cosYaw);
	point[0] = r * cosYaw;
	point[1] = r * sinYaw;
}

// The track functions are declared `float`. VC90 returns `cur + step` (and AngleNormalize180's result)
// from the x87 stack without rounding to float; defining them with a double result under the same
// decorated name does the same thing, as PitchForYawOnNormal does. Everything else is SSE as in the original.
double AngleNormalize180_x87(const float angle) __asm__("?AngleNormalize180@@YAMM@Z");

// 007A8010  com_math.cpp:379
double LinearTrack_x87(float tgt, float cur, float rate, float deltaTime) __asm__("?LinearTrack@@YAMMMMM@Z");
double LinearTrack_x87(float tgt, float cur, float rate, float deltaTime)
{
	float err;
	float step;

	err = tgt - cur;
	if (err > 0.0f)
		step = rate * deltaTime;
	else
		step = -rate * deltaTime;
	if (I_fabs(err) > 0.001f)
	{
		if (I_fabs(step) > I_fabs(err))
			return tgt;
		return (double)cur + (double)step;  // fld cur / fadd step
	}
	return tgt;
}

// 007A80B0  com_math.cpp:406
double LinearTrackAngle_x87(float tgt, float cur, float rate, float deltaTime) __asm__("?LinearTrackAngle@@YAMMMMM@Z");
double LinearTrackAngle_x87(float tgt, float cur, float rate, float deltaTime)
{
	float angle;

	while (tgt - cur > 180.0f)
		tgt = tgt - 360.0f;
	while (tgt - cur < -180.0f)
		tgt = tgt + 360.0f;
	angle = LinearTrack(tgt, cur, rate, deltaTime);
	return AngleNormalize180_x87(angle);
}

// 007A8150  com_math.cpp:425
double DiffTrack_x87(float tgt, float cur, float rate, float deltaTime) __asm__("?DiffTrack@@YAMMMMM@Z");
double DiffTrack_x87(float tgt, float cur, float rate, float deltaTime)
{
	float err;
	float step;

	err = tgt - cur;
	step = rate * err * deltaTime;
	if (I_fabs(err) > 0.001f)
	{
		if (I_fabs(step) > I_fabs(err))
			return tgt;
		return (double)cur + (double)step;
	}
	return tgt;
}

// 007A81D0  com_math.cpp:448
double DiffTrackAngle_x87(float tgt, float cur, float rate, float deltaTime) __asm__("?DiffTrackAngle@@YAMMMMM@Z");
double DiffTrackAngle_x87(float tgt, float cur, float rate, float deltaTime)
{
	float angle;

	while (tgt - cur > 180.0f)
		tgt = tgt - 360.0f;
	while (tgt - cur < -180.0f)
		tgt = tgt + 360.0f;
	angle = DiffTrack(tgt, cur, rate, deltaTime);
	return AngleNormalize180_x87(angle);
}

// 007A8270  com_math.cpp:462
float GraphGetValueFromFraction(const int knotCount, const float (*knots)[2], const float fraction)
{
	int knotIndex;
	float adjustedFrac;
	float result;

	result = -1.0f;
	iassert_at(467, knots);
	vassertp_at(468, knotCount >= 2, "(knotCount) = %i", knotCount);
	vassertp_at(469, fraction >= 0.0f && fraction <= 1.0f, "(fraction) = %g", fraction);
	vassertp_at(470, knots[knotCount - 1][0] == 1.0f, "(knots[knotCount - 1][0]) = %g", knots[knotCount - 1][0]);

	for (knotIndex = 1; knotIndex < knotCount; ++knotIndex)
	{
		if (knots[knotIndex][0] >= fraction)
		{
			adjustedFrac = (fraction - knots[knotIndex - 1][0]) / (knots[knotIndex][0] - knots[knotIndex - 1][0]);
			vassertp_at(478, adjustedFrac >= 0.0f && adjustedFrac <= 1.0f, "(adjustedFrac) = %g", adjustedFrac);
			vassertp_at(479, knots[knotIndex - 1][1] >= 0.0f && knots[knotIndex - 1][1] <= 1.0f,
				"(knots[knotIndex - 1][1]) = %g", knots[knotIndex - 1][1]);
			vassertp_at(480, knots[knotIndex][1] >= 0.0f && knots[knotIndex][1] <= 1.0f,
				"(knots[knotIndex][1]) = %g", knots[knotIndex][1]);
			result = (knots[knotIndex][1] - knots[knotIndex - 1][1]) * adjustedFrac + knots[knotIndex - 1][1];
			break;
		}
	}

	vassertp_at(487, result >= 0.0f && result <= 1.0f, "(result) = %g", result);
	return result;
}

// 007A85A0  com_math.cpp:522
float Q_acos(const float c)
{
	float angle;

	angle = acosf(c);
	if (angle > 3.14159265f)
		return 3.14159265f;
	if (angle < -3.14159265f)
		return 3.14159265f;  // sic: +pi on both sides
	return angle;
}

// 007A85F0  com_math.cpp:544
signed char ClampChar(const int i)
{
	if (i < -128)
		return -128;
	if (i > 127)
		return 127;
	return i;
}

// 007A8610  com_math.cpp:560
short ClampShort(const int i)
{
	if (i < -32768)
		return -32768;
	if (i > 32767)
		return 32767;
	return i;
}

// 007A8640  com_math.cpp:577
unsigned char DirToByte(const float dir[3])
{
	float bestd;
	unsigned char i;
	unsigned char best;
	float d;

	if (!dir)
		return 0;

	bestd = 0.0f;
	best = 0;
	for (i = 0; i < 162; ++i)
	{
		d = dir[0] * bytedirs[i][0] + dir[1] * bytedirs[i][1] + dir[2] * bytedirs[i][2];
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	return best;
}

// 007A86F0  com_math.cpp:605
void ByteToDir(const int b, float dir[3])
{
	if (b < 0 || b >= 162)
	{
		dir[0] = vec3_origin[0];
		dir[1] = vec3_origin[1];
		dir[2] = vec3_origin[2];
	}
	else
	{
		dir[0] = bytedirs[b][0];
		dir[1] = bytedirs[b][1];
		dir[2] = bytedirs[b][2];
	}
}

// 007A8770  com_math.cpp:616
int VecNCompareCustomEpsilon(const float v0[], const float v1[], float epsilon, int coordCount)
{
	int i;

	for (i = 0; i < coordCount; ++i)
	{
		if ((v0[i] - v1[i]) * (v0[i] - v1[i]) > epsilon * epsilon)
			return 0;
	}
	return 1;
}

// 007A87E0  com_math.cpp:670
void Vec3ProjectionCoords(const float dir[3], int *xCoord, int *yCoord)
{
	float dirSq[3];

	dirSq[0] = dir[0] * dir[0];
	dirSq[1] = dir[1] * dir[1];
	dirSq[2] = dir[2] * dir[2];
	if (dirSq[2] >= dirSq[0] && dirSq[2] >= dirSq[1])
	{
		if (dir[2] > 0.0f)
		{
			*xCoord = 0;
			*yCoord = 1;
		}
		else
		{
			*xCoord = 1;
			*yCoord = 0;
		}
	}
	else if (dirSq[1] >= dirSq[0] && dirSq[1] >= dirSq[2])
	{
		if (dir[1] > 0.0f)
		{
			*xCoord = 2;
			*yCoord = 0;
		}
		else
		{
			*xCoord = 0;
			*yCoord = 2;
		}
	}
	else
	{
		if (dir[0] > 0.0f)
		{
			*xCoord = 1;
			*yCoord = 2;
		}
		else
		{
			*xCoord = 2;
			*yCoord = 1;
		}
	}
}

// 007A8900  com_math.cpp:723
void RotatePointAroundVector(float dst[3], const float dir[3], const float point[3], const float degrees)
{
	float im[3][3];
	float tmpmat[3][3];
	int i;
	float vup[3];
	float vf[3];
	float rot[3][3];
	float zrot[3][3];
	float vr[3];
	float rad;
	float m[3][3];

	iassert_at(733, dir[0] || dir[1] || dir[2]);

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(dir, vr);
	Vec3Cross(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0f;

	rad = degrees * (3.14159265f / 180.0f);
	iassert_at(768, !IS_NAN(rad));
	FastSinCos(rad, &zrot[0][1], &zrot[0][0]);
	iassert_at(772, !IS_NAN(zrot[0][1]));
	iassert_at(773, !IS_NAN(zrot[0][0]));
	zrot[1][0] = -zrot[0][1];
	zrot[1][1] = zrot[0][0];

	MatrixMultiply(m, zrot, tmpmat);
	MatrixMultiply(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
}

// 007A8D10  com_math.cpp:815
void Vec3Basis_RightHanded(const float forward[3], float left[3], float up[3])
{
	PerpendicularVector(forward, up);
	Vec3Cross(up, forward, left);
}

// 007A8D40  com_math.cpp:826
float vectoyaw(const float vec[2])
{
	float yaw;

	if (vec[1] == 0.0f && vec[0] == 0.0f)
	{
		yaw = 0.0f;
	}
	else
	{
		yaw = atan2f(vec[1], vec[0]) * 180.0f / 3.14159265f;
		yaw += yaw < 0.0f ? 360.0f : 0.0f;  // always adds: turns -0 into +0, as the original does
	}
	return yaw;
}

// 007A8E00  com_math.cpp:848
float vectosignedyaw(const float vec[2])
{
	float yaw;

	if (vec[1] == 0.0f && vec[0] == 0.0f)
	{
		yaw = 0.0f;
	}
	else
	{
		yaw = atan2f(vec[1], vec[0]) * 180.0f / 3.14159265f;
		iassert_at(859, yaw >= -180);
		iassert_at(860, yaw <= 180);
	}
	return yaw;
}

// 007A8F00  com_math.cpp:872
float vectopitch(const float vec[3])
{
	float pitch;
	float forward;

	if (vec[1] == 0.0f && vec[0] == 0.0f)
	{
		pitch = I_fsel(-vec[2], 90.0f, 270.0f);
	}
	else
	{
		forward = __builtin_elementwise_sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
		pitch = atan2f(vec[2], forward) * -180.0f / 3.14159265f;
		pitch += pitch < 0.0f ? 360.0f : 0.0f;
	}
	return pitch;
}

// 007A9030  com_math.cpp:896
void vectoangles(const float vec[3], float angles[3])
{
	float forward;
	float yaw;
	float pitch;

	if (vec[1] == 0.0f && vec[0] == 0.0f)
	{
		yaw = 0.0f;
		pitch = I_fsel(-vec[2], 90.0f, 270.0f);
	}
	else
	{
		yaw = atan2f(vec[1], vec[0]) * 180.0f / 3.14159265f;
		yaw += yaw < 0.0f ? 360.0f : 0.0f;

		forward = __builtin_elementwise_sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
		pitch = atan2f(vec[2], forward) * -180.0f / 3.14159265f;
		pitch += pitch < 0.0f ? 360.0f : 0.0f;
	}
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0.0f;
}

// 007A9210  com_math.cpp:921
void UnitQuatToAngles(const float quat[4], float angles[3])
{
	float axis[3][3];

	UnitQuatToAxis(quat, axis);
	AxisToAngles(axis, angles);
}

// 007A9240  com_math.cpp:934
void YawVectors(float yaw, float forward[3], float right[3])
{
	float sy;
	float angle;
	float cy;

	angle = yaw * (3.14159265f / 180.0f);
	FastSinCos(angle, &sy, &cy);
	if (forward)
	{
		forward[0] = cy;
		forward[1] = sy;
		forward[2] = 0.0f;
	}
	if (right)
	{
		right[0] = sy;
		right[1] = -cy;
		right[2] = 0.0f;
	}
}

// 007A92E0  com_math.cpp:959
void YawVectors2D(const float yaw, float forward[2], float right[2])
{
	float sy;
	float angle;
	float cy;

	angle = yaw * (3.14159265f / 180.0f);
	FastSinCos(angle, &sy, &cy);
	if (forward)
	{
		forward[0] = cy;
		forward[1] = sy;
	}
	if (right)
	{
		right[0] = sy;
		right[1] = -cy;
	}
}

// 007A9360  com_math.cpp:987
void PerpendicularVector(const float src[3], float dst[3])
{
	float srcSq[3];
	float d;
	int pos;

	iassertmsg_at(992, Vec3IsNormalized( src ), va("(%g %g %g) len %g", src[0], src[1], src[2], Vec3Length(src)));

	srcSq[0] = src[0] * src[0];
	srcSq[1] = src[1] * src[1];
	srcSq[2] = src[2] * src[2];
	pos = srcSq[0] > srcSq[1];
	if (srcSq[pos] > srcSq[2])
		pos = 2;

	d = -src[pos];
	dst[0] = d * src[0];
	dst[1] = d * src[1];
	dst[2] = d * src[2];
	dst[pos] = dst[pos] + 1.0f;
	Vec3Normalize(dst);
}

// 007A94E0  com_math.cpp:1068
float PointToLineDistSq(const float point[3], const float start[3], const float end[3])
{
	float ptToLineDot;
	float ptToLine[3];
	float seg[3];
	float segDot;
	float perpendicularDelta[3];

	seg[0] = end[0] - start[0];
	seg[1] = end[1] - start[1];
	seg[2] = end[2] - start[2];
	ptToLine[0] = point[0] - start[0];
	ptToLine[1] = point[1] - start[1];
	ptToLine[2] = point[2] - start[2];
	ptToLineDot = ptToLine[0] * seg[0] + ptToLine[1] * seg[1] + ptToLine[2] * seg[2];
	segDot = seg[0] * seg[0] + seg[1] * seg[1] + seg[2] * seg[2];
	iassert_at(1082, segDot);
	Vec3Mad(ptToLine, -(ptToLineDot / segDot), seg, perpendicularDelta);
	// return Vec3LengthSq(perpendicularDelta);
	// VC90 evaluates this on the x87 stack and returns it unrounded in ST0, written out as in PointToLineDistSq2D.
	__asm
	{
		fld perpendicularDelta[8]
		fmul perpendicularDelta[8]
		fld perpendicularDelta[4]
		fmul perpendicularDelta[4]
		fld perpendicularDelta[0]
		fmul perpendicularDelta[0]
		faddp st(1), st
		faddp st(1), st
		mov esp, ebp
		pop ebp
		ret
	}
}

// 007A9660  com_math.cpp:1094
float PointToLineDistSq2D(const float point[2], const float start[2], const float end[2])
{
	float ptToLineDot;
	float ptToLine[2];
	float seg[2];
	float segDot;
	float perpendicularDelta[2];
	float scale;

	seg[0] = end[0] - start[0];
	seg[1] = end[1] - start[1];
	ptToLine[0] = point[0] - start[0];
	ptToLine[1] = point[1] - start[1];
	ptToLineDot = ptToLine[0] * seg[0] + ptToLine[1] * seg[1];
	segDot = seg[0] * seg[0] + seg[1] * seg[1];
	iassert_at(1108, segDot);
	scale = -(ptToLineDot / segDot);
	perpendicularDelta[0] = ptToLine[0] + scale * seg[0];
	perpendicularDelta[1] = ptToLine[1] + scale * seg[1];
	// return perpendicularDelta[0] * perpendicularDelta[0] + perpendicularDelta[1] * perpendicularDelta[1];
	// VC90 evaluates this return expression on the x87 stack and returns it unrounded in ST0. A C++
	// float return always rounds to float, so the same x87 sequence is written out here.
	__asm
	{
		fld perpendicularDelta[4]
		fmul perpendicularDelta[4]
		fld perpendicularDelta[0]
		fmul perpendicularDelta[0]
		faddp st(1), st
		mov esp, ebp
		pop ebp
		ret
	}
}

// 007A9780  com_math.cpp:1180
int BoxInPlanes(const float (*planes)[4], unsigned int numPlanes, const float mins[3], const float maxs[3])
{
	unsigned int planeIndex;
	float inside;
	const float *__restrict plane;

	for (planeIndex = 0; planeIndex < numPlanes; ++planeIndex)
	{
		plane = planes[planeIndex];
		inside = 0.0f;
		inside += plane[0] * mins[0] + plane[1] * mins[1] + plane[2] * mins[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		inside += plane[0] * mins[0] + plane[1] * mins[1] + plane[2] * maxs[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		inside += plane[0] * mins[0] + plane[1] * maxs[1] + plane[2] * mins[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		inside += plane[0] * mins[0] + plane[1] * maxs[1] + plane[2] * maxs[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		inside += plane[0] * maxs[0] + plane[1] * mins[1] + plane[2] * mins[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		inside += plane[0] * maxs[0] + plane[1] * mins[1] + plane[2] * maxs[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		inside += plane[0] * maxs[0] + plane[1] * maxs[1] + plane[2] * mins[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		inside += plane[0] * maxs[0] + plane[1] * maxs[1] + plane[2] * maxs[2] + plane[3] >= 0.0f ? 0.0f : 1.0f;
		if (inside == 0.0f)
			return 0;
	}
	return 1;
}

// 007A9B80  com_math.cpp:1204
int SphereInPlanes(const float (*planes)[4], unsigned int numPlanes, const float center[3], const float radius)
{
	unsigned int planeIndex;
	const float *__restrict plane;

	for (planeIndex = 0; planeIndex < numPlanes; ++planeIndex)
	{
		plane = planes[planeIndex];
		if (plane[0] * center[0] + plane[1] * center[1] + plane[2] * center[2] + plane[3] - radius > 0.0f)
			return 0;
	}
	return 1;
}

// 007A9C10  com_math.cpp:1226
void MatrixIdentity33(float out[3][3])
{
	iassert_at(1227, out);
	memset(out, 0, sizeof(float[3][3]));
	out[0][0] = 1.0f;
	out[1][1] = 1.0f;
	out[2][2] = 1.0f;
}

// 007A9C90  com_math.cpp:1235
void MatrixIdentity44(float out[4][4])
{
	iassert_at(1236, out);
	memcpy(out, identityMatrix44, sizeof(float[4][4]));
}

// 007A9CE0  com_math.cpp:1269
void MatrixMultiply(const float in1[3][3], const float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

// 007A9F20  com_math.cpp:1289
void MatrixVecMultiply(const float mulMat[3][3], const float mulVec[3], float solution[3])
{
	solution[0] = mulMat[0][0] * mulVec[0] + mulMat[0][1] * mulVec[1] + mulMat[0][2] * mulVec[2];
	solution[1] = mulMat[1][0] * mulVec[0] + mulMat[1][1] * mulVec[1] + mulMat[1][2] * mulVec[2];
	solution[2] = mulMat[2][0] * mulVec[0] + mulMat[2][1] * mulVec[1] + mulMat[2][2] * mulVec[2];
}

// 007A9FE0  com_math.cpp:1301
void MatrixVecMultiply(const float mulMat[4][4], const float mulVec[4], float solution[4])
{
	solution[0] = mulMat[0][0] * mulVec[0] + mulMat[0][1] * mulVec[1] + mulMat[0][2] * mulVec[2] + mulMat[0][3] * mulVec[3];
	solution[1] = mulMat[1][0] * mulVec[0] + mulMat[1][1] * mulVec[1] + mulMat[1][2] * mulVec[2] + mulMat[1][3] * mulVec[3];
	solution[2] = mulMat[2][0] * mulVec[0] + mulMat[2][1] * mulVec[1] + mulMat[2][2] * mulVec[2] + mulMat[2][3] * mulVec[3];
	solution[3] = mulMat[3][0] * mulVec[0] + mulMat[3][1] * mulVec[1] + mulMat[3][2] * mulVec[2] + mulMat[3][3] * mulVec[3];
}

// 007AA130  com_math.cpp:1314
void MatrixVecMultiplyProject(const float mulMat[4][4], const float mulVec[3], float solution[3])
{
	float invW;
	float mul[4];
	float ret[4];

	mul[0] = mulVec[0];
	mul[1] = mulVec[1];
	mul[2] = mulVec[2];
	mul[3] = 1.0f;
	MatrixVecMultiply(mulMat, mul, ret);
	invW = 1.0f / ret[3];
	solution[0] = ret[0] * invW;
	solution[1] = ret[1] * invW;
	solution[2] = ret[2] * invW;
}

// 007AA1D0  com_math.cpp:1428
void MatrixMultiplyEquals(const float in[3][3], float out[3][3])
{
	float temp[2][3];

	temp[0][0] = in[0][0] * out[0][0] + in[0][1] * out[1][0] + in[0][2] * out[2][0];
	temp[0][1] = in[0][0] * out[0][1] + in[0][1] * out[1][1] + in[0][2] * out[2][1];
	temp[0][2] = in[0][0] * out[0][2] + in[0][1] * out[1][2] + in[0][2] * out[2][2];
	temp[1][0] = in[1][0] * out[0][0] + in[1][1] * out[1][0] + in[1][2] * out[2][0];
	temp[1][1] = in[1][0] * out[0][1] + in[1][1] * out[1][1] + in[1][2] * out[2][1];
	temp[1][2] = in[1][0] * out[0][2] + in[1][1] * out[1][2] + in[1][2] * out[2][2];
	out[2][0] = in[2][0] * out[0][0] + in[2][1] * out[1][0] + in[2][2] * out[2][0];
	out[2][1] = in[2][0] * out[0][1] + in[2][1] * out[1][1] + in[2][2] * out[2][1];
	out[2][2] = in[2][0] * out[0][2] + in[2][1] * out[1][2] + in[2][2] * out[2][2];
	out[0][0] = temp[0][0];
	out[0][1] = temp[0][1];
	out[0][2] = temp[0][2];
	Vec3Copy(temp[1], out[1]);
}

// 007AA460  com_math.cpp:1448
void MatrixMultiply43(const float in1[4][3], const float in2[4][3], float out[4][3])
{
	iassert_at(1449, in1 != out);
	iassert_at(1450, in2 != out);
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in2[3][0];
	out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in2[3][1];
	out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in2[3][2];
}

// 007AA7E0  com_math.cpp:1487
void MatrixMultiply44(const float in1[4][4], const float in2[4][4], float out[4][4])
{
	iassert_at(1488, in1 != out);
	iassert_at(1489, in2 != out);
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0] + in1[0][3] * in2[3][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1] + in1[0][3] * in2[3][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2] + in1[0][3] * in2[3][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3] * in2[3][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0] + in1[1][3] * in2[3][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1] + in1[1][3] * in2[3][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2] + in1[1][3] * in2[3][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3] * in2[3][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0] + in1[2][3] * in2[3][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1] + in1[2][3] * in2[3][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2] + in1[2][3] * in2[3][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3] * in2[3][3];
	out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in1[3][3] * in2[3][0];
	out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in1[3][3] * in2[3][1];
	out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in1[3][3] * in2[3][2];
	out[3][3] = in1[3][0] * in2[0][3] + in1[3][1] * in2[1][3] + in1[3][2] * in2[2][3] + in1[3][3] * in2[3][3];
}

// 007AAD80  com_math.cpp:1519
void MatrixTranspose(const float in[3][3], float out[3][3])
{
	iassert_at(1520, in != out);
	out[0][0] = in[0][0];
	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][1] = in[1][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
	out[2][2] = in[2][2];
}

// 007AAE20  com_math.cpp:1547
void MatrixTranspose44(const float in[16], float out[16])
{
	iassert_at(1548, in != out);
	out[0] = in[0];
	out[1] = in[4];
	out[2] = in[8];
	out[3] = in[12];
	out[4] = in[1];
	out[5] = in[5];
	out[6] = in[9];
	out[7] = in[13];
	out[8] = in[2];
	out[9] = in[6];
	out[10] = in[10];
	out[11] = in[14];
	out[12] = in[3];
	out[13] = in[7];
	out[14] = in[11];
	out[15] = in[15];
}

// 007AAF20  com_math.cpp:1575
void MatrixInverse(const float in[3][3], float out[3][3])
{
	float det;

	iassert_at(1578, in != out);
	det = (in[2][2] * in[1][1] - in[2][1] * in[1][2]) * in[0][0]
		- (in[2][2] * in[0][1] - in[2][1] * in[0][2]) * in[1][0]
		+ (in[1][2] * in[0][1] - in[1][1] * in[0][2]) * in[2][0];
	iassert_at(1582, det);
	det = 1.0f / det;
	out[0][0] = (in[2][2] * in[1][1] - in[2][1] * in[1][2]) * det;
	out[0][1] = -(in[2][2] * in[0][1] - in[2][1] * in[0][2]) * det;
	out[0][2] = (in[1][2] * in[0][1] - in[1][1] * in[0][2]) * det;
	out[1][0] = -(in[2][2] * in[1][0] - in[2][0] * in[1][2]) * det;
	out[1][1] = (in[2][2] * in[0][0] - in[2][0] * in[0][2]) * det;
	out[1][2] = -(in[1][2] * in[0][0] - in[1][0] * in[0][2]) * det;
	out[2][0] = (in[2][1] * in[1][0] - in[2][0] * in[1][1]) * det;
	out[2][1] = -(in[2][1] * in[0][0] - in[2][0] * in[0][1]) * det;
	out[2][2] = (in[1][1] * in[0][0] - in[1][0] * in[0][1]) * det;
}

// 007AB200  com_math.cpp:1605
void MatrixInverseOrthogonal43(const float in[4][3], float out[4][3])
{
	float origin[3];

	MatrixTranspose(in, out);
	Vec3Sub(vec3_origin, in[3], origin);
	MatrixTransformVector(origin, out, out[3]);
}

// 007AB280  com_math.cpp:1634
void MatrixInverse44(const float mat[16], float dst[16])
{
	int i;
	float tmp[12];
	float det;
	float src[16];

	iassert_at(1640, mat != dst);
	for (i = 0; i < 4; ++i)
	{
		src[i] = mat[i * 4];
		src[i + 4] = mat[i * 4 + 1];
		src[i + 8] = mat[i * 4 + 2];
		src[i + 12] = mat[i * 4 + 3];
	}
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];
	dst[0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
	dst[0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
	dst[1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
	dst[1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
	dst[2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
	dst[2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
	dst[3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
	dst[3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
	dst[4] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
	dst[4] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
	dst[5] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
	dst[5] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
	dst[6] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
	dst[6] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
	dst[7] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
	dst[7] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];
	tmp[0] = src[2] * src[7];
	tmp[1] = src[3] * src[6];
	tmp[2] = src[1] * src[7];
	tmp[3] = src[3] * src[5];
	tmp[4] = src[1] * src[6];
	tmp[5] = src[2] * src[5];
	tmp[6] = src[0] * src[7];
	tmp[7] = src[3] * src[4];
	tmp[8] = src[0] * src[6];
	tmp[9] = src[2] * src[4];
	tmp[10] = src[0] * src[5];
	tmp[11] = src[1] * src[4];
	dst[8] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
	dst[8] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
	dst[9] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
	dst[9] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
	dst[10] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
	dst[10] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
	dst[11] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
	dst[11] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
	dst[12] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
	dst[12] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
	dst[13] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
	dst[13] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
	dst[14] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
	dst[14] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
	dst[15] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
	dst[15] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];
	det = src[0] * dst[0] + src[1] * dst[1] + src[2] * dst[2] + src[3] * dst[3];
	iassert_at(1715, det);
	det = 1.0f / det;
	for (i = 0; i < 16; ++i)
		dst[i] *= det;
}

// 007ABBD0  com_math.cpp:1741
void MatrixTransformVector44(const float vec[4], const float mat[4][4], float out[4])
{
	iassert_at(1742, vec != out);
	out[0] = vec[0] * mat[0][0] + vec[1] * mat[1][0] + vec[2] * mat[2][0] + vec[3] * mat[3][0];
	out[1] = vec[0] * mat[0][1] + vec[1] * mat[1][1] + vec[2] * mat[2][1] + vec[3] * mat[3][1];
	out[2] = vec[0] * mat[0][2] + vec[1] * mat[1][2] + vec[2] * mat[2][2] + vec[3] * mat[3][2];
	out[3] = vec[0] * mat[0][3] + vec[1] * mat[1][3] + vec[2] * mat[2][3] + vec[3] * mat[3][3];
}

// 007ABD50  com_math.cpp:1751
void MatrixTransformVector43(const float in1[3], const float in2[4][3], float out[3])
{
	iassert_at(1752, in1 != out);
	out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0] + in2[3][0];
	out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1] + in2[3][1];
	out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2] + in2[3][2];
}

// 007ABE60  com_math.cpp:1759
void MatrixTransposeTransformVector43(const float in1[3], const float in2[4][3], float out[3])
{
	float temp[3];

	iassert_at(1762, in1 != out);
	Vec3Sub(in1, in2[3], temp);
	out[0] = in2[0][0] * temp[0] + in2[0][1] * temp[1] + in2[0][2] * temp[2];
	out[1] = in2[1][0] * temp[0] + in2[1][1] * temp[1] + in2[1][2] * temp[2];
	out[2] = in2[2][0] * temp[0] + in2[2][1] * temp[1] + in2[2][2] * temp[2];
}

// 007ABF90  com_math.cpp:1770
void MatrixTransformVector43Equals(float *out, const float *in)
{
	float temp[2];

	temp[0] = out[0] * in[0] + out[1] * in[3] + out[2] * in[6] + in[9];
	temp[1] = out[0] * in[1] + out[1] * in[4] + out[2] * in[7] + in[10];
	out[2] = out[0] * in[2] + out[1] * in[5] + out[2] * in[8] + in[11];
	out[0] = temp[0];
	out[1] = temp[1];
}

// 007AC090  com_math.cpp:1782
void VectorAngleMultiply(float vec[2], float angle)
{
	float y;
	float x;
	float temp;
	float radians;  // parameter of the inlined x87 sincos helper

	radians = angle * 0.017453292f;
	__asm
	{
		fld radians
		fsincos
		fstp x
		fstp y
	}
	temp = vec[0] * x - vec[1] * y;
	vec[1] = vec[1] * x + vec[0] * y;
	vec[0] = temp;
}

// 007AC120  com_math.cpp:1795
void UnitQuatToAxis(const float quat[4], float axis[3][3])
{
	float xw;
	float xx;
	float yz;
	float xz;
	float yw;
	float scaledY;
	float scaledX;
	float zw;
	float zz;
	float scaledZ;
	float xy;
	float yy;

	iassertmsg_at(1799, Vec4IsNormalized( quat ), va("%g %g %g %g", quat[0], quat[1], quat[2], quat[3]));
	scaledX = quat[0] + quat[0];
	xx = scaledX * quat[0];
	xy = scaledX * quat[1];
	xz = scaledX * quat[2];
	xw = scaledX * quat[3];
	scaledY = quat[1] + quat[1];
	yy = scaledY * quat[1];
	yz = scaledY * quat[2];
	yw = scaledY * quat[3];
	scaledZ = quat[2] + quat[2];
	zw = scaledZ * quat[3];
	zz = scaledZ * quat[2];
	axis[0][0] = 1.0f - (yy + zz);
	axis[0][1] = xy + zw;
	axis[0][2] = xz - yw;
	axis[1][0] = xy - zw;
	axis[1][1] = 1.0f - (xx + zz);
	axis[1][2] = yz + xw;
	axis[2][0] = xz + yw;
	axis[2][1] = yz - xw;
	axis[2][2] = 1.0f - (xx + yy);
}

// 007AC350  com_math.cpp:1830
void UnitQuatToForward(const float quat[4], float forward[3])
{
	iassertmsg_at(1831, Vec4IsNormalized( quat ), va("%g %g %g %g", quat[0], quat[1], quat[2], quat[3]));
	forward[0] = 1.0f - (quat[1] * quat[1] + quat[2] * quat[2]) * 2.0f;
	forward[1] = (quat[0] * quat[1] + quat[2] * quat[3]) * 2.0f;
	forward[2] = (quat[0] * quat[2] - quat[1] * quat[3]) * 2.0f;
}

// 007AC480  com_math.cpp:1839
void QuatSlerp(const float from[4], const float to[4], float frac, float result[4])
{
	bool dotWasNeg;
	float dot;
	float scaleFrom;
	float scaleTo;
	float sinTheta;
	float theta;

	dot = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];
	if (dot < 0.0f)
	{
		dotWasNeg = true;
		dot = dot * -1.0f;
	}
	else
	{
		dotWasNeg = false;
	}
	if (dot > 0.95f)
	{
		scaleFrom = 1.0f - frac;
		scaleTo = frac;
	}
	else
	{
		theta = acosf(dot);
		sinTheta = sinf(theta);
		scaleFrom = sinf((1.0f - frac) * theta) / sinTheta;
		scaleTo = sinf(frac * theta) / sinTheta;
	}
	if (dotWasNeg)
	{
		result[0] = scaleFrom * from[0] + scaleTo * to[0] * -1.0f;
		result[1] = scaleFrom * from[1] + scaleTo * to[1] * -1.0f;
		result[2] = scaleFrom * from[2] + scaleTo * to[2] * -1.0f;
		result[3] = scaleFrom * from[3] + scaleTo * to[3] * -1.0f;
	}
	else
	{
		result[0] = scaleFrom * from[0] + scaleTo * to[0];
		result[1] = scaleFrom * from[1] + scaleTo * to[1];
		result[2] = scaleFrom * from[2] + scaleTo * to[2];
		result[3] = scaleFrom * from[3] + scaleTo * to[3];
	}
}

// 007AC700  com_math.cpp:1953
float RotationToYaw(const float rot[2])
{
	float zz;
	float r;
	float yaw;

	zz = rot[0] * rot[0];
	r = rot[1] * rot[1] + zz;
	iassert_at(1960, r);
	r = 2.0f / r;
	yaw = atan2f(rot[0] * rot[1] * r, 1.0f - zz * r) * (180.0f / 3.14159265f);
	return yaw;
}

// 007AC7E0  com_math.cpp:2018
void MatrixRotationZ(float mat[3][3], float degree)
{
	float s;
	float c;

	FastSinCos(degree * (3.14159265f / 180.0f), &s, &c);
	mat[0][0] = c;
	mat[0][1] = -s;
	mat[0][2] = 0.0f;
	mat[1][0] = s;
	mat[1][1] = c;
	mat[1][2] = 0.0f;
	mat[2][0] = 0.0f;
	mat[2][1] = 0.0f;
	mat[2][2] = 1.0f;
}

// 007AC8B0  com_math.cpp:2036
void FinitePerspectiveMatrix(float tanHalfFovX, float tanHalfFovY, float zNear, float zFar, float mtx[4][4])
{
	iassert_at(2037, mtx);
	vassert_at(2038, zNear > 0.0f, "%g, %g", zNear, 0.0f);
	vassert_at(2039, zFar > zNear, "%g, %g", zFar, zNear);

	memset(mtx, 0, sizeof(float[4][4]));
	mtx[0][0] = 1.0f / tanHalfFovX;
	mtx[1][1] = 1.0f / tanHalfFovY;
	mtx[2][2] = -zFar / (zNear - zFar);
	mtx[2][3] = 1.0f;
	mtx[3][2] = zNear * zFar / (zNear - zFar);
}

// 007ACA00  com_math.cpp:2061
void SpotLightViewMatrix(const float direction[3], float rotation, float mtx[4][4])
{
	float axis[3][3];
	float lookAtMatrix[4][4];
	float rotationMatrix[4][4];
	float s;
	float c;

	axis[0][0] = -direction[0];
	axis[0][1] = -direction[1];
	axis[0][2] = -direction[2];
	PerpendicularVector(axis[0], axis[2]);
	Vec3Cross(axis[2], axis[0], axis[1]);

	lookAtMatrix[0][0] = -axis[1][0];
	lookAtMatrix[1][0] = -axis[1][1];
	lookAtMatrix[2][0] = -axis[1][2];
	lookAtMatrix[3][0] = 0.0f;
	lookAtMatrix[0][1] = axis[2][0];
	lookAtMatrix[1][1] = axis[2][1];
	lookAtMatrix[2][1] = axis[2][2];
	lookAtMatrix[3][1] = 0.0f;
	lookAtMatrix[0][2] = axis[0][0];
	lookAtMatrix[1][2] = axis[0][1];
	lookAtMatrix[2][2] = axis[0][2];
	lookAtMatrix[3][2] = 0.0f;
	lookAtMatrix[0][3] = 0.0f;
	lookAtMatrix[1][3] = 0.0f;
	lookAtMatrix[2][3] = 0.0f;
	lookAtMatrix[3][3] = 1.0f;

	memset(rotationMatrix, 0, sizeof(rotationMatrix));
	s = sinf(rotation);
	c = cosf(rotation);
	rotationMatrix[0][0] = c;
	rotationMatrix[0][1] = -s;
	rotationMatrix[1][0] = s;
	rotationMatrix[1][1] = c;
	rotationMatrix[2][2] = 1.0f;
	rotationMatrix[3][3] = 1.0f;

	MatrixMultiply44(lookAtMatrix, rotationMatrix, mtx);
}

// 007ACC10  com_math.cpp:2107
void SpotLightViewMatrixDir3(const float dirx[3], const float diry[3], const float dirz[3], float mtx[4][4])
{
	float rotation;
	float axis[3][3];
	float lookAtMatrix[4][4];
	float rotationMatrix[4][4];
	float s;
	float c;

	rotation = 3.14159265f;
	axis[0][0] = dirx[0];
	axis[0][1] = dirx[1];
	axis[0][2] = dirx[2];
	axis[1][0] = diry[0];
	axis[1][1] = diry[1];
	axis[1][2] = diry[2];
	axis[2][0] = dirz[0];
	axis[2][1] = dirz[1];
	axis[2][2] = dirz[2];

	lookAtMatrix[0][0] = axis[1][0];
	lookAtMatrix[1][0] = axis[1][1];
	lookAtMatrix[2][0] = axis[1][2];
	lookAtMatrix[3][0] = 0.0f;
	lookAtMatrix[0][1] = axis[2][0];
	lookAtMatrix[1][1] = axis[2][1];
	lookAtMatrix[2][1] = axis[2][2];
	lookAtMatrix[3][1] = 0.0f;
	lookAtMatrix[0][2] = axis[0][0];
	lookAtMatrix[1][2] = axis[0][1];
	lookAtMatrix[2][2] = axis[0][2];
	lookAtMatrix[3][2] = 0.0f;
	lookAtMatrix[0][3] = 0.0f;
	lookAtMatrix[1][3] = 0.0f;
	lookAtMatrix[2][3] = 0.0f;
	lookAtMatrix[3][3] = 1.0f;

	memset(rotationMatrix, 0, sizeof(rotationMatrix));
	s = sinf(rotation);
	c = cosf(rotation);
	rotationMatrix[0][0] = c;
	rotationMatrix[0][1] = -s;
	rotationMatrix[1][0] = s;
	rotationMatrix[1][1] = c;
	rotationMatrix[2][2] = 1.0f;
	rotationMatrix[3][3] = 1.0f;

	MatrixMultiply44(lookAtMatrix, rotationMatrix, mtx);
}

// 007ACE30  com_math.cpp:2201
void SpotLightProjectionMatrix(float cosFov, float zNear, float zFar, float mtx[4][4])
{
	float nearPlane;
	float Q;
	float cotanFov;

	memset(mtx, 0, sizeof(float[4][4]));
	nearPlane = zNear < 0.001f ? 0.001f : zNear;
	Q = zFar / (zFar - nearPlane);
	// VC90 emitted sqrtss here (intrinsic float square root)
	cotanFov = 1.0f / (__builtin_elementwise_sqrt(1.0f - cosFov * cosFov) / cosFov);
	mtx[0][0] = cotanFov;
	mtx[1][1] = cotanFov;
	mtx[2][2] = Q;
	mtx[2][3] = 1.0f;
	mtx[3][2] = -Q * nearPlane;
}

// 007ACF20  com_math.cpp:2299
void InfinitePerspectiveMatrix(float tanHalfFovX, float tanHalfFovY, float zNear, float mtx[4][4])
{
	float invLedgeScale;

	iassert_at(2304, mtx);
	iassert_at(2305, zNear > 0);

	memset(mtx, 0, sizeof(float[4][4]));
	invLedgeScale = 0.99951171875f;
	mtx[0][0] = invLedgeScale / tanHalfFovX;
	mtx[1][1] = invLedgeScale / tanHalfFovY;
	mtx[2][2] = invLedgeScale;
	mtx[2][3] = 1.0f;
	mtx[3][2] = -zNear * invLedgeScale;
}

// 007AD000  com_math.cpp:2352
void MatrixForViewer(const float origin[3], const float axis[3][3], float mtx[4][4])
{
	iassert_at(2353, mtx);
	iassert_at(2354, origin);
	iassert_at(2355, axis);

	mtx[0][0] = -axis[1][0];
	mtx[1][0] = -axis[1][1];
	mtx[2][0] = -axis[1][2];
	mtx[3][0] = -(origin[0] * mtx[0][0] + origin[1] * mtx[1][0] + origin[2] * mtx[2][0]);
	mtx[0][1] = axis[2][0];
	mtx[1][1] = axis[2][1];
	mtx[2][1] = axis[2][2];
	mtx[3][1] = -(origin[0] * mtx[0][1] + origin[1] * mtx[1][1] + origin[2] * mtx[2][1]);
	mtx[0][2] = axis[0][0];
	mtx[1][2] = axis[0][1];
	mtx[2][2] = axis[0][2];
	mtx[3][2] = -(origin[0] * mtx[0][2] + origin[1] * mtx[1][2] + origin[2] * mtx[2][2]);
	mtx[0][3] = 0.0f;
	mtx[1][3] = 0.0f;
	mtx[2][3] = 0.0f;
	mtx[3][3] = 1.0f;
}

// 007AD230  com_math.cpp:2380
void AnglesSubtract(const float v1[3], const float v2[3], float v3[3])
{
	v3[0] = AngleNormalize180(v1[0] - v2[0]);
	v3[1] = AngleNormalize180(v1[1] - v2[1]);
	v3[2] = AngleNormalize180(v1[2] - v2[2]);
}

// 007AD2E0  com_math.cpp:2397
X87_MATH float AngleNormalize360(const float angle)
{
	float scaledAngle;
	float result;
	float result2;

	scaledAngle = angle * (1.0f / 360.0f);
	result = (scaledAngle - (float)floor(scaledAngle)) * 360.0f;  // x87 in the original: one rounding
	result2 = result - 360.0f;
	return result2 >= 0.0f ? result2 : result;
}

// 007AD360  com_math.cpp:2419
X87_MATH float AngleDelta(const float angle1, const float angle2)
{
	return AngleNormalize180(angle1 - angle2);  // passes ST0 through unrounded, as the original does
}

// 007AD380  com_math.cpp:2424
float RadiusFromBounds(const float mins[3], const float maxs[3])
{
	return sqrtf(RadiusFromBoundsSq(mins, maxs));
}

// 007AD3B0  com_math.cpp:2429
float RadiusFromBounds2D(const float mins[2], const float maxs[2])
{
	return sqrtf(RadiusFromBounds2DSq(mins, maxs));
}

// 007AD3E0  com_math.cpp:2434
X87_MATH float RadiusFromBoundsSq(const float mins[3], const float maxs[3])
{
	int i;
	float corner[3];
	float a;
	float b;

	for (i = 0; i < 3; ++i)
	{
		a = __builtin_fabsf(mins[i]);
		b = __builtin_fabsf(maxs[i]);
		corner[i] = a > b ? a : b;
	}
	return corner[0] * corner[0] + corner[1] * corner[1] + corner[2] * corner[2];  // returned unrounded in ST0
}

// 007AD490  com_math.cpp:2450
X87_MATH float RadiusFromBounds2DSq(const float mins[2], const float maxs[2])
{
	int i;
	float corner[2];
	float a;
	float b;

	for (i = 0; i < 2; ++i)
	{
		a = __builtin_fabsf(mins[i]);
		b = __builtin_fabsf(maxs[i]);
		corner[i] = a > b ? a : b;
	}
	return corner[0] * corner[0] + corner[1] * corner[1];
}

// 007AD540  com_math.cpp:2466
void ExtendBounds(float mins[3], float maxs[3], const float offset[3])
{
	if (offset[0] > 0.0f)
		maxs[0] = maxs[0] + offset[0];
	else
		mins[0] = mins[0] + offset[0];
	if (offset[1] > 0.0f)
		maxs[1] = maxs[1] + offset[1];
	else
		mins[1] = mins[1] + offset[1];
	if (offset[2] > 0.0f)
		maxs[2] = maxs[2] + offset[2];
	else
		mins[2] = mins[2] + offset[2];
}

// 007AD610  com_math.cpp:2484
void ExpandBoundsToWidth(float mins[3], float maxs[3])
{
	float size[3];
	float width;
	float diff;

	iassert_at(2489, maxs[0] >= mins[0]);
	iassert_at(2490, maxs[1] >= mins[1]);
	iassert_at(2491, maxs[2] >= mins[2]);

	size[0] = maxs[0] - mins[0];
	size[1] = maxs[1] - mins[1];
	size[2] = maxs[2] - mins[2];
	width = size[0] - size[1] >= 0.0f ? size[0] : size[1];
	if (width > size[2])
	{
		diff = (width - size[2]) * 0.5f;
		mins[2] = mins[2] - diff;
		maxs[2] = maxs[2] + diff;
	}
}

// 007AD780  com_math.cpp:2533
void ClearBounds(float mins[3], float maxs[3])
{
	mins[0] = 131072.0f;
	mins[1] = 131072.0f;
	mins[2] = 131072.0f;
	maxs[0] = -131072.0f;
	maxs[1] = -131072.0f;
	maxs[2] = -131072.0f;
}

// 007AD7F0  com_math.cpp:2540
bool IsClearedBounds(const float mins[3], const float maxs[3])
{
	if (mins[0] != 131072.0f || mins[1] != 131072.0f || mins[2] != 131072.0f)
		return false;
	if (maxs[0] != -131072.0f || maxs[1] != -131072.0f || maxs[2] != -131072.0f)
		return false;
	return true;
}

// 007AD880  com_math.cpp:2552
void ClearBounds2D(float mins[2], float maxs[2])
{
	mins[0] = 131072.0f;
	mins[1] = 131072.0f;
	maxs[0] = -131072.0f;
	maxs[1] = -131072.0f;
}

// 007AD8D0  com_math.cpp:2559
void AddPointToBounds(const float v[3], float mins[3], float maxs[3])
{
	if (v[0] < mins[0])
		mins[0] = v[0];
	if (v[0] > maxs[0])
		maxs[0] = v[0];
	if (v[1] < mins[1])
		mins[1] = v[1];
	if (v[1] > maxs[1])
		maxs[1] = v[1];
	if (v[2] < mins[2])
		mins[2] = v[2];
	if (v[2] > maxs[2])
		maxs[2] = v[2];
}

// 007AD980  com_math.cpp:2578
void AddPointToBounds2D(const float v[2], float mins[2], float maxs[2])
{
	if (v[0] < mins[0])
		mins[0] = v[0];
	if (v[0] > maxs[0])
		maxs[0] = v[0];
	if (v[1] < mins[1])
		mins[1] = v[1];
	if (v[1] > maxs[1])
		maxs[1] = v[1];
}

// 007ADA00  com_math.cpp:2592
int PointInBounds(const float v[3], const float mins[3], const float maxs[3])
{
	iassert_at(2593, v);
	iassert_at(2594, mins);
	iassert_at(2595, maxs);

	if (v[0] < mins[0] || v[0] > maxs[0])
		return 0;
	if (v[1] < mins[1] || v[1] > maxs[1])
		return 0;
	if (v[2] < mins[2] || v[2] > maxs[2])
		return 0;
	return 1;
}

// 007ADB00  com_math.cpp:2608
int PointInBounds2D(const float v[2], const float mins[2], const float maxs[2])
{
	iassert_at(2609, v);
	iassert_at(2610, mins);
	iassert_at(2611, maxs);

	if (v[0] < mins[0] || v[0] > maxs[0])
		return 0;
	if (v[1] < mins[1] || v[1] > maxs[1])
		return 0;
	return 1;
}

// 007ADBE0  com_math.cpp:2622
int BoundsOverlap(const float mins0[3], const float maxs0[3], const float mins1[3], const float maxs1[3])
{
	if (mins0[0] > maxs1[0] || mins1[0] > maxs0[0])
		return 0;
	if (mins0[1] > maxs1[1] || mins1[1] > maxs0[1])
		return 0;
	if (mins0[2] > maxs1[2] || mins1[2] > maxs0[2])
		return 0;
	return 1;
}

// 007ADC60  com_math.cpp:2662
void ExpandBounds(const float addedmins[3], const float addedmaxs[3], float mins[3], float maxs[3])
{
	if (addedmins[0] < mins[0])
		mins[0] = addedmins[0];
	if (addedmaxs[0] > maxs[0])
		maxs[0] = addedmaxs[0];
	if (addedmins[1] < mins[1])
		mins[1] = addedmins[1];
	if (addedmaxs[1] > maxs[1])
		maxs[1] = addedmaxs[1];
	if (addedmins[2] < mins[2])
		mins[2] = addedmins[2];
	if (addedmaxs[2] > maxs[2])
		maxs[2] = addedmaxs[2];
}

// 007ADD10  com_math.cpp:2680
void ExpandBounds2D(const float addedmins[2], const float addedmaxs[2], float totalmins[2], float totalmaxs[2])
{
	if (addedmins[0] < totalmins[0])
		totalmins[0] = addedmins[0];
	if (addedmaxs[0] > totalmaxs[0])
		totalmaxs[0] = addedmaxs[0];
	if (addedmins[1] < totalmins[1])
		totalmins[1] = addedmins[1];
	if (addedmaxs[1] > totalmaxs[1])
		totalmaxs[1] = addedmaxs[1];
}

// 007ADD90  com_math.cpp:2699
void GetRotatedBounds(const float baseBounds[2][3], const float origin[3], const float axis[3][3], float rotatedBounds[2][3])
{
	int axisIndex;
	int offset;

	// offset is a byte offset into baseBounds: 0 picks mins for the low bound, 12 picks maxs
	for (axisIndex = 0; axisIndex < 3; ++axisIndex)
	{
		rotatedBounds[0][axisIndex] = origin[axisIndex];
		rotatedBounds[1][axisIndex] = origin[axisIndex];

		offset = *(const int *)&axis[0][axisIndex] < 0 ? 12 : 0;
		rotatedBounds[0][axisIndex] = *(const float *)((const char *)baseBounds + offset) * axis[0][axisIndex] + rotatedBounds[0][axisIndex];
		rotatedBounds[1][axisIndex] = *(const float *)((const char *)baseBounds + 12 - offset) * axis[0][axisIndex] + rotatedBounds[1][axisIndex];

		offset = *(const int *)&axis[1][axisIndex] < 0 ? 12 : 0;
		rotatedBounds[0][axisIndex] = *(const float *)((const char *)baseBounds + offset + 4) * axis[1][axisIndex] + rotatedBounds[0][axisIndex];
		rotatedBounds[1][axisIndex] = *(const float *)((const char *)baseBounds + 16 - offset) * axis[1][axisIndex] + rotatedBounds[1][axisIndex];

		offset = *(const int *)&axis[2][axisIndex] < 0 ? 12 : 0;
		rotatedBounds[0][axisIndex] = *(const float *)((const char *)baseBounds + offset + 8) * axis[2][axisIndex] + rotatedBounds[0][axisIndex];
		rotatedBounds[1][axisIndex] = *(const float *)((const char *)baseBounds + 20 - offset) * axis[2][axisIndex] + rotatedBounds[1][axisIndex];
	}
}

// 007ADF60  com_math.cpp:2729
void AxisClear(float axis[3][3])
{
	axis[0][0] = 1.0f;
	axis[0][1] = 0.0f;
	axis[0][2] = 0.0f;
	axis[1][0] = 0.0f;
	axis[1][1] = 1.0f;
	axis[1][2] = 0.0f;
	axis[2][0] = 0.0f;
	axis[2][1] = 0.0f;
	axis[2][2] = 1.0f;
}

// 007AE000  com_math.cpp:2749
void AxisTranspose(const float in[3][3], float out[3][3])
{
	iassert_at(2750, in != out);

	out[0][0] = in[0][0];
	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][1] = in[1][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
	out[2][2] = in[2][2];
}

// 007AE0A0  com_math.cpp:2773
void AxisTransformVec3(const float axes[3][3], const float vec[3], float out[3])
{
	out[0] = vec[0] * axes[0][0] + vec[1] * axes[1][0] + vec[2] * axes[2][0];
	out[1] = vec[0] * axes[0][1] + vec[1] * axes[1][1] + vec[2] * axes[2][1];
	out[2] = vec[0] * axes[0][2] + vec[1] * axes[1][2] + vec[2] * axes[2][2];
}

// 007AE160  com_math.cpp:2780
void YawToAxis(float yaw, float axis[3][3])
{
	float right[3];

	YawVectors(yaw, axis[0], right);
	axis[2][0] = 0.0f;
	axis[2][1] = 0.0f;
	axis[2][2] = 1.0f;
	axis[1][0] = vec3_origin[0] - right[0];
	axis[1][1] = vec3_origin[1] - right[1];
	axis[1][2] = vec3_origin[2] - right[2];
}

// 007AE200  com_math.cpp:2799
void AxisToAngles(const float axis[3][3], float angles[3])
{
	float right[3];
	float rad;
	float fCos;
	float fSin;
	float temp;
	float pitch;

	vectoangles(axis[0], angles);
	right[0] = axis[1][0];
	right[1] = axis[1][1];
	right[2] = axis[1][2];

	rad = -angles[1] * (3.14159265f / 180.0f);
	FastSinCos(rad, &fSin, &fCos);
	temp = fCos * right[0] - fSin * right[1];
	right[1] = fSin * right[0] + fCos * right[1];

	rad = -angles[0] * (3.14159265f / 180.0f);
	FastSinCos(rad, &fSin, &fCos);
	right[0] = fSin * right[2] + fCos * temp;
	right[2] = fCos * right[2] - fSin * temp;

	pitch = vectosignedpitch(right);
	if (right[1] < 0.0f)
		angles[2] = pitch + (pitch < 0.0f ? 180.0f : -180.0f);
	else
		angles[2] = -pitch;
}

// 007AE3B0  com_math.cpp:2842
void Axis4ToAngles(const float axis[4][4], float angles[3])
{
	float right[4];
	float rad;
	float fCos;
	float fSin;
	float temp;
	float pitch;

	vectoangles(axis[0], angles);
	right[0] = axis[1][0];
	right[1] = axis[1][1];
	right[2] = axis[1][2];
	right[3] = axis[1][3];

	rad = -angles[1] * (3.14159265f / 180.0f);
	FastSinCos(rad, &fSin, &fCos);
	temp = fCos * right[0] - fSin * right[1];
	right[1] = fSin * right[0] + fCos * right[1];

	rad = -angles[0] * (3.14159265f / 180.0f);
	FastSinCos(rad, &fSin, &fCos);
	right[0] = fSin * right[2] + fCos * temp;
	right[2] = fCos * right[2] - fSin * temp;

	pitch = vectosignedpitch(right);
	if (right[1] < 0.0f)
		angles[2] = pitch + (pitch < 0.0f ? 180.0f : -180.0f);
	else
		angles[2] = -pitch;
}

// 007AE560  com_math.cpp:2885
int IntersectPlanes(const float *plane[3], float xyz[3])
{
	double determinantTerm[3];
	double determinant;
	double invDeterminant;

	determinantTerm[0] = (plane[1][1] * plane[2][2] - plane[2][1] * plane[1][2]) * plane[0][0];
	determinantTerm[1] = (plane[2][1] * plane[0][2] - plane[0][1] * plane[2][2]) * plane[1][0];
	determinantTerm[2] = (plane[0][1] * plane[1][2] - plane[1][1] * plane[0][2]) * plane[2][0];
	determinant = determinantTerm[0] + determinantTerm[1] + determinantTerm[2];
	if (__builtin_fabs(determinant) < 0.001f)
		return 0;
	invDeterminant = 1.0 / determinant;

	determinantTerm[0] = (plane[1][1] * plane[2][2] - plane[2][1] * plane[1][2]) * plane[0][3];
	determinantTerm[1] = (plane[2][1] * plane[0][2] - plane[0][1] * plane[2][2]) * plane[1][3];
	determinantTerm[2] = (plane[0][1] * plane[1][2] - plane[1][1] * plane[0][2]) * plane[2][3];
	xyz[0] = (determinantTerm[0] + determinantTerm[1] + determinantTerm[2]) * invDeterminant;

	determinantTerm[0] = (plane[1][2] * plane[2][0] - plane[2][2] * plane[1][0]) * plane[0][3];
	determinantTerm[1] = (plane[2][2] * plane[0][0] - plane[0][2] * plane[2][0]) * plane[1][3];
	determinantTerm[2] = (plane[0][2] * plane[1][0] - plane[1][2] * plane[0][0]) * plane[2][3];
	xyz[1] = (determinantTerm[0] + determinantTerm[1] + determinantTerm[2]) * invDeterminant;

	determinantTerm[0] = (plane[1][0] * plane[2][1] - plane[2][0] * plane[1][1]) * plane[0][3];
	determinantTerm[1] = (plane[2][0] * plane[0][1] - plane[0][0] * plane[2][1]) * plane[1][3];
	determinantTerm[2] = (plane[0][0] * plane[1][1] - plane[1][0] * plane[0][1]) * plane[2][3];
	xyz[2] = (determinantTerm[0] + determinantTerm[1] + determinantTerm[2]) * invDeterminant;

	return 1;
}

// 007AE930  com_math.cpp:2917
void SnapPointToIntersectingPlanes(const float *planes[3], float xyz[3], float snapGrid, float snapEpsilon)
{
	float snapped[3];
	float baseError;
	float maxBaseError;
	float snapError;
	float maxSnapError;
	float rounded;
	int axis;
	int planeIndex;

	for (axis = 0; axis < 3; axis++)
	{
		float scaled = xyz[axis] / snapGrid;
		double bias = 1.0 / (1 << 30);
		int roundedInt;

		__asm
		{
			fld scaled
			fadd bias
			fistp roundedInt
		}
		rounded = (float)roundedInt * snapGrid;
		if (__builtin_fabsf(rounded - xyz[axis]) < snapEpsilon)
			snapped[axis] = rounded;
		else
			snapped[axis] = xyz[axis];
	}

	if (snapped[0] == xyz[0] && snapped[1] == xyz[1] && snapped[2] == xyz[2])
		return;

	maxSnapError = 0.0f;
	maxBaseError = snapEpsilon;
	for (planeIndex = 0; planeIndex < 3; planeIndex++)
	{
		snapError = __builtin_fabsf(planes[planeIndex][0] * snapped[0] + planes[planeIndex][1] * snapped[1]
		                            + planes[planeIndex][2] * snapped[2] - planes[planeIndex][3]);
		if (snapError > maxSnapError)
			maxSnapError = snapError;
		baseError = __builtin_fabsf(planes[planeIndex][0] * xyz[0] + planes[planeIndex][1] * xyz[1]
		                            + planes[planeIndex][2] * xyz[2] - planes[planeIndex][3]);
		if (baseError > maxBaseError)
			maxBaseError = baseError;
	}

	if (maxBaseError > maxSnapError)
	{
		xyz[0] = snapped[0];
		xyz[1] = snapped[1];
		xyz[2] = snapped[2];
	}
}

// 007AEBB0  com_math.cpp:3001
int ProjectedWindingContainsCoplanarPoint(const float verts[][3], int vertCount, int x, int y, const float point[3])
{
	float pointDelta[2];
	int vertIndexPrev;
	int vertIndex;
	float edgeNormal[2];
	float projectionDist;

	vertIndexPrev = vertCount - 1;
	for (vertIndex = 0; vertIndex < vertCount; vertIndexPrev = vertIndex++)
	{
		edgeNormal[0] = verts[vertIndex][y] - verts[vertIndexPrev][y];
		edgeNormal[1] = verts[vertIndexPrev][x] - verts[vertIndex][x];
		pointDelta[0] = point[x] - verts[vertIndexPrev][x];
		pointDelta[1] = point[y] - verts[vertIndexPrev][y];
		projectionDist = pointDelta[0] * edgeNormal[0] + pointDelta[1] * edgeNormal[1];
		if (projectionDist < 0.0f)
			return 0;
	}
	return 1;
}

// 007AECC0  com_math.cpp:3096
int PlaneFromPoints(float plane[4], const float v0[3], const float v1[3], const float v2[3])
{
	float lengthSq;
	float length;
	float v1_v0[3];
	float v2_v0[3];

	v1_v0[0] = v1[0] - v0[0];
	v1_v0[1] = v1[1] - v0[1];
	v1_v0[2] = v1[2] - v0[2];
	v2_v0[0] = v2[0] - v0[0];
	v2_v0[1] = v2[1] - v0[1];
	v2_v0[2] = v2[2] - v0[2];
	Vec3Cross(v2_v0, v1_v0, plane);
	lengthSq = plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2];
	if (lengthSq < 2.0f)
	{
		if (lengthSq == 0.0f)
			return 0;
		if (lengthSq <= (v2_v0[0] * v2_v0[0] + v2_v0[1] * v2_v0[1] + v2_v0[2] * v2_v0[2])
		                 * (v1_v0[0] * v1_v0[0] + v1_v0[1] * v1_v0[1] + v1_v0[2] * v1_v0[2]) * (0.001f * 0.001f))
		{
			// nearly colinear: retry from v1, which gives a better-conditioned cross product
			v1_v0[0] = v2[0] - v1[0];
			v1_v0[1] = v2[1] - v1[1];
			v1_v0[2] = v2[2] - v1[2];
			v2_v0[0] = v0[0] - v1[0];
			v2_v0[1] = v0[1] - v1[1];
			v2_v0[2] = v0[2] - v1[2];
			Vec3Cross(v2_v0, v1_v0, plane);
			if (lengthSq <= (v2_v0[0] * v2_v0[0] + v2_v0[1] * v2_v0[1] + v2_v0[2] * v2_v0[2])
			                 * (v1_v0[0] * v1_v0[0] + v1_v0[1] * v1_v0[1] + v1_v0[2] * v1_v0[2]) * (0.001f * 0.001f))
				return 0;
		}
	}
	length = __builtin_elementwise_sqrt(lengthSq);
	plane[0] = plane[0] / length;
	plane[1] = plane[1] / length;
	plane[2] = plane[2] / length;
	plane[3] = v0[0] * plane[0] + v0[1] * plane[1] + v0[2] * plane[2];
	return 1;
}

// 007AEFB0  com_math.cpp:3135
void ProjectPointOnPlane(const float p[3], const float normal[3], float dst[3])
{
	float d;

	iassertmsg_at(3138, Vec3IsNormalized( normal ), va("(%g %g %g) len %g", normal[0], normal[1], normal[2], Vec3Length(normal)));
	d = -(normal[0] * p[0] + normal[1] * p[1] + normal[2] * p[2]);
	dst[0] = d * normal[0] + p[0];
	dst[1] = d * normal[1] + p[1];
	dst[2] = d * normal[2] + p[2];
}

// 007AF0E0  com_math.cpp:3152
void SetPlaneSignbits(cplane_s *out)
{
	unsigned char bits;
	int j;

	bits = 0;
	for (j = 0; j < 3; j++)
	{
		if (out->normal[j] < 0.0f)
			bits |= 1 << j;
	}
	out->signbits = bits;
}

// clang 18 names MS inline-asm labels `L__MSASMLABEL_.<n>__<label>` but emits `offset <label>` as a
// reference to `__MSASMLABEL_.<n>__<label>` (no L), which then never resolves. Alias the two so
// BoxOnPlaneSide's jump table can be written the way the original was. Build workaround only.
__asm__(".set __MSASMLABEL_.0__Lcase0, L__MSASMLABEL_.0__Lcase0\n"
        ".set __MSASMLABEL_.0__Lcase1, L__MSASMLABEL_.0__Lcase1\n"
        ".set __MSASMLABEL_.0__Lcase2, L__MSASMLABEL_.0__Lcase2\n"
        ".set __MSASMLABEL_.0__Lcase3, L__MSASMLABEL_.0__Lcase3\n"
        ".set __MSASMLABEL_.0__Lcase4, L__MSASMLABEL_.0__Lcase4\n"
        ".set __MSASMLABEL_.0__Lcase5, L__MSASMLABEL_.0__Lcase5\n"
        ".set __MSASMLABEL_.0__Lcase6, L__MSASMLABEL_.0__Lcase6\n"
        ".set __MSASMLABEL_.0__Lcase7, L__MSASMLABEL_.0__Lcase7\n");

// 007AF140  com_math.cpp:3220
__declspec(naked) int BoxOnPlaneSide(const float emins[3], const float emaxs[3], const cplane_s *p)
{
	T5_STATIC int bops_initialized T5_AT("BoxOnPlaneSide", "bops_initialized");
	T5_STATIC int Ljmptab[8] T5_AT("BoxOnPlaneSide", "Ljmptab");

	__asm
	{
		push ebx

		cmp bops_initialized, 1
		je initialized
		mov bops_initialized, 1

		mov Ljmptab[0*4], offset Lcase0
		mov Ljmptab[1*4], offset Lcase1
		mov Ljmptab[2*4], offset Lcase2
		mov Ljmptab[3*4], offset Lcase3
		mov Ljmptab[4*4], offset Lcase4
		mov Ljmptab[5*4], offset Lcase5
		mov Ljmptab[6*4], offset Lcase6
		mov Ljmptab[7*4], offset Lcase7

initialized:
		mov edx, dword ptr [4+12+esp]
		mov ecx, dword ptr [4+4+esp]
		xor eax, eax
		mov ebx, dword ptr [4+8+esp]
		mov al, byte ptr [edx+17]
		cmp al, 8
		jae Lerror
		fld dword ptr [edx+0]
		fld st(0)
		jmp dword ptr [Ljmptab+eax*4]
Lcase0:
		fmul dword ptr [ebx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ecx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ecx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase1:
		fmul dword ptr [ecx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ecx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ecx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase2:
		fmul dword ptr [ebx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ebx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ecx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase3:
		fmul dword ptr [ecx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ebx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ecx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase4:
		fmul dword ptr [ebx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ecx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ebx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase5:
		fmul dword ptr [ecx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ebx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ecx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ebx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase6:
		fmul dword ptr [ebx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ebx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ebx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase7:
		fmul dword ptr [ecx]
		fld dword ptr [edx+4]
		fxch st(2)
		fmul dword ptr [ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+4]
		fld dword ptr [edx+8]
		fxch st(2)
		fmul dword ptr [ebx+4]
		fxch st(2)
		fld st(0)
		fmul dword ptr [ecx+8]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr [ebx+8]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
LSetSides:
		faddp st(2), st(0)
		fcomp dword ptr [edx+12]
		xor ecx, ecx
		fnstsw ax
		fcomp dword ptr [edx+12]
		and ah, 1
		xor ah, 1
		add cl, ah
		fnstsw ax
		and ah, 1
		add ah, ah
		add cl, ah
		pop ebx
		mov eax, ecx
		ret
Lerror:
		int 3
	}
	// The original follows the asm block with an assert at line 3450 (type 1):
	//     Assert_MyHandler(__FILE__, 3450, 1, "BoxOnPlaneSide: invalid signbits for plane")
	// clang rejects C statements in a naked function. The assert is only reached by continuing past
	// the int 3 in a debugger.
}

// 007AF3A0  com_math.cpp:3466
int IsPosInsideArc(const float pos[3], float posRadius, const float arcOrigin[3], float arcRadius, float arcAngle0,
                   float arcAngle1, float arcHalfHeight)
{
	float adjustedDistDelta;
	float posYaw;
	float distDelta;
	float originDelta[3];

	iassert_at(3472, pos);
	iassert_at(3473, arcOrigin);

	originDelta[0] = pos[0] - arcOrigin[0];
	originDelta[1] = pos[1] - arcOrigin[1];
	originDelta[2] = pos[2] - arcOrigin[2];
	distDelta = Vec2Normalize(originDelta);
	adjustedDistDelta = distDelta - posRadius;
	if (adjustedDistDelta * adjustedDistDelta > arcRadius * arcRadius)
		return 0;

	if (pos[2] < arcOrigin[2] - arcHalfHeight || pos[2] > arcOrigin[2] + arcHalfHeight)
		return 0;

	posYaw = vectoyaw(originDelta);
	posYaw = AngleNormalize360(posYaw);
	if (arcAngle1 > arcAngle0)
	{
		if (posYaw < arcAngle1 && posYaw > arcAngle0)
			return 1;
	}
	else
	{
		if (posYaw < arcAngle1 || posYaw > arcAngle0)
			return 1;
	}
	return 0;
}

// 007AF520  com_math.cpp:3510
int BoxDistSqrdExceeds(const float absmin[3], const float absmax[3], const float org[3], const float fogOpaqueDistSqrd)
{
	float maxsSqrd;
	int i;
	float maxs[3];
	float total;
	float mins[3];
	float minsSqrd;

	mins[0] = absmin[0] - org[0];
	mins[1] = absmin[1] - org[1];
	mins[2] = absmin[2] - org[2];
	maxs[0] = absmax[0] - org[0];
	maxs[1] = absmax[1] - org[1];
	maxs[2] = absmax[2] - org[2];
	total = 0.0f;
	for (i = 0; i < 3; i++)
	{
		// org is inside the slab on this axis: no distance along it
		if (mins[i] * maxs[i] <= 0.0f)
			continue;
		minsSqrd = mins[i] * mins[i];
		maxsSqrd = maxs[i] * maxs[i];
		total += maxsSqrd < minsSqrd ? maxsSqrd : minsSqrd;
	}
	return total > fogOpaqueDistSqrd;
}

// 007AF670  com_math.cpp:3545
float Q_rint(const float in)
{
	// VC90 evaluated the sum on the x87 stack and handed it to floor() without rounding it to float.
#pragma clang fp eval_method(double)
	return floor(in + 0.5f);
}

// 007AF6A0  com_math.cpp:3555
float ColorNormalize(const float in[3], float out[3])
{
	float scale;
	float max;

	max = in[0];
	if (in[1] > max)
		max = in[1];
	if (in[2] > max)
		max = in[2];

	if (max == 0.0f)
	{
		out[0] = out[1] = out[2] = 1.0f;
		return 0.0f;
	}

	scale = 1.0f / max;
	out[0] = scale * in[0];
	out[1] = scale * in[1];
	out[2] = scale * in[2];
	return max;
}

// 007AF790  com_math.cpp:3582
void ColorSRGBtoLinear(const float in[3], float out[3])
{
	out[0] = in[0] > 0.04045f ? powf((in[0] + 0.055f) / 1.055f, 2.4f) : in[0] / 12.92f;
	out[1] = in[1] > 0.04045f ? powf((in[1] + 0.055f) / 1.055f, 2.4f) : in[1] / 12.92f;
	out[2] = in[2] > 0.04045f ? powf((in[2] + 0.055f) / 1.055f, 2.4f) : in[2] / 12.92f;
}

extern "C" void _CIatan(void);

// 007AF8E0  com_math.cpp:3672
// Declared `float` in the header. VC90 returns the final multiply and divide from the x87 stack
// without rounding them to float. Defining it with a double result under the same decorated name
// does the same thing: same register, same unrounded value.
// VC90 compiled atan() here as the x87 intrinsic: the argument goes to the CRT's _CIatan in ST0
// (not ___libm_sse2_atan, which clang's atan() would reach). clang cannot emit that call itself,
// so the asm block makes it.
double PitchForYawOnNormal_x87(const float fYaw, const float normal[3]) __asm__("?PitchForYawOnNormal@@YAMMQBM@Z");
double PitchForYawOnNormal_x87(const float fYaw, const float normal[3])
{
	float forward[3];

	iassert_at(3675, normal[0] || normal[1] || normal[2]);

	YawVectors(fYaw, forward, nullptr);
	if (normal[2] == 0.0f)
		return 270.0f;

	forward[2] = (normal[0] * forward[0] + normal[1] * forward[1]) / normal[2];
	__asm
	{
		fld forward[2*4]
		call _CIatan
		fstp forward[2*4]
	}
	return (double)forward[2] * 180.0f / 3.14159265f;
}

// 007AF9D0  com_math.cpp:3701
void NearestPitchAndYawOnPlane(const float angles[3], const float normal[3], float result[3])
{
	float forward[3];
	float projected[3];

	iassert_at(3705, normal[0] || normal[1] || normal[2]);

	AngleVectors(angles, forward, nullptr, nullptr);
	ProjectPointOnPlane(forward, normal, projected);
	vectoangles(projected, result);
}

// 007AFA80  com_math.cpp:3723
void Rand_Init(int seed)
{
	holdrand = seed;
}

// 007AFA90  com_math.cpp:3729
float flrand(float min, float max)
{
	float result;

	holdrand = 214013 * holdrand + 2531011;
	result = (float)(holdrand >> 17);
	return (max - min) * result / 32768.0f + min;
}

// 007AFAF0  com_math.cpp:3741
int irand(int min, int max)
{
	__int64 result;

	holdrand = 214013 * holdrand + 2531011;
	result = holdrand >> 17;
	result = result * (max - min) >> 15;
	result += min;
	return (int)result;
}

// 007AFB70  com_math.cpp:3753
void AxisToQuat(const float mat[3][3], float out[4])
{
	float test[4][4];
	int best;
	float testSizeSq;
	float invLength;

	test[0][0] = mat[1][2] - mat[2][1];
	test[0][1] = mat[2][0] - mat[0][2];
	test[0][2] = mat[0][1] - mat[1][0];
	test[0][3] = mat[0][0] + mat[1][1] + mat[2][2] + 1.0f;
	testSizeSq = Vec4LengthSq(test[0]);
	if (testSizeSq >= 1.0f)
	{
		best = 0;
	}
	else
	{
		test[1][0] = mat[2][0] + mat[0][2];
		test[1][1] = mat[2][1] + mat[1][2];
		test[1][2] = mat[2][2] - mat[1][1] - mat[0][0] + 1.0f;
		test[1][3] = test[0][2];
		testSizeSq = Vec4LengthSq(test[1]);
		if (testSizeSq >= 1.0f)
		{
			best = 1;
		}
		else
		{
			test[2][0] = mat[0][0] - mat[1][1] - mat[2][2] + 1.0f;
			test[2][1] = mat[1][0] + mat[0][1];
			test[2][2] = test[1][0];
			test[2][3] = test[0][0];
			testSizeSq = Vec4LengthSq(test[2]);
			if (testSizeSq >= 1.0f)
			{
				best = 2;
			}
			else
			{
				test[3][0] = test[2][1];
				test[3][1] = mat[1][1] - mat[0][0] - mat[2][2] + 1.0f;
				test[3][2] = test[1][1];
				test[3][3] = test[0][1];
				testSizeSq = Vec4LengthSq(test[3]);
				vassertp_at(3801, testSizeSq >= 1.0f, "(testSizeSq) = %g", testSizeSq);
				best = 3;
			}
		}
	}
	iassert_at(3806, testSizeSq);
	invLength = 1.0f / __builtin_elementwise_sqrt(testSizeSq);  // sqrtss: VC90 intrinsic sqrt
	out[0] = invLength * test[best][0];
	out[1] = invLength * test[best][1];
	out[2] = invLength * test[best][2];
	out[3] = invLength * test[best][3];
}

// 007AFE60  com_math.cpp:3813
void QuatLerp(const float qa[4], const float qb[4], float frac, float out[4])
{
	float dot;

	dot = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
	if (dot >= 0.0f)
	{
		Vec4Lerp(qa, qb, frac, out);
	}
	else
	{
		out[0] = -qb[0];
		out[1] = -qb[1];
		out[2] = -qb[2];
		out[3] = -qb[3];
		Vec4Lerp(qa, out, frac, out);
	}
}

// 007AFF60  com_math.cpp:3914
bool CullBoxFromCone(const float coneOrg[3], const float coneDir[3], float cosHalfFov, const float boxCenter[3],
                     const float boxHalfSize[3])
{
	float deltaMid[3];
	float farCorner[3];
	float dist;
	float perpendicular[3];
	float perpLenSq;
	float cosHalfFovSq;
	float sinHalfFovSq;
	float scale;
	float scaledSepAxis[3];
	float scaledSepDist;

	iassert_at(3926, cosHalfFov >= 0.0f);

	deltaMid[0] = boxCenter[0] - coneOrg[0];
	deltaMid[1] = boxCenter[1] - coneOrg[1];
	deltaMid[2] = boxCenter[2] - coneOrg[2];

	farCorner[0] = deltaMid[0] - boxHalfSize[0] * I_fsel(coneDir[0], 1.0f, -1.0f);
	farCorner[1] = deltaMid[1] - boxHalfSize[1] * I_fsel(coneDir[1], 1.0f, -1.0f);
	farCorner[2] = deltaMid[2] - boxHalfSize[2] * I_fsel(coneDir[2], 1.0f, -1.0f);

	dist = farCorner[0] * coneDir[0] + farCorner[1] * coneDir[1] + farCorner[2] * coneDir[2];
	if (dist >= 0.0f)
		return true;

	Vec3Mad(farCorner, -dist, coneDir, perpendicular);
	perpLenSq = perpendicular[0] * perpendicular[0] + perpendicular[1] * perpendicular[1]
	          + perpendicular[2] * perpendicular[2];
	cosHalfFovSq = cosHalfFov * cosHalfFov;
	sinHalfFovSq = 1.0f - cosHalfFovSq;
	if (dist * dist * sinHalfFovSq >= perpLenSq * cosHalfFovSq)
		return false;

	scale = cosHalfFov / __builtin_elementwise_sqrt(sinHalfFovSq * perpLenSq);
	Vec3Mad(coneDir, scale, perpendicular, scaledSepAxis);
	scaledSepDist = scaledSepAxis[0] * deltaMid[0] + scaledSepAxis[1] * deltaMid[1] + scaledSepAxis[2] * deltaMid[2];
	scaledSepDist -= I_fabs(boxHalfSize[0] * scaledSepAxis[0]);
	scaledSepDist -= I_fabs(boxHalfSize[1] * scaledSepAxis[1]);
	scaledSepDist -= I_fabs(boxHalfSize[2] * scaledSepAxis[2]);
	return scaledSepDist >= 0.0f;
}

// 007B02F0  com_math.cpp:3970
bool CullBoxFromSphere(const float sphereOrg[3], float radius, const float boxCenter[3], const float boxHalfSize[3])
{
	float distFromBoxToMid[3];

	distFromBoxToMid[0] = I_fmax(I_fabs(sphereOrg[0] - boxCenter[0]) - boxHalfSize[0], 0.0f);
	distFromBoxToMid[1] = I_fmax(I_fabs(sphereOrg[1] - boxCenter[1]) - boxHalfSize[1], 0.0f);
	distFromBoxToMid[2] = I_fmax(I_fabs(sphereOrg[2] - boxCenter[2]) - boxHalfSize[2], 0.0f);
	return distFromBoxToMid[0] * distFromBoxToMid[0] + distFromBoxToMid[1] * distFromBoxToMid[1]
	     + distFromBoxToMid[2] * distFromBoxToMid[2] > radius * radius;
}

// 007B0490  com_math.cpp:3986
bool CullBoxFromConicSectionOfSphere(const float coneOrg[3], const float coneDir[3], float cosHalfFov, float radius,
                                     const float boxCenter[3], const float boxHalfSize[3])
{
	float deltaMid[3];
	float distFromBoxToMid[3];
	float farCorner[3];
	float dist;
	float perpendicular[3];
	float perpLenSq;
	float cosHalfFovSq;
	float sinHalfFovSq;
	float scale;
	float scaledSepAxis[3];
	float scaledSepDist;

	iassert_at(3999, cosHalfFov >= 0.0f);

	deltaMid[0] = boxCenter[0] - coneOrg[0];
	deltaMid[1] = boxCenter[1] - coneOrg[1];
	deltaMid[2] = boxCenter[2] - coneOrg[2];

	distFromBoxToMid[0] = I_fmax(I_fabs(deltaMid[0]) - boxHalfSize[0], 0.0f);
	distFromBoxToMid[1] = I_fmax(I_fabs(deltaMid[1]) - boxHalfSize[1], 0.0f);
	distFromBoxToMid[2] = I_fmax(I_fabs(deltaMid[2]) - boxHalfSize[2], 0.0f);
	if (distFromBoxToMid[0] * distFromBoxToMid[0] + distFromBoxToMid[1] * distFromBoxToMid[1]
	    + distFromBoxToMid[2] * distFromBoxToMid[2] > radius * radius)
		return true;

	farCorner[0] = deltaMid[0] - boxHalfSize[0] * I_fsel(coneDir[0], 1.0f, -1.0f);
	farCorner[1] = deltaMid[1] - boxHalfSize[1] * I_fsel(coneDir[1], 1.0f, -1.0f);
	farCorner[2] = deltaMid[2] - boxHalfSize[2] * I_fsel(coneDir[2], 1.0f, -1.0f);

	dist = farCorner[0] * coneDir[0] + farCorner[1] * coneDir[1] + farCorner[2] * coneDir[2];
	if (dist >= 0.0f)
		return true;

	Vec3Mad(farCorner, -dist, coneDir, perpendicular);
	perpLenSq = perpendicular[0] * perpendicular[0] + perpendicular[1] * perpendicular[1]
	          + perpendicular[2] * perpendicular[2];
	cosHalfFovSq = cosHalfFov * cosHalfFov;
	sinHalfFovSq = 1.0f - cosHalfFovSq;
	if (dist * dist * sinHalfFovSq >= perpLenSq * cosHalfFovSq)
		return false;

	scale = cosHalfFov / __builtin_elementwise_sqrt(sinHalfFovSq * perpLenSq);
	Vec3Mad(coneDir, scale, perpendicular, scaledSepAxis);
	scaledSepDist = scaledSepAxis[0] * deltaMid[0] + scaledSepAxis[1] * deltaMid[1] + scaledSepAxis[2] * deltaMid[2];
	scaledSepDist -= I_fabs(boxHalfSize[0] * scaledSepAxis[0]);
	scaledSepDist -= I_fabs(boxHalfSize[1] * scaledSepAxis[1]);
	scaledSepDist -= I_fabs(boxHalfSize[2] * scaledSepAxis[2]);
	return scaledSepDist >= 0.0f;
}

// 007B09D0  com_math.cpp:4052
bool CullSphereFromCone(const float coneOrg[3], const float coneDir[3], float cosHalfFov, const float sphereCenter[3],
                        float radius)
{
	float delta[3];
	float dist;
	float perpendicular[3];
	float perpLenSq;
	float cosHalfFovSq;
	float sinHalfFovSq;
	float discriminant;

	iassert_at(4061, cosHalfFov >= 0.0f);

	delta[0] = sphereCenter[0] - coneOrg[0];
	delta[1] = sphereCenter[1] - coneOrg[1];
	delta[2] = sphereCenter[2] - coneOrg[2];
	dist = delta[0] * coneDir[0] + delta[1] * coneDir[1] + delta[2] * coneDir[2];
	if (dist >= radius)
		return true;

	Vec3Mad(delta, -dist, coneDir, perpendicular);
	perpLenSq = perpendicular[0] * perpendicular[0] + perpendicular[1] * perpendicular[1]
	          + perpendicular[2] * perpendicular[2];
	cosHalfFovSq = cosHalfFov * cosHalfFov;
	sinHalfFovSq = 1.0f - cosHalfFovSq;
	discriminant = __builtin_elementwise_sqrt(sinHalfFovSq) * dist - radius;
	return perpLenSq * cosHalfFovSq >= discriminant * discriminant;
}

// 007B1080  com_math.cpp:4341
void colorTempToXYZ(float colorTemp, float XYZ[4])
{
	double t;
	double t2;
	double t3;
	double xd;
	double xd2;
	double xd3;
	double yd;
	float x;
	float y;

	t = colorTemp < 1650.0f ? 1650.0 : colorTemp;
	t = t > 25000.0 ? 25000.0 : t;
	t2 = t * t;
	t3 = t * t * t;

	if (t < 4000.0)
		xd = -0.2661239 * 1e9 / t3 - 0.2343580 * 1e6 / t2 + 0.8776956 * 1e3 / t + 0.179910;
	else if (t < 7000.0)
		xd = -4.6070 * 1e9 / t3 + 2.9678 * 1e6 / t2 + 0.09911 * 1e3 / t + 0.244063;
	else
		xd = -2.0064 * 1e9 / t3 + 1.9018 * 1e6 / t2 + 0.24748 * 1e3 / t + 0.237040;

	xd2 = xd * xd;
	xd3 = xd * xd * xd;

	if (t < 4000.0)
	{
		if (xd <= 0.50338 && t > 2222.0)
			yd = -0.9549476 * xd3 - 1.37418593 * xd2 + 2.09137015 * xd - 0.16748867;
		else
			yd = -1.1063814 * xd3 - 1.34811020 * xd2 + 2.18555832 * xd - 0.20219683;
	}
	else
	{
		yd = -3.000 * xd2 + 2.870 * xd - 0.275;
	}

	x = (float)xd;
	y = (float)yd;
	XYZ[0] = x;
	XYZ[1] = y;
	XYZ[2] = 1.0f - x - y;
	XYZ[3] = 1.0f;
}

// 007B0B70  com_math.cpp:4414
void colorTempMatrix(float finalMatrix[4][4], float colorTemp)
{
	float whiteXYZ[4];
	float tempXYZ[4];
	float whiteLMS[4];
	float tempLMS[4];

	colorTempToXYZ(6500.0f, whiteXYZ);
	colorTempToXYZ(colorTemp, tempXYZ);
	MatrixTransformVector44(whiteXYZ, bradfordMA, whiteLMS);
	MatrixTransformVector44(tempXYZ, bradfordMA, tempLMS);

	float scaleLMS[4][4] = {
		{ tempLMS[0] / whiteLMS[0], 0.0f, 0.0f, 0.0f },
		{ 0.0f, tempLMS[1] / whiteLMS[1], 0.0f, 0.0f },
		{ 0.0f, 0.0f, tempLMS[2] / whiteLMS[2], 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	float tmp0[4][4];
	float tmp1[4][4];

	MatrixMultiply44(bradfordMA, scaleLMS, tmp0);
	MatrixMultiply44(tmp0, bradfordMI, tmp1);
	MatrixMultiply44(sRGBtoXYZ, tmp1, tmp0);
	MatrixMultiply44(tmp0, XYZtosRGB, finalMatrix);

	// round to 1/1024
	finalMatrix[0][0] += 1.0f / 2048.0f;
	finalMatrix[0][0] = floorf(finalMatrix[0][0] * 1024.0f) / 1024.0f;
	finalMatrix[0][1] += 1.0f / 2048.0f;
	finalMatrix[0][1] = floorf(finalMatrix[0][1] * 1024.0f) / 1024.0f;
	finalMatrix[0][2] += 1.0f / 2048.0f;
	finalMatrix[0][2] = floorf(finalMatrix[0][2] * 1024.0f) / 1024.0f;
	finalMatrix[1][0] += 1.0f / 2048.0f;
	finalMatrix[1][0] = floorf(finalMatrix[1][0] * 1024.0f) / 1024.0f;
	finalMatrix[1][1] += 1.0f / 2048.0f;
	finalMatrix[1][1] = floorf(finalMatrix[1][1] * 1024.0f) / 1024.0f;
	finalMatrix[1][2] += 1.0f / 2048.0f;
	finalMatrix[1][2] = floorf(finalMatrix[1][2] * 1024.0f) / 1024.0f;
	finalMatrix[2][0] += 1.0f / 2048.0f;
	finalMatrix[2][0] = floorf(finalMatrix[2][0] * 1024.0f) / 1024.0f;
	finalMatrix[2][1] += 1.0f / 2048.0f;
	finalMatrix[2][1] = floorf(finalMatrix[2][1] * 1024.0f) / 1024.0f;
	finalMatrix[2][2] += 1.0f / 2048.0f;
	finalMatrix[2][2] = floorf(finalMatrix[2][2] * 1024.0f) / 1024.0f;
}

// 007B1340  com_math.cpp:4486
void colorHueMatrix(float finalMatrix[4][4], float hue)
{
	float toGrayAxis[4][4] = {
		{ 0.81649655f, 0.0f, 0.53451085f, 0.0f },
		{ -0.40824828f, 0.70710665f, 1.0555116f, 0.0f },
		{ -0.40824828f, -0.70710677f, 0.14204822f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	float fromGrayAxis[4][4] = {
		{ 0.8467885f, -0.3779562f, -0.37795624f, 0.0f },
		{ -0.37292805f, 0.3341787f, -1.0800347f, 0.0f },
		{ 0.5773502f, 0.5773502f, 0.5773502f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	float sinHue = sinf(hue * 0.0174532f);
	float cosHue = cosf(hue * 0.0174532f);
	float hueRotate[4][4] = {
		{ cosHue, sinHue, 0.0f, 0.0f },
		{ -sinHue, cosHue, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	float tmp[4][4];

	MatrixMultiply44(toGrayAxis, hueRotate, tmp);
	MatrixMultiply44(tmp, fromGrayAxis, finalMatrix);

	// round to 1/1024
	finalMatrix[0][0] += 1.0f / 2048.0f;
	finalMatrix[0][0] = floorf(finalMatrix[0][0] * 1024.0f) / 1024.0f;
	finalMatrix[0][1] += 1.0f / 2048.0f;
	finalMatrix[0][1] = floorf(finalMatrix[0][1] * 1024.0f) / 1024.0f;
	finalMatrix[0][2] += 1.0f / 2048.0f;
	finalMatrix[0][2] = floorf(finalMatrix[0][2] * 1024.0f) / 1024.0f;
	finalMatrix[1][0] += 1.0f / 2048.0f;
	finalMatrix[1][0] = floorf(finalMatrix[1][0] * 1024.0f) / 1024.0f;
	finalMatrix[1][1] += 1.0f / 2048.0f;
	finalMatrix[1][1] = floorf(finalMatrix[1][1] * 1024.0f) / 1024.0f;
	finalMatrix[1][2] += 1.0f / 2048.0f;
	finalMatrix[1][2] = floorf(finalMatrix[1][2] * 1024.0f) / 1024.0f;
	finalMatrix[2][0] += 1.0f / 2048.0f;
	finalMatrix[2][0] = floorf(finalMatrix[2][0] * 1024.0f) / 1024.0f;
	finalMatrix[2][1] += 1.0f / 2048.0f;
	finalMatrix[2][1] = floorf(finalMatrix[2][1] * 1024.0f) / 1024.0f;
	finalMatrix[2][2] += 1.0f / 2048.0f;
	finalMatrix[2][2] = floorf(finalMatrix[2][2] * 1024.0f) / 1024.0f;
}

// 007B1A00  com_math.cpp:4524
void colorSaturationMatrix(float finalMatrix[4][4], float saturation)
{
	float r = 0.25f;
	float g = 0.5f;
	float b = 0.25f;

	r = (1.0f - saturation) * r;
	g = (1.0f - saturation) * g;
	b = (1.0f - saturation) * b;

	finalMatrix[0][0] = r + saturation;
	finalMatrix[0][1] = r;
	finalMatrix[0][2] = r;
	finalMatrix[0][3] = 0.0f;
	finalMatrix[1][0] = g;
	finalMatrix[1][1] = g + saturation;
	finalMatrix[1][2] = g;
	finalMatrix[1][3] = 0.0f;
	finalMatrix[2][0] = b;
	finalMatrix[2][1] = b;
	finalMatrix[2][2] = b + saturation;
	finalMatrix[3][3] = 0.0f;  // sic: [2][3] is never written in the original
	finalMatrix[3][0] = 0.0f;
	finalMatrix[3][1] = 0.0f;
	finalMatrix[3][2] = 0.0f;
	finalMatrix[3][3] = 1.0f;
}
