
#pragma once

template <typename... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};