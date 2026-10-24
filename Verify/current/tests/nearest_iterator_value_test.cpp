// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_NEAREST_SOURCE
#define SIRKIT_NEAREST_SOURCE "../../../Geometry/Nearest_Two_points.cpp"
#endif
#include SIRKIT_NEAREST_SOURCE
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
std::size_t checks = 0;
void require(bool value) {
    ++checks;
    if (!value) std::abort();
}
void inspect(const std::optional<NearestTwoPointsResult>& answer,
             const std::vector<P>& input, std::uint64_t expected) {
    require(bool(answer) == (input.size() >= 2));
    if (!answer) return;
    require(answer->first_index < answer->second_index);
    require(answer->second_index < input.size());
    require(answer->distance_squared == expected);
    const auto a = input[answer->first_index], b = input[answer->second_index];
    const auto dx = std::int64_t(a.first) - b.first;
    const auto dy = std::int64_t(a.second) - b.second;
    require(std::uint64_t(dx * dx + dy * dy) == expected);
}
void check(std::vector<P> input, std::uint64_t expected) {
    const auto original = input;
    for (bool sorted : {false, true}) {
        if (sorted) std::sort(input.begin(), input.end());
        for (int mode = 0; mode != 3; ++mode) {
            std::size_t reads = 0;
            auto values = input | std::views::transform([&](P point) {
                ++reads;
                return std::pair<std::int64_t, std::int64_t>{point.first, point.second};
            });
            static_assert(!std::is_reference_v<std::ranges::range_reference_t<decltype(values)>>);
            auto noncommon = values | std::views::take_while([](const auto&) { return true; });
            static_assert(!std::ranges::common_range<decltype(noncommon)>);
            std::optional<NearestTwoPointsResult> answer;
            if (mode == 0) answer = sorted
                ? nearest_two_points_sorted(values.begin(), values.end())
                : nearest_two_points(values.begin(), values.end());
            if (mode == 1) answer = sorted
                ? nearest_two_points_sorted(values.begin(), static_cast<std::ptrdiff_t>(input.size()))
                : nearest_two_points(values.begin(), static_cast<std::ptrdiff_t>(input.size()));
            if (mode == 2) answer = sorted
                ? nearest_two_points_sorted(noncommon.begin(), noncommon.end())
                : nearest_two_points(noncommon.begin(), noncommon.end());
            inspect(answer, input, expected);
            // 重複による早期終了でも、iteratorの全要素は読み取る。
            // take_whileの終端判定による追加の評価回数は規定しない。
            require(reads >= input.size());
        }
    }
    auto sorted_original = original;
    std::sort(sorted_original.begin(), sorted_original.end());
    require(input == sorted_original);
}
}
int main() {
    check({}, 0);
    check({{7, 19}}, 0);
    check({{100, 500}, {37, 100}, {-200, 300}, {38, 100}}, 1);
    check({{100, 500}, {37, 100}, {-200, 300}, {37, 100}}, 0);
    std::vector<P> duplicate(4096);
    for (std::size_t i = 0; i < duplicate.size(); ++i)
        duplicate[i] = {static_cast<std::int32_t>(i * 37), static_cast<std::int32_t>(i * 101)};
    duplicate[1] = duplicate[0];
    check(duplicate, 0);
    std::printf("PASS checks=%zu value-returning iterators\n", checks);
}
