
#pragma once

#include "builder/value.hpp"
#include "types/bool.hpp"

// TODO: `value_type::not_equal`, etc. as non-virtual functions

struct iseq_node final
{
    entt::entity lhs, rhs;
};

struct isne_node final
{
    entt::entity lhs, rhs;
};

struct islt_node final
{
    entt::entity lhs, rhs;
};

struct isle_node final
{
    entt::entity lhs, rhs;
};

struct isgt_node final
{
    entt::entity lhs, rhs;
};

struct isge_node final
{
    entt::entity lhs, rhs;
};

inline entt::entity make(builder &bld, iseq_node eq) noexcept
{
    if (eq.lhs == eq.rhs)
        return bld.make(value_node{new bool_const{true}});

    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(eq.lhs) != node_op::Const ||
        ops.get(eq.rhs) != node_op::Const)
    {
        entt::entity const ins[]{eq.lhs, eq.rhs};
        return bld.make(node_op::CmpEq, ins);
    }

    auto const lty = bld.reg.get<node_type const>(eq.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(eq.rhs).type->as<value_type>();

    auto const newty = lty->eq(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, isne_node ne) noexcept
{
    if (ne.lhs == ne.rhs)
        return bld.make(value_node{new bool_const{false}});

    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(ne.lhs) != node_op::Const ||
        ops.get(ne.rhs) != node_op::Const)
    {
        entt::entity const ins[]{ne.lhs, ne.rhs};
        return bld.make(node_op::CmpNe, ins);
    }

    auto const lty = bld.reg.get<node_type const>(ne.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(ne.rhs).type->as<value_type>();

    auto const newty = lty->ne(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, islt_node lt) noexcept
{
    if (lt.lhs == lt.rhs)
        return bld.make(value_node{new bool_const{false}});

    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(lt.lhs) != node_op::Const ||
        ops.get(lt.rhs) != node_op::Const)
    {
        entt::entity const ins[]{lt.lhs, lt.rhs};
        return bld.make(node_op::CmpLt, ins);
    }

    auto const lty = bld.reg.get<node_type const>(lt.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(lt.rhs).type->as<value_type>();

    auto const newty = lty->lt(rty); // TODO: set the memory node
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, isle_node le) noexcept
{
    if (le.lhs == le.rhs)
        return bld.make(value_node{new bool_const{true}});

    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(le.lhs) != node_op::Const ||
        ops.get(le.rhs) != node_op::Const)
    {
        entt::entity const ins[]{le.lhs, le.rhs};
        return bld.make(node_op::CmpNe, ins);
    }

    auto const lty = bld.reg.get<node_type const>(le.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(le.rhs).type->as<value_type>();

    auto const newty = lty->le(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, isgt_node gt) noexcept
{
    if (gt.lhs == gt.rhs)
        return bld.make(value_node{new bool_const{false}});

    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(gt.lhs) != node_op::Const ||
        ops.get(gt.rhs) != node_op::Const)
    {
        entt::entity const ins[]{gt.lhs, gt.rhs};
        return bld.make(node_op::CmpNe, ins);
    }

    auto const lty = bld.reg.get<node_type const>(gt.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(gt.rhs).type->as<value_type>();

    auto const newty = lty->gt(rty);
    return bld.make(value_node{newty});
}

inline entt::entity make(builder &bld, isge_node ge) noexcept
{
    if (ge.lhs == ge.rhs)
        return bld.make(value_node{new bool_const{true}});

    auto &&ops = bld.reg.storage<node_op>();
    if (ops.get(ge.lhs) != node_op::Const ||
        ops.get(ge.rhs) != node_op::Const)
    {
        entt::entity const ins[]{ge.lhs, ge.rhs};
        return bld.make(node_op::CmpNe, ins);
    }

    auto const lty = bld.reg.get<node_type const>(ge.lhs).type->as<value_type>();
    auto const rty = bld.reg.get<node_type const>(ge.rhs).type->as<value_type>();

    auto const newty = lty->ge(rty);
    return bld.make(value_node{newty});
}
