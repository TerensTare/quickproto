
#pragma once

#include <format>
#include <vector>

#include <entt/entity/entity.hpp>
#include "utils/smallvec.hpp"

// TODO:
// - `checked_index` by default, which can be lowered to `index` iff `index < len` is `true`
// - optimization: Loads at an index with known type are just a `const` node
// - no need for sized math operations, just generate a `truncate` (& ~0) node as needed and just have `Iadd`/`Uadd`/similar
// ^ you probably still need `F32add` and `F64add` + similar due to memory layout of floats
// ^ also for uint you don't need an "expand" node, but for  sint/floats you probably need an "s/fextendMtoN" instruction
// - `delay_typecheck` component to resolve out-of-order declarations and such
// - evaluate the type of every node when parsing, then `const_fold` would simply check `type.is_known` and replace the node with `Const(type)`
// - (maybe) it's a good idea to mark `Return` nodes with a component, so you can later easily jump to them to cut unused functions
// - most of the nodes have a fixed (small) number of children, so use a smallvector instead for less dynamic allocations
// - merge non-basic edges into a single component type, interpreted based on `node_op`
// - multi-assign + multi-declare variables
// - store nodes by hash (hash=id, but based on node kind, etc.) - called hash-cons; it also helps with constant pooling
// - Stores are right-to-left; enforce that (maybe by just reordering `In`?)
// - add a control flow edge to load/store
// - `Start` is essentially the first `Store` in the function and also a special case of `MultiNode`
// ^ So when you `Proj` on `Start` you get a local variable
// ^ for constant folding, you don't care about killing node inputs, just the users of a node
// ^ then if a node has no users it is a dead node
// - does MultiNode have many values or many inputs?
// - Addr is just a `Proj` now; address this
// - when visualizing, show the type of the node with `tooltip=\"\"`
// - should loads and stores have type information in them?
// ^ eg. `Load(from=mem_state, proj=0, as=int64_bot)` (if the value is known, the type will be lifted to have more info automatically)
// - drop the registry for a list of storages
// - add a `light_entity<Ts...>` to enforce component invariants
// - also embed the op kind in each entity's id for fast checks in `peephole`
// - (maybe): split nodes depending on type: value nodes, control flow nodes, etc...; this way you can specify more clearly the edges of nodes
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
struct value_type;

// component
enum class node_op : uint8_t
{
    Load,      // Load In=[Addr(someVar), start | lastStoreBeforeThis, Offset=size_t(default=0)]
    Store,     // Store In=[Addr(dstRegister), srcNode, start | lastStoreBeforeThis]
    MultiNode, // MultiNode Value=[n] In=array[node *; n]
    Proj,      // Proj Value=[i] In=[multiNode]
    Start,     // Start - a no-op node for preserving ordering between loads/stores and for representing side effects. Also indicates the beginning of a function
    Return,    // Return Value=[n] In=[ctrl, returnNode x n]
    Exit,      // Exit Value=[n]

    IfYes,  // IfYes In=[ctrlNode, condNode]
    IfNot,  // IfNot In=[ctrlNode, condNode]
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

    CallStatic, // CallStatic In=[ctrl, state, args..., func_to_call] Out=[ctrl, state, result...]

    Addr,  // Addr Value=someRegister
    Const, // Const Value=someValue
};

// component
// HACK: use a non-polymorphic C++ type to represent node type
struct node_type final
{
    value_type const *type = nullptr;
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
// TODO: inline small-size list depending on node type
// TODO: nodes have a fixed number of inputs (except maybe call when optimized?) so you can use a `dynamic_array<T>` here probably with a small buffer for math nodes
struct node_inputs final
{
    smallvec nodes;
};

// component
// (id) -> effect{target} means that `id` affects the `target` node
struct effect final
{
    entt::entity target;
};

// component
// present only on `Phi` nodes, connects them to the corresponding `Region`
struct region_of_phi final
{
    entt::entity region;
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
