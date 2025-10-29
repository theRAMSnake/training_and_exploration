#include <atomic>
#include <mutex>
#include <condition_variable>
#include <limits>
#include <cstddef>

namespace snake {

template <typename T>
class BoundedQueue {
private:
    struct Node {
        T value;
        Node* next = nullptr;
    };

public:
    BoundedQueue(int64_t capacity) : capacity_(capacity) {
        head_ = new Node{SENTINEL};
        tail_ = head_;
        size_.store(0);
    }

    void enq(const T& value) {
        bool must_wake_others = false;
        {
            std::unique_lock<std::mutex> lock(enq_mutex_);
            if(size_.load() >= capacity_) {
                wait_not_full_.wait(lock, [&] { return size_.load() < capacity_; });
            }

            auto new_node = new Node{value};
            tail_->next = new_node;
            tail_ = new_node;
            auto old_size = size_.fetch_add(1);
            if(old_size == 0) {
                must_wake_others = true;
            }
        }
        if(must_wake_others) {
            std::unique_lock<std::mutex> lock(deq_mutex_);
            wait_not_empty_.notify_all();
        }
    }

    T deq() {
        bool must_wake_others = false;
        T value;
        {
            std::unique_lock<std::mutex> lock(deq_mutex_);
            if(head_->next == nullptr) {
                wait_not_empty_.wait(lock, [&] { return size_.load() > 0; });
            }

            auto node = head_->next;
            value = node->value;
            node->value = SENTINEL;
            delete head_;
            head_ = node;

            auto old_size = size_.fetch_sub(1);
            if(old_size == capacity_) {
                must_wake_others = true;
            }
        }
        if(must_wake_others) {
            std::unique_lock<std::mutex> lock(enq_mutex_);
            wait_not_full_.notify_all();
        }
        return value;
    }

private:
    static constexpr int64_t SENTINEL = std::numeric_limits<int32_t>::max();
    Node* head_;
    Node* tail_;
    std::size_t capacity_;
    std::atomic<int64_t> size_;
    std::mutex enq_mutex_;
    std::mutex deq_mutex_;
    std::condition_variable wait_not_full_;
    std::condition_variable wait_not_empty_;
};
}