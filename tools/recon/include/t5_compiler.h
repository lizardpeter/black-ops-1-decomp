// Compiler-behaviour helpers for matching VC90 code generation.
//
// Build environment, not game source.
#pragma once

// VC90 kept some float expressions on the x87 stack and returned them unrounded in ST0 (excess precision,
// e.g. a returned sum of products, or arithmetic on a value returned in ST0). clang does the same only
// with SSE disabled for that function.
#define X87_MATH __attribute__((target("no-sse")))
