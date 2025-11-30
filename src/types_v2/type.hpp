
#pragma once

#include <string_view>
#include <span>

#include <entt/entity/entity.hpp>
#include <entt/container/dense_map.hpp>

// work in progress
// NOTE: qualifiers can only appear on type aliases, or params/members/variables (values), not type declarations (again, except alias)

struct value_id final
{
    uint32_t is_type : 1 = 0;
    uint32_t n : 31;
};

struct type_id final
{
    uint32_t is_type : 1 = 1;
    uint32_t n : 31;
};

union symbol_id final
{
    uint32_t is_type : 1;
    value_id value;
    type_id type;
};

static_assert(sizeof(symbol_id) == sizeof(uint32_t));

struct member_info final
{
    std::string_view name;
    type_id type;
};

struct type_spec
{
    virtual ~type_spec() {}
};

struct struct_type final : type_spec
{
    inline explicit struct_type(size_t n_members, std::unique_ptr<type_id[]> members) noexcept
        : n_members{n_members}, members(std::move(members)) {}

    size_t n_members;
    std::unique_ptr<type_id[]> members;
};

struct array_type final : type_spec
{
    inline explicit array_type(type_id base, size_t n) noexcept
        : base{base}, n{n} {}

    type_id base;
    size_t n;
};

struct value_spec final
{
    entt::entity node;
    type_id type;
};

struct scope final
{
    inline type_id define(std::string_view name, std::unique_ptr<type_spec> spec);
    inline value_id define(std::string_view name, value_spec &&spec);

    // TODO: keep a vector of errors
    // TODO: dual hash, useful for recursive lookups
    // lifetime invariant: this lives as long as the text file is open
    // ^ TODO: what about types from other packages?
    entt::dense_map<std::string_view, symbol_id> indices;
    std::vector<std::unique_ptr<type_spec>> types;
    std::vector<value_spec> values;
    scope const *prev = nullptr; // TODO: does this ever need to be mutable?
};

inline type_id scope::define(std::string_view name, std::unique_ptr<type_spec> spec)
{
    auto const id = type_id{.n = (uint32_t)types.size()};
    // TODO: check for redeclarations
    indices.insert({name, {.type = id}});
    types.push_back(std::move(spec));
    return id;
}

inline value_id scope::define(std::string_view name, value_spec &&spec)
{
    auto const id = value_id{.n = (uint32_t)values.size()};
    // TODO: check for redeclarations
    indices.insert({name, {.value = id}});
    values.push_back(std::move(spec));
    return id;
}

struct env final
{
    // TODO: how do you know which scope the id is from?
    inline symbol_id get(std::string_view name) const;

    scope global;
    scope const *top = &global;
};

inline symbol_id env::get(std::string_view name) const
{
    for (auto iter = top; iter; iter = iter->prev)
    {
        if (auto find = iter->indices.find(name); find != iter->indices.end())
            return find->second;
    }

    // TODO: denote invalid id-s
    return (symbol_id)~uint32_t{};
}