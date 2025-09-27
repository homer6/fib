#include <iostream>
#include <array>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <algorithm>

// Cache line size for optimal alignment (typical x86-64/ARM)
#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

// Branch prediction hints
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Force inline for critical performance paths
#define FORCE_INLINE __attribute__((always_inline)) inline

class alignas(CACHE_LINE_SIZE) Fibonacci {
private:
    // F(93) is the largest Fibonacci number that fits in a std::uint64_t (12200160415121876738).
    static constexpr std::uint32_t MAX_N = 93;

public:
    static constexpr std::size_t TABLE_SIZE = MAX_N + 1;

private:
    // The compile-time generated lookup table, cache-aligned for optimal performance
    alignas(CACHE_LINE_SIZE) static constexpr auto LookupTable = []() constexpr {
        // The lambda is marked constexpr to ensure compile-time execution.
        std::array<std::uint64_t, TABLE_SIZE> table = {};

        // C++14 allows modification of local variables and loops within constexpr functions.
        if (TABLE_SIZE > 0) table[0] = 0;
        if (TABLE_SIZE > 1) table[1] = 1;

        for (std::size_t i = 2; i < TABLE_SIZE; ++i) {
            table[i] = table[i - 1] + table[i - 2];
        }
        return table;
    }(); // <-- The lambda is immediately invoked here.

public:
    // The optimized lookup function: O(1) complexity with aggressive inlining
    [[nodiscard]] FORCE_INLINE std::uint64_t operator()(std::uint32_t n) const noexcept {
        // In release builds, skip bounds checking for maximum performance
#ifdef DEBUG
        if (UNLIKELY(n > MAX_N)) {
            return 0;
        }
#endif

        // Direct lookup with no bounds check in release mode
        // The compiler can optimize this to a single MOV instruction
        return LookupTable[n];
    }

    // Unsafe version for when you absolutely know n is valid
    [[nodiscard]] FORCE_INLINE std::uint64_t unsafe_get(std::uint32_t n) const noexcept {
        return LookupTable[n];
    }

    // Prefetch multiple values for better cache utilization
    FORCE_INLINE void prefetch_range(std::uint32_t start, std::uint32_t count) const noexcept {
        const std::uint32_t end = std::min(start + count, static_cast<std::uint32_t>(TABLE_SIZE));
        for (std::uint32_t i = start; i < end; i += 8) {
            __builtin_prefetch(&LookupTable[i], 0, 3);
        }
    }
};

// For strict C++14 compatibility (before inline variables were introduced in C++17),
// static constexpr members may require an out-of-class definition if they are ODR-used.
// We use decltype to ensure the type matches the auto-deduced type inside the class.
constexpr decltype(Fibonacci::LookupTable) Fibonacci::LookupTable;


// High-resolution micro benchmarking
int main() {
    // We use this accumulator as a "sink" to ensure the compiler doesn't optimize away the loops.
    // Mark as volatile to prevent over-optimization
    volatile std::uint64_t sink = 0;

    Fibonacci fib;

    // Warmup CPU caches and branch predictor
    for (int warmup = 0; warmup < 1000; ++warmup) {
        sink += fib.unsafe_get(warmup % 94);
    }

    std::cout << "Optimized Fibonacci micro-benchmark (Maximum Performance Edition):\n" << std::endl;

    // Benchmark individual calculations using unsafe_get for absolute minimum overhead
    for (std::uint32_t i = 0; i <= 10; ++i) {
        // Use high_resolution_clock for best precision
        const auto start = std::chrono::high_resolution_clock::now();

        // Multiple iterations to get more stable measurements
        constexpr int micro_iterations = 100;
        std::uint64_t local_sum = 0;
        for (int j = 0; j < micro_iterations; ++j) {
            local_sum += fib.unsafe_get(i);
        }

        const auto end = std::chrono::high_resolution_clock::now();
        sink += local_sum;

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const double avg_ns = static_cast<double>(duration.count()) / micro_iterations;

        // Increased width to 20 to accommodate F(93)
        std::cout << "F(" << std::setw(2) << i << ") = " << std::setw(20) << (local_sum / micro_iterations)
                  << " [" << std::fixed << std::setprecision(3) << avg_ns << " ns avg]" << std::endl;
    }

    // Benchmark larger values with prefetching
    std::cout << "\nLarger values:" << std::endl;
    // Explicit array definition for broad compatibility
    std::uint32_t large_values[] = {20, 40, 60, 80, 93};

    // Prefetch all large values
    fib.prefetch_range(20, 74);

    for (std::uint32_t n : large_values) {
        const auto start = std::chrono::high_resolution_clock::now();

        constexpr int micro_iterations = 100;
        std::uint64_t local_sum = 0;
        for (int j = 0; j < micro_iterations; ++j) {
            local_sum += fib.unsafe_get(n);
        }

        const auto end = std::chrono::high_resolution_clock::now();
        sink += local_sum;

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const double avg_ns = static_cast<double>(duration.count()) / micro_iterations;

        std::cout << "F(" << std::setw(2) << n << ") = " << std::setw(20) << (local_sum / micro_iterations)
                  << " [" << std::fixed << std::setprecision(3) << avg_ns << " ns avg]" << std::endl;
    }

    // Benchmark multiple iterations for statistical accuracy (Throughput measurement)
    std::cout << "\nThroughput test (10M iterations):" << std::endl;
    constexpr int iterations = 10000000;

    std::uint32_t iteration_values[] = {10, 50, 93};
    for (std::uint32_t n : iteration_values) {
        // Force memory barrier to ensure clean measurement
        __sync_synchronize();

        const auto start = std::chrono::high_resolution_clock::now();

        // Unroll loop manually for better performance
        std::uint64_t local_sink = 0;
        for (int i = 0; i < iterations; i += 4) {
            local_sink += fib.unsafe_get(n);
            local_sink += fib.unsafe_get(n);
            local_sink += fib.unsafe_get(n);
            local_sink += fib.unsafe_get(n);
        }

        const auto end = std::chrono::high_resolution_clock::now();
        sink += local_sink;

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const double avg_duration = static_cast<double>(duration.count()) / iterations;
        const double ops_per_second = 1e9 / avg_duration;

        std::cout << "F(" << std::setw(2) << n << ") avg: " << std::fixed << std::setprecision(4)
                  << avg_duration << " ns | "
                  << std::setprecision(1) << (ops_per_second / 1e6) << "M ops/sec" << std::endl;
    }

    // Print the sink to force calculation
    std::cout << "\nBenchmark sink: " << sink << std::endl;

    return 0;
}