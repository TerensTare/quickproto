
#pragma once

#include <cstdint>

// TODO:
// - use sized strings; not null-terminatec

union value final
{
    int64_t i64;
    char const *str;
};
