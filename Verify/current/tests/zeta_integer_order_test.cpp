// SPDX-License-Identifier: CC0-1.0
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta_Mebius.cpp"
#endif
#include SOURCE
#include <complex>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
std::uint64_t checks = 0;
void check(bool result) { ++checks; if (!result) std::abort(); }

// 全添字を走査する低速参照。素数ごとの昇順/降順という演算順を固定する。
template <bool ToMultiples, bool Subtract, class T>
void ordered_reference(std::vector<T>& values, const std::vector<int>& primes) {
    for (const int prime : primes) {
        const auto p = static_cast<std::size_t>(prime);
        auto step = [&](std::size_t to) {
            if (to % p != 0) return;
            auto& left = values[ToMultiples ? to : to / p];
            const auto& right = values[ToMultiples ? to / p : to];
            if constexpr (Subtract) left -= right; else left += right;
        };
        if constexpr (ToMultiples != Subtract) {
            for (std::size_t to = p; to < values.size(); ++to) step(to);
        } else {
            for (std::size_t to = values.size(); to > p;) step(--to);
        }
    }
}

template <bool ToMultiples, bool Subtract, class T>
void compare(std::vector<T> values, const IntegerSieve& sieve) {
    auto expected = values;
    ordered_reference<ToMultiples, Subtract>(expected, sieve.prime_list);
    reinsirk::detail::IntegerTransform::propagate<ToMultiples, Subtract>(values, sieve.prime_list);
    check(values == expected);
}

template <class T> void floating_suite() {
    std::mt19937 rng(412855);
    for (const std::size_t n : {0U, 1U, 2U, 3U, 7U, 8U, 15U, 16U, 17U, 31U, 32U,
                              127U, 128U, 129U, 255U, 256U, 257U, 258U, 511U, 512U, 513U, 1025U}) {
        IntegerSieve sieve(n ? static_cast<int>(n - 1) : 0);
        std::vector<T> values(n);
        for (auto& value : values) {
            const double real = (static_cast<int>(rng() % 2001) - 1000) / 997.0;
            if constexpr (std::is_floating_point_v<T>) value = static_cast<T>(real);
            else value = T(static_cast<typename T::value_type>(real),
                           static_cast<typename T::value_type>((static_cast<int>(rng() % 2001) - 1000) / 991.0));
        }
        compare<false, false>(values, sieve); compare<false, true>(values, sieve);
        compare<true, false>(values, sieve); compare<true, true>(values, sieve);
    }
}

// 自明にコピーできる小型値でも、演算が例外を送出し得る。
struct TrivialValue {
    std::int64_t value = 0;
    static inline int budget = -1;
    static inline std::uint64_t calls = 0, trace = 0;
    TrivialValue() = default;
    explicit TrivialValue(std::int64_t x) : value(x) {}
    void record(const TrivialValue& right, bool subtract) const {
        if (budget == 0) throw std::runtime_error("injected arithmetic failure");
        if (budget > 0) --budget;
        ++calls;
        trace = trace * 1000003 + static_cast<std::uint64_t>(value) * 10007
              + static_cast<std::uint64_t>(right.value) * 17 + subtract;
    }
    TrivialValue& operator+=(const TrivialValue& right) { record(right, false); value += right.value; return *this; }
    TrivialValue& operator-=(const TrivialValue& right) { record(right, true); value -= right.value; return *this; }
    friend bool operator==(const TrivialValue&, const TrivialValue&) = default;
};
static_assert(std::is_trivially_copyable_v<TrivialValue> && sizeof(TrivialValue) <= 8);

template <bool ToMultiples, bool Subtract> void exception_suite() {
    for (const std::size_t n : {257U, 258U, 513U}) {
        IntegerSieve sieve(static_cast<int>(n - 1));
        std::vector<TrivialValue> initial(n);
        for (std::size_t i = 0; i < n; ++i) initial[i] = TrivialValue(static_cast<std::int64_t>(i % 17) - 8);
        for (const int limit : {-1, 0, 1, 7, 8, 9, 15, 16, 31, 32, 63, 64, 127, 255, 511, 1023, 2047}) {
            auto expected = initial, actual = initial;
            TrivialValue::budget = limit; TrivialValue::calls = TrivialValue::trace = 0;
            bool expected_throw = false;
            try { ordered_reference<ToMultiples, Subtract>(expected, sieve.prime_list); }
            catch (const std::runtime_error&) { expected_throw = true; }
            const auto calls = TrivialValue::calls, trace = TrivialValue::trace;
            TrivialValue::budget = limit; TrivialValue::calls = TrivialValue::trace = 0;
            bool actual_throw = false;
            try { reinsirk::detail::IntegerTransform::propagate<ToMultiples, Subtract>(actual, sieve.prime_list); }
            catch (const std::runtime_error&) { actual_throw = true; }
            check(expected_throw == actual_throw); check(expected == actual);
            check(calls == TrivialValue::calls); check(trace == TrivialValue::trace);
        }
    }
}
}
int main() {
    floating_suite<float>(); floating_suite<double>();
    floating_suite<std::complex<float>>(); floating_suite<std::complex<double>>();
    exception_suite<false, false>(); exception_suite<false, true>();
    exception_suite<true, false>(); exception_suite<true, true>();
    std::cout << "PASS checks=" << checks << '\n';
}
