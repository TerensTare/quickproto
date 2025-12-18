
#pragma once

#include "nodegen/basic.hpp"

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

    auto const n = bld.make(val, kind);

    return n;
}