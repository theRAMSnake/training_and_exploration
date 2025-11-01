#include <atomic>
#include <chrono>
#include <optional>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <cstdint>

namespace snake {

// Note: we eat 2 bits out of the value range and it is not protected, so care must be taken to not overflow the value range.
template<class T>
class LockFreeExchanger {
    static_assert(std::is_integral_v<T>, "T must be integral");
    static_assert(sizeof(T) <= sizeof(int64_t), "T must be less than or equal to 8 bytes");

    constexpr static int64_t STATE_EMPTY = 0;
    constexpr static int64_t STATE_WAITING = 1;
    constexpr static int64_t STATE_BUSY = 2;

    constexpr static int64_t BITS_POS = 61;
    constexpr static int64_t STATE_MASK = 0b11;

public:
    LockFreeExchanger() : value_and_state_(STATE_EMPTY) {}

    std::optional<T> exchange(T value, std::chrono::nanoseconds timeout) {
        auto run_until = std::chrono::steady_clock::now() + timeout;
        while(true) {
            if(std::chrono::steady_clock::now() > run_until) {
                return {};
            }
            auto current_value_and_state = value_and_state_.load();
            auto current_state = (current_value_and_state >> BITS_POS) & STATE_MASK;
            if(current_state == STATE_EMPTY) {
                // Try to propose an exchange with value and reach waiting state.
                auto proposed_value_and_state = static_cast<int64_t>(value) & ~(STATE_MASK << BITS_POS) | STATE_WAITING << BITS_POS;
                if(value_and_state_.compare_exchange_strong(current_value_and_state, proposed_value_and_state)) {
                    // Run the loop until the timeout or the exchange is successful.
                    while(std::chrono::steady_clock::now() < run_until) {
                        auto other_value_and_state = value_and_state_.load();
                        auto other_state = (other_value_and_state >> BITS_POS) & STATE_MASK;
                        auto other_value = other_value_and_state & ~(STATE_MASK << BITS_POS);
                        if(other_state == STATE_BUSY) {
                            value_and_state_.store(STATE_EMPTY);
                            return other_value;
                        }
                    }
                    // Timed out.
                    if (value_and_state_.compare_exchange_strong(proposed_value_and_state, STATE_EMPTY)) {
                        // Revert back the proposal to the empty state.
                        return {};
                    } else {
                        // If we failed to propose an exchange, means other thread actually proposed an exchange, so let's grab their item
                        // and return it.
                        auto other_value_and_state = value_and_state_.load();
                        auto other_value = other_value_and_state & ~(STATE_MASK << BITS_POS);
                        value_and_state_.store(STATE_EMPTY);
                        return other_value;
                    }
                }
            } else if(current_state == STATE_WAITING) {
                // If the other thread is waiting, we can propose an exchange and reach busy state.
                auto current_value = current_value_and_state & ~(STATE_MASK << BITS_POS);
                auto proposed_value_and_state = static_cast<int64_t>(value) & ~(STATE_MASK << BITS_POS) | STATE_BUSY << BITS_POS;
                if(value_and_state_.compare_exchange_strong(current_value_and_state, proposed_value_and_state)) {
                    return static_cast<T>(current_value);
                }
            } else if(current_state == STATE_BUSY) {
                // Do nothing. The exchange is already in progress by the other thread.
            } else {
                throw std::runtime_error("Invalid state");
            }
        }

    }

private:
    std::atomic<int64_t> value_and_state_;
};

template<class T, int N>
class EliminationArray {
public:
    std::optional<T> visit(T value, std::chrono::nanoseconds timeout) {
        // Use function-local static thread_local variables for thread-local random number generation
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen{rd()};
        static thread_local std::uniform_int_distribution<> dist{0, N - 1};
        
        auto index = dist(gen);
        return exchangers_[index].exchange(value, timeout);
    }

private:
    LockFreeExchanger<T> exchangers_[N];
};
}