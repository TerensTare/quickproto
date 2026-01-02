
#pragma once

#include <memory>
#include <span>

#include <entt/entity/fwd.hpp>

#include "utils/function_ref.hpp"
#include "utils/stacklist.hpp"

// TODO: rename to `dynarray`
// TODO: generic smallvec

template <typename T>
    requires std::is_trivial_v<T>
struct smallvec final
{
    inline static smallvec<T> gen(size_t n, function_ref<T(size_t)> fn)
    {
        auto entries = new T[n];
        for (size_t i{}; i < n; ++i)
            entries[i] = fn(i);
        return {.n = n, .entries = std::unique_ptr<T[]>(entries)};
    }

    inline T &operator[](size_t i) noexcept { return entries[i]; }
    inline T const &operator[](size_t i) const noexcept { return entries[i]; }

    inline T const *begin() const noexcept { return entries.get(); }
    inline T const *end() const noexcept { return entries.get() + n; }

    // HACK: for now, this is just size + unique_ptr<T[]>
    size_t n;
    std::unique_ptr<T[]> entries;
};

// Transform a `std::span` (vector/array/etc) to a smallvec
template <typename T>
inline smallvec<T> compress(std::span<T const> vec) noexcept
{
    auto const n = vec.size();
    auto entries = new T[n];
    std::copy_n(vec.data(), n, entries);

    return {.n = n, .entries = std::unique_ptr<T[]>(entries)};
}

// Transform a `stacklist` to a smallvec. Make sure `n` is the number of elements you want to copy to your new smallvec
template <typename T>
inline smallvec<T> compress(stacklist<T> const *list, size_t n) noexcept
{
    auto entries = std::make_unique_for_overwrite<T[]>(n);
    for (size_t i{}; i < n; ++i)
    {
        entries[n - i - 1] = list->value;
        list = list->prev;
    }

    return {.n = n, .entries = std::move(entries)};
}
