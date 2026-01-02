

#pragma once

#include "nodegen/basic.hpp"
#include "types/composite.hpp"

struct composite_value_node final
{
    composite_type const *ty;
    smallvec<entt::entity> init;
    value const **values = nullptr; // calculated internally

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<composite_value_node>);

inline value const *composite_value_node::infer(type_storage const &types) const
{
    auto const n = ty->n_members;
    auto const init_n = init.n;

    // TODO: add more context
    if (n < init_n)
        return new too_much_init_values{};

    // HACK: handle this better
    const_cast<composite_value_node *>(this)->values = new value const *[ty->n_members];

    size_t i{};
    for (; i < init_n; ++i)
        values[i] = types.get(init[i]).type;

    // TODO: handle "missing" values by zero-init: make a new node per member, get the zero-value of the member's type

    return ty->init(values);
}

inline entt::entity composite_value_node::emit(builder &bld, value const *val) const
{
    return bld.make(
        val, node_op::Struct, init.n,                                    // HACK: handle this better
        const_cast<composite_value_node *>(this)->init.entries.release() //
    );
}