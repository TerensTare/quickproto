
#pragma once

#include "nodes.hpp"

// TODO:
// - make a better interface out of this (maybe `compile` should return some formattable object that is then formatted internally and written to a file?)
// ^ (maybe) you need `compile(node)` and `compile(edge)` instead, along with other event calls?
struct backend
{
    virtual ~backend() {}

    virtual void compile(FILE *out, entt::registry const &reg) = 0;
};