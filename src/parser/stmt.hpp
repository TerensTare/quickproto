
#pragma once

#include "parser/base.hpp"

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
    env.new_value(lhs.hash, rhs);

    return stmt();
}

inline entt::entity parser::assign(entt::entity lhs) noexcept
{
    // parsing
    eat(token_kind::Equal); // '='

    // auto const name = str.intern(id);
    // auto const lhs = env.get_var(name);

    // TODO: handle multi-expression case
    auto const rhs = expr();    // expr
    eat(token_kind::Semicolon); // ';'

    // codegen

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

    // auto const name = str.intern(id);
    // auto const lhs = env.get_var(name);

    // TODO: handle multi-expression case
    auto const rhs = expr();    // expr
    eat(token_kind::Semicolon); // ';'

    // codegen

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

    // auto const name = str.intern(id);
    // auto const lhs = env.get_var(name);
    auto const rhs = make(bld, value_node{int_const::make(1)});

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
        // TODO: add read/write here
        bld.state.mem = defer_stack->value;

        bld.reg.get<ctrl_effect>(defer_stack->value).target = bld.state.ctrl;
        bld.state.ctrl = defer_stack->value;

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
