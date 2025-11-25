

#pragma once

#include "nodegen/basic.hpp"
#include "types/func.hpp"

// TODO: codegen either `StaticCall` or `ExternCall` depending on the function information

struct call_node final
{
    using ctrl_node = void;
    using mem_node = void;

    // TODO: figure out the exact inputs you need for this node
    bool is_extern = false;
    entt::entity func;
    std::span<entt::entity const> args;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<call_node>);

inline value_type const *call_node::infer(type_storage const &types) const
{
    auto const n = args.size();
    std::vector<value_type const *> argty;
    argty.reserve(n);

    for (auto arg : args)
        argty.push_back(types.get(arg).type);

    // TODO: fail if no `$ctrl` specified for `ExternCall`
    return types.get(func).type->as<::func>()->call(argty);
}

inline entt::entity call_node::emit(builder &bld, value_type const *ty) const
{
    // HACK: implement this in a cleaner way
    auto const nodety = (is_extern) ? node_op::ExternCall : node_op::CallStatic;

    auto const call = bld.make(nodety, args);
    // TODO: as optimization, figure out if it's a pure call and cut the link to `mem_state` if so
    bld.reg.get<node_type>(call).type = ty;
    // TODO: this node still might affect I/O and similar

    return call;
}