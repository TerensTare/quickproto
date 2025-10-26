
#pragma once

#include <memory>
#include <span>

#include <entt/entity/fwd.hpp>
#include "utils/stacklist.hpp"

// TODO: rename to `dynarray`

struct smallvec final
{
    inline entt::entity &operator[](size_t i) noexcept { return entries[i]; }
    inline entt::entity const &operator[](size_t i) const noexcept { return entries[i]; }

    inline entt::entity const *begin() const noexcept { return entries.get(); }
    inline entt::entity const *end() const noexcept { return entries.get() + n; }

    // HACK: for now, this is just size + unique_ptr<entity[]>
    size_t n;
    std::unique_ptr<entt::entity[]> entries;
};

// Transform a `std::span` (vector/array/etc) to a smallvec
inline smallvec compress(std::span<entt::entity const> vec) noexcept
{
    auto entries = std::make_unique_for_overwrite<entt::entity[]>(vec.size());
    std::copy_n(vec.data(), vec.size(), entries.get());

    return {.n = vec.size(), .entries = std::move(entries)};
}

// Transform a `stacklist` to a smallvec. Make sure `n` is the number of elements you want to copy to your new smallvec
inline smallvec compress(stacklist<entt::entity> const *list, size_t n) noexcept
{
    auto entries = std::make_unique_for_overwrite<entt::entity[]>(n);
    for (size_t i{}; i < n; ++i)
    {
        entries[i] = list->value;
        list = list->prev;
    }

    return {.n = n, .entries = std::move(entries)};
}
