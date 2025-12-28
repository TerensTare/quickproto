
#pragma once

#include <print>
#include <stacktrace>

using uchar = unsigned char;

// TODO: get rid of this eventually
[[noreturn]]
inline void fail(char const *msg, std::stacktrace st = std::stacktrace::current()) noexcept
{
    std::print("{}\nStacktrace: {}", msg, st);
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