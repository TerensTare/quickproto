
#pragma once

// TODO:
// - have a common interface for operators, then each sub-type implements its own version
// ^ eg. `add` is `+`, but `int + int` is `iadd`
// - separate float32 from float64
// ^ `float32_bot` and `float64_bot` are two distinct types; same for top, etc.
// - `top` and `bot` should have a common representation for `value_type` types; and `top` should "bubble out" when encountered
// - `value_type.assign`, a non-customizable call that just sets `lhs = rhs`, but unless `lhs` is NOT assignable from `rhs`
// - what is the type of an error?
// - use a custom typeid solution, rather than dynamic cast
// ^ How do you represent subclassing with indices? (eg. how do you know a type is any kind of `int`?)
// ^ but do you really need subclassing?
// ^ value and control flow nodes have disjoint types, so make separate types somehow
// - signed and unsigned integers should be separate; then you only have these types for integers
// ^ then sized integers are for example: uint8 is `uint_range{0, 255}`
// - maybe use a value_stack and index into it when representing types
// - maybe handle operators using a function table, rather than letting the type handle them?

struct type
{
    virtual ~type() {}

    // TODO: T: type
    template <typename T>
    inline T const *as() const noexcept { return dynamic_cast<T const *>(this); }
};

// impossible type (unreachable code, UB, errors, etc.)
struct top final : type
{
    inline static type const *self() noexcept
    {
        static top top;
        return &top;
    }
};

// unknown type (runtime values, etc.)
struct bot final : type
{
    inline static type const *self() noexcept
    {
        static bot bot;
        return &bot;
    }
};
