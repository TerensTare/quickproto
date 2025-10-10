
#pragma once

#include "base.hpp"
#include "pass/pass.hpp"

// fold `Phi` nodes where the condition is constant to just one value
struct phi_fold_pass final : graph_pass
{
    inline bitset256 supported_nodes() const noexcept
    {
        return mask_ops({
            node_op::Phi,
        });
    }

    inline entt::entity run_pass(builder &bld, entt::entity node) noexcept
    {
        // TODO: enable this back
        // ^ the problem is that the `IfYes`/`IfNot` nodes are now not always present right before the `Region` node, so figure out how to get to them
        // ^ this can easily be done if you have an `If` node, and then you find the first If (?) to get the split point
        // auto &&ins_pool = bld.reg.storage<node_inputs>();
        // auto &&ins = ins_pool.get(node).nodes;

        // auto const region = bld.reg.get<region_of_phi const>(node).region;
        // auto const if_yes = ins_pool.get(region).nodes[0];
        // auto const cond = ins_pool.get(if_yes).nodes[0];

        // auto ty = bld.reg.get<node_type const>(cond).type;

        // // TODO: move this to typechecking phase
        // // (maybe): have a `ctrl.cond(cond_expr)` function on `ctrl` that typechecks this instead
        // ensure(ty->as<bool_>(), "Condition of an `if` must be a boolean type!");

        // if (auto cond = ty->as<bool_const>())
        //     return ins[2 - cond->b]; // if true, pick 1, else pick 2

        return entt::null;
    }
};