"""Turn a crash report's module+offset frames into function names and line numbers.

MinGW emits DWARF rather than a PDB, so the handler records offsets and addr2line
resolves them afterwards against the exe that produced them.

    python scripts/symbolise.py crashes/crash_20260816-131304.txt
    python scripts/symbolise.py crashes/crash_*.txt --exe build/.../AoNami.exe
"""
import argparse
import glob
import re
import subprocess
import sys
from pathlib import Path

FRAME = re.compile(r"\[(\d+)\]\s+(0x[0-9a-fA-F]+)\s+(\S+)\+0x([0-9a-fA-F]+)")

MINGW = Path(r"C:\Qt\Tools\mingw1310_64\bin")
DEFAULT_EXE = Path("build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug/AoNami.exe")


def tool(name):
    local = MINGW / (name + ".exe")
    return str(local) if local.exists() else name


def image_base(exe):
    try:
        out = subprocess.run([tool("objdump"), "-p", str(exe)],
                             capture_output=True, text=True, timeout=60).stdout
    except FileNotFoundError:
        return None
    m = re.search(r"ImageBase\s+([0-9a-fA-F]+)", out)
    return int(m.group(1), 16) if m else None


def resolve(exe, addresses):
    if not addresses:
        return {}
    try:
        out = subprocess.run([tool("addr2line"), "-f", "-C", "-e", str(exe)] +
                             [hex(a) for a in addresses],
                             capture_output=True, text=True, timeout=120).stdout
    except FileNotFoundError:
        return {}
    lines = [l.strip() for l in out.splitlines()]
    result = {}
    for i, addr in enumerate(addresses):
        func = lines[2 * i] if 2 * i < len(lines) else "?"
        loc = lines[2 * i + 1] if 2 * i + 1 < len(lines) else "?"
        result[addr] = (func, loc)
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reports", nargs="+")
    ap.add_argument("--exe", default=str(DEFAULT_EXE))
    args = ap.parse_args()

    exe = Path(args.exe)
    if not exe.exists():
        sys.exit(f"exe not found: {exe}\nPass the build that produced the crash with --exe.")
    base = image_base(exe)
    if base is None:
        sys.exit("could not read ImageBase - is objdump on PATH?")

    own = exe.name.lower()
    for pattern in args.reports:
        for path in sorted(glob.glob(pattern)):
            text = Path(path).read_text(encoding="utf-8", errors="replace")
            frames = FRAME.findall(text)
            # Only our own module resolves; system DLLs have no DWARF here.
            wanted = [int(off, 16) for _, _, mod, off in frames if mod.lower() == own]
            table = resolve(exe, [base + o for o in wanted])

            print(f"=== {path}")
            for line in text.splitlines():
                if line.startswith(("time", "reason", "exception", "address", "operation")):
                    print("   " + line.strip())
            print()
            for idx, addr, mod, off in frames:
                if mod.lower() != own:
                    print(f"   [{idx}] {mod}+0x{off}")
                    continue
                func, loc = table.get(base + int(off, 16), ("?", "?"))
                print(f"   [{idx}] {func}")
                print(f"        {loc}")
            print()


if __name__ == "__main__":
    main()
