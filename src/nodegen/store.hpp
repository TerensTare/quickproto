
#pragma once

#include "nodegen/value.hpp"

struct store_node final
{
    entt::entity lhs;
    entt::entity offset = entt::null;
    entt::entity rhs;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<store_node>);

inline value_type const *store_node::infer(type_storage const &types) const
{
    // TODO: include the offset here
    // if (offset == entt::null)
    return types.get(lhs).type->assign(types.get(rhs).type);
}

inline entt::entity store_node::emit(builder &bld, value_type const *ty) const
{
    auto lhs = this->lhs;
    auto offset = this->offset;
    auto mem_state = bld.state.mem;

    // HACK: fix this
    if (offset == entt::null)
        offset = make(bld, value_node{int_const::value(0)});

    auto dst_op = bld.reg.get<node_op const>(lhs);
    switch (dst_op)
    {
        // Optimization: Eliminate a Load that comes right before a Store
    case node_op::Load:
    {
        auto this_off = bld.reg.get<node_type const>(offset).type->as<int_const>();
        auto &&prev_ins = bld.reg.get<node_inputs const>(lhs).nodes;

        // TODO: recheck this
        if (this_off && this_off->n == 0)
        {
            lhs = prev_ins[0];
            offset = prev_ins[1];
            // TODO: re-enable this
            // mem_state = bld.reg.get<mem_effect const>(lhs).target;
        }
    }
    break;
    }

    // TODO: optimize `Store`s if possible
    entt::entity const ins[]{lhs, offset, rhs};
    auto const store = bld.make(node_op::Store, ins);
    bld.reg.get<node_type>(store).type = ty;
    // TODO: can you omit this and auto-generate?
    bld.reg.emplace<mem_effect>(store).target = bld.state.mem;
    bld.state.mem = store;

    return store;
}