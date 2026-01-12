
#pragma once

#include "types/value.hpp"

#define GENERATE_COVARIANT_OP(name)                                        \
    inline value const *name(value const *rhs) const noexcept final        \
    {                                                                      \
        if constexpr (requires { self().name((Derived const *)nullptr); }) \
        {                                                                  \
            if (auto r = rhs->as<Derived>())                               \
                return self().name(r);                                     \
        }                                                                  \
                                                                           \
        return value::name(rhs);                                           \
    }

#define GENERATE_BINARY_JUMP_TABLE(ret, Self, name, op)             \
    constexpr ret const *Self::name(Self const *rhs) const noexcept \
    {                                                               \
        switch (level * 3 + rhs->level)                             \
        {                                                           \
        case 0: /* (top, top) */                                    \
        case 1: /* (top, const) */                                  \
        case 3: /* (const, top) */                                  \
            return Self::top();                                     \
                                                                    \
        case 4:                                                     \
            return op(this, rhs);                                   \
                                                                    \
        case 2: /* (top, bot) */                                    \
        case 5: /* (const, bot) */                                  \
        case 6: /* (bot, top) */                                    \
        case 7: /* (bot, const) */                                  \
        case 8: /* (bot, bot) */                                    \
            return Self::bot();                                     \
                                                                    \
        default:                                                    \
            std::unreachable();                                     \
        }                                                           \
    }

// TODO: is it best if all virtual functions are here and the child classes have regular methods?
template <typename Derived>
struct covariant_helper : value
{
    // TODO: more operators
    inline value const *assign(value const *rhs) const noexcept final
    {
        // TODO: recheck this
        // TODO: it's probably best to have these =0 in base and give the errors from here
        return rhs->as<Derived>()
                   ? rhs
                   : value::assign(rhs);
    }

    GENERATE_COVARIANT_OP(add);
    GENERATE_COVARIANT_OP(sub);
    GENERATE_COVARIANT_OP(mul);
    GENERATE_COVARIANT_OP(div);

    GENERATE_COVARIANT_OP(band);
    GENERATE_COVARIANT_OP(bor);
    GENERATE_COVARIANT_OP(bxor);
    GENERATE_COVARIANT_OP(lsh);
    GENERATE_COVARIANT_OP(rsh);

    GENERATE_COVARIANT_OP(eq);
    // GENERATE_COVARIANT_OP(ne);
    GENERATE_COVARIANT_OP(lt);
    // GENERATE_COVARIANT_OP(le);
    // GENERATE_COVARIANT_OP(gt);
    // GENERATE_COVARIANT_OP(ge);

    GENERATE_COVARIANT_OP(phi);

private:
    inline Derived const &self() const noexcept
    {
        static_assert(std::derived_from<Derived, covariant_helper>);
        return *static_cast<Derived const *>(this);
    }
};

#undef GENERATE_COVARIANT_OP
