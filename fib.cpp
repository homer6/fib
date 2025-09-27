#include <iostream>
#include <array>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <string>
#include <sstream>

// Cache line size for optimal alignment (typical x86-64/ARM)
#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

// Branch prediction hints
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Force inline for critical performance paths
#define FORCE_INLINE __attribute__((always_inline)) inline

// Helper to convert unsigned __int128 to string
std::string uint128_to_string(unsigned __int128 value) {
    if (value == 0) return "0";

    std::string result;
    while (value > 0) {
        result = char('0' + value % 10) + result;
        value /= 10;
    }
    return result;
}

// Use unsigned __int128 for extended range
using uint128_t = unsigned __int128;

class alignas(CACHE_LINE_SIZE) Fibonacci {
private:
    // F(93) is the largest that fits in uint64_t, but we'll support up to F(186) with uint128_t
    static constexpr std::uint32_t MAX_N_64 = 93;
    static constexpr std::uint32_t MAX_N_128 = 186;  // F(186) is the largest that fits in unsigned __int128

public:
    static constexpr std::size_t TABLE_SIZE_64 = MAX_N_64 + 1;
    static constexpr std::size_t TABLE_SIZE_128 = MAX_N_128 + 1;

private:
    // The compile-time generated lookup table for uint64_t values
    alignas(CACHE_LINE_SIZE) static constexpr auto LookupTable64 = []() constexpr {
        std::array<std::uint64_t, TABLE_SIZE_64> table = {};

        if (TABLE_SIZE_64 > 0) table[0] = 0;
        if (TABLE_SIZE_64 > 1) table[1] = 1;

        for (std::size_t i = 2; i < TABLE_SIZE_64; ++i) {
            table[i] = table[i - 1] + table[i - 2];
        }
        return table;
    }();

    // The compile-time generated lookup table for uint128_t values
    alignas(CACHE_LINE_SIZE) static constexpr auto LookupTable128 = []() constexpr {
        std::array<uint128_t, TABLE_SIZE_128> table = {};

        if (TABLE_SIZE_128 > 0) table[0] = 0;
        if (TABLE_SIZE_128 > 1) table[1] = 1;

        for (std::size_t i = 2; i < TABLE_SIZE_128; ++i) {
            table[i] = table[i - 1] + table[i - 2];
        }
        return table;
    }();

public:
    // Get Fibonacci number as uint64_t (for backwards compatibility)
    [[nodiscard]] FORCE_INLINE std::uint64_t operator()(std::uint32_t n) const noexcept {
        if (UNLIKELY(n > MAX_N_64)) {
            return 0;
        }
        return LookupTable64[n];
    }

    // Get Fibonacci number as uint128_t for extended range
    [[nodiscard]] FORCE_INLINE uint128_t get128(std::uint32_t n) const noexcept {
        if (UNLIKELY(n > MAX_N_128)) {
            return 0;
        }
        return LookupTable128[n];
    }

    // Get Fibonacci number as string (handles full 128-bit range)
    [[nodiscard]] std::string get_string(std::uint32_t n) const noexcept {
        if (UNLIKELY(n > MAX_N_128)) {
            return "0";
        }
        return uint128_to_string(LookupTable128[n]);
    }

    // Unsafe version for when you absolutely know n is valid (<=93)
    [[nodiscard]] FORCE_INLINE std::uint64_t unsafe_get(std::uint32_t n) const noexcept {
        return LookupTable64[n];
    }

    // Unsafe version for 128-bit range
    [[nodiscard]] FORCE_INLINE uint128_t unsafe_get128(std::uint32_t n) const noexcept {
        return LookupTable128[n];
    }

    // Prefetch multiple values for better cache utilization
    FORCE_INLINE void prefetch_range(std::uint32_t start, std::uint32_t count) const noexcept {
        const std::uint32_t end = std::min(start + count, static_cast<std::uint32_t>(TABLE_SIZE_64));
        for (std::uint32_t i = start; i < end; i += 8) {
            __builtin_prefetch(&LookupTable64[i], 0, 3);
        }
    }
};

// For strict C++14 compatibility (before inline variables were introduced in C++17),
// static constexpr members may require an out-of-class definition if they are ODR-used.
// We use decltype to ensure the type matches the auto-deduced type inside the class.
constexpr decltype(Fibonacci::LookupTable64) Fibonacci::LookupTable64;
constexpr decltype(Fibonacci::LookupTable128) Fibonacci::LookupTable128;


// High-resolution micro benchmarking
int main() {
    // We use this accumulator as a "sink" to ensure the compiler doesn't optimize away the loops.
    std::uint64_t sink = 0;

    Fibonacci fib;

    // Warmup CPU caches and branch predictor
    for (int warmup = 0; warmup < 1000; ++warmup) {
        sink += fib.unsafe_get(warmup % 94);
    }

    std::cout << "Optimized Fibonacci micro-benchmark (Maximum Performance Edition):\n" << std::endl;
    std::cout << "Complete Fibonacci Sequence with Performance Metrics (F(0) to F(93)):\n" << std::endl;
    std::cout << std::string(75, '=') << std::endl;

    // Benchmark ALL values from F(0) to F(93)
    for (std::uint32_t i = 0; i <= 93; ++i) {
        // Use high_resolution_clock for best precision
        const auto start = std::chrono::high_resolution_clock::now();

        // Multiple iterations to get more stable measurements
        constexpr int micro_iterations = 1000;
        std::uint64_t local_sum = 0;
        std::uint64_t single_value = fib.unsafe_get(i);
        for (int j = 0; j < micro_iterations; ++j) {
            local_sum += single_value;
        }

        const auto end = std::chrono::high_resolution_clock::now();
        sink += local_sum;

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const double avg_ns = static_cast<double>(duration.count()) / micro_iterations;

        // Output with value and performance metrics
        std::cout << "F(" << std::setw(2) << i << ") = " << std::setw(20) << single_value
                  << " | " << std::fixed << std::setprecision(4) << avg_ns << " ns"
                  << " | " << std::setprecision(1) << (avg_ns > 0 ? (1000.0 / avg_ns) : 1000000) << "M ops/sec"
                  << std::endl;
    }

    std::cout << std::string(75, '=') << std::endl;

    // Test extended range with 128-bit support
    std::cout << "\nTesting extended range with 128-bit support (F(94) to F(186)):" << std::endl;
    std::cout << std::string(75, '-') << std::endl;

    std::uint32_t extended_values[] = {94, 100, 110, 120, 130, 140, 150, 160, 170, 180, 185, 186, 187, 200, 1000, 10000};
    for (std::uint32_t n : extended_values) {
        const auto start = std::chrono::high_resolution_clock::now();

        constexpr int micro_iterations = 1000;
        std::string value_str = fib.get_string(n);

        // Just benchmark the lookup
        for (int j = 0; j < micro_iterations; ++j) {
            volatile auto temp = fib.get128(n);
            (void)temp;
        }

        const auto end = std::chrono::high_resolution_clock::now();

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const double avg_ns = static_cast<double>(duration.count()) / micro_iterations;

        std::cout << "F(" << std::setw(4) << n << ") = ";
        if (n <= 186) {
            // Truncate very long numbers for display
            if (value_str.length() > 40) {
                std::cout << value_str.substr(0, 20) << "..."
                         << value_str.substr(value_str.length() - 17);
            } else {
                std::cout << std::setw(40) << value_str;
            }
        } else {
            std::cout << std::setw(40) << "0";
        }
        std::cout << " | " << std::fixed << std::setprecision(4) << avg_ns << " ns"
                  << " | " << (n <= 186 ? "VALID" : "OUT OF RANGE")
                  << std::endl;
    }

    // Test some random valid values to show performance
    std::cout << "\nRandom valid values performance test:" << std::endl;
    std::cout << std::string(75, '-') << std::endl;

    std::uint32_t random_values[] = {7, 23, 31, 47, 59, 67, 71, 83, 89, 91};
    for (std::uint32_t n : random_values) {
        const auto start = std::chrono::high_resolution_clock::now();

        constexpr int micro_iterations = 1000;
        std::uint64_t local_sum = 0;
        std::uint64_t single_value = fib.unsafe_get(n);
        for (int j = 0; j < micro_iterations; ++j) {
            local_sum += single_value;
        }

        const auto end = std::chrono::high_resolution_clock::now();
        sink += local_sum;

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const double avg_ns = static_cast<double>(duration.count()) / micro_iterations;

        std::cout << "F(" << std::setw(2) << n << ") = " << std::setw(20) << single_value
                  << " | " << std::fixed << std::setprecision(4) << avg_ns << " ns"
                  << " | " << std::setprecision(1) << (avg_ns > 0 ? (1000.0 / avg_ns) : 1000000) << "M ops/sec"
                  << std::endl;
    }

    std::cout << std::string(75, '=') << std::endl;

    // Additional targeted benchmarks
    std::cout << "\nTargeted benchmarks with prefetching:" << std::endl;
    // Explicit array definition for broad compatibility
    std::uint32_t large_values[] = {20, 40, 60, 80, 93};

    // Prefetch all large values
    fib.prefetch_range(20, 74);

    for (std::uint32_t n : large_values) {
        const auto start = std::chrono::high_resolution_clock::now();

        constexpr int micro_iterations = 100;
        std::uint64_t local_sum = 0;
        std::uint64_t single_value = fib.unsafe_get(n);
        for (int j = 0; j < micro_iterations; ++j) {
            local_sum += single_value;
        }

        const auto end = std::chrono::high_resolution_clock::now();
        sink += local_sum;

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const double avg_ns = static_cast<double>(duration.count()) / micro_iterations;

        std::cout << "F(" << std::setw(2) << n << ") = " << std::setw(20) << single_value
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

    // Force compiler to not optimize away the sink
    __asm__ __volatile__("" : : "r"(sink) : "memory");
    std::cout << "\nBenchmark sink: " << sink << std::endl;

    return 0;
}