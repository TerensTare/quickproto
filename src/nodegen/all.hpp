
#pragma once

#include "nodegen/composite.hpp"

// binary math
#include "nodegen/add.hpp"
#include "nodegen/div.hpp"
#include "nodegen/mul.hpp"
#include "nodegen/sub.hpp"

#include "nodegen/and_xor.hpp"
#include "nodegen/bit_and.hpp"
#include "nodegen/bit_or.hpp"
#include "nodegen/bit_xor.hpp"
#include "nodegen/compl.hpp"

#include "nodegen/shift_left.hpp"
#include "nodegen/shift_right.hpp"

#include "nodegen/logic_and.hpp"
#include "nodegen/logic_or.hpp"

// comparisons
#include "nodegen/eq.hpp"
#include "nodegen/ne.hpp"
#include "nodegen/le.hpp"
#include "nodegen/lt.hpp"
#include "nodegen/ge.hpp"
#include "nodegen/gt.hpp"

// unary math/logic
#include "nodegen/neg.hpp"
#include "nodegen/not.hpp"

// control flow state
#include "nodegen/exit.hpp"
#include "nodegen/loop.hpp"
#include "nodegen/region.hpp"
#include "nodegen/return.hpp"
#include "nodegen/phi.hpp"

// memory state
#include "nodegen/alloca.hpp"
#include "nodegen/cast.hpp"
#include "nodegen/load.hpp"
#include "nodegen/store.hpp"

#include "nodegen/addr.hpp"
#include "nodegen/deref.hpp"

// other
#include "nodegen/call.hpp"
