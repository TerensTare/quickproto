

#pragma once

#include "nodegen/basic.hpp"

// TODO:
// - a more specialized `if_else` node that generates `IfYes`, `IfNot`, `Region`, `Phi`, etc.
// ^ and optimizes them out as needed

struct region_node final
{
    entt::entity then_state;
    entt::entity else_state;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<region_node>);

inline value_type const *region_node::infer(type_storage const &types) const
{
    // TODO: find a better type instead
    return bot_type::self();
}

inline entt::entity region_node::emit(builder &bld, value_type const *ty) const
{
    // TODO: does this part belong here?
    // TODO: only add one region at the end; when the whole `if-else` tree is parsed
    // ^ even if there is just an `if` branch, the `else` is implicit on both `Region` and `Phi`, you just need to figure out its value on both cases
    // ^ (maybe) this can be done by a pass instead?
    entt::entity const ins[]{then_state, else_state};
    auto const ret = bld.make(node_op::Region, ins);
    return ret;
    // TODO:
    // - fold the `Region` if possible
    // - spawn a Region node here that links to the if/else branches or just the `If`
    // ^ also take care to handle multiple if-else case
    // - spawn a Phi node for merging values
}