// Copyright 2025
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <numeric>
#include <algorithm>
#include <cmath>

using namespace boost::decimal;

// Benchmark configuration
constexpr size_t DATA_SIZE = 100000;        // Number of test values
constexpr int WARMUP_ROUNDS = 3;            // Warmup iterations
constexpr int BENCHMARK_ROUNDS = 10;        // Actual benchmark iterations

// Statistics helper
template<typename T>
struct Statistics {
    T min;
    T max;
    T mean;
    T median;
    T stddev;
};

template<typename T>
Statistics<T> calculate_statistics(std::vector<T>& data) {
    Statistics<T> stats;
    
    std::sort(data.begin(), data.end());
    
    stats.min = data.front();
    stats.max = data.back();
    stats.median = data[data.size() / 2];
    
    T sum = std::accumulate(data.begin(), data.end(), T(0));
    stats.mean = sum / data.size();
    
    T sq_sum = std::accumulate(data.begin(), data.end(), T(0),
        [stats](T acc, T val) { return acc + (val - stats.mean) * (val - stats.mean); });
    stats.stddev = std::sqrt(sq_sum / data.size());
    
    return stats;
}

// Benchmark runner
template<typename DecimalType>
class SqrtBenchmark {
private:
    std::vector<DecimalType> test_data;
    std::vector<DecimalType> results;
    std::string type_name;
    
public:
    SqrtBenchmark(const std::string& name) : type_name(name) {
        test_data.reserve(DATA_SIZE);
        results.reserve(DATA_SIZE);
    }
    
    // Prepare test data
    void prepare_data() {
        std::cout << "Preparing test data for " << type_name << "..." << std::flush;
        
        std::random_device rd;
        std::mt19937_64 gen(42); // Fixed seed for reproducibility
        
        // Generate diverse test cases
        // 1. Small values (0.001 to 1)
        std::uniform_real_distribution<double> dist_small(0.001, 1.0);
        for (size_t i = 0; i < DATA_SIZE / 3; ++i) {
            test_data.push_back(DecimalType(dist_small(gen)));
        }
        
        // 2. Medium values (1 to 1000)
        std::uniform_real_distribution<double> dist_medium(1.0, 1000.0);
        for (size_t i = 0; i < DATA_SIZE / 3; ++i) {
            test_data.push_back(DecimalType(dist_medium(gen)));
        }
        
        // 3. Large values (1000 to 1000000)
        std::uniform_real_distribution<double> dist_large(1000.0, 1000000.0);
        for (size_t i = 0; i < DATA_SIZE / 3; ++i) {
            test_data.push_back(DecimalType(dist_large(gen)));
        }
        
        std::cout << " Done (" << test_data.size() << " values)" << std::endl;
    }
    
    // Warmup phase
    void warmup() {
        std::cout << "Warming up";
        for (int round = 0; round < WARMUP_ROUNDS; ++round) {
            results.clear();
            for (const auto& val : test_data) {
                results.push_back(sqrt(val));
            }
            std::cout << "." << std::flush;
        }
        std::cout << " Done" << std::endl;
    }
    
    // Run benchmark
    void run() {
        std::cout << "Running benchmark for " << type_name << "..." << std::endl;
        
        std::vector<double> timings;
        timings.reserve(BENCHMARK_ROUNDS);
        
        for (int round = 0; round < BENCHMARK_ROUNDS; ++round) {
            results.clear();
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (const auto& val : test_data) {
                results.push_back(sqrt(val));
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::milli> duration = end - start;
            timings.push_back(duration.count());
            
            std::cout << "  Round " << std::setw(2) << (round + 1) 
                      << ": " << std::fixed << std::setprecision(3) 
                      << duration.count() << " ms" << std::endl;
        }
        
        // Calculate and display statistics
        auto stats = calculate_statistics(timings);
        
        std::cout << "\n" << type_name << " Results:" << std::endl;
        std::cout << "  " << std::string(50, '-') << std::endl;
        std::cout << "  Data size:       " << DATA_SIZE << " values" << std::endl;
        std::cout << "  Warmup rounds:   " << WARMUP_ROUNDS << std::endl;
        std::cout << "  Benchmark rounds:" << BENCHMARK_ROUNDS << std::endl;
        std::cout << "  " << std::string(50, '-') << std::endl;
        std::cout << "  Min time:        " << std::fixed << std::setprecision(3) << stats.min << " ms" << std::endl;
        std::cout << "  Max time:        " << std::fixed << std::setprecision(3) << stats.max << " ms" << std::endl;
        std::cout << "  Mean time:       " << std::fixed << std::setprecision(3) << stats.mean << " ms" << std::endl;
        std::cout << "  Median time:     " << std::fixed << std::setprecision(3) << stats.median << " ms" << std::endl;
        std::cout << "  Std deviation:   " << std::fixed << std::setprecision(3) << stats.stddev << " ms" << std::endl;
        std::cout << "  " << std::string(50, '-') << std::endl;
        std::cout << "  Throughput:      " << std::fixed << std::setprecision(0) 
                  << (DATA_SIZE / (stats.mean / 1000.0)) << " ops/sec" << std::endl;
        std::cout << "  Avg time/op:     " << std::fixed << std::setprecision(3) 
                  << (stats.mean * 1000.0 / DATA_SIZE) << " µs" << std::endl;
        std::cout << std::endl;
    }
    
    // Full benchmark pipeline
    void execute() {
        prepare_data();
        warmup();
        run();
    }
};

// Compare with standard library double
class StdSqrtBenchmark {
private:
    std::vector<double> test_data;
    std::vector<double> results;
    
public:
    void prepare_data() {
        std::cout << "Preparing test data for std::sqrt (double)..." << std::flush;
        
        std::mt19937_64 gen(42);
        test_data.reserve(DATA_SIZE);
        results.reserve(DATA_SIZE);
        
        std::uniform_real_distribution<double> dist_small(0.001, 1.0);
        for (size_t i = 0; i < DATA_SIZE / 3; ++i) {
            test_data.push_back(dist_small(gen));
        }
        
        std::uniform_real_distribution<double> dist_medium(1.0, 1000.0);
        for (size_t i = 0; i < DATA_SIZE / 3; ++i) {
            test_data.push_back(dist_medium(gen));
        }
        
        std::uniform_real_distribution<double> dist_large(1000.0, 1000000.0);
        for (size_t i = 0; i < DATA_SIZE / 3; ++i) {
            test_data.push_back(dist_large(gen));
        }
        
        std::cout << " Done (" << test_data.size() << " values)" << std::endl;
    }
    
    void warmup() {
        std::cout << "Warming up";
        for (int round = 0; round < WARMUP_ROUNDS; ++round) {
            results.clear();
            for (const auto& val : test_data) {
                results.push_back(std::sqrt(val));
            }
            std::cout << "." << std::flush;
        }
        std::cout << " Done" << std::endl;
    }
    
    void run() {
        std::cout << "Running benchmark for std::sqrt (double)..." << std::endl;
        
        std::vector<double> timings;
        timings.reserve(BENCHMARK_ROUNDS);
        
        for (int round = 0; round < BENCHMARK_ROUNDS; ++round) {
            results.clear();
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (const auto& val : test_data) {
                results.push_back(std::sqrt(val));
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::milli> duration = end - start;
            timings.push_back(duration.count());
            
            std::cout << "  Round " << std::setw(2) << (round + 1) 
                      << ": " << std::fixed << std::setprecision(3) 
                      << duration.count() << " ms" << std::endl;
        }
        
        auto stats = calculate_statistics(timings);
        
        std::cout << "\nstd::sqrt (double) Results:" << std::endl;
        std::cout << "  " << std::string(50, '-') << std::endl;
        std::cout << "  Data size:       " << DATA_SIZE << " values" << std::endl;
        std::cout << "  Warmup rounds:   " << WARMUP_ROUNDS << std::endl;
        std::cout << "  Benchmark rounds:" << BENCHMARK_ROUNDS << std::endl;
        std::cout << "  " << std::string(50, '-') << std::endl;
        std::cout << "  Min time:        " << std::fixed << std::setprecision(3) << stats.min << " ms" << std::endl;
        std::cout << "  Max time:        " << std::fixed << std::setprecision(3) << stats.max << " ms" << std::endl;
        std::cout << "  Mean time:       " << std::fixed << std::setprecision(3) << stats.mean << " ms" << std::endl;
        std::cout << "  Median time:     " << std::fixed << std::setprecision(3) << stats.median << " ms" << std::endl;
        std::cout << "  Std deviation:   " << std::fixed << std::setprecision(3) << stats.stddev << " ms" << std::endl;
        std::cout << "  " << std::string(50, '-') << std::endl;
        std::cout << "  Throughput:      " << std::fixed << std::setprecision(0) 
                  << (DATA_SIZE / (stats.mean / 1000.0)) << " ops/sec" << std::endl;
        std::cout << "  Avg time/op:     " << std::fixed << std::setprecision(3) 
                  << (stats.mean * 1000.0 / DATA_SIZE) << " µs" << std::endl;
        std::cout << std::endl;
    }
    
    void execute() {
        prepare_data();
        warmup();
        run();
    }
};

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "      Boost.Decimal sqrt Benchmark       " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;
    
    // Benchmark decimal32_t
    {
        SqrtBenchmark<decimal32_t> bench("decimal32_t");
        bench.execute();
    }
    
    // Benchmark decimal64_t
    {
        SqrtBenchmark<decimal64_t> bench("decimal64_t");
        bench.execute();
    }
    
    // Benchmark decimal128_t
    {
        SqrtBenchmark<decimal128_t> bench("decimal128_t");
        bench.execute();
    }
    
    // Benchmark decimal_fast32_t
    {
        SqrtBenchmark<decimal_fast32_t> bench("decimal_fast32_t");
        bench.execute();
    }
    
    // Benchmark decimal_fast64_t
    {
        SqrtBenchmark<decimal_fast64_t> bench("decimal_fast64_t");
        bench.execute();
    }
    
    // Benchmark decimal_fast128_t
    {
        SqrtBenchmark<decimal_fast128_t> bench("decimal_fast128_t");
        bench.execute();
    }
    
    // Benchmark standard library double for comparison
    {
        StdSqrtBenchmark bench;
        bench.execute();
    }
    
    std::cout << "==========================================" << std::endl;
    std::cout << "         Benchmark Complete!              " << std::endl;
    std::cout << "==========================================" << std::endl;
    
    return 0;
}
