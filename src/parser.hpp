
#pragma once

#include <algorithm>
#include <charconv>

#include "builder/all.hpp"
#include "env.hpp"
#include "scanner.hpp"

#include "types/all.hpp"

// TODO:
// - most calls to block need to be followed by a semicolon, except for the block after an `if` and before an `else`
// ^ can you simply require no semicolon after `}` and avoid automatic semicolon insertion for it?
// ^ this is not optimal as `Type{}` also uses `}` and a `;` is a valid following
// - (maybe) literals should have a special type that can implicitly downcast as long as it fits? (eg. 257 cannot fit in a `uint8`)
// ^ or maybe you can check these stuff in `type::value` instead and return an error on failure?
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
// - make `then` be a `function_ref`
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
    // expr

    // call_index_or_self(base) ::= call(base) | index(base) | base
    // ^ie. it's `base` + any possible call following it
    [[nodiscard]]
    inline entt::entity call_index_or_self(entt::entity base) noexcept;

    // call(base) ::= call__index_or_self( base '(' expr,*,? ')' )
    // ^ meaning this rule will parse a call, then invoke `call_index_or_self` with the parsed call as `base`
    [[nodiscard]]
    inline entt::entity call(entt::entity base) noexcept;

    // index(base) ::= call__index_or_self( base '[' expr ']' )
    // ^ meaning this rule will parse an index, then invoke `call_index_or_self` with the parsed index as `base`
    [[nodiscard]]
    inline entt::entity index(entt::entity base) noexcept;

    // primary ::= 'true'
    //           | 'false'
    //           | <integer>
    //           | <decimal>
    //           | <unary_op> expr
    //           | '(' expr ')'
    //           | call_index_or_self(<ident>)
    // <unary_op> ::= '!' | '-'
    [[nodiscard]]
    inline entt::entity primary() noexcept;

    // expr ::= primary (<binary_op> primary)*
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
    inline entt::entity local_decl(std::span<token const> ids) noexcept;
    // expr comp_op expr ';' stmt
    // comp_op ::= '+=' | '-=' | '*=' | '/=' | '&=' | '^=' | '|='
    [[nodiscard]]
    inline entt::entity compound_assign(std::span<token const> ids) noexcept;
    // expr post_op ';' stmt
    // post_op ::= '++' | '--'
    [[nodiscard]]
    inline entt::entity post_op(std::span<token const> ids) noexcept;

    // 'if' expr block ('else' ( if_stmt | block ))? ';' stmt
    [[nodiscard]]
    inline entt::entity if_stmt() noexcept;
    // 'for' expr block stmt
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
    inline entt::entity block(auto &&then) noexcept;
    // local_decl | compound_assign | post_op
    [[nodiscard]]
    inline entt::entity simple_stmt() noexcept;

    // stmt ::= '}'
    //        | block stmt
    //        | if_stmt
    //        | for_stmt
    //        | return_stmt
    //        | break_stmt
    //        | continue_stmt
    //        | simple_stmt
    [[nodiscard]]
    inline entt::entity stmt() noexcept;

    // decl

    // 'const' <ident> '=' expr ';' decl
    inline void const_decl() noexcept;
    // 'func' <ident> '(' param_decl,*, ')' type? block ';' decl
    inline void func_decl() noexcept;
    // 'var' <ident> type '=' expr ';' decl
    inline void var_decl() noexcept;
    // const_decl | func_decl | var_decl
    inline void decl() noexcept;
    // decl
    inline void prog() noexcept;

    scanner scan;
    builder bld;

    scope global = global_scope();
    env env{.top = &global};

    entt::entity mem_state;

private:
    // helpers
    // <ident> <ident> (name type)
    // i is the index of the parameter in the function declaration (used by the Proj node emitted for the parameter)
    inline value_type const *param_decl(int64_t i) noexcept;

    // expr,*,? term
    inline smallvec expr_list(token_kind term) noexcept;

    // type

    // array_type | named_type
    inline value_type const *type() noexcept;

    // '[' <integer> ']' type
    inline value_type const *array_type() noexcept;

    // <ident>
    inline value_type const *named_type() noexcept;

    inline void merge(scope &parent, scope const &lhs, scope const &rhs) noexcept;

    inline entt::entity region(entt::entity then_state, entt::entity else_state) noexcept
    {

        // TODO: does this part belong here?
        // TODO: only add one region at the end; when the whole `if-else` tree is parsed
        // ^ even if there is just an `if` branch, the `else` is implicit on both `Region` and `Phi`, you just need to figure out its value on both cases
        // ^ (maybe) this can be done by a pass instead?
        entt::entity const region_children[]{then_state, else_state};
        // TODO: the nodes should be effect nodes, not input nodes
        auto const region = bld.make(node_op::Region, region_children);
        mem_state = region; // TODO: should this be here?
        return region;
        // TODO:
        // - spawn a Region node here that links to the if/else branches or just the `If`
        // ^ also take care to handle multiple if-else case
        // - spawn a Phi node for merging values
    }

    // merge `lhs` and `rhs` into a new phi node if either of the nodes is different from `old` and return it, otherwise return `old`
    // TODO: if no `else` branch, `rhs` should be `old`
    // TODO: move this on the builder API
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

        auto const ret = bld.make(node_op::Phi, ins);
        (void)bld.reg.emplace<region_of_phi>(ret, old); // TODO: specify the actual region here and run passes
        return ret;
    }

    // codegen the `Start` and `Exit` nodes of the program, pass the control flow to `main` (and initializing globals when added).
    inline void codegen_main() noexcept;

    // parsing helpers

    inline token eat(token_kind kind) noexcept;
    inline token eat(std::span<token_kind const> kinds) noexcept;

    inline entt::entity binary_node(token_kind kind, entt::entity lhs, entt::entity rhs) noexcept;

    inline static scope global_scope() noexcept;
};

inline entt::entity parser::call_index_or_self(entt::entity base) noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::LeftParen:
        return call(base);

    case token_kind::LeftBracket:
        return index(base);

    default:
        return base;
    }
}

inline entt::entity parser::call(entt::entity base) noexcept
{
    // parsing

    scan.next(); // '('
    // TODO: add `base` to the parameters list or maybe as a special node
    auto ins = expr_list(token_kind::RightParen); // expr,*,?)

    // codegen

    // TODO: is this correct? (consider for example, a call is made inside an `If`)
    auto const call = bld.make(node_op::CallStatic, ins);
    // TODO: the call should probably modify just the memory node, not the control flow node
    // TODO: the call should probably link to the `Return` node of the called function, not the `Start`
    bld.reg.emplace<effect>(call, mem_state);

    // TODO: is this correct? (only if there is a side effect)
    mem_state = call;

    // TODO: inline CPS this
    return call_index_or_self(call); // call_index_or_self(parsed)
}

inline entt::entity parser::index(entt::entity base) noexcept
{
    // parsing

    scan.next(); // '['
    // TODO: add `base` to the parameters list or maybe as a special node
    auto const i = expr();         // expr
    eat(token_kind::RightBracket); // ']'

    // codegen

    entt::entity const ins[]{base, i};

    // TODO: is this correct? (consider mutability, generalizing `Load`, etc.)
    auto const node = bld.make(node_op::Load, ins);
    // TODO: the call should probably modify just the memory node, not the control flow node
    // TODO: the call should probably link to the `Return` node of the called function, not the `Start`
    bld.reg.emplace<effect>(node, mem_state);

    // TODO: is this correct? how do you make sure it happens before any possible store, but still optimize it out if only load?
    mem_state = node;

    // TODO: inline CPS this
    return call_index_or_self(node); // call_index_or_self(parsed)
}

inline entt::entity parser::primary() noexcept
{
    // TODO: CPS the `expr` calls here, repeat for `call`, `index` and `call_index_or_self`, etc.
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
        return bld.make(value_node{int_const::value(val)});
    }

    case Decimal:
    {
        auto const txt = scan.lexeme(tok);

        double val{};
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        // HACK: figure out actual int size
        return bld.make(value_node{new float64{val}});
    }

    case KwFalse:
        return bld.make(value_node{new bool_const{false}});
    case KwTrue:
        return bld.make(value_node{new bool_const{true}});

    // TODO: don't use integer type here anymore
    case KwNil:
        return bld.make(value_node{int_const::value(0)});

        // call_index_or_self(<ident>)
    case Ident:
    {
        auto const name = scan.lexeme(tok);
        // TODO: recheck this after figuring out addressing
        // TODO: figure out the type of the node
        // auto const addr = bld.makeval(mem_state, op::Addr, int_bot::self(), {.i64 = 0});

        // TODO: in case of calls, this should get the function node
        auto const val = env.get_var(name);
        return call_index_or_self(val);
    }

    // '(' expr ')'
    case LeftParen:
    {
        auto const ret = expr(); // expr
        eat(RightParen);         // ')'
        // TODO: is this ok or should you wrap it?
        return ret;
    }

    // '['']' type '{' expr,*,? '}''
    case LeftBracket:
    {
        eat(token_kind::RightBracket);                 // ']'
        auto subty = type();                           // type
        eat(token_kind::LeftBrace);                    // '{'
        auto init = expr_list(token_kind::RightBrace); // '}'

        // TODO: actually store all the types, not just the first one
        // TODO: you also now need a `meet` operation to give the common lowest type between two given types
        // TODO: hack, make the correct node
        // TODO: typecheck that sizes match on `var_decl`
        auto const ret = bld.make(node_op::Const);
        bld.reg.get<node_type>(ret).type = new ::array_type{bld.reg.get<node_type const>(init[0]).type, init.n};
        return ret;
    }

    // '!' expr
    case Bang:
    {
        auto const sub = expr(); // expr
        return bld.make(unot_node{.sub = sub});
    }

    // '-' expr
    case Minus:
    {
        auto const sub = expr();
        return bld.make(uneg_node{.sub = sub});
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

inline entt::entity parser::local_decl(std::span<token const> ids) noexcept
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
    auto const name = scan.lexeme(ids[0]);
    env.new_var(name, rhs);
    // TODO: is this the correct node?
    // bld.effects[rhs->id] = mem_state;

    return stmt();
}

inline entt::entity parser::compound_assign(std::span<token const> ids) noexcept
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

    auto const rhs = expr(); // expr

    eat(token_kind::Semicolon); // ';'

    // codegen
    auto const node_op =
        optok == token_kind::PlusEqual
            ? node_op::Add
        : optok == token_kind::MinusEqual
            ? node_op::Sub
        : optok == token_kind::StarEqual
            ? node_op::Mul
        : optok == token_kind::SlashEqual
            ? node_op::Div
        : optok == token_kind::AndEqual
            ? node_op::BitAnd
        : optok == token_kind::XorEqual
            ? node_op::BitXor
            : node_op::BitOr;

    // TODO: do NOT use the lexeme here, use the address of lhs (eg. `a.b` is not a single token)
    auto const name = scan.lexeme(ids[0]);

    // TODO: use an actual address here
    // TODO: figure out the type of the node
    // auto const res = bld.makeval(op::Addr, int_bot::self(), {.i64 = 0});

    entt::entity const ins[]{env.get_var(name), rhs};
    auto const opnode = bld.make(node_op, ins);
    // TODO: fold `opnode` if possible
    // opnode = passes.run(opnode);

    env.set_var(name, opnode);
    return stmt();
}

inline entt::entity parser::post_op(std::span<token const> ids) noexcept
{
    // parsing

    token_kind const ops[]{
        token_kind::PlusPlus,
        token_kind::MinusMinus,
    };

    auto const optok = eat(ops).kind;

    eat(token_kind::Semicolon);

    // codegen
    // TODO: do NOT use the lexeme here, use the address of the result
    auto const name = scan.lexeme(ids[0]);

    // TODO: use an actual address here
    // TODO: figure out the type of the node
    // auto const res = bld.makeval(mem_state, op::Addr, int_bot::self());

    auto const lhs = env.get_var(name);
    auto const rhs = bld.make(value_node{int_const::value(1)});

    auto const opnode = optok == token_kind::PlusPlus
                            ? bld.make(add_node{lhs, rhs})
                            : bld.make(sub_node{lhs, rhs});

    env.set_var(name, opnode);

    return stmt();
}

inline entt::entity parser::if_stmt() noexcept
{
    // TODO: ensure condition is boolean-like

    // parsing
    eat(token_kind::KwIf);    // 'if'
    auto const cond = expr(); // expr

    // TODO: is this mem_state or $ctrl?
    auto const if_yes_node = bld.make(node_op::IfYes, std::span(&cond, 1));
    bld.reg.emplace<effect>(if_yes_node, mem_state);

    // TODO: is this mem_state or $ctrl?
    auto const if_not_node = bld.make(node_op::IfNot, std::span(&cond, 1));
    bld.reg.emplace<effect>(if_not_node, mem_state);

    mem_state = if_yes_node;

    return block(
        [&](scope const *then_env, entt::entity then_ret)
        {
            auto const then_state = mem_state; // TODO: link this to `region` instead
            mem_state = if_not_node;

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
                                       eat(token_kind::Semicolon); // ';'

                                       mem_state = region(then_state, mem_state);

                                       // TODO: all this is common on both branches
                                       // TODO: is this correct? (from here to return)
                                       // codegen
                                       merge(*env.top, *then_env, *else_env);

                                       // TODO: if either `if` or `else` has a return, codegen a phi node and return it
                                       // TODO: is this correct?
                                       // TODO: `phi` should merge the children of `return` nodes
                                       auto const merge_ret = bld.make(return_phi{mem_state, then_ret, else_ret});

                                       // TODO: parse after merging nodes
                                       // TODO: this should be yet another branch, marked as `if(false)` ie. `~ctrl`
                                       auto const rest_ret = stmt(); // rest of block statements

                                       return (merge_ret != entt::null) ? merge_ret : rest_ret; //
                                   });
            }
            else
            {
                eat(token_kind::Semicolon); // ';'

                mem_state = region(then_state, mem_state);

                // TODO: implement
                // TODO: also merge `then_env` with `env.top` in this case
                auto const rest_ret = stmt(); // trailing stmt

                // TODO: is this correct?
                return bld.make(return_phi{mem_state, then_ret, rest_ret});
            } //
        });
}

inline entt::entity parser::for_stmt() noexcept
{
    // parsing
    eat(token_kind::KwFor); // 'for'

    auto cond = expr();
    // TODO: correct this
    return block([](scope const *env, entt::entity ret)
                 {
                     return ret; //
                 });

    // codegen

    // TODO: implement
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
    // parsing
    eat(token_kind::KwReturn); // 'return'

    auto outs = expr_list(token_kind::Semicolon); // expr,*,?;

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

inline entt::entity parser::block(auto &&then) noexcept
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
    std::vector<token> ids;
    ids.push_back(eat(token_kind::Ident)); // ident

    // (,ident)*
    while (scan.peek.kind == token_kind::Comma)
    {
        scan.next(); // ,

        ids.push_back(eat(token_kind::Ident)); // ident
    }

    switch (scan.peek.kind)
    {
    case token_kind::Walrus:
        return local_decl(ids);

    case token_kind::PlusPlus:
    case token_kind::MinusMinus:
        return post_op(ids);

    case token_kind::PlusEqual:
    case token_kind::MinusEqual:
    case token_kind::StarEqual:
    case token_kind::SlashEqual:
        return compound_assign(ids);

    default:
        fail("Expected one of the following: `:=`, `++`, `--`, `+=`, `-=`, `*=`, `/=`");
        return entt::null;
        // TODO: return an "error type" (top I guess?)
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

    case token_kind::KwReturn:
        return return_stmt();

    case token_kind::KwIf:
        return if_stmt();

    case token_kind::KwFor:
        return for_stmt();

    case token_kind::KwBreak:
    case token_kind::KwContinue:
        return control_flow(scan.next().kind);

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
    eat(token_kind::Semicolon);               // ';'

    // TODO: do you need to codegen `Store` for all kinds of constants?
    // TODO: link a `Proj` node from the global `State`
    entt::entity const ins[]{init};
    auto const store = bld.make(node_op::Store, ins);
    auto &&ty = bld.reg.get<node_type>(store).type;
    bld.reg.get<node_type>(store).type = new const_type{ty};
    bld.reg.get_or_emplace<effect>(store).target = mem_state;
    mem_state = store;

    // TODO: mark the name as non-modifiable somehow
    env.new_var(scan.lexeme(name), store);

    // TODO: `prune_dead_code` is common for every declaration, find a way to address this
    // ^ merge `prune_dead_code` and `decl` so you don't jump around; make sure it's a tail call
    prune_dead_code(bld, store);

    decl();
}

inline void parser::func_decl() noexcept
{
    // some setup codegen before parsing
    // NOTE: this needs to be here because parameters depend on `mem_state`
    // TODO: rollback this in case of errors during parsing
    // TODO: `Start` is a value node; address that
    // TODO: `Start` should have a `$ctrl` child and `arg`, which are separate; address that
    auto const old_state = mem_state;
    mem_state = bld.make(node_op::Start);

    // parsing

    eat(token_kind::KwFunc);               // 'func'
    auto nametok = eat(token_kind::Ident); // ident
    eat(token_kind::LeftParen);            // '('

    // TODO: environment pointer should be restored to before parameters, not after
    // ^ also declare the function name before params; the environment should contain it
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

    auto const ret_type = (scan.peek.kind != token_kind::LeftBrace)
                              ? type()
                              : void_type::self();

    bld.reg.get<node_type>(mem_state).type = new func{ret_type, (size_t)param_i, std::move(param_types_list)};

    auto name = scan.lexeme(nametok);
    ensure(!env.top->defs.contains(name), "Function already defined");

    // TODO: should the function point to the `Start`, `Return`, or where?
    // ^ you can actually pre-define the `Return` node here, attach it to the env table, then set the node's inputs accordingly
    // ^ this way you don't need to specially handle the case where a `return void` function does not have a `return` stmt
    env.top->defs.insert({name, mem_state});

    // TODO: typecheck that the return type matches what's expected
    // TODO: is this actually the `Return` node?
    auto const ret = block([&](scope const *func_env, entt::entity ret)
                           {
                               // TODO: is this correct here?
                               eat(token_kind::Semicolon); // ';'

                               // TODO: handle multi-return case
                               entt::entity const params[]{ret};
                               auto const out = bld.make(node_op::Return, std::span(params, ret != entt::null));
                               // TODO: recheck this
                               (void)bld.reg.get_or_emplace<effect>(out, mem_state);

                               // TODO: move this to a function
                               // TODO: handle assignment to constants
                               // TODO: only merge variables, not functions/types, etc.
                               // TODO: is there any in-between scope that has not been merged?
                               for (auto &&[name, id] : env.top->defs)
                               {
                                   auto const iter = func_env->defs.find(name);
                                   if (iter != func_env->defs.end())
                                       id = iter->second;
                               }

                               // TODO: from here, go over the function until all nodes you see have a `reachable` tag
                               return out; //
                           });

    mem_state = old_state;
    prune_dead_code(bld, ret);

    decl(); // TODO: maybe call this inside the `block`?
}

inline void parser::var_decl() noexcept
{
    eat(token_kind::KwVar);                   // 'var'
    auto const name = eat(token_kind::Ident); // <ident>
    auto const ty = type();                   // type
    eat(token_kind::Equal);                   // '='
    auto const init = expr();                 // expr
    eat(token_kind::Semicolon);               // ';'

    // TODO: typecheck that rhs matches lhs using `value_type.assign`

    // TODO: codegen a `Store` only on global declarations
    // TODO: link a `Proj` node from the global `State`
    entt::entity const ins[]{init};
    auto const store = bld.make(node_op::Store, ins);
    bld.reg.get_or_emplace<effect>(store).target = mem_state;
    mem_state = store;

    env.new_var(scan.lexeme(name), store);

    // TODO: `prune_dead_code` is common for every declaration, find a way to address this
    // ^ merge `prune_dead_code` and `decl` so you don't jump around; make sure it's a tail call
    prune_dead_code(bld, store);

    decl();
}

inline void parser::decl() noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::KwConst:
        const_decl();
        break;

    case token_kind::KwFunc:
        func_decl();
        break;

    case token_kind::KwVar:
        var_decl();
        break;

    case token_kind::Eof:
        codegen_main();
        break;

    default:
        fail("Expected `func` or <EOF>");
        break;
    }
}

inline void parser::prog() noexcept
{
    // TODO: is this correct?
    scope_visibility vis;
    bld.push_vis<visibility::global>(vis);

    mem_state = bld.make(node_op::Start);

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
    auto const node = bld.make(node_op::Proj, std::span(&mem_state, 1));
    // TODO: figure out the exact type of this
    bld.reg.get<node_type>(node).type = int_const::value(i);

    // TODO: these params should be defined in the function's block, not on global env
    auto const name = scan.lexeme(nametok);
    env.new_var(name, node);

    return ty;
}

inline smallvec parser::expr_list(token_kind term) noexcept
{
    auto parse_arguments = [&](auto &&self, stacklist<entt::entity> *ins, int list_size = 0) -> smallvec
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

inline value_type const *parser::type() noexcept
{
    // TODO: inline CPS this
    switch (scan.peek.kind)
    {
    case token_kind::LeftBracket:
        return array_type();

    case token_kind::Ident:
        return named_type();

    default:
        fail("Expected `[' or <identifier>");
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
    // TODO: inline the check on `eat`
    auto const name = eat(token_kind::Ident); // type

    // TODO: do a full search for the type, not just on `top`
    return env.top->types[scan.lexeme(name)];
}

// codegen helpers

inline void parser::codegen_main() noexcept
{
    auto const main_iter = env.top->defs.find(std::string_view{"main"});
    ensure(main_iter != env.top->defs.end(), "Program does not define a main function!");
    auto const main_node = main_iter->second;
    auto const main_ty = bld.reg.get<node_type const>(main_node).type;
    ensure(main_ty->as<func>(), "Program does not define a main function!");
    auto const main_func = main_ty->as<func>();
    ensure(main_func->ret->as<void_type>(), "Function `main` cannot return a value!");
    ensure(main_func->n_params == 0, "Function `main` cannot have parameters!");

    entt::entity const main_ins[]{main_node};
    auto const call_main = bld.make(node_op::CallStatic, main_ins);
    bld.reg.get_or_emplace<effect>(call_main).target = mem_state; // TODO: this `mem_state` is not the global one, restore global state after every `decl`
    // TODO: is this correct?
    mem_state = call_main;

    // TODO: is this correct?
    (void)bld.make(exit_node{.mem_state = mem_state, .code = 0});
    // TODO: DCE on unused functions

    bld.pop_vis(); // just for correctness
}

// parsing helpers

inline token parser::eat(token_kind kind) noexcept
{
    ensure(scan.peek.kind == kind, "Unexpected token kind");
    return scan.next();
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

inline token parser::eat(std::span<token_kind const> kinds) noexcept
{
    for (auto k : kinds)
        if (scan.peek.kind == k)
            return scan.next();

    fail("Unexpected token kind out of list");
}

inline entt::entity parser::binary_node(token_kind kind, entt::entity lhs, entt::entity rhs) noexcept
{
    using enum token_kind;
    using enum node_op;

    switch (kind)
    {
    case Plus:
        return bld.make(add_node{lhs, rhs});
    case Minus:
        return bld.make(sub_node{lhs, rhs});
    case Star:
        return bld.make(mul_node{lhs, rhs});
    case Slash:
        return bld.make(div_node{lhs, rhs});

    case BangEqual:
        return bld.make(isne_node{lhs, rhs});
    case EqualEqual:
        return bld.make(iseq_node{lhs, rhs});
    case Less:
        return bld.make(islt_node{lhs, rhs});
    case LessEqual:
        return bld.make(isle_node{lhs, rhs});
    case Greater:
        return bld.make(isgt_node{lhs, rhs});
    case GreaterEqual:
        return bld.make(isge_node{lhs, rhs});

    case AndAnd:
        return bld.make(and_node{lhs, rhs});
    case OrOr:
        return bld.make(or_node{lhs, rhs});

    case And:
        return bld.make(bit_and_node{lhs, rhs});
    case Xor:
        return bld.make(bit_xor_node{lhs, rhs});
    case Or:
        return bld.make(bit_or_node{lhs, rhs});

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