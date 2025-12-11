

#pragma once

#include "nodegen/basic.hpp"
#include "types/int.hpp"

struct exit_node final
{
    using ctrl_node = void;

    int code = 0;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<exit_node>);

inline value const *exit_node::infer(type_storage const &types) const
{
    return int_const::make(code);
}

inline entt::entity exit_node::emit(builder &bld, value const *ty) const
{
    auto const n = bld.make(node_op::Exit);
    bld.reg.get<node_type>(n).type = ty;
    return n;
}