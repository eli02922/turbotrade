# TurboTrade

A low-latency C++20 message pipeline for market data ingestion and strategy execution, built from scratch using Windows/MSVC, CMake, and Ninja.

> **Status:** Work in progress. The feed pipeline (UDP ingestion → lock-free SPSC queue → latency measurement) is functional. The strategy engine, order gateway, and mock exchange are currently stubs under active development.

---

## Overview

TurboTrade is a hands-on exploration of low-latency systems engineering, targeting the skill set required for high-frequency trading infrastructure.

Key areas include:

* Lock-free data structures with cache-line alignment and tuned memory ordering
* Zero-allocation hot paths
* Nanosecond-resolution latency measurement
* End-to-end pipeline: network → parser → queue → strategy → gateway
* Cross-platform builds using CMake
* Windows/MSVC and Linux/GCC support
* Performance-oriented system design

The project is built incrementally: each component is implemented, measured, tested, and optimized before moving on to the next stage.

---

## Architecture

```text
┌──────────────────┐       UDP       ┌──────────────────┐
│ Feed Simulator   │ ──────────────► │  Feed Handler    │
│ (market data)   │                 │ (lock-free SPSC) │
└──────────────────┘                 └────────┬─────────┘
                                              │
                                              ▼
                                     ┌──────────────────┐
                                     │ Strategy Engine  │
                                     │  (market maker)  │
                                     │      [stub]      │
                                     └────────┬─────────┘
                                              │
                                              ▼
                                     ┌──────────────────┐
                                     │  Order Gateway   │
                                     │  (TCP / FIX-lite)│
                                     │      [stub]      │
                                     └────────┬─────────┘
                                              │
                                              ▼
                                     ┌──────────────────┐
                                     │  Mock Exchange   │
                                     │      [stub]      │
                                     └──────────────────┘
```

---

## Current Status

| Component                | Status     | Description                                                                     |
| ------------------------ | ---------- | ------------------------------------------------------------------------------- |
| `common/spsc_queue.hpp`  | ✅ Complete | Lock-free single-producer/single-consumer queue with cache-line-aligned atomics |
| `common/platform.hpp`    | ✅ Complete | Cross-platform thread pinning and socket utilities                              |
| `common/market_data.hpp` | ✅ Complete | Packed market data structure                                                    |
| `feed_simulator`         | ✅ Complete | Publishes synthetic market updates over UDP                                     |
| `feed_handler`           | ✅ Complete | Receives UDP packets, pushes data to the SPSC queue, and measures latency       |
| `tests/test_spsc`        | ✅ Passing  | Concurrent correctness test for the SPSC queue                                  |
| `strategy_engine`        | 🚧 Stub    | Market-making strategy                                                          |
| `order_gateway`          | 🚧 Stub    | TCP order sender                                                                |
| `mock_exchange`          | 🚧 Stub    | Order acknowledgement server                                                    |
| `latency_monitor`        | 📋 Planned | HdrHistogram-based p50/p99/p999 latency tracking                                |

---

## Requirements

### Windows

* Visual Studio 2022 (Community or Build Tools)
* CMake 3.20+
* Ninja (optional)
* Windows SDK

### Linux

* GCC 12+ or Clang 15+
* CMake 3.20+
* Ninja

### Language

* C++20

---

## Building

### Windows — Visual Studio Generator

This is the recommended configuration and works from a standard PowerShell window.

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Binaries are placed under:

```text
build\Release\
```

### Windows — Ninja

Ninja provides faster incremental builds.

Use the **Developer PowerShell for VS 2022** so that `cl.exe` is available on the PATH.

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binaries are placed under:

```text
build\
```

### Linux

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Running

### 1. Run the SPSC Queue Test

#### Windows

```powershell
.\build\Release\test_spsc.exe
```

Expected output:

```text
sum=4999950000 expected=4999950000
PASS
```

---

### 2. Run the Feed Pipeline

Open two PowerShell windows from the project root.

#### Window 1 — Start the Consumer

Start the feed handler first:

```powershell
.\build\Release\feed_handler.exe
```

Expected output:

```text
feed_handler: listening on 0.0.0.0:12345
consumed 10000, avg latency 4321 ns
consumed 20000, avg latency 3987 ns
...
```

#### Window 2 — Start the Producer

Start the feed simulator:

```powershell
.\build\Release\feed_simulator.exe
```

Expected output:

```text
feed_simulator: publishing 100k msgs to 127.0.0.1:12345
sent 0
sent 10000
sent 20000
...
sent 90000
```

The average latency reported by `feed_handler` represents the measured one-way latency between the feed simulator and feed handler.

> **Note:** Actual latency values depend on CPU, operating system scheduling, network stack behavior, system load, and whether the application is running in Debug or Release mode.

---

### 3. Run CTest

From the build directory:

```powershell
cd build
ctest --output-on-failure -C Release
```

On Linux:

```bash
cd build
ctest --output-on-failure
```

---

## Project Structure

```text
turbotrade/
├── CMakeLists.txt                  # Root build configuration
│
├── common/
│   ├── spsc_queue.hpp              # Lock-free SPSC ring buffer
│   ├── market_data.hpp             # Packed market data structure
│   └── platform.hpp                # Cross-platform abstractions
│
├── feed_simulator/
│   └── main.cpp                    # UDP market data publisher
│
├── feed_handler/
│   └── main.cpp                    # UDP receiver + queue producer
│
├── strategy/
│   └── main.cpp                    # Market-making strategy (stub)
│
├── order_gateway/
│   └── main.cpp                    # TCP order sender (stub)
│
├── mock_exchange/
│   └── main.cpp                    # Order acknowledgement server (stub)
│
├── latency_monitor/
│   └── main.cpp                    # Percentile latency tracking (stub)
│
├── tests/
│   ├── CMakeLists.txt
│   └── test_spsc.cpp               # SPSC queue correctness test
│
├── benchmarks/                     # Google Benchmark harness (planned)
├── docs/                           # Architecture, benchmarks, debugging notes
├── .github/workflows/              # CI configuration (planned)
├── .gitignore
└── README.md
```

---

## Design Notes

### Why a Lock-Free SPSC Queue?

The feed handler and strategy engine are designed to run on dedicated cores. Passing market data between them through a mutex can introduce contention, scheduling overhead, and unpredictable tail latency.

An SPSC ring buffer with acquire/release memory ordering provides a predictable communication mechanism between a single producer and a single consumer.

Key characteristics:

* **No locks** — avoids mutex contention on the hot path
* **No dynamic allocation** — the ring buffer is allocated once
* **Cache-line alignment** — producer and consumer indices are separated to reduce false sharing
* **Power-of-two capacity** — allows efficient index calculation using a bitmask
* **Fixed-size storage** — predictable memory usage

---

## Memory Ordering

The queue uses C++ atomic memory ordering to establish the required synchronization between producer and consumer.

The producer publishes an updated `head` using release semantics:

```cpp
head_.store(next_head, std::memory_order_release);
```

The consumer observes the published index using acquire semantics:

```cpp
head_.load(std::memory_order_acquire);
```

This establishes a happens-before relationship between writing an element into the buffer and publishing the updated index.

Conceptually:

```text
Producer                         Consumer
   │                                │
   │ write buffer element           │
   │                                │
   │ head.store(release)            │
   │ ─────────────────────────────► │
   │                                │ head.load(acquire)
   │                                │
   │                                │ read buffer element
```

Using weaker ordering where synchronization is required could allow the consumer to observe the index update before the corresponding buffer write is safely visible.

---

## Performance Goals

The project focuses on predictable latency rather than raw throughput alone.

Future benchmarks will measure:

* UDP ingestion latency
* Queue enqueue/dequeue latency
* Strategy processing latency
* Order gateway latency
* End-to-end market-data-to-order latency
* p50 latency
* p95 latency
* p99 latency
* p99.9 latency
* Throughput under sustained load
* CPU utilization
* Memory allocation behavior

---

## Roadmap

* [x] Lock-free SPSC queue with unit test
* [x] UDP feed simulator
* [x] UDP feed handler
* [x] Basic latency measurement
* [ ] HdrHistogram latency monitor
* [ ] p50 / p99 / p999 latency reporting
* [ ] Market-making strategy
* [ ] TCP order gateway with FIX-lite protocol
* [ ] Mock exchange with fill simulation
* [ ] Order book reconstruction
* [ ] Position tracking
* [ ] Risk limits
* [ ] Google Benchmark suite
* [ ] GitHub Actions CI
* [ ] Sanitizer builds
* [ ] Chaos testing with injected packet loss and delay
* [ ] Grafana real-time latency dashboard
* [ ] Deterministic replay engine

---

## Honest Scope Statement

TurboTrade is a learning and portfolio project focused on low-latency systems engineering.

It does **not** connect to any real exchange, handle real money, or currently implement the risk controls required for production trading systems.

The goal is to demonstrate practical understanding of:

* C++20 systems programming
* UDP networking
* Lock-free data structures
* Atomic memory ordering
* Cache-aware programming
* Zero-allocation hot paths
* Low-latency architecture
* Performance measurement
* Concurrent programming
* Build systems and cross-platform development

The project intentionally prioritizes **working, measurable implementations** over simply documenting theoretical concepts.

---

## License

MIT License

---

## Author

**Eleazar G. Embuscado**

Senior Full-Stack / Software Engineer

GitHub: [github.com/eli02922](https://github.com/eli02922)

Email: eleazarembuscado1@gmail.com

```
