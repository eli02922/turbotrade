#include "common/spsc_queue.hpp"
#include <cassert>
#include <cstdio>
#include <thread>

int main() {
    SPSCQueue<int, 1024> q;
    constexpr int N = 100000;

    std::thread producer([&]{
        for (int i = 0; i < N; ) {
            if (q.push(i)) ++i;
        }
    });

    long long sum = 0;
    for (int i = 0; i < N; ) {
        int v;
        if (q.pop(v)) { sum += v; ++i; }
    }
    producer.join();

    const long long expected = (long long)N * (N - 1) / 2;
    std::printf("sum=%lld expected=%lld\n", sum, expected);
    assert(sum == expected);
    std::printf("PASS\n");
    return 0;
}
