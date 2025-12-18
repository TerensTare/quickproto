
#pragma once

#include "parser/base.hpp"
#include "opt/all.hpp"

// TODO: move the `type` rules to a separate file

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
    env.new_value(name.hash, init);

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

    if (env.top->table.contains(nametok.hash))
        fail(nametok, "Function already defined");

    // TODO: should the function point to the `Start`, `Return`, or where?
    // ^ you can actually pre-define the `Return` node here, attach it to the env table, then set the node's inputs accordingly
    // ^ this way you don't need to specially handle the case where a `return void` function does not have a `return` stmt
    // TODO: should it be memory or ctrl? probably ctrl since after parsing it points to the `Return`
    // TODO: ^ maybe all state nodes, rather than just one
    env.new_value(nametok.hash, bld.state.mem);

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
                               eat(token_kind::Semicolon); // ;

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
        fail(attr, "Parsed unknown annotation");

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

    eat(token_kind::Semicolon); // ';'

    env.top = env.top->prev;

    // HACK: address this
    bld.reg.get<node_type>(bld.state.mem).type = func_type{true, ret_type, (size_t)param_i, std::move(param_types_list)}.top();

    // TODO: drop the check either from here or from env
    if (env.top->table.contains(nametok.hash))
        fail(nametok, "Function already defined");

    // TODO: should the function point to the `Start`, `Return`, or where?
    // ^ you can actually pre-define the `Return` node here, attach it to the env table, then set the node's inputs accordingly
    // ^ this way you don't need to specially handle the case where a `return void` function does not have a `return` stmt
    // TODO: should it be memory or ctrl? probably ctrl since after parsing it points to the `Return`
    // TODO: ^ maybe all state nodes, rather than just one
    env.new_value(nametok.hash, bld.state.mem);

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
    auto ty = type();           // type
    eat(token_kind::Semicolon); // ';'

    // codegen

    // TODO: check type is not already defined
    // TODO: are type names shadowed? if not, search the whole environment
    // TODO: drop the check either from here or from inside env.new_type
    if (auto const n = env.get_name(nametok.hash); is_type(n))
        fail(nametok, "Type is already defined!");

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
            fail(scan.peek, "Expected '}' or <identifier>");
            return nullptr;
        }
    };

    // parsing
    eat(token_kind::KwStruct);                          // 'struct'
    eat(token_kind::LeftBrace);                         // '{'
    auto ty = parse_members(parse_members, nullptr, 0); // (member_decl ';')* '}'
    eat(token_kind::Semicolon);                         // ';'

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
        return fail(scan.peek, "Expected `func` or <EOF>");
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
    bld.pkg_mem = bld.make(bot_type::self(), node_op::Program);
    bld.glob_mem = bld.make(bot_type::self(), node_op::Global);
    bld.reg.emplace<mem_effect>(bld.glob_mem);
    // TODO: read/write tag

    (void)bld.new_func();

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
    // TODO: figure out the exact type of this
    auto const node = bld.make(ty->top(), node_op::Proj);
    // TODO: recheck this
    // TODO: this variable should increment the scope's counter
    bld.reg.emplace<mem_effect>(node) = {
        .target = bld.state.func,
        .tag = (uint32_t)i, // TODO: uniform integer type
    };
    bld.reg.emplace<mem_read>(node);

    env.new_value(nametok.hash, node);

    return ty;
}

inline ::type const *parser::type() noexcept
{
    // TODO: mark the type node if not declared yet
    // TODO: inline CPS this
    switch (scan.peek.kind)
    {
    case token_kind::LeftBracket:
        return array_type();

    case token_kind::Star:
        return pointer_type();

    case token_kind::Ident:
        return named_type();

    default:
        fail(scan.peek, "Expected `[' or <identifier>");
        return nullptr;
    }
}

inline ::type const *parser::array_type() noexcept
{
    // TODO: allow deduced-size arrays for things like initializers and casts

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

inline ::type const *parser::pointer_type() noexcept
{
    eat(token_kind::Star); // '*'
    auto sub = type();     // type

    return new ::pointer_type{sub};
}

inline ::type const *parser::named_type() noexcept
{
    // TODO: inline the check of `eat`
    auto const nametok = eat(token_kind::Ident); // type

    // TODO: make this into a function
    auto const index = env.get_name(nametok.hash);

    // TODO: if the name is not declared yet, this still returns nullptr, is this correct?
    return (uint32_t(index) & uint32_t(name_index::type_mask))
               ? env.get_type(index)
               : nullptr;
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

    // TODO: should you pass `main_node` to the inputs of the function?
    entt::entity const main_ins[]{entt::null};
    auto const call_main = make(bld, call_node{
                                         .func = main_node,
                                         .args = std::span(main_ins, 0),
                                     });
    // HACK: check the type instead
    ensure(bld.reg.get<node_type>(call_main).type->as<void_value>(), "Function `main` cannot return a value!");

    // TODO: is this correct?
    auto const exit = make(bld, exit_node{.code = 0});
    // TODO: DCE on unused functions

    bld.pop_vis(); // just for correctness
}