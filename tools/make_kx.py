#!/usr/bin/env python3
import struct
import sys
from pathlib import Path

KX_MAGIC = 0x314B584B  # must match kernel

def main():
    if len(sys.argv) != 3:
        print("usage: make_kx.py <input_bin> <output_kx>")
        sys.exit(1)

    in_file = Path(sys.argv[1])
    out_file = Path(sys.argv[2])

    code = in_file.read_bytes()

    header = struct.pack(
        "<IIII",
        KX_MAGIC,   # magic
        0,          # entry offset
        len(code),  # code_size
        0           # data_size
    )

    out_file.write_bytes(header + code)
    print(f"Built {out_file} ({len(code)} bytes code)")

if __name__ == "__main__":
    main()
