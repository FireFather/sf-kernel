<div align="left">

  [![Release][release-badge]][release-link]
  [![Commits][commits-badge]][commits-link]

  ![Downloads][downloads-badge]
  [![License][license-badge]][license-link]
 
</div>

# ♟️ sf-kernel
A warning-free, surgically reduced, standards-clean Stockfish kernel...a minimal Stockfish-derived chess engine core

sf-kernel is a stripped-down, modernized, and aggressively cleaned extraction of the Stockfish 16 engine core, designed to serve as a minimal, embeddable, analyzable chess engine kernel.

## 🧭 Overview
sf-kernel preserves Stockfish’s proven search, evaluation, and move generation logic, while removing large amounts of legacy scaffolding, compatibility baggage, and platform-specific cruft.
The result is a compact, modern C++ chess engine core that is:

- Dramatically smaller than upstream Stockfish
- Fully warning-clean at maximum compiler warning levels
- Standards-focused and tool-friendly
- Ideal for research, experimentation, embedding, and educational use

The project is not a “fork” in the traditional sense — it is a structural distillation of Stockfish into a minimal, self-contained kernel.

## 🎯 Focus
- Deterministic, warning-free builds
- Strict standards correctness (MSVC / Clang / GCC / MinGW)
- Minimal code surface and binary size
- Clean move generation, search, and evaluation core
- Maximum suitability for embedding, study, and research
- C++ standard conformance & static analysis cleanliness
- Maximum warning hygiene

All non-essential layers:
- UCI front-end glue
- Debugging infrastructure
- Legacy hacks
- Compatibility shims
- Platform clutter
- Feature bloat
have been removed or rewritten.

## 🔬 What Makes sf-kernel Different
- Source code footprint size: ~680 KB has been reduced to ~218 KB
- Compiler warnings reduced (via strict MSVC/Resharper/Clang analysis):
- 
	from 3029...

	 see: https://raw.githubusercontent.com/FireFather/sf-kernel/main/docs/stockfish_16_warnings.xml

	to 118...

	 see: https://raw.githubusercontent.com/FireFather/sf-kernel/main/docs/sf-kernel_1.0_warnings.xml

- Build surface: Large to Minimal
- Dependencies: Many to Tiny
- Embeddable: Hard to Easy
- Readability: Low to High

sf-kernel preserves Stockfish’s world-class playing strength while making the engine auditable, portable, and clean.

## 🧠 Core Philosophy
sf-kernel treats Stockfish as a kernel, not an application.
Everything unnecessary for pure chess intelligence has been cut away.

## 🚀 Goal
- sf-kernel is designed to be:
- Minimal: only the core chess engine remains
- Clean: warning-free at strict compiler levels
- Modern: idiomatic, tool-friendly C++
- Transparent: readable, auditable, hackable
- Fast: retains Stockfish’s proven performance characteristics

## ✨ Why Does sf-kernel Exists?
- Stockfish is extremely powerful — but also extremely large, complex, and legacy-heavy.
- A chess engine does not need to be huge to be strong
- Clean code and high performance are compatible
- Modern static analysis can be embraced instead of ignored
- sf-kernel exists to answer a simple question:
- What does Stockfish look like when you keep only the engine — and remove everything else?
- The result is a tiny, fast, research-friendly kernel that is easy to understand, modify, embed, and experiment with.

## 🚦 Status
✅ Compiles cleanly at strict warning levels
✅ Functional engine core
✅ Major size reduction vs Stockfish

✅ Warning-clean under strict analysis modes
🚧 Ongoing: cleanup and subsystem documentation

## 🧩 Based On
This project is based on:
Stockfish https://github.com/official-stockfish/Stockfish

## ⚙️To Build
- Visual Studio -> use the included project files: sf-kernel.sln or sf-kernel.vcxproj
- Msts2 mingx64 -> use the included shell scripts: make_avx2.sh, make_bmi2.sh or make_all.sh

## 📜 License
- sf-kernel is derived from Stockfish and is released under the GNU General Public License v3.
- See LICENSE for details.

## 📦 Current Scope
Included:
- Move generation
- Search
- Evaluation (NNUE supported)
- Position & bitboards
- Transposition table
- Threading kernel
- Time management core

Excluded:
- UCI shell
- Debug/UI tooling
- Legacy portability shims
- Platform-specific wrappers

[license-badge]:https://img.shields.io/github/license/FireFather/sf-kernel?style=for-the-badge&label=license&color=success
[license-link]:https://github.com/FireFather/sf-kernel/blob/main/LICENSE
[release-badge]:https://img.shields.io/github/v/release/FireFather/sf-kernel?style=for-the-badge&label=official%20release
[release-link]:https://github.com/FireFather/sf-kernel/releases/latest
[commits-badge]:https://img.shields.io/github/commits-since/FireFather/sf-kernel/latest?style=for-the-badge
[commits-link]:https://github.com/FireFather/sf-kernel/commits/main
[downloads-badge]:https://img.shields.io/github/downloads/FireFather/sf-kernel/total?color=success&style=for-the-badge
