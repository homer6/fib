# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- `make` or `make all` - Build the optimized executable
- `make run` - Build and run the program
- `make debug` - Build with debug symbols (-g -O0)
- `make clean` - Remove all build artifacts
- `make rebuild` - Clean and rebuild from scratch

## Architecture

This is a high-performance Fibonacci calculator implementation in C++14 that uses compile-time computation to generate a lookup table.

### Key Design Decisions

The `Fibonacci` class (fib.cpp) employs an Immediately Invoked Lambda Expression (IILE) to generate a lookup table at compile time. This approach:
- Provides O(1) runtime complexity for Fibonacci number retrieval
- Supports Fibonacci numbers up to F(93), the maximum that fits in std::uint64_t
- Uses C++14 constexpr capabilities for compile-time table generation
- Includes micro-benchmarking code to measure sub-nanosecond performance

### Compilation Requirements

- C++23 compiler required (uses -std=c++2b)
- Uses g++ by default (configured in Makefile)
- Optimization level -O3 for release builds