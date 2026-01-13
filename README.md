# sf-kernel
A warning-free, standards-clean Stockfish 16 kernel...a minimal Stockfish-derived chess engine core

sf-kernel is a stripped-down, modernized, and aggressively cleaned extraction of the Stockfish 16 engine core, designed to serve as a minimal, embeddable, analyzable chess engine kernel.

It focuses on:

• Deterministic, warning-free builds
• Strict standards correctness (MSVC / Clang / GCC / MinGW)
• Minimal code surface and binary size
• Clean move generation, search, and evaluation core
• Maximum suitability for embedding, study, and research

All non-essential layers — UCI front-end glue, debugging infrastructure, legacy compatibility shims, and platform clutter — have been removed or rewritten.

The result is a tiny, fast, transparent Stockfish kernel suitable for:

• Game engines
• Chess AI research
• Reinforcement learning backends
• Engine embedding
• Minimal chess programs
• Code analysis and teaching

🔬 What Makes sf-kernel Different
Feature	Stockfish 16	sf-kernel
Binary size	~680 KB	~218 KB
Compiler warnings	Thousands	Zero (MSVC / Clang)
Build surface	Large	Minimal
Dependencies	Many	Tiny
Embeddable	Hard	Easy
Readability	Low	High

sf-kernel preserves Stockfish’s world-class playing strength while making the engine auditable, portable, and clean.

🧠 Core Philosophy

sf-kernel treats Stockfish as a kernel, not an application.

Everything unnecessary for pure chess intelligence has been cut away.

🧩 Based On

This project is based on:

Stockfish 16
https://github.com/official-stockfish/Stockfish

📦 Current Scope

Included:

• Move generation
• Search
• Evaluation (NNUE supported)
• Position & bitboards
• Transposition table
• Threading kernel
• Time management core

Excluded:

• UCI shell
• Debug/UI tooling
• Legacy portability shims
• Platform-specific wrappers

🚀 Goal

To be the cleanest Stockfish kernel ever published.