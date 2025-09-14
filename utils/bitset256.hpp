
#pragma once

// TODO: lighten up `nodes.hpp`; you only need `node_op` from there
#include "nodes.hpp"

struct bitset256 final
{
    constexpr bool test(node_op op) const noexcept
    {
        auto const iop = std::to_underlying(op);
        return bits[iop >> 6] & (1 << (iop & 63));
    }

    constexpr void each(auto &&fn) const noexcept
    {
        for (int i = 0; i < 256; ++i)
            test(node_op(i)) && (fn(i), true);
    }

    uint64_t bits[4] = {};
};

constexpr bitset256 mask_ops(std::initializer_list<node_op> ops) noexcept
{
    bitset256 bits;
    for (auto op : ops)
    {
        auto const iop = std::to_underlying(op);
        bits.bits[iop >> 6] |= (1 << (iop & 63));
    }

    return bits;
}