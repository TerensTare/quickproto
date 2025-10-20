
#pragma once

#include "builder.hpp"
#include "types/value.hpp"

struct and_node final
{
    entt::entity lhs, rhs;
};

struct or_node final
{
    entt::entity lhs, rhs;
};

inline entt::entity make(builder &bld, and_node land) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(land.lhs) != node_op::Const ||
        ops.get(land.rhs) != node_op::Const)
    {
        entt::entity const ins[]{land.lhs, land.rhs};
        return bld.make(node_op::LogicAnd, ins);
    }

    auto const lty = bld.reg.get<node_type const>(land.lhs).type;
    auto const rty = bld.reg.get<node_type const>(land.rhs).type;

    auto const newty = lty->logic_and(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, or_node lor) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(lor.lhs) != node_op::Const ||
        ops.get(lor.rhs) != node_op::Const)
    {
        entt::entity const ins[]{lor.lhs, lor.rhs};
        return bld.make(node_op::LogicOr, ins);
    }

    auto const lty = bld.reg.get<node_type const>(lor.lhs).type;
    auto const rty = bld.reg.get<node_type const>(lor.rhs).type;

    auto const newty = lty->logic_or(rty);
    return bld.make(value_node{newty});
}
