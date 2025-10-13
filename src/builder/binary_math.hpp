
#pragma once

#include "builder/value.hpp"

// TODO:
// - codegen Iadd vs Fadd depending on argument types here

// lhs + rhs
struct add_node final
{
    entt::entity lhs, rhs;
};

// lhs - rhs
struct sub_node final
{
    entt::entity lhs, rhs;
};

// lhs * rhs
struct mul_node final
{
    entt::entity lhs, rhs;
};

// lhs / rhs
struct div_node final
{
    entt::entity lhs, rhs;
};

inline entt::entity make(builder &bld, add_node add) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(add.lhs) != node_op::Const ||
        ops.get(add.rhs) != node_op::Const)
    {
        entt::entity const ins[]{add.lhs, add.rhs};
        return bld.make(node_op::Add, ins);
    }

    auto const lty = bld.reg.get<node_type const>(add.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(add.rhs).type->as<value_type>();

    auto const newty = lty->add(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, sub_node sub) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(sub.lhs) != node_op::Const ||
        ops.get(sub.rhs) != node_op::Const)
    {
        entt::entity const ins[]{sub.lhs, sub.rhs};
        return bld.make(node_op::Sub, ins);
    }

    auto const lty = bld.reg.get<node_type const>(sub.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(sub.rhs).type->as<value_type>();

    auto const newty = lty->sub(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, mul_node mul) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(mul.lhs) != node_op::Const ||
        ops.get(mul.rhs) != node_op::Const)
    {
        entt::entity const ins[]{mul.lhs, mul.rhs};
        return bld.make(node_op::Mul, ins);
    }

    auto const lty = bld.reg.get<node_type const>(mul.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(mul.rhs).type->as<value_type>();

    auto const newty = lty->mul(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, div_node div) noexcept
{
    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(div.lhs) != node_op::Const ||
        ops.get(div.rhs) != node_op::Const)
    {
        entt::entity const ins[]{div.lhs, div.rhs};
        return bld.make(node_op::Add, ins);
    }

    auto const lty = bld.reg.get<node_type const>(div.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(div.rhs).type->as<value_type>();

    auto const newty = lty->div(rty);
    return bld.make(value_node{newty});
}
