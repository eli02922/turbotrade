#include "common/market_data.hpp"
#include "common/platform.hpp"
#include "common/spsc_queue.hpp"
#include <cstdio>
#include <thread>
#include <atomic>

static SPSCQueue<MarketUpdate, 1 << 16> g_queue;
static std::atomic<bool> g_running{true};

void consumer_thread() {
    pin_thread_to_core(1);
    MarketUpdate u{};
    std::uint64_t count = 0;
    std::uint64_t total_latency = 0;

    while (g_running.load(std::memory_order_relaxed)) {
        if (g_queue.pop(u)) {
            const auto lat = now_ns() - u.timestamp_ns;
            total_latency += lat;
            ++count;
            if ((count % 10000) == 0) {
                std::printf("consumed %llu, avg latency %llu ns\n",
                            (unsigned long long)count,
                            (unsigned long long)(total_latency / count));
            }
        }
    }
    std::printf("feed_handler: total=%llu avg=%llu ns\n",
                (unsigned long long)count,
                (unsigned long long)(count ? total_latency / count : 0));
}

int main() {
    init_networking();
    pin_thread_to_core(0);

    socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCK) { std::fprintf(stderr, "socket failed\n"); return 1; }

    // Increase recv buffer
    int rcvbuf = 8 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
               reinterpret_cast<const char*>(&rcvbuf), sizeof(rcvbuf));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        return 1;
    }

    std::printf("feed_handler: listening on 0.0.0.0:12345\n");

    std::thread consumer(consumer_thread);

    MarketUpdate u{};
    while (g_running.load(std::memory_order_relaxed)) {
        int n = recvfrom(sock, reinterpret_cast<char*>(&u), sizeof(u), 0, nullptr, nullptr);
        if (n == sizeof(u)) {
            g_queue.push(u);
        }
    }

    consumer.join();
    cleanup_networking();
    return 0;
}