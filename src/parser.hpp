
#pragma once

#include <algorithm>
#include <charconv>

#include "nodegen/all.hpp"
#include "env.hpp"
#include "scanner.hpp"

#include "types/all.hpp"
#include "utils/function_ref.hpp"

// TODO:
// - generate `Store`s when creating an array
// - enable multiple declarations and multiple assignments once you fully figure them out
// - types and instances model; "operations" on types are type decorators, on instances are normal operators
// - `then` should probably accept a `stacklist` for return instead; this solves the multi-return problem + marking return as `entt::null`
// - have a typecheck phase right before codegen that fires out errors related to type checking
// ^ then you remove all the errors from `value_type`, as they should be handled on this phase instead
// - inline loads from values at the same scope level (eg. loading globals in global scope or locals in local scope)
// - define function names only after they are fully parsed
// ^ in the meantime you can mark their call occurrences as `unresolved` and resolve later
// ^ this allows you to also point the function node to the `Return` rather than `Start`
// - compute an `inline score` for each function based on number of nodes, recursion, number of calls, etc.
// - most calls to block need to be followed by a semicolon, except for the block after an `if` and before an `else`
// ^ can you simply require no semicolon after `}` and avoid automatic semicolon insertion for it?
// ^ this is not optimal as `Type{}` also uses `}` and a `;` is a valid following
// - (maybe) literals should have a special type that can implicitly downcast as long as it fits? (eg. 257 cannot fit in a `uint8`)
// ^ or maybe you can check these stuff in `type::value` instead and return an error on failure?
// ^ ie. `value_type::narrow(value_type const *sub) { if sub out of this return out-of-bounds-value; else return sub; }
// - find a way to avoid needing trailing newline on files (expected Semicolon but got Eof)
// - codegen a `Load`/`Store` for global variables, as these operations need to be "lazy"
// ^ (maybe) it's best to codegen `Load`/`Store` for everything, then cut the nodes for local stuff?
// - do you need a `memory` edge for operations that affect memory state? (then `effect` becomes `happens_after`)
// ^ (maybe) current `memory` and `ctrl` node should be on `builder`, not here? (they both start from `node::Start`)
// - have lookup tables that map operator to node type, depending on operands (eg. `Iadd` or `Fadd` for `+`)
// - (maybe) return expr type when parsing for cache friendliness?
// - when encountering a return, you should probably set the memory state to the `return` node
// - the returned values (on multi-return case) outlive the `then` call inside `block`, so you can just pass them as a span
// ^ also have local storage for return params that is reused, just so you don't reallocate every function call
// - inline all the CPS calls
// - apparently you don't need `Store` nodes most of the time; you only need them for nodes where it has an effect on
// ^ (which is most of the time just class/global variables; for pointer function params you can just change the variable value from the calling function)
// ^ but you still need to take care of loads of non-const variables (eg. `a = 10; b += a; a = 20`)
// - structured logging
// ^ type-erased logging destination output (stdout, json file, etc...)
// - the type of the `State` node should be a tuple of parameters of the function (+ maybe the return type?), so that when you `Proj` it you know how much to offset
// ^ parameter types start with the bottom version of their type, but when called they "meet" the value passed into the corresponding parameter slot
// - do you ever want negative `Proj`? if no, then you should use `uint64_t` when counting params
// - merge when a block exits, eg. in:
/*
```go
func foo(a int) {
    b := 42
    {
        b += 10
    }
    print(b)
}
```
*/
// ^ this means that you don't really need the old env after a block terminates, the new env will be a simple merge
// ^- also in the case of a unconditional block, you don't really need a merge, you just keep the env alive
// ^- but you cannot fully inline an unconditional block because the locals of that block don't persist
// ^ take special care when one branch returns and the other does not
// ^- but in that case the other branch merges with the rest of the function so you know the return point of the other branch as well

/*
```cpp
var_env merge(var_env const &parent, var_env const &left, var_env const &right) {
    for (auto key : parent) {
        codegen_phi(left[key], right[key]);
    }
}
```
*/
// ^ take care of merging when there is shadowing; you don't want to merge those

// - you don't really need to have a `global` env anymore; it is simply the current env and is merged as needed
// ^ you might still need it for codegen though (eg. `global.set` vs `local.set`, etc.)

// - lemmas: in a correct program:
// ^ a package is followed by an import or Eof
// ^ an import is followed by an import or Eof
// ^ a declaration is followed by a declaration or an Eof
// ^ a statement is followed by a statement or a `}`

// precedence table; operators with higher precedence get parsed earlier (eg. `a + b * c` gets parsed as `a + (b * c)`)
enum class parse_prec : uint8_t
{
    None,
    Or,         // ||
    And,        // &&
    Equality,   // == !=
    Comparison, // < <= >= >
    BitOr,      // |
    BitXor,     // ^
    BitAnd,     // &
    Term,       // + -
    Factor,     // * /
};

static constexpr auto prec_table = []()
{
    std::array<parse_prec, 256> table;
    // MSVC does not like a for loop here
    table.fill(parse_prec::None);

    table[(uint8_t)token_kind::OrOr] = parse_prec::Or;
    table[(uint8_t)token_kind::AndAnd] = parse_prec::And;
    table[(uint8_t)token_kind::EqualEqual] = parse_prec::Equality;
    table[(uint8_t)token_kind::BangEqual] = parse_prec::Equality;
    table[(uint8_t)token_kind::Greater] = parse_prec::Comparison;
    table[(uint8_t)token_kind::GreaterEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::Less] = parse_prec::Comparison;
    table[(uint8_t)token_kind::LessEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::Or] = parse_prec::BitOr;
    table[(uint8_t)token_kind::Xor] = parse_prec::BitXor;
    table[(uint8_t)token_kind::And] = parse_prec::BitAnd;
    table[(uint8_t)token_kind::Plus] = parse_prec::Term;
    table[(uint8_t)token_kind::Minus] = parse_prec::Term;
    table[(uint8_t)token_kind::Star] = parse_prec::Factor;
    table[(uint8_t)token_kind::Slash] = parse_prec::Factor;

    return table;
}();

struct parser final
{
    using block_then = function_ref<entt::entity(scope const *, entt::entity)>;

    // expr

    // post_expr(base) ::= call(base) | index(base) | member(base) | base
    // ^ie. it's `base` + any possible call following it
    [[nodiscard]]
    inline entt::entity post_expr(entt::entity base) noexcept;

    // call(base) ::= post_expr( base '(' expr,*,? ')' )
    // ^ meaning this rule will parse a call, then invoke `post_expr` with the parsed call as `base`
    [[nodiscard]]
    inline entt::entity call(entt::entity base) noexcept;

    // index(base) ::= post_expr( base '[' expr ']' )
    // ^ meaning this rule will parse an index, then invoke `post_expr` with the parsed index as `base`
    [[nodiscard]]
    inline entt::entity index(entt::entity base) noexcept;

    // TODO: add support for methods
    // member(base) ::= post_expr( base '.' <ident> )
    // ^ meaning this rule will parse an index, then invoke `post_expr` with the parsed index as `base`
    [[nodiscard]]
    inline entt::entity member(entt::entity base) noexcept;

    // primary ::= 'true'
    //           | 'false'
    //           | <integer>
    //           | <decimal>
    //           | <unary_op> expr
    //           | post_expr( '(' expr ')' )
    //           | post_expr( array_literal )
    //           | post_expr(<ident>)
    // <unary_op> ::= '!' | '-'
    // array_literal ::= '[' ']' type '{' expr,*,? '}'
    [[nodiscard]]
    inline entt::entity primary() noexcept;

    // expr ::= primary ( <binary_op> expr(<binary_op-prec>) )*
    // <binary_op> ::= '||' | '&&' | '==' | '!=' | '<' | '<=' | '>' | '>=' | '+' | '-' | '*' | '/' | '&' | '^' | '|'
    // ^ how much is parsed by a specific call to `expr` depends on the precedence level parsed
    [[nodiscard]]
    inline entt::entity expr(parse_prec prec = parse_prec::None) noexcept;

    // stmt
    // - the entity returned is the node of the `return` statement in this block, if any; `null` otherwise
    // ^ if a `continue` or `break` statement is reached before a `return` (not in a nested block), but on this block, the return is also `null` as the statement is unreachable
    // - also note that any of these functions will fully parse the remaining of the containing block for performance reasons

    // <ident>,+ ':=' expr,+ ';' stmt
    [[nodiscard]]
    inline entt::entity local_decl(token lhs) noexcept;
    // expr '=' expr ';' stmt
    // HACK: can you merge this with `compound_assign`?
    [[nodiscard]]
    inline entt::entity assign(entt::entity lhs) noexcept;

    // expr comp_op expr ';' stmt
    // comp_op ::= '+=' | '-=' | '*=' | '/=' | '&=' | '^=' | '|='
    [[nodiscard]]
    inline entt::entity compound_assign(entt::entity lhs) noexcept;
    // expr post_op ';' stmt
    // post_op ::= '++' | '--'
    [[nodiscard]]
    inline entt::entity post_op(entt::entity lhs) noexcept;

    // 'defer' call ';' stmt
    [[nodiscard]]
    inline entt::entity defer_stmt() noexcept;

    // 'if' expr block ('else' ( if_stmt | block ))? ';' stmt
    [[nodiscard]]
    inline entt::entity if_stmt() noexcept;
    // 'for' expr block stmt
    // ^ ie. right now this only parses a `while`-like for loop
    [[nodiscard]]
    inline entt::entity for_stmt() noexcept;
    // break_stmt | continue_stmt
    //  break_stmt    ::= 'break' <ident>? ';' stmt
    //  continue_stmt ::= 'continue' <ident>? ';' stmt
    [[nodiscard]]
    inline entt::entity control_flow(token_kind which) noexcept;
    // 'return' expr,* ';' stmt
    [[nodiscard]]
    inline entt::entity return_stmt() noexcept;
    // '{' stmt
    // ^ no `}` as that is handled by the `stmt` recursion
    [[nodiscard]]
    inline entt::entity block(block_then then) noexcept;
    // local_decl | assign | compound_assign | post_op
    // TODO: local_decl is disabled for now. Please use `var` syntax instead
    [[nodiscard]]
    inline entt::entity simple_stmt() noexcept;

    // stmt ::= '}'
    //        | alias_decl
    //        | var_decl
    //        | block stmt
    //        | defer_stmt
    //        | if_stmt
    //        | for_stmt
    //        | return_stmt
    //        | break_stmt
    //        | continue_stmt
    //        | simple_stmt
    //        | ';' stmt //< empty statement
    [[nodiscard]]
    inline entt::entity stmt() noexcept;

    // decl

    // 'const' <ident> '=' expr ';' decl
    inline void const_decl() noexcept;
    // 'func' <ident> '(' param_decl,*, ')' type? block ';' decl
    inline void inline_func_decl() noexcept;
    // '@' 'extern' 'func' <ident> '(' param_decl,*, ')' type? ';' decl
    // ^ external function declaration; they do not have a body
    // HACK: handle `extern` just like other annotations
    inline void extern_func_decl() noexcept;

    // 'var' <ident> type '=' expr ';' decl
    // if parsed inside a block, this is treated as a statement and is followed by a `stmt` rather than a decl
    template <bool AsStmt>
    inline auto var_decl() noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;
    // alias_decl
    // ^ ie. right now this rule only parses alias declarations
    // if parsed inside a block, this is treated as a statement and is followed by a `stmt` rather than a decl
    template <bool AsStmt>
    inline auto type_decl() noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;
    // 'type' <ident> type ';' decl
    // if parsed inside a block, this is treated as a statement and is followed by a `stmt` rather than a decl
    template <bool AsStmt>
    inline auto alias_decl(token nametok) noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;
    // 'type' <ident> 'struct' '{' (member_decl ';')* '}'
    // member_decl ::= <ident> type
    // TODO: is `member_decl` and `param_decl` the same rule?
    template <bool AsStmt>
    inline auto struct_decl(token nametok) noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;

    // const_decl | func_decl | var_decl
    // func_decl ::= extern_func_decl | inline_func_decl
    inline void decl() noexcept;
    // decl
    inline void prog() noexcept;

    scanner scan;
    builder bld;

    scope global = global_scope();
    env env{.top = &global};

    stacklist<entt::entity> *defer_stack = nullptr;

private:
    // helpers

    // <ident> type
    // i is the index of the parameter in the function declaration (used by the Proj node emitted for the parameter)
    inline value_type const *param_decl(int64_t i) noexcept;

    // expr,*,? term
    inline smallvec expr_list_term(token_kind term) noexcept;

    // expr (,expr)*
    // ^ ie. there cannot be a trailing comma and the terminator can be anything except something that starts an expression
    // TODO: should this be able to parse 0 expressions?
    inline smallvec expr_list() noexcept;

    // type

    // array_type | named_type
    inline value_type const *type() noexcept;

    // '[' <integer> ']' type
    inline value_type const *array_type() noexcept;

    // <ident>
    inline value_type const *named_type() noexcept;

    // other helpers

    inline void merge(scope &parent, scope const &lhs, scope const &rhs) noexcept;

    // merge `lhs` and `rhs` into a new phi node if either of the nodes is different from `old` and return it, otherwise return `old`
    // TODO: if no `else` branch, `rhs` should be `old`
    // TODO: move this on the nodegen API
    inline entt::entity phi(entt::entity old, entt::entity lhs, entt::entity rhs) noexcept
    {
        auto const l = lhs != old, r = rhs != old;
        entt::entity ins[2]{old, old};
        if (l && r)
        {
            ins[0] = lhs;
            ins[1] = rhs;
        }
        else if (l || r)
            ins[0] = l ? lhs : rhs;
        else
            return old;

        // TODO: specify the type of this node
        auto const ret = bld.make(node_op::Phi, ins);
        (void)bld.reg.emplace<region_of_phi>(ret, old); // TODO: specify the actual region here and run passes
        // HACK: generalize the type
        bld.reg.get<node_type>(ret).type = int_bot::self();
        return ret;
    }

    // codegen the `Start` and `Exit` nodes of the program, pass the control flow to `main` (and initializing globals when added).
    inline void codegen_main() noexcept;

    // parsing helpers

    // TODO: eventually pass a span here instead of token
    [[noreturn]]
    inline void fail(token const &t, std::string_view msg) const
    {
        std::string_view txt{scan.text, t.start};
        auto const start = txt.rfind('\n') + 1;

        auto end = t.start;
        while (scan.text[end])
        {
            if (scan.text[end++] == '\n')
                break;
        }

        --end; // if no '\n', go to last character, otherwise go before the '\n'

        size_t line_num = 1, iter = 0;
        while (std::string_view::npos != (iter = txt.find('\n')))
        {
            ++line_num;
            txt = txt.substr(iter + 1);
        }

        // TODO: clamp the [start, end) range to, say 80 characters or maybe based on token?

        auto const lcode = std::string_view{scan.text + start, t.start - start};
        auto const ecode = std::string_view{scan.text + t.start, t.len};
        auto const rcode = std::string_view{scan.text + t.start + t.len, end - (t.start + t.len)};

        auto ok_part = std::format("{} | {}", line_num, lcode);

#define console_red "\033[31m"
#define console_reset "\033[0m"

        std::println(
            "{}:{}:{}: {}.\n\n{}" console_red "{}" console_reset "{}\n{:>{}}^--",
            // TODO: use actual file name
            "<source>", line_num, t.start - start,
            msg, ok_part, ecode, rcode,
            "", ok_part.size() //
        );

#undef console_red
#undef console_reset

        // TODO: do something cross-platform
        __debugbreak();
        std::exit(-1);
    }

    inline token eat(token_kind kind) noexcept;
    inline token eat(std::span<token_kind const> kinds) noexcept;

    inline entt::entity binary_node(token_kind kind, entt::entity lhs, entt::entity rhs) noexcept;

    inline static scope global_scope() noexcept;
};

inline entt::entity parser::post_expr(entt::entity base) noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::LeftParen:
        return call(base);

    case token_kind::LeftBracket:
        return index(base);

    case token_kind::Dot:
        return member(base);

    default:
        return base;
    }
}

inline entt::entity parser::call(entt::entity base) noexcept
{
    // parsing

    scan.next(); // '('
    // TODO: add `base` to the parameters list or maybe as a special node
    auto ins = expr_list_term(token_kind::RightParen); // expr,*,?)

    // codegen

    auto const is_extern = bld.reg.get<node_type>(base).type->as<func>()->is_extern;

    // TODO: is this correct? (consider for example, a call is made inside an `If`)
    auto const call = make(bld, call_node{
                                    .is_extern = is_extern,
                                    .func = base,
                                    .args = ins,
                                });
    // TODO: the call should probably modify just the memory node, not the control flow node
    // TODO: the call should probably link to the `Return` node of the called function, not the `Start`

    // TODO: inline CPS this
    return post_expr(call); // post_expr(parsed)
}

inline entt::entity parser::index(entt::entity base) noexcept
{
    // parsing

    scan.next(); // '['
    // TODO: add `base` to the parameters list or maybe as a special node
    auto const i = expr();         // expr
    eat(token_kind::RightBracket); // ']'

    // codegen

    // TODO: is this correct? (consider mutability, generalizing `Load`, etc.)
    auto const node = make(bld, load_node{.base = base, .offset = i});

    // TODO: how do you make sure it happens before any possible store, but still optimize it out if only load?

    // TODO: inline CPS this
    return post_expr(node); // post_expr(parsed)
}

inline entt::entity parser::member(entt::entity base) noexcept
{
    // parsing

    auto const dot = scan.next(); // '.'
    // TODO: add `base` to the parameters list or maybe as a special node
    auto const mem = eat(token_kind::Ident); // <ident>

    // codegen

    // HACK: handle this better
    auto ty = bld.reg.get<node_type>(base).type->as<struct_type>();
    if (!ty)
        fail(dot, "Cannot access member of type!");

    auto const mem_name = scan.lexeme(mem);
    int32_t offset = -1;
    for (size_t i{}; i < ty->n_members; ++i)
    {
        if (ty->members[i].name == mem_name)
        {
            offset = i;
            break;
        }
    }

    // TODO: find a better way to represent `Load`s at known indices
    // TODO: `int_const` is int64 but `offset` is int32
    auto const offset_node = make(bld, value_node{int_const::value(offset)});

    // TODO: is this correct? (consider mutability, generalizing `Load`, etc.)
    // TODO: calculate the offset of the member and pass it to the load nodes
    auto const node = make(bld, load_node{
                                    .base = base,
                                    .offset = offset_node,
                                });

    // TODO: how do you make sure it happens before any possible store, but still optimize it out if only load?

    // TODO: inline CPS this
    return post_expr(node); // post_expr(parsed)
}

inline entt::entity parser::primary() noexcept
{
    // TODO: CPS the `expr` calls here, repeat for `call`, `index` and `post_expr`, etc.
    using enum token_kind;

    // parsing

    token_kind const primary_tokens[]{
        // literal
        Integer,
        Decimal,
        KwFalse,
        KwTrue,
        KwNil,
        Ident,       // variable/function/type/const/whatever
        LeftParen,   // (expr)
        LeftBracket, // [] type { expr,*,? } - array initializer
        Bang,
        Minus, // unary
    };

    auto const tok = eat(primary_tokens);

    // codegen

    switch (tok.kind)
    {
    case Integer:
    {
        auto const txt = scan.lexeme(tok);

        uint64_t val{};
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        // HACK: figure out actual int size
        return make(bld, value_node{int_const::value(val)});
    }

    case Decimal:
    {
        auto const txt = scan.lexeme(tok);

        double val{};
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        // HACK: figure out actual int size
        return make(bld, value_node{new float64{val}});
    }

    case KwFalse:
        return make(bld, value_node{new bool_const{false}});
    case KwTrue:
        return make(bld, value_node{new bool_const{true}});

    // TODO: don't use integer type here anymore
    case KwNil:
        return make(bld, value_node{int_const::value(0)});

        // post_expr(<ident>)
    case Ident:
    {
        auto const name = scan.lexeme(tok);
        // TODO: recheck this after figuring out addressing
        // TODO: figure out the type of the node
        // auto const addr = bld.makeval(mem_state, op::Addr, int_bot::self(), {.i64 = 0});

        // TODO: in case of calls, this should get the function node, and in case of `{}` it should get the type node
        auto const val = env.get_var(name);
        return post_expr(val);
    }

    // post_expr( '(' expr ')' )
    case LeftParen:
    {
        auto const ret = expr(); // expr
        eat(RightParen);         // ')'
        // TODO: is this ok or should you wrap it?
        return post_expr(ret);
    }

    // post_expr( '['']' type '{' expr,*,? '}' )
    case LeftBracket:
    {
        eat(token_kind::RightBracket);                      // ']'
        auto subty = type();                                // type
        eat(token_kind::LeftBrace);                         // '{'
        auto init = expr_list_term(token_kind::RightBrace); // '}'

        // TODO: actually store all the types, not just the first one
        // TODO: you also now need a `meet` operation to give the common lowest type between two given types
        // TODO: hack, make the correct node
        // TODO: typecheck that sizes match on `var_decl`
        auto arrty = new ::array_type{subty, init.n};
        auto const ret = make(bld, value_node{arrty});
        return post_expr(ret);
    }

    // '!' expr
    case Bang:
    {
        auto const sub = expr(); // expr
        return make(bld, not_node{.sub = sub});
    }

    // '-' expr
    case Minus:
    {
        auto const sub = expr();
        return make(bld, neg_node{.sub = sub});
    }

    default:
        std::unreachable();
        return entt::null;
    }
}

inline entt::entity parser::expr(parse_prec prec) noexcept
{
    auto lhs = primary();

    parse_prec rprec;
    while (prec < (rprec = prec_table[(uint8_t)scan.peek.kind]))
    {
        auto const op = scan.next().kind;
        // TODO: if right-associative op, increase precedence of rprec by 1
        // TODO: do you even have right-associative operators?
        auto const rhs = expr(rprec);

        lhs = binary_node(op, lhs, rhs);
    }

    return lhs;
}

inline entt::entity parser::local_decl(token lhs) noexcept
{
    // parsing

    // TODO: eat this outside of this function
    eat(token_kind::Walrus); // :=
    // TODO: parse an expression list instead
    auto rhs = expr();          // expr
    eat(token_kind::Semicolon); // ;

    // codegen

    // TODO: is this correct?
    // TODO: pass a list<expr> and keep a list of their spans; if they are 'ident's their span should be the name
    auto const name = scan.lexeme(lhs);
    env.new_var(name, rhs);

    return stmt();
}

inline entt::entity parser::assign(entt::entity lhs) noexcept
{
    // TODO:
    // - the assignment should be evaluated RTL

    // parsing
    eat(token_kind::Equal); // '='

    // auto const name = scan.lexeme(id);
    // auto const lhs = env.get_var(name);

    // TODO: handle multi-expression case
    auto const rhs = expr();    // expr
    eat(token_kind::Semicolon); // ';'

    // codegen

    // TODO: is this correct? it ensures that rhs is evaluated before lhs
    // bld.reg.get<mem_effect>(lhs).target = rhs;
    // bld.state.mem = lhs;

    // TODO: do you need to codegen a `Load` for lhs?

    // TODO: update the environment
    auto const store = make(bld, store_node{
                                     .lhs = lhs,
                                     .rhs = rhs,
                                 });
    // env.set_var(name, opnode);

    return stmt();
}

inline entt::entity parser::compound_assign(entt::entity lhs) noexcept
{
    // TODO:
    // - the assignment should be evaluated RTL

    // parsing

    token_kind const ops[]{
        token_kind::PlusEqual,
        token_kind::MinusEqual,
        token_kind::StarEqual,
        token_kind::SlashEqual,
        token_kind::AndEqual,
        token_kind::XorEqual,
        token_kind::OrEqual,
    };

    auto const optok = eat(ops).kind; // compound_op

    // auto const name = scan.lexeme(id);
    // auto const lhs = env.get_var(name);

    // TODO: handle multi-expression case
    auto const rhs = expr();    // expr
    eat(token_kind::Semicolon); // ';'

    // codegen

    // TODO: is this correct? it ensures that rhs is evaluated before lhs
    bld.reg.get<mem_effect>(lhs).target = rhs;
    bld.state.mem = lhs;

    // TODO: do you need to codegen a `Load` for lhs?

    auto const opnode =
        optok == token_kind::PlusEqual
            ? make(bld, add_node{lhs, rhs})
        : optok == token_kind::MinusEqual
            ? make(bld, sub_node{lhs, rhs})
        : optok == token_kind::StarEqual
            ? make(bld, mul_node{lhs, rhs})
        : optok == token_kind::SlashEqual
            ? make(bld, div_node{lhs, rhs})
        : optok == token_kind::AndEqual
            ? make(bld, bit_and_node{lhs, rhs})
        : optok == token_kind::XorEqual
            ? make(bld, bit_xor_node{lhs, rhs})
            : make(bld, bit_or_node{lhs, rhs});

    // TODO: update the environment
    auto const store = make(bld, store_node{
                                     .lhs = lhs,
                                     .rhs = opnode,
                                 });
    // env.set_var(name, opnode);

    return stmt();
}

inline entt::entity parser::post_op(entt::entity lhs) noexcept
{
    // parsing

    token_kind const ops[]{
        token_kind::PlusPlus,
        token_kind::MinusMinus,
    };

    auto const optok = eat(ops).kind;

    eat(token_kind::Semicolon); // ;

    // codegen

    // TODO: do you need to codegen a load for lhs?

    // auto const name = scan.lexeme(id);
    // auto const lhs = env.get_var(name);
    auto const rhs = make(bld, value_node{int_const::value(1)});

    auto const opnode = optok == token_kind::PlusPlus
                            ? make(bld, add_node{lhs, rhs})
                            : make(bld, sub_node{lhs, rhs});

    // TODO: update the environment's status
    auto const store = make(bld, store_node{
                                     .lhs = lhs,
                                     .rhs = opnode,
                                 });
    // env.set_var(name, opnode);

    return stmt();
}

inline entt::entity parser::defer_stmt() noexcept
{
    // the function's state should not be affected by the `defer` when it's declared, but rather when it's called
    auto const old_state = bld.state;

    // parsing
    auto defer = eat(token_kind::KwDefer); // 'defer'
    auto call = expr();                    // call
    eat(token_kind::Semicolon);            // ';'

    bld.state = old_state;

    // TODO: add other call conventions eventually
    switch (bld.reg.get<node_op const>(call))
    {
    case node_op::CallStatic:
    case node_op::ExternCall:
        break;

    default:
        fail(defer, "Expected call expression after `defer`.");
        break;
    }

    stacklist defer_head{.value = call, .prev = defer_stack};
    defer_stack = &defer_head;

    return stmt(); // stmt
}

inline entt::entity parser::if_stmt() noexcept
{
    // TODO: ensure condition is boolean-like

    // parsing
    eat(token_kind::KwIf);    // 'if'
    auto const cond = expr(); // expr

    auto const if_yes_node = bld.make(node_op::IfYes, std::span(&cond, 1));
    bld.reg.emplace<ctrl_effect>(if_yes_node, bld.state.ctrl);

    auto const if_not_node = bld.make(node_op::IfNot, std::span(&cond, 1));
    bld.reg.emplace<ctrl_effect>(if_not_node, bld.state.ctrl);

    bld.state.ctrl = if_yes_node;

    return block(
        [&](scope const *then_env, entt::entity then_ret)
        {
            auto const then_state = bld.state.ctrl; // TODO: link this to `region` instead
            bld.state.ctrl = if_not_node;

            if (scan.peek.kind == token_kind::KwElse)
            {
                scan.next(); // 'else'

                ensure(
                    (scan.peek.kind == token_kind::KwIf || scan.peek.kind == token_kind::LeftBrace),
                    "Expected `if` or `{` after `else`!" //
                );

                // TODO: parse the `stmt` after the `if`, return or not
                // if_stmt | block, then the outer `stmt` that tails
                // TODO: also merge the `env` even on the `if` case
                return (scan.peek.kind == token_kind::KwIf)
                           // TODO: make a region for this case too
                           ? if_stmt()
                           : block([&](scope const *else_env, entt::entity else_ret)
                                   {
                                       // TODO: this should be common for both case (`else if` or just `else`)
                                       eat(token_kind::Semicolon); // ';'

                                       auto const region = make(bld, region_node{then_state, bld.state.ctrl});

                                       // TODO: all this is common on both branches
                                       // TODO: is this correct? (from here to return)
                                       // codegen
                                       merge(*env.top, *then_env, *else_env);

                                       // TODO: if either `if` or `else` has a return, codegen a phi node and return it
                                       // TODO: is this correct?
                                       // TODO: `phi` should merge the children of `return` nodes
                                       auto const merge_ret = make(bld, return_phi_node{region, then_ret, else_ret});

                                       // TODO: parse after merging nodes
                                       // TODO: this should be yet another branch, marked as `if(false)` ie. `~ctrl`
                                       auto const rest_ret = stmt(); // rest of block statements

                                       return (merge_ret != entt::null) ? merge_ret : rest_ret; //
                                   });
            }
            else
            {
                eat(token_kind::Semicolon); // ';'

                auto const region = make(bld, region_node{then_state, bld.state.ctrl});
                merge(*env.top, *then_env, *env.top); // TODO: is this correct?

                // TODO: implement
                // TODO: also merge `then_env` with `env.top` in this case
                auto const rest_ret = stmt(); // trailing stmt

                // TODO: is this correct?
                return make(bld, return_phi_node{region, then_ret, rest_ret});
            } //
        });
}

inline entt::entity parser::for_stmt() noexcept
{
    // TODO: everything here is temporary, fix the problems eventually

    // parsing

    eat(token_kind::KwFor); // 'for'
    auto cond = expr();     // expr

    auto const loop = make(bld, loop_node{});

    auto const if_yes_node = bld.make(node_op::IfYes, std::span(&cond, 1));
    bld.reg.emplace<ctrl_effect>(if_yes_node, bld.state.ctrl);
    bld.state.ctrl = if_yes_node;

    auto const for_ret = block([&](scope const *loop_env, entt::entity ret)
                               {
                                   // TODO: implement
                                   auto const region = make(bld, region_node{loop, bld.state.ctrl});

                                   // TODO: all this is common on both branches
                                   // TODO: is this correct? (from here to return)
                                   // codegen
                                   merge(*env.top, *loop_env, *env.top);

                                   // TODO: generate the "loop-back" node

                                   return ret; //
                               });

    bld.state.ctrl = loop;
    auto const rest_node = bld.make(node_op::IfNot, std::span(&cond, 1));
    bld.reg.emplace<ctrl_effect>(rest_node, bld.state.ctrl);
    bld.state.ctrl = rest_node;

    auto const rest_ret = stmt(); // stmt

    return rest_ret;
}

inline entt::entity parser::control_flow(token_kind which) noexcept
{
    // TODO: split this for better tail calls

    // parsing

    if (scan.peek.kind != token_kind::Semicolon)
    {
        auto where = eat(token_kind::Ident); // <ident>?
        // TODO: implement
    }

    eat(token_kind::Semicolon); // ;

    // codegen

    // TODO: implement
    // TODO: check that this is inside a `for`/`switch`/etc.

    scope_visibility vis;
    bld.push_vis<visibility::unreachable>(vis);

    // TODO: unreachable code
    // TODO: tail-call this instead (`unreachable_stmt(return_)`)
    (void)stmt();

    bld.pop_vis();

    return entt::null;
}

inline entt::entity parser::return_stmt() noexcept
{
    // TODO: is this correct?
    while (defer_stack)
    {
        bld.reg.get<mem_effect>(defer_stack->value).target = bld.state.mem;
        bld.state.mem = defer_stack->value;
        defer_stack = defer_stack->prev;
    }

    // parsing
    eat(token_kind::KwReturn); // 'return'

    auto outs = expr_list_term(token_kind::Semicolon); // expr,*,?;

    scope_visibility vis;
    bld.push_vis<visibility::unreachable>(vis);

    // codegen
    // NOTE: everything after `return` is unreachable code, so address that with a warning or something
    // TODO: tail-call this instead (`unreachable_stmt(return_)`)
    (void)stmt();

    bld.pop_vis();

    // TODO: return all values
    return (outs.n == 0) ? (entt::entity)entt::null : outs[0];
}

inline entt::entity parser::block(block_then then) noexcept
{
    // TODO: figure out merging with parent
    scope block_env{.prev = env.top};
    env.top = &block_env;

    eat(token_kind::LeftBrace); // {
    auto ret = stmt();          // stmt*}

    env.top = env.top->prev;
    return then(&block_env, ret);
}

inline entt::entity parser::simple_stmt() noexcept
{
    // TODO: parse a `stacklist` and convert to `smallvec`
    // TODO: error out if using a non-identifier as a lhs of a definition
    auto const lhs = expr(); // expr

    switch (scan.peek.kind)
    {
    case token_kind::Walrus:
        // TODO: enable local declarations back
        fail(scan.peek, "Local declarations are disabled for now. Please use `var` syntax in the meantime.");
        return entt::null;
        // return local_decl(lhs);

    case token_kind::Equal:
        return assign(lhs);

    case token_kind::PlusPlus:
    // TODO: enable this back
    // if (lhs.n != 1)
    //     fail(scan.peek, "Unexpected `++` after expression list");
    case token_kind::MinusMinus:
        // TODO: enable this back
        // if (lhs.n != 1)
        //     fail(scan.peek, "Unexpected `--` after expression list");

        return post_op(lhs);

    case token_kind::PlusEqual:
    case token_kind::MinusEqual:
    case token_kind::StarEqual:
    case token_kind::SlashEqual:
        // TODO: enable this back
        // if (lhs.n != 1)
        //     fail(scan.peek, "Unexpected compound assignment after expression list");

        return compound_assign(lhs);

    default:
        fail(scan.peek, "Expected one of the following: `:=`, `=`, `++`, `--`, `+=`, `-=`, `*=`, `/=`");
        return entt::null;
    }
}

inline entt::entity parser::stmt() noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::LeftBrace:
        return block([&](scope const *env, entt::entity ret)
                     {
                         // TODO: is this correct here?
                         eat(token_kind::Semicolon); // ';'
                         auto const left = stmt();

                         // TODO: also merge env with parent

                         return ret != entt::null ? ret : left; //
                     });

    case token_kind::RightBrace:
        scan.next();       // '}'
        return entt::null; // if `}` is reached, it means there was no return

    case token_kind::KwVar:
        return var_decl<true>();

    case token_kind::KwType:
        return type_decl<true>();

    case token_kind::KwDefer:
        return defer_stmt();

    case token_kind::KwReturn:
        return return_stmt();

    case token_kind::KwIf:
        return if_stmt();

    case token_kind::KwFor:
        return for_stmt();

    case token_kind::KwBreak:
    case token_kind::KwContinue:
        return control_flow(scan.next().kind);

    // empty statement
    case token_kind::Semicolon:
        scan.next();   // ';'
        return stmt(); // stmt

    default:
        return simple_stmt();
    }
}

// decl

inline void parser::const_decl() noexcept
{
    eat(token_kind::KwConst);                 // 'const'
    auto const name = eat(token_kind::Ident); // <ident>
    eat(token_kind::Equal);                   // '='
    auto const init = expr();                 // expr
    eat(token_kind::Semicolon);               // ;

    // TODO: link a `Proj` node from the global `State`
    // TODO: typecheck in case the type is specified
    // entt::entity const ins[]{init};
    // auto const store = bld.make(node_op::Store, ins);
    // auto &&ty = bld.reg.get<node_type>(store).type;
    // bld.reg.get<node_type>(store).type = new const_type{ty};
    // bld.reg.get_or_emplace<effect>(store).target = mem_state;
    // mem_state = store;

    // TODO: mark the name as non-modifiable somehow
    env.new_var(scan.lexeme(name), init);

    // NOTE: no need to prune dead code here, everything is done at compile time

    // TODO: `prune_dead_code` is common for every declaration, find a way to address this
    // ^ merge `prune_dead_code` and `decl` so you don't jump around; make sure it's a tail call
    // NOTE: you still need to prune here to cut temporary nodes
    prune_dead_code(bld, init);

    // TODO: inline CPS this
    decl();
}

inline void parser::inline_func_decl() noexcept
{
    // TODO: is this correct?
    scope_visibility vis;
    bld.push_vis<visibility::maybe_reachable>(vis);

    // some setup codegen before parsing
    // NOTE: this needs to be here because parameters depend on `mem_state`
    // TODO: rollback this in case of errors during parsing
    // TODO: `Start` is a value node; address that
    // TODO: `Start` should have a `$ctrl` child and `arg`, which are separate; address that
    // TODO: are `CtrlState` and `MemState` the same node initially? when inlining they might be different
    auto const old_state = bld.new_func();

    // parsing

    eat(token_kind::KwFunc);               // 'func'
    auto nametok = eat(token_kind::Ident); // ident
    eat(token_kind::LeftParen);            // '('

    // TODO: environment pointer should be restored to before parameters, not after
    // ^ also declare the function name before params; the environment should contain it
    // ^ or rather just mark it as unresolved and resolve it later
    // TODO: parse the parameters + return in a separate function, return the node id

    // param_decl,*,?
    int64_t param_i = 0;
    std::vector<value_type const *> param_types;

    while (scan.peek.kind != token_kind::RightParen)
    {
        auto const ty = param_decl(param_i++); // param_decl
        param_types.push_back(ty);

        if (scan.peek.kind != token_kind::Comma)
            break;
        scan.next(); // ,
    }

    eat(token_kind::RightParen); // ')'

    auto param_types_list = std::make_unique_for_overwrite<value_type const *[]>((size_t)param_i);
    std::copy_n(param_types.data(), param_i, param_types_list.get());

    // type?
    auto const ret_type = (scan.peek.kind != token_kind::LeftBrace)
                              ? type()
                              : void_type::self();

    bld.reg.get<node_type>(bld.state.mem).type = new func{false, ret_type, (size_t)param_i, std::move(param_types_list)};

    auto name = scan.lexeme(nametok);
    if (env.top->defs.contains(name))
        fail(nametok, "Function already defined");

    // TODO: should the function point to the `Start`, `Return`, or where?
    // ^ you can actually pre-define the `Return` node here, attach it to the env table, then set the node's inputs accordingly
    // ^ this way you don't need to specially handle the case where a `return void` function does not have a `return` stmt
    // TODO: should it be memory or ctrl? probably ctrl since after parsing it points to the `Return`
    // TODO: ^ maybe all state nodes, rather than just one
    env.top->defs.insert({name, bld.state.mem});

    // TODO: typecheck that the return type matches what's expected
    // TODO: is this actually the `Return` node?
    auto const ret = block([&](scope const *func_env, entt::entity ret)
                           {
                               eat(token_kind::Semicolon); // ;

                               // TODO: handle multi-return case
                               entt::entity const params[]{ret};
                               auto const out = make(bld, return_node{params});

                               // TODO: move this to a function
                               // TODO: handle assignment to constants
                               // TODO: only merge variables, not functions/types, etc.
                               // TODO: codegen loads + stores
                               // TODO: is there any in-between scope that has not been merged?
                               for (auto &&[name, id] : env.top->defs)
                               {
                                   auto const iter = func_env->defs.find(name);
                                   if (iter != func_env->defs.end())
                                       id = iter->second;
                               }

                               return out; //
                           });

    bld.state = old_state;
    prune_dead_code(bld, ret);

    // TODO: is this correct?
    bld.pop_vis();

    ensure(defer_stack == nullptr, "Internal compiler error: defer stack not cleared");

    decl(); // TODO: maybe call this inside the `block`?
}

inline void parser::extern_func_decl() noexcept
{
    // TODO: simplify this; you just need to add the declaration
    // ^ do not codegen `Proj` for the parameters, just parse their type
    // ^ just make a `ExternFunc` node to keep the foreign function information (name + type)

    // TODO: is this correct?
    scope_visibility vis;
    bld.push_vis<visibility::maybe_reachable>(vis);

    // some setup codegen before parsing
    // NOTE: this needs to be here because parameters depend on `mem_state`
    // TODO: rollback this in case of errors during parsing
    // TODO: `Start` is a value node; address that
    // TODO: `Start` should have a `$ctrl` child and `arg`, which are separate; address that
    // TODO: are `CtrlState` and `MemState` the same node initially? when inlining they might be different
    auto const old_state = bld.new_func();

    // parsing

    eat(token_kind::At);                      // '@'
    auto const attr = eat(token_kind::Ident); // <ident>
    if (scan.lexeme(attr) != "extern")
        fail(attr, "Parsed unknown annotation");

    eat(token_kind::KwFunc);               // 'func'
    auto nametok = eat(token_kind::Ident); // ident
    eat(token_kind::LeftParen);            // '('

    // TODO: environment pointer should be restored to before parameters, not after
    // ^ also declare the function name before params; the environment should contain it
    // ^ or rather just mark it as unresolved and resolve it later
    // TODO: parse the parameters + return in a separate function, return the node id

    // param_decl,*,?
    int64_t param_i = 0;
    std::vector<value_type const *> param_types;

    while (scan.peek.kind != token_kind::RightParen)
    {
        auto const ty = param_decl(param_i++); // param_decl
        param_types.push_back(ty);

        if (scan.peek.kind != token_kind::Comma)
            break;
        scan.next(); // ,
    }

    eat(token_kind::RightParen); // ')'

    auto param_types_list = std::make_unique_for_overwrite<value_type const *[]>((size_t)param_i);
    std::copy_n(param_types.data(), param_i, param_types_list.get());

    // type?
    auto const ret_type = (scan.peek.kind != token_kind::Semicolon)
                              ? type()
                              : void_type::self();

    eat(token_kind::Semicolon); // ';'

    bld.reg.get<node_type>(bld.state.mem).type = new func{true, ret_type, (size_t)param_i, std::move(param_types_list)};

    auto name = scan.lexeme(nametok);
    if (env.top->defs.contains(name))
        fail(nametok, "Function already defined");

    // TODO: should the function point to the `Start`, `Return`, or where?
    // ^ you can actually pre-define the `Return` node here, attach it to the env table, then set the node's inputs accordingly
    // ^ this way you don't need to specially handle the case where a `return void` function does not have a `return` stmt
    // TODO: should it be memory or ctrl? probably ctrl since after parsing it points to the `Return`
    // TODO: ^ maybe all state nodes, rather than just one
    env.top->defs.insert({name, bld.state.mem});

    bld.state = old_state;

    // TODO: is this correct?
    bld.pop_vis();

    decl(); // TODO: maybe call this inside the `block`?
}

template <bool AsStmt>
inline auto parser::var_decl() noexcept -> std::conditional_t<AsStmt, decltype(stmt()), decltype(decl())>
{
    // parsing

    eat(token_kind::KwVar);                   // 'var'
    auto const name = eat(token_kind::Ident); // <ident>
    auto const ty = type();                   // type
    eat(token_kind::Equal);                   // '='
    auto const init = expr();                 // expr
    eat(token_kind::Semicolon);               // ;

    // codegen

    env.new_var(scan.lexeme(name), init);

    if constexpr (AsStmt)
        return stmt();
    else
    {
        // TODO: `prune_dead_code` is common for every declaration, find a way to address this
        // ^ merge `prune_dead_code` and `decl` so you don't jump around; make sure it's a tail call
        // - probably it's best to tail-call `decl` from `prune_dead_code`
        prune_dead_code(bld, init);

        decl();
    }
}

template <bool AsStmt>
inline auto parser::type_decl() noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>
{
    // parsing

    eat(token_kind::KwType);               // 'type'
    auto nametok = eat(token_kind::Ident); // <ident>

    switch (scan.peek.kind)
    {
    case token_kind::KwStruct:
        return struct_decl<AsStmt>(nametok);

    default:
        return alias_decl<AsStmt>(nametok);
    }
}

template <bool AsStmt>
inline auto parser::alias_decl(token nametok) noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>
{
    auto ty = type();           // type
    eat(token_kind::Semicolon); // ';'

    // codegen

    auto name = scan.lexeme(nametok);

    // TODO: check type is not already defined
    // TODO: are type names shadowed? if not, search the whole environment
    if (env.top->types.contains(name))
        fail(nametok, "Type is already defined!");

    env.top->types.insert({name, ty});

    // NOTE: no need to prune dead code here, everything is done at compile time

    if constexpr (AsStmt)
        return stmt();
    else
        decl();
}

template <bool AsStmt>
inline auto parser::struct_decl(token nametok) noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>
{
    auto parse_members = [&](auto &&self, stacklist<member_info> *members, size_t n) -> value_type const *
    {
        switch (scan.peek.kind)
        {
        case token_kind::RightBrace:
        {
            scan.next(); // '}'

            auto mem_ptr = new member_info[n];
            auto iter = members;
            for (size_t i{}; i < n; ++i)
            {
                mem_ptr[n - i - 1] = iter->value;
                iter = iter->prev;
            }

            return new struct_type{n, std::unique_ptr<member_info[]>(mem_ptr)};
        }

        case token_kind::Ident:
        {
            auto const mem_name = scan.next(); // <ident>
            auto const mem_ty = type();        // type
            eat(token_kind::Semicolon);        // ';'

            auto mem_node = stacklist{.value = member_info{.name = scan.lexeme(mem_name), .type = mem_ty}, .prev = members};
            return self(self, &mem_node, n + 1);
        }

        default:
            fail(scan.peek, "Expected '}' or <identifier>");
            return nullptr;
        }
    };

    // parsing
    eat(token_kind::KwStruct);                          // 'struct'
    eat(token_kind::LeftBrace);                         // '{'
    auto ty = parse_members(parse_members, nullptr, 0); // (member_decl ';')* '}'
    eat(token_kind::Semicolon);                         // ';'

    auto name = scan.lexeme(nametok);
    env.top->types.insert({name, ty});

    // NOTE: no need to prune dead code here, everything is done at compile time

    if constexpr (AsStmt)
        return stmt();
    else
        decl();
}

inline void parser::decl() noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::KwConst:
        return const_decl();

    case token_kind::KwFunc:
        return inline_func_decl();

    case token_kind::At:
        return extern_func_decl();

    case token_kind::KwVar:
        return var_decl<false>();

    case token_kind::KwType:
        return type_decl<false>();

    case token_kind::Eof:
        return codegen_main();

    default:
        return fail(scan.peek, "Expected `func` or <EOF>");
    }
}

inline void parser::prog() noexcept
{
    // TODO: is this correct?
    scope_visibility vis;
    bld.push_vis<visibility::global>(vis);

    (void)bld.new_func();

    decl(); // decl*
}

// helper rules

inline value_type const *parser::param_decl(int64_t i) noexcept
{
    // parsing

    auto const nametok = eat(token_kind::Ident); // name
    auto const ty = type();                      // type

    // codegen

    // TODO: recheck this
    auto const node = bld.make(node_op::Proj, std::span(&bld.state.mem, 1));
    // TODO: figure out the exact type of this
    // TODO: is this correct now?
    bld.reg.get<node_type>(node).type = ty;

    // TODO: these params should be defined in the function's block, not on global env
    auto const name = scan.lexeme(nametok);
    env.new_var(name, node);

    return ty;
}

inline smallvec parser::expr_list_term(token_kind term) noexcept
{
    auto parse_arguments = [&](auto &&self, stacklist<entt::entity> *ins, int list_size = 0)
        -> smallvec
    {
        // TODO: is the control flow correct here?
        if (scan.peek.kind != term)
        {
            auto const arg = expr(); // expr
            stacklist node{.value = arg, .prev = ins};
            ins = &node;
            ++list_size;

            if (scan.peek.kind == token_kind::Comma)
            {
                scan.next(); // ','
                return self(self, ins, list_size);
            }
        }

        eat(term); // term
        return compress(ins, list_size);
    };

    return parse_arguments(parse_arguments, nullptr, 0);
}

inline smallvec parser::expr_list() noexcept
{
    auto const first = expr(); // expr
    stacklist head{.value = first};

    auto parse_arguments = [&](auto &&self, stacklist<entt::entity> *ins, int list_size = 0)
        -> smallvec
    {
        // TODO: is the control flow correct here?
        if (scan.peek.kind == token_kind::Comma)
        {
            scan.next();             // ','
            auto const arg = expr(); // expr
            stacklist node{.value = arg, .prev = ins};

            return self(self, &node, list_size + 1);
        }

        return compress(ins, list_size);
    };

    return parse_arguments(parse_arguments, &head, 1);
}

inline value_type const *parser::type() noexcept
{
    // TODO: mark the type node if not declared yet
    // TODO: inline CPS this
    switch (scan.peek.kind)
    {
    case token_kind::LeftBracket:
        return array_type();

    case token_kind::Ident:
        return named_type();

    default:
        fail(scan.peek, "Expected `[' or <identifier>");
        return nullptr;
    }
}

inline value_type const *parser::array_type() noexcept
{
    // TODO: inline the check on `eat`
    eat(token_kind::LeftBracket);                // '['
    auto const n_tok = eat(token_kind::Integer); // <integer>
    eat(token_kind::RightBracket);               // ']'

    auto const txt = scan.lexeme(n_tok);

    uint64_t n{};
    std::from_chars(txt.data(), txt.data() + txt.size(), n);

    auto base = type(); // TODO: you can CPS this and pass a `then` call that is propagated up to `named_type`
    return new ::array_type{base, n};
}

inline value_type const *parser::named_type() noexcept
{
    // TODO: inline the check of `eat`
    auto const nametok = eat(token_kind::Ident); // type

    // TODO: make this into a function
    auto name = scan.lexeme(nametok);
    for (auto scope = env.top; scope; scope = scope->prev)
    {
        auto iter = scope->types.find(name);
        if (iter != scope->types.end())
            return iter->second;
    }

    // TODO: error out here
    return bot_type::self();
}

// codegen helpers

inline void parser::codegen_main() noexcept
{
    // ensure the storages we need to use exist
    // this is needed because the registry is passed as `const` to the backend
    bld.reg.storage<node_type>();
    bld.reg.storage<node_inputs>();
    bld.reg.storage<ctrl_effect>();
    bld.reg.storage<mem_effect>();
    bld.reg.storage<region_of_phi>();

    // TODO: most of these checks should be made by `call_static`
    auto const main_iter = env.top->defs.find(std::string_view{"main"});
    ensure(main_iter != env.top->defs.end(), "Program does not define a main function!");
    auto const main_node = main_iter->second;
    auto const main_ty = bld.reg.get<node_type const>(main_node).type;
    ensure(main_ty->as<func>(), "Program does not define a main function!");

    // TODO: should you pass `main_node` to the inputs of the function?
    entt::entity const main_ins[]{entt::null};
    auto const call_main = make(bld, call_node{
                                         .is_extern = false,
                                         .func = main_node,
                                         .args = std::span(main_ins, 0),
                                     });
    ensure(bld.reg.get<node_type>(call_main).type->as<void_type>(), "Function `main` cannot return a value!");

    // TODO: is this correct?
    auto const exit = make(bld, exit_node{
                                    .code = 0,
                                });
    // TODO: DCE on unused functions

    bld.pop_vis(); // just for correctness
}

// parsing helpers

inline token parser::eat(token_kind kind) noexcept
{
    if (scan.peek.kind == kind)
        return scan.next();

    // TODO: unified error interface
    fail(scan.peek, "Unexpected token.");
}

inline token parser::eat(std::span<token_kind const> kinds) noexcept
{
    for (auto k : kinds)
        if (scan.peek.kind == k)
            return scan.next();

    // TODO: unified error interface
    fail(scan.peek, "Unexpected token.");
}

inline void parser::merge(scope &parent, scope const &lhs, scope const &rhs) noexcept
{
    // invariant: `lhs` and `rhs` are branches of `parent` and never `null`

    // TODO: hash the `key` for faster lookup

    // TODO: only merge variables, nothing else
    for (auto &&[key, value] : parent.defs)
    {
        // NOTE: `lhs` and `rhs` have every key from `parent` and a node is needed only if there is a change in either lhs or rhs
        // ^ this means you can search until you reach the beginning of `parent` for a variable and ignore everything before
        // TODO: handle case where `else` is missing
        auto const left = lhs.defs.contains(key) ? lhs.defs.at(key) : entt::null;
        auto const right = rhs.defs.contains(key) ? rhs.defs.at(key) : entt::null;

        auto const node = phi(value, left, right);
        value = node;
    }
}

inline entt::entity parser::binary_node(token_kind kind, entt::entity lhs, entt::entity rhs) noexcept
{
    using enum token_kind;
    using enum node_op;

    switch (kind)
    {
    case Plus:
        return make(bld, add_node{lhs, rhs});
    case Minus:
        return make(bld, sub_node{lhs, rhs});
    case Star:
        return make(bld, mul_node{lhs, rhs});
    case Slash:
        return make(bld, div_node{lhs, rhs});

    case BangEqual:
        return make(bld, ne_node{lhs, rhs});
    case EqualEqual:
        return make(bld, eq_node{lhs, rhs});
    case Less:
        return make(bld, lt_node{lhs, rhs});
    case LessEqual:
        return make(bld, le_node{lhs, rhs});
    case Greater:
        return make(bld, gt_node{lhs, rhs});
    case GreaterEqual:
        return make(bld, ge_node{lhs, rhs});

    case AndAnd:
        return make(bld, logic_and_node{lhs, rhs});
    case OrOr:
        return make(bld, logic_or_node{lhs, rhs});

    case And:
        return make(bld, bit_and_node{lhs, rhs});
    case Xor:
        return make(bld, bit_xor_node{lhs, rhs});
    case Or:
        return make(bld, bit_or_node{lhs, rhs});

    default:
        std::unreachable();
        return entt::null;
    }
}

inline scope parser::global_scope() noexcept
{
    // TODO: eventually this can be replaced for an implicit `import "builtin"`

    // defs
    entt::dense_map<std::string_view, entt::entity, dual_hash, dual_cmp> defs;
    // TODO: add built-in functions, such as `print`

    // types
    entt::dense_map<std::string_view, value_type const *, dual_hash, dual_cmp> types;
    types.insert({"int", int_bot::self()});
    types.insert({"bool", bool_bot::self()});
    types.insert({"float64", float_bot::self()}); // HACK: replace with `float64_bot`
    // TODO: more built-in types

    return scope{
        .defs = defs,
        .types = types,
        .prev = nullptr,
    };
}