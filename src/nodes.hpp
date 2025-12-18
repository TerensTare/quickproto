
#pragma once

#include <format>
#include <vector>

#include <entt/entity/entity.hpp>

#include "types/int.hpp"
#include "utils/smallvec.hpp"

// TODO:
// - projections should have names; the projection of a function is a parameter, the projections of program memory are heap, global, local (functions)
// - `CallStatic` and `PureCallStatic`; the second one doesn't have an `effect` link
// - `checked_index` by default, which can be lowered to `index` iff `index < len` is `true`
// - optimization: Loads at an index with known type are just a `const` node
// - no need for sized math operations, just generate a `truncate` (& ~0) node as needed and just have `Iadd`/`Uadd`/similar
// ^ you probably still need `F32add` and `F64add` + similar due to memory layout of floats
// ^ also for uint you don't need an "expand" node, but for  sint/floats you probably need an "s/fextendMtoN" instruction
// - `delay_typecheck` component to resolve out-of-order declarations and such
// - evaluate the type of every node when parsing, then `const_fold` would simply check `type.is_known` and replace the node with `Const(type)`
// - (maybe) it's a good idea to mark `Return` nodes with a component, so you can later easily jump to them to cut unused functions
// - merge non-basic edges into a single component type, interpreted based on `node_op`
// - multi-assign + multi-declare variables
// - store nodes by hash (hash=id, but based on node kind, etc.) - called hash-cons; it also helps with constant pooling
// - Addr is just a `Proj` now; address this
// - when visualizing, show the type of the node with `tooltip=\"\"`
// - drop the registry for a list of storages
// - add a `light_entity<Ts...>` to enforce component invariants
// - local variables do not need `Load`/`Store`, but globals/members do

// DONE:
// - [x] encode flow information in nodes such as `if`, etc.
// - [x] nodes need a custom deleter (cannot just do `delete node`)
// - [x] store special edges separately in a map of linked-list mix
// - [x] `Start` node to be specified on `load`/`store` nodes to avoid reordering
// - [x] export graph to dot format
// - [x] separate node from builder in files

// TODO: can you fit all ops in a uint8_t?
// TODO: specify `Out` links of each node (ie. can you link a ctrl, memory, data edge to this node?)

// forward declaration
struct value;

// component
enum class node_op : uint8_t
{
    Program, // Program - a no-op node for preserving ordering between loads/stores and for representing side effects. Indicates the beginning of a program + the program's memory (stack + heap)
             // ^ this is the "local memory" node of a function (in terms of memory) + the "function start" in terms of control flow
    // TODO: Package node
    Global, // Global - like `Program` but at global level only (ie. this does not include local/heap allocations or local control flow)
    Start,  // Start - like `Program` but for a function

    Load,   // Load In=[somePlace, indexNode; memState]
    Store,  // Store In=[dstPlace, indexNode, srcPlace; memState]
    Proj,   // Proj Value=[i] In=[multiNode]
    Alloca, // Alloca - create a known-sized object of node's type in the stack
    Return, // Return Value=[n] In=[ctrl, returnNode x n]
    Exit,   // Exit Value=[n]

    IfYes,  // IfYes In=[ctrlNode, condNode]
    IfNot,  // IfNot In=[ctrlNode, condNode]
    Loop,   // Loop In=[ctrlNode] Out=[IfYes, IfNot]
    Region, // Region - a merge point for multiple control flows
    Phi,    // Phi In=[regionNode, dataNode x len(regionIn)]

    UnaryNeg,
    UnaryNot,

    Add,
    Sub,
    Mul,
    Div,

    // boolean logic
    LogicAnd,
    LogicOr,

    // bit logic
    BitAnd,
    BitXor,
    BitOr,

    // floating point operations
    Fadd,
    Fsub,
    Fmul,
    Fdiv,

    // TODO: simplify these to just < and ==
    CmpEq,
    CmpNe,
    CmpLt,
    CmpLe,
    CmpGt,
    CmpGe,

    // TODO: consistent naming (either `CallStatic` or `StaticCall`, pick one)
    CallStatic, // CallStatic In=[mem, args..., func_to_call] Out=[mem, result...]
    ExternCall, // ExternCall In=[ctrl, mem, args..., func_to_call] Out=[ctrl, mem, result...]

    Cast, // Cast In=[mem, expr] Type=[TargetType] Out=[mem, exprCastedToTargetType]

    Deref, // Deref Value=derefableExpr
    Addr,  // Addr Value=addressableExpr

    IConst, // IConst Value=someInt
    FConst, // FConst Value=someFloat
    BConst, // BConst Value=someBool
    Error,  // TODO: temporary hack
};

// component
// HACK: use a non-polymorphic C++ type to represent node type
struct node_type final
{
    value const *type = nullptr;
};

struct users final
{
    struct entry final
    {
        entt::entity id;
        std::uint32_t index; // index in the `node_inputs` array
    };

    std::vector<entry> entries;
};

// component
// TODO: nodes have a fixed number of inputs (except maybe call when optimized?) so you can use a `dynamic_array<T>` here probably with a small buffer for math nodes
struct node_inputs final
{
    smallvec nodes;
};

// component
// (id) -> effect{target} means that `id` affects the `target` node
// used for nodes that affect control flow
struct ctrl_effect final
{
    entt::entity target;
};

// component
// used for nodes that affect memory, such as stores
/*
    - ProgramMemory (only for executables?)
        -> PackageMemory (Top)
            -> HeapMemory (Heap)
            -> StackMemory
                -> StaticMemory (Global)
                -> FunctionMemory (Start/Local)
    ^ then when an executable is compiled, all `PackageMemory` is propagated to `ProgramMemory` for whole program optimizations
*/
struct mem_effect final
{
    entt::entity prev;
    entt::entity target; // the node from which this node reads from/writes to
    uint32_t tag;        // if -1, equivalent to `Top` (eg. accessing a runtime index of an array)
    // ^ in all cases, `tag` should never be top but rather `parent` is propagated to the previous level (eg. `Top(Offset)` is `Local(x)`/`Global(x)`/`Heap(x)` depending on the parent)
    // TODO: for structs, `tag` can probably just be the hash of the member, but then what is `Top`?
};

// component
// present on nodes with a `mem_effect` that perform reads
struct mem_read final
{
};

// component
// present on nodes with a `mem_effect` that performs writes
struct mem_write final
{
};

// component
// present only on `Phi` nodes, connects them to the corresponding `Region`
struct region_of_phi final
{
    entt::entity region;
};

// component
// present only for nodes where type checking fails (eg. trying to add a boolean to an array)
struct error_node final
{
};

// extra components:
// `storage<void>("maybe_reachable"_hs) - this node might be reachable, will be determined later by dead code elimination once the function is parsed
// `storage<void>("reachable"_hs) - this node won't be visited by dead code elimination
// 'storage<void>("unreachable"_hs) - this node will be cut by dead code elimination

// TODO: mark globals as `reachable`

template <>
struct std::formatter<entt::entity, char> final
{
    // TODO: recheck the return
    constexpr auto parse(auto &ctx) const noexcept { return ctx.begin(); }

    inline auto format(entt::entity id, auto &ctx) const noexcept
    {
        auto const just_id = entt::to_entity(id);
        return std::format_to(ctx.out(), "{}", just_id);
    }
};
