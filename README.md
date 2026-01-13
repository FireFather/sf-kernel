# sf-kernel
A warning-free, surgically reduced, standards-clean Stockfish kernel...a minimal Stockfish-derived chess engine core

sf-kernel is a stripped-down, modernized, and aggressively cleaned extraction of the Stockfish 16 engine core, designed to serve as a minimal, embeddable, analyzable chess engine kernel.

## Overview
sf-kernel preserves Stockfish’s proven search, evaluation, and move generation logic, while removing large amounts of legacy scaffolding, compatibility baggage, and platform-specific cruft.
The result is a compact, modern C++ chess engine core that is:

- Dramatically smaller than upstream Stockfish
- Fully warning-clean at maximum compiler warning levels
- Standards-focused and tool-friendly
- Ideal for research, experimentation, embedding, and educational use

The project is not a “fork” in the traditional sense — it is a structural distillation of Stockfish into a minimal, self-contained kernel.

## Focus
- Deterministic, warning-free builds
- Strict standards correctness (MSVC / Clang / GCC / MinGW)
- Minimal code surface and binary size
- Clean move generation, search, and evaluation core
- Maximum suitability for embedding, study, and research

All non-essential layers:
- UCI front-end glue
- Debugging infrastructure
- Legacy hacks & compatibility shims
- Platform clutter
have been removed or rewritten.

The result is a tiny, fast, transparent Stockfish kernel suitable for:

- Game engines
- Chess AI research
- Reinforcement learning backends
- Engine embedding
- Minimal chess programs
- Code analysis and teaching

## 🔬 What Makes sf-kernel Different
- Source code footprint size:	~680 KB	has been reduced to ~218 KB
- Compiler warnings reduced from 3029 to 24 (via MSVC / Resharper / Clang analysis)
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

## Why sf-kernel?
- Stockfish is extremely powerful — but also extremely large, complex, and legacy-heavy.
- sf-kernel exists to answer a simple question:
- What does Stockfish look like when you keep only the engine — and remove everything else?
- The result is a tiny, fast, research-friendly kernel that is easy to understand, modify, embed, and experiment with.

## Status
✔ Compiles cleanly at strict warning levels

✔ Functional engine core

✔ Major size reduction vs Stockfish

🚧 Ongoing: cleanup and subsystem documentation

## 🧩 Based On
This project is based on:
Stockfish https://github.com/official-stockfish/Stockfish

## License
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

