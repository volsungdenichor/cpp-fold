#pragma once

#include <functional>
#include <type_traits>

namespace cpp_reduce
{

enum class step_result_type
{
    should_continue,
    should_break,
};

template <class T>
struct step_t
{
    T m_value;
    step_result_type m_type;

    template <class U, class = std::enable_if_t<std::is_constructible_v<T, U>>>
    step_t(U&& value, step_result_type type = step_result_type::should_continue) : m_value(std::move(value))
                                                                                 , m_type(type)
    {
    }

    constexpr step_result_type type() noexcept
    {
        return m_type;
    }

    constexpr const T& get() const& noexcept
    {
        return m_value;
    }

    constexpr T&& get() && noexcept
    {
        return std::move(m_value);
    }
};

template <class T>
constexpr auto break_with(T&& item) -> step_t<std::decay_t<T>>
{
    return { std::forward<T>(item), step_result_type::should_break };
}

template <class T, class Pred>
constexpr auto break_with_if(T&& item, Pred&& pred) -> step_t<std::decay_t<T>>
{
    const auto type = std::invoke(pred, item) ? step_result_type::should_break : step_result_type::should_continue;
    return { std::forward<T>(item), type };
}

template <class T>
constexpr auto continue_with(T&& item) -> step_t<std::decay_t<T>>
{
    return { std::forward<T>(item), step_result_type::should_continue };
}

template <class State, class Reducer>
struct reducer_proxy_t
{
    State state;
    Reducer reducer;

    template <class Range>
    auto operator()(Range&& range) const -> State
    {
        auto current = step_t<State>{ state };
        for (auto it = std::begin(range); it != std::end(range); ++it)
        {
            current = std::invoke(reducer, std::move(current).get(), *it);
            if (current.type() == step_result_type::should_break)
            {
                break;
            }
        }
        return current.get();
    }
};

namespace detail
{

struct reduce_fn
{
    template <class State, class Reducer>
    constexpr auto operator()(State state, Reducer reducer) const -> reducer_proxy_t<State, Reducer>
    {
        return { std::move(state), std::move(reducer) };
    }
};

template <class Op, bool Init, bool Value, bool Stop = !Init>
struct logical_sum_fn
{
    template <class Pred>
    struct impl_t
    {
        Pred pred;

        template <class... Args>
        auto operator()(bool total, Args&&... args) const -> step_t<bool>
        {
            static const auto op = Op{};
            return break_with_if(
                op(total, std::invoke(pred, std::forward<Args>(args)...) == Value), [](bool v) { return v == Stop; });
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred pred) const -> reducer_proxy_t<bool, impl_t<Pred>>
    {
        return { Init, impl_t<Pred>{ std::move(pred) } };
    }
};

struct copy_fn
{
    struct impl_t
    {
        template <class Out, class T>
        auto operator()(Out out, T&& item) const -> Out
        {
            *out = std::forward<T>(item);
            ++out;
            return out;
        }
    };

    template <class Out>
    constexpr auto operator()(Out out) const -> reducer_proxy_t<Out, impl_t>
    {
        return { std::move(out), impl_t{} };
    }
};

template <class Impl>
struct transducer_interface_t
{
    Impl m_impl;

    template <class Reducer>
    constexpr auto operator()(Reducer&& reducer) const -> std::invoke_result_t<Impl, Reducer>
    {
        return std::invoke(m_impl, std::forward<Reducer>(reducer));
    }

    template <class Reducer>
    constexpr auto operator>>=(Reducer&& reducer) const -> std::invoke_result_t<Impl, Reducer>
    {
        return std::invoke(m_impl, std::forward<Reducer>(reducer));
    }
};

struct transform_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;

        template <class State, class... Args>
        constexpr auto operator()(State state, Args&&... args) const -> step_t<State>
        {
            return m_next_reducer(std::move(state), std::invoke(m_func, std::forward<Args>(args)...));
        }
    };

    template <class Func>
    struct transducer_t
    {
        Func m_func;

        template <class State, class Reducer>
        constexpr auto operator()(reducer_proxy_t<State, Reducer> next) const
            -> reducer_proxy_t<State, reducer_t<Reducer, Func>>
        {
            return { std::move(next.state), reducer_t<Reducer, Func>{ std::move(next.reducer), m_func } };
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> transducer_interface_t<transducer_t<std::decay_t<Func>>>
    {
        return { { std::forward<Func>(func) } };
    }
};

struct filter_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        template <class State, class... Args>
        constexpr auto operator()(State state, Args&&... args) const -> step_t<State>
        {
            if (std::invoke(m_pred, args...))
            {
                return m_next_reducer(std::move(state), std::forward<Args>(args)...);
            }
            return state;
        }
    };

    template <class Pred>
    struct transducer_t
    {
        Pred m_pred;

        template <class State, class Reducer>
        constexpr auto operator()(reducer_proxy_t<State, Reducer> next) const
            -> reducer_proxy_t<State, reducer_t<Reducer, Pred>>
        {
            return { std::move(next.state), reducer_t<Reducer, Pred>{ std::move(next.reducer), m_pred } };
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> transducer_interface_t<transducer_t<std::decay_t<Pred>>>
    {
        return { { std::forward<Pred>(pred) } };
    }
};

}  // namespace detail

constexpr inline auto reduce = detail::reduce_fn{};

constexpr inline auto all_of = detail::logical_sum_fn<std::logical_and<>, true, true>{};
constexpr inline auto any_of = detail::logical_sum_fn<std::logical_or<>, false, true>{};
constexpr inline auto none_of = detail::logical_sum_fn<std::logical_and<>, true, false>{};

constexpr inline auto copy = detail::copy_fn{};

constexpr inline auto transform = detail::transform_fn{};
constexpr inline auto filter = detail::filter_fn{};

}  // namespace cpp_reduce
