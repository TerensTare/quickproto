
#pragma once

#include "types/value.hpp"

// TODO: encode supported operators on `Base`

template <std::derived_from<value> Base>
struct top_value final : Base
{
    
};