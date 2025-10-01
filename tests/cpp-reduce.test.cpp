#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <cpp-reduce/cpp-reduce.hpp>

TEST_CASE("simple reduce invocation", "")
{
    const std::vector<int> input = { 2, 3, 5, 10 };
    REQUIRE(cpp_reduce::fold(0, std::plus<>{})(input) == 20);
}

TEST_CASE("all_of - regular implementation", "")
{
    const auto folder = cpp_reduce::fold(true, [](bool res, int value) -> bool {  //
        return res && (10 <= value && value <= 20);
    });

    REQUIRE(folder(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(folder(std::vector<int>{}) == true);
    REQUIRE(folder(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("all_of - short circuit implementation", "")
{
    const auto folder = cpp_reduce::fold(
        true,
        [](bool res, int value) -> cpp_reduce::step_t<bool>
        {  //
            return cpp_reduce::break_with_if(res && (10 <= value && value <= 20), [](bool v) { return !v; });
        });

    REQUIRE(folder(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(folder(std::vector<int>{}) == true);
    REQUIRE(folder(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("all_of - built-in implementation", "")
{
    const auto folder = cpp_reduce::all_of([](int value) { return 10 <= value && value <= 20; });

    REQUIRE(folder(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(folder(std::vector<int>{}) == true);
    REQUIRE(folder(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("any_of - built-in implementation", "")
{
    const auto folder = cpp_reduce::any_of([](int value) { return 10 <= value && value <= 20; });

    REQUIRE(folder(std::vector<int>{ 12, 13, 14, 19 }) == true);
    REQUIRE(folder(std::vector<int>{}) == false);
    REQUIRE(folder(std::vector<int>{ 12, 1401, 13, 14, 19 }) == true);
}

TEST_CASE("none_of - built-in implementation", "")
{
    const auto folder = cpp_reduce::none_of([](int value) { return 10 <= value && value <= 20; });

    REQUIRE(folder(std::vector<int>{ 12, 13, 14, 19 }) == false);
    REQUIRE(folder(std::vector<int>{}) == true);
    REQUIRE(folder(std::vector<int>{ 12, 1401, 13, 14, 19 }) == false);
}

TEST_CASE("copy", "")
{
    std::vector<int> res = {};
    const auto folder = cpp_reduce::copy(std::back_inserter(res));
    folder(std::vector<int>{ 12, 13, 14, 19 });
    folder(std::vector<int>{ 999, 990 });
    REQUIRE_THAT(res, Catch::Matchers::RangeEquals({ 12, 13, 14, 19, 999, 990 }));
}
