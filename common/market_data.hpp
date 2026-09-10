#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct MarketUpdate {
    std::uint64_t timestamp_ns;
    std::uint32_t symbol_id;
    std::uint32_t bid_price;
    std::uint32_t ask_price;
    std::uint32_t bid_size;
    std::uint32_t ask_size;
};
#pragma pack(pop)

static_assert(sizeof(MarketUpdate) == 28, "MarketUpdate must be packed");