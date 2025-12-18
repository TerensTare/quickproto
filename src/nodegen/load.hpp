
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
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<load_node>);

inline value const *load_node::infer(type_storage const &types) const
{
    // TODO: fail if `offset` is negative
    // TODO: probably best to have `Load` and `LoadN`
    return types.get(base).type->index(offset);
}

inline entt::entity load_node::emit(builder &bld, value const *val) const
{
    // TODO: is the value correct?
    auto const load = bld.make(val, node_op::Load);

    // TODO: tag=-1 should not be Top, instead it should propagate up
    bld.reg.emplace<mem_effect>(load) = {
        .target = base,
        .tag = offset->as<int_const>() ? (uint32_t)offset->as<int_const>()->n : ~uint32_t{},
    };
    bld.reg.emplace<mem_read>(load);
    bld.state.mem = load;

    return load;
}