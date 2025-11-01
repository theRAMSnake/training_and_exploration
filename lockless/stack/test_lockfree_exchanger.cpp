#include "lockfree_exchanger.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <mutex>
#include <set>

using namespace snake;

class LockFreeExchangerTest : public ::testing::Test {
protected:
    void SetUp() override {
        exchanger = std::make_unique<LockFreeExchanger<int>>();
    }

    void TearDown() override {
        exchanger.reset();
    }

    std::unique_ptr<LockFreeExchanger<int>> exchanger;
    static constexpr auto TIMEOUT = std::chrono::milliseconds(100);
};

// Test basic exchange functionality - single exchange
TEST_F(LockFreeExchangerTest, SingleExchange) {
    std::optional<int> result1, result2;
    std::atomic<bool> threads_ready{false};
    
    std::thread t1([this, &result1, &threads_ready]() {
        threads_ready.wait(false);
        result1 = exchanger->exchange(10, TIMEOUT);
    });
    
    std::thread t2([this, &result2, &threads_ready]() {
        threads_ready.wait(false);
        result2 = exchanger->exchange(20, TIMEOUT);
    });
    
    // Start both threads simultaneously
    threads_ready.store(true);
    threads_ready.notify_all();
    
    t1.join();
    t2.join();
    
    // Both should succeed - no timeout
    EXPECT_TRUE(result1.has_value()) << "Thread 1 should not timeout";
    EXPECT_TRUE(result2.has_value()) << "Thread 2 should not timeout";
    
    // Values should be swapped
    ASSERT_TRUE(result1.has_value() && result2.has_value());
    EXPECT_TRUE((result1.value() == 10 && result2.value() == 20) ||
                (result1.value() == 20 && result2.value() == 10))
        << "Values not exchanged correctly: got " << result1.value() << " and " << result2.value();
}

// Test exchange with multiple pairs
TEST_F(LockFreeExchangerTest, MultipleExchanges) {
    const int num_exchanges = 10;
    std::vector<std::thread> threads;
    std::vector<std::optional<int>> results(num_exchanges * 2);
    std::atomic<int> thread_counter{0};
    
    // Create pairs of threads that exchange
    for (int i = 0; i < num_exchanges; ++i) {
        int thread_id = thread_counter.fetch_add(1);
        threads.emplace_back([this, i, thread_id, &results, &thread_counter]() {
            results[thread_id] = exchanger->exchange(i * 10, TIMEOUT);
        });
        
        thread_id = thread_counter.fetch_add(1);
        threads.emplace_back([this, i, thread_id, &results, &thread_counter]() {
            results[thread_id] = exchanger->exchange(i * 10 + 1, TIMEOUT);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify that exchanges happened (any two threads can pair, not just the ones created together)
    int successful_exchanges = 0;
    int timeouts = 0;
    for (const auto& result : results) {
        if (result.has_value()) {
            successful_exchanges++;
        } else {
            timeouts++;
        }
    }
    
    // With sufficient timeout and proper pairing, all exchanges should succeed
    EXPECT_EQ(timeouts, 0) << "No exchanges should timeout";
    EXPECT_EQ(successful_exchanges, num_exchanges * 2) << "All exchanges should succeed";
}

// Test timeout behavior
TEST_F(LockFreeExchangerTest, TimeoutWhenNoPartner) {
    auto short_timeout = std::chrono::milliseconds(10);
    auto result = exchanger->exchange(42, short_timeout);
    
    // Should timeout and return empty optional
    EXPECT_FALSE(result.has_value());
}

// Test timeout with multiple threads
TEST_F(LockFreeExchangerTest, TimeoutWithMultipleThreads) {
    const int num_threads = 5;
    std::vector<std::thread> threads;
    std::vector<std::optional<int>> results(num_threads);
    auto short_timeout = std::chrono::milliseconds(10);
    
    // Create threads - exchanges can happen sequentially (pairs), 
    // but with short timeout, some threads may timeout before finding a partner
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &results, short_timeout]() {
            results[i] = exchanger->exchange(i, short_timeout);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Count successes and timeouts
    int successes = 0;
    int timeouts = 0;
    for (const auto& result : results) {
        if (result.has_value()) {
            successes++;
        } else {
            timeouts++;
        }
    }
    
    // With 5 threads, we can have multiple sequential exchanges (pairs)
    // But with very short timeout, some threads may timeout
    // Exchanges happen in pairs, so successes must be even
    EXPECT_EQ(successes % 2, 0) << "Successes must be even (exchanges happen in pairs)";
    EXPECT_GT(timeouts, 0) << "At least one thread should timeout with short timeout and odd number of threads";
    EXPECT_LE(successes, num_threads) << "Can't have more successes than threads";
    EXPECT_LE(successes, 4) << "With 5 threads and short timeout, at most 4 can succeed (2 pairs)";
}

// Test that exchanged values are correct
TEST_F(LockFreeExchangerTest, CorrectExchangeValues) {
    std::optional<int> result1, result2;
    
    std::thread t1([this, &result1]() {
        result1 = exchanger->exchange(100, TIMEOUT);
    });
    
    std::thread t2([this, &result2]() {
        result2 = exchanger->exchange(200, TIMEOUT);
    });
    
    t1.join();
    t2.join();
    
    // Both should succeed - no timeout
    EXPECT_TRUE(result1.has_value()) << "Thread 1 should not timeout";
    EXPECT_TRUE(result2.has_value()) << "Thread 2 should not timeout";
    
    // Values should be swapped
    ASSERT_TRUE(result1.has_value() && result2.has_value());
    EXPECT_TRUE((result1.value() == 100 && result2.value() == 200) ||
                (result1.value() == 200 && result2.value() == 100));
}

// Test with negative numbers
TEST_F(LockFreeExchangerTest, NegativeNumbers) {
    std::optional<int> result1, result2;
    
    std::thread t1([this, &result1]() {
        result1 = exchanger->exchange(-10, TIMEOUT);
    });
    
    std::thread t2([this, &result2]() {
        result2 = exchanger->exchange(-20, TIMEOUT);
    });
    
    t1.join();
    t2.join();
    
    // Both should succeed - no timeout
    EXPECT_TRUE(result1.has_value()) << "Thread 1 should not timeout";
    EXPECT_TRUE(result2.has_value()) << "Thread 2 should not timeout";
    
    // Values should be swapped
    ASSERT_TRUE(result1.has_value() && result2.has_value());
    EXPECT_TRUE((result1.value() == -10 && result2.value() == -20) ||
                (result1.value() == -20 && result2.value() == -10));
}

// Concurrent test class
class LockFreeExchangerConcurrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        exchanger = std::make_unique<LockFreeExchanger<int>>();
    }

    void TearDown() override {
        exchanger.reset();
    }

    std::unique_ptr<LockFreeExchanger<int>> exchanger;
    static constexpr int NUM_THREADS = 4;
    static constexpr int OPERATIONS_PER_THREAD = 100;
    static constexpr auto TIMEOUT = std::chrono::milliseconds(1000);
};

// Test concurrent exchanges with many threads
TEST_F(LockFreeExchangerConcurrentTest, ConcurrentExchanges) {
    std::vector<std::thread> threads;
    std::vector<std::atomic<int>> successful_exchanges(NUM_THREADS);
    std::vector<std::atomic<int>> received_values(NUM_THREADS);
    std::atomic<int> total_timeout_attempts{0};
    std::atomic<int> total_remaining_operations{NUM_THREADS * OPERATIONS_PER_THREAD};
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        successful_exchanges[i].store(0);
        received_values[i].store(0);
    }
    
    // CRITICAL: All threads must stay alive until ALL operations are complete
    // This prevents the scenario where 3 threads finish quickly and exit, leaving the 4th stranded
    // Each thread continues trying until total_remaining_operations reaches zero
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &successful_exchanges, &received_values, &total_timeout_attempts, &total_remaining_operations]() {
            // Keep trying until ALL work across ALL threads is complete
            // CRITICAL: Even if this thread is done, keep participating in exchanges to help others
            while (total_remaining_operations.load() > 0) {
                bool has_own_work = (successful_exchanges[t].load() < OPERATIONS_PER_THREAD);
                if (has_own_work) {
                    // This thread has its own work - try to exchange
                    int value_to_send = t * 10000 + successful_exchanges[t].load();
                    auto result = exchanger->exchange(value_to_send, TIMEOUT);
                    if (result.has_value()) {
                        successful_exchanges[t]++;
                        received_values[t] += result.value();
                        total_remaining_operations--;
                    } else {
                        // Timeout - retry with fixed backoff to reduce contention
                        total_timeout_attempts++;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                } else {
                    // This thread is done with its own work, but must stay alive and help others
                    // Try to exchange with a dummy value - this allows other threads to find a partner
                    // The other thread will handle decrementing total_remaining_operations when it counts its success
                    auto result = exchanger->exchange(t * 10000 + OPERATIONS_PER_THREAD, TIMEOUT);
                    if (result.has_value()) {
                        // Successfully helped another thread - the other thread counted this exchange
                        // Just yield and continue helping
                        std::this_thread::yield();
                    } else {
                        // Timeout while helping - yield briefly and try again
                        std::this_thread::yield();
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all exchanges succeeded - no timeouts expected with sufficient timeout
    int total_successes = 0;
    for (int i = 0; i < NUM_THREADS; ++i) {
        total_successes += successful_exchanges[i].load();
    }
    
    // With sufficient timeout and threads staying alive, all exchanges should succeed
    const int expected_exchanges = NUM_THREADS * OPERATIONS_PER_THREAD;
    EXPECT_EQ(total_successes, expected_exchanges) << "All exchanges should succeed, no timeouts";
    EXPECT_EQ(total_remaining_operations.load(), 0) << "All operations should be completed";
    
    // Verify each thread completed its target
    for (int i = 0; i < NUM_THREADS; ++i) {
        EXPECT_EQ(successful_exchanges[i].load(), OPERATIONS_PER_THREAD) 
            << "Thread " << i << " should complete all " << OPERATIONS_PER_THREAD << " exchanges";
    }
    
    // With sufficient timeout and backoff, timeouts should be minimal
    // Allow a small number of timeouts due to contention, but all work should complete
    EXPECT_LE(total_timeout_attempts.load(), expected_exchanges / 10) 
        << "Timeouts should be minimal with sufficient timeout and backoff";
}

// Test that exchanges are properly matched (paired)
TEST_F(LockFreeExchangerConcurrentTest, ExchangeMatching) {
    const int num_pairs = 50;
    std::vector<std::thread> threads;
    std::vector<std::optional<int>> results(num_pairs * 2);
    std::atomic<int> thread_idx{0};
    
    // Create pairs that should exchange
    for (int i = 0; i < num_pairs; ++i) {
        int idx1 = thread_idx.fetch_add(1);
        threads.emplace_back([this, i, idx1, &results]() {
            results[idx1] = exchanger->exchange(i * 100, TIMEOUT);
        });
        
        int idx2 = thread_idx.fetch_add(1);
        threads.emplace_back([this, i, idx2, &results]() {
            results[idx2] = exchanger->exchange(i * 100 + 50, TIMEOUT);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify that exchanges happened
    // Note: With a shared exchanger, any two threads can pair, not necessarily the ones created together
    int successful_exchanges = 0;
    int timeouts = 0;
    std::set<int> all_values_sent;
    std::set<int> all_values_received;
    
    // Track all sent values
    for (int i = 0; i < num_pairs; ++i) {
        all_values_sent.insert(i * 100);
        all_values_sent.insert(i * 100 + 50);
    }
    
    // Track all received values
    for (const auto& result : results) {
        if (result.has_value()) {
            successful_exchanges++;
            all_values_received.insert(result.value());
        } else {
            timeouts++;
        }
    }
    
    // All exchanges should succeed - no timeouts
    EXPECT_EQ(timeouts, 0) << "No exchanges should timeout";
    EXPECT_EQ(successful_exchanges, num_pairs * 2) << "All exchanges should succeed";
    
    // Verify received values are from the set of sent values
    for (int received : all_values_received) {
        EXPECT_NE(all_values_sent.find(received), all_values_sent.end()) 
            << "Received value " << received << " that was never sent";
    }
}

// Test stress with high contention
TEST_F(LockFreeExchangerConcurrentTest, HighContentionStressTest) {
    const int num_threads = 16;
    const int operations = 200;
    std::vector<std::thread> threads;
    std::atomic<int> total_successes{0};
    std::atomic<int> total_timeouts{0};
    std::vector<std::atomic<int>> successful_exchanges(num_threads);
    std::atomic<int> total_remaining_operations{num_threads * operations};
    
    for (int i = 0; i < num_threads; ++i) {
        successful_exchanges[i].store(0);
    }
    
    // CRITICAL: All threads must stay alive until ALL operations are complete
    // This prevents the scenario where some threads finish quickly and exit, leaving others stranded
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, operations, &total_successes, &total_timeouts, &successful_exchanges, &total_remaining_operations]() {
            // Keep trying until ALL work across ALL threads is complete
            // CRITICAL: Even if this thread is done, keep participating in exchanges to help others
            while (total_remaining_operations.load() > 0) {
                bool has_own_work = (successful_exchanges[t].load() < operations);
                if (has_own_work) {
                    // This thread has its own work - try to exchange
                    int value_to_send = t * 100000 + successful_exchanges[t].load();
                    auto result = exchanger->exchange(value_to_send, TIMEOUT);
                    if (result.has_value()) {
                        successful_exchanges[t]++;
                        total_successes++;
                        total_remaining_operations--;
                    } else {
                        total_timeouts++;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                } else {
                    // This thread is done with its own work, but must stay alive and help others
                    // Try to exchange with a dummy value - this allows other threads to find a partner
                    // The other thread will handle decrementing total_remaining_operations when it counts its success
                    auto result = exchanger->exchange(t * 100000 + operations, TIMEOUT);
                    if (result.has_value()) {
                        // Successfully helped another thread - the other thread counted this exchange
                        // Just yield and continue helping
                        std::this_thread::yield();
                    } else {
                        // Timeout while helping - yield briefly and try again
                        std::this_thread::yield();
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All exchanges should succeed - no timeouts expected with sufficient timeout
    const int expected_operations = num_threads * operations;
    EXPECT_EQ(total_timeouts.load(), 0) << "No exchanges should timeout with sufficient timeout";
    EXPECT_EQ(total_successes.load(), expected_operations) << "All exchanges should succeed";
    EXPECT_EQ(total_remaining_operations.load(), 0) << "All operations should be completed";
}

// Test with different timeout values
TEST_F(LockFreeExchangerConcurrentTest, DifferentTimeoutValues) {
    std::vector<std::thread> threads;
    std::atomic<int> successes_short{0};
    std::atomic<int> successes_long{0};
    
    // Threads with short timeout
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, &successes_short]() {
            auto result = exchanger->exchange(i, std::chrono::milliseconds(5));
            if (result.has_value()) {
                successes_short++;
            }
        });
    }
    
    // Threads with long timeout
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, &successes_long]() {
            auto result = exchanger->exchange(i + 100, std::chrono::milliseconds(200));
            if (result.has_value()) {
                successes_long++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Threads with long timeout should all succeed
    EXPECT_EQ(successes_long.load(), 4) << "All threads with long timeout should succeed";
    // Threads with short timeout might timeout - that's expected behavior being tested
}

// Test that exchanger can be reused after timeout
TEST_F(LockFreeExchangerConcurrentTest, ReuseAfterTimeout) {
    // First, let a thread timeout
    std::atomic<bool> first_timeout_done{false};
    std::thread t1([this, &first_timeout_done]() {
        auto result = exchanger->exchange(1, std::chrono::milliseconds(10));
        EXPECT_FALSE(result.has_value()); // Should timeout
        first_timeout_done.store(true);
    });
    
    t1.join();
    first_timeout_done.wait(false);
    
    // Now try a real exchange
    std::optional<int> result1, result2;
    std::thread t2([this, &result1]() {
        result1 = exchanger->exchange(10, TIMEOUT);
    });
    
    std::thread t3([this, &result2]() {
        result2 = exchanger->exchange(20, TIMEOUT);
    });
    
    t2.join();
    t3.join();
    
    // Exchange should succeed after timeout - no timeout here
    EXPECT_TRUE(result1.has_value()) << "Thread 1 should not timeout";
    EXPECT_TRUE(result2.has_value()) << "Thread 2 should not timeout";
    
    // Values should be swapped
    ASSERT_TRUE(result1.has_value() && result2.has_value());
    EXPECT_TRUE((result1.value() == 10 && result2.value() == 20) ||
                (result1.value() == 20 && result2.value() == 10));
}

// More rigorous concurrent tests
class LockFreeExchangerRigorousTest : public ::testing::Test {
protected:
    void SetUp() override {
        exchanger = std::make_unique<LockFreeExchanger<int>>();
    }

    void TearDown() override {
        exchanger.reset();
    }

    std::unique_ptr<LockFreeExchanger<int>> exchanger;
    static constexpr int NUM_THREADS = 16;
    static constexpr auto TIMEOUT = std::chrono::milliseconds(100);
};

// Test memory consistency under high load
TEST_F(LockFreeExchangerRigorousTest, MemoryConsistencyTest) {
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> total_operations{0};
    std::atomic<int> successful_exchanges{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &stop_flag, &total_operations, &successful_exchanges]() {
            int counter = 0;
            while (!stop_flag.load()) {
                auto result = exchanger->exchange(t * 1000000 + counter, TIMEOUT);
                total_operations++;
                if (result.has_value()) {
                    successful_exchanges++;
                }
                counter++;
            }
        });
    }
    
    // Run for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop_flag.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(total_operations.load(), 0);
    // Most exchanges should succeed - timeouts should be rare with sufficient timeout
    // Note: Some threads might be interrupted by stop_flag mid-exchange, so we allow small number of incomplete exchanges
    const int total_ops = total_operations.load();
    const int successes = successful_exchanges.load();
    // At least 90% of operations should succeed (allowing for stop_flag interruption)
    EXPECT_GT(successes, total_ops * 0.9) << "Most exchanges should succeed, very few timeouts expected";
}

// Test logical correctness - values are properly exchanged
TEST_F(LockFreeExchangerRigorousTest, LogicalCorrectnessTest) {
    const int NUM_PAIRS = 100;
    std::vector<std::thread> threads;
    std::vector<std::optional<int>> results(NUM_PAIRS * 2);
    std::atomic<int> thread_idx{0};
    
    // Create pairs with known values
    for (int p = 0; p < NUM_PAIRS; ++p) {
        int idx1 = thread_idx.fetch_add(1);
        threads.emplace_back([this, p, idx1, &results]() {
            results[idx1] = exchanger->exchange(p * 2, TIMEOUT);
        });
        
        int idx2 = thread_idx.fetch_add(1);
        threads.emplace_back([this, p, idx2, &results]() {
            results[idx2] = exchanger->exchange(p * 2 + 1, TIMEOUT);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify exchanges happened correctly
    // Note: With a shared exchanger, any two threads can pair, not necessarily the ones created together
    int successful_exchanges = 0;
    int timeouts = 0;
    std::set<int> all_values_sent;
    std::set<int> all_values_received;
    
    // Track all sent values
    for (int p = 0; p < NUM_PAIRS; ++p) {
        all_values_sent.insert(p * 2);
        all_values_sent.insert(p * 2 + 1);
    }
    
    // Track all received values
    for (const auto& result : results) {
        if (result.has_value()) {
            successful_exchanges++;
            all_values_received.insert(result.value());
        } else {
            timeouts++;
        }
    }
    
    // All exchanges should succeed - no timeouts
    EXPECT_EQ(timeouts, 0) << "No exchanges should timeout";
    EXPECT_EQ(successful_exchanges, NUM_PAIRS * 2) << "All exchanges should succeed";
    
    // Verify received values are from the set of sent values
    for (int received : all_values_received) {
        EXPECT_NE(all_values_sent.find(received), all_values_sent.end()) 
            << "Received value " << received << " that was never sent";
    }
}

// Test with very short timeouts
TEST_F(LockFreeExchangerRigorousTest, VeryShortTimeouts) {
    // Use an odd number of threads to guarantee at least one timeout
    // With even threads, all can pair; with odd, one must timeout
    const int num_threads = 21;
    std::vector<std::thread> threads;
    std::atomic<int> timeouts{0};
    std::atomic<int> successes{0};
    
    auto very_short_timeout = std::chrono::microseconds(100);
    
    // Start threads with small staggered delays to increase chance of timeouts
    // Early threads may timeout before later threads arrive
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, very_short_timeout, &timeouts, &successes]() {
            // Small delay based on thread ID to stagger start times
            std::this_thread::sleep_for(std::chrono::microseconds(t * 5));
            auto result = exchanger->exchange(t, very_short_timeout);
            if (result.has_value()) {
                successes++;
            } else {
                timeouts++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all threads completed (succeeded or timed out)
    EXPECT_EQ(successes.load() + timeouts.load(), num_threads) 
        << "All threads should either succeed or timeout";
    
    // With odd number of threads, at least one must timeout
    // (exchanges happen in pairs, so odd number leaves one without a partner)
    // Additionally, with very short timeout and staggered starts, early threads
    // will timeout before later threads arrive, increasing timeout count
    EXPECT_GE(timeouts.load(), 1) << "With odd number of threads and very short timeout, at least one should timeout";
    
    // Successes must be even (exchanges happen in pairs)
    EXPECT_EQ(successes.load() % 2, 0) << "Successes must be even (exchanges are paired)";
}

// Test fairness - all threads get a chance
TEST_F(LockFreeExchangerRigorousTest, FairnessTest) {
    const int num_threads = 8;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    std::vector<std::atomic<int>> successes_per_thread(num_threads);
    std::atomic<int> total_remaining_operations{num_threads * operations_per_thread};
    
    for (int i = 0; i < num_threads; ++i) {
        successes_per_thread[i].store(0);
    }
    
    // CRITICAL: All threads must stay alive until ALL operations are complete
    // This prevents the scenario where some threads finish quickly and exit, leaving others stranded
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, operations_per_thread, &successes_per_thread, &total_remaining_operations]() {
            // Keep trying until ALL work across ALL threads is complete
            // CRITICAL: Even if this thread is done, keep participating in exchanges to help others
            while (total_remaining_operations.load() > 0) {
                bool has_own_work = (successes_per_thread[t].load() < operations_per_thread);
                if (has_own_work) {
                    // This thread has its own work - try to exchange
                    int value_to_send = t * 1000 + successes_per_thread[t].load();
                    auto result = exchanger->exchange(value_to_send, TIMEOUT);
                    if (result.has_value()) {
                        successes_per_thread[t]++;
                        total_remaining_operations--;
                    } else {
                        // Timeout - retry with backoff
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                } else {
                    // This thread is done with its own work, but must stay alive and help others
                    // Try to exchange with a dummy value - this allows other threads to find a partner
                    // The other thread will handle decrementing total_remaining_operations when it counts its success
                    auto result = exchanger->exchange(t * 1000 + operations_per_thread, TIMEOUT);
                    if (result.has_value()) {
                        // Successfully helped another thread - the other thread counted this exchange
                        // Just yield and continue helping
                        std::this_thread::yield();
                    } else {
                        // Timeout while helping - yield briefly and try again
                        std::this_thread::yield();
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All operations should succeed - no timeouts
    const int expected_total = num_threads * operations_per_thread;
    int total_successes = 0;
    for (int i = 0; i < num_threads; ++i) {
        total_successes += successes_per_thread[i].load();
    }
    
    EXPECT_EQ(total_successes, expected_total) << "All exchanges should succeed, no timeouts";
    EXPECT_EQ(total_remaining_operations.load(), 0) << "All operations should be completed";
    
    // Verify all threads got some successes (fairness)
    int threads_with_successes = 0;
    for (int i = 0; i < num_threads; ++i) {
        if (successes_per_thread[i].load() > 0) {
            threads_with_successes++;
        }
    }
    
    EXPECT_EQ(threads_with_successes, num_threads) << "All threads should have successful exchanges";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

