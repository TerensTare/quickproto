
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
    { t.infer(types) } -> std::same_as<value const *>;
    // TODO: do you really need the type on `emit`? if not, you can assign it automatically from `make` to avoid errors
    { t.emit(bld, types.get(entt::entity{}).type) } -> std::same_as<entt::entity>;
};

struct type_error final
{
    value_error const *err;

    inline value const *infer(type_storage const &types) const { return err; }

    inline entt::entity emit(builder &bld, value const *val) const
    {
        // TODO: figure out the exact type of this node
        auto const n = bld.make(val, node_op::Error, {});
        (void)bld.reg.emplace<error_node>(n);
        return n;
    }
};

static_assert(nodegen<type_error>);

struct value_node final
{
    value const *val;

    inline value const *infer(type_storage const &) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<value_node>);

inline value const *value_node::infer(type_storage const &) const { return val; }

inline entt::entity value_node::emit(builder &bld, value const *val) const
{
    // TODO: more cases here
    auto const kind = (val->as<int_const>())
                          ? node_op::IConst
                      : (val->as<float64>())
                          ? node_op::FConst
                          : node_op::BConst;

    return bld.make(val, kind, {});
}

static_assert(nodegen<value_node>);

template <nodegen Gen>
[[nodiscard]]
inline entt::entity make(builder &bld, Gen const &gen)
{
    // TODO: cache the storage here then later drop it for embedded type info
    // TODO: the errors are still handled by `value`, maybe separate them from there
    // TODO: also keep location information, maybe as a `node_span` component
    // ^ the `node_span` of `make(children...)` is `join(first, last)`
    auto val = gen.infer(bld.reg.storage<node_type>());
    if (auto err = val->as<value_error>())
        return type_error{err}.emit(bld, err); // NOTE: `make` is not used here to avoid the recursive call

    if (val->is_const())
        return value_node{val}.emit(bld, val); // NOTE: `make` is not used here to avoid the recursive call + type checks

    // problem: the resulting node might be optimized and does not have the same effects as the original one; address that
    auto const n = gen.emit(bld, val);
    if constexpr (requires { typename Gen::ctrl_node; })
    {
        bld.reg.emplace<ctrl_effect>(n, bld.state.ctrl);
        bld.state.ctrl = n;
    }

    if constexpr (requires { typename Gen::mem_read; })
    {
        // TODO: set the target here; for alloca/callstatic it is the function Start
        // TODO: update this to reflect the changes
        // TODO: set the tag
        bld.reg.emplace<mem_effect>(n) = {
            .prev = bld.state.mem,
            .target = bld.state.func,
        };
        bld.reg.emplace<mem_read>(n);
        bld.state.mem = n;
    }

    if constexpr (requires { typename Gen::mem_write; })
    {
        // TODO: set the target here; for alloca/callstatic it is the function Start
        // TODO: update this to reflect the changes
        // TODO: set the tag
        bld.reg.emplace<mem_effect>(n) = {
            .prev = bld.state.mem,
            .target = bld.state.func,
        };
        bld.reg.emplace<mem_write>(n);
        bld.state.mem = n;
    }

    return n;
}