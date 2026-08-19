#pragma once
#include <atomic>
#include <memory>
#include <QVarLengthArray>

// Shared cancellation flag - copies share one flag, so cancelling any cancels all.
// Composed on one thread, polled and cancelled concurrently.
class CancelToken {
public:
    CancelToken() : m_flag(std::make_shared<std::atomic<bool>>(false)) {}

    void cancel() const { m_flag->store(true,  std::memory_order_relaxed); }
    void reset()  const { m_flag->store(false, std::memory_order_relaxed); }

    bool isCancelled() const {
        if (m_flag->load(std::memory_order_relaxed)) return true;
        for (const auto &flag : m_extra)
            if (flag->load(std::memory_order_relaxed)) return true;
        return false;
    }

    // Cancelled when this or `other` is. Chains, so composing twice keeps both sources
    // rather than silently dropping the first.
    CancelToken composeWith(const CancelToken &other) const {
        CancelToken c = *this;
        c.m_extra.append(other.m_flag);
        for (const Flag &flag : other.m_extra) c.m_extra.append(flag);
        return c;
    }

private:
    using Flag = std::shared_ptr<std::atomic<bool>>;
    Flag                     m_flag;
    QVarLengthArray<Flag, 2> m_extra;
};
