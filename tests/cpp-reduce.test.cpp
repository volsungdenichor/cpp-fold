#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <cpp-reduce/cpp-reduce.hpp>
#include <sstream>

constexpr inline struct str_fn
{
    template <class... Args>
    std::string operator()(Args&&... args) const
    {
        std::stringstream ss;
        (ss << ... << std::forward<Args>(args));
        return ss.str();
    }
} str;

TEST_CASE("simple reduce invocation", "")
{
    const std::vector<int> input = { 2, 3, 5, 10 };
    REQUIRE(cpp_reduce::reduce(0, std::plus<>{})(input) == 20);
}

TEST_CASE("all_of - regular implementation", "")
{
    const auto reducer = cpp_reduce::reduce(true, [](bool res, int value) -> bool {  //
        return res && (10 <= value && value <= 20);
    });

    REQUIRE(reducer(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(reducer(std::vector<int>{}) == true);
    REQUIRE(reducer(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("all_of - short circuit implementation", "")
{
    const auto reducer = cpp_reduce::reduce(
        true,
        [](bool res, int value) -> cpp_reduce::step_t<bool>
        {  //
            return cpp_reduce::break_with_if(res && (10 <= value && value <= 20), [](bool v) { return !v; });
        });

    REQUIRE(reducer(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(reducer(std::vector<int>{}) == true);
    REQUIRE(reducer(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("all_of - built-in implementation", "")
{
    const auto reducer = cpp_reduce::all_of([](int value) { return 10 <= value && value <= 20; });

    REQUIRE(reducer(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(reducer(std::vector<int>{}) == true);
    REQUIRE(reducer(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("any_of - built-in implementation", "")
{
    const auto reducer = cpp_reduce::any_of([](int value) { return 10 <= value && value <= 20; });

    REQUIRE(reducer(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(reducer(std::vector<int>{}) == false);
    REQUIRE(reducer(std::vector<int>{ 12, 1401, 13, 14, 19 }) == true);
}

TEST_CASE("none_of - built-in implementation", "")
{
    const auto reducer = cpp_reduce::none_of([](int value) { return 10 <= value && value <= 20; });

    REQUIRE(reducer(std::vector<int>{ 12, 13, 14, 19 }) == false);
    REQUIRE(reducer(std::vector<int>{}) == true);
    REQUIRE(reducer(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("copy", "")
{
    std::vector<int> res = {};
    const auto reducer = cpp_reduce::copy(std::back_inserter(res));
    reducer(std::vector<int>{ 12, 13, 14, 19 });
    reducer(std::vector<int>{ 999, 990 });
    REQUIRE_THAT(res, Catch::Matchers::RangeEquals({ 12, 13, 14, 19, 999, 990 }));
}

TEST_CASE("transform-copy", "")
{
    std::vector<int> res = {};
    const auto reducer = cpp_reduce::transform([](int x) { return x * 10 + 1; })
        >>= cpp_reduce::copy(std::back_inserter(res));
    reducer(std::vector<int>{ 12, 13, 14, 19 });
    REQUIRE_THAT(res, Catch::Matchers::RangeEquals({ 121, 131, 141, 191 }));
}

TEST_CASE("filter-copy", "")
{
    std::vector<int> res = {};
    const auto reducer = cpp_reduce::filter([](int x) { return x < 14; }) >>= cpp_reduce::copy(std::back_inserter(res));
    reducer(std::vector<int>{ 12, 13, 14, 19 });
    REQUIRE_THAT(res, Catch::Matchers::RangeEquals({ 12, 13 }));
}

TEST_CASE("transform-filter-copy", "")
{
    std::vector<std::string> res = {};
    const auto reducer = cpp_reduce::transform(str)  //
        >>= cpp_reduce::filter([](const std::string& s) { return s.size() == 2; })
        >>= cpp_reduce::copy(std::back_inserter(res));
    reducer(std::vector<int>{ 1, 9, 12, 99, 101, 110 });
    REQUIRE_THAT(res, Catch::Matchers::RangeEquals({ "12", "99" }));
}

TEST_CASE("transform-filter-all_of", "")
{
    std::vector<std::string> res = {};
    const auto reducer = cpp_reduce::transform(str)  //
        >>= cpp_reduce::filter([](const std::string& s) { return s.size() == 2; })
        >>= cpp_reduce::all_of([](const std::string& s) { return s.size() == 2; });

    REQUIRE(reducer(std::vector<int>{ 1, 9, 12, 99, 101, 110 }) == true);
}

TEST_CASE("filter-transform-copy", "")
{
    std::vector<std::string> res = {};
    const auto reducer = cpp_reduce::filter([](int x) { return x % 2 == 0; })  //
        >>= cpp_reduce::transform(str)                                         //
        >>= cpp_reduce::copy(std::back_inserter(res));
    reducer(std::vector<int>{ 1, 9, 12, 99, 101, 110 });
    REQUIRE_THAT(res, Catch::Matchers::RangeEquals({ "12", "110" }));
}
