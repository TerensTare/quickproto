
#pragma once

#include <string_view>

#include <entt/container/dense_map.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>

#include "base.hpp"
#include "types/value.hpp"

// NOTE:
// - when you get a variable, you can get from parent scope; but when you set you only set on this scope
// - you only ever merge (Phi) variables of parent scopes; not types nor functions nor locals of this scope

// TODO:
// - have a push/pop mechanism for the arrays in `env`
// - `get(var)` is always recursive but `set(var)` only affects the topmost environment
// - (maybe) it's better if you just clone the maps instead of using parent links?
// ^ pros:
// ^- no pointer chasing for parent data
// ^- you don't care much about `Phi` and stuff; simply have a pass that reduces links to `Phi(n, n)` to a link to `n`
// ^ cons
// ^- you have to copy everything all the time (maybe you can keep constant stuff separately and have a pointer to there?)
// ^-- to mitigate this, you can have a separate `const_env` for non-mutable stuff and store it as links, everything else is deep copied

struct dual_hash final
{
    using is_transparent = void;

    static constexpr std::size_t operator()(std::string_view str) noexcept
    {
        return (std::size_t)entt::hashed_string::value(str.data(), str.size());
    }

    static constexpr std::size_t operator()(entt::id_type id) noexcept
    {
        return (std::size_t)id;
    }
};

struct dual_cmp final
{
    using is_transparent = void;

    template <typename T>
    static constexpr bool is_ok = std::is_same_v<T, std::string_view> or std::is_same_v<T, entt::id_type>;

    template <typename L, typename R>
        requires(is_ok<std::remove_cvref_t<L>> and is_ok<std::remove_cvref_t<R>>)
    static constexpr bool operator()(L lhs, R rhs) noexcept
    {
        return dual_hash{}(lhs) == dual_hash{}(rhs);
    }
};

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
    inline name_index get_name(std::string_view name) const noexcept
    {
        auto const hash = entt::hashed_string::value(name.data(), name.size());
        return get_impl(hash);
    }

    // TODO(maybe): move constant stuff to a separate part; they never get merged
    // ^ never mind, just merge everything to one map, implement `value_type.assign` and let it handle everything
    // ^ but in that case, how do you distinguish between struct types and struct instances? (types implement `value_type.construct`)

    // (name -> index) mapping; if not MSB it's a value, if MSB it's a type
    entt::dense_map<std::string_view, name_index, dual_hash, dual_cmp> table;
    scope *prev = nullptr;

private:
    inline name_index get_impl(entt::id_type hash) const noexcept
    {
        auto iter = table.find(hash);
        if (iter != table.end())
            return iter->second;
        if (prev)
            return prev->get_impl(hash);

        // TODO: create a placeholder node instead with a `resolve` tag that signals the type checker to resolve this later
        return name_index::missing;
    }
};

struct env final
{
    // TODO: probably best to remove this from here
    inline auto get_name(std::string_view name) const noexcept { return top->get_name(name); }

    inline auto type(name_index name) const noexcept
    {
        return types[(uint32_t)name & ~(uint32_t)name_index::type_mask];
    }

    // rename these since now it's not only variables; also make sure scoping rules still work
    inline entt::entity get_var(std::string_view name) const noexcept
    {
        auto const n = top->get_name(name);
        return n != name_index::missing
                   ? values[(uint32_t)n]
                   : entt::null;
    }

    inline void new_value(std::string_view name, entt::entity id) noexcept
    {
        auto iter = top->table.find(name);
        ensure(iter == top->table.end(), "Name redeclared in block!");

        auto const n = values.size();
        top->table.insert({name, name_index(n)});
        values.push_back(id);
    }

    inline void new_type(std::string_view name, value_type const *ty) noexcept
    {
        auto iter = top->table.find(name);
        ensure(iter == top->table.end(), "Name redeclared in block!");

        auto const n = types.size();
        top->table.insert({name, type_index(n)});
        types.push_back(ty);
    }

    inline void set_var(std::string_view name, entt::entity id) noexcept
    {
        // semantics: check if existing, then write to this env
        ensure(get_var(name) != entt::null, "Variable assigned to before declaration!");

        auto const n = values.size();
        top->table.insert_or_assign(name, name_index(n));
        values.push_back(id);
    }

    // TODO: use a non-shrinking page_vector here
    std::vector<value_type const *> types;
    std::vector<entt::entity> values; // (name -> (mem_state, type)) mapping

    scope *top;
};
