// universal/com_math.cpp
//
// Reconstruction of codsrc/src/universal/com_math.cpp, in original line order.
// Each function is tagged with its address in build CoDMPServer and its first line in the original file.
// Status of every function lives in records/functions.tsv.
#include "universal/com_math.h"

// 007A7F30  com_math.cpp:190
int RandWithSeed(int *seed)
{
	*seed = 1103515245 * *seed + 12345;
	return (unsigned int)(*seed / 65536) % 32768;
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

// 007AD880  com_math.cpp:2552
void ClearBounds2D(float mins[2], float maxs[2])
{
	mins[0] = 131072.0f;
	mins[1] = 131072.0f;
	maxs[0] = -131072.0f;
	maxs[1] = -131072.0f;
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
