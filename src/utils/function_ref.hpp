
#pragma once

#include <type_traits>

template <typename>
struct function_ref;

template <typename R, typename... Args>
struct function_ref<R(Args...)> final
{
    template <typename Func>
        requires(std::is_invocable_r_v<R, Func, Args...> //
                 and !std::is_same_v<std::remove_cvref_t<Func>, function_ref>)
    inline function_ref(Func &&fn)
        : func{&fn}, impl{&call_impl<Func>}
    {
    }

    inline R operator()(Args... args) const
    {
        return impl(func, static_cast<Args &&>(args)...);
    }

private:
    template <typename Func>
    static R call_impl(void *func, Args... args)
    {
        return (*static_cast<Func *>(func))(static_cast<Args &&>(args)...);
    }

    void *func;
    R (*impl)(void *, Args...);
};