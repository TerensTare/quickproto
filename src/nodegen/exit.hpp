

#pragma once

#include "nodegen/basic.hpp"
#include "types/int.hpp"

struct exit_node final
{
    entt::entity mem_state;
    int code = 0;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<exit_node>);

inline value_type const *exit_node::infer(type_storage const &types) const
{
    return int_const::value(code);
}

inline entt::entity exit_node::emit(builder &bld, value_type const *ty) const
{
    auto const n = bld.make(node_op::Exit);
    bld.reg.get<node_type>(n).type = ty;
    bld.reg.emplace<effect>(n, mem_state);
    return n;
}