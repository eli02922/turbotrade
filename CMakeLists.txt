cmake_minimum_required(VERSION 3.20)
project(turbotrade VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- Platform-specific settings ---
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    add_compile_options(/W4 /permissive- /EHsc /Zc:__cplusplus)
    add_link_options(ws2_32)     # Winsock
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    find_package(Threads REQUIRED)
endif()

# Release optimizations
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob3 /DNDEBUG" CACHE STRING "" FORCE)

# --- Common library ---
add_library(turbo_common INTERFACE)
target_include_directories(turbo_common INTERFACE ${CMAKE_SOURCE_DIR})

# --- Executables ---
add_executable(feed_simulator  feed_simulator/main.cpp)
add_executable(feed_handler    feed_handler/main.cpp)
add_executable(strategy_engine strategy/main.cpp)
add_executable(order_gateway   order_gateway/main.cpp)
add_executable(mock_exchange   mock_exchange/main.cpp)
add_executable(latency_monitor latency_monitor/main.cpp)

foreach(tgt feed_simulator feed_handler strategy_engine order_gateway mock_exchange latency_monitor)
    target_link_libraries(${tgt} PRIVATE turbo_common)
    if(NOT WIN32)
        target_link_libraries(${tgt} PRIVATE Threads::Threads)
    endif()
endforeach()

# --- Tests ---
enable_testing()
add_subdirectory(tests)