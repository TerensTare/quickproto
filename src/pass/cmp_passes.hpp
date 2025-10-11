
#pragma once

#include "base.hpp"
#include "pass/pass.hpp"

// TODO:
// - `a == a` does not always hold (eg. NaN)
// ^ but what about `a != a`?

// run passes on comparison nodes
struct cmp_pass final : graph_pass
{
    inline bitset256 supported_nodes() const noexcept
    {
        return mask_ops({
            node_op::CmpEq,
            node_op::CmpNe,
            node_op::CmpGe,
            node_op::CmpGt,
            node_op::CmpLe,
            node_op::CmpLt,
            // TODO: passes that do type inference, eg. `a < b` proves that forward (only on that branch) `a < b` always holds until reassignment
        });
    }

    inline entt::entity run_pass(builder &bld, entt::entity node) noexcept
    {
        auto &&ins = bld.reg.get<node_inputs>(node).nodes;
        // TODO: handle more complex cases, maybe using partial equality and such
        auto const op = bld.reg.get<node_op const>(node);
        auto const self_cmp_is_true = (op == node_op::CmpEq) or
                                      (op == node_op::CmpLe) or
                                      (op == node_op::CmpGe);

        return (ins[0] == ins[1])
                   // TODO: eventually set the correct memory link
                   ? bld.makeval(entt::null, node_op::Const, new bool_const{self_cmp_is_true})
                   : entt::null;
    }
};