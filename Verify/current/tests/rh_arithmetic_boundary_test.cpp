// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RH_SOURCE
#define SIRKIT_RH_SOURCE "../../../String/RollingHash.cpp"
#endif
#include SIRKIT_RH_SOURCE
#include <boost/multiprecision/cpp_int.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {
using Big = boost::multiprecision::cpp_int;
constexpr auto p = RollingHash::modulus;
std::size_t checks = 0;
void require(bool value) { ++checks; if (!value) std::abort(); }
std::uint64_t residue(Big value) {
    value %= p;
    if (value < 0) value += p;
    return value.convert_to<std::uint64_t>();
}
void inspect(const RollingHash& hash, const std::string& text) {
    require(hash.size() == text.size());
    std::vector<std::uint64_t> prefixes(text.size() + 1), powers(text.size() + 1, 1);
    for (std::size_t i = 0; i < text.size(); ++i) {
        prefixes[i + 1] = residue(Big(prefixes[i]) * hash.base() +
                                  static_cast<unsigned char>(text[i]));
        powers[i + 1] = residue(Big(powers[i]) * hash.base());
        require(hash.get(0, i + 1) == prefixes[i + 1]);
    }
    require(hash.whole() == prefixes.back());
    for (std::size_t r = 0; r <= text.size(); ++r) {
        const std::size_t l = r / 3;
        require(hash.get(l, r) == residue(Big(prefixes[r]) - Big(prefixes[l]) * powers[r - l]));
    }
    // 任意の正規化済みハッシュを、独立した多倍長の積和と照合する。
    for (auto left : {std::uint64_t{0}, std::uint64_t{1}, p / 2, p - 2, p - 1}) {
        for (auto right : {std::uint64_t{0}, std::uint64_t{1}, p / 2, p - 2, p - 1}) {
            for (std::size_t length : {std::size_t{0}, std::size_t{1}, std::size_t{3},
                                      std::size_t{4}, std::size_t{5}, text.size()}) {
                if (length <= text.size())
                    require(hash.concat(left, right, length) == residue(Big(left) * powers[length] + right));
            }
        }
    }
}
}

int main() {
    const std::vector<std::size_t> lengths{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 63, 64, 65,
        255, 256, 257, 4095, 4096, 4097};
    for (std::uint64_t base : {std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{2},
                              std::uint64_t{10}, std::uint64_t{256}, std::uint64_t{1000000007},
                              p / 2, p - 2, p - 1, p, std::numeric_limits<std::uint64_t>::max()}) {
        RollingHash reusable("", base);
        for (std::size_t length : lengths) {
            std::string text(length, '\0');
            for (std::size_t i = 0; i < length; ++i) text[i] = static_cast<char>((i * 137 + length) % 256);
            RollingHash fresh(text, base);
            inspect(fresh, text);
            reusable.assign(text);
            inspect(reusable, text);
            const std::string short_text = text.substr(0, length / 2);
            reusable.assign(short_text);
            inspect(reusable, short_text);
            reusable.assign(text);
            inspect(reusable, text);
            reusable.clear();
            require(reusable.concat(p - 1, p - 1, 0) == p - 2);
            reusable.assign(text);
            inspect(reusable, text);
            RollingHash moved(std::move(fresh));
            require(fresh.empty() && fresh.concat(p - 1, p - 1, 0) == p - 2);
            inspect(moved, text);
        }
    }
    std::printf("PASS arithmetic boundary checks=%zu\n", checks);
}
