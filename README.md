# sf-kernel
A warning-free, standards-clean Stockfish 16 kernel...a minimal Stockfish-derived chess engine core

sf-kernel is a stripped-down, modernized, and aggressively cleaned extraction of the Stockfish 16 engine core, designed to serve as a minimal, embeddable, analyzable chess engine kernel.

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
— have been removed or rewritten.

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

## 🧩 Based On

This project is based on:

Stockfish 16
https://github.com/official-stockfish/Stockfish

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

## 🚀 Goal

To published a clean modern and optimized Stockfish kernel.