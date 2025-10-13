
#pragma once

#include "types/bool.hpp"

struct flow_type : type
{
    // TODO: is there any other type except `bool_` that might be useful here?
    virtual flow_type const *branch(bool_ const *cond) const noexcept = 0;

    // TODO:
    // - flow_type const *merge() that determines the type of a `Region`
};

// unreachable node
struct not_ctrl final : flow_type
{
    inline static flow_type const *self() noexcept
    {
        static not_ctrl c;
        return &c;
    }

    // unreachable code is always unreachable, no matter the branches you make of it
    inline flow_type const *branch(bool_ const *cond) const noexcept { return this; }
};

// reachable node
struct ctrl final : flow_type
{
    inline static flow_type const *self() noexcept
    {
        static ctrl c;
        return &c;
    }

    inline flow_type const *branch(bool_ const *cond) const noexcept;
};

inline flow_type const *ctrl::branch(bool_ const *cond) const noexcept
{
    // if the condition can be evaluated at compile time and is `true`, this branch is reachable
    if (auto rhs = cond->as<bool_const>(); !rhs || rhs->b)
        return this;
    // otherwise, this branch is unreachable
    return not_ctrl::self();
}