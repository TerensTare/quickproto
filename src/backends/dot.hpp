
#pragma once

#include <print>
#include "backends/backend.hpp"
#include "types/all.hpp"

// TODO:
// - on hover, show the type of the node
// - show `Load`/`Store` nodes as `.x`/`.x = `

struct dot_backend final : backend
{
    inline void compile(FILE *out, entt::registry const &reg);
};

inline auto print_node(auto &out, entt::registry const &reg, entt::entity id) noexcept -> decltype(auto)
{
    std::format_to(out, "n{} ", id);

#define named_node(label) std::format_to(out, "[label=\"" label "\"]")
#define circle_node(label, ...) std::format_to(out, "[label=\"" label "\", shape=circle" __VA_ARGS__ "]")

    auto &&[op, type] = reg.get<node_op const, node_type const>(id);
    switch (op)
    {
    case node_op::Program:
        return named_node("Program");
    case node_op::GlobalMemory:
        return named_node("GlobalMemory");
    case node_op::Start:
        return named_node("State");

    case node_op::Load:
        return named_node("Load");

    case node_op::Store:
        return named_node("Store");

    case node_op::Alloca:
        return named_node("Alloca"); // TODO: show the type of the node instead
    case node_op::Return:
        return named_node("Return");
    // HACK: use a separate function to print the type
    case node_op::Exit:
        return named_node("Exit"); // TODO: show the exit code if possible

        // HACK: use a separate function to print the type
    case node_op::Proj:
        // return std::format_to(out, "[label=\"Proj({})\"]", type.type->as<int_const>()->n);
        // HACK: temporary
        return std::format_to(out, "[label=\"Proj\"]");

    case node_op::IConst:
        return std::format_to(out, "[label=\"{}\"]", type.type->as<int_const>()->n);
    case node_op::FConst:
        return std::format_to(out, "[label=\"{}\"]", type.type->as<float64>()->d);
    case node_op::SConst:
        return std::format_to(out, "[label=<string>]"); // TODO: show the contents here
    case node_op::BConst:
        return std::format_to(out, "[label=\"{}\"]", type.type->as<bool_const>()->b);

        // TODO: show struct name here
    case node_op::Struct:
        return named_node("Struct");

    case node_op::Addr:
        return named_node("Addr");
    case node_op::Deref:
        return named_node("Deref");

    case node_op::UnaryNeg:
        return circle_node("-");
    case node_op::UnaryNot:
        return circle_node("!");
    case node_op::Add:
        return circle_node("+", ", tooltip=int32");
    case node_op::Sub:
        return circle_node("-", ", tooltip=int32");
    case node_op::Mul:
        return circle_node("*", ", tooltip=int32");
    case node_op::Div:
        return circle_node("/", ", tooltip=int32");
    case node_op::CmpEq:
        return circle_node("==");
    case node_op::CmpNe:
        return circle_node("!=");
    case node_op::CmpLt:
        return circle_node("<");
    case node_op::CmpLe:
        return circle_node("<=");
    case node_op::CmpGt:
        return circle_node(">");
    case node_op::CmpGe:
        return circle_node(">=");
    case node_op::LogicAnd:
        return circle_node("And");
    case node_op::LogicOr:
        return circle_node("Or");
    case node_op::BitAnd:
        return circle_node("&");
    case node_op::BitXor:
        return circle_node("^");
    case node_op::BitOr:
        return circle_node("|");

    case node_op::Fadd:
        return circle_node("+", ", tooltip=float64");
    case node_op::Fsub:
        return circle_node("-", ", tooltip=float64");
    case node_op::Fmul:
        return circle_node("*", ", tooltip=float64");
    case node_op::Fdiv:
        return circle_node("/", ", tooltip=float64");

    case node_op::IfYes:
        return named_node("IfYes");
    case node_op::IfNot:
        return named_node("IfNot");
    case node_op::Loop:
        return named_node("Loop");
    case node_op::Region:
        return named_node("Region");
    case node_op::Phi:
        return named_node("Phi");

    case node_op::CallStatic:
        return named_node("CallStatic");
    case node_op::ExternCall:
        return named_node("ExternCall");

    case node_op::Cast:
        return named_node("Cast");

    default:
#define console_yellow "\033[33m"
#define console_reset "\033[0m"

        std::println(console_yellow "(Dot backend) Dev warning: found unhandled node kind {}" console_reset, (int)op);

#undef console_reset
#undef console_yellow

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
            std::println(out, "  {};", str);
        }

        for (size_t i{}; auto &&in : ins.nodes)
            std::println(out, "  n{} -> n{} [label=\"in#{}\"];",
                         id, in, i++);
    }

    for (auto [dep, in] : reg.storage<ctrl_effect>()->each())
        std::println(out, "  n{} -> n{} [color=red];", dep, in.target);

    // TODO: do you really need to show memory effect nodes?
    for (auto [dep, in] : reg.storage<mem_effect>()->each())
    {
        if (in.prev != entt::null)
            std::println(out, "  n{} -> n{} [color=blue, style=dotted];", dep, in.prev);
        std::println(out, "  n{} -> n{} [color=blue, label=\"#{}\"];", dep, in.target, in.tag);
    }

    for (auto [phi, region] : reg.storage<region_of_phi>()->each())
        std::println(out, "  n{} -> n{} [style=dotted];", phi, region.region);

    std::print(out, "}}");
}
