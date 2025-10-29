#include "bounded_queue.hpp"
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

class BoundedQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = std::make_unique<BoundedQueue<int>>(10);
    }

    void TearDown() override {
        queue.reset();
    }

    std::unique_ptr<BoundedQueue<int>> queue;
};

// Test basic enqueue and dequeue functionality
TEST_F(BoundedQueueTest, EnqueueDequeueSingleElement) {
    queue->enq(5);
    int value = queue->deq();
    EXPECT_EQ(value, 5);
}

TEST_F(BoundedQueueTest, EnqueueDequeueMultipleElements) {
    queue->enq(1);
    queue->enq(2);
    queue->enq(3);
    
    EXPECT_EQ(queue->deq(), 1);
    EXPECT_EQ(queue->deq(), 2);
    EXPECT_EQ(queue->deq(), 3);
}

// Test FIFO ordering
TEST_F(BoundedQueueTest, FIFOOrdering) {
    std::vector<int> values = {10, 20, 30, 40, 50};
    
    for (int val : values) {
        queue->enq(val);
    }
    
    for (int val : values) {
        EXPECT_EQ(queue->deq(), val);
    }
}

TEST_F(BoundedQueueTest, FIFOOrderingMixed) {
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

// Test capacity limits
TEST_F(BoundedQueueTest, FillToCapacity) {
    // Queue capacity is 10
    for (int i = 0; i < 10; ++i) {
        queue->enq(i);
    }
    
    // Dequeue all
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(queue->deq(), i);
    }
}

TEST_F(BoundedQueueTest, EnqueueDequeuePattern) {
    // Fill half
    for (int i = 0; i < 5; ++i) {
        queue->enq(i);
    }
    
    // Dequeue half
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(queue->deq(), i);
    }
    
    // Fill completely
    for (int i = 5; i < 15; ++i) {
        queue->enq(i);
    }
    
    // Dequeue all
    for (int i = 5; i < 15; ++i) {
        EXPECT_EQ(queue->deq(), i);
    }
}

// Test with different data types
TEST_F(BoundedQueueTest, NegativeNumbers) {
    queue->enq(-5);
    queue->enq(-10);
    queue->enq(0);
    queue->enq(7);
    
    EXPECT_EQ(queue->deq(), -5);
    EXPECT_EQ(queue->deq(), -10);
    EXPECT_EQ(queue->deq(), 0);
    EXPECT_EQ(queue->deq(), 7);
}

// Test stress scenarios
TEST_F(BoundedQueueTest, StressTest) {
    const int num_operations = 1000;
    
    // Enqueue and dequeue many elements, staying within capacity
    for (int i = 0; i < num_operations; ++i) {
        // Dequeue first to make room if at capacity
        if (i >= 10) {
            int val = queue->deq();
            EXPECT_EQ(val, i - 10);
        }
        // Then enqueue
        queue->enq(i);
    }
    
    // Dequeue remaining
    for (int i = num_operations - 10; i < num_operations; ++i) {
        EXPECT_EQ(queue->deq(), i);
    }
}

// Test destructor and memory management
TEST_F(BoundedQueueTest, DestructorTest) {
    queue->enq(1);
    queue->enq(2);
    queue->enq(3);
    
    // Reset the queue (should call destructor)
    queue.reset();
    
    // Create a new queue
    queue = std::make_unique<BoundedQueue<int>>(5);
    queue->enq(10);
    EXPECT_EQ(queue->deq(), 10);
}

// Concurrent test class
class BoundedQueueConcurrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = std::make_unique<BoundedQueue<int>>(100);
    }

    void TearDown() override {
        queue.reset();
    }

    std::unique_ptr<BoundedQueue<int>> queue;
    static constexpr int NUM_THREADS = 4;
    static constexpr int OPERATIONS_PER_THREAD = 1000;
};

// Test single producer, single consumer
TEST_F(BoundedQueueConcurrentTest, SingleProducerSingleConsumer) {
    const int num_items = 10000;
    std::atomic<bool> producer_done{false};
    std::vector<int> consumed_values;
    std::mutex consumed_mutex;
    
    // Producer thread
    std::thread producer([this, num_items, &producer_done]() {
        for (int i = 0; i < num_items; ++i) {
            queue->enq(i);
        }
        producer_done.store(true);
    });
    
    // Consumer thread
    std::thread consumer([this, num_items, &consumed_values, &consumed_mutex]() {
        for (int i = 0; i < num_items; ++i) {
            int val = queue->deq();
            std::lock_guard<std::mutex> lock(consumed_mutex);
            consumed_values.push_back(val);
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
TEST_F(BoundedQueueConcurrentTest, MultipleProducersSingleConsumer) {
    const int items_per_producer = 1000;
    std::vector<std::thread> producers;
    std::vector<int> consumed_values;
    std::mutex consumed_mutex;
    
    // Start producers
    for (int t = 0; t < NUM_THREADS; ++t) {
        producers.emplace_back([this, t, items_per_producer]() {
            for (int i = 0; i < items_per_producer; ++i) {
                queue->enq(t * 10000 + i);
            }
        });
    }
    
    // Consumer thread
    std::thread consumer([this, items_per_producer, &consumed_values, &consumed_mutex]() {
        for (int i = 0; i < NUM_THREADS * items_per_producer; ++i) {
            int val = queue->deq();
            std::lock_guard<std::mutex> lock(consumed_mutex);
            consumed_values.push_back(val);
        }
    });
    
    // Wait for all threads
    for (auto& thread : producers) {
        thread.join();
    }
    consumer.join();
    
    // Verify all values were consumed
    EXPECT_EQ(consumed_values.size(), NUM_THREADS * items_per_producer);
    
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
TEST_F(BoundedQueueConcurrentTest, SingleProducerMultipleConsumers) {
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
    
    // Consumer threads - each consumes exactly items_per_consumer items
    const int items_per_consumer = num_items / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; ++t) {
        consumers.emplace_back([this, items_per_consumer, &consumed_count, &all_consumed_values, &consumed_mutex]() {
            for (int i = 0; i < items_per_consumer; ++i) {
                int val = queue->deq();
                consumed_count++;
                std::lock_guard<std::mutex> lock(consumed_mutex);
                all_consumed_values.push_back(val);
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
TEST_F(BoundedQueueConcurrentTest, MultipleProducersMultipleConsumers) {
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
    for (int t = 0; t < NUM_THREADS; ++t) {
        consumers.emplace_back([this, items_per_producer, &consumed_count, &all_consumed_values, &consumed_mutex]() {
            for (int i = 0; i < items_per_producer; ++i) {
                int val = queue->deq();
                consumed_count++;
                std::lock_guard<std::mutex> lock(consumed_mutex);
                all_consumed_values.push_back(val);
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
    
    // Verify all values are present (but not necessarily in strict FIFO per producer)
    std::sort(all_consumed_values.begin(), all_consumed_values.end());
    for (int t = 0; t < NUM_THREADS; ++t) {
        for (int i = 0; i < items_per_producer; ++i) {
            int expected_val = t * 10000 + i;
            auto it = std::find(all_consumed_values.begin(), all_consumed_values.end(), expected_val);
            EXPECT_NE(it, all_consumed_values.end()) << "Value " << expected_val << " not found";
        }
    }
}

// Test blocking behavior when queue is full
TEST_F(BoundedQueueConcurrentTest, BlockingWhenFull) {
    auto small_queue = std::make_unique<BoundedQueue<int>>(5);
    std::atomic<bool> producer_blocked{false};
    std::atomic<bool> producer_resumed{false};
    
    // Fill the queue
    for (int i = 0; i < 5; ++i) {
        small_queue->enq(i);
    }
    
    // Start a producer that will block
    std::thread producer([&small_queue, &producer_blocked, &producer_resumed]() {
        producer_blocked.store(true);
        small_queue->enq(100); // This should block
        producer_resumed.store(true);
    });
    
    // Wait a bit to ensure producer is blocked
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(producer_blocked.load());
    EXPECT_FALSE(producer_resumed.load());
    
    // Dequeue one item to unblock producer
    small_queue->deq();
    
    // Wait for producer to resume
    producer.join();
    EXPECT_TRUE(producer_resumed.load());
}

// Test blocking behavior when queue is empty
TEST_F(BoundedQueueConcurrentTest, BlockingWhenEmpty) {
    auto empty_queue = std::make_unique<BoundedQueue<int>>(5);
    std::atomic<bool> consumer_blocked{false};
    std::atomic<bool> consumer_resumed{false};
    std::atomic<int> consumed_value{-1};
    
    // Start a consumer that will block
    std::thread consumer([&empty_queue, &consumer_blocked, &consumer_resumed, &consumed_value]() {
        consumer_blocked.store(true);
        int val = empty_queue->deq(); // This should block
        consumed_value.store(val);
        consumer_resumed.store(true);
    });
    
    // Wait a bit to ensure consumer is blocked
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(consumer_blocked.load());
    EXPECT_FALSE(consumer_resumed.load());
    
    // Enqueue an item to unblock consumer
    empty_queue->enq(42);
    
    // Wait for consumer to resume
    consumer.join();
    EXPECT_TRUE(consumer_resumed.load());
    EXPECT_EQ(consumed_value.load(), 42);
}

// More rigorous concurrent tests
class BoundedQueueRigorousTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = std::make_unique<BoundedQueue<int>>(200);
    }

    void TearDown() override {
        queue.reset();
    }

    std::unique_ptr<BoundedQueue<int>> queue;
    static constexpr int NUM_THREADS = 8;
    static constexpr int OPERATIONS_PER_THREAD = 2000;
};

// Test high concurrency stress
TEST_F(BoundedQueueRigorousTest, HighConcurrencyStressTest) {
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
    
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        consumers.emplace_back([this, &consumed_count, &all_consumed_values, &consumed_mutex]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int val = queue->deq();
                consumed_count++;
                std::lock_guard<std::mutex> lock(consumed_mutex);
                all_consumed_values.push_back(val);
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
TEST_F(BoundedQueueRigorousTest, RapidEnqueueDequeueCycles) {
    std::vector<std::thread> threads;
    std::atomic<int> cycles_completed{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &cycles_completed]() {
            for (int cycle = 0; cycle < 100; ++cycle) {
                // Producer phase
                for (int i = 0; i < 10; ++i) {
                    queue->enq(t * 10000 + cycle * 100 + i);
                }
                
                // Consumer phase
                for (int i = 0; i < 10; ++i) {
                    queue->deq();
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

// Test with varying queue sizes
TEST_F(BoundedQueueRigorousTest, VaryingQueueSizes) {
    std::vector<std::unique_ptr<BoundedQueue<int>>> queues;
    std::vector<std::thread> threads;
    
    // Create queues with different capacities
    std::vector<int> capacities = {5, 10, 20, 50, 100, 200, 500, 1000};
    for (int capacity : capacities) {
        queues.push_back(std::make_unique<BoundedQueue<int>>(capacity));
    }
    
    // Test each queue with concurrent operations
    for (size_t q = 0; q < queues.size(); ++q) {
        threads.clear();
        std::atomic<int> produced{0};
        std::atomic<int> consumed{0};
        const int ops_per_thread = 100;
        
        // Producer thread
        threads.emplace_back([&queues, q, &produced, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                queues[q]->enq(i);
                produced++;
            }
        });
        
        // Consumer thread
        threads.emplace_back([&queues, q, &consumed, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                queues[q]->deq();
                consumed++;
            }
        });
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        EXPECT_EQ(produced.load(), ops_per_thread);
        EXPECT_EQ(consumed.load(), ops_per_thread);
    }
}

// Test memory consistency under high load
TEST_F(BoundedQueueRigorousTest, MemoryConsistencyTest) {
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> operations_count{0};
    
    // Producer threads
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        producers.emplace_back([this, t, &stop_flag, &operations_count]() {
            int counter = 0;
            while (!stop_flag.load()) {
                queue->enq(t * 1000000 + counter);
                operations_count++;
                counter++;
            }
        });
    }
    
    // Consumer threads
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        consumers.emplace_back([this, &stop_flag, &operations_count]() {
            while (!stop_flag.load()) {
                queue->deq();
                operations_count++;
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
    
    EXPECT_GT(operations_count.load(), 0);
}

// Test logical correctness - all items consumed
TEST_F(BoundedQueueRigorousTest, LogicalCorrectnessTest) {
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
    for (int c = 0; c < NUM_PRODUCERS; ++c) {
        consumers.emplace_back([this, ITEMS_PER_PRODUCER, &all_consumed_values, 
                                &consumed_mutex, &total_consumed]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                int val = queue->deq();
                total_consumed++;
                
                std::lock_guard<std::mutex> lock(consumed_mutex);
                all_consumed_values.push_back(val);
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
TEST_F(BoundedQueueRigorousTest, BurstyWorkload) {
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
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        threads.emplace_back([this, &total_dequeued]() {
            for (int burst = 0; burst < 10; ++burst) {
                // Burst of dequeues
                for (int i = 0; i < 100; ++i) {
                    queue->deq();
                    total_dequeued++;
                }
                // Small pause
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
TEST_F(BoundedQueueRigorousTest, FairnessTest) {
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
    for (int c = 0; c < NUM_PRODUCERS; ++c) {
        consumers.emplace_back([this, NUM_PRODUCERS, ITEMS_PER_PRODUCER, &items_consumed_from_producer]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                int val = queue->deq();
                int producer_id = val / 1000000;
                items_consumed_from_producer[producer_id]++;
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

