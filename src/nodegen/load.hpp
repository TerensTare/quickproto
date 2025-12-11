
#pragma once

#include "nodegen/value.hpp"

// TODO:
// - `offset` should never be `nullptr` anymore

// base[offset] or base.offset depending on `base`'s type
struct load_node final
{
    entt::entity base;
    int_value const *offset = nullptr;
    // entt::entity offset = entt::null; // HACK: if null, simply return `base`

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<load_node>);

inline value const *load_node::infer(type_storage const &types) const
{
    // // HACK: do not branch here
    // if (offset == nullptr)
    //     return types.get(base).type;
    // TODO: fail if `offset` is negative
    // TODO: probably best to have `Load` and `LoadN`
    return types.get(base).type->index(offset);
}

inline entt::entity load_node::emit(builder &bld, value const *ty) const
{
    // HACK: do not branch here
    // if (offset == nullptr)
    //     return base;

    auto const load = bld.make(node_op::Load);

    // TODO: is this correct?
    bld.reg.get<node_type>(load).type = ty;

    entt::entity target = bld.state.mem;

    bld.reg.emplace<mem_effect>(load) = {
        .prev = bld.state.mem,
        .target = base,
        .offset = offset,
    };
    bld.reg.emplace<mem_read>(load);
    bld.state.mem = load;

    return load;
}