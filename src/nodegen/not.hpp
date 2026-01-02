

#pragma once

#include "nodegen/basic.hpp"

// TODO: try any De-Morgan's law simplifications, if of benefit

struct not_node final
{
    entt::entity sub;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<not_node>);

inline value const *not_node::infer(type_storage const &types) const
{
    return types.get(sub).type->bnot();
}

inline entt::entity not_node::emit(builder &bld, value const *val) const
{
    // TODO: enable eventually when you have COW nodes
    // switch (auto &&op = bld.reg.get<node_op>(sub))
    // {
    // case node_op::CmpLt:
    //     op = node_op::CmpGe;
    //     return sub;
    // case node_op::CmpLe:
    //     op = node_op::CmpGt;
    //     return sub;
    // case node_op::CmpEq:
    //     op = node_op::CmpNe;
    //     return sub;
    // case node_op::CmpNe:
    //     op = node_op::CmpEq;
    //     return sub;
    // case node_op::CmpGt:
    //     op = node_op::CmpLe;
    //     return sub;
    // case node_op::CmpGe:
    //     op = node_op::CmpLt;
    //     return sub;
    // }

    entt::entity const ins[]{sub};
    return bld.make(val, node_op::UnaryNot, ins);
}