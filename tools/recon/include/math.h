// VC90 (Visual Studio 2008) <math.h>, as far as the reconstruction needs it.
//
// Build environment, not game source: it reproduces what the original compiler saw so the
// reconstructed code can be written the way the original was. Notably, VC90's float math
// functions on x86 are inline wrappers around the double versions (atan2f(y, x) is
// (float)atan2((double)y, (double)x)), which is exactly the cvtss2sd / call / cvtsd2ss
// pattern in the original binary.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double cos(double);
double sin(double);
double tan(double);
double exp(double);
double log(double);
double pow(double, double);
double sqrt(double);
double floor(double);
double ceil(double);
double fabs(double);
double fmod(double, double);

#ifdef __cplusplus
}

inline float acosf(float x) { return (float)acos((double)x); }
inline float asinf(float x) { return (float)asin((double)x); }
inline float atanf(float x) { return (float)atan((double)x); }
inline float atan2f(float y, float x) { return (float)atan2((double)y, (double)x); }
inline float cosf(float x) { return (float)cos((double)x); }
inline float sinf(float x) { return (float)sin((double)x); }
inline float tanf(float x) { return (float)tan((double)x); }
inline float expf(float x) { return (float)exp((double)x); }
inline float logf(float x) { return (float)log((double)x); }
inline float powf(float x, float y) { return (float)pow((double)x, (double)y); }
inline float sqrtf(float x) { return (float)sqrt((double)x); }
inline float floorf(float x) { return (float)floor((double)x); }
inline float ceilf(float x) { return (float)ceil((double)x); }
inline float fabsf(float x) { return (float)fabs((double)x); }
#endif
