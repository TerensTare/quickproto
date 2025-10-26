
#pragma once

#include "builder.hpp"
#include "types/int.hpp"

struct exit_node final
{
    entt::entity mem_state;
    int code = 0;
};

// TODO: merge with `phi`
struct return_phi final
{
    entt::entity region;
    entt::entity lhs, rhs;
};

inline entt::entity make(builder &bld, exit_node ex) noexcept
{
    auto const n = bld.make(node_op::Exit);
    bld.reg.get<node_type>(n).type = int_const::value(ex.code);
    bld.reg.emplace<effect>(n, ex.mem_state);
    return n;
}

// TODO: return one entity per expr in return
inline entt::entity make(builder &bld, return_phi const &phi) noexcept
{
    // TODO: handle case when either branch is null
    // TODO: handle multiple returns
    // TODO: ensure number of expr matches on both
    // TODO: apply optimizations
    entt::entity const ins[]{phi.lhs, phi.rhs};
    auto const ret = bld.make(node_op::Phi, ins);
    (void)bld.reg.emplace<region_of_phi>(ret, phi.region);
    return ret; // TODO: fold the `Phi` if possible
}
