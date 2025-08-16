
#pragma once

#include <algorithm>
#include <charconv>

#include "builder.hpp"
#include "dce.hpp"
#include "scanner.hpp"

// TODO:
// - variable mapping should be block-specific
// - drop recursion from expr and stmt rules
// - apparently you don't need `Store` nodes most of the time; only one per variable (the last store that happens)

enum class parse_prec : uint8_t
{
    None,
    Assign,     // =
    Or,         // or
    And,        // and
    Equality,   // == !=
    Comparison, // < <= >= >
    Term,       // + -
    Factor,     // * /
    Unary,      // ! -
    Call,       // . ()
    Primary,
};

inline static std::unordered_map<token_kind, parse_prec, std::identity> const prec_table{
    {token_kind::EqualEqual, parse_prec::Equality},
    {token_kind::BangEqual, parse_prec::Equality},
    {token_kind::Greater, parse_prec::Comparison},
    {token_kind::GreaterEqual, parse_prec::Comparison},
    {token_kind::Less, parse_prec::Comparison},
    {token_kind::LessEqual, parse_prec::Comparison},
    {token_kind::Plus, parse_prec::Term},
    {token_kind::Minus, parse_prec::Term},
    {token_kind::Star, parse_prec::Factor},
    {token_kind::Slash, parse_prec::Factor},
};

constexpr op op_node(token_kind kind) noexcept
{
    using enum token_kind;
    using enum op;

    switch (kind)
    {
    case Plus:
        return Add;
    case Minus:
        return Sub;
    case Star:
        return Mul;
    case Slash:
        return Div;

    case BangEqual:
        return CmpNe;
    case EqualEqual:
        return CmpEq;
    case Less:
        return CmpLt;
    case LessEqual:
        return CmpLe;
    case Greater:
        return CmpGt;
    case GreaterEqual:
        return CmpGe;
    }
}

struct parser final
{
    // expr

    inline node *primary() noexcept;
    inline node *expr(parse_prec prec = parse_prec::None) noexcept;

    // stmt

    // <ident>,+ ':=' expr,+ ';'
    inline void local_decl(std::span<token const> ids) noexcept;
    // expr comp_op expr ';'
    // comp_op ::= '+=' | '-=' | '*=' | '/='
    inline void comp_assign(std::span<token const> ids) noexcept;
    // expr post_op ';'
    // post_op ::= '++' | '--'
    inline void post_op(std::span<token const> ids) noexcept;
    // 'return' expr,* ';'
    inline void return_stmt() noexcept;
    // { stmt* }
    inline void block() noexcept;
    // local_decl | comp_assign | post_op
    inline void simple_stmt() noexcept;

    // block | return_stmt | simple_stmt
    inline void stmt() noexcept;

    // decl

    // 'func' <ident> '(' ')' block
    inline void func_decl() noexcept;
    // func_decl
    inline void decl() noexcept { func_decl(); }
    // decl*
    inline void prog() noexcept;

    inline token eat(token_kind kind) noexcept;
    inline token eat(std::span<token_kind const> kinds) noexcept;

    scanner scan;
    builder bld;
    std::unordered_map<std::string_view, node *> vars;  // TODO: also account for local variables; can this be a function-local variable?
    std::unordered_map<std::string_view, node *> funcs; // (name -> mem_state) mapping
    node *mem_state;                                    // TODO: this should be per function
    int64_t next_memory_slot = 0;                       // for variables
};

inline node *parser::primary() noexcept
{
    using enum token_kind;

    // parsing

    token_kind const primary_tokens[]{
        Integer, KwFalse, KwTrue, KwNil, // literal
        Ident,                           // variable/function/type/const/whatever
        LeftParen,                       // (expr)
        Bang, Minus,                     // unary
    };

    auto const tok = eat(primary_tokens);

    // codegen

    switch (tok.kind)
    {
    case Integer:
    {
        auto const txt = scan.lexeme(tok);

        int64_t val{};
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        return bld.makeval(op::Const, type::IntConst, {.i64 = val});
    }

    // TODO: don't use integer type here anymore
    case KwFalse:
        return bld.makeval(op::Const, type::IntConst, {.i64 = false});
    case KwTrue:
        return bld.makeval(op::Const, type::IntConst, {.i64 = true});

    case KwNil:
        return bld.makeval(op::Const, type::IntConst, {.i64 = INT64_MIN});

    case Ident:
    {
        auto const name = scan.lexeme(tok);
        // TODO: recheck this after figuring out addressing
        // TODO: figure out the type of the node
        auto const addr = bld.makeval(op::Addr, type::IntBottom, {.i64 = 0});

        return bld.make(op::Load, addr, vars[name]);
    }

    case LeftParen:
    {
        auto ret = expr();
        eat(RightParen);
        // TODO: is this ok or should you wrap it?
        return ret;
    }

    case Bang:
    {
        auto ret = expr();
        return bld.make(op::UnaryNot, ret);
        // TODO: peephole this
    }

    case Minus:
    {
        auto sub = expr();
        auto ret = bld.make(op::UnaryNeg, sub);
        return bld.peephole(ret);
    }

    // TODO: unreachable
    default:
        return nullptr;
    }
}

inline node *parser::expr(parse_prec prec) noexcept
{
    auto lhs = primary();

    while (scan.peek.kind != token_kind::Eof)
    {
        auto iter = prec_table.find(scan.peek.kind);
        if (iter == prec_table.end() || iter->second < prec)
            break;

        auto rprec = iter->second;

        auto op = scan.next().kind;
        // TODO: if right-associative op, increase precedence of rprec by 1
        auto rhs = expr(rprec);

        lhs = bld.make(op_node(op), lhs, rhs);
        lhs = bld.peephole(lhs);
    }

    return lhs;
}

inline void parser::local_decl(std::span<token const> ids) noexcept
{
    // parsing

    // TODO: eat this outside of this function
    eat(token_kind::Walrus);

    // TODO: parse an expression list instead
    auto rhs = expr();

    eat(token_kind::Semicolon);

    // codegen

    // TODO: declare all variables, not just the first one
    // TODO: figure out the type of the node
    auto lhs = bld.make(op::Proj, mem_state);
    lhs->value = {.i64 = next_memory_slot++};

    vars[scan.lexeme(ids[0])] = bld.make(op::Store, lhs, rhs, mem_state);
}

inline void parser::comp_assign(std::span<token const> ids) noexcept
{
    // TODO:
    // - the assignment should be evaluated RTL

    // parsing

    token_kind const ops[]{
        token_kind::PlusEqual,
        token_kind::MinusEqual,
        token_kind::StarEqual,
        token_kind::SlashEqual,
    };

    auto const optok = eat(ops).kind;

    auto rhs = expr();

    eat(token_kind::Semicolon);

    // codegen
    auto const node_op = optok == token_kind::PlusEqual
                             ? op::Add
                         : optok == token_kind::MinusEqual
                             ? op::Sub
                         : optok == token_kind::StarEqual
                             ? op::Mul
                             : op::Div;

    // TODO: do NOT use the lexeme here, use the address of the result
    auto const name = scan.lexeme(ids[0]);

    // TODO: use an actual address here
    // TODO: figure out the type of the node
    auto const res = bld.makeval(op::Addr, type::IntBottom, {.i64 = 0});

    auto const ld = bld.make(op::Load, res, vars[name]);
    auto const opnode = bld.make(node_op, ld, rhs);
    vars[name] = bld.make(op::Store, res, opnode, vars[name]); // TODO: does this depend on `vars[name]` or on the load?
}

inline void parser::post_op(std::span<token const> ids) noexcept
{
    // parsing

    token_kind const ops[]{
        token_kind::PlusPlus,
        token_kind::MinusMinus,
    };

    auto const optok = eat(ops).kind;

    eat(token_kind::Semicolon);

    // codegen
    auto const node_op = optok == token_kind::PlusPlus
                             ? op::Add
                             : op::Sub;

    // TODO: do NOT use the lexeme here, use the address of the result
    auto const name = scan.lexeme(ids[0]);
    // TODO: use an actual address here
    // TODO: figure out the type of the node
    auto const res = bld.makeval(op::Addr, type::IntBottom, {.i64 = 0});

    auto const ld = bld.make(op::Load, res, vars[name]);
    auto const opnode = bld.make(
        node_op,
        ld,
        bld.makeval(op::Const, type::IntConst, {.i64 = 1}) //
    );
    vars[name] = bld.make(op::Store, res, opnode, vars[name]);
}

inline void parser::return_stmt() noexcept
{
    // parsing
    eat(token_kind::KwReturn); // 'return'

    std::vector<node *> outs;
    if (scan.peek.kind != token_kind::Semicolon)
    {
        outs.push_back(expr()); // expr

        while (scan.peek.kind == token_kind::Comma)
        {
            scan.next();            // ,
            outs.push_back(expr()); // expr
        }
    }

    eat(token_kind::Semicolon); // ;

    // codegen
    bld.make(op::Return, outs);
}

inline void parser::block() noexcept
{
    eat(token_kind::LeftBrace); // {

    // stmt*
    while (scan.peek.kind != token_kind::RightBrace)
        stmt();

    scan.next(); // }
}

inline void parser::simple_stmt() noexcept
{
    std::vector<token> ids;
    ids.push_back(eat(token_kind::Ident));

    while (scan.peek.kind == token_kind::Comma)
    {
        scan.next(); // ,

        ids.push_back(eat(token_kind::Ident));
    }

    switch (scan.peek.kind)
    {
    case token_kind::Walrus:
        local_decl(ids);
        break;

    case token_kind::PlusPlus:
    case token_kind::MinusMinus:
        post_op(ids);
        break;

    case token_kind::PlusEqual:
    case token_kind::MinusEqual:
    case token_kind::StarEqual:
    case token_kind::SlashEqual:
        comp_assign(ids);
        break;

    default:
        fail("Expected one of the following: `:=`, `++`, `--`, `+=`, `-=`, `*=`, `/=`");
    }
}

inline void parser::stmt() noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::LeftBrace:
        block();
        break;

    case token_kind::KwReturn:
        return_stmt();
        break;

    default:
        simple_stmt();
        break;
    }
}

// decl

inline void parser::func_decl() noexcept
{
    // parsing

    eat(token_kind::KwFunc);               // 'func'
    auto nametok = eat(token_kind::Ident); // ident
    eat(token_kind::LeftParen);            // '('
    // TODO: parse parameters
    eat(token_kind::RightParen); // ')'

    auto name = scan.lexeme(nametok);
    ensure(!funcs.contains(name), "Function already defined");

    // some setup codegen before calling `block`
    mem_state = bld.make(op::Start);
    // TODO: ^ add parameters here
    funcs.insert({name, mem_state});

    block(); // block

    // TODO: for each function, this will run over all the program which is sub-optimal; so prune starting from the return instead
    prune_dead_code(bld);
}

inline void parser::prog() noexcept
{
    while (scan.peek.kind != token_kind::Eof)
        decl(); // decl*
}

// helpers

inline token parser::eat(token_kind kind) noexcept
{
    ensure(scan.peek.kind == kind, "Unexpected token kind");
    return scan.next();
}

inline token parser::eat(std::span<token_kind const> kinds) noexcept
{
    for (auto k : kinds)
        if (scan.peek.kind == k)
            return scan.next();

    fail("Unexpected token kind out of list");
}
