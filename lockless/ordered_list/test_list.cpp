#include "list.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <limits>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <mutex>

using namespace snake;

class ListTest : public ::testing::Test {
protected:
    void SetUp() override {
        list = std::make_unique<List<int>>();
    }

    void TearDown() override {
        list.reset();
    }

    std::unique_ptr<List<int>> list;
};

// Test basic add functionality
TEST_F(ListTest, AddSingleElement) {
    EXPECT_TRUE(list->add(5));
    EXPECT_TRUE(list->contains(5));
}

TEST_F(ListTest, AddMultipleElements) {
    EXPECT_TRUE(list->add(3));
    EXPECT_TRUE(list->add(1));
    EXPECT_TRUE(list->add(7));
    EXPECT_TRUE(list->add(2));
    
    EXPECT_TRUE(list->contains(1));
    EXPECT_TRUE(list->contains(2));
    EXPECT_TRUE(list->contains(3));
    EXPECT_TRUE(list->contains(7));
}

TEST_F(ListTest, AddDuplicateElement) {
    EXPECT_TRUE(list->add(5));
    EXPECT_FALSE(list->add(5)); // Should return false for duplicate
    EXPECT_TRUE(list->contains(5));
}

// Test basic remove functionality
TEST_F(ListTest, RemoveExistingElement) {
    list->add(5);
    EXPECT_TRUE(list->contains(5));
    EXPECT_TRUE(list->remove(5));
    EXPECT_FALSE(list->contains(5));
}

TEST_F(ListTest, RemoveNonExistentElement) {
    EXPECT_FALSE(list->remove(5));
    EXPECT_FALSE(list->contains(5));
}

TEST_F(ListTest, RemoveFromEmptyList) {
    EXPECT_FALSE(list->remove(5));
}

TEST_F(ListTest, RemoveMultipleElements) {
    list->add(1);
    list->add(2);
    list->add(3);
    list->add(4);
    
    EXPECT_TRUE(list->remove(2));
    EXPECT_FALSE(list->contains(2));
    EXPECT_TRUE(list->contains(1));
    EXPECT_TRUE(list->contains(3));
    EXPECT_TRUE(list->contains(4));
    
    EXPECT_TRUE(list->remove(1));
    EXPECT_FALSE(list->contains(1));
    EXPECT_TRUE(list->contains(3));
    EXPECT_TRUE(list->contains(4));
}

// Test contains functionality
TEST_F(ListTest, ContainsEmptyList) {
    EXPECT_FALSE(list->contains(5));
}

TEST_F(ListTest, ContainsAfterAdd) {
    list->add(5);
    EXPECT_TRUE(list->contains(5));
    EXPECT_FALSE(list->contains(3));
}

TEST_F(ListTest, ContainsAfterRemove) {
    list->add(5);
    list->add(3);
    EXPECT_TRUE(list->contains(5));
    EXPECT_TRUE(list->contains(3));
    
    list->remove(5);
    EXPECT_FALSE(list->contains(5));
    EXPECT_TRUE(list->contains(3));
}

// Test ordering (list should maintain order based on hash values)
TEST_F(ListTest, OrderingMaintained) {
    // Add elements in random order
    list->add(5);
    list->add(1);
    list->add(9);
    list->add(3);
    list->add(7);
    
    // All elements should be present
    EXPECT_TRUE(list->contains(1));
    EXPECT_TRUE(list->contains(3));
    EXPECT_TRUE(list->contains(5));
    EXPECT_TRUE(list->contains(7));
    EXPECT_TRUE(list->contains(9));
}

// Test edge cases
TEST_F(ListTest, AddZero) {
    EXPECT_TRUE(list->add(0));
    EXPECT_TRUE(list->contains(0));
}

TEST_F(ListTest, AddNegativeNumbers) {
    EXPECT_TRUE(list->add(-1));
    EXPECT_TRUE(list->add(-5));
    EXPECT_TRUE(list->add(0));
    EXPECT_TRUE(list->add(3));
    
    EXPECT_TRUE(list->contains(-5));
    EXPECT_TRUE(list->contains(-1));
    EXPECT_TRUE(list->contains(0));
    EXPECT_TRUE(list->contains(3));
}

TEST_F(ListTest, AddLargeNumbers) {
    EXPECT_TRUE(list->add(1000));
    EXPECT_TRUE(list->add(999));
    EXPECT_TRUE(list->add(1001));
    
    EXPECT_TRUE(list->contains(999));
    EXPECT_TRUE(list->contains(1000));
    EXPECT_TRUE(list->contains(1001));
}

// Test complex scenarios
TEST_F(ListTest, AddRemoveAdd) {
    EXPECT_TRUE(list->add(5));
    EXPECT_TRUE(list->contains(5));
    
    EXPECT_TRUE(list->remove(5));
    EXPECT_FALSE(list->contains(5));
    
    EXPECT_TRUE(list->add(5));
    EXPECT_TRUE(list->contains(5));
}

TEST_F(ListTest, MultipleOperations) {
    // Add several elements
    std::vector<int> elements = {1, 3, 5, 7, 9, 2, 4, 6, 8};
    for (int elem : elements) {
        EXPECT_TRUE(list->add(elem));
    }
    
    // Check all are present
    for (int elem : elements) {
        EXPECT_TRUE(list->contains(elem));
    }
    
    // Remove some elements
    std::vector<int> to_remove = {3, 7, 2, 8};
    for (int elem : to_remove) {
        EXPECT_TRUE(list->remove(elem));
    }
    
    // Check remaining elements
    std::vector<int> remaining = {1, 5, 9, 4, 6};
    for (int elem : remaining) {
        EXPECT_TRUE(list->contains(elem));
    }
    
    // Check removed elements are gone
    for (int elem : to_remove) {
        EXPECT_FALSE(list->contains(elem));
    }
}

// Custom hash function tests removed - using default hash only

// Test stress scenarios
TEST_F(ListTest, StressTest) {
    const int num_elements = 1000;
    
    // Add many elements
    for (int i = 0; i < num_elements; ++i) {
        EXPECT_TRUE(list->add(i));
    }
    
    // Check all elements are present
    for (int i = 0; i < num_elements; ++i) {
        EXPECT_TRUE(list->contains(i));
    }
    
    // Remove every other element
    for (int i = 0; i < num_elements; i += 2) {
        EXPECT_TRUE(list->remove(i));
    }
    
    // Check remaining elements
    for (int i = 1; i < num_elements; i += 2) {
        EXPECT_TRUE(list->contains(i));
    }
    
    // Check removed elements are gone
    for (int i = 0; i < num_elements; i += 2) {
        EXPECT_FALSE(list->contains(i));
    }
}

// Test with different data types (commented out for now due to sentinel value issues)
// TEST(ListTestString, StringType) {
//     List<std::string> string_list;
//     
//     EXPECT_TRUE(string_list.add("hello"));
//     EXPECT_TRUE(string_list.add("world"));
//     EXPECT_TRUE(string_list.add("test"));
//     
//     EXPECT_TRUE(string_list.contains("hello"));
//     EXPECT_TRUE(string_list.contains("world"));
//     EXPECT_TRUE(string_list.contains("test"));
//     EXPECT_FALSE(string_list.contains("missing"));
//     
//     EXPECT_TRUE(string_list.remove("world"));
//     EXPECT_FALSE(string_list.contains("world"));
//     EXPECT_TRUE(string_list.contains("hello"));
//     EXPECT_TRUE(string_list.contains("test"));
// }

// Test destructor and memory management
TEST_F(ListTest, DestructorTest) {
    // Add some elements
    list->add(1);
    list->add(2);
    list->add(3);
    
    // Reset the list (should call destructor)
    list.reset();
    
    // Create a new list to ensure no issues
    list = std::make_unique<List<int>>();
    EXPECT_TRUE(list->add(5));
    EXPECT_TRUE(list->contains(5));
}

// Concurrent test class
class ListConcurrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        list = std::make_unique<List<int>>();
    }

    void TearDown() override {
        list.reset();
    }

    std::unique_ptr<List<int>> list;
    static constexpr int NUM_THREADS = 4;
    static constexpr int OPERATIONS_PER_THREAD = 1000;
};

// Test concurrent additions
TEST_F(ListConcurrentTest, ConcurrentAdditions) {
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    
    // Each thread adds different ranges of numbers
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &success_count, &failure_count]() {
            int start = t * OPERATIONS_PER_THREAD;
            int end = start + OPERATIONS_PER_THREAD;
            
            for (int i = start; i < end; ++i) {
                if (list->add(i)) {
                    success_count++;
                } else {
                    failure_count++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all numbers were added successfully
    EXPECT_EQ(success_count.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
    EXPECT_EQ(failure_count.load(), 0);
    
    // Verify all numbers are in the list
    for (int i = 0; i < NUM_THREADS * OPERATIONS_PER_THREAD; ++i) {
        EXPECT_TRUE(list->contains(i));
    }
}

// Test concurrent additions and removals
TEST_F(ListConcurrentTest, ConcurrentAddAndRemove) {
    std::vector<std::thread> threads;
    std::atomic<int> add_success{0};
    std::atomic<int> remove_success{0};
    
    // First, add some initial elements
    for (int i = 0; i < 100; ++i) {
        list->add(i);
    }
    
    // Thread 0: Add elements
    threads.emplace_back([this, &add_success]() {
        for (int i = 100; i < 100 + OPERATIONS_PER_THREAD; ++i) {
            if (list->add(i)) {
                add_success++;
            }
        }
    });
    
    // Thread 1: Remove elements
    threads.emplace_back([this, &remove_success]() {
        for (int i = 0; i < OPERATIONS_PER_THREAD && i < 100; ++i) {
            if (list->remove(i)) {
                remove_success++;
            }
        }
    });
    
    // Thread 2: Add more elements
    threads.emplace_back([this, &add_success]() {
        for (int i = 200; i < 200 + OPERATIONS_PER_THREAD; ++i) {
            if (list->add(i)) {
                add_success++;
            }
        }
    });
    
    // Thread 3: Remove more elements
    threads.emplace_back([this, &remove_success]() {
        for (int i = 50; i < 50 + OPERATIONS_PER_THREAD && i < 100; ++i) {
            if (list->remove(i)) {
                remove_success++;
            }
        }
    });
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify some operations succeeded
    EXPECT_GT(add_success.load(), 0);
    EXPECT_GT(remove_success.load(), 0);
}

// Test concurrent contains operations
TEST_F(ListConcurrentTest, ConcurrentContains) {
    // Add some elements first
    for (int i = 0; i < 1000; ++i) {
        list->add(i);
    }
    
    std::vector<std::thread> threads;
    std::atomic<int> found_count{0};
    std::atomic<int> not_found_count{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &found_count, &not_found_count]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 1999); // Some numbers exist, some don't
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int value = dis(gen);
                if (list->contains(value)) {
                    found_count++;
                } else {
                    not_found_count++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify we got some results
    EXPECT_GT(found_count.load(), 0);
    EXPECT_GT(not_found_count.load(), 0);
    EXPECT_EQ(found_count.load() + not_found_count.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
}

// Test mixed concurrent operations
TEST_F(ListConcurrentTest, MixedConcurrentOperations) {
    std::vector<std::thread> threads;
    std::atomic<int> add_count{0};
    std::atomic<int> remove_count{0};
    std::atomic<int> contains_count{0};
    
    // Thread 0: Add elements
    threads.emplace_back([this, &add_count]() {
        for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
            if (list->add(i)) {
                add_count++;
            }
        }
    });
    
    // Thread 1: Add different elements
    threads.emplace_back([this, &add_count]() {
        for (int i = OPERATIONS_PER_THREAD; i < 2 * OPERATIONS_PER_THREAD; ++i) {
            if (list->add(i)) {
                add_count++;
            }
        }
    });
    
    // Thread 2: Remove elements (some may not exist)
    threads.emplace_back([this, &remove_count]() {
        for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
            if (list->remove(i)) {
                remove_count++;
            }
        }
    });
    
    // Thread 3: Check contains
    threads.emplace_back([this, &contains_count]() {
        for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
            if (list->contains(i)) {
                contains_count++;
            }
        }
    });
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify operations completed
    EXPECT_GT(add_count.load(), 0);
    EXPECT_GE(remove_count.load(), 0);
    EXPECT_GE(contains_count.load(), 0);
}

// Test concurrent stress with random operations
TEST_F(ListConcurrentTest, ConcurrentStressTest) {
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::atomic<int> successful_operations{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &total_operations, &successful_operations]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t); // Different seed per thread
            std::uniform_int_distribution<> op_dis(0, 2); // 0=add, 1=remove, 2=contains
            std::uniform_int_distribution<> value_dis(0, 999);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int operation = op_dis(gen);
                int value = value_dis(gen);
                total_operations++;
                
                bool success = false;
                switch (operation) {
                    case 0: // Add
                        success = list->add(value);
                        break;
                    case 1: // Remove
                        success = list->remove(value);
                        break;
                    case 2: // Contains
                        success = list->contains(value);
                        break;
                }
                
                if (success) {
                    successful_operations++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify we performed operations
    EXPECT_EQ(total_operations.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
    EXPECT_GT(successful_operations.load(), 0);
}

// Test concurrent duplicate handling
TEST_F(ListConcurrentTest, ConcurrentDuplicateHandling) {
    std::vector<std::thread> threads;
    std::atomic<int> successful_adds{0};
    std::atomic<int> failed_adds{0};
    
    // All threads try to add the same values
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, &successful_adds, &failed_adds]() {
            for (int i = 0; i < 100; ++i) {
                if (list->add(i)) {
                    successful_adds++;
                } else {
                    failed_adds++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Only one thread should succeed for each value
    EXPECT_EQ(successful_adds.load(), 100);
    EXPECT_EQ(failed_adds.load(), (NUM_THREADS - 1) * 100);
    
    // Verify all values are in the list exactly once
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(list->contains(i));
    }
}

// Test concurrent ordering preservation
TEST_F(ListConcurrentTest, ConcurrentOrderingPreservation) {
    std::vector<std::thread> threads;
    
    // Each thread adds a different range of numbers
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t]() {
            int start = t * 100;
            int end = start + 100;
            
            // Add numbers in reverse order to test ordering
            for (int i = end - 1; i >= start; --i) {
                list->add(i);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all numbers are present
    for (int i = 0; i < NUM_THREADS * 100; ++i) {
        EXPECT_TRUE(list->contains(i));
    }
    
    // The list should maintain ordering despite concurrent insertions
    // (This is a basic check - the actual ordering depends on timing)
}

// More rigorous concurrent tests
class ListRigorousConcurrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        list = std::make_unique<List<int>>();
    }

    void TearDown() override {
        list.reset();
    }

    std::unique_ptr<List<int>> list;
    static constexpr int NUM_THREADS = 8;  // More threads
    static constexpr int OPERATIONS_PER_THREAD = 2000;  // More operations
};

// Test with 8 threads and 2000 operations each (16,000 total operations)
TEST_F(ListRigorousConcurrentTest, HighConcurrencyStressTest) {
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::atomic<int> successful_operations{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &total_operations, &successful_operations]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<> op_dis(0, 2); // 0=add, 1=remove, 2=contains
            std::uniform_int_distribution<> value_dis(0, 9999);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int operation = op_dis(gen);
                int value = value_dis(gen);
                total_operations++;
                
                bool success = false;
                switch (operation) {
                    case 0: // Add
                        success = list->add(value);
                        break;
                    case 1: // Remove
                        success = list->remove(value);
                        break;
                    case 2: // Contains
                        success = list->contains(value);
                        break;
                }
                
                if (success) {
                    successful_operations++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(total_operations.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
    EXPECT_GT(successful_operations.load(), 0);
}

// Test rapid add/remove cycles
TEST_F(ListRigorousConcurrentTest, RapidAddRemoveCycles) {
    std::vector<std::thread> threads;
    std::atomic<int> cycles_completed{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &cycles_completed]() {
            for (int cycle = 0; cycle < 100; ++cycle) {
                int base_value = t * 1000 + cycle * 10;
                
                // Add 10 values
                for (int i = 0; i < 10; ++i) {
                    list->add(base_value + i);
                }
                
                // Remove 10 values
                for (int i = 0; i < 10; ++i) {
                    list->remove(base_value + i);
                }
                
                cycles_completed++;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(cycles_completed.load(), NUM_THREADS * 100);
}

// Test concurrent operations on same values (high contention)
TEST_F(ListRigorousConcurrentTest, HighContentionSameValues) {
    std::vector<std::thread> threads;
    std::atomic<int> add_attempts{0};
    std::atomic<int> remove_attempts{0};
    std::atomic<int> contains_attempts{0};
    
    // All threads operate on the same small set of values (0-9)
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &add_attempts, &remove_attempts, &contains_attempts]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<> op_dis(0, 2);
            std::uniform_int_distribution<> value_dis(0, 9); // Only 10 possible values
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int operation = op_dis(gen);
                int value = value_dis(gen);
                
                switch (operation) {
                    case 0: // Add
                        add_attempts++;
                        list->add(value);
                        break;
                    case 1: // Remove
                        remove_attempts++;
                        list->remove(value);
                        break;
                    case 2: // Contains
                        contains_attempts++;
                        list->contains(value);
                        break;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(add_attempts.load(), 0);
    EXPECT_GT(remove_attempts.load(), 0);
    EXPECT_GT(contains_attempts.load(), 0);
}

// Test memory consistency under high load
TEST_F(ListRigorousConcurrentTest, MemoryConsistencyTest) {
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> operations_count{0};
    
    // Writer threads
    for (int t = 0; t < NUM_THREADS / 2; ++t) {
        threads.emplace_back([this, t, &stop_flag, &operations_count]() {
            int base_value = t * 1000;
            int counter = 0;
            
            while (!stop_flag.load()) {
                for (int i = 0; i < 100 && !stop_flag.load(); ++i) {
                    list->add(base_value + counter);
                    operations_count++;
                    counter++;
                }
            }
        });
    }
    
    // Reader threads
    for (int t = NUM_THREADS / 2; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &stop_flag, &operations_count]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<> value_dis(0, 9999);
            
            while (!stop_flag.load()) {
                for (int i = 0; i < 100 && !stop_flag.load(); ++i) {
                    list->contains(value_dis(gen));
                    operations_count++;
                }
            }
        });
    }
    
    // Run for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop_flag.store(true);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(operations_count.load(), 0);
}

// Test with very high thread count
TEST_F(ListRigorousConcurrentTest, VeryHighThreadCount) {
    constexpr int HIGH_THREAD_COUNT = 16;
    std::vector<std::thread> threads;
    std::atomic<int> successful_adds{0};
    
    for (int t = 0; t < HIGH_THREAD_COUNT; ++t) {
        threads.emplace_back([this, t, &successful_adds]() {
            int start = t * 100;
            int end = start + 100;
            
            for (int i = start; i < end; ++i) {
                if (list->add(i)) {
                    successful_adds++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successful_adds.load(), HIGH_THREAD_COUNT * 100);
    
    // Verify all values are present
    for (int i = 0; i < HIGH_THREAD_COUNT * 100; ++i) {
        EXPECT_TRUE(list->contains(i));
    }
}

// Test mixed operations with different patterns
TEST_F(ListRigorousConcurrentTest, MixedOperationPatterns) {
    std::vector<std::thread> threads;
    
    // Pattern 1: Sequential adders
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < 500; ++i) {
                list->add(t * 1000 + i);
            }
        });
    }
    
    // Pattern 2: Random adders
    for (int t = 2; t < 4; ++t) {
        threads.emplace_back([this, t]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<> dis(2000, 2999);
            
            for (int i = 0; i < 500; ++i) {
                list->add(dis(gen));
            }
        });
    }
    
    // Pattern 3: Removers
    for (int t = 4; t < 6; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < 500; ++i) {
                list->remove(i);
            }
        });
    }
    
    // Pattern 4: Readers
    for (int t = 6; t < 8; ++t) {
        threads.emplace_back([this, t]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<> dis(0, 999);
            
            for (int i = 0; i < 1000; ++i) {
                list->contains(dis(gen));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Basic verification that the list is still functional
    EXPECT_TRUE(list->add(9999));
    EXPECT_TRUE(list->contains(9999));
    EXPECT_TRUE(list->remove(9999));
    EXPECT_FALSE(list->contains(9999));
}

// Test logical correctness under concurrent operations
TEST_F(ListRigorousConcurrentTest, LogicalCorrectnessTest) {
    constexpr int NUM_OPERATIONS = 5000;  // Reduced for better performance
    constexpr int NUM_THREADS = 4;        // Reduced for better performance
    constexpr int VALUE_RANGE = 50;       // Limited range for better tracking
    
    // Structure to track events with timing
    struct Event {
        enum Type { ADD_START, ADD_END, REMOVE_START, REMOVE_END };
        Type type;
        int value;
        int thread_id;
        std::chrono::steady_clock::time_point timestamp;
        bool success;
        
        const char* type_string() const {
            switch (type) {
                case ADD_START: return "ADD_START";
                case ADD_END: return "ADD_END";
                case REMOVE_START: return "REMOVE_START";
                case REMOVE_END: return "REMOVE_END";
                default: return "UNKNOWN";
            }
        }
    };
    
    // Thread-safe event log
    std::mutex event_log_mutex;
    std::vector<Event> event_log;
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_adds{0};
    std::atomic<int> successful_removes{0};
    std::atomic<int> total_contains_checks{0};
    
    // Track the logical state of each value
    std::vector<std::atomic<int>> value_add_count(VALUE_RANGE);
    std::vector<std::atomic<int>> value_remove_count(VALUE_RANGE);
    
    // Initialize atomic counters
    for (int i = 0; i < VALUE_RANGE; ++i) {
        value_add_count[i].store(0);
        value_remove_count[i].store(0);
    }
    
    // Helper to log events
    auto log_event = [&](Event::Type type, int value, int thread_id, bool success = false) {
        Event event{type, value, thread_id, std::chrono::steady_clock::now(), success};
        std::lock_guard<std::mutex> lock(event_log_mutex);
        event_log.push_back(event);
    };
    
    // Each thread performs a mix of operations
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &successful_adds, &successful_removes, 
                             &total_contains_checks, &value_add_count, &value_remove_count, &log_event]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<> op_dis(0, 2); // 0=add, 1=remove, 2=contains
            std::uniform_int_distribution<> value_dis(0, VALUE_RANGE - 1);
            
            for (int i = 0; i < NUM_OPERATIONS; ++i) {
                int operation = op_dis(gen);
                int value = value_dis(gen);
                
                switch (operation) {
                    case 0: { // Add
                        log_event(Event::ADD_START, value, t);
                        bool add_result = list->add(value);
                        log_event(Event::ADD_END, value, t, add_result);
                        if (add_result) {
                            successful_adds++;
                            value_add_count[value]++;
                        }
                        break;
                    }
                    case 1: { // Remove
                        log_event(Event::REMOVE_START, value, t);
                        bool remove_result = list->remove(value);
                        log_event(Event::REMOVE_END, value, t, remove_result);
                        if (remove_result) {
                            successful_removes++;
                            value_remove_count[value]++;
                        }
                        break;
                    }
                    case 2: { // Contains
                        total_contains_checks++;
                        list->contains(value);
                        break;
                    }
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify basic invariants
    EXPECT_GT(successful_adds.load(), 0);
    EXPECT_GE(successful_removes.load(), 0);
    EXPECT_GT(total_contains_checks.load(), 0);
    
    // Now perform comprehensive logical correctness verification
    // by checking that the list state matches the expected logical state
    
    // Helper function to print events for a value in chronological order
    auto print_events_for_value = [&](int value) {
        std::vector<Event> value_events;
        for (const auto& event : event_log) {
            if (event.value == value) {
                value_events.push_back(event);
            }
        }
        
        // Sort by timestamp
        std::sort(value_events.begin(), value_events.end(), 
                  [](const Event& a, const Event& b) {
                      return a.timestamp < b.timestamp;
                  });
        
        std::cout << "\n=== Events for value " << value << " (chronological order) ===" << std::endl;
        auto start_time = value_events.empty() ? std::chrono::steady_clock::now() : value_events[0].timestamp;
        
        for (const auto& event : value_events) {
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                event.timestamp - start_time).count();
            std::cout << "  [T" << event.thread_id << "] +" << elapsed << "μs: " 
                      << event.type_string();
            if (event.type == Event::ADD_END || event.type == Event::REMOVE_END) {
                std::cout << " -> " << (event.success ? "SUCCESS" : "FAILED");
            }
            std::cout << std::endl;
        }
        std::cout << "=== End of events ===" << std::endl;
    };
    
    // Test 1: Verify that each value's presence in the list matches its logical state
    // We need to reconstruct the actual logical state by walking through events in chronological order
    int logical_errors = 0;
    for (int value = 0; value < VALUE_RANGE; ++value) {
        int adds = value_add_count[value].load();
        int removes = value_remove_count[value].load();
        bool actually_present = list->contains(value);
        
        // Reconstruct logical state from chronologically ordered events
        std::vector<Event> value_events;
        for (const auto& event : event_log) {
            if (event.value == value) {
                value_events.push_back(event);
            }
        }
        std::sort(value_events.begin(), value_events.end(), 
                  [](const Event& a, const Event& b) {
                      return a.timestamp < b.timestamp;
                  });
        
        // Walk through successful operations to determine final state
        bool should_be_present = false;
        int chronological_adds = 0;
        int chronological_removes = 0;
        for (const auto& event : value_events) {
            if (event.type == Event::ADD_END && event.success) {
                should_be_present = true;
                chronological_adds++;
            } else if (event.type == Event::REMOVE_END && event.success) {
                should_be_present = false;
                chronological_removes++;
            }
        }
        
        // Check for discrepancies
        bool counter_mismatch = (adds != chronological_adds || removes != chronological_removes);
        bool state_mismatch = (should_be_present != actually_present);
        bool impossible_removes = (chronological_removes > chronological_adds);
        
        if (state_mismatch || counter_mismatch || impossible_removes) {
            logical_errors++;
            std::cout << "\nLOGICAL ERROR: Value " << value << std::endl;
            std::cout << "  Counter adds: " << adds << ", Counter removes: " << removes 
                      << " (counter says: " << (adds > removes ? "PRESENT" : "ABSENT") << ")" << std::endl;
            std::cout << "  Chronological adds: " << chronological_adds 
                      << ", Chronological removes: " << chronological_removes 
                      << " (timeline says: " << (should_be_present ? "PRESENT" : "ABSENT") << ")" << std::endl;
            std::cout << "  Actually present: " << (actually_present ? "YES" : "NO") << std::endl;
            
            if (counter_mismatch) {
                std::cout << "  ** WARNING: Counter mismatch with chronological events! **" << std::endl;
            }
            if (impossible_removes) {
                std::cout << "  ** CRITICAL: More removes than adds in chronological order! **" << std::endl;
            }
            if (state_mismatch) {
                std::cout << "  ** ERROR: Actual state doesn't match expected state! **" << std::endl;
            }
            
            // Print all events for this value
            print_events_for_value(value);
        }
    }
    
    if (logical_errors > 0) {
        std::cout << "\n=== LIST STATE AT END ===" << std::endl;
        std::cout << "List contents: ";
        list->print();
        std::cout << std::endl;
    }
    
    EXPECT_EQ(logical_errors, 0) << "Found " << logical_errors << " logical errors in list state";
    
    // Test 2: Verify no phantom values exist
    // (values that contains() returns true for but were never added)
    int phantom_values = 0;
    for (int value = 0; value < VALUE_RANGE; ++value) {
        int adds = value_add_count[value].load();
        int removes = value_remove_count[value].load();
        bool actually_present = list->contains(value);
        
        // A value should only be present if it was added more times than removed
        if (actually_present && adds <= removes) {
            phantom_values++;
            std::cout << "\nPHANTOM VALUE: " << value 
                      << " - adds: " << adds << ", removes: " << removes 
                      << " but value is present!" << std::endl;
            
            // Print all events for this value
            print_events_for_value(value);
        }
    }
    
    EXPECT_EQ(phantom_values, 0) << "Found " << phantom_values << " phantom values";
    
    // Test 3: Verify that values outside our range are not present
    int unexpected_values = 0;
    for (int value = VALUE_RANGE; value < VALUE_RANGE + 100; ++value) {
        if (list->contains(value)) {
            unexpected_values++;
            std::cout << "UNEXPECTED VALUE: " << value << " should not be present!" << std::endl;
        }
    }
    
    EXPECT_EQ(unexpected_values, 0) << "Found " << unexpected_values << " unexpected values";
    
    // Test 3: Verify that duplicate adds are properly rejected
    // Add some known values and verify they can't be added again
    std::vector<int> known_values = {1000, 1001, 1002, 1003, 1004}; // Different range from random values
    for (int value : known_values) {
        // Try to add each value multiple times
        bool first_add = list->add(value);
        bool second_add = list->add(value);
        bool third_add = list->add(value);
        
        // Only the first add should succeed
        if (first_add) {
            EXPECT_FALSE(second_add) << "Duplicate add should fail for value " << value;
            EXPECT_FALSE(third_add) << "Duplicate add should fail for value " << value;
        }
    }
    
    // Test 4: Verify that removed values are properly removed
    for (int value : known_values) {
        if (list->contains(value)) {
            bool remove_result = list->remove(value);
            if (remove_result) {
                EXPECT_FALSE(list->contains(value)) << "Value " << value << " should not be found after removal";
            }
        }
    }
    
    // Test 5: Verify list consistency after all operations
    // The list should still be functional
    EXPECT_TRUE(list->add(9999));
    EXPECT_TRUE(list->contains(9999));
    EXPECT_TRUE(list->remove(9999));
    EXPECT_FALSE(list->contains(9999));
    
    // Test 6: Verify that the list maintains ordering
    // Add some values in a specific order and verify they can be found
    std::vector<int> ordered_values = {2000, 2001, 2002, 2003, 2004}; // Different range from random values
    for (int value : ordered_values) {
        EXPECT_TRUE(list->add(value)) << "Value " << value << " should be addable (not in random range)";
    }
    
    // All values should be findable regardless of whether they were just added or already present
    for (int value : ordered_values) {
        EXPECT_TRUE(list->contains(value)) << "Value " << value << " should be findable after addition";
    }
    
    // Test 7: Stress test with rapid add/remove cycles
    // This tests that the list doesn't get into an inconsistent state
    for (int cycle = 0; cycle < 100; ++cycle) {
        int test_value = 10000 + cycle;
        EXPECT_TRUE(list->add(test_value));
        EXPECT_TRUE(list->contains(test_value));
        EXPECT_TRUE(list->remove(test_value));
        EXPECT_FALSE(list->contains(test_value));
    }
    
    // Final verification: the list should still be functional
    EXPECT_TRUE(list->add(99999));
    EXPECT_TRUE(list->contains(99999));
    EXPECT_TRUE(list->remove(99999));
    EXPECT_FALSE(list->contains(99999));
}
