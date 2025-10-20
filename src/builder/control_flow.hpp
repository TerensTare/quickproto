
#pragma once

#include "builder.hpp"
#include "types/int.hpp"

struct exit_node final
{
    entt::entity mem_state;
    int code = 0;
};

inline entt::entity make(builder &bld, exit_node ex) noexcept
{
    auto const n = bld.make(node_op::Exit);
    bld.reg.get<node_type>(n).type = int_const::value(ex.code);
    bld.reg.emplace<effect>(n, ex.mem_state);
    return n;
}
