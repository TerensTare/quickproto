

#pragma once

#include "nodegen/basic.hpp"
#include "types/func.hpp"

struct call_static_node final
{
    // TODO: figure out the exact inputs you need for this node
    entt::entity mem_state;
    // TODO: make use of `func`
    entt::entity func;
    std::span<entt::entity const> args;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<call_static_node>);

inline value_type const *call_static_node::infer(type_storage const &types) const
{
    // TODO: typecheck that values can be passed to `func`, by using `value::call`
    return types.get(func).type->as<::func>()->ret;
}

inline entt::entity call_static_node::emit(builder &bld, value_type const *ty) const
{
    auto const call = bld.make(node_op::CallStatic, args);
    // TODO: as optimization, figure out if it's a pure call and cut the link to `mem_state` if so
    bld.reg.emplace<effect>(call, mem_state);

    return call;
}