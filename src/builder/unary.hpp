
#pragma once

#include "types/value.hpp"
#include "builder.hpp"

// -sub
struct uneg_node final
{
    entt::entity sub;
};

// !sub
struct unot_node final
{
    entt::entity sub;
};

inline entt::entity make(builder &bld, uneg_node neg) noexcept
{
    auto const op = bld.reg.get<node_op const>(neg.sub);
    if (op == node_op::Const)
    {
        auto const newty = bld.reg.get<node_type const>(neg.sub).type->neg();
        return bld.make(value_node{newty});
    }

    return bld.make(node_op::UnaryNeg, std::span(&neg.sub, 1));
}

inline entt::entity make(builder &bld, unot_node unot) noexcept
{
    auto const op = bld.reg.get<node_op const>(unot.sub);
    if (op == node_op::Const)
    {
        auto const newty = bld.reg.get<node_type const>(unot.sub).type->bnot();
        return bld.make(value_node{newty});
    }

    return bld.make(node_op::UnaryNeg, std::span(&unot.sub, 1));
}