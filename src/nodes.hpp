
#pragma once

#include <print>
#include <string>
#include <vector>

#include <entt/entity/registry.hpp>

#include "type.hpp"

// TODO:
// - most of the nodes have a fixed (small) number of children, so use a smallvector instead for less dynamic allocations
// - embed node type on id
// - multi-assign + multi-declare variables
// - store nodes by hash (hash=id, but based on node kind, etc.) - called hash-cons; it also helps with constant pooling
// - Stores are right-to-left; enforce that (maybe by just reordering `In`?)
// - arena-allocate builder data
// - add a control flow edge to load/store
// - `Start` is essentially the first `Store` in the function and also a special case of `MultiNode`
// ^ So when you `Proj` on `Start` you get a local variable
// ^ for constant folding, you don't care about killing node inputs, just the users of a node
// ^ then if a node has no users it is a dead node
// - consider storing edges as inputs instead of just nodes, to model different kind of edges (memory, etc.)
// - does MultiNode have many values or many inputs?
// - Addr is just a `Proj` now; address this
// - when visualizing, show the type of the node with `tooltip=\"\"`
// - should loads and stores have type information in them?
// ^ eg. `Load(from=mem_state, proj=0, as=int64_bot)` (if the value is known, the type will be lifted to have more info automatically)
// - `ctrl` edges for control flow
// - drop the registry for a list of storages
// - use a custom printer for `entt::entity`
// - try to make `users` present only if there are actual users (helps with DCE and other passes)
// - add a `light_entity<Ts...>` to enforce component invariants
// - also embed the op kind in each entity's id for fast checks in `peephole`
// - (maybe): split nodes depending on type: value nodes, control flow nodes, etc...; this way you can specify more clearly the edges of nodes
// - local variables do not need `Load`/`Store`, but globals/members do
// - there are two types of `Proj` nodes: `Proj` on a value and `Proj` on control flow
// ^ `Proj`s on values have input links and `Proj`s on control flow have `effect` links

// DONE:
// - [x] encode flow information in nodes such as `if`, etc.
// - [x] nodes need a custom deleter (cannot just do `delete node`)
// - [x] store special edges separately in a map of linked-list mix
// - [x] `Start` node to be specified on `load`/`store` nodes to avoid reordering
// - [x] export graph to dot format
// - [x] separate node from builder in files

// TODO: can you fit all ops in a uint8_t?

// component
enum class node_op : uint8_t
{
    Load,      // Load In=[Addr(someVar), start | lastStoreBeforeThis]
    Store,     // Store In=[Addr(dstRegister), srcNode, start | lastStoreBeforeThis]
    MultiNode, // MultiNode Value=[n] In=array[node *; n]
    Proj,      // Proj Value=[i] In=[multiNode]
    Start,     // Start - a no-op node for preserving ordering between loads/stores and for representing side effects. Also indicates the beginning of a function
    Return,    // Return Value=[n] In=[ctrl, returnNode x n]

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

    Addr,  // Addr Value=someRegister
    Const, // Const Value=someValue
};

// component
// HACK: use a non-polymorphic C++ type to represent node type
struct node_type final
{
    ::type const *type = nullptr;
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
    // TODO: use something better
    std::vector<entt::entity> nodes;
};

// component
// (id) -> effect{target} means that `id` affects the `target` node
struct effect final
{
    entt::entity target;
};

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
