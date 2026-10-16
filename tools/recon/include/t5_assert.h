// Assert macros as they expand in the original build.
//
// Build environment, not game source. The original's asserts call Assert_MyHandler(__FILE__, __LINE__,
// type, fmt, ...) and break into the debugger when it returns false. The reconstruction's line numbers
// differ from the original's, so each call site gives the ORIGINAL line explicitly, and each .cpp defines
// T5_ASSERT_FILE as the original __FILE__ string. The original binary shows four argument shapes; each
// macro below reproduces one of them exactly (format string and argument order).
#pragma once

extern "C" bool Assert_MyHandler(const char *filename, int line, int type, const char *fmt, ...);  // universal/assertive.cpp

// Assert_MyHandler(file, line, 0, "%s", "expr")
#define iassert_at(line, expr) \
	do { if (!(expr) && !Assert_MyHandler(T5_ASSERT_FILE, line, 0, "%s", #expr)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "%s\n\t%s", "expr", msg)
#define iassertmsg_at(line, expr, msg) \
	do { if (!(expr) && !Assert_MyHandler(T5_ASSERT_FILE, line, 0, "%s\n\t%s", #expr, msg)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "expr\n\t<fmt>", args...)
#define vassert_at(line, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(T5_ASSERT_FILE, line, 0, #expr "\n\t" fmt, __VA_ARGS__)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "%s\n\t<fmt>", "(expr)", args...)
#define vassertp_at(line, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(T5_ASSERT_FILE, line, 0, "%s\n\t" fmt, "(" #expr ")", __VA_ARGS__)) \
		__debugbreak(); } while (0)

// Assert_MyHandler(file, line, type, "%s\n\t<fmt>", "(expr)", args...): vassertp_at with an explicit assert
// type (the original passes 1 for some range checks, e.g. cg_camera.cpp's viewFov check)
#define vassertpt_at(line, type, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(T5_ASSERT_FILE, line, type, "%s\n\t" fmt, "(" #expr ")", __VA_ARGS__)) \
		__debugbreak(); } while (0)
