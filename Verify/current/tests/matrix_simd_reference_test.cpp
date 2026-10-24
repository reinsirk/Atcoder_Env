// SPDX-License-Identifier: CC0-1.0
#ifndef MATRIX_HEADER
#define MATRIX_HEADER "../../../Algorithm/rotate.cpp"
#endif
#include MATRIX_HEADER
#include MATRIX_HEADER
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>
namespace {
std::uint64_t checks = 0;
void check(bool x) { ++checks; if (!x) std::abort(); }
struct Box { std::uint32_t bits; };
enum class Code : std::uint32_t {};
struct VolatileBox { volatile std::uint32_t bits; };
static_assert(std::is_trivially_default_constructible_v<Box> && std::is_trivially_copyable_v<Box>);
static_assert(std::is_trivially_copyable_v<VolatileBox> && std::is_trivially_copy_assignable_v<VolatileBox>);
void verify_volatile() {
    std::vector<std::vector<VolatileBox>> input(65, std::vector<VolatileBox>(129));
    for (std::size_t i = 0; i < input.size(); ++i)
        for (std::size_t j = 0; j < input[i].size(); ++j)
            input[i][j].bits = static_cast<std::uint32_t>(i * 1009 + j);
    const auto a = transpose(input), b = rotate_90r(input), c = rotate_90l(input);
    for (std::size_t i = 0; i < input.size(); ++i)
        for (std::size_t j = 0; j < input[i].size(); ++j) {
            check(a[j][i].bits == input[i][j].bits);
            check(b[j][input.size() - 1 - i].bits == input[i][j].bits);
            check(c[input[i].size() - 1 - j][i].bits == input[i][j].bits);
        }
}
template<class T> void verify(std::size_t h, std::size_t w, std::mt19937& random) {
    std::vector<std::vector<T>> input(h, std::vector<T>(w));
    for (auto& row : input) for (auto& value : row) value = std::bit_cast<T>(static_cast<std::uint32_t>(random()));
    const auto original = input;
    for (int mode : {0, 1, 3}) {
        const auto output = mode == 0 ? transpose(input) : mode == 1 ? rotate_90r(input) : rotate_90l(input);
        check(output.size() == w);
        for (const auto& row : output) check(row.size() == h);
        for (std::size_t i = 0; i < h; ++i) for (std::size_t j = 0; j < w; ++j) {
            const auto r = mode == 3 ? w - 1 - j : j;
            const auto c = mode == 1 ? h - 1 - i : i;
            check(std::bit_cast<std::uint32_t>(output[r][c]) == std::bit_cast<std::uint32_t>(input[i][j]));
        }
    }
    const auto clockwise = rotate_quarter(input, -3);
    const auto counterclockwise = rotate_quarter(input, -1);
    const auto twice = transpose(transpose(input));
    for (std::size_t i = 0; i < h; ++i) for (std::size_t j = 0; j < w; ++j) {
        const auto bits = std::bit_cast<std::uint32_t>(input[i][j]);
        check(std::bit_cast<std::uint32_t>(clockwise[j][h - 1 - i]) == bits);
        check(std::bit_cast<std::uint32_t>(counterclockwise[w - 1 - j][i]) == bits);
        check(std::bit_cast<std::uint32_t>(twice[i][j]) == bits);
        check(std::bit_cast<std::uint32_t>(original[i][j]) == bits);
    }
}
}
int main() {
    std::mt19937 random(261022);
    for (auto [h, w] : {std::pair{31,128}, {32,32}, {32,63}, {32,64}, {33,33}, {48,48}, {32,127}, {32,128}, {32,129}, {33,128}, {63,129}, {64,128}, {65,129}, {127,32}, {128,31}, {128,32}, {129,33}, {128,128}, {129,129}, {255,257}, {257,255}, {384,511}, {513,384}, {1024,1024}, {32,16383}, {32,16384}, {33,15887}, {33,15888}, {63,8322}, {63,8323}, {64,8191}, {64,8192}, {65,8192}}) {
        verify<std::int32_t>(h,w,random); verify<std::uint32_t>(h,w,random);
        verify<float>(h,w,random); verify<Box>(h,w,random); verify<Code>(h,w,random);
    }
    verify_volatile();
#ifndef SKIP_RUNTIME_CHECKS
    for (std::size_t bad : {std::size_t{0}, std::size_t{7}, std::size_t{8}, std::size_t{127}, std::size_t{128}}) {
        std::vector<std::vector<int>> input(129, std::vector<int>(129));
        input[bad].resize(128);
        for (int mode : {0, 1, 3}) {
            bool caught = false;
            try { (void)(mode == 0 ? transpose(input) : mode == 1 ? rotate_90r(input) : rotate_90l(input)); }
            catch (const std::invalid_argument&) { caught = true; }
            check(caught);
        }
    }
#endif
#ifndef SKIP_RUNTIME_CHECKS
    for (std::size_t bad : {std::size_t{0}, std::size_t{7}, std::size_t{8}, std::size_t{63}}) {
        std::vector<std::vector<int>> input(64, std::vector<int>(8192));
        input[bad].resize(8191);
        for (int mode : {0,1,3}) {
            bool caught = false;
            try { (void)(mode == 0 ? transpose(input) : mode == 1 ? rotate_90r(input) : rotate_90l(input)); }
            catch (const std::invalid_argument&) { caught = true; }
            check(caught);
        }
    }
#endif
    std::cout << "PASS checks=" << checks << '\n';
}
