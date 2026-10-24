#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/arc117/tasks/arc117_c
// Submission: https://atcoder.jp/contests/arc117/submissions/80184601

// Library: Math/Combination_AtCoderMod.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/binomial_coefficient.md
//
// API一覧
// [PrimeBinomial<Mint>]
// AtCoder modint用・素数専用。Mintは利用側で用意し、動的な法を変えたら計算器も作り直す。
// PrimeBinomial(max_n=5'000'000,max_steps=1'000'000): 前計算範囲と表外の積の回数上限を指定して構築。
// prepare(max_n=5'000'000): min(max_n,mod-1)まで階乗・逆階乗を準備。move元も再準備して利用可。
// COM(n,k): nCkをMintで返す。n<0,k<0,k>nは0。準備済みn<modはO(1)、n>=modはLucas。
// Lucas(n,k): COMと同じ。COMもLucasを自動選択する。
// H(n,k): nHkをMintで返す。0H0=1、0Hk=0（k>0）。int64_t超過はoverflow_error。
// 表外は積で計算し、回数上限超過はlength_error。SKIP_RUNTIME_CHECKSで入力検査を除く。

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#if !defined(SKIP_RUNTIME_CHECKS) && !defined(SIRKIT_PRIMALITY32_DEFINED)
// 内部の素数判定。定義はファイル末尾。
namespace reinsirk::detail {
inline bool is_prime_u32(std::uint32_t n) noexcept;
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_PRIME_BINOMIAL_DEFINED
#define SIRKIT_PRIME_BINOMIAL_DEFINED
#ifndef SIRKIT_PRIME_BINOMIAL_LUCAS_THREE_DEFINED
#define SIRKIT_PRIME_BINOMIAL_LUCAS_THREE_DEFINED
namespace reinsirk::detail {
// 法3は6桁ごとの桁和で0を判定し、2の個数の偶奇から符号を求める。
inline std::uint32_t lucas_mod3(std::uint64_t n, std::uint64_t k) noexcept {
    constexpr std::uint32_t Base = 729;
    static constexpr auto table = [] {
        struct Table { std::uint8_t value[Base]{}; };
        Table result;
        for (std::uint32_t i = 1; i < Base; ++i) {
            const auto previous = result.value[i / 3];
            const auto digit = i % 3;
            result.value[i] = static_cast<std::uint8_t>(((previous & 31) + digit) |
                ((previous & 32) ^ (digit == 2 ? 32 : 0)));
        }
        return result;
    }();
    unsigned parity = 0;
    while (k) {
        const auto a = static_cast<std::uint32_t>(n % Base);
        const auto b = static_cast<std::uint32_t>(k % Base);
        if (b > a) return 0;
        const auto x = table.value[a], y = table.value[b], z = table.value[a - b];
        if ((x & 31) != (y & 31) + (z & 31)) return 0;
        parity ^= x ^ y ^ z;
        n /= Base;
        k /= Base;
    }
    return 1 + ((parity >> 5) & 1);
}
} // namespace reinsirk::detail
#endif

template <class Mint>
class PrimeBinomial
{
    using i64 = std::int64_t;
    using u64 = std::uint64_t;
    const int modulus_;
    const u64 max_steps_;
        struct RawTableValue
    {
        std::uint32_t value;
        RawTableValue() noexcept {}
        RawTableValue(Mint x) : value(static_cast<std::uint32_t>(x.val())) {}
        operator Mint() const { return Mint::raw(static_cast<int>(value)); }
        Mint operator*(Mint x) const { return Mint::raw(static_cast<int>(value)) * x; }
        Mint operator-() const requires requires(Mint x) { -x; }
        { return -Mint::raw(static_cast<int>(value)); }
        Mint inv() const { return Mint::raw(static_cast<int>(value)).inv(); }
    };
    using TableValue = decltype([] {
        if constexpr (requires(const Mint& x) { static_cast<std::uint32_t>(x.val()); })
            return RawTableValue{};
        else
            return Mint{};
    }());
    std::vector<TableValue> fac_{Mint(1)}, ifac_{Mint(1)};
    static void grow_table(std::vector<TableValue>& table, std::size_t count)
    {
#if defined(__GNUC__) && !defined(__clang__)
        if constexpr (requires(const Mint& x) { static_cast<std::uint32_t>(x.val()); })
        {
            if (count > table.capacity() && count >= 4096)
            {
                std::vector<TableValue> fresh;
                const auto size = table.size();
                const auto growth = size + std::min(size, fresh.max_size() - size);
                fresh.reserve(std::max(count, growth));
                fresh.resize(count);
                std::copy(table.begin(), table.end(), fresh.begin());
                table.swap(fresh);
                return;
            }
        }
#endif
        table.resize(count);
    }

    // 固定法は定数として参照し、動的な法だけ変更検査を行う。
    static constexpr bool fixed_modulus_ = requires { typename std::integral_constant<int, Mint::mod()>; };

    int modulus() const noexcept
    {
        if constexpr (fixed_modulus_)
            return Mint::mod();
        else
            return modulus_;
    }

    // 古い法のテーブルを使わないようにする。
    void check_modulus() const
    {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (!fixed_modulus_)
            if (Mint::mod() != modulus_)
                throw std::logic_error("PrimeBinomial: modulus changed");
#endif
    }
    // 大きい追加領域だけをまとめて更新し、小さい表は従来の逐次計算を使う。
    void extend_large(std::size_t old_size, std::size_t target)
    {
        const u64 p = static_cast<u64>(modulus());
        std::size_t i = old_size;
        if (static_cast<u64>(target) * 4 + 6 < p)
        {
            u64 pair = static_cast<u64>(i) * (i + 1) % p;
            for (; i + 3 <= target; i += 4)
            {
                const Mint previous = fac_[i - 1];
                const Mint p2 = Mint::raw(static_cast<int>(pair));
                u64 next_pair = pair + 4 * i + 6;
                next_pair -= next_pair >= p ? p : 0;
                const Mint q2 = Mint::raw(static_cast<int>(next_pair));
                pair = next_pair + 4 * i + 14;
                pair -= pair >= p ? p : 0;
                fac_[i] = previous * Mint::raw(static_cast<int>(i));
                fac_[i + 1] = previous * p2;
                fac_[i + 2] = previous * (p2 * Mint::raw(static_cast<int>(i + 2)));
                fac_[i + 3] = previous * (p2 * q2);
            }
        }
        else
        {
            for (; i + 1 <= target; i += 2)
            {
                const Mint previous = fac_[i - 1];
                const Mint pair = Mint::raw(static_cast<int>(i)) * Mint::raw(static_cast<int>(i + 1));
                fac_[i] = previous * Mint::raw(static_cast<int>(i));
                fac_[i + 1] = previous * pair;
            }
        }
        for (; i <= target; ++i)
            fac_[i] = fac_[i - 1] * Mint::raw(static_cast<int>(i));
        if constexpr (requires(Mint x) { -x; })
        {
            // Wilsonの定理: ifac[k]=(-1)^(k+1)*fac[p-1-k]。
            if (target == p - 1)
            {
                i = old_size;
                if (i & 1)
                    ifac_[i] = fac_[target - i], ++i;
                for (; i < target; i += 2)
                {
                    ifac_[i] = -fac_[target - i];
                    ifac_[i + 1] = fac_[target - i - 1];
                }
                if (i <= target)
                    ifac_[i] = -fac_[target - i];
                return;
            }
        }
        ifac_[target] = fac_[target].inv();
        i = target;
        if (static_cast<u64>(target) * 4 + 6 < p)
        {
            u64 pair = static_cast<u64>(i) * (i - 1) % p;
            for (; i > old_size + 3; i -= 4)
            {
                const Mint previous = ifac_[i];
                const Mint p2 = Mint::raw(static_cast<int>(pair));
                const u64 delta = 4 * i - 6;
                const u64 next_pair = pair + (pair < delta ? p : 0) - delta;
                const Mint q2 = Mint::raw(static_cast<int>(next_pair));
                const u64 next_delta = 4 * i - 14;
                pair = next_pair + (next_pair < next_delta ? p : 0) - next_delta;
                ifac_[i - 1] = previous * Mint::raw(static_cast<int>(i));
                ifac_[i - 2] = previous * p2;
                ifac_[i - 3] = previous * (p2 * Mint::raw(static_cast<int>(i - 2)));
                ifac_[i - 4] = previous * (p2 * q2);
            }
        }
        else
        {
            for (; i > old_size + 1; i -= 2)
            {
                const Mint previous = ifac_[i];
                const Mint pair = Mint::raw(static_cast<int>(i)) * Mint::raw(static_cast<int>(i - 1));
                ifac_[i - 1] = previous * Mint::raw(static_cast<int>(i));
                ifac_[i - 2] = previous * pair;
            }
        }
        for (; i > old_size; --i)
            ifac_[i - 1] = ifac_[i] * Mint::raw(static_cast<int>(i));
    }

    // 0<=k<=n<modのnCk。表外では小さい側のk回だけ積を取る。
    Mint digit(int n, int k, u64 &remaining_steps) const
    {
        if (static_cast<std::size_t>(n) < fac_.size())
            return fac_[n] * ifac_[k] * ifac_[n - k];
        k = std::min(k, n - k);
        if (k == 0)
            return Mint(1);
        if (k == 1)
            return Mint::raw(n);
        if (static_cast<u64>(k) > remaining_steps)
            throw std::length_error("PrimeBinomial: increase max_n or max_steps");
        remaining_steps -= k;
        Mint numerator(1), denominator(1);
        for (int i = 1; i <= k; ++i)
        {
            numerator *= Mint::raw(n - k + i);
            denominator *= Mint::raw(i);
        }
        return numerator * denominator.inv();
    }

    // 全桁の0判定を先に済ませ、同じ桁を再び除算しない。
    // GCC/Clangでは桁処理を展開し、短い問い合わせの関数呼び出しを省く。
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    Mint lucas(i64 n, i64 k, u64 &remaining_steps) const
    {
        struct Digit { int n, k; };
        Digit digits[std::numeric_limits<i64>::digits];
        unsigned count = 0;
        for (i64 nn = n, kk = k; kk; nn /= modulus(), kk /= modulus())
        {
            const int ni = static_cast<int>(nn % modulus());
            const int ki = static_cast<int>(kk % modulus());
            if (ki > ni)
                return Mint(0);
            digits[count++] = {ni, ki};
        }
        Mint result(1);
        for (unsigned i = 0; i < count; ++i)
            result *= digit(digits[i].n, digits[i].k, remaining_steps);
        return result;
    }

    // 階乗表で即答できない問い合わせを計算する。
    Mint compute_uncached(i64 n, i64 k) const
    {
        k = std::min(k, n - k);
        if (k == 0)
            return Mint(1);
        if (k == 1)
            return Mint(n);
        // 全表がある法2・3はLucasの桁を整数だけで処理する。
        const auto p = modulus();
        if (p == 2 && fac_.size() == 2)
            return Mint::raw((static_cast<u64>(k) & ~static_cast<u64>(n)) == 0);
        if (p == 3 && fac_.size() == 3) {
            // kは2以上かつn/2以下。2桁以内なら定数個の整数演算で返す。
            if (n < 9)
                return Mint::raw(k == 2 ? (n % 3 == 2) : (k == 3 ? 2 : 1));
            return Mint::raw(static_cast<int>(reinsirk::detail::lucas_mod3(
                static_cast<u64>(n), static_cast<u64>(k))));
        }
        u64 remaining_steps = max_steps_;
        if (n < modulus())
            return digit(static_cast<int>(n), static_cast<int>(k), remaining_steps);
        return lucas(n, k, remaining_steps);
    }

public:
    explicit PrimeBinomial(i64 max_n = 5'000'000, u64 max_steps = 1'000'000)
        : modulus_(Mint::mod()), max_steps_(max_steps)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (modulus_ < 2 || !reinsirk::detail::is_prime_u32(static_cast<std::uint32_t>(modulus_)))
            throw std::domain_error("PrimeBinomial: prime modulus required");
#endif
        prepare(max_n);
    }

    void prepare(i64 max_n = 5'000'000)
    {
        check_modulus();
#ifndef SKIP_RUNTIME_CHECKS
        if (max_n < 0)
            throw std::invalid_argument("PrimeBinomial: negative bound");
#endif
        const auto bound = static_cast<u64>(std::min<i64>(max_n, modulus() - 1));
        if (bound >= std::numeric_limits<std::size_t>::max())
            throw std::length_error("PrimeBinomial: table size exceeds size_t");
        const auto target = static_cast<std::size_t>(bound);
        auto old_size = fac_.size();
        if (target < old_size)
            return;
        try
        {
            grow_table(fac_, target + 1);
            grow_table(ifac_, target + 1);
        }
        catch (...)
        {
            fac_.resize(old_size);
            ifac_.resize(old_size);
            throw;
        }
        if (old_size == 0) {
            fac_[0] = Mint(1);
            ifac_[0] = Mint(1);
            old_size = 1;
        }
        if (target + 1 - old_size >= 4096)
        {
            extend_large(old_size, target);
            return;
        }
        for (std::size_t i = old_size; i <= target; ++i)
            fac_[i] = fac_[i - 1] * Mint::raw(static_cast<int>(i));
        ifac_[target] = fac_[target].inv();
        for (std::size_t i = target; i > old_size; --i)
            ifac_[i - 1] = ifac_[i] * Mint::raw(static_cast<int>(i));
    }

    [[nodiscard]] Mint COM(i64 n, i64 k) const
    {
        check_modulus();
        if (n < 0 || k < 0 || k > n)
            return Mint(0);
        if (static_cast<u64>(n) < fac_.size())
            return fac_[n] * ifac_[k] * ifac_[n - k];
        return compute_uncached(n, k);
    }

    [[nodiscard]] Mint Lucas(i64 n, i64 k) const { return COM(n, k); }

    [[nodiscard]] Mint H(i64 n, i64 k) const
    {
        check_modulus();
        if (n < 0 || k < 0)
            return Mint(0);
        if (k == 0)
            return Mint(1);
        if (n == 0)
            return Mint(0);
        if (n - 1 > std::numeric_limits<i64>::max() - k)
            throw std::overflow_error("PrimeBinomial: n+k-1");
        return COM(n - 1 + k, k);
    }
};
#endif

#if !defined(SKIP_RUNTIME_CHECKS) && !defined(SIRKIT_PRIMALITY32_DEFINED)
#define SIRKIT_PRIMALITY32_DEFINED
namespace reinsirk::detail {
// uint32_t全域の素数判定。Miller-Rabinの底2,7,61を使う。
inline bool is_prime_u32(std::uint32_t n) noexcept
{
    // 競技プログラミングでよく使う素数は、累乗計算を省く。
    switch (n)
    {
    case 167772161u:
    case 469762049u:
    case 754974721u:
    case 998244353u:
    case 1000000007u:
    case 1000000009u:
    case 1004535809u:
    case 1224736769u:
    case 2013265921u:
        return true;
    default:
        break;
    }
    if (n < 2)
        return false;
    if (n % 2u == 0) return n == 2u;
    if (n % 3u == 0) return n == 3u;
    if (n % 5u == 0) return n == 5u;
    if (n % 7u == 0) return n == 7u;
    if (n % 11u == 0) return n == 11u;
    std::uint32_t odd = n - 1;
    unsigned shifts = 0;
    while ((odd & 1u) == 0)
    {
        odd >>= 1;
        ++shifts;
    }
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
    // R=2^64。n<2^32なので、累乗中の剰余は2n未満のまま保持できる。
    using u64 = std::uint64_t;
    using u128 = __uint128_t; // GCC/Clangの128ビット整数拡張。
    u64 inverse = n;
    for (int i = 0; i < 5; ++i) inverse *= 2u - u64(n) * inverse;
    const u64 negative_inverse = u64{0} - inverse;
    const auto multiply = [n, negative_inverse](u64 a, u64 b) {
        const u128 product = static_cast<u128>(a) * b;
        const u128 correction = static_cast<u128>(static_cast<u64>(product) * negative_inverse) * n;
        return static_cast<u64>((product + correction) >> 64);
    };
    const u64 one = (u64{0} - n) % n;
    const u64 r_squared = one * one % n;
    for (std::uint32_t base : {2u, 7u, 61u}) {
        if (base % n == 0) continue;
        u64 value = one, power = multiply(base % n, r_squared);
        for (std::uint32_t exponent = odd; exponent; exponent >>= 1) {
            if (exponent & 1u) value = multiply(value, power);
            power = multiply(power, power);
        }
        if (value >= n) value -= n;
        if (value == one || value == n - one) continue;
        unsigned step = 1;
        for (; step < shifts; ++step) {
            value = multiply(value, value);
            if (value >= n) value -= n;
            if (value == n - one) break;
        }
        if (step == shifts) return false;
    }
#else
    // R=2^32のMontgomery乗算。33ビットの還元結果もuint64_tで扱う。
    std::uint32_t inverse = n;
    for (int i = 0; i < 5; ++i)
        inverse *= 2u - n * inverse;
    const std::uint32_t negative_inverse = 0u - inverse;
    const auto multiply = [n, negative_inverse](std::uint32_t a, std::uint32_t b)
    {
        const std::uint64_t product = static_cast<std::uint64_t>(a) * b;
        const std::uint32_t factor = static_cast<std::uint32_t>(product) * negative_inverse;
        const std::uint64_t correction = static_cast<std::uint64_t>(factor) * n;
        std::uint64_t result = (product >> 32) + (correction >> 32) +
                               (static_cast<std::uint32_t>(product) != 0);
        if (result >= n)
            result -= n;
        return static_cast<std::uint32_t>(result);
    };
    const auto one = static_cast<std::uint32_t>((std::uint64_t{1} << 32) % n);
    const auto r_squared = static_cast<std::uint32_t>(static_cast<std::uint64_t>(one) * one % n);
    for (std::uint32_t base : {2u, 7u, 61u})
    {
        if (base % n == 0)
            continue;
        std::uint32_t value = one, power = multiply(base % n, r_squared);
        for (std::uint32_t exponent = odd; ;)
        {
            if (exponent & 1u)
                value = multiply(value, power);
            exponent >>= 1;
            if (exponent == 0) break;
            power = multiply(power, power);
        }
        if (value == one || value == n - one)
            continue;
        unsigned step = 1;
        for (; step < shifts; ++step)
        {
            value = multiply(value, value);
            if (value == n - one)
                break;
        }
        if (step == shifts)
            return false;
    }
#endif
    return true;
}
} // namespace reinsirk::detail
#endif


// SPDX-License-Identifier: CC0-1.0
// Verify I/O: Verify/current/common/fast_io.hpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef SIRKIT_VERIFY_FAST_IO_DEFINED
#define SIRKIT_VERIFY_FAST_IO_DEFINED

// VerifyInput(file=stdin): 非所有のFILEから読む。別の入力方式と混用しない。
// >> 整数: 64bit以下の十進整数を読む。検査省略時は値域内の正しい入力を前提とする。
// >> char/string: 空白を飛ばし、1文字／空白までの文字列を読む。
// >> 浮動小数点: 十進文字列を変換する。通常のCロケールで使う。
// bool / !: 直前までの読み取りが成功しているか／失敗したかを返す。
class VerifyInput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<unsigned char, buffer_size> buffer_;
    std::size_t position_ = 0, size_ = 0;
    bool good_ = true;

    int peek() {
        if (position_ == size_) {
            size_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            position_ = 0;
            if (size_ == 0) return EOF;
        }
        return buffer_[position_];
    }

    int skip_space() {
        int c;
        while ((c = peek()) != EOF && c <= ' ') ++position_;
        if (c == EOF) good_ = false;
        return c;
    }

public:
    explicit VerifyInput(std::FILE* file = stdin) : file_(file) {}
    VerifyInput(const VerifyInput&) = delete;
    VerifyInput& operator=(const VerifyInput&) = delete;

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyInput& operator>>(T& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        bool negative = false;
        if (c == '-' || c == '+') {
            negative = c == '-';
            ++position_;
            c = peek();
        }
        if (c == EOF) {
            good_ = false;
            return *this;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (c < '0' || c > '9') {
            good_ = false;
            return *this;
        }
        // 先頭の0を除けば、20桁以下の末尾直前の値はuint64_tに収まる。
        while (c == '0') {
            ++position_;
            c = peek();
        }
        std::uint64_t previous = 0;
        unsigned last_digit = 0;
        std::size_t digits = 0;
#endif
        std::uint64_t value = 0;
        while (c >= '0' && c <= '9') {
#ifndef SKIP_RUNTIME_CHECKS
            previous = value;
            last_digit = static_cast<unsigned>(c - '0');
            if (digits <= 20) ++digits;
#endif
            value = value * 10 + static_cast<unsigned>(c - '0');
            ++position_;
            c = peek();
        }
#ifndef SKIP_RUNTIME_CHECKS
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if ((c != EOF && c > ' ') || digits > 20
            || (digits == 20 && (previous > maximum / 10
                || (previous == maximum / 10 && last_digit > maximum % 10)))) {
            good_ = false;
            return *this;
        }
        std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max());
        if constexpr (std::is_signed_v<T>) limit += negative;
        else if (negative && value != 0) good_ = false;
        if (value > limit) good_ = false;
        if (!good_) return *this;
#endif
        if constexpr (std::is_signed_v<T>) {
            constexpr std::uint64_t minimum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<T>::max()) + 1;
            // -(最小値)は評価せず、絶対値が最大値+1のときだけ最小値を使う。
            result = negative
                ? (value == minimum_magnitude ? std::numeric_limits<T>::min()
                                  : static_cast<T>(-static_cast<T>(value)))
                : static_cast<T>(value);
        } else result = static_cast<T>(value);
        return *this;
    }

    VerifyInput& operator>>(char& result) {
        if (!good_) return *this;
        const int c = skip_space();
        if (c != EOF) {
            result = static_cast<char>(c);
            ++position_;
        }
        return *this;
    }

    VerifyInput& operator>>(std::string& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        result.clear();
        do {
            result.push_back(static_cast<char>(c));
            ++position_;
            c = peek();
        } while (c != EOF && c > ' ');
        return *this;
    }

    template<class T> requires std::is_floating_point_v<T>
    VerifyInput& operator>>(T& result) {
        std::string token;
        *this >> token;
        if (!good_) return *this;
#ifndef SKIP_RUNTIME_CHECKS
        char* end = nullptr;
        const long double value = std::strtold(token.c_str(), &end);
        if (end != token.c_str() + token.size()) good_ = false;
        else result = static_cast<T>(value);
#else
        result = static_cast<T>(std::strtold(token.c_str(), nullptr));
#endif
        return *this;
    }
};

// VerifyOutput(file=stdout): 非所有のFILEへ書く。破棄時にflushし、FILEは閉じない。
// << 整数/char/文字列/浮動小数点: 値を出力する。整数は64bit以下に対応する。
// fixed(digits): 浮動小数点を小数点以下digits桁で出力する（digits>=0）。
// flush(): バッファの内容をFILEへ渡す。bool / !で書き込み失敗を確認できる。
class VerifyOutput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<char, buffer_size> buffer_;
    std::size_t size_ = 0;
    int precision_ = 6;
    bool fixed_ = false, good_ = true;

    void write(std::string_view text) {
        while (!text.empty() && good_) {
            if (size_ == buffer_.size()) flush();
            const auto count = std::min(text.size(), buffer_.size() - size_);
            std::memcpy(buffer_.data() + size_, text.data(), count);
            size_ += count;
            text.remove_prefix(count);
        }
    }

public:
    explicit VerifyOutput(std::FILE* file = stdout) : file_(file) {}
    VerifyOutput(const VerifyOutput&) = delete;
    VerifyOutput& operator=(const VerifyOutput&) = delete;
    ~VerifyOutput() { flush(); }

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    VerifyOutput& fixed(int digits) {
        if (digits < 0) throw std::invalid_argument("negative output precision");
        fixed_ = true;
        precision_ = digits;
        return *this;
    }

    void flush() noexcept {
        std::size_t written = 0;
        while (written < size_ && good_) {
            const auto count = std::fwrite(buffer_.data() + written, 1, size_ - written, file_);
            if (count == 0) good_ = false;
            written += count;
        }
        size_ = 0;
    }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyOutput& operator<<(T value) {
        if constexpr (std::is_same_v<T, bool>) return *this << static_cast<char>('0' + value);
        else {
            char digits[std::numeric_limits<std::uint64_t>::digits10 + 3];
            const auto end = std::to_chars(digits, digits + sizeof(digits), value).ptr;
            write(std::string_view(digits, static_cast<std::size_t>(end - digits)));
            return *this;
        }
    }

    VerifyOutput& operator<<(char value) {
        if (size_ == buffer_.size()) flush();
        if (good_) buffer_[size_++] = value;
        return *this;
    }

    VerifyOutput& operator<<(std::string_view value) { write(value); return *this; }
    VerifyOutput& operator<<(const std::string& value) { write(value); return *this; }
    VerifyOutput& operator<<(const char* value) { write(value); return *this; }

    template<class T> requires std::is_floating_point_v<T>
    VerifyOutput& operator<<(T value) {
        char local[128];
        const char* format = fixed_ ? "%.*Lf" : "%.*Lg";
        const int count = std::snprintf(local, sizeof(local), format, precision_,
                                        static_cast<long double>(value));
        if (count < 0) good_ = false;
        else if (static_cast<std::size_t>(count) < sizeof(local))
            write(std::string_view(local, static_cast<std::size_t>(count)));
        else {
            std::vector<char> text(static_cast<std::size_t>(count) + 1);
            std::snprintf(text.data(), text.size(), format, precision_, static_cast<long double>(value));
            write(std::string_view(text.data(), static_cast<std::size_t>(count)));
        }
        return *this;
    }
};

inline VerifyInput verify_in;
inline VerifyOutput verify_out;
#endif

#include <string>
// ACL: atcoder/modint
#ifndef ATCODER_MODINT_HPP
#define ATCODER_MODINT_HPP 1

#include <cassert>
#include <numeric>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// ACL: atcoder/internal_math
#ifndef ATCODER_INTERNAL_MATH_HPP
#define ATCODER_INTERNAL_MATH_HPP 1

#include <utility>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace atcoder {

namespace internal {

// @param m `1 <= m`
// @return x mod m
constexpr long long safe_mod(long long x, long long m) {
    x %= m;
    if (x < 0) x += m;
    return x;
}

// Fast modular multiplication by barrett reduction
// Reference: https://en.wikipedia.org/wiki/Barrett_reduction
// NOTE: reconsider after Ice Lake
struct barrett {
    unsigned int _m;
    unsigned long long im;

    // @param m `1 <= m`
    explicit barrett(unsigned int m) : _m(m), im((unsigned long long)(-1) / m + 1) {}

    // @return m
    unsigned int umod() const { return _m; }

    // @param a `0 <= a < m`
    // @param b `0 <= b < m`
    // @return `a * b % m`
    unsigned int mul(unsigned int a, unsigned int b) const {
        // [1] m = 1
        // a = b = im = 0, so okay

        // [2] m >= 2
        // im = ceil(2^64 / m)
        // -> im * m = 2^64 + r (0 <= r < m)
        // let z = a*b = c*m + d (0 <= c, d < m)
        // a*b * im = (c*m + d) * im = c*(im*m) + d*im = c*2^64 + c*r + d*im
        // c*r + d*im < m * m + m * im < m * m + 2^64 + m <= 2^64 + m * (m + 1) < 2^64 * 2
        // ((ab * im) >> 64) == c or c + 1
        unsigned long long z = a;
        z *= b;
#ifdef _MSC_VER
        unsigned long long x;
        _umul128(z, im, &x);
#else
        unsigned long long x =
            (unsigned long long)(((unsigned __int128)(z)*im) >> 64);
#endif
        unsigned long long y = x * _m;
        return (unsigned int)(z - y + (z < y ? _m : 0));
    }
};

// @param n `0 <= n`
// @param m `1 <= m`
// @return `(x ** n) % m`
constexpr long long pow_mod_constexpr(long long x, long long n, int m) {
    if (m == 1) return 0;
    unsigned int _m = (unsigned int)(m);
    unsigned long long r = 1;
    unsigned long long y = safe_mod(x, m);
    while (n) {
        if (n & 1) r = (r * y) % _m;
        y = (y * y) % _m;
        n >>= 1;
    }
    return r;
}

// Reference:
// M. Forisek and J. Jancina,
// Fast Primality Testing for Integers That Fit into a Machine Word
// @param n `0 <= n`
constexpr bool is_prime_constexpr(int n) {
    if (n <= 1) return false;
    if (n == 2 || n == 7 || n == 61) return true;
    if (n % 2 == 0) return false;
    long long d = n - 1;
    while (d % 2 == 0) d /= 2;
    constexpr long long bases[3] = {2, 7, 61};
    for (long long a : bases) {
        long long t = d;
        long long y = pow_mod_constexpr(a, t, n);
        while (t != n - 1 && y != 1 && y != n - 1) {
            y = y * y % n;
            t <<= 1;
        }
        if (y != n - 1 && t % 2 == 0) {
            return false;
        }
    }
    return true;
}
template <int n> constexpr bool is_prime = is_prime_constexpr(n);

// @param b `1 <= b`
// @return pair(g, x) s.t. g = gcd(a, b), xa = g (mod b), 0 <= x < b/g
constexpr std::pair<long long, long long> inv_gcd(long long a, long long b) {
    a = safe_mod(a, b);
    if (a == 0) return {b, 0};

    // Contracts:
    // [1] s - m0 * a = 0 (mod b)
    // [2] t - m1 * a = 0 (mod b)
    // [3] s * |m1| + t * |m0| <= b
    long long s = b, t = a;
    long long m0 = 0, m1 = 1;

    while (t) {
        long long u = s / t;
        s -= t * u;
        m0 -= m1 * u;  // |m1 * u| <= |m1| * s <= b

        // [3]:
        // (s - t * u) * |m1| + t * |m0 - m1 * u|
        // <= s * |m1| - t * u * |m1| + t * (|m0| + |m1| * u)
        // = s * |m1| + t * |m0| <= b

        auto tmp = s;
        s = t;
        t = tmp;
        tmp = m0;
        m0 = m1;
        m1 = tmp;
    }
    // by [3]: |m0| <= b/g
    // by g != b: |m0| < b/g
    if (m0 < 0) m0 += b / s;
    return {s, m0};
}

// Compile time primitive root
// @param m must be prime
// @return primitive root (and minimum in now)
constexpr int primitive_root_constexpr(int m) {
    if (m == 2) return 1;
    if (m == 167772161) return 3;
    if (m == 469762049) return 3;
    if (m == 754974721) return 11;
    if (m == 998244353) return 3;
    int divs[20] = {};
    divs[0] = 2;
    int cnt = 1;
    int x = (m - 1) / 2;
    while (x % 2 == 0) x /= 2;
    for (int i = 3; (long long)(i)*i <= x; i += 2) {
        if (x % i == 0) {
            divs[cnt++] = i;
            while (x % i == 0) {
                x /= i;
            }
        }
    }
    if (x > 1) {
        divs[cnt++] = x;
    }
    for (int g = 2;; g++) {
        bool ok = true;
        for (int i = 0; i < cnt; i++) {
            if (pow_mod_constexpr(g, (m - 1) / divs[i], m) == 1) {
                ok = false;
                break;
            }
        }
        if (ok) return g;
    }
}
template <int m> constexpr int primitive_root = primitive_root_constexpr(m);

// @param n `n < 2^32`
// @param m `1 <= m < 2^32`
// @return sum_{i=0}^{n-1} floor((ai + b) / m) (mod 2^64)
unsigned long long floor_sum_unsigned(unsigned long long n,
                                      unsigned long long m,
                                      unsigned long long a,
                                      unsigned long long b) {
    unsigned long long ans = 0;
    while (true) {
        if (a >= m) {
            ans += n * (n - 1) / 2 * (a / m);
            a %= m;
        }
        if (b >= m) {
            ans += n * (b / m);
            b %= m;
        }

        unsigned long long y_max = a * n + b;
        if (y_max < m) break;
        // y_max < m * (n + 1)
        // floor(y_max / m) <= n
        n = (unsigned long long)(y_max / m);
        b = (unsigned long long)(y_max % m);
        std::swap(m, a);
    }
    return ans;
}

}  // namespace internal

}  // namespace atcoder

#endif  // ATCODER_INTERNAL_MATH_HPP

// ACL: atcoder/internal_type_traits
#ifndef ATCODER_INTERNAL_TYPE_TRAITS_HPP
#define ATCODER_INTERNAL_TYPE_TRAITS_HPP 1

#include <cassert>
#include <numeric>
#include <type_traits>

namespace atcoder {

namespace internal {

#ifndef _MSC_VER
template <class T>
using is_signed_int128 =
    typename std::conditional<std::is_same<T, __int128_t>::value ||
                                  std::is_same<T, __int128>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using is_unsigned_int128 =
    typename std::conditional<std::is_same<T, __uint128_t>::value ||
                                  std::is_same<T, unsigned __int128>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using make_unsigned_int128 =
    typename std::conditional<std::is_same<T, __int128_t>::value,
                              __uint128_t,
                              unsigned __int128>;

template <class T>
using is_integral = typename std::conditional<std::is_integral<T>::value ||
                                                  is_signed_int128<T>::value ||
                                                  is_unsigned_int128<T>::value,
                                              std::true_type,
                                              std::false_type>::type;

template <class T>
using is_signed_int = typename std::conditional<(is_integral<T>::value &&
                                                 std::is_signed<T>::value) ||
                                                    is_signed_int128<T>::value,
                                                std::true_type,
                                                std::false_type>::type;

template <class T>
using is_unsigned_int =
    typename std::conditional<(is_integral<T>::value &&
                               std::is_unsigned<T>::value) ||
                                  is_unsigned_int128<T>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using to_unsigned = typename std::conditional<
    is_signed_int128<T>::value,
    make_unsigned_int128<T>,
    typename std::conditional<std::is_signed<T>::value,
                              std::make_unsigned<T>,
                              std::common_type<T>>::type>::type;

#else

template <class T> using is_integral = typename std::is_integral<T>;

template <class T>
using is_signed_int =
    typename std::conditional<is_integral<T>::value && std::is_signed<T>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using is_unsigned_int =
    typename std::conditional<is_integral<T>::value &&
                                  std::is_unsigned<T>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using to_unsigned = typename std::conditional<is_signed_int<T>::value,
                                              std::make_unsigned<T>,
                                              std::common_type<T>>::type;

#endif

template <class T>
using is_signed_int_t = std::enable_if_t<is_signed_int<T>::value>;

template <class T>
using is_unsigned_int_t = std::enable_if_t<is_unsigned_int<T>::value>;

template <class T> using to_unsigned_t = typename to_unsigned<T>::type;

}  // namespace internal

}  // namespace atcoder

#endif  // ATCODER_INTERNAL_TYPE_TRAITS_HPP

namespace atcoder {

namespace internal {

struct modint_base {};
struct static_modint_base : modint_base {};

template <class T> using is_modint = std::is_base_of<modint_base, T>;
template <class T> using is_modint_t = std::enable_if_t<is_modint<T>::value>;

}  // namespace internal

template <int m, std::enable_if_t<(1 <= m)>* = nullptr>
struct static_modint : internal::static_modint_base {
    using mint = static_modint;

  public:
    static constexpr int mod() { return m; }
    static mint raw(int v) {
        mint x;
        x._v = v;
        return x;
    }

    static_modint() : _v(0) {}
    template <class T, internal::is_signed_int_t<T>* = nullptr>
    static_modint(T v) {
        long long x = (long long)(v % (long long)(umod()));
        if (x < 0) x += umod();
        _v = (unsigned int)(x);
    }
    template <class T, internal::is_unsigned_int_t<T>* = nullptr>
    static_modint(T v) {
        _v = (unsigned int)(v % umod());
    }

    int val() const { return _v; }

    mint& operator++() {
        _v++;
        if (_v == umod()) _v = 0;
        return *this;
    }
    mint& operator--() {
        if (_v == 0) _v = umod();
        _v--;
        return *this;
    }
    mint operator++(int) {
        mint result = *this;
        ++*this;
        return result;
    }
    mint operator--(int) {
        mint result = *this;
        --*this;
        return result;
    }

    mint& operator+=(const mint& rhs) {
        _v += rhs._v;
        if (_v >= umod()) _v -= umod();
        return *this;
    }
    mint& operator-=(const mint& rhs) {
        _v -= rhs._v;
        if (_v >= umod()) _v += umod();
        return *this;
    }
    mint& operator*=(const mint& rhs) {
        unsigned long long z = _v;
        z *= rhs._v;
        _v = (unsigned int)(z % umod());
        return *this;
    }
    mint& operator/=(const mint& rhs) { return *this = *this * rhs.inv(); }

    mint operator+() const { return *this; }
    mint operator-() const { return mint() - *this; }

    mint pow(long long n) const {
        assert(0 <= n);
        mint x = *this, r = 1;
        while (n) {
            if (n & 1) r *= x;
            x *= x;
            n >>= 1;
        }
        return r;
    }
    mint inv() const {
        if (prime) {
            assert(_v);
            return pow(umod() - 2);
        } else {
            auto eg = internal::inv_gcd(_v, m);
            assert(eg.first == 1);
            return eg.second;
        }
    }

    friend mint operator+(const mint& lhs, const mint& rhs) {
        return mint(lhs) += rhs;
    }
    friend mint operator-(const mint& lhs, const mint& rhs) {
        return mint(lhs) -= rhs;
    }
    friend mint operator*(const mint& lhs, const mint& rhs) {
        return mint(lhs) *= rhs;
    }
    friend mint operator/(const mint& lhs, const mint& rhs) {
        return mint(lhs) /= rhs;
    }
    friend bool operator==(const mint& lhs, const mint& rhs) {
        return lhs._v == rhs._v;
    }
    friend bool operator!=(const mint& lhs, const mint& rhs) {
        return lhs._v != rhs._v;
    }

  private:
    unsigned int _v;
    static constexpr unsigned int umod() { return m; }
    static constexpr bool prime = internal::is_prime<m>;
};

template <int id> struct dynamic_modint : internal::modint_base {
    using mint = dynamic_modint;

  public:
    static int mod() { return (int)(bt.umod()); }
    static void set_mod(int m) {
        assert(1 <= m);
        bt = internal::barrett(m);
    }
    static mint raw(int v) {
        mint x;
        x._v = v;
        return x;
    }

    dynamic_modint() : _v(0) {}
    template <class T, internal::is_signed_int_t<T>* = nullptr>
    dynamic_modint(T v) {
        long long x = (long long)(v % (long long)(mod()));
        if (x < 0) x += mod();
        _v = (unsigned int)(x);
    }
    template <class T, internal::is_unsigned_int_t<T>* = nullptr>
    dynamic_modint(T v) {
        _v = (unsigned int)(v % mod());
    }

    int val() const { return _v; }

    mint& operator++() {
        _v++;
        if (_v == umod()) _v = 0;
        return *this;
    }
    mint& operator--() {
        if (_v == 0) _v = umod();
        _v--;
        return *this;
    }
    mint operator++(int) {
        mint result = *this;
        ++*this;
        return result;
    }
    mint operator--(int) {
        mint result = *this;
        --*this;
        return result;
    }

    mint& operator+=(const mint& rhs) {
        _v += rhs._v;
        if (_v >= umod()) _v -= umod();
        return *this;
    }
    mint& operator-=(const mint& rhs) {
        _v += mod() - rhs._v;
        if (_v >= umod()) _v -= umod();
        return *this;
    }
    mint& operator*=(const mint& rhs) {
        _v = bt.mul(_v, rhs._v);
        return *this;
    }
    mint& operator/=(const mint& rhs) { return *this = *this * rhs.inv(); }

    mint operator+() const { return *this; }
    mint operator-() const { return mint() - *this; }

    mint pow(long long n) const {
        assert(0 <= n);
        mint x = *this, r = 1;
        while (n) {
            if (n & 1) r *= x;
            x *= x;
            n >>= 1;
        }
        return r;
    }
    mint inv() const {
        auto eg = internal::inv_gcd(_v, mod());
        assert(eg.first == 1);
        return eg.second;
    }

    friend mint operator+(const mint& lhs, const mint& rhs) {
        return mint(lhs) += rhs;
    }
    friend mint operator-(const mint& lhs, const mint& rhs) {
        return mint(lhs) -= rhs;
    }
    friend mint operator*(const mint& lhs, const mint& rhs) {
        return mint(lhs) *= rhs;
    }
    friend mint operator/(const mint& lhs, const mint& rhs) {
        return mint(lhs) /= rhs;
    }
    friend bool operator==(const mint& lhs, const mint& rhs) {
        return lhs._v == rhs._v;
    }
    friend bool operator!=(const mint& lhs, const mint& rhs) {
        return lhs._v != rhs._v;
    }

  private:
    unsigned int _v;
    static internal::barrett bt;
    static unsigned int umod() { return bt.umod(); }
};
template <int id> internal::barrett dynamic_modint<id>::bt(998244353);

using modint998244353 = static_modint<998244353>;
using modint1000000007 = static_modint<1000000007>;
using modint = dynamic_modint<-1>;

namespace internal {

template <class T>
using is_static_modint = std::is_base_of<internal::static_modint_base, T>;

template <class T>
using is_static_modint_t = std::enable_if_t<is_static_modint<T>::value>;

template <class> struct is_dynamic_modint : public std::false_type {};
template <int id>
struct is_dynamic_modint<dynamic_modint<id>> : public std::true_type {};

template <class T>
using is_dynamic_modint_t = std::enable_if_t<is_dynamic_modint<T>::value>;

}  // namespace internal

}  // namespace atcoder

#endif  // ATCODER_MODINT_HPP

int main() {
    int n{};std::string s;verify_in>>n>>s;
    using Mint=atcoder::static_modint<3>;PrimeBinomial<Mint> comb(2);Mint answer=0;
    const std::string colors="BWR";
    for(int i=0;i<n;++i) answer+=comb.Lucas(n-1,i)*static_cast<int>(colors.find(s[i]));
    if((n-1)%2) answer=-answer;
    verify_out<<colors[answer.val()]<<'\n';
}
