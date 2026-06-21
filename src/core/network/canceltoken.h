#pragma once
#include <atomic>
#include <memory>

// Shared cancellation flag - copies share one shared_ptr flag, so cancelling any cancels all.
// Composed on one thread, polled/cancelled concurrently via relaxed atomics.
class CancelToken {
public:
    CancelToken() : m_flag(std::make_shared<std::atomic<bool>>(false)) {}

    void cancel() const { m_flag->store(true,  std::memory_order_relaxed); }
    void reset()  const { m_flag->store(false, std::memory_order_relaxed); }

    bool isCancelled() const {
        return m_flag->load(std::memory_order_relaxed) ||
               (m_secondary && m_secondary->load(std::memory_order_relaxed));
    }

    // A token that is cancelled when either this or `other` is.
    // ponytail: one level of composition (enough for ServerSelector's race);
    // a chain would need a vector of flags.
    CancelToken composeWith(const CancelToken &other) const {
        CancelToken c = *this;
        c.m_secondary = other.m_flag;
        return c;
    }

private:
    std::shared_ptr<std::atomic<bool>> m_flag;
    std::shared_ptr<std::atomic<bool>> m_secondary;  // optional second source
};
