
#pragma once

#include "nodegen/value.hpp"

// TODO:
// - `offset` should never be null anymore

struct store_node final
{
    entt::entity lhs;
    int_value const *offset = nullptr;
    entt::entity rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<store_node>);

inline value const *store_node::infer(type_storage const &types) const
{
    // TODO: include the offset here
    // if (offset == entt::null)
    return types.get(lhs).type->assign(types.get(rhs).type);
}

inline entt::entity store_node::emit(builder &bld, value const *ty) const
{
    auto lhs = this->lhs;
    auto offset = this->offset;
    auto mem_state = bld.state.mem;

    // HACK: fix this
    if (offset == nullptr)
        offset = int_const::make(0);

    // TODO: move these optimizations out of here
    // auto dst_op = bld.reg.get<node_op const>(lhs);
    // switch (dst_op)
    // {
    //     // Optimization: Eliminate a Load that comes right before a Store
    // case node_op::Load:
    // {
    //     // TODO: handle the non-const case
    //     auto this_off = offset->as<int_const>();
    //     auto &&prev_ins = bld.reg.get<node_inputs const>(lhs).nodes;

    //     // TODO: recheck this
    //     if (this_off && this_off->n == 0)
    //     {
    //         // TODO: handle non-const case
    //         offset = bld.reg.get<mem_effect>(lhs).offset->as<int_const>();
    //         lhs = prev_ins[0];
    //         // TODO: re-enable this
    //         // mem_state = bld.reg.get<mem_effect const>(lhs).target;
    //     }
    // }
    // break;
    // }

    // TODO: optimize `Store`s if possible
    entt::entity const ins[]{rhs};
    auto const store = bld.make(node_op::Store, ins);
    bld.reg.get<node_type>(store).type = ty;

    // HACK: used to inline `Load`/`Store` as there is no way in the parser to detect rvalues right now...
    if (bld.reg.get<node_op>(lhs) == node_op::Load)
    {
        bld.reg.emplace<mem_effect>(store, bld.reg.get<mem_effect const>(lhs));
    }
    else
    {
        // TODO: can you omit this and auto-generate?
        // TODO: recheck this
        bld.reg.emplace<mem_effect>(store) = {
            .prev = bld.state.mem,
            .target = lhs,
            .offset = offset,
        };
    }

    bld.reg.emplace<mem_write>(store);
    bld.state.mem = store;

    return store;
}