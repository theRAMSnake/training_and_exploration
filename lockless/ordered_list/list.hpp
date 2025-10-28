#include <mutex>
#include <limits>
#include <cassert>
#include <functional>
#include <type_traits>
#include <atomic>
#include <tuple>
#include <iostream>

namespace snake {

template<class T>
struct NoHash {
    static int64_t operator()(const T& t) {
        return static_cast<int64_t>(t);
    }
};

// Note, for simplicity this class leaks memory. Todo figure out a way to recycle nodes without using of a shared pointers,
// which adds cost.
template<class T, class Hash = NoHash<T>>
class List {
private:
    // Contains packed node address and removed flag
    template<class U>
    struct AddressAndRemoved {
        static constexpr uintptr_t FLAG_MASK = 1;
        static constexpr uintptr_t PTR_MASK  = ~FLAG_MASK;
        std::atomic<uintptr_t> data_{0};

        static uintptr_t pack(U* ptr, bool removed) noexcept {
            uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
            // assumes alignment >= 2
            return (v & PTR_MASK) | (removed ? FLAG_MASK : 0);
        }

        AddressAndRemoved() = default;
        AddressAndRemoved(U* ptr, bool removed) noexcept {
            data_.store(pack(ptr, removed));
        }

        void store(U* ptr, bool removed) noexcept {
            data_.store(pack(ptr, removed));
        }

        std::pair<U*, bool> load() const noexcept {
            uintptr_t v = data_.load();
            return { reinterpret_cast<U*>(v & PTR_MASK),
                    static_cast<bool>(v & FLAG_MASK) };
        }

        bool compare_exchange(U* expected_ptr, bool expected_flag, U* desired_ptr, bool desired_flag) noexcept {
            uintptr_t exp = pack(expected_ptr, expected_flag);
            uintptr_t des = pack(desired_ptr, desired_flag);
            return data_.compare_exchange_strong(exp, des);
        }
    };
    struct Node {
        T value;
        AddressAndRemoved<Node> next_and_removed;
    };

public:
    List() {
        // We have two sentinel nodes to simplify the algorithm
        head = new Node {SENTINEL_BEGIN};
        auto end_node = new Node {SENTINEL_END};
        head->next_and_removed.store(end_node, false);
        end_node->next_and_removed.store(nullptr, false);
    }

    bool add(const T& value) {
        auto hash = Hash()(value);
        assert(hash != SENTINEL_BEGIN);
        assert(hash != SENTINEL_END);

        while(true) {
            auto [pred, cur] = find(value);
            if (Hash()(cur->value) != hash) {
                auto newNode = new Node {value};
                newNode->next_and_removed.store(cur, false);
                if(pred->next_and_removed.compare_exchange(cur, false, newNode, false)) {
                    return true;
                } else {
                    delete newNode;
                }
            } else {
                return false;
            }
        }
    }

    bool remove(const T& value) {
        auto hash = Hash()(value);
        assert(hash != SENTINEL_BEGIN);
        assert(hash != SENTINEL_END);

        while(true) {
            auto [pred, cur] = find(value);
            if (Hash()(cur->value) == hash) {
                // A mem leak occurs here as we need to delete the node
                auto [cur_next, cur_removed] = cur->next_and_removed.load();
                auto changed = cur->next_and_removed.compare_exchange(cur_next, false, cur_next, true);
                if(changed) {
                    pred->next_and_removed.compare_exchange(cur, false, cur_next, false);
                    return true;
                }
            } else {
                return false;
            }
        }
    }

    /*
     * Check if the value is in the list. Returns true if the value is in the list and false otherwise.
     */
    bool contains(const T& value) {
        auto hash = Hash()(value);
        assert(hash != SENTINEL_BEGIN);
        assert(hash != SENTINEL_END);

        auto cur = head;
        while(Hash()(cur->value) < hash) {
            cur = next(cur);
        }
        return Hash()(cur->value) == hash && !removed(cur);
    }

    void print() {
        auto cur = head;
        while(cur != nullptr) {
            std::cout << "(" << cur->value << ", " << removed(cur) << ") ";
            cur = next(cur);
        }
        std::cout << std::endl;
    }

private:
    Node* next(Node* node) {
        return node->next_and_removed.load().first;
    }

    bool removed(Node* node) {
        return node->next_and_removed.load().second;
    }

    std::tuple<Node*, Node*> find(const T& value) {
        while(true) {
            bool need_restart = false;
            Node* pred = head;
            Node* cur = next(head);
            while(true) {
                auto [cur_next, cur_removed] = cur->next_and_removed.load();
                auto [pred_next, pred_removed] = pred->next_and_removed.load();
                while(cur_removed) {
                    // Check that successor is still valid and remove it otherwise.
                    bool changed = pred->next_and_removed.compare_exchange(
                        pred_next, false, cur_next, false);
                    if(!changed) {
                        need_restart = true;
                        break;
                    } else {
                        cur = cur_next;
                        std::tie(cur_next, cur_removed) = cur->next_and_removed.load();
                    }
                }
                if(need_restart) {
                    break;
                }
                if (Hash()(cur->value) >= value) {
                    return std::make_tuple(pred, cur);
                }
                pred = cur;
                cur = cur_next;
            }
        }
    }

    static constexpr int64_t SENTINEL_BEGIN = std::numeric_limits<int32_t>::min();
    static constexpr int64_t SENTINEL_END = std::numeric_limits<int32_t>::max();

    Node* head;
};

}