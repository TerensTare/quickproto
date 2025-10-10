
#pragma once

#include <span>
#include "nodes.hpp"

// TODO:
// - memory optimizations:
// ^ optimized memory layout
// ^ allocate everything on an allocator, keep builder and graph trivial
// - make `StartNode` hold `$ctrl; params...` as input and use `$ctrl`, `params...` as normal
// - `Store` should be `Store(memory, proj, value)`
// - move passes to `builder` instead of `parser` and run on each node parsed
// - build-from-blueprint style for less error-prone code + it might help with defining passes

inline type const *type_by_op(node_op op) noexcept
{
    switch (op)
    {
    case node_op::Add:
    case node_op::Sub:
    case node_op::Mul:
    case node_op::Div:
    case node_op::UnaryNeg:
        return int_bot::self();

    case node_op::Fadd:
    case node_op::Fsub:
    case node_op::Fmul:
    case node_op::Fdiv:
        return float_bot::self();

    case node_op::CmpEq:
    case node_op::CmpGe:
    case node_op::CmpGt:
    case node_op::CmpLe:
    case node_op::CmpLt:
    case node_op::CmpNe:
        return bool_bot::self();

    case node_op::IfYes:
    case node_op::IfNot:
    case node_op::Region:
        return ctrl::self();

        // HACK: figure each node out instead
    default:
        return bot::self();
    }
}

struct builder final
{
    // invariant: The returned entity has the following components:
    // `node`
    // `node_type`
    // `node_inputs`
    // invariant: `users` is updated for any entity in `nins`
    inline entt::entity make(node_op op, std::span<entt::entity const> nins) noexcept;

    inline entt::entity make(node_op op) noexcept
    {
        entt::entity dummy[1];
        return make(op, std::span(dummy, 0));
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
    reg.emplace<node_type>(n, type_by_op(op));
    reg.emplace<node_inputs>(n, std::vector(nins.begin(), nins.end()));
    reg.emplace<maybe_reachable>(n);

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

inline void prune_dead_code(builder &bld, entt::entity ret) noexcept
{
    struct reachable final
    {
    };

    auto &&new_nodes = bld.reg.storage<maybe_reachable>();
    auto &&visited = bld.reg.storage<reachable>();

    auto const &ins_storage = bld.reg.storage<node_inputs>();
    auto const &effects = bld.reg.storage<effect>();

    // TODO: do you need to repeat this?
    std::vector<entt::entity> to_visit;
    to_visit.push_back(ret);

    while (!to_visit.empty())
    {
        auto top = to_visit.back();
        to_visit.pop_back();

        // TODO: is this ever the case that this node was visited previously?
        if (visited.contains(top))
            continue;

        // TODO: add other links here as well
        // TODO: maybe just follow the effect and memory chain instead?
        // ^ also you can check here `visited` so that you don't push into the vector, causing a possible reallocation
        auto &&ins = ins_storage.get(top).nodes;
        to_visit.insert(to_visit.end(), ins.begin(), ins.end());

        if (effects.contains(top))
            to_visit.push_back(effects.get(top).target);

        visited.emplace(top);
    }

    // TODO: these are unreachable, so maybe leave a warning for significant nodes? (functions, etc.)
    auto to_cut_view = entt::basic_view{std::tie(new_nodes), std::tie(visited)};
    bld.reg.destroy(to_cut_view.begin(), to_cut_view.end());

    new_nodes.clear();
}
