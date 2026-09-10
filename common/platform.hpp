#pragma once
#include <thread>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  using socket_t = SOCKET;
  constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  using socket_t = int;
  constexpr socket_t INVALID_SOCK = -1;
#endif

inline void pin_thread_to_core(int core_id) {
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1ULL << core_id);
#else
    cpu_set_t set; CPU_ZERO(&set); CPU_SET(core_id, &set);
    pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
#endif
}

inline std::uint64_t now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

inline void init_networking() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    // Windows timer resolution for accurate sleep
    timeBeginPeriod(1);
#endif
}

inline void cleanup_networking() {
#ifdef _WIN32
    timeEndPeriod(1);
    WSACleanup();
#endif
}