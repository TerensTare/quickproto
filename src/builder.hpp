
#pragma once

#include <span>
#include "nodes.hpp"

// TODO:
// - memory optimizations:
// ^ optimized memory layout
// ^ allocate everything on an allocator, keep builder and graph trivial
// - add a free-list to node allocation
// - set the type of each node, either by specifying or by deduction
// - optimize constant stores on `peephole`
// - make `StartNode` hold `$ctrl; params...` as input and use `$ctrl`, `params...` as normal
// - `Store` should be `Store(memory, proj, value)`
// - link constants back to `mem_state` on `peephole`

// component
// Nodes having this component will be deleted by DCE
struct dead_node final
{
    inline static void on_construct(entt::registry &reg, entt::entity id) noexcept
    {
        // "dead nodes" are infective to their children (TODO: make sure you do NOT delete shared nodes)
        auto &&dead = reg.storage<dead_node>();
        auto &&to_add = reg.get<node_inputs const>(id).nodes;
        dead.insert(to_add.begin(), to_add.end());
    }
};

struct builder final
{
    // invariant: The returned entity has the following components:
    // `node`
    // `node_inputs`
    // invariant: `users` and `user_counter` are updated for any entity in `nins`
    inline entt::entity make(node_op op, std::span<entt::entity const> nins) noexcept;

    template <std::same_as<entt::entity>... Ins>
    inline entt::entity make(node_op op, Ins... ins) noexcept
    {
        // TODO: recheck this
        entt::entity nins_arr[]{entt::null, ins...}; // Add null in the beginning because C++ doesn't allow arrays of size 0
        return make(op, std::span(nins_arr + 1, sizeof...(Ins)));
    }

    // Add some kind of static checks to `op` here
    inline entt::entity makeval(entt::entity memory, node_op op, type const *type) noexcept;

    // TODO: is there any node with more than 1 effect dependency?
    entt::registry reg;
};

inline entt::entity builder::make(node_op op, std::span<entt::entity const> nins) noexcept
{
    auto const num_ins = nins.size();

    auto const n = reg.create();
    reg.emplace<node_op>(n, op);
    reg.emplace<node_type>(n, bot::self());
    reg.emplace<node_inputs>(n, std::vector(nins.begin(), nins.end()));

    // TODO: optimize
    for (uint32_t i{}; auto id : nins)
    {
        reg.get_or_emplace<users>(id).entries.push_back({n, i++});
    }

    return n;
}

inline entt::entity builder::makeval(entt::entity memory, node_op op, type const *type) noexcept
{
    auto n = make(op);
    reg.get<node_type>(n).type = type;

    // TODO: actually set this
    // effects[n->id] = memory;

    return n;
}

inline void prune_dead_code(builder &bld) noexcept
{
    auto &&to_cut = bld.reg.storage<dead_node>();
    bld.reg.destroy(to_cut.begin(), to_cut.end());
}