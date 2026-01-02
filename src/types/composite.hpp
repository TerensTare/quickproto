
#pragma once

#include "types/int.hpp"

struct composite_value : value
{
};

struct composite_type : type
{
    inline explicit composite_type(size_t n_members) noexcept : n_members{n_members} {}

    // TODO: probably should pass the number of values here so that you can error in case there are more than expected
    // brace-init the type with the given values. The number of values must match to the number of members (ie. if less, the "missing" ones should be zero-init before being passed here)
    virtual composite_value const *init(value const **values) const noexcept = 0;

    size_t n_members;
};

struct too_much_init_values final : value_error
{
};