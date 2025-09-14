
#pragma once

#include <algorithm>
#include <charconv>

#include "builder.hpp"
#include "scanner.hpp"

#include "pass/pass_registry.hpp"

// TODO:
// - variable mapping should be block-specific
// - drop recursion from expr and stmt rules
// - apparently you don't need `Store` nodes most of the time; you only need them for nodes where it has an effect on
// ^ (which is most of the time just class/global variables; for pointer function params you can just change the variable value from the calling function)
// ^ but you still need to take care of loads of non-const variables (eg. `a = 10; b += a; a = 20`)
// - implement codegen for comparison (<, <=, ==, !=, >, >=)
// - structured logging
// ^ type-erased logging destination output (stdout, json file, etc...)

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

constexpr node_op op_node(token_kind kind) noexcept
{
    using enum token_kind;
    using enum node_op;

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

    inline entt::entity primary() noexcept;
    inline entt::entity expr(parse_prec prec = parse_prec::None) noexcept;

    // stmt

    // <ident>,+ ':=' expr,+ ';'
    inline void local_decl(std::span<token const> ids) noexcept;
    // expr comp_op expr ';'
    // comp_op ::= '+=' | '-=' | '*=' | '/='
    inline void comp_assign(std::span<token const> ids) noexcept;
    // expr post_op ';'
    // post_op ::= '++' | '--'
    inline void post_op(std::span<token const> ids) noexcept;

    // 'if' expr block ('else' ( if_stmt | block ))?
    inline void if_stmt() noexcept;
    // 'for' expr block
    inline void for_stmt() noexcept;
    // break_stmt | continue_stmt
    //  break_stmt    ::= 'break'  <ident>? ';'
    //  continue_stmt ::= 'continue' <ident>? ';'
    inline void control_flow(token_kind which) noexcept;
    // 'return' expr,* ';'
    inline void return_stmt() noexcept;
    // { stmt* }
    inline void block() noexcept;
    // local_decl | comp_assign | post_op
    inline void simple_stmt() noexcept;

    // block
    // | if_stmt
    // | for_stmt
    // | return_stmt
    // | break_stmt
    // | continue_stmt
    // | simple_stmt
    inline void stmt() noexcept;

    // decl

    // 'func' <ident> '(' ')' block
    inline void func_decl() noexcept;
    // func_decl | type_decl
    inline void decl() noexcept { func_decl(); }
    // decl*
    inline void prog() noexcept;

    // helpers
    // <ident> <ident> (name type)
    inline void param_decl() noexcept;

    inline token eat(token_kind kind) noexcept;
    inline token eat(std::span<token_kind const> kinds) noexcept;

    scanner scan;
    builder bld;
    pass_registry passes{.bld = bld};

    std::unordered_map<std::string_view, entt::entity> vars;  // TODO: also account for local variables; can this be a function-local variable?
    std::unordered_map<std::string_view, entt::entity> funcs; // (name -> mem_state) mapping
    entt::entity mem_state;                                   // TODO: this should be per function
    int64_t next_memory_slot = 0;                             // for variables
};

inline entt::entity parser::primary() noexcept
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
        return bld.makeval(mem_state, node_op::Const, new int_const{val});
    }

    // TODO: don't use integer type here anymore
    case KwFalse:
        return bld.makeval(mem_state, node_op::Const, new int_const{false});
    case KwTrue:
        return bld.makeval(mem_state, node_op::Const, new int_const{true});

    case KwNil:
        return bld.makeval(mem_state, node_op::Const, new int_const{0});

    case Ident:
    {
        auto const name = scan.lexeme(tok);
        // TODO: recheck this after figuring out addressing
        // TODO: figure out the type of the node
        // auto const addr = bld.makeval(mem_state, op::Addr, int_bot::self(), {.i64 = 0});

        return vars[name];
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
        auto sub = expr();
        entt::entity inputs[]{sub};
        return bld.make(node_op::UnaryNot, inputs);
    }

    case Minus:
    {
        auto sub = expr();
        entt::entity inputs[]{sub};
        return bld.make(node_op::UnaryNeg, inputs);
    }

    default:
        std::unreachable();
        return entt::null;
    }
}

inline entt::entity parser::expr(parse_prec prec) noexcept
{
    auto lhs = passes.run(primary());

    while (scan.peek.kind != token_kind::Eof)
    {
        auto iter = prec_table.find(scan.peek.kind);
        if (iter == prec_table.end() || iter->second < prec)
            break;

        auto rprec = iter->second;

        auto op = scan.next().kind;
        // TODO: if right-associative op, increase precedence of rprec by 1
        // TODO: do you even have right-associative operators?
        auto rhs = expr(rprec);

        entt::entity inputs[]{lhs, rhs};
        lhs = bld.make(op_node(op), inputs);
        lhs = passes.run(lhs);
    }

    return lhs;
}

inline void parser::local_decl(std::span<token const> ids) noexcept
{
    // parsing

    // TODO: eat this outside of this function
    eat(token_kind::Walrus); // :=

    // TODO: parse an expression list instead
    auto rhs = expr();

    eat(token_kind::Semicolon); // ;

    // codegen

    // TODO: declare all variables, not just the first one
    // TODO: figure out the type of the node
    // auto lhs = bld.make(op::Proj, mem_state);
    // lhs->value = {.i64 = next_memory_slot++};

    // auto out = bld.make(op::Store, lhs, rhs);
    vars[scan.lexeme(ids[0])] = rhs;
    // TODO: is this the correct node?
    // bld.effects[rhs->id] = mem_state;
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
                             ? node_op::Add
                         : optok == token_kind::MinusEqual
                             ? node_op::Sub
                         : optok == token_kind::StarEqual
                             ? node_op::Mul
                             : node_op::Div;

    // TODO: do NOT use the lexeme here, use the address of lhs (eg. `a.b` is not a single token)
    auto const name = scan.lexeme(ids[0]);

    // TODO: use an actual address here
    // TODO: figure out the type of the node
    // auto const res = bld.makeval(op::Addr, int_bot::self(), {.i64 = 0});

    auto opnode = bld.make(node_op, vars[name], rhs);
    opnode = passes.run(opnode);
    vars[name] = opnode; // TODO: does this depend on `vars[name]` or on the load?
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
                             ? node_op::Add
                             : node_op::Sub;

    // TODO: do NOT use the lexeme here, use the address of the result
    auto const name = scan.lexeme(ids[0]);

    // TODO: use an actual address here
    // TODO: figure out the type of the node
    // auto const res = bld.makeval(mem_state, op::Addr, int_bot::self());

    auto opnode = bld.make(
        node_op,
        vars[name],
        bld.makeval(mem_state, node_op::Const, new int_const{1}) //
    );
    opnode = passes.run(opnode);

    vars[name] = opnode;
}

inline void parser::if_stmt() noexcept
{
    // parsing
    eat(token_kind::KwIf); // 'if'
    auto cond = expr();    // expr

    // TODO: is this mem_state or $ctrl?
    auto if_node = bld.make(node_op::If, mem_state, cond);

    auto then = bld.make(node_op::Proj, if_node);
    // then->value = {.i64 = 0};

    // TODO: copy the variables on scope and set the copy's `$ctrl` to `then` or `else_` depending on the branch taken, then pop back when done
    // ^ then also merge the two scopes into the main scope
    block(); // block

    if (scan.peek.kind == token_kind::KwElse)
    {
        auto else_ = bld.make(node_op::Proj, if_node);
        // else_->value = {.i64 = 1};

        scan.next(); // 'else'
        (scan.peek.kind == token_kind::KwIf)
            ? if_stmt() // if_stmt
            : block();  // | block
    }

    // codegen
}

inline void parser::for_stmt() noexcept
{
    // parsing
    eat(token_kind::KwFor); // 'for'

    auto cond = expr();
    block();

    // codegen

    // TODO: implement
}

inline void parser::control_flow(token_kind which) noexcept
{
    // parsing

    if (scan.peek.kind != token_kind::Semicolon)
    {
        auto where = eat(token_kind::Ident); // <ident>?
    }

    eat(token_kind::Semicolon); // ;

    // codegen

    // TODO: implement
}

inline void parser::return_stmt() noexcept
{
    // parsing
    eat(token_kind::KwReturn); // 'return'

    std::vector<entt::entity> outs;

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
    auto out = bld.make(node_op::Return, outs);
    // TODO: recheck this
    (void)bld.reg.get_or_emplace<effect>(out, mem_state);
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

    case token_kind::KwIf:
        if_stmt();
        break;

    case token_kind::KwFor:
        for_stmt();
        break;

    case token_kind::KwBreak:
    case token_kind::KwContinue:
        control_flow(scan.next().kind);
        break;

    default:
        simple_stmt();
        break;
    }
}

// decl

inline void parser::func_decl() noexcept
{
    // some setup codegen before parsing
    // NOTE: this needs to be here because parameters depend on `mem_state`
    // TODO: rollback this in case of errors during parsing
    mem_state = bld.make(node_op::Start);
    next_memory_slot = 0;

    // parsing

    eat(token_kind::KwFunc);               // 'func'
    auto nametok = eat(token_kind::Ident); // ident
    eat(token_kind::LeftParen);            // '('

    // param_decl,*
    if (scan.peek.kind != token_kind::RightParen)
    {
        param_decl(); // param_decl

        // (',' param_decl)*
        while (scan.peek.kind == token_kind::Comma)
        {
            scan.next();  // ,
            param_decl(); // param_decl
        }
    }

    // TODO: parse parameters
    eat(token_kind::RightParen); // ')'

    auto name = scan.lexeme(nametok);
    ensure(!funcs.contains(name), "Function already defined");

    funcs.insert({name, mem_state});

    block(); // block

    // TODO: for each function, this will run over all the program which is sub-optimal; so prune starting from the return instead
    // TODO: enable back
    // lemma: alive_nodes(func) = (alive_nodes(stmt) + ...) for stmt in func.body
    // ^ ie. if a node is not reachable after a statement is added (except the main node of the stmt), it is not reachable after other statements are added
    // ^ (maybe) you can simply mark the unreachable nodes with time and delete them when a function is fully parsed
    prune_dead_code(bld);
}

inline void parser::prog() noexcept
{
    while (scan.peek.kind != token_kind::Eof)
    {
        decl(); // decl*
    }
}

// helper rules

inline void parser::param_decl() noexcept
{
    auto nametok = eat(token_kind::Ident); // name
    auto type = eat(token_kind::Ident);    // type

    // TODO: recheck this
    auto node = bld.makeval(mem_state, node_op::Proj, new int_const{next_memory_slot++});
    // TODO: remove this once `makeval` sets effects
    (void)bld.reg.get_or_emplace<effect>(node, mem_state);

    auto name = scan.lexeme(nametok);
    vars[name] = node;
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
