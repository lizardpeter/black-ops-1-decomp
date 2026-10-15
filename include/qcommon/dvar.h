// qcommon dvars: the parts the reconstruction uses.
//
// Layouts from the PDB (dvar_s, DvarValue, DvarLimits); signatures fixed by the original decorated names
// in the MAP. Original header location not yet confirmed (dvar.h in the IW engine family).
#pragma once

enum dvarType_t
{
	DVAR_TYPE_BOOL = 0,
	DVAR_TYPE_FLOAT = 1,
	DVAR_TYPE_FLOAT_2 = 2,
	DVAR_TYPE_FLOAT_3 = 3,
	DVAR_TYPE_FLOAT_4 = 4,
	DVAR_TYPE_INT = 5,
	DVAR_TYPE_ENUM = 6,
	DVAR_TYPE_STRING = 7,
	DVAR_TYPE_COLOR = 8,
	DVAR_TYPE_INT64 = 9,
	DVAR_TYPE_LINEAR_COLOR_RGB = 10,
	DVAR_TYPE_COLOR_XYZ = 11,
	DVAR_TYPE_COUNT = 12,
};

union DvarValue
{
	bool enabled;
	int integer;
	unsigned int unsignedInt;
	long long integer64;
	unsigned long long unsignedInt64;
	float value;
	float vector[4];
	const char *string;
	unsigned char color[4];
};

union DvarLimits
{
	struct { int stringCount; const char **strings; } enumeration;
	struct { int min; int max; } integer;
	struct { long long min; long long max; } integer64;
	struct { float min; float max; } value;
	struct { float min; float max; } vector;
};

struct dvar_s
{
	const char *name;          // 0x00
	const char *description;   // 0x04
	long hash;                 // 0x08
	unsigned short flags;      // 0x0C
	dvarType_t type;           // 0x10
	bool modified;             // 0x14
	bool loadedFromSaveGame;   // 0x15
	DvarValue current;         // 0x18
	DvarValue latched;         // 0x28
	DvarValue reset;           // 0x38
	DvarValue saved;           // 0x48
	DvarLimits domain;         // 0x58
	dvar_s *hashNext;          // 0x68
};
typedef dvar_s dvar_t;

// ---- qcommon/dvar*.cpp: resolved to the original binary by the hybrid link -------------------------------
bool Dvar_GetBool(const char *dvarName);
void Dvar_ClearModified(const dvar_s *dvar);
void Dvar_SetBool(const dvar_s *dvar, bool value);
const dvar_s *_Dvar_RegisterBool(const char *dvarName, bool value, unsigned short flags, const char *description);
#define Dvar_RegisterBool _Dvar_RegisterBool
