
#pragma once

#include "nodegen/value.hpp"

// TODO:
// - optimize load/load, similar

struct load_node final
{
    entt::entity mem_state;
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

    // optimization: two consecutive loads are independent from each other
    auto const target = (bld.reg.get<node_op>(mem_state) == node_op::Load)
                            ? bld.reg.get<mem_effect>(mem_state).target
                            : mem_state;

    bld.reg.emplace<mem_effect>(load).target = target;

    return load;
}