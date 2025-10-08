
#pragma once

#include <algorithm>
#include <charconv>

#include "builder.hpp"
#include "env.hpp"
#include "scanner.hpp"

#include "pass/pass_registry.hpp"

// TODO:
// - do you need a `memory` edge for operations that affect memory state? (then `effect` becomes `happens_after`)
// - have lookup tables that map operator to node type, depending on operands (eg. `Iadd` or `Fadd` for `+`)
// - (maybe) return expr type when parsing for cache friendliness?
// - when encountering a return, you should probably set the memory state to the `return` node
// - the returned values (on multi-return case) outlive the `then` call inside `block`, so you can just pass them as a span
// ^ also have local storage for return params that is reused, just so you don't reallocate every function call
// - make `then` be a `function_ref`
// - have a `then` function on every rule to do stuff like clean up variables from env, etc.
// - inline all the CPS calls
// - eventually transition to CPS for stmt/decl rules
// ^ benefits:
//   - more optimized assembly generated
//   - easier way to express block semantics + merging, with less dynamic allocations
//   - easier way to propagate return values from `return` statements to the function's exit point
//   - easier way to detect unreachable code (anything after `continue`/`break`/`return` until block finishes is unreachable)
// ^ cons:
//   - parsing rules are more complex now and maybe more error prone?
//   - how do you separate shadowed variable from non-shadowed ones? (you need to know when merging)
//
// - apparently you don't need `Store` nodes most of the time; you only need them for nodes where it has an effect on
// ^ (which is most of the time just class/global variables; for pointer function params you can just change the variable value from the calling function)
// ^ but you still need to take care of loads of non-const variables (eg. `a = 10; b += a; a = 20`)
// - implement codegen for comparison (<, <=, ==, !=, >, >=)
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

// - store `return value;` in a `$return` variable and handle it at the end of a block (take special care when there are statements after `return`)
// ^ careful that `$return` shouldn't always exist, only inside `block`s; so maybe you just return it from `block` instead?

// - lemmas: in a correct program:
// ^ a package is followed by an import or Eof
// ^ an import is followed by an import or Eof
// ^ a declaration is followed by a declaration or an Eof
// ^ a statement is followed by a statement or a `}`
// ---
// ^ so you can make `stmt` be: `if('{') { next(); stmt(); } else if ('}') { next(); } else ...
// ^ and when you parse a function, you call `block` instead

enum class parse_prec : uint8_t
{
    Stop, // non-expr token, when hit the parser will stop parsing an expression and move on to the next rule
    None,
    Or,         // or
    And,        // and
    Equality,   // == !=
    Comparison, // < <= >= >
    Term,       // + -
    Factor,     // * /
};

static constexpr auto prec_table = []()
{
    std::array<parse_prec, 256> table;
    // MSVC does not like a for loop here
    std::fill_n(table.data(), 256, parse_prec::Stop);

    table[(uint8_t)token_kind::EqualEqual] = parse_prec::Equality;
    table[(uint8_t)token_kind::BangEqual] = parse_prec::Equality;
    table[(uint8_t)token_kind::Greater] = parse_prec::Comparison;
    table[(uint8_t)token_kind::GreaterEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::Less] = parse_prec::Comparison;
    table[(uint8_t)token_kind::LessEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::Plus] = parse_prec::Term;
    table[(uint8_t)token_kind::Minus] = parse_prec::Term;
    table[(uint8_t)token_kind::Star] = parse_prec::Factor;
    table[(uint8_t)token_kind::Slash] = parse_prec::Factor;

    return table;
}();

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

    // TODO: parse call expression in case an `Ident` is found
    // ^ still you need to find a way to parse things such as `a.b(c)` or similar
    // parse any calls by using `rule ::= ( '(' expr,* ')' )*`, assuming an `ident` was encountered right before
    inline entt::entity call_or_ident(entt::entity base) noexcept;
    inline entt::entity primary() noexcept;

    inline entt::entity expr(parse_prec prec = parse_prec::None) noexcept;

    // stmt
    // - the entity returned is the node of the `return` statement in this block, if any; `null` otherwise
    // ^ if a `continue` or `break` statement is reached before a `return` (not in a nested block), but on this block, the return is also `null` as the statement is unreachable
    // - also note that any of these functions will fully parse the remaining of the containing block for performance reasons

    // <ident>,+ ':=' expr,+ ';' stmt
    [[nodiscard]]
    inline entt::entity local_decl(std::span<token const> ids) noexcept;
    // expr comp_op expr ';' stmt
    // comp_op ::= '+=' | '-=' | '*=' | '/='
    [[nodiscard]]
    inline entt::entity comp_assign(std::span<token const> ids) noexcept;
    // expr post_op ';' stmt
    // post_op ::= '++' | '--'
    [[nodiscard]]
    inline entt::entity post_op(std::span<token const> ids) noexcept;

    // 'if' expr block ('else' ( if_stmt | block ))? stmt
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
    // local_decl | comp_assign | post_op
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
    inline entt::entity stmt() noexcept;

    // decl

    // 'func' <ident> '(' param_decl,*, ')' type? block decl
    inline void func_decl() noexcept;
    // func_decl | type_decl
    inline void decl() noexcept;
    // decl
    inline void prog() noexcept;

    scanner scan;
    builder bld;
    pass_registry passes{bld};

    scope global{.types = global_types()};
    env env{.top = &global};

    entt::entity mem_state;

private:
    // helpers
    // <ident> <ident> (name type)
    // i is the index of the parameter in the function declaration (used by the Proj node emitted for the parameter)
    inline value_type const *param_decl(int64_t i) noexcept;

    // type

    inline value_type const *type() noexcept;

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

    // `phi`, but used for `return` nodes
    // TODO: return one entity per expr in return
    inline entt::entity return_phi(entt::entity region, entt::entity lhs, entt::entity rhs) noexcept
    {
        // TODO: handle case when either branch is null
        // TODO: handle multiple returns
        // TODO: ensure number of expr matches on both
        entt::entity const ins[]{region, lhs, rhs};
        auto const ret = bld.make(node_op::Phi, ins);
        return passes.run(ret);
    }

    // merge `lhs` and `rhs` into a new phi node if either of the nodes is different from `old` and return it, otherwise return `old`
    // TODO: if no `else` branch, `rhs` should be `old`
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
        return ret; // TODO: link the region here and run passes
    }

    // codegen the `Start` and `Exit` nodes of the program, pass the control flow to `main` (and initializing globals when added).
    inline void codegen_main() noexcept;

    // parsing helpers

    inline token eat(token_kind kind) noexcept;
    inline token eat(std::span<token_kind const> kinds) noexcept;

    inline static entt::dense_map<std::string_view, value_type const *, dual_hash, dual_cmp> global_types() noexcept;
};

inline entt::entity parser::call_or_ident(entt::entity base) noexcept
{
    while (scan.peek.kind == token_kind::LeftParen)
    {
        scan.next(); // '('

        // TODO: parse multiple arguments
        auto const arg = expr();

        eat(token_kind::RightParen);

        // TODO: ensure `base` is callable

        // TODO: is this correct? (consider for example, a call is made inside an `If`)
        entt::entity const ins[]{mem_state, arg, base};
        auto const call = bld.make(node_op::CallStatic, ins);

        // TODO: is this correct?
        mem_state = call;
        base = call;
    }

    return base;
}

inline entt::entity parser::primary() noexcept
{
    using enum token_kind;

    // parsing

    token_kind const primary_tokens[]{
        Integer,
        KwFalse,
        KwTrue,
        KwNil,     // literal
        Ident,     // variable/function/type/const/whatever
        LeftParen, // (expr)
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

        int64_t val{};
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        return bld.makeval(mem_state, node_op::Const, new int_const{val});
    }

    case KwFalse:
        return bld.makeval(mem_state, node_op::Const, new bool_const{false});
    case KwTrue:
        return bld.makeval(mem_state, node_op::Const, new bool_const{true});

    // TODO: don't use integer type here anymore
    case KwNil:
        return bld.makeval(mem_state, node_op::Const, new int_const{0});

    case Ident:
    {
        auto const name = scan.lexeme(tok);
        // TODO: recheck this after figuring out addressing
        // TODO: figure out the type of the node
        // auto const addr = bld.makeval(mem_state, op::Addr, int_bot::self(), {.i64 = 0});

        // TODO: in case of calls, this should get the function node
        auto const val = env.get_var(name);
        return call_or_ident(val);
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

    parse_prec rprec;
    while (prec < (rprec = prec_table[(uint8_t)scan.peek.kind]))
    {
        auto const op = scan.next().kind;
        // TODO: if right-associative op, increase precedence of rprec by 1
        // TODO: do you even have right-associative operators?
        auto const rhs = expr(rprec);

        entt::entity const inputs[]{lhs, rhs};
        lhs = bld.make(op_node(op), inputs);
        lhs = passes.run(lhs);
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

inline entt::entity parser::comp_assign(std::span<token const> ids) noexcept
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

    entt::entity const ins[]{env.get_var(name), rhs};
    auto opnode = bld.make(node_op, ins);
    opnode = passes.run(opnode);

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
    auto const node_op = optok == token_kind::PlusPlus
                             ? node_op::Add
                             : node_op::Sub;

    // TODO: do NOT use the lexeme here, use the address of the result
    auto const name = scan.lexeme(ids[0]);

    // TODO: use an actual address here
    // TODO: figure out the type of the node
    // auto const res = bld.makeval(mem_state, op::Addr, int_bot::self());

    entt::entity const ins[]{
        env.get_var(name),
        bld.makeval(mem_state, node_op::Const, new int_const{1}),
    };

    auto opnode = bld.make(node_op, ins);
    opnode = passes.run(opnode);

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

    // TODO: return should be the merged node of return from either branch or return of the statement after the `if`
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
                                       mem_state = region(then_state, mem_state);

                                       // TODO: all this is common on both branches
                                       // TODO: is this correct? (from here to return)
                                       // codegen
                                       merge(*env.top, *then_env, *else_env);

                                       // TODO: if either `if` or `else` has a return, codegen a phi node and return it
                                       // TODO: is this correct?
                                       // TODO: `phi` should merge the children of `return` nodes
                                       auto const merge_ret = return_phi(mem_state, then_ret, else_ret);

                                       // TODO: parse after merging nodes
                                       // TODO: this should be yet another branch, marked as `if(false)` ie. `~ctrl`
                                       auto const rest_ret = stmt(); // rest of block statements

                                       return (merge_ret != entt::null) ? merge_ret : rest_ret; //
                                   });
            }
            else
            {
                auto const region = this->region(then_state, mem_state);

                // TODO: implement
                // TODO: also merge `then_env` with `env.top` in this case
                auto const rest_ret = stmt(); // trailing stmt

                // TODO: is this correct?
                return return_phi(region, then_ret, rest_ret);
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
    }

    eat(token_kind::Semicolon); // ;

    // codegen

    // TODO: implement
    // TODO: check that this is inside a `for`/`switch`/etc.

    // TODO: unreachable code
    (void)stmt();
    return entt::null;
}

inline entt::entity parser::return_stmt() noexcept
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
    // NOTE: everything after `return` is unreachable code, so address that with a warning or something
    (void)stmt();

    return outs[0]; // TODO: return all values
}

inline entt::entity parser::block(auto &&then) noexcept
{
    // TODO: figure out merging with parent
    scope block_env{.parent = env.top};
    env.top = &block_env;

    eat(token_kind::LeftBrace); // {
    auto ret = stmt();          // stmt*}

    env.top = env.top->parent;
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
        return comp_assign(ids);

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
                         auto left = stmt();

                         // TODO: also merge env with parent

                         return ret != entt::null ? ret : left; //
                     });

    // TODO: break on `RightBrace`
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

inline void parser::func_decl() noexcept
{
    // some setup codegen before parsing
    // NOTE: this needs to be here because parameters depend on `mem_state`
    // TODO: rollback this in case of errors during parsing
    // TODO: `Start` is a value node; address that
    // TODO: `Start` should have a `$ctrl` child and `arg`, which are separate; address that
    mem_state = bld.make(node_op::Start);

    // parsing

    eat(token_kind::KwFunc);               // 'func'
    auto nametok = eat(token_kind::Ident); // ident
    eat(token_kind::LeftParen);            // '('

    // TODO: environment pointer should be restored to before parameters, not after
    // ^ also declare the function name before params; the environment should contain it
    // TODO: parse the parameters + return in a separate function, return the node id

    // param_decl,*
    int64_t param_i = 0;
    std::vector<value_type const *> param_types;
    if (scan.peek.kind != token_kind::RightParen)
    {
        auto ty = param_decl(param_i++); // param_decl
        param_types.push_back(ty);

        // (',' param_decl)*
        while (scan.peek.kind == token_kind::Comma)
        {
            scan.next();                // ,
            ty = param_decl(param_i++); // param_decl
            param_types.push_back(ty);
        }
    }

    eat(token_kind::RightParen); // ')'

    auto param_types_list = std::make_unique_for_overwrite<value_type const *[]>((size_t)param_i);
    std::copy_n(param_types.data(), param_i, param_types_list.get());

    auto const ret_type = (scan.peek.kind != token_kind::LeftBrace)
                              ? type()
                              : void_type::self();

    bld.reg.get<node_type>(mem_state).type = new func{ret_type, (size_t)param_i, std::move(param_types_list)};

    auto name = scan.lexeme(nametok);
    ensure(!env.top->funcs.contains(name), "Function already defined");

    env.top->funcs.insert({name, mem_state});

    // TODO: typecheck that the return type matches what's expected
    (void)block([&](scope const *func_env, entt::entity ret)
                {
                    // TODO: handle multi-return case
                    entt::entity const params[]{ret};
                    auto const out = bld.make(node_op::Return, std::span(params, ret != entt::null));
                    // TODO: recheck this
                    (void)bld.reg.get_or_emplace<effect>(out, mem_state);

                    // TODO: move this to a function
                    // TODO: handle assignment to constants
                    for (auto &&[name, id] : env.top->vars)
                    {
                        auto const iter = func_env->vars.find(name);
                        if (iter != func_env->vars.end())
                        {
                            id = iter->second;
                        }
                    }

                    // TODO: from here, go over the function until all nodes you see have a `reachable` tag
                    return out; //
                });

    // TODO: find a better way to do this (eg. the last declaration might not be a func decl)
    prune_dead_code(bld);

    decl(); // TODO: maybe call this inside the `block`?
}

inline void parser::decl() noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::KwFunc:
        func_decl();
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
    decl(); // decl*
}

// helper rules

inline value_type const *parser::param_decl(int64_t i) noexcept
{
    // parsing

    auto nametok = eat(token_kind::Ident); // name
    auto ty = type();                      // type

    // codegen

    // TODO: recheck this
    auto const node = bld.make(node_op::Proj, std::span(&mem_state, 1));
    bld.reg.get<node_type>(node).type = new int_const{i};

    auto name = scan.lexeme(nametok);
    env.new_var(name, node);

    return ty;
}

inline value_type const *parser::type() noexcept
{
    // TODO: parse qualifiers, etc.
    auto name = eat(token_kind::Ident); // type

    // TODO: do a full search for the type, not just on `top`
    return env.top->types[scan.lexeme(name)];
}

// codegen helpers

inline void parser::codegen_main() noexcept
{
    // TODO: implement this as a call to `main`, it should take care of checking that the function exists, type signature, memory state, etc.
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

    for (auto &&[key, value] : parent.vars)
    {
        // NOTE: `lhs` and `rhs` have every key from `parent` and a node is needed only if there is a change in either lhs or rhs
        // ^ this means you can search until you reach the beginning of `parent` for a variable and ignore everything before
        // TODO: handle case where `else` is missing
        auto const left = lhs.vars.contains(key) ? lhs.vars.at(key) : entt::null;
        auto const right = rhs.vars.contains(key) ? rhs.vars.at(key) : entt::null;

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

inline entt::dense_map<std::string_view, value_type const *, dual_hash, dual_cmp> parser::global_types() noexcept
{
    entt::dense_map<std::string_view, value_type const *, dual_hash, dual_cmp> ret;
    ret.insert({"int", int_bot::self()});
    return ret;
}