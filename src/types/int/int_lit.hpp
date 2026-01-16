
#pragma once

#include <cstdint>
#include <entt/container/dense_map.hpp>

#include "types/covariant_helper.hpp"
#include "types/float.hpp"
#include "types/int/sized_int.hpp"

// HACK: do something better
struct invalid_cast final : value_error
{
    inline invalid_cast(value const *val, type const *target) noexcept : val{val}, target{target} {}

    value const *val;
    type const *target;
};

struct int_value : covariant_helper<int_value>
{
    inline static int_value const *top() noexcept;
    // TODO: cache the constants (do you have to?)
    inline static int_value *make(uint64_t n) noexcept;
    inline static int_value const *bot() noexcept;

    inline static void operator delete(int_value *val, std::destroying_delete_t) noexcept;

    constexpr bool is_const() const noexcept final { return level == 1; }

    constexpr int_value const *bcompl() const noexcept final;
    constexpr int_value const *neg() const noexcept final;

    constexpr int_value const *add(int_value const *rhs) const noexcept;
    constexpr int_value const *sub(int_value const *rhs) const noexcept;
    constexpr int_value const *mul(int_value const *rhs) const noexcept;
    constexpr int_value const *div(int_value const *rhs) const noexcept;

    constexpr int_value const *band(int_value const *rhs) const noexcept;
    constexpr int_value const *bor(int_value const *rhs) const noexcept;
    constexpr int_value const *bxor(int_value const *rhs) const noexcept;
    constexpr int_value const *lsh(int_value const *rhs) const noexcept;
    constexpr int_value const *rsh(int_value const *rhs) const noexcept;

    constexpr value const *eq(int_value const *rhs) const noexcept;
    // constexpr value const *ne(int_value const *rhs) const noexcept;
    constexpr value const *lt(int_value const *rhs) const noexcept;
    // constexpr value const *le(int_value const *rhs) const noexcept;
    // constexpr value const *gt(int_value const *rhs) const noexcept;
    // constexpr value const *ge(int_value const *rhs) const noexcept;

    constexpr int_value const *phi(int_value const *rhs) const noexcept;

protected:
    explicit constexpr int_value(uint8_t level) : level{level} {}

private:
    uint8_t level; // top, const or bot
};

// TODO: rename this
struct sint_type final : type
{
    inline value const *top() const noexcept { return int_value::top(); }
    inline value const *zero() const noexcept;

    inline char const *name() const noexcept { return "<integer>"; }
};

struct int_top final : int_value
{
    constexpr int_top() : int_value{0} {}
};

struct int_const final : int_value
{
    explicit constexpr int_const(uint64_t n) noexcept : int_value{1}, n{n} {}

    inline value const *cast(type const *target) const noexcept
    {
        if (target->as<float64_type>())
            return new float64{(double)n};
        else if (target->as<sint_type>())
            return this;
            // intN
        else if (target->as<sized_int_type<int8_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<int8_t>();
        else if (target->as<sized_int_type<int16_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<int16_t>();
                else if (target->as<sized_int_type<int32_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<int32_t>();
        else if (target->as<sized_int_type<int64_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<int64_t>();
            // uintN
        else if (target->as<sized_int_type<uint8_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<uint8_t>();
        else if (target->as<sized_int_type<uint16_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<uint16_t>();
                else if (target->as<sized_int_type<uint32_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<uint32_t>();
        else if (target->as<sized_int_type<uint64_t>>())
            // HACK: propagate const-ness if possible
            return new sized_int_top<uint64_t>();
        else
            return new invalid_cast{this, target};
    }

    uint64_t n;
};

struct int_bot final : int_value
{
    constexpr int_bot() : int_value{2} {}
};

inline value const *sint_type::zero() const noexcept { return int_value::make(0); }

inline int_value const *int_value::top() noexcept
{
    static int_top top;
    return &top;
}

inline int_value *int_value::make(uint64_t n) noexcept
{
    // TODO: use something more optimal, remove caching once you get "in-place" types
    // static entt::dense_map<uint64_t, std::unique_ptr<int_const const>, std::identity> cache;
    // if (auto iter = cache.find(n); iter != cache.end())
    //     return (int_value *)iter->second.get();
    // return (int_value *)cache.insert({n, std::unique_ptr<int_const>(new int_const{n})}).first->second.get();

    return new int_const{n};
}

inline int_value const *int_value::bot() noexcept
{
    static int_bot bot;
    return &bot;
}

inline void int_value::operator delete(int_value *val, std::destroying_delete_t) noexcept
{
    switch (val->level)
    {
    case 0:
        static_cast<int_top *>(val)->~int_top();
        ::operator delete(val, sizeof(int_top));
        break;

    case 1:
        static_cast<int_const *>(val)->~int_const();
        ::operator delete(val, sizeof(int_const));
        break;

    case 2:
        static_cast<int_bot *>(val)->~int_bot();
        ::operator delete(val, sizeof(int_bot));
        break;

    default:
        std::unreachable();
    }
}

#define int_op(op, lhs, rhs) \
    int_value::make(static_cast<int_const const *>(this)->n op static_cast<int_const const *>(rhs)->n)

#define int_cmp_op(op, lhs, rhs) \
    bool_const::make(static_cast<int_const const *>(this)->n op static_cast<int_const const *>(rhs)->n)

#define iadd(lhs, rhs) int_op(+, lhs, rhs)
#define isub(lhs, rhs) int_op(-, lhs, rhs)
#define imul(lhs, rhs) int_op(*, lhs, rhs)
#define idiv(lhs, rhs) int_op(/, lhs, rhs)
#define iband(lhs, rhs) int_op(&, lhs, rhs)
#define ibor(lhs, rhs) int_op(|, lhs, rhs)
#define ixor(lhs, rhs) int_op(^, lhs, rhs)
#define ilsh(lhs, rhs) int_op(<<, lhs, rhs)
#define irsh(lhs, rhs) int_op(>>, lhs, rhs)
#define ine(lhs, rhs) int_cmp_op(+, lhs, rhs)
#define ieq(lhs, rhs) int_cmp_op(+, lhs, rhs)
#define ilt(lhs, rhs) int_cmp_op(+, lhs, rhs)
#define ile(lhs, rhs) int_cmp_op(+, lhs, rhs)
#define igt(lhs, rhs) int_cmp_op(+, lhs, rhs)
#define ige(lhs, rhs) int_cmp_op(+, lhs, rhs)

#define iphi(lhs, rhs) [](auto l, auto r) { return (l->n == r->n) ? (int_value const *)l : int_value::top(); }(static_cast<int_const const *>(lhs), static_cast<int_const const *>(rhs))

GENERATE_BINARY_JUMP_TABLE(int_value, int_value, add, iadd);
GENERATE_BINARY_JUMP_TABLE(int_value, int_value, sub, isub);
GENERATE_BINARY_JUMP_TABLE(int_value, int_value, mul, imul);
GENERATE_BINARY_JUMP_TABLE(int_value, int_value, div, idiv);

GENERATE_BINARY_JUMP_TABLE(int_value, int_value, band, iband);
GENERATE_BINARY_JUMP_TABLE(int_value, int_value, bor, ibor);
GENERATE_BINARY_JUMP_TABLE(int_value, int_value, bxor, ixor);
GENERATE_BINARY_JUMP_TABLE(int_value, int_value, lsh, ilsh);
GENERATE_BINARY_JUMP_TABLE(int_value, int_value, rsh, irsh);

constexpr value const *int_value::eq(int_value const *rhs) const noexcept
{
    switch (level * 3 + rhs->level)
    {
    case 0:
    case 1:
    case 3:
        return bool_top::self();
    case 4:
        return bool_const::make(static_cast<int_const const *>(this)->n == static_cast<int_const const *>(rhs)->n);
    case 2:
    case 5:
    case 6:
    case 7:
    case 8:
        return bool_bot::self();
    default:
        std::unreachable();
    }
}

constexpr value const *int_value::lt(int_value const *rhs) const noexcept
{
    switch (level * 3 + rhs->level)
    {
    case 0:
    case 1:
    case 3:
        return bool_top::self();
    case 4:
        return bool_const::make(static_cast<int_const const *>(this)->n < static_cast<int_const const *>(rhs)->n);
    case 2:
    case 5:
    case 6:
    case 7:
    case 8:
        return bool_bot::self();
    default:
        std::unreachable();
    }
}

GENERATE_BINARY_JUMP_TABLE(int_value, int_value, phi, iphi);

constexpr int_value const *int_value::bcompl() const noexcept
{
    switch (level)
    {
    case 0:
        return int_value::top();
    case 1:
        return int_value::make(~static_cast<int_const const *>(this)->n);
    case 2:
        return int_value::bot();
    }
}

constexpr int_value const *int_value::neg() const noexcept
{
    switch (level)
    {
    case 0:
        return int_value::top();
    case 1:
        return int_value::make(-static_cast<int_const const *>(this)->n);
    case 2:
        return int_value::bot();
    }
}