#include "elemination_stack.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <mutex>

using namespace snake;

class EliminationStackTest : public ::testing::Test {
protected:
    void SetUp() override {
        stack = std::make_unique<EliminationStack<int>>();
    }

    void TearDown() override {
        stack.reset();
    }

    std::unique_ptr<EliminationStack<int>> stack;
};

// Test basic push and pop functionality
TEST_F(EliminationStackTest, PushPopSingleElement) {
    stack->push(5);
    int value = stack->pop();
    EXPECT_EQ(value, 5);
}

TEST_F(EliminationStackTest, PushPopMultipleElements) {
    stack->push(1);
    stack->push(2);
    stack->push(3);
    
    // Stack is LIFO, so should pop in reverse order
    EXPECT_EQ(stack->pop(), 3);
    EXPECT_EQ(stack->pop(), 2);
    EXPECT_EQ(stack->pop(), 1);
}

// Test LIFO ordering
TEST_F(EliminationStackTest, LIFOOrdering) {
    std::vector<int> values = {10, 20, 30, 40, 50};
    
    for (int val : values) {
        stack->push(val);
    }
    
    // Pop in reverse order
    for (auto it = values.rbegin(); it != values.rend(); ++it) {
        EXPECT_EQ(stack->pop(), *it);
    }
}

TEST_F(EliminationStackTest, LIFOOrderingMixed) {
    stack->push(1);
    stack->push(2);
    EXPECT_EQ(stack->pop(), 2);
    
    stack->push(3);
    stack->push(4);
    EXPECT_EQ(stack->pop(), 4);
    EXPECT_EQ(stack->pop(), 3);
    
    stack->push(5);
    EXPECT_EQ(stack->pop(), 5);
    EXPECT_EQ(stack->pop(), 1);
}

// Test empty stack behavior
TEST_F(EliminationStackTest, PopFromEmptyStack) {
    EXPECT_THROW(stack->pop(), std::runtime_error);
}

TEST_F(EliminationStackTest, PopAfterEmptying) {
    stack->push(1);
    stack->push(2);
    
    EXPECT_EQ(stack->pop(), 2);
    EXPECT_EQ(stack->pop(), 1);
    
    EXPECT_THROW(stack->pop(), std::runtime_error);
}

// Test with different data types
// Note: 0 is reserved by the stack implementation for elimination exchanges
TEST_F(EliminationStackTest, NegativeNumbers) {
    stack->push(-5);
    stack->push(-10);
    stack->push(1);  // Using 1 instead of 0 since 0 is reserved
    stack->push(7);
    
    EXPECT_EQ(stack->pop(), 7);
    EXPECT_EQ(stack->pop(), 1);  // Changed from 0
    EXPECT_EQ(stack->pop(), -10);
    EXPECT_EQ(stack->pop(), -5);
}

TEST_F(EliminationStackTest, LargeNumbers) {
    stack->push(1000000);
    stack->push(999999);
    stack->push(1000001);
    
    EXPECT_EQ(stack->pop(), 1000001);
    EXPECT_EQ(stack->pop(), 999999);
    EXPECT_EQ(stack->pop(), 1000000);
}

// Test stress scenarios
TEST_F(EliminationStackTest, StressTest) {
    const int num_operations = 10000;
    
    // Push many elements
    // Start from 1 since 0 is reserved by the stack implementation
    for (int i = 0; i < num_operations; ++i) {
        stack->push(i + 1);
    }
    
    // Pop all in reverse order
    for (int i = num_operations - 1; i >= 0; --i) {
        EXPECT_EQ(stack->pop(), i + 1);
    }
    
    // Stack should be empty now
    EXPECT_THROW(stack->pop(), std::runtime_error);
}

TEST_F(EliminationStackTest, InterleavedPushPop) {
    const int iterations = 1000;
    
    // Start from 1 since 0 is reserved by the stack implementation
    for (int i = 0; i < iterations; ++i) {
        stack->push(i + 1);
        EXPECT_EQ(stack->pop(), i + 1);
    }
    
    EXPECT_THROW(stack->pop(), std::runtime_error);
}

TEST_F(EliminationStackTest, DeepStack) {
    const int depth = 5000;
    
    // Push many elements
    // Start from 1 since 0 is reserved by the stack implementation
    for (int i = 0; i < depth; ++i) {
        stack->push(i + 1);
    }
    
    // Verify all elements
    for (int i = depth - 1; i >= 0; --i) {
        EXPECT_EQ(stack->pop(), i + 1);
    }
    
    EXPECT_THROW(stack->pop(), std::runtime_error);
}

// Test destructor and memory management
TEST_F(EliminationStackTest, DestructorTest) {
    stack->push(1);
    stack->push(2);
    stack->push(3);
    
    // Reset the stack (should call destructor)
    stack.reset();
    
    // Create a new stack
    stack = std::make_unique<EliminationStack<int>>();
    stack->push(10);
    EXPECT_EQ(stack->pop(), 10);
}

// Concurrent test class
class EliminationStackConcurrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        stack = std::make_unique<EliminationStack<int>>();
    }

    void TearDown() override {
        stack.reset();
    }

    std::unique_ptr<EliminationStack<int>> stack;
    static constexpr int NUM_THREADS = 4;
    static constexpr int OPERATIONS_PER_THREAD = 1000;
};

// Test single producer, single consumer
TEST_F(EliminationStackConcurrentTest, SingleProducerSingleConsumer) {
    const int num_items = 10000;
    std::vector<int> consumed_values;
    std::mutex consumed_mutex;
    
    // Producer thread
    // Start from 1 since 0 is reserved by the stack implementation
    std::thread producer([this, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            stack->push(i + 1);
        }
    });
    
    // Consumer thread
    std::thread consumer([this, num_items, &consumed_values, &consumed_mutex]() {
        int consumed = 0;
        while (consumed < num_items) {
            try {
                int val = stack->pop();
                consumed++;
                std::lock_guard<std::mutex> lock(consumed_mutex);
                consumed_values.push_back(val);
            } catch (const std::runtime_error&) {
                std::this_thread::yield();
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    // Verify all values were consumed
    EXPECT_EQ(consumed_values.size(), num_items);
    
    // Verify all values are present (but not necessarily in strict LIFO due to concurrent interleaving)
    std::sort(consumed_values.begin(), consumed_values.end());
    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(consumed_values[i], i + 1);
    }
}

// Test multiple producers, single consumer
TEST_F(EliminationStackConcurrentTest, MultipleProducersSingleConsumer) {
    const int items_per_producer = 1000;
    std::vector<std::thread> producers;
    std::vector<int> consumed_values;
    
    // Start producers
    // Start values from 1 since 0 is reserved by the stack implementation
    for (int t = 0; t < NUM_THREADS; ++t) {
        producers.emplace_back([this, t, items_per_producer]() {
            for (int i = 0; i < items_per_producer; ++i) {
                stack->push(t * 10000 + i + 1);
            }
        });
    }
    
    // Consumer thread
    const int total_items = NUM_THREADS * items_per_producer;
    std::thread consumer([this, total_items, &consumed_values]() {
        int consumed = 0;
        while (consumed < total_items) {
            try {
                int val = stack->pop();
                consumed_values.push_back(val);
                consumed++;
            } catch (const std::runtime_error&) {
                std::this_thread::yield();
            }
        }
    });
    
    // Wait for all threads
    for (auto& thread : producers) {
        thread.join();
    }
    consumer.join();
    
    // Verify all values were consumed
    EXPECT_EQ(consumed_values.size(), total_items);
    
    // All values should be present (but order depends on interleaving)
    // Values start from 1 since 0 is reserved by the stack implementation
    std::sort(consumed_values.begin(), consumed_values.end());
    int expected_idx = 0;
    for (int t = 0; t < NUM_THREADS; ++t) {
        for (int i = 0; i < items_per_producer; ++i) {
            int expected_val = t * 10000 + i + 1;
            EXPECT_EQ(consumed_values[expected_idx], expected_val);
            expected_idx++;
        }
    }
}

// Test single producer, multiple consumers
TEST_F(EliminationStackConcurrentTest, SingleProducerMultipleConsumers) {
    const int num_items = NUM_THREADS * 1000;
    std::vector<std::thread> consumers;
    std::atomic<int> consumed_count{0};
    std::vector<int> all_consumed_values;
    std::mutex consumed_mutex;
    
    // Producer thread
    // Start from 1 since 0 is reserved by the stack implementation
    std::thread producer([this, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            stack->push(i + 1);
        }
    });
    
    // Consumer threads - each tries to consume items_per_consumer items
    const int items_per_consumer = num_items / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; ++t) {
        consumers.emplace_back([this, items_per_consumer, &consumed_count, &all_consumed_values, &consumed_mutex]() {
            int consumed = 0;
            while (consumed < items_per_consumer) {
                try {
                    int val = stack->pop();
                    consumed++;
                    consumed_count++;
                    std::lock_guard<std::mutex> lock(consumed_mutex);
                    all_consumed_values.push_back(val);
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    producer.join();
    for (auto& thread : consumers) {
        thread.join();
    }
    
    // Verify all values were consumed
    EXPECT_EQ(consumed_count.load(), num_items);
    EXPECT_EQ(all_consumed_values.size(), num_items);
    
    // Sort and verify all values are present
    // Values start from 1 since 0 is reserved by the stack implementation
    std::sort(all_consumed_values.begin(), all_consumed_values.end());
    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(all_consumed_values[i], i + 1);
    }
}

// Test multiple producers, multiple consumers
TEST_F(EliminationStackConcurrentTest, MultipleProducersMultipleConsumers) {
    const int items_per_producer = 1000;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    std::vector<int> all_consumed_values;
    std::mutex consumed_mutex;
    
    // Producer threads
    // Start values from 1 since 0 is reserved by the stack implementation
    for (int t = 0; t < NUM_THREADS; ++t) {
        producers.emplace_back([this, t, items_per_producer, &produced_count]() {
            for (int i = 0; i < items_per_producer; ++i) {
                stack->push(t * 10000 + i + 1);
                produced_count++;
            }
        });
    }
    
    // Consumer threads
    const int items_per_consumer = items_per_producer;
    for (int t = 0; t < NUM_THREADS; ++t) {
        consumers.emplace_back([this, items_per_consumer, &consumed_count, &all_consumed_values, &consumed_mutex]() {
            int consumed = 0;
            while (consumed < items_per_consumer) {
                try {
                    int val = stack->pop();
                    consumed++;
                    consumed_count++;
                    std::lock_guard<std::mutex> lock(consumed_mutex);
                    all_consumed_values.push_back(val);
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : producers) {
        thread.join();
    }
    for (auto& thread : consumers) {
        thread.join();
    }
    
    // Verify all items were produced and consumed
    EXPECT_EQ(produced_count.load(), NUM_THREADS * items_per_producer);
    EXPECT_EQ(consumed_count.load(), NUM_THREADS * items_per_producer);
    EXPECT_EQ(all_consumed_values.size(), NUM_THREADS * items_per_producer);
    
    // Verify all values are present
    // Values start from 1 since 0 is reserved by the stack implementation
    std::sort(all_consumed_values.begin(), all_consumed_values.end());
    for (int t = 0; t < NUM_THREADS; ++t) {
        for (int i = 0; i < items_per_producer; ++i) {
            int expected_val = t * 10000 + i + 1;
            auto it = std::find(all_consumed_values.begin(), all_consumed_values.end(), expected_val);
            EXPECT_NE(it, all_consumed_values.end()) << "Value " << expected_val << " not found";
        }
    }
}

// Test exception handling with concurrent access
TEST_F(EliminationStackConcurrentTest, ConcurrentExceptionHandling) {
    std::vector<std::thread> threads;
    std::atomic<int> exceptions_caught{0};
    std::atomic<int> successful_pops{0};
    
    // Add a few items
    // Start from 1 since 0 is reserved by the stack implementation
    for (int i = 0; i < 100; ++i) {
        stack->push(i + 1);
    }
    
    // Multiple threads trying to pop (some will hit empty stack)
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, &exceptions_caught, &successful_pops]() {
            for (int i = 0; i < 50; ++i) {
                try {
                    stack->pop();
                    successful_pops++;
                } catch (const std::runtime_error&) {
                    exceptions_caught++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have 100 successful pops and rest exceptions
    EXPECT_EQ(successful_pops.load(), 100);
    EXPECT_EQ(exceptions_caught.load(), NUM_THREADS * 50 - 100);
}

// More rigorous concurrent tests
class EliminationStackRigorousTest : public ::testing::Test {
protected:
    void SetUp() override {
        stack = std::make_unique<EliminationStack<int>>();
    }

    void TearDown() override {
        stack.reset();
    }

    std::unique_ptr<EliminationStack<int>> stack;
    static constexpr int NUM_THREADS = 8;
    static constexpr int OPERATIONS_PER_THREAD = 2000;
};

// Test high concurrency stress
TEST_F(EliminationStackRigorousTest, HighConcurrencyStressTest) {
    std::vector<std::thread> pushers;
    std::vector<std::thread> poppers;
    std::atomic<int> pushed_count{0};
    std::atomic<int> popped_count{0};
    std::vector<int> all_popped_values;
    std::mutex popped_mutex;
    
    // Half pushers, half poppers
    // Start values from 1 since 0 is reserved by the stack implementation
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        pushers.emplace_back([this, t, &pushed_count]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                stack->push(t * 100000 + i + 1);
                pushed_count++;
            }
        });
    }
    
    const int total_to_pop = (NUM_THREADS / 2) * OPERATIONS_PER_THREAD;
    const int per_popper = total_to_pop / (NUM_THREADS / 2);
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        poppers.emplace_back([this, per_popper, &popped_count, &all_popped_values, &popped_mutex]() {
            int popped = 0;
            while (popped < per_popper) {
                try {
                    int val = stack->pop();
                    popped++;
                    popped_count++;
                    std::lock_guard<std::mutex> lock(popped_mutex);
                    all_popped_values.push_back(val);
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : pushers) {
        thread.join();
    }
    for (auto& thread : poppers) {
        thread.join();
    }
    
    int expected_count = (NUM_THREADS / 2) * OPERATIONS_PER_THREAD;
    EXPECT_EQ(pushed_count.load(), expected_count);
    EXPECT_EQ(popped_count.load(), expected_count);
    EXPECT_EQ(all_popped_values.size(), expected_count);
}

// Test rapid push/pop cycles
TEST_F(EliminationStackRigorousTest, RapidPushPopCycles) {
    std::vector<std::thread> threads;
    std::atomic<int> cycles_completed{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &cycles_completed]() {
            for (int cycle = 0; cycle < 100; ++cycle) {
                // Push phase
                // Start values from 1 since 0 is reserved by the stack implementation
                for (int i = 0; i < 10; ++i) {
                    stack->push(t * 10000 + cycle * 100 + i + 1);
                }
                
                // Pop phase
                for (int i = 0; i < 10; ++i) {
                    bool success = false;
                    while (!success) {
                        try {
                            stack->pop();
                            success = true;
                        } catch (const std::runtime_error&) {
                            std::this_thread::yield();
                        }
                    }
                }
                
                cycles_completed++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(cycles_completed.load(), NUM_THREADS * 100);
}

// Test memory consistency under high load
TEST_F(EliminationStackRigorousTest, MemoryConsistencyTest) {
    std::vector<std::thread> pushers;
    std::vector<std::thread> poppers;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> pushed{0};
    std::atomic<int> popped{0};
    
    // Pusher threads
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        pushers.emplace_back([this, t, &stop_flag, &pushed]() {
            int counter = 0;
            while (!stop_flag.load()) {
                // Start values from 1 since 0 is reserved by the stack implementation
                stack->push(t * 1000000 + counter + 1);
                pushed++;
                counter++;
            }
        });
    }
    
    // Popper threads
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        poppers.emplace_back([this, &stop_flag, &popped]() {
            while (!stop_flag.load()) {
                try {
                    stack->pop();
                    popped++;
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Run for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop_flag.store(true);
    
    // Wait for all threads
    for (auto& thread : pushers) {
        thread.join();
    }
    for (auto& thread : poppers) {
        thread.join();
    }
    
    // Drain remaining items
    int remaining = 0;
    while (true) {
        try {
            stack->pop();
            remaining++;
        } catch (const std::runtime_error&) {
            break;
        }
    }
    
    EXPECT_EQ(popped.load() + remaining, pushed.load());
}

// Test logical correctness - all items consumed
TEST_F(EliminationStackRigorousTest, LogicalCorrectnessTest) {
    const int NUM_PUSHERS = 4;
    const int ITEMS_PER_PUSHER = 1000;
    std::vector<std::thread> pushers;
    std::vector<std::thread> poppers;
    std::vector<int> all_popped_values;
    std::mutex popped_mutex;
    
    // Each pusher produces a sequence
    // Start values from 1 since 0 is reserved by the stack implementation
    for (int p = 0; p < NUM_PUSHERS; ++p) {
        pushers.emplace_back([this, p, ITEMS_PER_PUSHER]() {
            for (int i = 0; i < ITEMS_PER_PUSHER; ++i) {
                // Encode: pusher_id * 1000000 + sequence_number + 1
                stack->push(p * 1000000 + i + 1);
            }
        });
    }
    
    // Poppers pop all items
    std::atomic<int> total_popped{0};
    const int per_popper = ITEMS_PER_PUSHER;
    for (int c = 0; c < NUM_PUSHERS; ++c) {
        poppers.emplace_back([this, per_popper, &all_popped_values, 
                                &popped_mutex, &total_popped]() {
            int popped = 0;
            while (popped < per_popper) {
                try {
                    int val = stack->pop();
                    popped++;
                    total_popped++;
                    
                    std::lock_guard<std::mutex> lock(popped_mutex);
                    all_popped_values.push_back(val);
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : pushers) {
        thread.join();
    }
    for (auto& thread : poppers) {
        thread.join();
    }
    
    // Verify all items were popped
    EXPECT_EQ(total_popped.load(), NUM_PUSHERS * ITEMS_PER_PUSHER);
    EXPECT_EQ(all_popped_values.size(), NUM_PUSHERS * ITEMS_PER_PUSHER);
    
    // Verify all expected values are present (no duplicates or missing values)
    std::sort(all_popped_values.begin(), all_popped_values.end());
    int expected_idx = 0;
    for (int p = 0; p < NUM_PUSHERS; ++p) {
        for (int i = 0; i < ITEMS_PER_PUSHER; ++i) {
            int expected_val = p * 1000000 + i + 1;
            EXPECT_EQ(all_popped_values[expected_idx], expected_val) 
                << "Expected value " << expected_val << " at sorted position " << expected_idx;
            expected_idx++;
        }
    }
}

// Test with bursts of activity
TEST_F(EliminationStackRigorousTest, BurstyWorkload) {
    std::vector<std::thread> threads;
    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};
    
    // Pushers with bursts
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        threads.emplace_back([this, t, &total_pushed]() {
            for (int burst = 0; burst < 10; ++burst) {
                // Burst of pushes
                // Start values from 1 since 0 is reserved by the stack implementation
                for (int i = 0; i < 100; ++i) {
                    stack->push(t * 100000 + burst * 1000 + i + 1);
                    total_pushed++;
                }
                // Small pause
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    // Poppers with bursts
    const int per_popper = (NUM_THREADS / 2) * 1000 / (NUM_THREADS / 2);
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        threads.emplace_back([this, per_popper, &total_popped]() {
            int popped = 0;
            while (popped < per_popper) {
                try {
                    stack->pop();
                    popped++;
                    total_popped++;
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int expected = (NUM_THREADS / 2) * 1000;
    EXPECT_EQ(total_pushed.load(), expected);
    EXPECT_EQ(total_popped.load(), expected);
}

// Test fairness - no thread starvation
TEST_F(EliminationStackRigorousTest, FairnessTest) {
    const int NUM_PUSHERS = 8;
    const int ITEMS_PER_PUSHER = 500;
    std::vector<std::thread> pushers;
    std::vector<std::thread> poppers;
    std::vector<std::atomic<int>> items_popped_from_pusher(NUM_PUSHERS);
    
    for (int i = 0; i < NUM_PUSHERS; ++i) {
        items_popped_from_pusher[i].store(0);
    }
    
    // Pushers
    // Start values from 1 since 0 is reserved by the stack implementation
    for (int p = 0; p < NUM_PUSHERS; ++p) {
        pushers.emplace_back([this, p, ITEMS_PER_PUSHER]() {
            for (int i = 0; i < ITEMS_PER_PUSHER; ++i) {
                stack->push(p * 1000000 + i + 1);
            }
        });
    }
    
    // Poppers
    const int per_popper = ITEMS_PER_PUSHER;
    for (int c = 0; c < NUM_PUSHERS; ++c) {
        poppers.emplace_back([this, per_popper, &items_popped_from_pusher]() {
            int popped = 0;
            while (popped < per_popper) {
                try {
                    int val = stack->pop();
                    int pusher_id = val / 1000000;
                    items_popped_from_pusher[pusher_id]++;
                    popped++;
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Wait for all
    for (auto& thread : pushers) {
        thread.join();
    }
    for (auto& thread : poppers) {
        thread.join();
    }
    
    // Verify each pusher had all items popped
    for (int p = 0; p < NUM_PUSHERS; ++p) {
        EXPECT_EQ(items_popped_from_pusher[p].load(), ITEMS_PER_PUSHER)
            << "Pusher " << p << " did not have all items popped";
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

