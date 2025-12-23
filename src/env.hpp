
#pragma once

#include <entt/container/dense_map.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>

#include "base.hpp"
#include "token.hpp"

#include "types/type.hpp"

// NOTE:
// - when you get a variable, you can get from parent scope; but when you set you only set on this scope
// - you only ever merge (Phi) variables of parent scopes; not types nor functions nor locals of this scope

// TODO:
// - remove alias class from here
// - interned strings are sequential for now, so find a way to exploit it on scope storage
// - have a push/pop mechanism for the arrays in `env`
// - `get(var)` is always recursive but `set(var)` only affects the topmost environment
// - (maybe) it's better if you just clone the maps instead of using parent links?
// ^ pros:
// ^- no pointer chasing for parent data
// ^- you don't care much about `Phi` and stuff; simply have a pass that reduces links to `Phi(n, n)` to a link to `n`
// ^ cons
// ^- you have to copy everything all the time (maybe you can keep constant stuff separately and have a pointer to there?)
// ^-- to mitigate this, you can have a separate `const_env` for non-mutable stuff and store it as links, everything else is deep copied

enum class name_index : uint32_t
{
    type_mask = 1u << 31,
    missing = ~uint32_t{},
};

constexpr bool is_type(name_index n)
{
    return n != name_index::missing && n >= name_index::type_mask;
}

constexpr bool is_value(name_index n) { return n < name_index::type_mask; }

constexpr name_index type_index(uint32_t t) { return name_index((uint32_t)name_index::type_mask | t); }

struct scope final
{
    inline name_index get_name(hashed_name name) const noexcept
    {
        auto iter = table.find(name);
        if (iter != table.end())
            return iter->second;
        if (prev)
            return prev->get_name(name);

        // TODO: create a placeholder node instead with a `resolve` tag that signals the type checker to resolve this later
        return name_index::missing;
    }

    // alias_class next_alias{.parent = parent_memory::Function, .tag = 0};
    // (name -> index) mapping; if not MSB it's a value, if MSB it's a type
    entt::dense_map<hashed_name, name_index, token_hash> table;
    scope *prev = nullptr;
};

// TODO: you probably want to push `Program`, `Global`, etc. to the global env for ease of access
struct env final
{
    // TODO: probably best to remove this from here
    inline auto get_name(hashed_name name) const noexcept { return top->get_name(name); }

    // TODO: is this needed? (branches, loops?)
    inline auto get_mem(name_index name) const noexcept { return value_memory[(uint32_t)name]; }

    inline auto get_type(name_index name) const noexcept
    {
        return types[(uint32_t)name & ~(uint32_t)name_index::type_mask];
    }

    // rename these since now it's not only variables; also make sure scoping rules still work
    inline entt::entity get_var(hashed_name name) const noexcept
    {
        auto const n = top->get_name(name);
        return n != name_index::missing
                   ? values[(uint32_t)n]
                   : entt::null;
    }

    // TODO: how does this handle the heap case?
    inline void new_value(hashed_name name, entt::entity id) noexcept
    {
        auto iter = top->table.find(name);
        ensure(iter == top->table.end(), "Name redeclared in block!");

        auto const n = values.size();
        top->table.insert({name, name_index(n)});
        values.push_back(id);
        value_memory.push_back(id); // TODO: is this correct?
        // alias.push_back(top->next_alias);
        // top->next_alias.tag++;
    }

    inline void new_type(hashed_name name, type const *ty) noexcept
    {
        auto iter = top->table.find(name);
        ensure(iter == top->table.end(), "Name redeclared in block!");

        auto const n = types.size();
        top->table.insert({name, type_index(n)});
        types.push_back(ty);
    }

    // TODO: is this needed? (branches, loops?)
    inline void set_value(name_index name, entt::entity id) noexcept
    {
        // old semantics: check if existing, then write to this env
        // current semantics: write to the value's index
        // ^ TODO: are these semantics correct?
        ensure(name < name_index::type_mask, "Variable assigned to before declaration!");

        value_memory[(uint32_t)name] = id;
    }

    // TODO: use a non-shrinking page_vector here
    // TODO: if using a page vector, you might as well use a page list instead and make it implicit (no reallocations, inline the `next` in the end of the array)
    // TODO: you can still cut some values out from time to time (function scopes)
    std::vector<type const *> types;
    std::vector<entt::entity> values; // (name -> value) mapping
    // TODO: this should be per field
    std::vector<entt::entity> value_memory; // (name -> memory_state) mapping

    scope *top;
};
