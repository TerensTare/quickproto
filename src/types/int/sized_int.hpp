
#pragma once

#include <concepts>
#include "types/value.hpp"

// TODO: try doing something better to avoid template instantiations

template <std::integral T>
struct sized_int_ : value
{
};

template <std::integral T>
struct sized_int_top final : sized_int_<T>
{
    inline static value const *self() noexcept
    {
        static sized_int_top top;
        return &top;
    }

    inline value const *phi(value const *other) const noexcept
    {
        // HACK: temporary impl
        if (auto rhs = other->as<sized_int_<T>>())
            return sized_int_top::self();
        else
            return top_value::self();
    }

    inline value const *bcompl() const noexcept { return this; }

    inline value const *band(value const *rhs) const noexcept { return rhs; }
};

template <std::integral T>
struct sized_int_const final : sized_int_<T>
{
    inline explicit sized_int_const(T value) : value{value} {}

    inline value const *phi(value const *other) const noexcept
    {
        // HACK: temporary impl
        if (auto rhs = other->as<sized_int_<T>>())
            return sized_int_top<T>::self();
        else
            return top_value::self();
    }

    T value;
};

template <std::integral T>
struct sized_int_bot final : sized_int_<T>
{
};

template <std::integral T>
struct sized_int_type : type
{
    inline value const *top() const noexcept { return sized_int_top<T>::self(); }
    inline value const *zero() const noexcept { return new sized_int_const<T>{0}; }

    inline char const *name() const noexcept
    {
        if constexpr (std::is_same_v<T, int8_t>)
            return "int8";
        else if constexpr (std::is_same_v<T, int16_t>)
            return "int16";
        else if constexpr (std::is_same_v<T, int32_t>)
            return "int32";
        else if constexpr (std::is_same_v<T, int64_t>)
            return "int64";

        else if constexpr (std::is_same_v<T, uint8_t>)
            return "uint8";
        else if constexpr (std::is_same_v<T, uint16_t>)
            return "uint16";
        else if constexpr (std::is_same_v<T, uint32_t>)
            return "uint32";
        else if constexpr (std::is_same_v<T, uint64_t>)
            return "uint64";
    }
};