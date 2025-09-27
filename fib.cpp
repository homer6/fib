#include <iostream>
#include <array>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <algorithm>

class Fibonacci {
private:
    // F(93) is the largest Fibonacci number that fits in a std::uint64_t (12200160415121876738).
    static constexpr std::uint32_t MAX_N = 93;
    
public:
    static constexpr std::size_t TABLE_SIZE = MAX_N + 1;

private:
    // The compile-time generated lookup table. 
    // We use an Immediately Invoked Lambda Expression (IILE) to generate the table.
    static constexpr auto LookupTable = []() constexpr {
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
    // The optimized lookup function: O(1) complexity.
    [[nodiscard]] inline std::uint64_t operator()(std::uint32_t n) const noexcept {
        // Bounds check for safety.
        if (n > MAX_N) {
            // Handle requests outside the 64-bit range.
            return 0; 
        }
        
        // Direct lookup. This compiles down to a single memory read instruction.
        return LookupTable[n];
    }
};

// For strict C++14 compatibility (before inline variables were introduced in C++17), 
// static constexpr members may require an out-of-class definition if they are ODR-used.
// We use decltype to ensure the type matches the auto-deduced type inside the class.
constexpr decltype(Fibonacci::LookupTable) Fibonacci::LookupTable;


// High-resolution micro benchmarking
int main() {
    // We use this accumulator as a "sink" to ensure the compiler doesn't optimize away the loops.
    // If the results are unused, the compiler might eliminate the calls entirely.
    std::uint64_t sink = 0;
    
    Fibonacci fib;
    
    // Warmup CPU caches.
    sink += fib(50);
    
    std::cout << "Optimized Fibonacci micro-benchmark (Compile-Time LUT via IILE):\n" << std::endl;
    
    // Benchmark individual calculations
    // Note: At this speed, we are often measuring the clock overhead itself.
    for (std::uint32_t i = 0; i <= 10; ++i) {
        const auto start = std::chrono::high_resolution_clock::now();
        const auto result = fib(i);
        const auto end = std::chrono::high_resolution_clock::now();
        sink += result;
        
        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        // Increased width to 20 to accommodate F(93)
        std::cout << "F(" << std::setw(2) << i << ") = " << std::setw(20) << result 
                  << " [" << std::setw(4) << duration.count() << " ns]" << std::endl;
    }
    
    // Benchmark larger values (extended up to the limit)
    std::cout << "\nLarger values:" << std::endl;
    // Explicit array definition for broad compatibility
    std::uint32_t large_values[] = {20, 40, 60, 80, 93};
    for (std::uint32_t n : large_values) {
        const auto start = std::chrono::high_resolution_clock::now();
        const auto result = fib(n);
        const auto end = std::chrono::high_resolution_clock::now();
        sink += result;
        
        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        std::cout << "F(" << std::setw(2) << n << ") = " << std::setw(20) << result 
                  << " [" << std::setw(4) << duration.count() << " ns]" << std::endl;
    }
    
    // Benchmark multiple iterations for statistical accuracy (Throughput measurement)
    std::cout << "\nAverage performance over 1000000 iterations:" << std::endl;
    constexpr int iterations = 1000000;
    
    std::uint32_t iteration_values[] = {10, 50, 93};
    for (std::uint32_t n : iteration_values) {
        const auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            // Accumulate into the sink.
            sink += fib(n);
        }
        const auto end = std::chrono::high_resolution_clock::now();
        
        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        // Use double to measure average throughput, which may be sub-nanosecond.
        const double avg_duration = static_cast<double>(duration.count()) / iterations;
        
        std::cout << "F(" << std::setw(2) << n << ") avg: " << std::fixed << std::setprecision(3) 
                  << avg_duration << " ns over " << iterations << " calls" << std::endl;
    }
    
    // Print the sink. This forces the compiler to calculate the final value of 'sink', 
    // thereby ensuring the execution of the benchmark loops.
    std::cout << "\nBenchmark sink (prevents optimization): " << sink << std::endl;
    
    return 0;
}