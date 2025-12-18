
#pragma once

// TODO: move each type to the corresponding value definition
// TODO: `type` should keep track of the name, not `value`

struct value;

// TODO: see if you can reduce this to a struct rather than a vtable
struct type
{
    virtual ~type() {}

    // the "unknown" representation of the type; nothing can be assumed about this value
    virtual value const *top() const noexcept = 0;

    template <typename T>
    inline type const *as() const noexcept
    {
        static_assert(std::derived_from<T, decltype(auto(*this))>);
        return dynamic_cast<T const *>(this);
    }
};
