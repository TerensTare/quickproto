
#pragma once

#include "nodegen/basic.hpp"

// TODO:
// - `offset` should never be null anymore

struct store_node final
{
    entt::entity lhs;
    int_value const *offset = nullptr;
    entt::entity rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<store_node>);

inline value const *store_node::infer(type_storage const &types) const
{
    // TODO: include the offset here
    // if (offset == entt::null)
    return types.get(lhs).type->assign(types.get(rhs).type);
}

inline entt::entity store_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{rhs};
    auto const store = bld.make(val, node_op::Store, ins);

    // HACK: used to inline `Load`/`Store` as there is no way in the parser to detect rvalues right now...
    if (bld.reg.get<node_op>(lhs) == node_op::Load)
    {
        bld.reg.emplace<mem_effect>(store, bld.reg.get<mem_effect const>(lhs));
    }
    else
    {
        // TODO: can you omit this and auto-generate?
        // TODO: recheck this
        // TODO: tag=-1 should not be Top, instead it should propagate up
        bld.reg.emplace<mem_effect>(store) = {
            .target = lhs,
            .tag = offset->as<int_const>() ? (uint32_t)offset->as<int_const>()->n : ~uint32_t{},
        };
    }

    bld.reg.emplace<mem_write>(store);
    bld.state.mem = store;

    return store;
}