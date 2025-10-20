
#pragma once

#include <concepts>

// TODO: push/pop + raii version

// The node of a stack-stored list; this can have a `null` head as opposed to `stacklist`
template <typename T>
struct stacklist final
{
    T value;
    stacklist *prev = nullptr;
};
