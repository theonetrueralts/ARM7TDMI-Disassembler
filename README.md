# ARM7TDMI-Decoder

Stateless ARM v4T disassembler aimed at the little-endian memory space that Game Boy Advance ROMs use.

**Decodes both ARM and THUMB** instructions, auto-detects mode boundaries, and outputs human-readable assembly.

[![license](https://img.shields.io/github/license/theonetrueralts/ARM7TDMI-Disassembler)](LICENSE)

## Features
- Can decode little-endian ARMv4T opcodes for the ARM7TDMI.
- A heuristic/override mechanism for switching between ARM and THUMB modes.
- A small CLI that allows users to load the ROM/override from a text file and output to either the console or a text file.

## Quick-start (CMake)
`CMakePresets.json` defaults to the **Ninja** generator.

If Ninja isn’t on your PATH, either install it (winget install Ninja-build.Ninja) or open an x64 Native Tools Command Prompt for VS 2022.

If you prefer another tool, create a `CMakeUserPresets.json` that overrides the generator, then call it with `cmake --preset <your-preset>`.

```bash
git clone https://github.com/theonetrueralts/ARM7TDMI-Disassembler.git

cd ARM7TDMI-Disassembler
cmake --preset x64-release # or x64-debug, linux-debug, etc.
cmake --build out/build/x64-release --target Disassembler

out\build\x64-release\Disassembler example.rom -r override.txt -d
```

## CLI Usage
```bash
Disassembler.exe <ROM file> [options]

Options:
  -r, --override <file>  Path to mode-override table
  -o, --out <file>       Write disassembly to <file> instead of stdout
  -d, --dec              Print immediates in decimal (default: hex)
Examples:
  Disassembler.exe example.rom
  Disassembler.exe example.rom --dec
  Disassembler.exe example.rom -r overrides.txt -o dump.txt
```

## Limitations
- Only supports little-endian ROMs; big-endian not yet implemented.
- Because CPU state isn't monitored, mode switching between THUMB and ARM mode cannot be determined with certainty. Manual overrides are required for cases where the mode cannot be determined by the heuristic.
- None of the coprocessor opcodes are currently implemented.

## License

MIT © 2025 theonetrueralts
