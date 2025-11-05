
#pragma once

#include "types/value.hpp"
#include "builder.hpp"

// 2-phase build (or rather 3)
// - typecheck
// - peephole (optional)
// - codegen

// TODO:
// - embed type info in entity id for fast type check
// - pass the optimization flags as parameter, build a jump table from that
// - only run type checking iff the node has a resolved name, then codegen the node

using type_storage = entt::storage_for_t<node_type>;

template <typename T>
concept nodegen = requires(T const &t, builder &bld, type_storage const &types) {
    { t.infer(types) } -> std::same_as<value_type const *>;
    // TODO: you probably need the inferred type on `emit`
    { t.emit(bld, types.get(entt::entity{}).type) } -> std::same_as<entt::entity>;
};

struct type_error final
{
    value_error const *err;

    inline value_type const *infer(type_storage const &types) const { return err; }

    inline entt::entity emit(builder &bld, value_type const *ty) const
    {
        // TODO: figure out the exact type of this node
        auto const n = bld.make(node_op::Error);
        bld.reg.get<node_type>(n).type = ty;
        (void)bld.reg.emplace<error_node>(n);
        return n;
    }
};

static_assert(nodegen<type_error>);

template <nodegen Gen>
inline entt::entity make(builder &bld, Gen const &gen)
{
    // TODO: cache the storage here then later drop it for embedded type info
    // TODO: the errors are still handled by `value_type`, maybe separate them from there
    // TODO: also keep location information, maybe as a `node_span` component
    // ^ the `node_span` of `make(children...)` is `join(first, last)`
    auto ty = gen.infer(bld.reg.storage<node_type>());
    if (auto err = ty->as<value_error>())
        return make(bld, type_error{err});

    return gen.emit(bld, ty);
}