"""Run original and reconstructed x86 functions side by side in one emulator.

The original exe (verified by sha256 against target.toml) and build/recon.dll are both mapped into a
single Unicorn x86-32 address space. A call pushes cdecl arguments, runs to a sentinel return address,
then reads EAX/ST0 and every buffer passed by pointer. Comparing the two runs on the same inputs
checks behaviour, independent of which compiler built the reconstruction.
"""
import hashlib
import math
import os
import struct
import tomllib
from pathlib import Path

import pefile
from unicorn import UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_MODE_32, Uc, UcError
from unicorn.x86_const import (UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EIP,
                               UC_X86_REG_ES, UC_X86_REG_ESP, UC_X86_REG_FS, UC_X86_REG_GDTR, UC_X86_REG_GS,
                               UC_X86_REG_SS, UC_X86_REG_FP0,
                               UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_MXCSR)

ROOT = Path(__file__).resolve().parents[2]
STUBS, STUBS_SIZE = 0x00080000, 0x00008000
STACK, STACK_SIZE = 0x00100000, 0x00100000
HEAP, HEAP_SIZE = 0x00300000, 0x00100000
FIXTURE, FIXTURE_SIZE = 0x0C000000, 0x00100000   # Fixture.alloc(): dvars and other state a case builds (cg_s alone is 455 KB)
TEB, TEB_SIZE = 0x00090000, 0x00001000           # thread environment block (FS base)
GDT = 0x00091000                                 # one descriptor: FS -> TEB
TLS_THREAD_VALUES = 0x14       # offset in the TLS block of the engine's thread-value array pointer (Sys_GetValue)
THREAD_VALUE_ARRAY = 0x3000    # where the harness puts that array, inside the TLS region
THREAD_VALUE_BUFFERS = 0x4000  # one 4 KB zeroed buffer per thread value
THREAD_VALUE_COUNT = 8
CRT_PTD = 0x0009A000           # the CRT's per-thread data block (_tiddata, 0x214 bytes in VC90), inside TLS
CRT_HEAP, CRT_HEAP_SIZE = 0x000A0000, 0x00060000  # HeapAlloc/_calloc_crt for CRT code the tests reach
DVARS, DVARS_SIZE = 0x0E000000, 0x00200000       # a default dvar_s for every dvar pointer global
TLS, TLS_SIZE = 0x00094000, 0x0000C000           # the exe's __declspec(thread) block for this thread
SENTINEL = 0x00200000
INSTRUCTION_LIMIT = 2_000_000


# --- Win32 ------------------------------------------------------------------------------------------------
# Imports the original calls are pointed at stubs (a `ret n` each) whose behaviour comes from a small model
# of one window, its cursor and the cursor-show counter. Every stub call is logged with its arguments and
# results; the original and the reconstruction must make the same calls in the same order. The
# reconstruction reaches the same stubs: the hybrid link resolves its __imp__ references to the original's
# import table. Imports without a stub keep their unmapped address, so calling one faults visibly.

def _rect(mu, p):
    return struct.unpack("<4i", bytes(mu.mem_read(p, 16)))


def _w32_GetWindowRect(m, hwnd, p):
    if hwnd != m.win["hwnd"]:
        return 0
    m.mu.mem_write(p, struct.pack("<4i", *m.win["rect"]))
    return 1


def _w32_GetCursorPos(m, p):
    m.mu.mem_write(p, struct.pack("<2i", *m.win["cursor"]))
    return 1


def _w32_SetCursorPos(m, x, y):
    x, y = _s32(x), _s32(y)
    m.win["cursor"] = (x, y)
    return 1


def _w32_GetForegroundWindow(m):
    return m.win["foreground"]


def _w32_client_origin(m):
    left, top = m.win["rect"][0], m.win["rect"][1]
    return left + m.win["border"][0], top + m.win["border"][1]


def _w32_ScreenToClient(m, hwnd, p):
    if hwnd != m.win["hwnd"]:
        return 0
    ox, oy = _w32_client_origin(m)
    x, y = struct.unpack("<2i", bytes(m.mu.mem_read(p, 8)))
    m.mu.mem_write(p, struct.pack("<2i", x - ox, y - oy))
    return 1


def _w32_ClientToScreen(m, hwnd, p):
    if hwnd != m.win["hwnd"]:
        return 0
    ox, oy = _w32_client_origin(m)
    x, y = struct.unpack("<2i", bytes(m.mu.mem_read(p, 8)))
    m.mu.mem_write(p, struct.pack("<2i", x + ox, y + oy))
    return 1


def _w32_ShowCursor(m, show):
    m.win["show_count"] += 1 if show else -1
    if m.win["show_count"] < -50 or m.win["show_count"] > 50:
        raise RuntimeError("ShowCursor runaway")  # a loop that never converges: stop the run
    return m.win["show_count"] & 0xFFFFFFFF


def _w32_PostMessageA(m, hwnd, msg, wparam, lparam):
    return 1


def _k32_EnterCriticalSection(m, cs):
    return 0


def _k32_LeaveCriticalSection(m, cs):
    return 0


def _k32_GetLastError(m):
    return m.win["last_error"]


def _k32_SetLastError(m, code):
    m.win["last_error"] = code
    return 0


def _k32_TlsGetValue(m, index):
    # the CRT caches its FlsGetValue pointer in TLS slot __getvalueindex; hand it the stub instead
    if index == m.crt_getvalueindex:
        return m.fls_getvalue_stub
    return m.tls_slots.get(index, 0)


def _k32_TlsSetValue(m, index, value):
    m.tls_slots[index] = value
    return 1


def _k32_FlsGetValue(m, index):
    return CRT_PTD  # one thread: its CRT data block is always the same (prepared in Machine._install_crt)


def _k32_GetCurrentThreadId(m):
    return 0x1000


def _k32_InterlockedIncrement(m, p):
    v = (struct.unpack("<i", bytes(m.mu.mem_read(p, 4)))[0] + 1) & 0xFFFFFFFF
    m.mu.mem_write(p, struct.pack("<I", v))
    return v


def _k32_InterlockedDecrement(m, p):
    v = (struct.unpack("<i", bytes(m.mu.mem_read(p, 4)))[0] - 1) & 0xFFFFFFFF
    m.mu.mem_write(p, struct.pack("<I", v))
    return v


def _k32_InitializeCriticalSectionAndSpinCount(m, cs, spin):
    return 1


def _k32_InitializeCriticalSection(m, cs):
    return 0


def _k32_HeapAlloc(m, heap, flags, size):
    va = m.crt_heap_next
    m.crt_heap_next = (va + size + 15) & ~15
    if m.crt_heap_next > CRT_HEAP + CRT_HEAP_SIZE:
        raise RuntimeError("harness CRT heap exhausted")
    m.mu.mem_write(va, b"\0" * size)
    return va


def _k32_HeapFree(m, heap, flags, p):
    return 1


def _k32_GetModuleHandleW(m, name):
    return 0x7C800000  # a module handle (the CRT waits forever on NULL); GetProcAddress finds nothing in it


def _k32_GetProcAddress(m, module, name):
    return 0  # optional entry points (EncodePointer, ...) are absent: CRT code takes its fallbacks


ERROR_DEVICE_NOT_CONNECTED = 1167


def _xinput_GetState(m, user, p):
    pad = m.win["pad"] if user == 0 else None
    if pad is None:
        return ERROR_DEVICE_NOT_CONNECTED
    m.mu.mem_write(p, struct.pack("<IHBBhhhh", pad["packet"], pad["buttons"], pad["lt"], pad["rt"],
                                  pad["lx"], pad["ly"], pad["rx"], pad["ry"]))
    return 0


def _xinput_GetCapabilities(m, user, flags, p):
    if user != 0 or m.win["pad"] is None:
        return ERROR_DEVICE_NOT_CONNECTED
    # XINPUT_CAPABILITIES: Type = XINPUT_DEVTYPE_GAMEPAD, SubType = XINPUT_DEVSUBTYPE_GAMEPAD, full ranges
    m.mu.mem_write(p, struct.pack("<BBHHBBhhhhHH", 1, 1, 0, 0xFFFF, 0xFF, 0xFF, -64, -64, -64, -64, 0xFFFF, 0xFFFF))
    return 0


def _xinput_SetState(m, user, p):
    return ERROR_DEVICE_NOT_CONNECTED if user != 0 or m.win["pad"] is None else 0


def _k32_OutputDebugStringA(m, text):
    return 0  # the text is in the program's memory; what was printed shows up there (console buffers)


def _winmm_timeGetTime(m):
    return m.win["time_ms"]


WIN32 = {  # name: (argument count, handler, positions of pointer arguments)
    "TlsGetValue": (1, _k32_TlsGetValue, set()),
    "TlsSetValue": (2, _k32_TlsSetValue, set()),
    "FlsGetValue": (1, _k32_FlsGetValue, set()),
    "GetCurrentThreadId": (0, _k32_GetCurrentThreadId, set()),
    "InterlockedIncrement": (1, _k32_InterlockedIncrement, {0}),
    "InterlockedDecrement": (1, _k32_InterlockedDecrement, {0}),
    "InitializeCriticalSectionAndSpinCount": (2, _k32_InitializeCriticalSectionAndSpinCount, {0}),
    "InitializeCriticalSection": (1, _k32_InitializeCriticalSection, {0}),
    "HeapAlloc": (3, _k32_HeapAlloc, set()),
    "HeapFree": (3, _k32_HeapFree, {2}),
    "GetModuleHandleW": (1, _k32_GetModuleHandleW, {0}),
    "GetProcAddress": (2, _k32_GetProcAddress, {1}),
    "timeGetTime": (0, _winmm_timeGetTime, set()),
    "OutputDebugStringA": (1, _k32_OutputDebugStringA, {0}),
    # one gamepad on port 0, connected or not (window model "pad")
    "XInputGetState": (2, _xinput_GetState, {1}),
    "XInputGetCapabilities": (3, _xinput_GetCapabilities, {2}),
    "XInputSetState": (2, _xinput_SetState, {1}),
    # one thread: critical sections never block
    "EnterCriticalSection": (1, _k32_EnterCriticalSection, {0}),
    "LeaveCriticalSection": (1, _k32_LeaveCriticalSection, {0}),
    "GetLastError": (0, _k32_GetLastError, set()),
    "SetLastError": (1, _k32_SetLastError, set()),
    "GetWindowRect": (2, _w32_GetWindowRect, {1}),
    "GetCursorPos": (1, _w32_GetCursorPos, {0}),
    "SetCursorPos": (2, _w32_SetCursorPos, set()),
    "GetForegroundWindow": (0, _w32_GetForegroundWindow, set()),
    "ScreenToClient": (2, _w32_ScreenToClient, {1}),
    "ClientToScreen": (2, _w32_ClientToScreen, {1}),
    "ShowCursor": (1, _w32_ShowCursor, set()),
    "PostMessageA": (4, _w32_PostMessageA, set()),
}


# imports by ordinal: (dll, ordinal) -> name
ORDINALS = {("xinput1_3.dll", 2): "XInputGetState", ("xinput1_3.dll", 3): "XInputSetState",
            ("xinput1_3.dll", 4): "XInputGetCapabilities", ("xinput1_3.dll", 5): "XInputEnable"}


def default_window():
    """The model of the machine a call starts from: one window and its cursor, the cursor-show counter,
    the time, and a gamepad on port 0 ("pad": None for disconnected, or packet/buttons/lt/rt/lx/ly/rx/ry
    as in XINPUT_STATE). A case's setup can change any of it."""
    return {"hwnd": 0x00010042, "foreground": 0x00010042, "rect": (100, 50, 1380, 770), "border": (8, 31),
            "cursor": (640, 400), "show_count": 0, "last_error": 0, "time_ms": 1234567, "pad": None}


def _s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def _page(n):
    return (n + 0xFFF) & ~0xFFF


def _st0(mu):
    top = (mu.reg_read(UC_X86_REG_FPSW) >> 11) & 7
    mant, se = mu.reg_read(UC_X86_REG_FP0 + top)
    sign = -1.0 if se & 0x8000 else 1.0
    exp = se & 0x7FFF
    if exp == 0 and mant == 0:
        return math.copysign(0.0, sign)
    if exp == 0x7FFF:
        return sign * math.inf if (mant << 1) & ((1 << 64) - 1) == 0 else math.nan
    return sign * mant / (1 << 63) * 2.0 ** (exp - 16383)


def dvar_undefined_bytes(dvar_va, defined=1):
    """Addresses of a dvar_s whose contents the original leaves undefined: Dvar_SetBool, Dvar_SetInt and
    Dvar_SetFloat build a 16-byte DvarValue with only one member set (`defined` bytes: 1 for bool, 4 for int
    and float) and copy the stack garbage in the rest into current and latched (records/bugs.tsv B0009).
    Comparing them would compare two builds' stack layouts."""
    out = set()
    for field in (0x18, 0x28):  # current, latched
        out.update(range(dvar_va + field + defined, dvar_va + field + 16))
    return out


def _descriptor(base, limit, access, flags):
    """An x86 segment descriptor (8 bytes)."""
    return struct.pack("<HHBBBB", limit & 0xFFFF, base & 0xFFFF, (base >> 16) & 0xFF, access,
                       ((limit >> 16) & 0x0F) | (flags << 4), (base >> 24) & 0xFF)


class Image:
    def __init__(self, path: Path):
        self.pe = pefile.PE(str(path), fast_load=True)
        self.pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]])
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.size = _page(self.pe.OPTIONAL_HEADER.SizeOfImage)
        self.data = self.pe.get_memory_mapped_image()
        self.exports = {}
        if hasattr(self.pe, "DIRECTORY_ENTRY_EXPORT"):
            for e in self.pe.DIRECTORY_ENTRY_EXPORT.symbols:
                self.exports[e.name.decode()] = self.base + e.address


def original_symbols(build="CoDMPServer"):
    """Decorated name -> address, from the reference build's MAP (analysis/<build>/map/publics.tsv)."""
    import csv
    out = {}
    with open(ROOT / "analysis" / build / "map" / "publics.tsv", newline="") as f:
        for r in csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE):
            out.setdefault(r["name"], int(r["va"], 16))
    return out


def original_dvar_statics(build="CoDMPServer"):
    """(address, name) of every file-static `const dvar_s *` in the reference build's PDB (pdb/statics.tsv)."""
    import csv
    with open(ROOT / "analysis" / build / "pdb" / "statics.tsv", newline="") as f:
        return [(int(r["va"], 16), r["name"]) for r in csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE)
                if r["type"] == "const dvar_s*" and r["va"] != "00000000"]


def original_exe(build="CoDMPServer") -> Path:
    """Locate the original exe: $T5_ORIGINALS/<build>.exe, verified against target.toml."""
    cfg = tomllib.loads((ROOT / "target.toml").read_text())
    spec = next(b for b in cfg["build"] if b["id"] == build)["exe"]
    path = Path(os.environ.get("T5_ORIGINALS", ROOT / ".work" / "originals")) / f"{build}.exe"
    if not path.exists():
        raise SystemExit(f"missing {path}: run tools/fetch_originals.py {build} {path.parent}")
    if hashlib.sha256(path.read_bytes()).hexdigest() != spec["sha256"]:
        raise SystemExit(f"{path} does not match target.toml sha256")
    return path


class Fixture:
    """State a test case builds before a call: memory in a scratch region, and pokes into the original's
    globals. Pokes are journaled like any other write, so they are rolled back after the call."""

    def __init__(self, machine):
        self.m = machine
        self.next = FIXTURE

    def alloc(self, data: bytes, align=16):
        va = self.next
        if va + len(data) > FIXTURE + FIXTURE_SIZE:
            raise MemoryError(f"fixture region full: {len(data):#x} bytes more needed at {va:#x}")
        self.m.mu.mem_write(va, bytes(data))
        self.next = (va + len(data) + align - 1) & ~(align - 1)
        return va

    def poke(self, va, data: bytes):
        self.m.mu.mem_write(va, bytes(data))

    def u32(self, va, v):
        self.poke(va, struct.pack("<I", v & 0xFFFFFFFF))

    def i32(self, va, v):
        self.poke(va, struct.pack("<i", v))

    def f32(self, va, v):
        self.poke(va, struct.pack("<f", v))

    def u8(self, va, v):
        self.poke(va, bytes([v & 0xFF]))

    @property
    def win(self):
        return self.m.win


class Machine:
    def __init__(self, *images: Image):
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        for img in images:
            self.mu.mem_map(img.base, img.size)
            self.mu.mem_write(img.base, img.data[:img.size])
        self.mu.mem_map(STUBS, STUBS_SIZE)
        self.mu.mem_map(STACK, STACK_SIZE)
        self.mu.mem_map(FIXTURE, FIXTURE_SIZE)
        self.mu.mem_map(HEAP, HEAP_SIZE)
        self.mu.mem_map(SENTINEL, 0x1000)
        self.images = images
        self.undefined = set()
        self.win = default_window()
        self.calls = []
        self._install_win32(images[0])
        self._install_tls(images[0])
        self._install_crt()
        self._install_dvars()
        # Writes into any mapped image (the original's globals) and into the thread's TLS block are
        # journaled so each call can be rolled back and its side effects compared: see call().
        # fixture memory too: state a test built (a client struct, a dvar) is state the call may change
        self._journaled = [(img.base, img.size) for img in images] + [(TLS, TLS_SIZE), (DVARS, DVARS_SIZE),
                                                                      (FIXTURE, FIXTURE_SIZE)]
        self._journal = {}
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self._on_write)

    def _install_win32(self, img):
        """Point the original's import slots for WIN32 at stubs."""
        img.pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
        self._stubs = {}
        slot = STUBS
        for d in getattr(img.pe, "DIRECTORY_ENTRY_IMPORT", []):
            for imp in d.imports:
                name = imp.name.decode() if imp.name else ORDINALS.get((d.dll.decode().lower(), imp.ordinal))
                if name in WIN32:
                    nargs, fn, ptrs = WIN32[name]
                    code = b"\xC3" if nargs == 0 else b"\xC2" + struct.pack("<H", 4 * nargs)  # ret / ret n
                    self._stubs[slot] = (name, nargs, fn, ptrs)
                else:
                    # any other import: a trap that stops the run naming the import. (Left alone, the slot
                    # holds a name offset from the file, and a call through it lands somewhere random.)
                    code = b"\xC3"
                    self._stubs[slot] = (f"{d.dll.decode()}!{name or imp.ordinal}", 0, None, set())
                self.mu.mem_write(slot, code)
                self.mu.mem_write(imp.address, struct.pack("<I", slot))
                slot += 16
        # functions the CRT looks up at run time instead of importing (FlsGetValue through a cached pointer)
        for name in ("FlsGetValue",):
            nargs, fn, ptrs = WIN32[name]
            self.mu.mem_write(slot, b"\xC2" + struct.pack("<H", 4 * nargs))
            self._stubs[slot] = (name, nargs, fn, ptrs)
            slot += 16
        self.mu.hook_add(UC_HOOK_CODE, self._on_stub, begin=STUBS, end=STUBS + STUBS_SIZE - 1)

    def _install_tls(self, img):
        """One thread: a TEB at FS (self pointer at 0x18, TLS array at 0x2C) whose TLS array slot for the
        exe's TLS index points at a fresh copy of the exe's __declspec(thread) template (reset per call)."""
        self.mu.mem_map(TEB, TEB_SIZE)
        self.mu.mem_map(TLS, TLS_SIZE)
        img.pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_TLS"]])
        self._tls_template = b""
        tls = getattr(img.pe, "DIRECTORY_ENTRY_TLS", None)
        index = 0
        if tls is not None:
            t = tls.struct
            start, end = t.StartAddressOfRawData - img.base, t.EndAddressOfRawData - img.base
            self._tls_template = bytes(img.data[start:end]) + b"\0" * t.SizeOfZeroFill
            assert len(self._tls_template) + 16 <= TLS_SIZE, "TLS block larger than the harness region"
            index = struct.unpack("<I", bytes(img.data[t.AddressOfIndex - img.base:t.AddressOfIndex - img.base + 4]))[0]
        array = TEB + 0x800
        self.mu.mem_write(TEB + 0x18, struct.pack("<I", TEB))
        self.mu.mem_write(TEB + 0x2C, struct.pack("<I", array))
        self.mu.mem_write(array + 4 * index, struct.pack("<I", TLS))
        # 32-bit mode ignores FS_BASE: FS must select a GDT descriptor whose base is the TEB. With a GDT
        # in place every segment register is loaded from it: flat code and data, FS on the TEB.
        self.mu.mem_map(GDT, 0x1000)
        self.mu.mem_write(GDT + 8 * 1, _descriptor(0, 0xFFFFF, access=0x9A, flags=0xC))  # code, flat
        self.mu.mem_write(GDT + 8 * 2, _descriptor(0, 0xFFFFF, access=0x92, flags=0xC))  # data, flat
        self.mu.mem_write(GDT + 8 * 3, _descriptor(TEB, TEB_SIZE - 1, access=0x92, flags=0x4))
        self.mu.reg_write(UC_X86_REG_GDTR, (0, GDT, 8 * 8 - 1, 0))
        self.mu.reg_write(UC_X86_REG_CS, 1 << 3)
        for reg in (UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_GS):
            self.mu.reg_write(reg, 2 << 3)
        self.mu.reg_write(UC_X86_REG_FS, 3 << 3)

    def _install_crt(self):
        """The VC90 CRT's per-thread data (_getptd): run the original __initptd on a fresh block once, the way
        the CRT sets up a thread, and keep it as the template every call starts from. FlsGetValue (via the
        TlsGetValue cache, see _k32_TlsGetValue) returns this block."""
        syms = original_symbols()
        self.tls_slots = {}
        self.crt_heap_next = CRT_HEAP
        self.mu.mem_map(CRT_HEAP, CRT_HEAP_SIZE)
        self.crt_getvalueindex = struct.unpack("<I", bytes(self.mu.mem_read(syms["___getvalueindex"], 4)))[0]
        # a stub function for FlsGetValue that the CRT calls through the pointer TlsGetValue hands it
        self.fls_getvalue_stub = next(a for a, st in self._stubs.items() if st[0] == "FlsGetValue")
        self._ptd_template = None
        # what the CRT's _heap_init would have set: a process heap handle, and the system heap as the
        # active heap (__SYSTEM_HEAP), so malloc goes to HeapAlloc
        self.mu.mem_write(syms["__crtheap"], struct.pack("<I", 0x00CE0000))
        self.mu.mem_write(syms["___active_heap"], struct.pack("<I", 1))
        self.mu.mem_write(CRT_PTD, b"\0" * 0x214)
        sp = STACK + STACK_SIZE - 0x100
        # the CRT's predefined locks (_mtinitlocks at startup); without them _lock recurses forever
        self.mu.mem_write(sp, struct.pack("<I", SENTINEL))
        self.mu.reg_write(UC_X86_REG_ESP, sp)
        self.mu.emu_start(syms["__mtinitlocks"], SENTINEL, count=100_000)
        frame = struct.pack("<III", SENTINEL, CRT_PTD, 0)  # return address, ptd, locinfo = NULL
        self.mu.mem_write(sp, frame)
        self.mu.reg_write(UC_X86_REG_ESP, sp)
        self.mu.emu_start(syms["__initptd"], SENTINEL, count=100_000)
        self.mu.mem_write(CRT_PTD, struct.pack("<Ii", 0x1000, -1))  # _tid, _thandle as _getptd sets them
        self._ptd_template = bytes(self.mu.mem_read(CRT_PTD, 0x214))
        self.crt_heap_base = self.crt_heap_next

    def _install_dvars(self):
        """Every `const dvar_s *` global in the MAP (and file static in the PDB) that is still NULL points at its
        own default dvar: as if
        all dvars were registered, each with its name, type bool and value 0. Code under test then reads
        dvars it doesn't care about without faulting; a test's setup replaces the ones it does care about.
        The dvars are journaled and reset per call like globals (code may change them, e.g. Dvar_SetBool)."""
        self.mu.mem_map(DVARS, DVARS_SIZE)
        self._dvars_undefined = set()
        img = self.images[0]
        out = bytearray()
        names = [(va, n) for n, va in original_symbols().items() if n.endswith("@@3PBUdvar_s@@B")]
        names += [(va, f"?{n}@@") for va, n in original_dvar_statics()]  # file-static dvar pointers (PDB)
        for va, n in sorted(set(names)):
            if struct.unpack("<I", bytes(self.mu.mem_read(va, 4)))[0] != 0 or not (img.base <= va < img.base + img.size):
                continue
            name = n[1:n.index("@@")].encode() + b"\0"
            dvar_va = DVARS + len(out)
            name_va = dvar_va + 0x70
            d = bytearray(0x70)
            struct.pack_into("<I", d, 0x00, name_va)
            self._dvars_undefined.update(dvar_undefined_bytes(dvar_va))
            out += d + name + b"\0" * (-(len(name)) % 16)
            self.mu.mem_write(va, struct.pack("<I", dvar_va))
        assert len(out) <= DVARS_SIZE
        self._dvars_template = bytes(out)
        self.mu.mem_write(DVARS, self._dvars_template)

    def _on_stub(self, mu, address, size, user):
        stub = self._stubs.get(address)
        if stub is None:
            return
        name, nargs, fn, ptrs = stub
        if fn is None:
            raise RuntimeError(f"unstubbed import {name}")
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = list(struct.unpack(f"<{nargs}I", bytes(mu.mem_read(esp + 4, 4 * nargs)))) if nargs else []
        result = fn(self, *args)
        mu.reg_write(UC_X86_REG_EAX, result & 0xFFFFFFFF)
        # log what the program could observe: arguments by value, except pointers (a pointer to a local
        # legitimately differs between builds; what the stub wrote through it is compared through the
        # program's own memory and results)
        self.calls.append((name, tuple("ptr" if i in ptrs else _s32(a) for i, a in enumerate(args)),
                           _s32(result & 0xFFFFFFFF)))

    def _on_write(self, mu, access, address, size, value, user):
        for base, length in self._journaled:
            if base <= address < base + length:
                for a in range(address, address + size):
                    if a not in self._journal:
                        self._journal[a] = bytes(mu.mem_read(a, 1))
                return

    def poke(self, va, data: bytes):
        self.mu.mem_write(va, data)

    def call(self, va, args, ret="int", setup=None, state=None):
        """args: ints, floats ('f', x), or bytes (passed by pointer, read back after).

        setup(fixture, state) runs first: it builds dvars and other state (Fixture.alloc), pokes the
        original's globals and adjusts the window model from the case's `state`. Both runs of a comparison
        get identical setups.

        Returns (return value, [buffer contents after the call], {address: byte} written in mapped
        images, [Win32 calls made], final window model). Image writes are rolled back afterwards, so calls
        never see each other's state. A crash returns ("fault: ...", [], writes, calls, window)."""
        mu = self.mu
        mu.mem_write(HEAP, b"\0" * HEAP_SIZE)
        mu.mem_write(FIXTURE, b"\0" * FIXTURE_SIZE)
        mu.mem_write(TLS, self._tls_template + b"\0" * (TLS_SIZE - len(self._tls_template)))
        # the engine's per-thread values (Sys_SetValue/Sys_GetValue, qcommon/threads.cpp): the TLS block
        # points at a value array; each value points at its own zeroed buffer, as a fresh thread's would
        mu.mem_write(TLS + TLS_THREAD_VALUES, struct.pack("<I", TLS + THREAD_VALUE_ARRAY))
        for i in range(THREAD_VALUE_COUNT):
            mu.mem_write(TLS + THREAD_VALUE_ARRAY + 4 * i, struct.pack("<I", TLS + THREAD_VALUE_BUFFERS + i * 0x1000))
        mu.mem_write(CRT_PTD, self._ptd_template)
        mu.mem_write(DVARS, self._dvars_template)
        self.undefined = set(self._dvars_undefined)  # bytes whose value the original leaves undefined
        self.tls_slots = {}
        self.crt_heap_next = self.crt_heap_base
        self.win = default_window()
        self.calls = []
        self._journal = {}
        if setup is not None:
            setup(Fixture(self), state)
        heap = HEAP
        words, bufs = [], []
        for a in args:
            if isinstance(a, (bytes, bytearray)):
                mu.mem_write(heap, bytes(a))
                bufs.append((heap, len(a)))
                words.append(heap)
                heap += _page(len(a) + 16)
            elif isinstance(a, tuple) and a[0] == "f":
                words.append(struct.unpack("<I", struct.pack("<f", a[1]))[0])
            elif isinstance(a, tuple) and a[0] == "d":
                words.extend(struct.unpack("<II", struct.pack("<d", a[1])))
            else:
                words.append(a & 0xFFFFFFFF)
        sp = STACK + STACK_SIZE - 0x100
        for w in reversed(words):
            sp -= 4
            mu.mem_write(sp, struct.pack("<I", w))
        sp -= 4
        mu.mem_write(sp, struct.pack("<I", SENTINEL))
        mu.reg_write(UC_X86_REG_ESP, sp)
        mu.reg_write(UC_X86_REG_EBP, 0)
        # FPU state as Windows starts a thread: x87 control word 0x27F (53-bit precision, round to
        # nearest, all exceptions masked), empty x87 stack, MXCSR 0x1F80 (all SSE exceptions masked).
        # Unmasked exceptions would send the CRT's floor() etc. into exception handling.
        mu.reg_write(UC_X86_REG_FPCW, 0x27F)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        mu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        mu.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        try:
            mu.emu_start(va, SENTINEL, count=INSTRUCTION_LIMIT)
            fault = None
            if mu.reg_read(UC_X86_REG_EIP) != SENTINEL:  # stopped by the instruction limit, not by returning
                fault = f"fault: no return within {INSTRUCTION_LIMIT:,} instructions (at {mu.reg_read(UC_X86_REG_EIP):08X})"
        except UcError as e:
            fault = f"fault: {e} at {mu.reg_read(UC_X86_REG_EIP):08X}"
        except RuntimeError as e:  # raised by a stub (e.g. a ShowCursor loop that never converges)
            mu.emu_stop()
            fault = f"fault: {e}"
        # side effects on globals: final bytes of every written address, then roll back
        globals_written = {a: bytes(mu.mem_read(a, 1)) for a in sorted(self._journal) if a not in self.undefined}
        for a, old in self._journal.items():
            mu.mem_write(a, old)
        self._journal = {}
        calls, window = list(self.calls), dict(self.win)
        if fault:
            return fault, [], globals_written, calls, window
        eax = mu.reg_read(UC_X86_REG_EAX)
        value = {"int": eax if eax < 1 << 31 else eax - (1 << 32),
                 "bool": eax & 0xFF,
                 "char": struct.unpack("<b", bytes([eax & 0xFF]))[0],
                 "short": struct.unpack("<h", struct.pack("<H", eax & 0xFFFF))[0],
                 "float": _st0(mu) if ret == "float" else None,
                 "void": None}[ret]
        return value, [bytes(mu.mem_read(a, n)) for a, n in bufs], globals_written, calls, window
