
#pragma once

#include "parser/base.hpp"
#include "opt/all.hpp"

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

    if (env.top->table.contains(nametok.hash))
        fail(nametok, "Function already defined", ""); // TODO: say something here

    // TODO: should the function point to the `Start`, `Return`, or where?
    // ^ you can actually pre-define the `Return` node here, attach it to the env table, then set the node's inputs accordingly
    // ^ this way you don't need to specially handle the case where a `return void` function does not have a `return` stmt
    // TODO: should it be memory or ctrl? probably ctrl since after parsing it points to the `Return`
    // TODO: ^ maybe all state nodes, rather than just one
    env.new_value(nametok.hash, bld.state.mem);

    scope s{.prev = env.top};
    env.top = &s;

    // TODO: parse the parameters + return in a separate function, return the node id

    // param_decl,*,?
    int64_t param_i = 0;
    std::vector<::type const *> param_types;

    while (scan.peek.kind != token_kind::RightParen)
    {
        auto const ty = param_decl(param_i++); // param_decl
        param_types.push_back(ty);

        if (scan.peek.kind != token_kind::Comma)
            break;
        scan.next(); // ,
    }

    eat(token_kind::RightParen); // ')'

    auto param_types_list = std::make_unique_for_overwrite<::type const *[]>((size_t)param_i);
    std::copy_n(param_types.data(), param_i, param_types_list.get());

    // type?
    auto const ret_type = (scan.peek.kind != token_kind::LeftBrace)
                              ? type()
                              // TODO: use singleton
                              : new void_type{};

    // HACK: address this
    bld.reg.get<node_type>(bld.state.func).type = func_type{false, ret_type, (size_t)param_i, std::move(param_types_list)}.top();

    // TODO: typecheck that the return type matches what's expected
    // TODO: is this actually the `Return` node?
    auto const ret = block(noscope_t{}, [&](scope const *func_env, entt::entity ret)
                           {
                               rule_sep<false>(); // ';' or <end-of-file>

                               // TODO: handle multi-return case
                               entt::entity const params[]{ret};
                               auto const out = make(bld, return_node{params});

                               // TODO: move this to a function
                               // TODO: handle assignment to constants
                               // TODO: only merge variables, not functions/types, etc.
                               // TODO: codegen loads + stores
                               // TODO: is there any in-between scope that has not been merged?
                               for (auto &&[name, idx] : env.top->table)
                               {
                                   auto const iter = func_env->table.find(name);
                                   if (iter != func_env->table.end())
                                       idx = iter->second;
                               }

                               return out; //
                           });

    bld.state = old_state;
    prune_dead_code(bld, ret);

    // TODO: is this correct?
    bld.pop_vis();

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

    using namespace entt::literals;

    eat(token_kind::At);                      // '@'
    auto const attr = eat(token_kind::Ident); // <ident>
    if ((uint32_t)attr.hash != "extern"_hs)
        fail(attr, "Parsed unknown annotation", ""); // TODO: say something here

    eat(token_kind::KwFunc);               // 'func'
    auto nametok = eat(token_kind::Ident); // ident
    eat(token_kind::LeftParen);            // '('

    scope s{.prev = env.top};
    env.top = &s;

    // TODO: environment pointer should be restored to before parameters, not after
    // ^ also declare the function name before params; the environment should contain it
    // ^ or rather just mark it as unresolved and resolve it later
    // TODO: parse the parameters + return in a separate function, return the node id

    // param_decl,*,?
    int64_t param_i = 0;
    std::vector<::type const *> param_types;

    while (scan.peek.kind != token_kind::RightParen)
    {
        auto const ty = param_decl(param_i++); // param_decl
        param_types.push_back(ty);

        if (scan.peek.kind != token_kind::Comma)
            break;
        scan.next(); // ,
    }

    eat(token_kind::RightParen); // ')'

    auto param_types_list = std::make_unique_for_overwrite<::type const *[]>((size_t)param_i);
    std::copy_n(param_types.data(), param_i, param_types_list.get());

    // type?
    auto const ret_type = (scan.peek.kind != token_kind::Semicolon)
                              ? type()
                              // TODO: singleton
                              : new void_type{};

    rule_sep<false>(); // ';' or <end-of-file>

    env.top = env.top->prev;

    // HACK: address this
    bld.reg.get<node_type>(bld.state.mem).type = func_type{true, ret_type, (size_t)param_i, std::move(param_types_list)}.top();

    // TODO: drop the check either from here or from env
    if (env.top->table.contains(nametok.hash))
        fail(nametok, "Function already defined", ""); // TODO: say something here

    // TODO: should the function point to the `Start`, `Return`, or where?
    // ^ you can actually pre-define the `Return` node here, attach it to the env table, then set the node's inputs accordingly
    // ^ this way you don't need to specially handle the case where a `return void` function does not have a `return` stmt
    // TODO: should it be memory or ctrl? probably ctrl since after parsing it points to the `Return`
    // TODO: ^ maybe all state nodes, rather than just one
    env.new_value(nametok.hash, bld.state.mem);

    bld.state = old_state;

    // TODO: is this correct?
    bld.pop_vis();

    decl();
}

inline void parser::const_decl() noexcept
{
    eat(token_kind::KwConst);                 // 'const'
    auto const name = eat(token_kind::Ident); // <ident>
    eat(token_kind::Equal);                   // '='
    auto const init = expr().node;            // expr
    rule_sep<false>();                        // ';' or <end-of-file>
    // ^ TODO: use AsStmt when enabling const declarations in function body

    // TODO: link a `Proj` node from the global `State`
    // TODO: typecheck in case the type is specified
    // entt::entity const ins[]{init};
    // auto const store = bld.make(node_op::Store, ins);
    // auto &&ty = bld.reg.get<node_type>(store).type;
    // bld.reg.get<node_type>(store).type = new const_type{ty};
    // bld.reg.get_or_emplace<effect>(store).target = mem_state;
    // mem_state = store;

    // TODO: mark the name as non-modifiable somehow
    env.new_value(name.hash, init);

    // TODO: `prune_dead_code` is common for every declaration, find a way to address this
    // ^ merge `prune_dead_code` and `decl` so you don't jump around; make sure it's a tail call
    // NOTE: you still need to prune here to cut temporary nodes
    prune_dead_code(bld, init);

    // TODO: inline CPS this
    decl();
}

template <bool AsStmt>
inline auto parser::var_decl() noexcept -> std::conditional_t<AsStmt, decltype(stmt()), decltype(decl())>
{
    // parsing

    eat(token_kind::KwVar);                   // 'var'
    auto const name = eat(token_kind::Ident); // <ident>
    ::type const *ty = nullptr;
    entt::entity init;

    // untyped_var
    if (scan.peek.kind == token_kind::Equal)
    {
        scan.next();        // '='
        init = expr().node; // expr
        // TODO: initialize ty to typeof(expr)
    }
    else
    {
        ty = type(); // type?

        if (scan.peek.kind == token_kind::Equal)
        {
            // TODO: typecheck in this case using assignability
            scan.next();        // '='
            init = expr().node; // expr
        }
        else // just make the value either way, parsing then fails if an unexpected token follows
        {
            // HACK: handle this better
            if (ty->as<struct_type>() || ty->as<::array_type>())
                init = make(bld, alloca_node{ty});
            else
                init = make(bld, value_node{ty->zero()});
        }
    }

    rule_sep<AsStmt>(); // ';' or <end-of-file>

    // TODO: type check, if present

    // codegen

    env.new_value(name.hash, init);

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
    auto ty = type();   // type
    rule_sep<AsStmt>(); // ';' or <end-of-file>

    // codegen

    // TODO: check type is not already defined
    // TODO: are type names shadowed? if not, search the whole environment
    // TODO: drop the check either from here or from inside env.new_type
    if (auto const n = env.get_name(nametok.hash); n != name_index::missing)
        fail(nametok, "Type is already defined!", ""); // TODO: say something better here

    env.new_type(nametok.hash, ty);

    // NOTE: no need to prune dead code here, everything is done at compile time

    if constexpr (AsStmt)
        return stmt();
    else
        decl();
}

template <bool AsStmt>
inline auto parser::struct_decl(token nametok) noexcept -> std::conditional_t<AsStmt, decltype(stmt()), void>
{
    auto parse_members = [&](auto &&self, stacklist<member_decl> *members, size_t n) -> ::type const *
    {
        switch (scan.peek.kind)
        {
        case token_kind::RightBrace:
        {
            scan.next(); // '}'

            auto mem_ptr = new member_decl[n];
            auto iter = members;
            for (size_t i{}; i < n; ++i)
            {
                mem_ptr[n - i - 1] = iter->value;
                iter = iter->prev;
            }

            return new struct_type{n, std::unique_ptr<member_decl[]>(mem_ptr)};
        }

        case token_kind::Ident:
        {
            auto const mem_name = scan.next(); // <ident>
            auto mem_ty = type();              // type
            eat(token_kind::Semicolon);        // ';'

            auto mem_node = stacklist{
                .value = member_decl{.name = mem_name.hash, .ty = mem_ty},
                .prev = members,
            };
            return self(self, &mem_node, n + 1);
        }

        default:
            fail(scan.peek, "Expected '}' or <identifier>", ""); // TODO: say something better here
            return nullptr;
        }
    };

    // parsing
    eat(token_kind::KwStruct);                          // 'struct'
    eat(token_kind::LeftBrace);                         // '{'
    auto ty = parse_members(parse_members, nullptr, 0); // (member_decl ';')* '}'
    rule_sep<AsStmt>();                                 // ';' or <end-of-file>

    env.new_type(nametok.hash, ty);

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
        return fail(scan.peek, "Expected `@`, `const`, `var`, `type`, `func` or <end-of-file>", ""); // TODO: say something here
    }
}

inline void parser::package() noexcept
{
    // TODO: use the name of the package when exporting
    eat(token_kind::KwPackage);                  // 'package'
    auto const nametok = eat(token_kind::Ident); // <ident>
    eat(token_kind::Semicolon);                  // ';'

    // TODO: is this correct?
    scope_visibility vis;
    bld.push_vis<visibility::global>(vis);

    // TODO: correctly represent this
    // TODO: recheck the node's value
    // TODO: these should be top
    bld.pkg_mem = bld.make(top_type::self(), node_op::Program, {});
    bld.glob_mem = bld.make(top_type::self(), node_op::Global, {});
    // TODO: is this correct?
    bld.state = {
        .func = bld.glob_mem,
        .ctrl = bld.pkg_mem,
        .mem = bld.glob_mem,
    };

    bld.reg.emplace<mem_effect>(bld.glob_mem);
    // TODO: read/write tag

    // TODO: do you need a new scope here?
    // (void)bld.new_func();

    scope global;
    env.top = &global;
    import_builtin(env);

    decl(); // decl*
}

// helper rules

inline ::type const *parser::param_decl(int64_t i) noexcept
{
    // parsing

    auto const nametok = eat(token_kind::Ident); // name
    auto const ty = type();                      // type

    // codegen

    // TODO: recheck this
    auto const node = bld.make(ty->top(), node_op::Proj, {});
    // TODO: recheck this
    bld.reg.emplace<mem_effect>(node) = {
        .prev = bld.state.func,
        .target = bld.state.func,
        .tag = (uint32_t)i, // TODO: uniform integer type
    };
    bld.reg.emplace<mem_read>(node);

    env.new_value(nametok.hash, node);

    return ty;
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
    bld.reg.storage<mem_read>();
    bld.reg.storage<mem_write>();
    bld.reg.storage<region_of_phi>();

    using namespace entt::literals;

    // TODO: most of these checks should be made by `call_static`
    // TODO: do something better for lookup here
    auto const main_iter = env.top->table.find((hashed_name)(uint32_t)"main"_hs);
    ensure(main_iter != env.top->table.end(), "Program does not define a `main` function!");

    auto const main_node = env.values[(uint32_t)main_iter->second];
    auto const main_ty = bld.reg.get<node_type const>(main_node).type;
    ensure(main_ty->as<func>(), "`main` should be a function!");
    // HACK: check the type instead
    ensure(main_ty->as<func>()->ret->as<void_value>(), "`main` cannot return a value!");

    // TODO: should you pass `main_node` to the inputs of the function?
    auto const call_main = make(bld, call_node{.func = main_node});

    // TODO: is this correct?
    auto const exit = make(bld, exit_node{.code = 0});
    // TODO: DCE on unused functions

    bld.pop_vis(); // just for correctness
}