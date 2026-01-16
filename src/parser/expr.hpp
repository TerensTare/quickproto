
#pragma once

#include "parser/base.hpp"

static constexpr auto prec_table = []()
{
    std::array<parse_prec, 256> table;
    for (size_t i{}; i < std::size(table); ++i)
        table[i] = parse_prec::None;

    table[(uint8_t)token_kind::OrOr] = parse_prec::Or;
    table[(uint8_t)token_kind::AndAnd] = parse_prec::And;
    table[(uint8_t)token_kind::EqualEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::BangEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::Greater] = parse_prec::Comparison;
    table[(uint8_t)token_kind::GreaterEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::Less] = parse_prec::Comparison;
    table[(uint8_t)token_kind::LessEqual] = parse_prec::Comparison;
    table[(uint8_t)token_kind::Plus] = parse_prec::Addition;
    table[(uint8_t)token_kind::Minus] = parse_prec::Addition;
    table[(uint8_t)token_kind::Or] = parse_prec::Addition;
    table[(uint8_t)token_kind::Xor] = parse_prec::Addition;
    table[(uint8_t)token_kind::Star] = parse_prec::Multiplication;
    table[(uint8_t)token_kind::Slash] = parse_prec::Multiplication;
    table[(uint8_t)token_kind::And] = parse_prec::Multiplication;
    table[(uint8_t)token_kind::AndXor] = parse_prec::Multiplication;
    table[(uint8_t)token_kind::Lshift] = parse_prec::Multiplication;
    table[(uint8_t)token_kind::Rshift] = parse_prec::Multiplication;

    return table;
}();

inline expr_info parser::post_expr(expr_info base) noexcept
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

inline expr_info parser::call(expr_info base) noexcept
{
    // parsing

    scan.next(); // '('
    // TODO: add `base` to the parameters list or maybe as a special node
    auto ins = expr_list_term(token_kind::RightParen); // expr,*,?)

    // codegen

    // TODO: is this correct? (consider for example, a call is made inside an `If`)
    auto const call = make(bld, call_node{
                                    .func = base.node,
                                    .args = ins,
                                });
    // TODO: the call should probably modify just the memory node, not the control flow node
    // TODO: the call should probably link to the `Return` node of the called function, not the `Start`

    // TODO: inline CPS this
    return post_expr({
        .node = call,
        .assign = no_name,
    }); // post_expr(parsed)
}

inline expr_info parser::index(expr_info base) noexcept
{
    // parsing

    scan.next(); // '['
    // TODO: add `base` to the parameters list or maybe as a special node
    auto const i = expr().node;    // expr
    eat(token_kind::RightBracket); // ']'

    // codegen

    // TODO: is this correct? (consider mutability, generalizing `Load`, etc.)
    // TODO: error if node is not integer
    auto const node = make(bld, load_node{
                                    .base = base.node,
                                    .offset = bld.reg.get<node_type>(i).type->as<int_value>(),
                                });

    // TODO: inline CPS this
    return post_expr({
        .node = node,
        .assign = base.assign,
    }); // post_expr(parsed)
}

inline expr_info parser::member(expr_info base) noexcept
{
    // parsing

    auto const dot = scan.next(); // '.'
    // TODO: add `base` to the parameters list or maybe as a special node
    auto const mem = eat(token_kind::Ident); // <ident>

    // codegen

    // HACK: handle this better
    // TODO: handle pointers as well
    auto rawval = bld.reg.get<node_type>(base.node).type;
    auto baseval = rawval->as<struct_value>();
    // TODO: this is not a syntax error
    if (!baseval)
        fail(dot, "Cannot access member of type!", ""); // TODO: say something better here

    auto ty = baseval->type;

    int32_t offset = -1;
    for (size_t i{}; i < ty->n_members; ++i)
    {
        if (ty->members[i].name == mem.hash)
        {
            offset = i;
            break;
        }
    }

    // TODO: this is not a syntax error
    if (offset == -1)
        fail(mem, "Member not present in type", ""); // TODO: say something better here

    // TODO: find a better way to represent `Load`s at known indices
    // TODO: `int_lit` is int64 but `offset` is int32
    auto const offset_node = make(bld, value_node{int_value::make(offset)});

    // TODO: is this correct? (consider mutability, generalizing `Load`, etc.)
    // TODO: calculate the offset of the member and pass it to the load nodes
    // TODO: error if the node is not integer
    auto const node = make(bld, load_node{
                                    .base = base.node,
                                    .offset = bld.reg.get<node_type>(offset_node).type->as<int_value>(),
                                });

    // TODO: inline CPS this
    return post_expr({
        .node = node,
        .assign = base.assign,
    }); // post_expr(parsed)
}

// TODO: recheck the rule to parse `&` and `*` rhs at correct precedence
inline expr_info parser::primary() noexcept
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
        Rune,
        String,
        KwFalse,
        KwTrue,
        KwNil,
        LeftParen, // (expr)
        Bang,
        Minus,
        Xor,
        And,
        Star, // unary
    };

    auto const tok = eat(primary_tokens);

    // codegen

    switch (tok.kind)
    {
    case Integer:
    {
        auto const txt = scan.lexeme(tok);

        uint64_t val{};
        // TODO: handle integer base here
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        // HACK: figure out actual int size (8/16/32/64)
        return {
            .node = make(bld, value_node{int_value::make(val)}),
            // integers are not assignable to
            .assign = no_name,
        };
    }

    case Decimal:
    {
        auto const txt = scan.lexeme(tok);

        double val{};
        // TODO: handle decimal base here
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        // HACK: figure out actual decimal size (32/64)
        return {
            .node = make(bld, value_node{new float64{val}}),
            // decimals are not assignable to
            .assign = no_name,
        };
    }

    case Rune:
    {
        // TODO: decode/unquote the rune
        auto const txt = scan.lexeme(tok);
        // TODO: do you normalize the rune here or later?

        return {
            .node = make(bld, value_node{new rune_value{txt[1]}}), // skip the opening `'`
            // runes are not assignable to
            .assign = no_name,
        };
    }

    case String:
    {
        // TODO: throw the string in the pool, but decode/unquote first
        auto const txt = scan.lexeme(tok);
        // TODO: do you normalize the string here or later?
        // ^ when normalizing, use cow strings for efficiency

        return {
            .node = make(bld, value_node{new string_value}),
            // strings are not assignable to
            .assign = no_name,
        };
    }

    case KwFalse:
        return {
            .node = make(bld, value_node{bool_const::False()}),
            // you cannot assign to `false`
            .assign = no_name,
        };
    case KwTrue:
        return {
            .node = make(bld, value_node{bool_const::True()}),
            // you cannot assign to `true`
            .assign = no_name,
        };

    // TODO: don't use integer type here anymore
    case KwNil:
        return {
            .node = make(bld, value_node{nil_value::self()}),
            // you cannot assign to `nil`
            .assign = no_name,
        };

    // post_expr( '(' expr ')' )
    case LeftParen:
    {
        auto const ret = expr(); // expr
        eat(RightParen);         // ')'
        // TODO: is this ok or should you wrap it? (assign, address)
        return post_expr(ret);
    }

    // '!' expr
    case Bang:
    {
        auto const sub = expr().node;
        return {
            .node = make(bld, not_node{.sub = sub}),
            // you cannot assign to `!expr`
            .assign = no_name,
        };
    }

    // '-' expr
    case Minus:
    {
        auto const sub = expr().node;
        return {
            .node = make(bld, neg_node{.sub = sub}),
            // you cannot assign to `-expr`
            .assign = no_name,
        };
    }

    // '^' expr
    case Xor:
    {
        auto const sub = expr().node;
        return {
            .node = make(bld, compl_node{.sub = sub}),
            // you cannot assign to `^expr`
            .assign = no_name,
        };
    }

    // '*' expr
    case Star:
    {
        auto const sub = expr();
        return {
            .node = make(bld, deref_node{.sub = sub.node}),
            // TODO: is this correct?
            .assign = sub.assign,
        };
    }

    // '&' expr
    case And:
    {
        auto const sub = expr().node;
        return {
            .node = make(bld, addr_node{.sub = sub}),
            // TODO: is this correct?
            .assign = no_name,
        };
    }

    default:
        std::unreachable();
        return {
            .node = entt::null,
            .assign = no_name,
        };
    }
}

inline expr_info parser::expr(parse_prec prec) noexcept
{
    auto lhs = primary();

    parse_prec rprec;
    while (prec < (rprec = prec_table[(uint8_t)scan.peek.kind]))
    {
        auto const op = scan.next().kind;
        auto const rhs = expr(rprec);

        lhs = {
            .node = binary_node(op, lhs.node, rhs.node),
            // you cannot assign to a binary expression
            .assign = no_name,
        };
    }

    return lhs;
}

inline smallvec<entt::entity> parser::expr_list_term(token_kind term) noexcept
{
    // TODO: do you ever need the assign/address info for the parsed nodes?

    auto parse_arguments = [&](this auto &&self, stacklist<entt::entity> *ins, int list_size = 0)
        -> smallvec<entt::entity>
    {
        // TODO: is the control flow correct here?
        if (scan.peek.kind != term)
        {
            auto const arg = expr().node; // expr
            stacklist node{.value = arg, .prev = ins};
            ins = &node;
            ++list_size;

            if (scan.peek.kind == token_kind::Comma)
            {
                scan.next(); // ','
                return self(ins, list_size);
            }
        }

        eat(term); // term
        return compress(ins, list_size);
    };

    return parse_arguments(nullptr, 0);
}

inline smallvec<entt::entity> parser::expr_list() noexcept
{
    // TODO: do you ever need the assign/address info for the parsed nodes?
    auto const first = expr().node; // expr
    stacklist head{.value = first};

    auto parse_arguments = [&](this auto &&self, stacklist<entt::entity> *ins, int list_size = 0)
        -> smallvec<entt::entity>
    {
        // TODO: is the control flow correct here?
        if (scan.peek.kind == token_kind::Comma)
        {
            scan.next();                  // ','
            auto const arg = expr().node; // expr
            stacklist node{.value = arg, .prev = ins};

            return self(&node, list_size + 1);
        }

        return compress(ins, list_size);
    };

    return parse_arguments(&head, 1);
}

inline entt::entity parser::binary_node(token_kind kind, entt::entity lhs, entt::entity rhs) noexcept
{
    using enum token_kind;

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

    case AndXor:
        // HACK: do you need to make this into a separate node?
        return make(bld, and_xor_node{lhs, rhs});

    case And:
        return make(bld, bit_and_node{lhs, rhs});
    case Xor:
        return make(bld, bit_xor_node{lhs, rhs});
    case Or:
        return make(bld, bit_or_node{lhs, rhs});

    case Lshift:
        return make(bld, shift_left_node{lhs, rhs});
    case Rshift:
        return make(bld, shift_right_node{lhs, rhs});

    default:
        std::unreachable();
        return entt::null;
    }
}

inline expr_info parser::type_expr_or_ident() noexcept
{
    ::type const *ty = nullptr;

    // HACK: do something better
    if (scan.peek.kind == token_kind::Ident)
    {
        auto const nametok = scan.next();
        auto const index = env.get_name(nametok.hash);

        if (is_type(index))
            ty = env.get_type(index);
        else if (index != name_index::missing)
            return post_expr({
                // TODO: is this correct?
                .node = env.values[(uint32_t)index],
                .assign = nametok.hash, // TODO: is this correct?
            });
        // TODO: address this
        else
        {
            fail(nametok, "Using a name before declaration is not supported yet", ""); // TODO: say something better here
            return {
                .node = entt::null,
                .assign = no_name,
            };
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
        // parsing

        scan.next();                  // '('
        auto const sub = expr().node; // expr
        eat(token_kind::RightParen);  // ')'

        // codegen

        auto const cast = make(bld, cast_node{.target = ty, .base = sub});
        return post_expr({
            .node = cast,
            // you cannot assign to cast expressions, but maybe to the resulting sub-expression (can you?)
            .assign = no_name,
        });
    }

    // init
    case token_kind::LeftBrace:
    {
        // parsing

        auto const lbrace = scan.next();                       // '{'
        auto members = expr_list_term(token_kind::RightBrace); // '}'

        // codegen

        auto cty = ty->as<composite_type>();
        if (!cty)
            fail(lbrace, "Type cannot be brace-initialized", ""); // TODO: say something better here

        auto const init = make(bld, composite_value_node{.ty = cty, .init = std::move(members)});

        return post_expr({
            .node = init, // TODO: is this correct here?
            // you cannot assign to type literal expressions, but maybe to the resulting sub-expression (can you?)
            .assign = no_name,
        });
    }

    default:
        fail(scan.peek, "Expected '(` or `{` after type", ""); // TODO: say something better here
        return {
            .node = entt::null,
            .assign = no_name,
        };
    }
}