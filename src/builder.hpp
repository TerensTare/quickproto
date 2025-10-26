
#pragma once

#include <concepts>
#include <entt/entity/registry.hpp>
#include "nodes.hpp"

// TODO:
/*
    scope-based tiered visibility marking:
    - reachable: exported names and everything left by DCE
    - global: global variables/constants, functions, etc., subject to DCE after whole file is parsed
    - maybe_reachable: local declarations inside a function, subject to function-local DCE
    - unreachable, everything cut by DCE, or following a control flow statement (`return`, `continue`, `break`)
*/
// - memory optimizations:
// ^ optimized memory layout
// ^ allocate everything on an allocator, keep builder and graph trivial
// - make `StartNode` hold `$ctrl; params...` as input and use `$ctrl`, `params...` as normal
// - `Store` should be `Store(memory, proj, value)`
// - move passes to `builder` instead of `parser` and run on each node parsed
// - build-from-blueprint style for less error-prone code + it might help with defining passes

inline value_type const *type_by_op(node_op op) noexcept
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
        // TODO: figure this out

        // HACK: figure each node out instead
    default:
        return int_bot::self();
    }
}

using tag_storage = std::remove_cvref_t<decltype(std::declval<entt::registry>().storage<void>())>;

// TODO: ensure `push` only lowers visibility and `pop` only increases with debug checks
enum class visibility : entt::id_type
{
    // TODO: this is out-of-order compared to what follows; the first one should be `global` and this is -1
    reachable = entt::hashed_string::value("reachable"),
    // global nodes, subject to dead code elimination after the whole file is parsed
    global = entt::hashed_string::value("global"),
    maybe_reachable = entt::hashed_string::value("maybe_reachable"),
    unreachable = entt::hashed_string::value("unreachable"),
};

// TODO: store this on scopes instead
struct scope_visibility final
{
    visibility name;
    tag_storage *pool;
    scope_visibility *prev = nullptr;
};

struct builder final
{
    builder()
        : global{
              // TODO: is this correct?
              .name = visibility::maybe_reachable,
              .pool = &reg.storage<void>((entt::id_type)visibility::maybe_reachable),
          }
    {
    }

    template <typename T>
    inline entt::entity make(T const &t) noexcept
        requires requires {
            { ::make(*this, t) } -> std::same_as<entt::entity>;
        }
    {
        return ::make(*this, t);
    }

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

    // TODO: debug_ensure that the given visibility is below current level
    template <visibility Vis>
    inline void push_vis(scope_visibility &scope)
    {
        scope = {
            .name = Vis,
            .pool = &reg.storage<void>((entt::id_type)Vis),
            .prev = scopes,
        };
        scopes = &scope;
    }

    // TODO: ensure not null
    inline void pop_vis() { scopes = scopes->prev; }

    // TODO: is there any node (other than `Region`) with more than 1 effect dependency?
    entt::registry reg;
    scope_visibility global;
    scope_visibility *scopes = &global;
};

inline entt::entity builder::make(node_op op, std::span<entt::entity const> nins) noexcept
{
    auto const num_ins = nins.size();

    auto const n = reg.create();
    reg.emplace<node_op>(n, op);
    reg.emplace<node_type>(n, type_by_op(op));
    reg.emplace<node_inputs>(n, compress(nins));
    scopes->pool->emplace(n);

    // TODO: optimize
    for (uint32_t i{}; auto id : nins)
    {
        reg.get_or_emplace<users>(id).entries.push_back({n, i++});
    }

    return n;
}

// TODO: this again, but for global nodes
inline void prune_dead_code(builder &bld, entt::entity ret) noexcept
{
    auto &&new_nodes = bld.reg.storage<void>((entt::id_type)visibility::maybe_reachable);
    auto &&visited = bld.reg.storage<void>((entt::id_type)visibility::reachable);

    auto const &ins_storage = bld.reg.storage<node_inputs>();
    auto const &effects = bld.reg.storage<effect>();

    // TODO: cache this storage because why not
    std::vector<entt::entity> to_visit;
    to_visit.push_back(ret);

    while (!to_visit.empty())
    {
        auto const top = to_visit.back();
        to_visit.pop_back();

        // TODO: is this ever the case that this node was visited previously?
        if (visited.contains(top))
            continue;

        // TODO: add other links here as well
        // TODO: maybe just follow the effect and memory chain instead?
        // ^ also you can check here `visited` so that you don't push into the vector, causing a possible reallocation
        // ^ lemma: if there is only one candidate node to add to `to_visit`, you don't need `to_visit` at all
        auto &&ins = ins_storage.get(top).nodes;
        to_visit.insert(to_visit.end(), ins.begin(), ins.end());

        if (effects.contains(top))
            to_visit.push_back(effects.get(top).target);

        visited.emplace(top);
    }

    // TODO: these are unreachable, so maybe leave a warning for significant nodes? (functions, etc.)
    {
        // auto to_cut_view = entt::basic_view{std::tie(new_nodes), std::tie(visited)};
        // TODO: is this correct?
        auto to_cut_view = entt::basic_view{std::tie(bld.reg.storage<entt::entity>()), std::tie(visited)};
        bld.reg.destroy(to_cut_view.begin(), to_cut_view.end());
    }

    // TODO: is this correct?
    {
        auto &&to_cut = bld.reg.storage<void>((entt::id_type)visibility::unreachable);
        bld.reg.destroy(to_cut.begin(), to_cut.end());
    }

    new_nodes.clear();
}
