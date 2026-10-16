// Compiler-behaviour helpers for matching VC90 code generation.
//
// Build environment, not game source.
#pragma once

// VC90 kept some float expressions on the x87 stack and returned them unrounded in ST0 (excess precision,
// e.g. a returned sum of products, or arithmetic on a value returned in ST0). clang does the same only
// with SSE disabled for that function.
#define X87_MATH __attribute__((target("no-sse")))

// A float copied through the x87 stack (fld / fstp), as VC90 sometimes compiles a plain float assignment.
// The difference from a plain copy: a signaling NaN comes out quiet (fld raises invalid and quiets it).
// On a real CPU, that is: the differential harness's emulator (Unicorn) leaves the NaN signaling, so tests
// can't tell this from a plain copy (docs/verification.md, limits). Used where the disassembly shows it.
#define T5_X87_COPY(dst, src) __asm__("flds %1\n\tfstps %0" : "=m"(dst) : "m"(src))
