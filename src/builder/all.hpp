
#pragma once

#include "builder/binary_math.hpp"
#include "builder/compare.hpp"
#include "builder/control_flow.hpp"
#include "builder/logic.hpp"
#include "builder/unary.hpp"
#include "builder/value.hpp"

// TODO:
// - figure out how to have runtime-customizable peepholes with this.
// - do you need to tail-call `make` on nodes where peephole produced a new node?
