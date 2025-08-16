
#pragma once

#include <print>

[[noreturn]]
inline void fail(char const *msg) noexcept
{
    std::print("{}", msg);
    __debugbreak(); // TODO: do something cross-platform
    std::exit(-1);
}

#define ensure(x, msg) ensure_impl(x, msg)
#define ensure_impl(x, msg) \
    do                      \
    {                       \
        if (!(x))           \
        {                   \
            ::fail(msg);    \
        }                   \
    } while (0)