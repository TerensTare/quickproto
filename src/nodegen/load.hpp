
#pragma once

#include "nodegen/value.hpp"

// TODO:
// - optimize load/load, similar
/*
    if the previous memory state is a `Store`:
    - if the place and index is the same as this, just drop this `Load` and return the `Store`
    - if some other place or index, the `Load` and `Store` can happen in parallel
    if the second branch is taken, set previous memory state to the `Store`'s memory state and repeat the steps
*/

// base[offset] or base.offset depending on `base`'s type
struct load_node final
{
    entt::entity base;
    entt::entity offset = entt::null; // HACK: if null, simply return `base`

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<load_node>);

inline value_type const *load_node::infer(type_storage const &types) const
{
    // HACK: do not branch here
    if (offset == entt::null)
        return types.get(base).type;
    // TODO: fail if `offset` is negative
    // TODO: probably best to have `Load` and `LoadN`
    return types.get(base).type->index(types.get(offset).type);
}

inline entt::entity load_node::emit(builder &bld, value_type const *ty) const
{
    // HACK: do not branch here
    if (offset == entt::null)
        return base;

    entt::entity const ins[]{base, offset};
    auto const load = bld.make(node_op::Load, ins);

    // TODO: is this correct?
    bld.reg.get<node_type>(load).type = ty;

    entt::entity target = bld.state.mem;

    while (true)
    {
        auto const prev_mem = bld.reg.get<node_op>(target);

        switch (prev_mem)
        {
            // Load/Load can happen in parallel
        case node_op::Load:
            // TODO: repeat possible optimizations in case one happens
            target = bld.reg.get<mem_effect>(bld.state.mem).target;
            break;

        // If a Load has the same target + index as the previous Store, eliminate the Load
        case node_op::Store:
        {
            auto this_off = bld.reg.get<node_type const>(offset).type->as<int_const>();
            auto &&store_ins = bld.reg.get<node_inputs const>(bld.state.mem).nodes;
            auto prev_off = bld.reg.get<node_type const>(store_ins[1]).type->as<int_const>();

            // if Load after Store targets same value + index, then omit the Load
            if (store_ins[0] == base)
            {
                if (this_off->n == prev_off->n)
                    return bld.state.mem;

                // otherwise, they can happen in parallel if the index is different (they do not touch the same memory)
                // target = bld.reg.get<mem_effect const>(bld.state.mem).target;
            }
        }
            [[fallthrough]];

        default:
            bld.reg.emplace<mem_effect>(load).target = target;
            bld.state.mem = load;

            return load;
        }
    }
}