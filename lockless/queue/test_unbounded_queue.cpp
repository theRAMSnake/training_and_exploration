#include "unbounded_lockless_queue.hpp"
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

class UnboundedQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = std::make_unique<UnboundedQueue<int>>();
    }

    void TearDown() override {
        queue.reset();
    }

    std::unique_ptr<UnboundedQueue<int>> queue;
};

// Test basic enqueue and dequeue functionality
TEST_F(UnboundedQueueTest, EnqueueDequeueSingleElement) {
    queue->enq(5);
    int value = queue->deq();
    EXPECT_EQ(value, 5);
}

TEST_F(UnboundedQueueTest, EnqueueDequeueMultipleElements) {
    queue->enq(1);
    queue->enq(2);
    queue->enq(3);
    
    EXPECT_EQ(queue->deq(), 1);
    EXPECT_EQ(queue->deq(), 2);
    EXPECT_EQ(queue->deq(), 3);
}

// Test FIFO ordering
TEST_F(UnboundedQueueTest, FIFOOrdering) {
    std::vector<int> values = {10, 20, 30, 40, 50};
    
    for (int val : values) {
        queue->enq(val);
    }
    
    for (int val : values) {
        EXPECT_EQ(queue->deq(), val);
    }
}

TEST_F(UnboundedQueueTest, FIFOOrderingMixed) {
    queue->enq(1);
    queue->enq(2);
    EXPECT_EQ(queue->deq(), 1);
    
    queue->enq(3);
    queue->enq(4);
    EXPECT_EQ(queue->deq(), 2);
    EXPECT_EQ(queue->deq(), 3);
    
    queue->enq(5);
    EXPECT_EQ(queue->deq(), 4);
    EXPECT_EQ(queue->deq(), 5);
}

// Test empty queue behavior
TEST_F(UnboundedQueueTest, DequeueFromEmptyQueue) {
    EXPECT_THROW(queue->deq(), std::runtime_error);
}

TEST_F(UnboundedQueueTest, DequeueAfterEmptying) {
    queue->enq(1);
    queue->enq(2);
    
    EXPECT_EQ(queue->deq(), 1);
    EXPECT_EQ(queue->deq(), 2);
    
    EXPECT_THROW(queue->deq(), std::runtime_error);
}

// Test with different data types
TEST_F(UnboundedQueueTest, NegativeNumbers) {
    queue->enq(-5);
    queue->enq(-10);
    queue->enq(0);
    queue->enq(7);
    
    EXPECT_EQ(queue->deq(), -5);
    EXPECT_EQ(queue->deq(), -10);
    EXPECT_EQ(queue->deq(), 0);
    EXPECT_EQ(queue->deq(), 7);
}

TEST_F(UnboundedQueueTest, LargeNumbers) {
    queue->enq(1000000);
    queue->enq(999999);
    queue->enq(1000001);
    
    EXPECT_EQ(queue->deq(), 1000000);
    EXPECT_EQ(queue->deq(), 999999);
    EXPECT_EQ(queue->deq(), 1000001);
}

// Test stress scenarios
TEST_F(UnboundedQueueTest, StressTest) {
    const int num_operations = 10000;
    
    // Enqueue many elements
    for (int i = 0; i < num_operations; ++i) {
        queue->enq(i);
    }
    
    // Dequeue all
    for (int i = 0; i < num_operations; ++i) {
        EXPECT_EQ(queue->deq(), i);
    }
    
    // Queue should be empty now
    EXPECT_THROW(queue->deq(), std::runtime_error);
}

TEST_F(UnboundedQueueTest, InterleavedEnqueueDequeue) {
    const int iterations = 1000;
    
    for (int i = 0; i < iterations; ++i) {
        queue->enq(i);
        EXPECT_EQ(queue->deq(), i);
    }
    
    EXPECT_THROW(queue->deq(), std::runtime_error);
}

// Test destructor and memory management
TEST_F(UnboundedQueueTest, DestructorTest) {
    queue->enq(1);
    queue->enq(2);
    queue->enq(3);
    
    // Reset the queue (should call destructor)
    queue.reset();
    
    // Create a new queue
    queue = std::make_unique<UnboundedQueue<int>>();
    queue->enq(10);
    EXPECT_EQ(queue->deq(), 10);
}

// Concurrent test class
class UnboundedQueueConcurrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = std::make_unique<UnboundedQueue<int>>();
    }

    void TearDown() override {
        queue.reset();
    }

    std::unique_ptr<UnboundedQueue<int>> queue;
    static constexpr int NUM_THREADS = 4;
    static constexpr int OPERATIONS_PER_THREAD = 1000;
};

// Test single producer, single consumer
TEST_F(UnboundedQueueConcurrentTest, SingleProducerSingleConsumer) {
    const int num_items = 10000;
    std::atomic<bool> producer_done{false};
    std::vector<int> consumed_values;
    
    // Producer thread
    std::thread producer([this, num_items, &producer_done]() {
        for (int i = 0; i < num_items; ++i) {
            queue->enq(i);
        }
        producer_done.store(true);
    });
    
    // Consumer thread - wait for items
    std::thread consumer([this, num_items, &producer_done, &consumed_values]() {
        int consumed = 0;
        while (consumed < num_items) {
            try {
                int val = queue->deq();
                consumed_values.push_back(val);
                consumed++;
            } catch (const std::runtime_error&) {
                // Queue empty, wait a bit
                std::this_thread::yield();
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    // Verify FIFO order
    EXPECT_EQ(consumed_values.size(), num_items);
    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(consumed_values[i], i);
    }
}

// Test multiple producers, single consumer
TEST_F(UnboundedQueueConcurrentTest, MultipleProducersSingleConsumer) {
    const int items_per_producer = 1000;
    std::vector<std::thread> producers;
    std::atomic<int> total_produced{0};
    std::vector<int> consumed_values;
    
    // Start producers
    for (int t = 0; t < NUM_THREADS; ++t) {
        producers.emplace_back([this, t, items_per_producer, &total_produced]() {
            for (int i = 0; i < items_per_producer; ++i) {
                queue->enq(t * 10000 + i);
                total_produced++;
            }
        });
    }
    
    // Consumer thread
    const int total_items = NUM_THREADS * items_per_producer;
    std::thread consumer([this, total_items, &consumed_values]() {
        int consumed = 0;
        while (consumed < total_items) {
            try {
                int val = queue->deq();
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
    
    // Verify each producer's values are in order (FIFO per producer)
    std::vector<std::vector<int>> producer_values(NUM_THREADS);
    for (int val : consumed_values) {
        int producer_id = val / 10000;
        producer_values[producer_id].push_back(val % 10000);
    }
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        EXPECT_EQ(producer_values[t].size(), items_per_producer);
        for (size_t i = 0; i < producer_values[t].size(); ++i) {
            EXPECT_EQ(producer_values[t][i], static_cast<int>(i));
        }
    }
}

// Test single producer, multiple consumers
TEST_F(UnboundedQueueConcurrentTest, SingleProducerMultipleConsumers) {
    const int num_items = NUM_THREADS * 1000;
    std::vector<std::thread> consumers;
    std::atomic<int> consumed_count{0};
    std::vector<int> all_consumed_values;
    std::mutex consumed_mutex;
    
    // Producer thread
    std::thread producer([this, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            queue->enq(i);
        }
    });
    
    // Consumer threads - each tries to consume items_per_consumer items
    const int items_per_consumer = num_items / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; ++t) {
        consumers.emplace_back([this, items_per_consumer, &consumed_count, &all_consumed_values, &consumed_mutex]() {
            int consumed = 0;
            while (consumed < items_per_consumer) {
                try {
                    int val = queue->deq();
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
    std::sort(all_consumed_values.begin(), all_consumed_values.end());
    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(all_consumed_values[i], i);
    }
}

// Test multiple producers, multiple consumers
TEST_F(UnboundedQueueConcurrentTest, MultipleProducersMultipleConsumers) {
    const int items_per_producer = 1000;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    std::vector<int> all_consumed_values;
    std::mutex consumed_mutex;
    
    // Producer threads
    for (int t = 0; t < NUM_THREADS; ++t) {
        producers.emplace_back([this, t, items_per_producer, &produced_count]() {
            for (int i = 0; i < items_per_producer; ++i) {
                queue->enq(t * 10000 + i);
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
                    int val = queue->deq();
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
    std::sort(all_consumed_values.begin(), all_consumed_values.end());
    for (int t = 0; t < NUM_THREADS; ++t) {
        for (int i = 0; i < items_per_producer; ++i) {
            int expected_val = t * 10000 + i;
            auto it = std::find(all_consumed_values.begin(), all_consumed_values.end(), expected_val);
            EXPECT_NE(it, all_consumed_values.end()) << "Value " << expected_val << " not found";
        }
    }
}

// Test exception handling with concurrent access
TEST_F(UnboundedQueueConcurrentTest, ConcurrentExceptionHandling) {
    std::vector<std::thread> threads;
    std::atomic<int> exceptions_caught{0};
    std::atomic<int> successful_dequeues{0};
    
    // Add a few items
    for (int i = 0; i < 100; ++i) {
        queue->enq(i);
    }
    
    // Multiple threads trying to dequeue (some will hit empty queue)
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, &exceptions_caught, &successful_dequeues]() {
            for (int i = 0; i < 50; ++i) {
                try {
                    queue->deq();
                    successful_dequeues++;
                } catch (const std::runtime_error&) {
                    exceptions_caught++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have 100 successful dequeues and rest exceptions
    EXPECT_EQ(successful_dequeues.load(), 100);
    EXPECT_EQ(exceptions_caught.load(), NUM_THREADS * 50 - 100);
}

// More rigorous concurrent tests
class UnboundedQueueRigorousTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = std::make_unique<UnboundedQueue<int>>();
    }

    void TearDown() override {
        queue.reset();
    }

    std::unique_ptr<UnboundedQueue<int>> queue;
    static constexpr int NUM_THREADS = 8;
    static constexpr int OPERATIONS_PER_THREAD = 2000;
};

// Test high concurrency stress
TEST_F(UnboundedQueueRigorousTest, HighConcurrencyStressTest) {
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    std::vector<int> all_consumed_values;
    std::mutex consumed_mutex;
    
    // Half producers, half consumers
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        producers.emplace_back([this, t, &produced_count]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                queue->enq(t * 100000 + i);
                produced_count++;
            }
        });
    }
    
    const int total_to_consume = (NUM_THREADS / 2) * OPERATIONS_PER_THREAD;
    const int per_consumer = total_to_consume / (NUM_THREADS / 2);
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        consumers.emplace_back([this, per_consumer, &consumed_count, &all_consumed_values, &consumed_mutex]() {
            int consumed = 0;
            while (consumed < per_consumer) {
                try {
                    int val = queue->deq();
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
    
    int expected_count = (NUM_THREADS / 2) * OPERATIONS_PER_THREAD;
    EXPECT_EQ(produced_count.load(), expected_count);
    EXPECT_EQ(consumed_count.load(), expected_count);
    EXPECT_EQ(all_consumed_values.size(), expected_count);
}

// Test rapid enqueue/dequeue cycles
TEST_F(UnboundedQueueRigorousTest, RapidEnqueueDequeueCycles) {
    std::vector<std::thread> threads;
    std::atomic<int> cycles_completed{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &cycles_completed]() {
            for (int cycle = 0; cycle < 100; ++cycle) {
                // Enqueue phase
                for (int i = 0; i < 10; ++i) {
                    queue->enq(t * 10000 + cycle * 100 + i);
                }
                
                // Dequeue phase
                for (int i = 0; i < 10; ++i) {
                    bool success = false;
                    while (!success) {
                        try {
                            queue->deq();
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
TEST_F(UnboundedQueueRigorousTest, MemoryConsistencyTest) {
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    
    // Producer threads
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        producers.emplace_back([this, t, &stop_flag, &produced]() {
            int counter = 0;
            while (!stop_flag.load()) {
                queue->enq(t * 1000000 + counter);
                produced++;
                counter++;
            }
        });
    }
    
    // Consumer threads
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        consumers.emplace_back([this, &stop_flag, &consumed]() {
            while (!stop_flag.load()) {
                try {
                    queue->deq();
                    consumed++;
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
    for (auto& thread : producers) {
        thread.join();
    }
    for (auto& thread : consumers) {
        thread.join();
    }
    
    // Drain remaining items
    int remaining = 0;
    while (true) {
        try {
            queue->deq();
            remaining++;
        } catch (const std::runtime_error&) {
            break;
        }
    }
    
    EXPECT_EQ(consumed.load() + remaining, produced.load());
}

// Test logical correctness - all items consumed
TEST_F(UnboundedQueueRigorousTest, LogicalCorrectnessTest) {
    const int NUM_PRODUCERS = 4;
    const int ITEMS_PER_PRODUCER = 1000;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::vector<int> all_consumed_values;
    std::mutex consumed_mutex;
    
    // Each producer produces a sequence
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([this, p, ITEMS_PER_PRODUCER]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                // Encode: producer_id * 1000000 + sequence_number
                queue->enq(p * 1000000 + i);
            }
        });
    }
    
    // Consumers dequeue all items
    std::atomic<int> total_consumed{0};
    const int per_consumer = ITEMS_PER_PRODUCER;
    for (int c = 0; c < NUM_PRODUCERS; ++c) {
        consumers.emplace_back([this, per_consumer, &all_consumed_values, 
                                &consumed_mutex, &total_consumed]() {
            int consumed = 0;
            while (consumed < per_consumer) {
                try {
                    int val = queue->deq();
                    consumed++;
                    total_consumed++;
                    
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
    
    // Verify all items were consumed
    EXPECT_EQ(total_consumed.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    EXPECT_EQ(all_consumed_values.size(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    
    // Verify all expected values are present (no duplicates or missing values)
    std::sort(all_consumed_values.begin(), all_consumed_values.end());
    int expected_idx = 0;
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
            int expected_val = p * 1000000 + i;
            EXPECT_EQ(all_consumed_values[expected_idx], expected_val) 
                << "Expected value " << expected_val << " at sorted position " << expected_idx;
            expected_idx++;
        }
    }
}

// Test with bursts of activity
TEST_F(UnboundedQueueRigorousTest, BurstyWorkload) {
    std::vector<std::thread> threads;
    std::atomic<int> total_enqueued{0};
    std::atomic<int> total_dequeued{0};
    
    // Producers with bursts
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        threads.emplace_back([this, t, &total_enqueued]() {
            for (int burst = 0; burst < 10; ++burst) {
                // Burst of enqueues
                for (int i = 0; i < 100; ++i) {
                    queue->enq(t * 100000 + burst * 1000 + i);
                    total_enqueued++;
                }
                // Small pause
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    // Consumers with bursts
    const int per_consumer = (NUM_THREADS / 2) * 1000 / (NUM_THREADS / 2);
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        threads.emplace_back([this, per_consumer, &total_dequeued]() {
            int consumed = 0;
            while (consumed < per_consumer) {
                try {
                    queue->deq();
                    consumed++;
                    total_dequeued++;
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
    EXPECT_EQ(total_enqueued.load(), expected);
    EXPECT_EQ(total_dequeued.load(), expected);
}

// Test fairness - no thread starvation
TEST_F(UnboundedQueueRigorousTest, FairnessTest) {
    const int NUM_PRODUCERS = 8;
    const int ITEMS_PER_PRODUCER = 500;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::vector<std::atomic<int>> items_consumed_from_producer(NUM_PRODUCERS);
    
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        items_consumed_from_producer[i].store(0);
    }
    
    // Producers
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([this, p, ITEMS_PER_PRODUCER]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                queue->enq(p * 1000000 + i);
            }
        });
    }
    
    // Consumers
    const int per_consumer = ITEMS_PER_PRODUCER;
    for (int c = 0; c < NUM_PRODUCERS; ++c) {
        consumers.emplace_back([this, per_consumer, &items_consumed_from_producer]() {
            int consumed = 0;
            while (consumed < per_consumer) {
                try {
                    int val = queue->deq();
                    int producer_id = val / 1000000;
                    items_consumed_from_producer[producer_id]++;
                    consumed++;
                } catch (const std::runtime_error&) {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Wait for all
    for (auto& thread : producers) {
        thread.join();
    }
    for (auto& thread : consumers) {
        thread.join();
    }
    
    // Verify each producer had all items consumed
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        EXPECT_EQ(items_consumed_from_producer[p].load(), ITEMS_PER_PRODUCER)
            << "Producer " << p << " did not have all items consumed";
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

