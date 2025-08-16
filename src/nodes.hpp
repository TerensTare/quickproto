
#pragma once

#include <print>
#include <string>
#include <unordered_map>
#include <vector>

// #include "type.hpp"
#include "value.hpp"

// HACK: temporary
enum class type
{
    Top,
    Bottom,

    IntConst,
    IntBottom,
};

// TODO:
// - do you really need a node id inside the node?
// - multi-assign + multi-declare variables
// - store nodes by hash (hash=id, but based on node kind, etc.)
// - Stores are right-to-left; enforce that (maybe by just reordering `In`?)
// - arena-allocate builder data
// - nodes need a custom deleter (cannot just do `delete node`)
// - add a control flow node to load/store
// - `Start` is essentially the first `Store` in the function and also a special case of `MultiNode`
// ^ So when you `Proj` on `Start` you get a local variable
// ^ for constant folding, you don't care about killing node inputs, just the users of a node
// ^ then if a node has no users it is a dead node
// - consider storing edges as inputs instead of just nodes, to model different kind of edges (memory, etc.)
// - destroying delete for `node`

// DONE:
// - [x] `Start` node to be specified on `load`/`store` nodes to avoid reordering
// - [x] export graph to dot format
// - [x] builder.constant(x)
// - [x] separate node from builder in files

enum class node_id : uint32_t
{
};

// TODO: can you fit all ops in a uint8_t?
enum class op : uint8_t
{
    Load,      // Load In=[Addr(someVar), start | lastStoreBeforeThis]
    Store,     // Store In=[Addr(dstRegister), srcNode, start | lastStoreBeforeThis]
    Start,     // Start - a no-op node for preserving ordering between loads/stores and for representing side effects. Also indicates the beginning of a function
    MultiNode, // MultiNode Value=[n] In=array[node *; n]
    Proj,      // Proj Value=[i] In=[multiNode]
    Return,    // Return Value=[n] In=[node *; n]

    UnaryNeg,
    UnaryNot,

    Add,
    Sub,
    Mul,
    Div,

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

inline std::string_view to_string(enum op op) noexcept
{
    using enum op;

    switch (op)
    {
    case Load:
        return "Load";
    case Store:
        return "Store";
    case Start:
        return "Start";
    case Return:
        return "Return";
    case Proj:
        return "Proj";

    case UnaryNeg:
        return "UnaryNeg";
    case UnaryNot:
        return "UnaryNot";

    case Add:
        return "Add";
    case Sub:
        return "Sub";
    case Mul:
        return "Mul";
    case Div:
        return "Div";

    case CmpEq:
        return "CmpEq";
    case CmpLe:
        return "CmpLe";

    case Addr:
        return "Addr";
    case Const:
        return "Const";

    default:
        return "<Unknown>";
    }
}

struct node final
{
    inline node **inputs() noexcept { return (node **)(this + 1); }

    node_id id;
    enum op op;
    enum type type;
    value value;
    size_t in_size;
    // the rest of this is an array of `node *` (ie. `node *[in_size]`)
};

template <typename T>
using node_map = std::unordered_map<node_id, T, std::identity>;

struct graph final
{
    inline void export_dot(FILE *f) const noexcept;

    std::vector<node *> nodes;
};

inline void graph::export_dot(FILE *f) const noexcept
{
    std::print(f, "digraph G {{\n"
                  "  rankdir=BT;\n"
                  "  node [shape=box];\n");

    for (auto n : nodes)
    {
        auto meta = [&]() -> std::string
        {
            switch (n->op)
            {
            case op::Const:
                return std::format("[label=\"{}\"]", n->value.i64);

            case op::Add:
                return "[label=\"+\", shape=circle]";

            case op::Sub:
            case op::UnaryNeg:
                return "[label=\"-\", shape=circle]";

            case op::Mul:
                return "[label=\"*\", shape=circle]";

            case op::Div:
                return "[label=\"/\", shape=circle]";

            default:
                return std::format("[label=\"{}\"]", to_string(n->op));
            }
        }();

        std::print(f, "  n{} {};\n", (uint32_t)n->id, meta);

        auto ins = n->inputs();
        for (size_t i{}; i < n->in_size; ++i)
            std::print(f, "  n{} -> n{};\n", (uint32_t)n->id, (uint32_t)ins[i]->id);
    }

    std::print(f, "}}");
}