
#pragma once

#include <algorithm>
#include <charconv>

#include "builder.hpp"
#include "env.hpp"
#include "scanner.hpp"

#include "pass/pass_registry.hpp"

// TODO:
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
// - variable mapping should be block-specific
// - drop recursion from expr and stmt rules
// - apparently you don't need `Store` nodes most of the time; you only need them for nodes where it has an effect on
// ^ (which is most of the time just class/global variables; for pointer function params you can just change the variable value from the calling function)
// ^ but you still need to take care of loads of non-const variables (eg. `a = 10; b += a; a = 20`)
// - implement codegen for comparison (<, <=, ==, !=, >, >=)
// - merge `terminal_stmt` at call site
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
    // '{' stmt* '}'
    [[nodiscard]]
    inline scope *block() noexcept;
    // local_decl | comp_assign | post_op
    inline void simple_stmt() noexcept;

    // stmt ::= block
    //        | if_stmt
    //        | for_stmt
    //        | return_stmt
    //        | break_stmt
    //        | continue_stmt
    //        | simple_stmt
    inline void stmt() noexcept;

    // decl

    // 'func' <ident> '(' param_decl,*, ')' block
    inline void func_decl() noexcept;
    // func_decl | type_decl
    inline void decl() noexcept;
    // decl*
    inline void prog() noexcept;

    scanner scan;
    builder bld;
    pass_registry passes{.bld = bld};

    scope global{.types = global_types()};
    env env{.top = &global};

    entt::entity mem_state;

private:
    // helpers
    // <ident> <ident> (name type)
    // i is the index of the parameter in the function declaration (used by the Proj node emitted for the parameter)
    inline value_type const *param_decl(int64_t i) noexcept;

    inline void merge(scope &parent, scope const &lhs, scope const &rhs) noexcept;

    // merge `lhs` and `rhs` into a new phi node if either of the nodes is different from `old` and return it, otherwise return `old`
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

        return bld.make(node_op::Phi, ins);
    }

    inline token eat(token_kind kind) noexcept;
    inline token eat(std::span<token_kind const> kinds) noexcept;

    inline static entt::dense_map<std::string_view, value_type const *, dual_hash, dual_cmp> global_types() noexcept;
};

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

        return env.get_var(name);
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
    auto const name = scan.lexeme(ids[0]);
    env.new_var(name, rhs);
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

    auto opnode = bld.make(node_op, env.get_var(name), rhs);
    opnode = passes.run(opnode);

    env.set_var(name, opnode);
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
        env.get_var(name),
        bld.makeval(mem_state, node_op::Const, new int_const{1}) //
    );
    opnode = passes.run(opnode);

    env.set_var(name, opnode);
}

inline void parser::if_stmt() noexcept
{
    // TODO:
    // if both the `if` and `else` branch have no `return` stmt, then evaluate everything else and return the remaining body's return as a return value

    // parsing
    eat(token_kind::KwIf); // 'if'
    auto cond = expr();    // expr

    auto old_state = mem_state;

    // TODO: is this mem_state or $ctrl?
    auto if_node = bld.make(node_op::If, cond);
    bld.reg.emplace<effect>(if_node, mem_state);
    mem_state = if_node;

    auto then = bld.makeval(if_node, node_op::Proj, new int_const{0});
    // TODO: remove this when enabling on `makeval`
    bld.reg.emplace<effect>(then, if_node);
    // mem_state = then;

    auto else_ = bld.makeval(if_node, node_op::Proj, new int_const{1});
    // TODO: remove this when enabling on `makeval`
    bld.reg.emplace<effect>(else_, if_node);
    // mem_state = else_;

    // TODO: does this part belong here?
    // TODO: only add one region at the end; when the whole `if-else` tree is parsed
    // TODO: even if there is just an `if` branch, the `else` is implicit on both `Region` and `Phi`, you just need to figure out its value on both cases
    // TODO: merge returns into a single node per function; use Phi nodes to merge return values into one
    entt::entity region_children[]{then, else_};
    // TODO: the nodes should be effect nodes, not input nodes
    auto const region = bld.make(node_op::Region, region_children);
    // TODO:
    // - spawn a Region node here that links to the if/else branches or just the `If`
    // ^ also take care to handle multiple if-else case
    // - spawn a Phi node for merging values

    mem_state = region;

    auto parent = env.top;

    auto then_env = block();

    scope *else_env = nullptr;
    entt::entity else_ret = entt::null; // TODO: is this correct?

    if (scan.peek.kind == token_kind::KwElse)
    {
        scan.next(); // 'else'

        ensure(
            (scan.peek.kind == token_kind::KwIf || scan.peek.kind == token_kind::LeftBrace),
            "Expected `if` or `{` after `else`!" //
        );

        // TODO: get the return of each branch and merge them too
        (scan.peek.kind == token_kind::KwIf)
            // TODO: the `if` should be evaluated inside the `else` environment; handle that
            ? if_stmt()                 // if_stmt
            : void(else_env = block()); // | block
    }

    // codegen
    merge(*parent, *then_env, *else_env);
    env.top = parent;

    // TODO: merge the returns
    // return phi(entt::null, ret, else_ret); //
}

inline void parser::for_stmt() noexcept
{
    // parsing
    eat(token_kind::KwFor); // 'for'

    auto cond = expr();
    (void)block();

    // codegen

    // TODO: implement
}

inline void parser::control_flow(token_kind which) noexcept
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

inline scope *parser::block() noexcept
{
    // TODO: figure out merging with parent
    // TODO: do not heap allocate these when switching to CPS
    auto block_env = new scope{.parent = env.top};
    env.top = block_env;

    eat(token_kind::LeftBrace); // {
    while (scan.peek.kind != token_kind::RightBrace)
        stmt();
    eat(token_kind::RightBrace);

    env.top = env.top->parent;
    return block_env;
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
        return fail("Expected one of the following: `:=`, `++`, `--`, `+=`, `-=`, `*=`, `/=`");
        // TODO: return an "unreachable type" (top I guess?)
    }
}

inline void parser::stmt() noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::LeftBrace:
        return (void)block(); // TODO: also merge env with parent

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

    // TODO: parse return type and specify here
    bld.reg.get<node_type>(mem_state).type = new func{void_type::self(), (size_t)param_i, std::move(param_types_list)};

    auto name = scan.lexeme(nametok);
    ensure(!env.top->funcs.contains(name), "Function already defined");

    // TODO: pass the type here
    env.top->funcs.insert({name, mem_state});

    // TODO: merge with global env
    (void)block();
}

inline void parser::decl() noexcept
{
    switch (scan.peek.kind)
    {
    case token_kind::KwFunc:
        func_decl();
        break;
        // TODO: default: error
    }
}

inline void parser::prog() noexcept
{
    while (scan.peek.kind != token_kind::Eof)
        decl(); // decl*

    // TODO: codegen a node that passes control flow to initializing globals and then `main`
}

// helper rules

inline value_type const *parser::param_decl(int64_t i) noexcept
{
    // parsing

    auto nametok = eat(token_kind::Ident); // name
    auto type = eat(token_kind::Ident);    // type

    // codegen

    // TODO: recheck this
    auto node = bld.make(node_op::Proj, mem_state);
    bld.reg.get<node_type>(node).type = new int_const{i};

    auto name = scan.lexeme(nametok);
    env.new_var(name, node);

    return env.top->types[scan.lexeme(type)];
}

// helpers

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