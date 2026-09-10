#pragma once
#include <atomic>
#include <array>
#include <cstddef>
#include <new>

#ifdef _MSC_VER
  #define CACHELINE_ALIGN alignas(64)
#else
  #define CACHELINE_ALIGN alignas(64)
#endif

template <typename T, std::size_t N>
class SPSCQueue {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
    static constexpr std::size_t MASK = N - 1;

    CACHELINE_ALIGN std::atomic<std::size_t> head_{0};
    CACHELINE_ALIGN std::atomic<std::size_t> tail_{0};
    CACHELINE_ALIGN std::array<T, N> buf_{};

public:
    bool push(const T& v) noexcept {
        const auto h = head_.load(std::memory_order_relaxed);
        const auto t = tail_.load(std::memory_order_acquire);
        if ((h - t) >= N) return false;
        buf_[h & MASK] = v;
        head_.store(h + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& out) noexcept {
        const auto t = tail_.load(std::memory_order_relaxed);
        const auto h = head_.load(std::memory_order_acquire);
        if (t == h) return false;
        out = buf_[t & MASK];
        tail_.store(t + 1, std::memory_order_release);
        return true;
    }
};