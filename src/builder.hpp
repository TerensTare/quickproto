
#pragma once

#include <span>
#include "nodes.hpp"

// TODO:
// - memory optimizations:
// ^ optimized memory layout
// ^ allocate everything on an allocator, keep builder and graph trivial
// - add a free-list to node allocation
// - block_builder, not just register_builder
// - set the type of each node, either by specifying or by deduction
// - optimize constant stores on `peephole`
// - prune dead nodes on `peephole`

struct builder final
{
    // TODO: can these overloads ever collide?
    inline node *make(op op, std::span<node *> nins) noexcept;

    template <std::same_as<node *>... Ins>
    inline node *make(op op, Ins... ins) noexcept
    {
        // TODO: recheck this
        node *nins_arr[]{nullptr, ins...}; // Add null in the beginning because C++ doesn't allow arrays of size 0
        return make(op, std::span(nins_arr + 1, sizeof...(Ins)));
    }

    // Add some kind of static checks to `op` here
    inline node *makeval(op op, type type, value value) noexcept;

    inline node_id next_id() noexcept;

    inline graph freeze() const noexcept;

    inline node *replace(node_id oldn, node *newn) noexcept;
    inline node *peephole(node *n) noexcept;

    std::vector<node *> nodes;
    node_id next;
    node_map<std::vector<node *>> users;
};

inline node *builder::make(op op, std::span<node *> nins) noexcept
{
    auto const num_ins = nins.size();

    auto n = (node *)::operator new(sizeof(node) + sizeof(node *) * num_ins);
    *n = {
        .id = next_id(),
        .op = op,
        .in_size = num_ins,
    };

    if (num_ins != 0)
        std::copy_n(nins.data(), num_ins, n->inputs());

    users.try_emplace(n->id);
    nodes.push_back(n);

    for (size_t i{}; i < num_ins; ++i)
    {
        auto in = nins[i];
        users[in->id].push_back(n);
    }

    return n;
}

inline node *builder::makeval(op op, type type, value value) noexcept
{
    auto n = make(op);
    n->type = type;
    n->value = value;
    return n;
}

inline node_id builder::next_id() noexcept
{
    auto const ret = next;
    next = node_id{(uint32_t)next + 1};
    return ret;
}

inline graph builder::freeze() const noexcept
{
    return graph{
        .nodes = nodes,
    };
}

inline node *builder::replace(node_id oldn, node *newn) noexcept
{
    for (auto user : users[oldn])
    {
        auto const count = user->in_size;
        auto inputs = user->inputs();

        for (size_t i{}; i < count; ++i)
            if (inputs[i]->id == oldn)
            {
                inputs[i] = newn;
                break;
            }
    }

    return newn;
}

inline node *builder::peephole(node *n) noexcept
{
    auto const newn = [&]
    {
        // TODO: use `meet` here instead
        auto const ins = n->inputs();

        auto const can_fold = [&]
        {
            switch (n->op)
            {
            case op::Add:
            case op::Sub:
            case op::Mul:
            case op::Div:
                // TODO: check the type instead
                return ins[0]->op == op::Const && ins[1]->op == op::Const;

            case op::UnaryNeg:
                return ins[0]->op == op::Const;
            }

            return false;
        }();

        if (!can_fold)
            return n;

        auto const res = [&]() -> value
        {
            switch (n->op)
            {
            case op::Add:
                return {.i64 = ins[0]->value.i64 + ins[1]->value.i64};

            case op::Sub:
                return {.i64 = ins[0]->value.i64 - ins[1]->value.i64};

            case op::Mul:
                return {.i64 = ins[0]->value.i64 * ins[1]->value.i64};

            case op::Div:
                return {.i64 = ins[0]->value.i64 / ins[1]->value.i64};

            case op::UnaryNeg:
                return {.i64 = -ins[0]->value.i64};
            }

            return value{};
        }();

        auto const oldn = n->id;
        auto newn = makeval(op::Const, type::IntConst, res);

        return replace(oldn, newn);
    }();

    // TODO: kill any nodes here

    return newn;
}