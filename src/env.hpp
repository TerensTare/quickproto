
#pragma once

#include <string_view>

#include <entt/container/dense_map.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>

#include "base.hpp"

// NOTE:
// - when you get a variable, you can get from parent scope; but when you set you only set on this scope
// - you only ever merge (Phi) variables of parent scopes; not types nor functions nor locals of this scope

// TODO:
// - `get(var)` is always recursive but `set(var)` only affects the topmost environment
// - (maybe) it's better if you just clone the maps instead of using parent links?
// ^ pros:
// ^- no pointer chasing for parent data
// ^- you don't care much about `Phi` and stuff; simply have a pass that reduces links to `Phi(n, n)` to a link to `n`
// ^ cons
// ^- you have to copy everything all the time (maybe you can keep constant stuff separately and have a pointer to there?)
// ^-- to mitigate this, you can have a separate `const_env` for non-mutable stuff and store it as links, everything else is deep copied

struct type;

struct dual_hash final
{
    using is_transparent = void;

    constexpr std::size_t operator()(std::string_view str) const noexcept
    {
        return (std::size_t)entt::hashed_string::value(str.data(), str.size());
    }

    constexpr std::size_t operator()(entt::id_type id) const noexcept
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
    constexpr bool operator()(L lhs, R rhs) const noexcept
    {
        return dual_hash{}(lhs) == dual_hash{}(rhs);
    }
};

struct scope final
{
    // TODO(maybe): move constant stuff to a separate part; they never get merged
    // ^ never mind, just merge everything to one map, implement `value_type.assign` and let it handle everything
    // ^ but in that case, how do you distinguish between struct types and struct instances? (types implement `value_type.construct`)
    entt::dense_map<std::string_view, entt::entity, dual_hash, dual_cmp> defs; // (name -> (mem_state, type)) mapping
    entt::dense_map<std::string_view, value_type const *, dual_hash, dual_cmp> types;
    scope *parent = nullptr;
};

struct env final
{
    // rename these since now it's not only variables; also make sure scoping rules still work
    inline entt::entity get_var(std::string_view name) const noexcept
    {
        auto const hash = entt::hashed_string::value(name.data(), name.size());
        return get_var_impl(top, hash);
    }

    inline void new_var(std::string_view name, entt::entity id) noexcept
    {
        ensure(top->defs.insert({name, id}).second, "Variable redeclaration in block!");
    }

    inline void set_var(std::string_view name, entt::entity id) noexcept
    {
        // semantics: check if existing, then write to this env
        ensure(get_var(name) != entt::null, "Variable assigned to before declaration!");
        top->defs.insert_or_assign(name, id);
    }

    scope *top;

private:
    inline static entt::entity get_var_impl(scope *s, entt::id_type hash) noexcept
    {
        // assuming s is not null
        do
        {
            auto iter = s->defs.find(hash);
            if (iter != s->defs.end())
                return iter->second;
            s = s->parent;
        } while (s);

        return entt::null;
    }
};
