#include "common/market_data.hpp"
#include "common/platform.hpp"
#include <cstdio>
#include <random>
#include <thread>

int main() {
    init_networking();
    pin_thread_to_core(0);

    socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCK) { std::fprintf(stderr, "socket failed\n"); return 1; }

    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port   = htons(12345);
    inet_pton(AF_INET, "239.1.1.1", &dst.sin_addr);

    // Route multicast through loopback on Windows
    inet_pton(AF_INET, "127.0.0.1", &dst.sin_addr);

    std::mt19937 rng(42);
    std::uniform_int_distribution<std::uint32_t> px(9900, 10100);

    MarketUpdate u{};
    u.symbol_id = 1;

    std::printf("feed_simulator: publishing 100k msgs to 127.0.0.1:12345\n");

    for (std::uint64_t i = 0; i < 100'000; ++i) {
        u.timestamp_ns = now_ns();
        u.bid_price = px(rng);
        u.ask_price = u.bid_price + 1;
        u.bid_size  = 100;
        u.ask_size  = 100;

        sendto(sock, reinterpret_cast<const char*>(&u), sizeof(u), 0,
               reinterpret_cast<sockaddr*>(&dst), sizeof(dst));

        if ((i % 10000) == 0) std::printf("sent %llu\n", (unsigned long long)i);
    }

    cleanup_networking();
    return 0;
}