
#pragma once

#include "parser/base.hpp"

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
    // TODO: error if node is not integer
    auto const node = make(bld, load_node{
                                    .base = base,
                                    .offset = bld.reg.get<node_type>(i).type->as<int_>(),
                                });

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

    int32_t offset = -1;
    for (size_t i{}; i < ty->n_members; ++i)
    {
        if (ty->members[i].name == mem.hash)
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
    // TODO: error if the node is not integer
    auto const node = make(bld, load_node{
                                    .base = base,
                                    .offset = bld.reg.get<node_type>(offset_node).type->as<int_>(),
                                });

    // TODO: how do you make sure it happens before any possible store, but still optimize it out if only load?

    // TODO: inline CPS this
    return post_expr(node); // post_expr(parsed)
}

inline entt::entity parser::primary() noexcept
{
    // TODO: CPS the `expr` calls here, repeat for `call`, `index` and `post_expr`, etc.
    using enum token_kind;

    // HACK: do something nicer/cleaner
    switch (scan.peek.kind)
    {
    case token_kind::LeftBracket:
    case token_kind::Ident:
        return type_expr_or_ident();
    }

    // parsing

    token_kind const primary_tokens[]{
        // literal
        Integer,
        Decimal,
        KwFalse,
        KwTrue,
        KwNil,
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

    // post_expr( '(' expr ')' )
    case LeftParen:
    {
        auto const ret = expr(); // expr
        eat(RightParen);         // ')'
        // TODO: is this ok or should you wrap it?
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
        auto const rhs = expr(rprec);

        lhs = binary_node(op, lhs, rhs);
    }

    return lhs;
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

inline entt::entity parser::type_expr_or_ident() noexcept
{
    value_type const *ty = nullptr;

    // HACK: do something better
    if (scan.peek.kind == token_kind::Ident)
    {
        auto const nametok = scan.next();
        auto const index = env.get_name(nametok.hash);

        if (is_type(index))
            ty = env.type(index);
        else if (index != name_index::missing)
            return post_expr(env.values[(uint32_t)index]);
        // TODO: address this
        else
        {
            fail(nametok, "Using a name before declaration is not supported yet");
            return entt::null;
        }
    }
    else
    {
        ty = type();
    }

    switch (scan.peek.kind)
    {
    // cast
    case token_kind::LeftParen:
    {
        scan.next();                 // '('
        auto const sub = expr();     // expr
        eat(token_kind::RightParen); // ')'

        auto const cast = make(bld, cast_node{.target = ty, .base = sub});
        return post_expr(cast);
    }

    // init
    case token_kind::LeftBrace:
    {
        scan.next();                                           // '{'
        auto members = expr_list_term(token_kind::RightBrace); // '}'

        // TODO: construct the object (emit the Stores)
        auto const init = make(bld, alloca_node{.ty = ty});
        return post_expr(init);
    }

    default:
        fail(scan.peek, "Expected '(` or `{` after type");
        return entt::null;
    }
}