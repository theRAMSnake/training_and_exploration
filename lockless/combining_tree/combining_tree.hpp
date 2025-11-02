#pragma once

#include <mutex>
#include <condition_variable>
#include <array>
#include <stdexcept>
#include <cassert>
#include <cmath>
#include <new>

namespace snake {

template<int NUM_THREADS>
class CombiningTree {
    struct Node {
        enum class State {
            IDLE,
            FIRST,
            SECOND,
            RESULT,
            ROOT
        };

        State state;
        bool is_locked = false;
        Node* parent = nullptr;

        int first = 0;
        int second = 0;
        int result = 0;
        std::mutex mutex;
        std::condition_variable cv;

        Node() : state(State::ROOT) {}
        Node(Node* parent) : state(State::IDLE), parent(parent) {}

        // Returns true if my thread is the first arriving thread.
        bool precombine() {
            std::unique_lock<std::mutex> lock(mutex);
            while(is_locked) {
                cv.wait(lock);
            }
            switch(state) {
                case State::IDLE:
                    state = State::FIRST;
                    return true;
                case State::FIRST:
                    is_locked = true;
                    state = State::SECOND;
                    return false;
                case State::ROOT:
                    return false;
                default:
                    throw std::runtime_error("Invalid state");
            }
        }

        int combine(const int combined) {
            std::unique_lock<std::mutex> lock(mutex);
            while(is_locked) {
                cv.wait(lock);
            }
            is_locked = true;

            first = combined;
            switch(state) {
                case State::FIRST:
                    return first;
                case State::SECOND:
                    return first + second;
                default:
                    throw std::runtime_error("Invalid state");
            }
        }

        int op(const int combined) {
            std::unique_lock<std::mutex> lock(mutex);
            switch(state) {
                case State::ROOT: {
                    int before = result;
                    result += combined;
                    return before;
                }
                case State::SECOND:
                    second = combined;
                    is_locked = false;
                    cv.notify_all();
                    while(state != State::RESULT) {
                        cv.wait(lock);
                    }
                    is_locked = false;
                    cv.notify_all();
                    state = State::IDLE;
                    return result;
                default:
                    throw std::runtime_error("Invalid state");
            }
        }

        void distribute(const int before) {
            std::unique_lock<std::mutex> lock(mutex);
            switch(state) {
                case State::FIRST:
                    state = State::IDLE;
                    is_locked = false;
                    break;
                case State::SECOND:
                    result = before + first;
                    state = State::RESULT;
                    break;
                default:
                    throw std::runtime_error("Invalid state");
            }
            cv.notify_all();
        }
    };

public:
    CombiningTree() {
        // Initialize root node using placement new
        new (&nodes_[0]) Node();
        // Initialize remaining nodes (children) using placement new
        for(int i = 1; i < 2 * NUM_THREADS - 1; ++i) {
            new (&nodes_[i]) Node(&nodes_[(i - 1) / 2]);
        }
    }
    
    ~CombiningTree() {
        // Explicitly destroy nodes (mutex/condition_variable need proper destruction)
        for(int i = 0; i < 2 * NUM_THREADS - 1; ++i) {
            nodes_[i].~Node();
        }
    }
    
    // Delete copy and move constructors/assignments since nodes contain non-moveable types
    CombiningTree(const CombiningTree&) = delete;
    CombiningTree& operator=(const CombiningTree&) = delete;
    CombiningTree(CombiningTree&&) = delete;
    CombiningTree& operator=(CombiningTree&&) = delete;

    int get_and_increment(const int thread_id) {
        assert(thread_id >= 0 && thread_id < NUM_THREADS);

        int stack_size = 0;
        const int max_stack_depth = static_cast<int>(std::ceil(std::log2(static_cast<double>(NUM_THREADS))));
        Node* stack[max_stack_depth];

        // Precombining phase. Walk up the tree until we reach the root or a thread that is not the first arriving thread.
        Node* node = get_leaf(thread_id);
        while(node->precombine()) {
            node = node->parent;
        }
        Node* stop = node;

        // Combining phase. Walk all the same path as the precombining phase, but accumulate the combined value.
        int combined = 1;
        node = get_leaf(thread_id);
        while(node != stop) {
            combined = node->combine(combined);
            stack[stack_size++] = node;
            node = node->parent;
        }

        // Operation phase.
        int before = stop->op(combined);

        // Distribution phase.
        while(stack_size > 0) {
            node = stack[--stack_size];
            node->distribute(before);
        }

        return before;
    }

private:
    Node* get_leaf(const int thread_id) {
        constexpr int total_nodes = 2 * NUM_THREADS - 1;
        return &nodes_[total_nodes - NUM_THREADS + thread_id];
    }

    std::array<Node, 2 * NUM_THREADS - 1> nodes_;
};

}