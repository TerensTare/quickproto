
#pragma once

#include <entt/entity/registry.hpp>

#include "base.hpp"
#include "nodes.hpp"

// TODO:
// - do not cut error nodes during DCE

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
    inline builder()
        : global{
              // TODO: is this correct?
              .name = visibility::maybe_reachable,
              .pool = &reg.storage<void>((entt::id_type)visibility::maybe_reachable),
          }
    {
    }

    // invariant: The returned entity has the following components:
    // `node`
    // `node_type`
    // `node_inputs`
    // invariant: `users` is updated for any entity in `nins`
    inline entt::entity make(value const *val, node_op op, std::span<entt::entity const> nins) noexcept;

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

    inline void report_errors();

    // TODO: is there any node (other than `Region`) with more than 1 effect dependency?
    entt::registry reg;
    scope_visibility global;
    scope_visibility *scopes = &global;

    struct func_state
    {
        entt::entity func; // the `Start` node of the function
        entt::entity ctrl;
        entt::entity mem;
        uint32_t next_slot = 0;
    };

    // Push a new function state, return the old one so you can reset it
    // TODO: pass the function name here
    [[nodiscard]]
    inline func_state new_func() noexcept
    {
        // TODO: recheck this
        auto const ns = make(top_type::self(), node_op::Start, {});
        reg.emplace<mem_effect>(ns) = {
            .prev = glob_mem,
            .target = glob_mem,
            // TODO: figure out the tag here from the scope or somewhere
            .tag = state.next_slot++,
        };
        // TODO: add read/write tag

        return std::exchange(
            state, func_state{
                       .func = ns,
                       .ctrl = ns,
                       .mem = ns,
                       .next_slot = 0,
                   });
    }

    entt::entity pkg_mem;  // general package memory
    entt::entity glob_mem; // global memory (not heap)
    func_state state;
};

inline entt::entity builder::make(value const *val, node_op op, std::span<entt::entity const> nins) noexcept
{
    auto const n = reg.create();
    reg.emplace<node_op>(n, op);
    reg.emplace<node_type>(n, val);
    reg.emplace<node_inputs>(n, compress(nins));
    scopes->pool->emplace(n);

    // TODO: optimize
    for (uint32_t i{}; auto id : nins)
    {
        reg.get_or_emplace<users>(id).entries.push_back({n, i++});
    }

    return n;
}

inline void builder::report_errors()
{
    for (auto &&[id, ty] : reg.view<error_node const, node_type const>().each())
    {
        // HACK
        // TODO: structured error messages
        // TODO: enable these back
        if (auto b = ty.type->as<binary_op_not_implemented_type>())
        {
            // std::println("Cannot do {} {} {}!", b->lhs->name(), b->op, b->rhs->name());
        }
        else if (auto u = ty.type->as<unary_op_not_implemented_type>())
        {
            // std::println("Cannot do {}{}!", u->op, u->sub->name());
        }
        else
        {
            fail("Internal compiler error: unhandled error type");
        }
    }

    reg.clear<error_node>();
}
