
#pragma once

#include "builder/value.hpp"

// lhs & rhs
struct bit_and_node final
{
    entt::entity lhs, rhs;
};

// lhs ^ rhs
struct bit_xor_node final
{
    entt::entity lhs, rhs;
};

// lhs | rhs
struct bit_or_node final
{
    entt::entity lhs, rhs;
};

inline entt::entity make(builder &bld, bit_and_node add) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(add.lhs) != node_op::Const ||
        ops.get(add.rhs) != node_op::Const)
    {
        entt::entity const ins[]{add.lhs, add.rhs};
        return bld.make(node_op::BitAnd, ins);
    }

    auto const lty = bld.reg.get<node_type const>(add.lhs).type;
    auto const rty = bld.reg.get<node_type const>(add.rhs).type;

    // TODO: more optimizations (b & b, b & 0, etc)
    auto const newty = lty->band(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, bit_xor_node sub) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(sub.lhs) != node_op::Const ||
        ops.get(sub.rhs) != node_op::Const)
    {
        entt::entity const ins[]{sub.lhs, sub.rhs};
        return bld.make(node_op::BitXor, ins);
    }

    auto const lty = bld.reg.get<node_type const>(sub.lhs).type;
    auto const rty = bld.reg.get<node_type const>(sub.rhs).type;

    // TODO: more optimizations (b ^ b, b ^ 0, etc.)
    auto const newty = lty->bxor(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, bit_or_node mul) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(mul.lhs) != node_op::Const ||
        ops.get(mul.rhs) != node_op::Const)
    {
        entt::entity const ins[]{mul.lhs, mul.rhs};
        return bld.make(node_op::BitOr, ins);
    }

    auto const lty = bld.reg.get<node_type const>(mul.lhs).type;
    auto const rty = bld.reg.get<node_type const>(mul.rhs).type;

    // TODO: more optimizations (b | b, b | 0, etc.)
    auto const newty = lty->bor(rty);
    return bld.make(value_node{newty});
}
