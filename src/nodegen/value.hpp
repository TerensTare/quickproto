
#pragma once

#include "nodegen/basic.hpp"

struct value_node final
{
    value const *ty;

    inline value const *infer(type_storage const &) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<value_node>);

inline value const *value_node::infer(type_storage const &) const { return ty; }

inline entt::entity value_node::emit(builder &bld, value const *ty) const
{
    // TODO: more cases here
    auto const kind = (ty->as<int_const>())
                          ? node_op::IConst
                      : (ty->as<float64>())
                          ? node_op::FConst
                          : node_op::BConst;

    auto const n = bld.make(kind);
    bld.reg.get<node_type>(n).type = ty;

    return n;
}