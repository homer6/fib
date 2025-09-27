# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- `make` or `make all` - Build the optimized executable
- `make run` - Build and run the program
- `make debug` - Build with debug symbols (-g -O0)
- `make clean` - Remove all build artifacts
- `make rebuild` - Clean and rebuild from scratch

## Architecture

This is a high-performance Fibonacci calculator implementation in C++23 that uses compile-time computation to generate lookup tables.

### Key Design Decisions

The `Fibonacci` class (fib.cpp) employs an Immediately Invoked Lambda Expression (IILE) to generate lookup tables at compile time. This approach:
- Provides O(1) runtime complexity for Fibonacci number retrieval
- Supports Fibonacci numbers up to F(186) using two lookup tables:
  - F(0) to F(93): 64-bit unsigned integers (std::uint64_t)
  - F(0) to F(186): 128-bit unsigned integers (unsigned __int128)
- Uses C++23 constexpr capabilities for compile-time table generation
- Includes micro-benchmarking code to measure sub-nanosecond performance

### Performance Optimizations

- Cache line alignment (64 bytes) for optimal memory access
- Forced inlining with `__attribute__((always_inline))`
- Branch prediction hints using `__builtin_expect`
- Link-Time Optimization (LTO) enabled
- Manual loop unrolling for throughput benchmarks
- Compiler flags: `-O3 -march=native -mtune=native -flto -funroll-loops -fomit-frame-pointer`

### Benchmark Methodology

- Uses `volatile` indices to force actual memory lookups in each iteration
- Prevents compiler optimization from eliminating the lookups
- 1000 iterations per measurement for stability
- High-resolution clock for sub-nanosecond precision

## Performance Results

Based on actual measurements with volatile indices (preventing compiler optimization):

- **64-bit lookups (F(0)-F(93))**: ~0.29-0.42 ns per lookup (2.4-3.4 billion ops/sec)
- **128-bit lookups (F(94)-F(186))**: ~0.75-1.08 ns per lookup (0.9-1.3 billion ops/sec)
- **Throughput**: 3.4-3.6 billion operations per second sustained

### Compilation Requirements

- C++23 compiler required (uses -std=c++2b)
- Uses g++ by default (configured in Makefile)
- Optimization level -O3 for release builds
- Requires compiler support for `unsigned __int128` (GCC/Clang extension)