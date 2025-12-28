
#pragma once

#include <charconv>
#include "parser/base.hpp"

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
        fail(scan.peek, "Expected `*`, `[' or <identifier>", ""); // TODO: say something better here
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
    // TODO: inline this check
    eat(token_kind::Star); // '*'
    auto sub = type();     // type

    return new ::pointer_type{sub};
}

inline ::type const *parser::named_type() noexcept
{
    // TODO: inline the check of `eat`
    auto const nametok = eat(token_kind::Ident); // type

    auto const index = env.get_name(nametok.hash);

    // TODO: if the name is not declared yet, this still returns nullptr, is this correct?
    return (uint32_t(index) & uint32_t(name_index::type_mask))
               ? env.get_type(index)
               : nullptr;
}
