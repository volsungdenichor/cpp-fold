#pragma once

#include <type_traits>

namespace cpp_fold
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

template <class State, class Func>
struct folder_t
{
    State state;
    Func func;

    template <class Range>
    auto operator()(Range&& range) const -> State
    {
        auto current = step_t<State>{ state };
        for (auto it = std::begin(range); it != std::end(range); ++it)
        {
            current = std::invoke(func, std::move(current).get(), *it);
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

struct fold_fn
{
    template <class State, class Func>
    constexpr auto operator()(State state, Func func) const -> folder_t<State, Func>
    {
        return { std::move(state), std::move(func) };
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
    constexpr auto operator()(Pred pred) const -> folder_t<bool, impl_t<Pred>>
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
    constexpr auto operator()(Out out) const -> folder_t<Out, impl_t>
    {
        return { std::move(out), impl_t{} };
    }
};

}  // namespace detail

constexpr inline auto fold = detail::fold_fn{};

constexpr inline auto all_of = detail::logical_sum_fn<std::logical_and<>, true, true>{};
constexpr inline auto any_of = detail::logical_sum_fn<std::logical_or<>, false, true>{};
constexpr inline auto none_of = detail::logical_sum_fn<std::logical_and<>, true, false>{};

constexpr inline auto copy = detail::copy_fn{};

}  // namespace cpp_fold
