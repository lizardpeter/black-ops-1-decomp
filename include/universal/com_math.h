// universal/com_math.h
//
// Reconstructed declarations. Signatures are fixed by the original decorated names in the MAP;
// each function's own comment in src/universal/com_math.cpp carries its address and line.
// Grows as more of com_math.cpp is reconstructed.
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

int RandWithSeed(int *seed);
signed char ClampChar(const int i);
short ClampShort(const int i);
float vectoyaw(const float vec[2]);
int VecNCompareCustomEpsilon(const float v0[], const float v1[], float epsilon, int coordCount);

extern "C" void ClearBounds(float mins[3], float maxs[3]);  // unmangled in the original: _ClearBounds
void ClearBounds2D(float mins[2], float maxs[2]);
void AddPointToBounds2D(const float v[2], float mins[2], float maxs[2]);
void ExpandBounds2D(const float addedmins[2], const float addedmaxs[2], float totalmins[2], float totalmaxs[2]);

void SetPlaneSignbits(cplane_s *out);
void Rand_Init(int seed);
