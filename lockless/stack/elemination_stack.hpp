#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <random>
#include <cassert>
#include "lockfree_exchanger.hpp"

namespace snake {

// Note, for simplicity this class leaks memory. Todo figure out a way to recycle nodes without using of a shared pointers,
// which adds cost.
// For simplicity 0 is reseved for elimination array pop token. That value can be easily changed for something else.
template<class T>
class EliminationStack {
private:
    struct Node {
        T value;
        Node* next;
    };

public:
    EliminationStack() : top_(nullptr) {}

    void push(const T& value) {
        assert(value != 0);
        auto new_node = new Node{value};
        while(true) {
            if(try_push(new_node)) {
                return;
            } else {
                auto val = elimination_array_.visit(value, std::chrono::nanoseconds(1000));
                if(val.has_value() && val.value() == 0) {
                    // Exchanged using elimination array.
                    delete new_node;
                    return;
                }
            }
        }
    }

    T pop() {
        while(true) {
            auto node = try_pop();
            if(node != nullptr) {
                return node->value;
            } else {
                auto val = elimination_array_.visit(0, std::chrono::nanoseconds(1000));
                if(val.has_value() && val.value() != 0) {
                    // Exchanged using elimination array.
                    return val.value();
                }
            }
        }
    }

private:
    void random_backoff() {
        std::this_thread::sleep_for(std::chrono::nanoseconds(std::rand() % 1000));
    }

    bool try_push(Node* new_node) {
        auto top = top_.load();
        new_node->next = top;
        return top_.compare_exchange_strong(top, new_node);
    }
    
    Node* try_pop() {
        auto top = top_.load();
        if(top == nullptr) {
            throw std::runtime_error("Stack is empty");
        }
        auto new_top = top->next;
        if(top_.compare_exchange_strong(top, new_top)) {
            return top;
        }
        return nullptr;
    }

    std::atomic<Node*> top_;
    EliminationArray<T, 4> elimination_array_;
};
}