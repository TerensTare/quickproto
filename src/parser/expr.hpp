
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
        .assign = assign_index::none,
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
    auto rawval = bld.reg.get<node_type>(base.node).type;
    auto baseval = rawval->as<struct_value>();
    if (!baseval)
        fail(dot, "Cannot access member of type!");

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

    // TODO: find a better way to represent `Load`s at known indices
    // TODO: `int_const` is int64 but `offset` is int32
    auto const offset_node = make(bld, value_node{int_const::make(offset)});

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
        KwFalse,
        KwTrue,
        KwNil,
        LeftParen, // (expr)
        Bang,
        Minus,
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
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        // HACK: figure out actual int size
        return {
            .node = make(bld, value_node{int_const::make(val)}),
            // integers are not assignable to
            .assign = assign_index::none,
        };
    }

    case Decimal:
    {
        auto const txt = scan.lexeme(tok);

        double val{};
        std::from_chars(txt.data(), txt.data() + txt.size(), val);
        // HACK: figure out actual int size
        return {
            .node = make(bld, value_node{new float64{val}}),
            // decimals are not assignable to
            .assign = assign_index::none,
        };
    }

    case KwFalse:
        return {
            .node = make(bld, value_node{bool_const::False()}),
            // you cannot assign to `false`
            .assign = assign_index::none,
        };
    case KwTrue:
        return {
            .node = make(bld, value_node{bool_const::True()}),
            // you cannot assign to `true`
            .assign = assign_index::none,
        };

    // TODO: don't use integer type here anymore
    case KwNil:
        return {
            .node = make(bld, value_node{nil_value::self()}),
            // you cannot assign to `nil`
            .assign = assign_index::none,
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
            .assign = assign_index::none,
        };
    }

    // '-' expr
    case Minus:
    {
        auto const sub = expr().node;
        return {
            .node = make(bld, neg_node{.sub = sub}),
            // you cannot assign to `-expr`
            .assign = assign_index::none,
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
            .assign = assign_index::none,
        };
    }

    default:
        std::unreachable();
        return {
            .node = entt::null,
            .assign = assign_index::none,
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
            .assign = assign_index::none,
        };
    }

    return lhs;
}

inline smallvec parser::expr_list_term(token_kind term) noexcept
{
    // TODO: do you ever need the assign/address info for the parsed nodes?

    auto parse_arguments = [&](auto &&self, stacklist<entt::entity> *ins, int list_size = 0)
        -> smallvec
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
    // TODO: do you ever need the assign/address info for the parsed nodes?
    auto const first = expr().node; // expr
    stacklist head{.value = first};

    auto parse_arguments = [&](auto &&self, stacklist<entt::entity> *ins, int list_size = 0)
        -> smallvec
    {
        // TODO: is the control flow correct here?
        if (scan.peek.kind == token_kind::Comma)
        {
            scan.next();                  // ','
            auto const arg = expr().node; // expr
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
                .node = env.values[(uint32_t)index],
                .assign = assign_index{(uint32_t)index}, // TODO: is this correct?
            });
        // TODO: address this
        else
        {
            fail(nametok, "Using a name before declaration is not supported yet");
            return {
                .node = entt::null,
                .assign = assign_index::none,
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
        scan.next();                  // '('
        auto const sub = expr().node; // expr
        eat(token_kind::RightParen);  // ')'

        auto const cast = make(bld, cast_node{.target = ty, .base = sub});
        return post_expr({
            .node = cast,
            // you cannot assign to cast expressions, but maybe to the resulting sub-expression (can you?)
            .assign = assign_index::none,
        });
    }

    // init
    case token_kind::LeftBrace:
    {
        // parsing

        auto lbrace = scan.next();                             // '{'
        auto members = expr_list_term(token_kind::RightBrace); // '}'

        // codegen

        // HACK: do something better here
        if (!ty->as<struct_type>() && !ty->as<::array_type>())
            fail(lbrace, "Type cannot be brace-initialized");

        // TODO: construct the object (emit the Stores)
        auto const init = make(bld, alloca_node{.ty = ty});
        auto ret = init;

        // TODO: if arg_members==0, do a full Zero (memset)

        size_t i{};
        for (; i < members.n; ++i)
        {
            auto set = make(bld, store_node{
                                     .lhs = init,
                                     .offset = int_const::make((int64_t)i),
                                     .rhs = members[i],
                                 });
            ret = set;
        }

        // HACK: implement this better
        if (auto sty = ty->as<struct_type>())
        {
            auto const n_ty_members = sty->n_members;

            for (; i < n_ty_members; ++i)
            {
                auto set = make(bld, store_node{
                                         .lhs = init,
                                         .offset = int_const::make((int64_t)i),
                                         .rhs = make(bld, value_node{sty->members[i].ty->zero()}),
                                     });
                ret = set;
            }
        }
        else if (auto aty = ty->as<::array_type>())
        {
            auto const n_ty_members = ty->as<::array_type>()->n;
            // optimizations: do not make the zero-value node unless there is a member to initialize
            if (i < n_ty_members)
            {
                // optimization: all array members have the same type, hence the same zero-value
                auto zero = make(bld, value_node{aty->base->zero()});

                for (; i < n_ty_members; ++i)
                {
                    auto set = make(bld, store_node{
                                             .lhs = init,
                                             .offset = int_const::make((int64_t)i),
                                             .rhs = zero,
                                         });
                    ret = set;
                }
            }
        }
        else
        {
            std::unreachable();
        }

        return post_expr({
            .node = init, // TODO: is this correct here?
            // you cannot assign to type literal expressions, but maybe to the resulting sub-expression (can you?)
            .assign = assign_index::none,
        });
    }

    default:
        fail(scan.peek, "Expected '(` or `{` after type");
        return {
            .node = entt::null,
            .assign = assign_index::none,
        };
    }
}