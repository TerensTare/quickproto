

#pragma once

#include "nodegen/basic.hpp"

#include "types/tuple.hpp"
#include "types/void.hpp"

struct return_node final
{
    using ctrl_node = void;
    using mem_write = void; // TODO: is this correct?

    // TODO: try to see if you can instead pass an implicit list here
    std::span<entt::entity const> values;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<return_node>);

inline value const *return_node::infer(type_storage const &types) const
{
    // TODO: typecheck with the function return type
    auto const n = values.size();
    // TODO: should you handle these as special cases?
    if (n == 0)
        // TODO: use singleton
        return void_type{}.top();
    if (n == 1)
    {
        // HACK: only for now
        if (values[0] == entt::null)
            // TODO: use singleton
            return void_type{}.top();
        return types.get(values[0]).type;
    }

    // TODO: find a better type instead
    auto tys = std::make_unique_for_overwrite<value const *[]>(n);
    for (size_t i{}; i < n; ++i)
        tys[i] = types.get(values[i]).type;

    return new tuple_n{n, std::move(tys)};
}

inline entt::entity return_node::emit(builder &bld, value const *val) const
{
    // HACK: only for now
    if (values[0] == entt::null)
    {
        // TODO: is this correct?
        return bld.make(val, node_op::Return, {});
    }

    // TODO: is this correct?
    return bld.make(val, node_op::Return, values);
}