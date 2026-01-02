
#pragma once

#include <algorithm>

#include "nodegen/all.hpp"
#include "env.hpp"
#include "scanner.hpp"

#include "types/all.hpp"
#include "utils/function_ref.hpp"

// TODO:
// - drop `Proj` nodes; instead keep track of each member's state and use `Load`/`Store` nodes
// - generate `Store`s when creating an array
// - enable multiple declarations and multiple assignments once you fully figure them out
// - `then` should probably accept a `stacklist` for return instead; this solves the multi-return problem + marking return as `entt::null`
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
// ^ ie. `value::narrow(value const *sub) { if sub out of this return out-of-bounds-value; else return sub; }
// - codegen a `Load`/`Store` for global variables, as these operations need to be "lazy"
// ^ (maybe) it's best to codegen `Load`/`Store` for everything, then cut the nodes for local stuff?
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

static constexpr hashed_name no_name = hashed_name{(entt::id_type)entt::hashed_string::value("<no-name>")};

struct expr_info final
{
    entt::entity node;
    hashed_name assign = no_name;
    // ^ The `hashed_name` for the base `value` of the expression if assignable; `no_name` otherwise
};

struct parser final
{
    using block_then = function_ref<entt::entity(scope const *, entt::entity)>;

    // expr

    // post_expr(base) ::= call(base) | index(base) | member(base) | base
    // ^ie. it's `base` + any possible rule following it
    [[nodiscard]]
    inline expr_info post_expr(expr_info base) noexcept;

    // call(base) ::= post_expr( base '(' expr,*,? ')' )
    // ^ meaning this rule will parse a call, then invoke `post_expr` with the parsed call as `base`
    [[nodiscard]]
    inline expr_info call(expr_info base) noexcept;

    // index(base) ::= post_expr( base '[' expr ']' )
    // ^ meaning this rule will parse an index, then invoke `post_expr` with the parsed index as `base`
    [[nodiscard]]
    inline expr_info index(expr_info base) noexcept;

    // TODO: add support for methods
    // member(base) ::= post_expr( base '.' <ident> )
    // ^ meaning this rule will parse a member, then invoke `post_expr` with the parsed index as `base`
    [[nodiscard]]
    inline expr_info member(expr_info base) noexcept;

    // primary ::= 'true'
    //           | 'false'
    //           | <integer>
    //           | <decimal>
    //           | <rune>
    //           | <string>
    //           | <unary_op> expr
    //           | post_expr( '(' expr ')' )
    //           | post_expr( type_expr_or_ident )
    // <unary_op> ::= '!' | '-' | '*' | '&'
    // ^ this means that the `post_expr` recursion will apply IFF <ident> is followed by `(`, `[` or `.`
    [[nodiscard]]
    inline expr_info primary() noexcept;

    // expr ::= primary ( <binary_op> expr(<binary_op-prec>) )*
    // <binary_op> ::= '||' | '&&' | '==' | '!=' | '<' | '<=' | '>' | '>=' | '+' | '-' | '*' | '/' | '&' | '^' | '|'
    // ^ how much is parsed by a specific call to `expr` depends on the precedence level parsed
    [[nodiscard]]
    inline expr_info expr(parse_prec prec = parse_prec::None) noexcept;

    // stmt
    // - the entity returned is the node of the `return` statement in this block, if any; `null` otherwise
    // ^ if a `continue` or `break` statement is reached before a `return` (not in a nested block), but on this block, the return is also `null` as the statement is unreachable
    // - note that any of these functions will fully parse the remaining of the containing block for performance reasons
    // - also note that these rules don't return an `expr_info` as we don't care about the extra information on it, we only need the resulting node

    // <ident>,+ ':=' expr,+ ';' stmt
    [[nodiscard]]
    inline entt::entity local_decl(token lhs) noexcept;
    // expr '=' expr ';' stmt
    // HACK: can you merge this with `compound_assign`?
    [[nodiscard]]
    inline entt::entity assign(expr_info lhs) noexcept;

    // expr comp_op expr ';' stmt
    // comp_op ::= '+=' | '-=' | '*=' | '/=' | '&=' | '^=' | '|='
    [[nodiscard]]
    inline entt::entity compound_assign(expr_info lhs) noexcept;
    // expr post_op ';' stmt
    // post_op ::= '++' | '--'
    [[nodiscard]]
    inline entt::entity post_op(expr_info lhs) noexcept;

    // 'defer' call ';' stmt
    [[nodiscard]]
    inline entt::entity defer_stmt() noexcept;

    // 'if' expr block ('else' ( if_stmt | block ))? ';' stmt
    [[nodiscard]]
    inline entt::entity if_stmt() noexcept;

    // for_range_stmt | for_cond_stmt
    // or 'for' ( ( 'range' expr ) | expr ) block stmt
    [[nodiscard]]
    inline entt::entity for_stmt() noexcept;

    // 'for' 'range' expr block stmt
    [[nodiscard]]
    inline entt::entity for_range_stmt() noexcept;
    // 'for' expr block stmt
    [[nodiscard]]
    inline entt::entity for_cond_stmt() noexcept;
    // 'break' <ident>? ';' stmt
    [[nodiscard]]
    inline entt::entity break_stmt() noexcept;
    // 'continue' <ident>? ';' stmt
    [[nodiscard]]
    inline entt::entity continue_stmt() noexcept;
    // 'return' expr,* ';' stmt
    [[nodiscard]]
    inline entt::entity return_stmt() noexcept;
    // '{' stmt
    // ^ no `}` as that is handled by the `stmt` recursion
    [[nodiscard]]
    inline entt::entity block(block_then then) noexcept;

    struct noscope_t final
    {
    };
    // HACK: do something better
    // used to handle function parsing without creating a lot of scopes
    // '{' stmt
    // ^ no `}` as that is handled by the `stmt` recursion
    [[nodiscard]]
    inline entt::entity block(noscope_t, block_then then) noexcept;

    // local_decl | assign | compound_assign | post_op
    // TODO: local_decl is disabled for now. Please use `var` syntax instead
    [[nodiscard]]
    inline entt::entity simple_stmt() noexcept;

    // stmt ::= '}'
    //        | type_decl
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

    // 'func' <ident> '(' param_decl,*, ')' type? block ';' decl
    inline void inline_func_decl() noexcept;
    // '@' 'extern' 'func' <ident> '(' param_decl,*, ')' type? ';' decl
    // ^ external function declaration; they do not have a body
    // HACK: handle `extern` just like other annotations
    inline void extern_func_decl() noexcept;

    // 'const' <ident> '=' expr ';' decl
    // TODO: make this a statement too
    inline void const_decl() noexcept;
    // ( typed_var | untyped_var ) ';' stmt-or-decl
    // typed_var   ::= 'var' <ident> type ( '=' expr )?
    // untyped_var ::= 'var' <ident> type? '=' expr
    // - ie. you have to either specify the type and omit the initializer or specify the initializer and omit the type, or you can specify both
    // - in case you omit the initializer, the value is zero-init
    // - if parsed inside a block, this is treated as a statement and is followed by a `stmt` rather than a decl
    template <bool AsStmt>
    inline auto var_decl() noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;
    // struct_decl | alias_decl
    // if parsed inside a block, this is treated as a statement and is followed by a `stmt` rather than a decl
    template <bool AsStmt>
    inline auto type_decl() noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;
    // 'type' <ident> type ';' stmt-or-decl
    // if parsed inside a block, this is treated as a statement and is followed by a `stmt` rather than a decl
    template <bool AsStmt>
    inline auto alias_decl(token nametok) noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;
    // 'type' <ident> 'struct' '{' (member_decl ';')* '}' ';' stmt-or-decl
    // member_decl ::= <ident> type
    // TODO: is `member_decl` and `param_decl` the same rule?
    template <bool AsStmt>
    inline auto struct_decl(token nametok) noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>;

    // const_decl | func_decl | var_decl | type_decl
    // func_decl ::= extern_func_decl | inline_func_decl
    inline void decl() noexcept;
    // 'package' <ident> ';' decl
    inline void package() noexcept;

    scanner scan;
    builder bld;
    env env;

    stacklist<entt::entity> *defer_stack = nullptr;

private:
    // helpers

    // expr

    // type '{' expr,*,? '}'
    // | type '(' expr ')'
    // | ident
    [[nodiscard]]
    inline expr_info type_expr_or_ident() noexcept;

    // expr,*,? term
    inline smallvec<entt::entity> expr_list_term(token_kind term) noexcept;

    // expr (,expr)*
    // ^ ie. there cannot be a trailing comma and the terminator can be anything except something that starts an expression
    // TODO: should this be able to parse 0 expressions?
    inline smallvec<entt::entity> expr_list() noexcept;

    // stmt/decl

    // eat the separator token of the rule
    // - if the rule is a stmt rule, this is equivalent to eat(Semicolon)
    // - if the rule is a decl rule, this either eats a Semicolon, does nothing on Eof, or errors otherwise
    template <bool AsStmt>
    inline void rule_sep() noexcept
    {
        if constexpr (AsStmt)
            return (void)eat(token_kind::Semicolon);
        else
        {
            switch (scan.peek.kind)
            {
            case token_kind::Semicolon:
                scan.next();
                return;

            case token_kind::Eof:
                return;

            default:
                fail(scan.peek, "Expected `;` or <end-of-file>", ""); // TODO: say something better here
                return;
            }
        }
    }

    // type

    // <ident> type
    // i is the index of the parameter in the function declaration (used by the Proj node emitted for the parameter)
    inline ::type const *param_decl(int64_t i) noexcept;

    // array_type | pointer_type | named_type
    inline ::type const *type() noexcept;

    // '[' <integer> ']' type
    inline ::type const *array_type() noexcept;

    // '*' type
    inline ::type const *pointer_type() noexcept;

    // <ident>
    inline ::type const *named_type() noexcept;

    // '(' param_decl,*,? ')'
    // ^ used by `func`-related rules to parse the list of parameters
    inline smallvec<::type const *> param_list() noexcept;

    // other helpers

    inline void merge(scope &parent, scope const &lhs, scope const &rhs) noexcept;

    // merge `lhs` and `rhs` into a new phi node if either of the nodes is different from `old` and return it, otherwise return `old`
    // TODO: if no `else` branch, `rhs` should be `old`
    // TODO: move this on the nodegen API
    inline entt::entity phi(entt::entity old, entt::entity lhs, entt::entity rhs) noexcept;

    // codegen the `Start` and `Exit` nodes of the program, pass the control flow to `main` (and initializing globals when added).
    inline void codegen_main() noexcept;

    // parsing helpers

    // TODO: eventually pass a span here instead of token
    // TODO: can you pass format_string + args here?
    // TODO: can the context ever have UTF8 contents?
    // TODO: eventually make `msg` be an error enum, and pass the concrete information on `ctx`
    [[noreturn]]
    inline void fail(token const &t, std::string_view msg, std::string_view ctx) const;

    inline token eat(token_kind kind) noexcept;
    inline token eat(std::span<token_kind const> kinds) noexcept;

    inline entt::entity binary_node(token_kind kind, entt::entity lhs, entt::entity rhs) noexcept;

    inline static void import_builtin(::env &e) noexcept;
};

inline void parser::merge(scope &parent, scope const &lhs, scope const &rhs) noexcept
{
    // TODO: pass the `Region` id here, don't assume or deduce it
    // invariant: `lhs` and `rhs` are branches of `parent` and never `null`

    // TODO: hash the `key` for faster lookup

    // TODO: only merge variables, nothing else
    for (auto &&[key, index] : parent.table)
        if (is_value(index))
        {
            // NOTE: `lhs` and `rhs` have every key from `parent` and a node is needed only if there is a change in either lhs or rhs
            // ^ this means you can search until you reach the beginning of `parent` for a variable and ignore everything before
            // TODO: handle case where `else` is missing
            auto const left = lhs.get_name(key);
            auto const right = rhs.get_name(key);

            // TODO: recheck this
            // TODO: is it that if a value is not modified in a branch, it will point to `old`?
            auto &&val = env.values[(uint32_t)index];
            auto &&lval = env.values[(uint32_t)left];
            auto &&rval = env.values[(uint32_t)right];

            auto const node = phi(val, lval, rval);
            val = node;
        }
}

inline entt::entity parser::phi(entt::entity old, entt::entity lhs, entt::entity rhs) noexcept
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
    // HACK: generalize the value
    auto const ret = bld.make(int_top::self(), node_op::Phi, ins);
    (void)bld.reg.emplace<region_of_phi>(ret, old); // TODO: specify the actual region here and run passes

    return ret;
}

inline void parser::fail(token const &t, std::string_view msg, std::string_view ctx) const
{
    // TODO: use uchars here
    std::string_view txt{(char const *)scan.text, t.start};
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

    // TODO: do tab expansion rather than tab trimming
    auto const lcode = [&]
    {
        auto const str = std::string_view{(char const *)scan.text + start, t.start - start};
        auto const n = str.size();

        size_t tabs{};
        while (tabs < n && str[tabs] == '\t')
            tabs++;

        return str.substr(tabs);
    }();

    auto const ecode = scan.lexeme(t);
    auto const rcode = std::string_view{(char const *)scan.text + t.start + t.len, end - (t.start + t.len)};

    auto ok_part = std::format("{} | {}", line_num, lcode);

#define console_red "\033[31m"
#define console_reset "\033[0m"

    // TODO: handle unicode characters case when showing the arrow (nr-chars(unicode) != nr-bytes)
    std::println(
        "{}:{}:{}: {}. {}.\n\n{}" console_red "{}" console_reset "{}\n{:>{}}^--",
        // TODO: use actual file name
        "<source>", line_num, t.start - start,
        msg, ctx, ok_part, ecode, rcode,
        "", ok_part.size() //
    );

#undef console_red
#undef console_reset

    // TODO: do something cross-platform
    __debugbreak();
    std::exit(-1);
}

inline token parser::eat(token_kind kind) noexcept
{
    if (scan.peek.kind == kind)
        return scan.next();

    // TODO: unified error interface
    if (scan.peek.kind >= token_kind::Unknown)
        fail(scan.peek, "Bad token", token_to_error[size_t(scan.peek.kind) - size_t(token_kind::Unknown)]);
    else
        fail(scan.peek, "Unexpected token", ""); // TODO: say something here
}

inline token parser::eat(std::span<token_kind const> kinds) noexcept
{
    // TODO: optimize this based on array size (maybe pass span size and let the compiler do its thing?)
    for (auto k : kinds)
        if (scan.peek.kind == k)
            return scan.next();

    // TODO: unified error interface
    if (scan.peek.kind >= token_kind::Unknown)
        fail(scan.peek, "Bad token", token_to_error[size_t(scan.peek.kind) - size_t(token_kind::Unknown)]);
    else
        fail(scan.peek, "Unexpected token", ""); // TODO: say something here
}

inline void parser::import_builtin(::env &e) noexcept
{
    // TODO: eventually this can be replaced for an implicit `import "builtin"`

    // global->next_alias = {.parent = parent_memory::Global};

    // defs
    // TODO: add built-in functions, such as `print`

    using namespace entt::literals;

    // types

    // TODO: use a singleton here for now
    e.new_type((hashed_name)(uint32_t)"bool"_hs, new bool_type);
    e.new_type((hashed_name)(uint32_t)"int"_hs, new sint_type);
    e.new_type((hashed_name)(uint32_t)"float64"_hs, new float64_type);
    e.new_type((hashed_name)(uint32_t)"rune"_hs, new rune_type);
    e.new_type((hashed_name)(uint32_t)"string"_hs, new string_type);
    // TODO: more built-in types
}