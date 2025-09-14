
#pragma once

#include "pass/pass.hpp"

struct const_fold_pass final : graph_pass
{
    inline bitset256 supported_nodes() const noexcept
    {
        return mask_ops({
            node_op::Add, node_op::Sub, node_op::Mul, node_op::Div, // binary
            node_op::UnaryNeg,                                      // unary
        });
    }

    inline entt::entity run_pass(builder &bld, entt::entity node) noexcept
    {
        // TODO: should you peephole the inputs externally first, before calling this?
        // TODO: ideally, use an entity type that can tell you whether a node is constant or not for fast queries
        auto op = bld.reg.get<node_op const>(node);
        auto &&ins = bld.reg.get<node_inputs const>(node).nodes;

        for (auto &&in : ins)
        {
            // TODO: peephole here (ie. call all passes, not just this one)
        }

        if (op == node_op::UnaryNeg)
        {
            if (bld.reg.get<node_op const>(ins[0]) != node_op::Const)
                return entt::null;
        }
        else
        {
            if (bld.reg.get<node_op const>(ins[0]) != node_op::Const ||
                bld.reg.get<node_op const>(ins[1]) != node_op::Const)
                return entt::null;
        }

        auto newtype = [&] -> value_type const *
        {
            switch (op)
            {
            case node_op::UnaryNeg:
                return bld.reg.get<node_type>(ins[0]).type->as<value_type>()->neg();

            case node_op::Add:
                return bld.reg.get<node_type>(ins[0]).type->as<value_type>() //
                    ->add(bld.reg.get<node_type>(ins[1]).type->as<value_type>());

            case node_op::Sub:
                return bld.reg.get<node_type>(ins[0]).type->as<value_type>() //
                    ->sub(bld.reg.get<node_type>(ins[1]).type->as<value_type>());

            case node_op::Mul:
                return bld.reg.get<node_type>(ins[0]).type->as<value_type>() //
                    ->mul(bld.reg.get<node_type>(ins[1]).type->as<value_type>());

            case node_op::Div:
                return bld.reg.get<node_type>(ins[0]).type->as<value_type>() //
                    ->div(bld.reg.get<node_type>(ins[1]).type->as<value_type>());

            default:
                std::unreachable();
                return nullptr;
            }
        }();

        // FIXME: specify correct memory link
        return bld.makeval(entt::null, node_op::Const, newtype);
    }
};