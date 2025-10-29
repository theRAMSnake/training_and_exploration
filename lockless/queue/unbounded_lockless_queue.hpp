#include <atomic>
#include <mutex>
#include <condition_variable>
#include <limits>
#include <cstddef>

namespace snake {

// Note, for simplicity this class leaks memory. Todo figure out a way to recycle nodes without using of a shared pointers,
// which adds cost.
template <typename T>
class UnboundedQueue {
private:
    struct Node {
        T value;
        std::atomic<Node*> next = nullptr;
    };

public:
    UnboundedQueue() {
        head_.store(new Node{SENTINEL});
        tail_.store(head_.load());
    }

    void enq(const T& value) {
        auto new_node = new Node{value};
        while(true) {
            auto expected_tail = tail_.load();
            auto next = expected_tail->next.load();
            if(expected_tail == tail_.load()) {
                if(next == nullptr) {
                    if(expected_tail->next.compare_exchange_strong(next, new_node)) {
                        tail_.compare_exchange_strong(expected_tail, new_node);
                        return;
                    }
                } else {
                    tail_.compare_exchange_strong(expected_tail, next);
                }
            }
        }
    }

    T deq() {
        while(true) {
            auto expected_head = head_.load();
            auto expected_tail = tail_.load();
            auto next = expected_head->next.load();
            if(expected_head == head_.load()) {
                if(expected_head == expected_tail) {
                    if(next == nullptr) {
                        throw std::runtime_error("Queue is empty");
                    }
                    tail_.compare_exchange_strong(expected_tail, next);
                } else {
                    auto value = next->value;
                    if(head_.compare_exchange_strong(expected_head, next)) {
                        //delete expected_head;
                        return value;
                    }
                }
            }
        }
    }

private:
    static constexpr int64_t SENTINEL = std::numeric_limits<int32_t>::max();
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
};
}