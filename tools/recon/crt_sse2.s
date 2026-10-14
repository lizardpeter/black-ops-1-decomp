# VC90 with SSE2 compiles calls to atan2(), sin(), ... into calls to the CRT's ___libm_sse2_*
# routines, which take arguments in XMM0/XMM1 and return in XMM0. These cdecl shims give the
# reconstruction the normal C entry points and forward to those exact routines. The routines
# themselves resolve to the ORIGINAL binary's copies through the hybrid link (tools/build_recon.py),
# so reconstructed and original code run the same math library bit for bit.
	.intel_syntax noprefix
	.text

.macro UNARY name
	.globl _\name
_\name:
	movsd xmm0, qword ptr [esp + 4]
	call ___libm_sse2_\name
	sub esp, 8
	movsd qword ptr [esp], xmm0
	fld qword ptr [esp]
	add esp, 8
	ret
.endm

.macro BINARY name
	.globl _\name
_\name:
	movsd xmm0, qword ptr [esp + 4]
	movsd xmm1, qword ptr [esp + 12]
	call ___libm_sse2_\name
	sub esp, 8
	movsd qword ptr [esp], xmm0
	fld qword ptr [esp]
	add esp, 8
	ret
.endm

	UNARY acos
	UNARY asin
	UNARY atan
	UNARY cos
	UNARY sin
	UNARY tan
	UNARY exp
	UNARY log
	BINARY atan2
	BINARY pow
