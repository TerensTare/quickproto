
#pragma once

#include "backends/backend.hpp"

struct dot_backend final : backend
{
    inline void compile(FILE *out, entt::registry const &reg);
};

inline auto print_node(auto &out, entt::registry const &reg, entt::entity id) noexcept -> decltype(auto)
{
    std::format_to(out, "n{} ", id);

#define named_node(label) std::format_to(out, "[label=\"" label "\"]")
#define circle_node(label) std::format_to(out, "[label=\"" label "\", shape=circle]")

    auto &&[op, type] = reg.get<node_op const, node_type const>(id);
    switch (op)
    {
    case node_op::Load:
        return named_node("Load");
    case node_op::Store:
        return named_node("Store");
    case node_op::Start:
        return named_node("State");
    case node_op::Return:
        return named_node("Return");
        // HACK: use a separate function to print the type
    case node_op::Proj:
        return std::format_to(out, "[label=\"Proj({})\"]", type.type->as<int_const>()->n);
    case node_op::Const:
    {
        if (auto &&int_ = type.type->as<int_const>())
            return std::format_to(out, "[label=\"{}\"]", int_->n);
        else if (auto &&bool_ = type.type->as<bool_const>())
            return std::format_to(out, "[label=\"{}\"]", bool_->b);
    }
    case node_op::Addr:
        return std::format_to(out, "[label=\"Addr({})\"]", type.type->as<int_const>()->n);
    case node_op::UnaryNeg:
        return circle_node("-");
    case node_op::UnaryNot:
        return circle_node("!");
    case node_op::Add:
        return circle_node("+");
    case node_op::Sub:
        return circle_node("-");
    case node_op::Mul:
        return circle_node("*");
    case node_op::Div:
        return circle_node("/");
    case node_op::CmpEq:
        return circle_node("==");
    case node_op::CmpLe:
        return circle_node("<=");

    case node_op::IfYes:
        return named_node("IfYes");
    case node_op::IfNot:
        return named_node("IfNot");
    case node_op::Region:
        return named_node("Region");
    case node_op::Phi:
        return named_node("Phi");

    default:
        return named_node("<Unknown>");
    }

#undef circle_node
#undef named_node
}

inline void dot_backend::compile(FILE *out, entt::registry const &reg)
{
    std::print(out, "digraph G {{\n"
                    "  rankdir=BT;\n"
                    "  node [shape=box];\n");

    for (auto [id, ins] : reg.view<node_inputs const>().each())
    {
        // TODO: find something more elegant and efficient
        {
            std::string str;
            auto iter = std::back_inserter(str);
            print_node(iter, reg, id);
            std::print(out, "  {};\n", str);
        }

        for (size_t i{}; auto &&in : ins.nodes)
            std::print(out, "  n{} -> n{} [label=\"in#{}\"];\n",
                       id, in, i++);
    }

    for (auto [dep, in] : reg.storage<effect>()->each())
        std::print(out, "  n{} -> n{} [color=\"red\"];\n", dep, in.target);

    std::print(out, "}}");
}
