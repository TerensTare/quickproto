
#pragma once

#include "utils/bitset256.hpp"
#include "builder.hpp"

// TODO:
// - notice how you know at call site what node type you are running a pass on; use that as a filter
// - shortcut method that tells you if the pass might work on the given node or not (is this a bloom filter?)
// ^ ```cpp
// pass_registry passes;
// passes.add<const_fold>(math_ops_list);
// ...
// passes.run(builder, some_node);
// ```
// ^ pre-made list of op masks for easy writing passes
// ^ move the `.pass` function to `pass_registry` and quick-query all passes that might match with a node op

struct graph_pass
{
    virtual ~graph_pass() {}

protected:
    /// Instantiate a new graph pass that might run on the given set of nodes.
    /// @return An inclusive list of node kinds that the pass can run on. It can include nodes that are not modified by the pass but shouldn't exclude nodes that might be modified by it.
    /// For example, a constant folding pass over math operations should include all nodes with math operations, and on `run_pass` it can filter nodes that cannot be optimized by the pass.
    /// It should NOT miss any node operation that it supports as the pass registry then has no way to know it should run the pass over those nodes.
    ///
    /// In short, the list should include all nodes that you want the pass to run on, then you can filter out further on `run_pass`.
    virtual bitset256 supported_nodes() const noexcept = 0;

    // return the new entity if you replaced the node; entt::null otherwise
    virtual entt::entity run_pass(builder &bld, entt::entity node) = 0;

    friend struct pass_registry;
};