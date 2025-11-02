#include "combining_tree.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <set>
#include <chrono>
#include <cmath>

using namespace snake;

// Test class for combining tree with 16 threads
class CombiningTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        tree = std::make_unique<CombiningTree<16>>();
    }

    void TearDown() override {
        tree.reset();
    }

    std::unique_ptr<CombiningTree<16>> tree;
    static constexpr int NUM_THREADS = 16;
};

// Basic test: single thread increments
TEST_F(CombiningTreeTest, SingleThreadBasic) {
    int value = tree->get_and_increment(0);
    EXPECT_EQ(value, 0);
    
    value = tree->get_and_increment(0);
    EXPECT_EQ(value, 1);
    
    value = tree->get_and_increment(0);
    EXPECT_EQ(value, 2);
}

// Test: all 16 threads each get a unique value
TEST_F(CombiningTreeTest, AllThreadsUniqueValues) {
    std::vector<std::thread> threads;
    std::vector<int> results(16);
    std::atomic<int> completion_count{0};
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, &results, &completion_count]() {
            results[t] = tree->get_and_increment(t);
            completion_count++;
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All threads should have completed
    EXPECT_EQ(completion_count.load(), 16);
    
    // All values should be unique and in range [0, 15]
    std::set<int> unique_values(results.begin(), results.end());
    EXPECT_EQ(unique_values.size(), 16) << "All 16 threads should get unique values";
    
    // Verify all values are in expected range
    for (int val : results) {
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 16);
    }
    
    // Verify sequentiality: sum should be 0+1+2+...+15 = 120
    int sum = 0;
    for (int val : results) {
        sum += val;
    }
    EXPECT_EQ(sum, 120) << "Sum of values from 0 to 15 should be 120";
}

// Test: multiple increments per thread
TEST_F(CombiningTreeTest, MultipleIncrementsPerThread) {
    const int increments_per_thread = 100;
    std::vector<std::thread> threads;
    std::vector<std::vector<int>> thread_results(16);
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, increments_per_thread, &thread_results]() {
            for (int i = 0; i < increments_per_thread; ++i) {
                int value = tree->get_and_increment(t);
                thread_results[t].push_back(value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Collect all values
    std::vector<int> all_values;
    for (const auto& results : thread_results) {
        all_values.insert(all_values.end(), results.begin(), results.end());
    }
    
    // Should have exactly 16 * 100 = 1600 values
    EXPECT_EQ(all_values.size(), 16 * increments_per_thread);
    
    // All values should be unique
    std::set<int> unique_values(all_values.begin(), all_values.end());
    EXPECT_EQ(unique_values.size(), 16 * increments_per_thread) 
        << "All values should be unique across all threads and increments";
    
    // Values should be in range [0, 1599]
    for (int val : all_values) {
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 16 * increments_per_thread);
    }
    
    // Verify sequentiality: sum should be 0+1+2+...+1599 = 1599*1600/2 = 1279200
    long long sum = 0;
    for (int val : all_values) {
        sum += val;
    }
    long long expected_sum = (long long)(16 * increments_per_thread - 1) * (16 * increments_per_thread) / 2;
    EXPECT_EQ(sum, expected_sum) << "Sum should match arithmetic progression";
}

// Test: stress test with many concurrent operations
TEST_F(CombiningTreeTest, StressTest) {
    const int total_operations = 10000;
    std::vector<std::thread> threads;
    std::atomic<int> total_increments{0};
    std::vector<std::atomic<int>> thread_operations(16);
    
    for (int i = 0; i < 16; ++i) {
        thread_operations[i].store(0);
    }
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, total_operations, &total_increments, &thread_operations]() {
            int local_count = 0;
            while (local_count < total_operations / 16) {
                tree->get_and_increment(t);
                local_count++;
                total_increments++;
                thread_operations[t]++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all threads completed their operations
    int total_ops = 0;
    for (int i = 0; i < 16; ++i) {
        total_ops += thread_operations[i].load();
    }
    EXPECT_GE(total_ops, total_operations * 0.95) 
        << "At least 95% of operations should complete";
}

// Test: verify no value duplication under high contention
TEST_F(CombiningTreeTest, NoDuplicationHighContention) {
    const int operations_per_thread = 500;
    std::vector<std::thread> threads;
    std::vector<std::vector<int>> thread_values(16);
    std::vector<std::mutex> value_mutexes(16);
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, operations_per_thread, &thread_values, &value_mutexes]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                int value = tree->get_and_increment(t);
                std::lock_guard<std::mutex> lock(value_mutexes[t]);
                thread_values[t].push_back(value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Collect all values
    std::vector<int> all_values;
    for (const auto& values : thread_values) {
        all_values.insert(all_values.end(), values.begin(), values.end());
    }
    
    // Check for duplicates
    std::set<int> unique_values(all_values.begin(), all_values.end());
    EXPECT_EQ(unique_values.size(), all_values.size()) 
        << "No duplicate values should exist";
    
    EXPECT_EQ(all_values.size(), 16 * operations_per_thread)
        << "All operations should complete";
}

// Test: correctness - verify sequential behavior
TEST_F(CombiningTreeTest, SequentialCorrectness) {
    std::vector<std::thread> threads;
    std::vector<int> results(16 * 100);
    std::atomic<int> index{0};
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, &results, &index]() {
            for (int i = 0; i < 100; ++i) {
                int value = tree->get_and_increment(t);
                int idx = index.fetch_add(1);
                results[idx] = value;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Sort results and verify they form a complete sequence
    std::vector<int> sorted_results = results;
    std::sort(sorted_results.begin(), sorted_results.end());
    
    // Should have values 0, 1, 2, ..., 1599
    for (size_t i = 0; i < sorted_results.size(); ++i) {
        EXPECT_EQ(sorted_results[i], static_cast<int>(i)) 
            << "Value at position " << i << " should be " << i;
    }
}

// Test: fairness - all threads get roughly equal number of values
TEST_F(CombiningTreeTest, Fairness) {
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::vector<std::atomic<int>> thread_counts(16);
    std::vector<std::atomic<int>> value_ranges(16); // min value seen by each thread
    
    for (int i = 0; i < 16; ++i) {
        thread_counts[i].store(0);
        value_ranges[i].store(std::numeric_limits<int>::max());
    }
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, operations_per_thread, &thread_counts, &value_ranges]() {
            int min_val = std::numeric_limits<int>::max();
            int max_val = std::numeric_limits<int>::min();
            for (int i = 0; i < operations_per_thread; ++i) {
                int value = tree->get_and_increment(t);
                min_val = std::min(min_val, value);
                max_val = std::max(max_val, value);
                thread_counts[t]++;
            }
            value_ranges[t].store(max_val - min_val);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all threads completed their operations
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(thread_counts[i].load(), operations_per_thread)
            << "Thread " << i << " should complete all operations";
    }
    
    // Check that value ranges are reasonable (each thread should see a spread of values)
    for (int i = 0; i < 16; ++i) {
        int range = value_ranges[i].load();
        EXPECT_GT(range, operations_per_thread / 4)
            << "Thread " << i << " should see values across a reasonable range";
    }
}

// Test: memory consistency - verify operations complete in order
TEST_F(CombiningTreeTest, MemoryConsistency) {
    const int operations = 100;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::pair<int, int>>> thread_ops(16); // (op_index, value)
    std::vector<std::mutex> mutexes(16);
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, operations, &thread_ops, &mutexes]() {
            for (int i = 0; i < operations; ++i) {
                int value = tree->get_and_increment(t);
                std::lock_guard<std::mutex> lock(mutexes[t]);
                thread_ops[t].emplace_back(i, value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // For each thread, values should be increasing (though not necessarily consecutive)
    for (int t = 0; t < 16; ++t) {
        const auto& ops = thread_ops[t];
        for (size_t i = 1; i < ops.size(); ++i) {
            EXPECT_LT(ops[i-1].second, ops[i].second)
                << "Thread " << t << " operation " << i 
                << " should have value greater than previous";
        }
    }
}

// Test: rapid successive calls
TEST_F(CombiningTreeTest, RapidSuccessiveCalls) {
    std::vector<std::thread> threads;
    std::atomic<int> total_calls{0};
    const int calls_per_thread = 200;
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, calls_per_thread, &total_calls]() {
            for (int i = 0; i < calls_per_thread; ++i) {
                tree->get_and_increment(t);
                total_calls++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(total_calls.load(), 16 * calls_per_thread);
}

// Test: verify tree structure and thread ID validation
TEST_F(CombiningTreeTest, ThreadIdValidation) {
    // Valid thread IDs should work
    for (int t = 0; t < 16; ++t) {
        EXPECT_NO_THROW(tree->get_and_increment(t));
    }
    
    // Note: The assert in get_and_increment will catch invalid thread IDs at runtime
    // In release builds, this might not throw, but in debug builds it will abort
}

// Rigorous concurrent test
class CombiningTreeRigorousTest : public ::testing::Test {
protected:
    void SetUp() override {
        tree = std::make_unique<CombiningTree<16>>();
    }

    void TearDown() override {
        tree.reset();
    }

    std::unique_ptr<CombiningTree<16>> tree;
    static constexpr int NUM_THREADS = 16;
};

// Test: very high contention with many operations
TEST_F(CombiningTreeRigorousTest, VeryHighContention) {
    const int operations_per_thread = 2000;
    std::vector<std::thread> threads;
    std::vector<int> all_values;
    std::mutex values_mutex;
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, operations_per_thread, &all_values, &values_mutex]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                int value = tree->get_and_increment(t);
                std::lock_guard<std::mutex> lock(values_mutex);
                all_values.push_back(value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(all_values.size(), 16 * operations_per_thread);
    
    // Verify no duplicates
    std::set<int> unique_values(all_values.begin(), all_values.end());
    EXPECT_EQ(unique_values.size(), all_values.size());
    
    // Verify complete sequence
    std::sort(all_values.begin(), all_values.end());
    for (size_t i = 0; i < all_values.size(); ++i) {
        EXPECT_EQ(all_values[i], static_cast<int>(i));
    }
}

// Test: mixed workload with varying operation counts
TEST_F(CombiningTreeRigorousTest, MixedWorkload) {
    std::vector<std::thread> threads;
    std::vector<int> all_values;
    std::mutex values_mutex;
    std::vector<int> operations_per_thread = {100, 200, 300, 400, 500, 600, 700, 800,
                                               900, 1000, 1100, 1200, 1300, 1400, 1500, 1600};
    
    for (int t = 0; t < 16; ++t) {
        threads.emplace_back([this, t, &operations_per_thread, &all_values, &values_mutex]() {
            for (int i = 0; i < operations_per_thread[t]; ++i) {
                int value = tree->get_and_increment(t);
                std::lock_guard<std::mutex> lock(values_mutex);
                all_values.push_back(value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int expected_total = 0;
    for (int ops : operations_per_thread) {
        expected_total += ops;
    }
    
    EXPECT_EQ(all_values.size(), expected_total);
    
    // Verify no duplicates
    std::set<int> unique_values(all_values.begin(), all_values.end());
    EXPECT_EQ(unique_values.size(), all_values.size());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

