
#pragma once

#include "pass/const_fold.hpp"
#include "pass/phi_fold.hpp"

// TODO:
// - the control flow of `peephole` would most likely be like this:
/*
```cpp
loop {
    node_loop:
    while !nodes.empty() {
        auto node = nodes.pop();
        for pass in matching(node) {
            auto new_node = pass(node);
            if new_node != null {
                replace(node, new_node); // push new_node to nodes2
                repeat_again = true;
                continue node_loop;
            }
        }

        // node not replaced; push to node2
    }

    swap(nodes, nodes2);
}
```
*/

struct pass_registry final
{
    inline entt::entity run(entt::entity node)
    {
        auto &&op = bld.reg.get<node_op const>(node);
        if (const_fold_nodes.test(op))
        {
            if (auto const newn = const_fold_pass.run_pass(bld, node); newn != entt::null)
            {
                // TODO: can you make it so that passes are ran only when a node is created? (ie. no users yet)
                replace(node, newn);
                return newn;
            }
        }

        if (phi_fold_nodes.test(op))
        {
            if (auto const newn = phi_fold_pass.run_pass(bld, node); newn != entt::null)
            {
                // TODO: can you make it so that passes are ran only when a node is created? (ie. no users yet)
                replace(node, newn);
                return newn;
            }
        }

        return node;
    }

    // TODO: make it extensible
    builder &bld;
    // const_fold
    const_fold_pass const_fold_pass;
    bitset256 const_fold_nodes = const_fold_pass.supported_nodes();
    // phi_fold
    phi_fold_pass phi_fold_pass;
    bitset256 phi_fold_nodes = phi_fold_pass.supported_nodes();

private:
    inline void replace(entt::entity oldn, entt::entity newn);
};

inline void pass_registry::replace(entt::entity oldn, entt::entity newn)
{
    // TODO: implement
    bld.reg.emplace<dead_node>(oldn);
}