// universal/com_math.h
//
// Reconstructed declarations. Signatures are fixed by the original decorated names in the MAP;
// each function's own comment in src/universal/com_math.cpp carries its address and line.
#pragma once

// Layout from PDB type 0x1186 (sizeof 20). Original header location not yet confirmed.
struct cplane_s
{
	float normal[3];
	float dist;
	unsigned char type;
	unsigned char signbits;
	unsigned char pad[2];
};

// ---- defined outside com_math.cpp: resolved to the original binary by the hybrid link -------------

// CRT (no <string.h> in the build environment); VC90 expands small constant-size calls inline
extern "C" void *memset(void *dst, int val, unsigned int size);
extern "C" void *memcpy(void *dst, const void *src, unsigned int size);

extern "C" int rand(void);  // CRT

char *va(const char *format, ...);  // universal/q_shared.cpp
extern "C" void track_static_alloc_internal(void *ptr, int size, const char *name, int type);  // qcommon/mem_track.cpp

// universal/com_vector.h inlines; the original binary keeps out-of-line copies of these
void Vec3Copy(const float from[3], float to[3]);
void Vec3Sub(const float a[3], const float b[3], float out[3]);
void Vec3Cross(const float v0[3], const float v1[3], float cross[3]);
int Vec3IsNormalized(const float v[3]);
float Vec3Length(const float v[3]);
float Vec3Normalize(float v[3]);
float Vec2Normalize(float v[2]);
int Vec4IsNormalized(const float v[4]);
float Vec4LengthSq(const float v[4]);
void Vec4Lerp(const float from[4], const float to[4], float frac, float result[4]);

// universal/com_math.h inlines; out-of-line copies elsewhere in the original
float AngleNormalize180(const float angle);
void MatrixTransformVector(const float in1[3], const float in2[3][3], float out[3]);

// universal/com_math_anglevectors.cpp
float vectosignedpitch(const float vec[3]);
void AngleVectors(const float angles[3], float forward[3], float right[3], float up[3]);

extern const float vec3_origin[3];          // com_vector4_const.cpp
extern const float identityMatrix44[4][4];  // public data of com_math.cpp, not reconstructed yet
extern float bytedirs[162][3];              // public data of com_math.cpp, not reconstructed yet

// ---- inline helpers the original expands in com_math.cpp -------------------------------------------

// universal/com_math.h:88
__forceinline float I_fsel(float comparand, float valGE, float valLT)
{
	return comparand >= 0.0f ? valGE : valLT;
}

// universal/com_math.h:471
__forceinline float I_fmax(float x, float y)
{
	return x - y >= 0.0f ? x : y;
}

// universal/com_math.h (line not confirmed): the min counterpart of I_fmax, as client_mp/cl_input_mp.cpp
// expands it
__forceinline float I_fmin(float x, float y)
{
	return x - y >= 0.0f ? y : x;
}

// VC90 math.h fabs(float) as compiled with SSE2: andps with the sign mask
__forceinline float I_fabs(float f)
{
	return __builtin_fabsf(f);
}

// universal/com_vector.h:328
__forceinline void Vec3Mad(const float start[3], float scale, const float dir[3], float result[3])
{
	result[0] = start[0] + scale * dir[0];
	result[1] = start[1] + scale * dir[1];
	result[2] = start[2] + scale * dir[2];
}

// Exponent-bits test (so it is also true for infinities). The original copies its argument to a stack
// temporary and tests the bits with and/cmp/setz; it has no MAP entry, so it was a macro or forced inline.
__forceinline int IS_NAN(float f)
{
	return (__builtin_bit_cast(int, f) & 0x7f800000) == 0x7f800000;
}

// Header helper, always force-inlined (so it has no MAP entry): every function that uses it is flagged
// "has inline asm" in the PDB and keeps its parameters as stack temporaries. Its name is not recorded anywhere.
__forceinline void FastSinCos(const float radians, float *s, float *c)
{
	__asm
	{
		push eax
		push ecx
		fld radians
		fsincos
		mov ecx, c
		mov eax, s
		fstp dword ptr [ecx]
		fstp dword ptr [eax]
		pop ecx
		pop eax
	}
}

// ---- com_math.cpp, in original line order ----------------------------------------------------------

void TRACK_com_math();
float random();
float crandom();
void GaussianRandom(float *f0, float *f1);
int RandWithSeed(int *seed);
float randomhash(int value);
void PointInCircleFromUniformDeviates(float radiusDeviate, float yawDeviate, float point[2]);
float LinearTrack(float tgt, float cur, float rate, float deltaTime);
float LinearTrackAngle(float tgt, float cur, float rate, float deltaTime);
float DiffTrack(float tgt, float cur, float rate, float deltaTime);
float DiffTrackAngle(float tgt, float cur, float rate, float deltaTime);
float GraphGetValueFromFraction(const int knotCount, const float (*knots)[2], const float fraction);
float Q_acos(const float c);
signed char ClampChar(const int i);
short ClampShort(const int i);
unsigned char DirToByte(const float dir[3]);
void ByteToDir(const int b, float dir[3]);
int VecNCompareCustomEpsilon(const float v0[], const float v1[], float epsilon, int coordCount);
void Vec3ProjectionCoords(const float dir[3], int *xCoord, int *yCoord);
void RotatePointAroundVector(float dst[3], const float dir[3], const float point[3], const float degrees);
void Vec3Basis_RightHanded(const float forward[3], float left[3], float up[3]);
float vectoyaw(const float vec[2]);
float vectosignedyaw(const float vec[2]);
float vectopitch(const float vec[3]);
void vectoangles(const float vec[3], float angles[3]);
void UnitQuatToAngles(const float quat[4], float angles[3]);
void YawVectors(float yaw, float forward[3], float right[3]);
void YawVectors2D(const float yaw, float forward[2], float right[2]);
void PerpendicularVector(const float src[3], float dst[3]);
float PointToLineDistSq(const float point[3], const float start[3], const float end[3]);
float PointToLineDistSq2D(const float point[2], const float start[2], const float end[2]);
int BoxInPlanes(const float (*planes)[4], unsigned int numPlanes, const float mins[3], const float maxs[3]);
int SphereInPlanes(const float (*planes)[4], unsigned int numPlanes, const float center[3], const float radius);
void MatrixIdentity33(float out[3][3]);
void MatrixIdentity44(float out[4][4]);
void MatrixMultiply(const float in1[3][3], const float in2[3][3], float out[3][3]);
void MatrixVecMultiply(const float mulMat[3][3], const float mulVec[3], float solution[3]);
void MatrixVecMultiply(const float mulMat[4][4], const float mulVec[4], float solution[4]);
void MatrixVecMultiplyProject(const float mulMat[4][4], const float mulVec[3], float solution[3]);
void MatrixMultiplyEquals(const float in[3][3], float out[3][3]);
void MatrixMultiply43(const float in1[4][3], const float in2[4][3], float out[4][3]);
void MatrixMultiply44(const float in1[4][4], const float in2[4][4], float out[4][4]);
void MatrixTranspose(const float in[3][3], float out[3][3]);
void MatrixTranspose44(const float in[16], float out[16]);
void MatrixInverse(const float in[3][3], float out[3][3]);
void MatrixInverseOrthogonal43(const float in[4][3], float out[4][3]);
void MatrixInverse44(const float mat[16], float dst[16]);
void MatrixTransformVector44(const float vec[4], const float mat[4][4], float out[4]);
void MatrixTransformVector43(const float in1[3], const float in2[4][3], float out[3]);
void MatrixTransposeTransformVector43(const float in1[3], const float in2[4][3], float out[3]);
void MatrixTransformVector43Equals(float *out, const float *in);
void VectorAngleMultiply(float vec[2], float angle);
void UnitQuatToAxis(const float quat[4], float axis[3][3]);
void UnitQuatToForward(const float quat[4], float forward[3]);
void QuatSlerp(const float from[4], const float to[4], float frac, float result[4]);
float RotationToYaw(const float rot[2]);
void MatrixRotationZ(float mat[3][3], float degree);
void FinitePerspectiveMatrix(float tanHalfFovX, float tanHalfFovY, float zNear, float zFar, float mtx[4][4]);
void SpotLightViewMatrix(const float direction[3], float rotation, float mtx[4][4]);
void SpotLightViewMatrixDir3(const float dirx[3], const float diry[3], const float dirz[3], float mtx[4][4]);
void SpotLightProjectionMatrix(float cosFov, float zNear, float zFar, float mtx[4][4]);
void InfinitePerspectiveMatrix(float tanHalfFovX, float tanHalfFovY, float zNear, float mtx[4][4]);
void MatrixForViewer(const float origin[3], const float axis[3][3], float mtx[4][4]);
void AnglesSubtract(const float v1[3], const float v2[3], float v3[3]);
float AngleNormalize360(const float angle);
float AngleDelta(const float angle1, const float angle2);
float RadiusFromBounds(const float mins[3], const float maxs[3]);
float RadiusFromBounds2D(const float mins[2], const float maxs[2]);
float RadiusFromBoundsSq(const float mins[3], const float maxs[3]);
float RadiusFromBounds2DSq(const float mins[2], const float maxs[2]);
void ExtendBounds(float mins[3], float maxs[3], const float offset[3]);
void ExpandBoundsToWidth(float mins[3], float maxs[3]);
extern "C" void ClearBounds(float mins[3], float maxs[3]);  // unmangled in the original: _ClearBounds
bool IsClearedBounds(const float mins[3], const float maxs[3]);
void ClearBounds2D(float mins[2], float maxs[2]);
void AddPointToBounds(const float v[3], float mins[3], float maxs[3]);
void AddPointToBounds2D(const float v[2], float mins[2], float maxs[2]);
int PointInBounds(const float v[3], const float mins[3], const float maxs[3]);
int PointInBounds2D(const float v[2], const float mins[2], const float maxs[2]);
int BoundsOverlap(const float mins0[3], const float maxs0[3], const float mins1[3], const float maxs1[3]);
void ExpandBounds(const float addedmins[3], const float addedmaxs[3], float mins[3], float maxs[3]);
void ExpandBounds2D(const float addedmins[2], const float addedmaxs[2], float totalmins[2], float totalmaxs[2]);
extern "C" void GetRotatedBounds(const float baseBounds[2][3], const float origin[3], const float axis[3][3], float rotatedBounds[2][3]);  // unmangled in the original: _GetRotatedBounds
void AxisClear(float axis[3][3]);
void AxisTranspose(const float in[3][3], float out[3][3]);
void AxisTransformVec3(const float axes[3][3], const float vec[3], float out[3]);
void YawToAxis(float yaw, float axis[3][3]);
void AxisToAngles(const float axis[3][3], float angles[3]);
void Axis4ToAngles(const float axis[4][4], float angles[3]);
int IntersectPlanes(const float *plane[3], float xyz[3]);
void SnapPointToIntersectingPlanes(const float *planes[3], float xyz[3], float snapGrid, float snapEpsilon);
int ProjectedWindingContainsCoplanarPoint(const float verts[][3], int vertCount, int x, int y, const float point[3]);
int PlaneFromPoints(float plane[4], const float v0[3], const float v1[3], const float v2[3]);
void ProjectPointOnPlane(const float p[3], const float normal[3], float dst[3]);
void SetPlaneSignbits(cplane_s *out);
int BoxOnPlaneSide(const float emins[3], const float emaxs[3], const cplane_s *p);
int IsPosInsideArc(const float pos[3], float posRadius, const float arcOrigin[3], float arcRadius, float arcAngle0, float arcAngle1, float arcHalfHeight);
int BoxDistSqrdExceeds(const float absmin[3], const float absmax[3], const float org[3], const float fogOpaqueDistSqrd);
float Q_rint(const float in);
float ColorNormalize(const float in[3], float out[3]);
void ColorSRGBtoLinear(const float in[3], float out[3]);
float PitchForYawOnNormal(const float fYaw, const float normal[3]);
void NearestPitchAndYawOnPlane(const float angles[3], const float normal[3], float result[3]);
void Rand_Init(int seed);
float flrand(float min, float max);
int irand(int min, int max);
void AxisToQuat(const float mat[3][3], float out[4]);
void QuatLerp(const float qa[4], const float qb[4], float frac, float out[4]);
bool CullBoxFromCone(const float coneOrg[3], const float coneDir[3], float cosHalfFov, const float boxCenter[3], const float boxHalfSize[3]);
bool CullBoxFromSphere(const float sphereOrg[3], float radius, const float boxCenter[3], const float boxHalfSize[3]);
bool CullBoxFromConicSectionOfSphere(const float coneOrg[3], const float coneDir[3], float cosHalfFov, float radius, const float boxCenter[3], const float boxHalfSize[3]);
bool CullSphereFromCone(const float coneOrg[3], const float coneDir[3], float cosHalfFov, const float sphereCenter[3], float radius);
void colorTempToXYZ(float colorTemp, float XYZ[4]);
void colorTempMatrix(float finalMatrix[4][4], float colorTemp);
void colorHueMatrix(float finalMatrix[4][4], float hue);
void colorSaturationMatrix(float finalMatrix[4][4], float saturation);
