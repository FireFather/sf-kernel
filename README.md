<div align="left">

  [![Release][release-badge]][release-link]
  [![Commits][commits-badge]][commits-link]

  ![Downloads][downloads-badge]
  [![License][license-badge]][license-link]
 
</div>

# ♟️ sf-kernel
A warning-free, surgically reduced, standards-clean Stockfish kernel...a minimal Stockfish-derived chess engine core.

This is a stripped-down, modernized, and aggressively cleaned extraction of the Stockfish engine core, designed to serve as a minimal, embeddable, analyzable chess engine kernel.

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

## 🧠 Core Philosophy
sf-kernel treats Stockfish as a kernel, not an application.
Everything unnecessary for pure chess intelligence has been cut away.

## 📦 What Was Removed
All non-essential layers:

- Debugging 
- Legacy hacks
- Compatibility shims
- Platform clutter
- Feature bloat
- Syzygy TB code
- Classical Evaluation
- Training infrastructure (Not runtime logic)
- Legacy platform glue (Modern C++ only)
- Debug infrastructure (Clean release kernel)

## 🧱 What Remains
- Move generation
- Search & pruning logic
- Evaluation & NNUE interface
- Hash tables
- Threading
- Position, legality, SEE, repetition, etc.
- UCI interface
- Time Management

It is Stockfish — just without the noise.

```
perft 7
a2a3: 106743106
b2b3: 133233975
c2c3: 144074944
d2d3: 227598692
e2e3: 306138410
f2f3: 102021008
g2g3: 135987651
h2h3: 106678423
a2a4: 137077337
b2b4: 134087476
c2c4: 157756443
d2d4: 269605599
e2e4: 309478263
f2f4: 119614841
g2g4: 130293018
h2h4: 138495290
b1a3: 120142144
b1c3: 148527161
g1f3: 147678554
g1h3: 120669525

Nodes: 3195901860
Time:  8.86661 sec
Speed: 360442269 N/s
```

## 🔬 What Makes sf-kernel Different
- Source code footprint size: ~680 KB has been reduced to ~218 KB
- Compiler warnings reduced from 3029 to 118 (via strict MSVC/Resharper/Clang analysis, see docs/stockfish_16_warnings.xml and docs/sf-kernel_1.0_warnings.xml for details)
- Build surface: from Large to Minimal
- Dependencies: from Many to Tiny
- Embeddable: from Hard to Easy
- Readability: from Low to High

sf-kernel preserves Stockfish’s world-class playing strength while making the engine auditable, portable, and clean.

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

## ⚙️To Build
- Visual Studio -> use the included project files: sf-kernel.sln or sf-kernel.vcxproj
- MSYS2 mingw64 -> use the included shell scripts: make_avx2.sh, make_bmi2.sh or make_all.sh

## 🧩 Based On
This project is based on:
Stockfish https://github.com/official-stockfish/Stockfish

## 📜 License
- sf-kernel (like Stockfish) is released under the GNU General Public License v3.
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
