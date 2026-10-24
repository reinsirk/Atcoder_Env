// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/binomial_coefficient.md
//
// API一覧
// [BinomialOptions]
// 事前計算と、積による代替計算の上限。
// table_limit: 各テーブルの最大添字（既定5'100'000）。0ならmax_n=0で構築する。
// max_multiplicative_steps: 1照会の積の回数上限（既定1'000'000）。超過はlength_error。
//
// [BinomialMod]
// 法1〜UINT32_MAXの二項係数を整数で返す。n,kはint64_t、合成数の法にも対応。
// BinomialMod(modulus=998244353,max_n=5'000'000,options={}): 法・前計算範囲・上限を指定。範囲外の法はinvalid_argument。
// BinomialMod(modulus,options): 同じ設定で500万まで前計算する。
// mod(): 使用中の法を返す。is_prime_modulus(): 素数の法ならtrue（法1はfalse）。
// prepare(max_n=5'000'000): 上限内で表を準備。照会時の自動拡張も可。上限超過はlength_error、負数はinvalid_argument。
// COM(n,k): nCkの剰余。n<0,k<0,k>nと法1は0。素数でn<modかつ準備済みならO(1)。
// Lucas(n,k): 素数の法でCOMと同じ結果。合成数・法1ではdomain_error（COMを使う）。
// H(n,k): (n+k-1)Ckの剰余。0H0=1（法1は0）、0Hk=0（k>0）。int64_t超過はoverflow_error。
// 表外は積で計算し、積の回数上限も超えるとlength_error。SKIP_RUNTIME_CHECKSで入力検査を除く。
//
// [ModIntBinomial<Mint>]
// AtCoder modint用。Mintは利用側で用意し、動的な法を変えたら計算器も作り直す。
// ModIntBinomial(max_n=5'000'000,options={}): Mint::mod()を法として構築・前計算。合成数にも対応。
// ModIntBinomial(options): 指定した上限で500万まで前計算する。
// prepare(max_n=5'000'000): 上限内で表を準備。制限・例外はBinomialModと同じ。
// COM(n,k): nCkをMintで返す。n<0,k<0,k>nと法1は0。素数でn<modかつ準備済みならO(1)。
// Lucas(n,k): 素数の法でCOMと同じ結果。合成数・法1ではCOMを使う。
// H(n,k): nHkをMintで返す。0H0=1（法1は0）、0Hk=0（k>0）。int64_t超過はoverflow_error。

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#ifndef SIRKIT_PRIMALITY32_DEFINED
// 内部の素数判定。定義はファイル末尾。
namespace reinsirk::detail {
inline bool is_prime_u32(std::uint32_t n) noexcept;
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_BINOMIAL_OPTIONS_DEFINED
#define SIRKIT_BINOMIAL_OPTIONS_DEFINED
struct BinomialOptions
{
    std::uint32_t table_limit = 5'100'000;

    std::uint64_t max_multiplicative_steps = 1'000'000;
};
#endif

#ifndef SIRKIT_BINOMIAL_MOD_DEFINED
#define SIRKIT_BINOMIAL_MOD_DEFINED
// 理論参考: A. Granville, Arithmetic properties of Binomial Coefficients.
// https://dms.umontreal.ca/~andrew/Binomial/intro.html
class BinomialMod
{
public:
    using value_type = std::uint32_t;
    using index_type = std::int64_t;

private:
    using u64 = std::uint64_t;
    using i64 = std::int64_t;

    struct PrimePower
    {
        value_type prime;
        value_type modulus;
        unsigned exponent;
        value_type crt_weight = 0;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        u64 reciprocal;
        u64 prime_multiplier;
        unsigned prime_shift;
        // CRT用逆数の下位・上位を、最初の2因子に分けて保持する。
        value_type crt_reciprocal_part = 0;
#endif
        std::vector<value_type> fac{1};
        std::vector<value_type> ifac{1};
        std::vector<value_type> prefix{1};

        // 素数p、その冪m=p^e、指数eを保持する。
        PrimePower(value_type p, value_type m, unsigned e)
            : prime(p), modulus(m), exponent(e)
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
              , reciprocal(std::numeric_limits<u64>::max() / m + 1),
                prime_multiplier(0), prime_shift(0)
#endif
        {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
            prime_shift = std::bit_width(p) - 1 - (std::has_single_bit(p) ? 1 : 0);
            const __uint128_t scale = __uint128_t{1} << (64 + prime_shift);
            prime_multiplier = static_cast<u64>((scale + p - 1) / p);
#endif
        }
    };

    value_type modulus_;
    BinomialOptions options_;
    std::vector<PrimePower> factors_;

    // 0<=a,b<modulusの積。Barrett還元の128ビット積はGCC/Clang拡張。
    static value_type multiply(value_type a, value_type b, const PrimePower &factor) noexcept
    {
        const u64 product = static_cast<u64>(a) * b;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        const u64 quotient = static_cast<u64>((static_cast<__uint128_t>(product) * factor.reciprocal) >> 64);
        const u64 correction = quotient * factor.modulus;
        return static_cast<value_type>(product - correction + (product < correction ? factor.modulus : 0));
#else
        return static_cast<value_type>(product % factor.modulus);
#endif
    }

    // n<=INT64_MAXを素数冪で還元する。近似商の過剰分は高々1。
    static value_type reduce(u64 n, const PrimePower &factor) noexcept
    {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        const u64 quotient = static_cast<u64>((static_cast<__uint128_t>(n) * factor.reciprocal) >> 64);
        const u64 product = quotient * factor.modulus;
        return static_cast<value_type>(n - product + (n < product ? factor.modulus : 0));
#else
        return static_cast<value_type>(n % factor.modulus);
#endif
    }

    // n<=INT64_MAXの固定除数による商。入力幅の余裕を使い補正を省く。
    static u64 divide_prime(u64 n, const PrimePower &factor) noexcept
    {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        return static_cast<u64>((static_cast<__uint128_t>(n) * factor.prime_multiplier) >> 64)
               >> factor.prime_shift;
#else
        return n / factor.prime;
#endif
    }

    // 二分累乗で a^e mod m を求める。
    static value_type power(value_type a, u64 e, const PrimePower &factor) noexcept
    {
        value_type result = 1;
        while (e)
        {
            if (e & 1)
                result = multiply(result, a, factor);
            a = multiply(a, a, factor);
            e >>= 1;
        }
        return result;
    }

    // 0<=v<eかつp^e<=UINT32_MAX。中間積も法未満なので還元は不要。
    static value_type valuation_power(value_type prime, u64 valuation) noexcept
    {
        value_type result = 1;
        while (valuation--)
            result = static_cast<value_type>(static_cast<u64>(result) * prime);
        return result;
    }

    // 奇数の逆元をmod2^32で求める。初期値は5bit正しく、3回で全幅に達する。
    static value_type inverse_binary(value_type value) noexcept
    {
        value_type result = static_cast<value_type>(u64{3} * value) ^ value_type{2};
        for (unsigned i = 0; i < 3; ++i)
        {
            const auto correction = static_cast<value_type>(u64{2} - static_cast<u64>(value) * result);
            result = static_cast<value_type>(static_cast<u64>(result) * correction);
        }
        return result;
    }

    // 互いに素なa,mについて拡張ユークリッド互除法で逆元を求める。
    static value_type inverse(value_type a, value_type m)
    {
        i64 r0 = a, r1 = m, x0 = 1, x1 = 0;
        while (r1)
        {
            const i64 q = r0 / r1;
            const i64 next_r = r0 - q * r1;
            const i64 next_x = x0 - q * x1;
            r0 = r1;
            r1 = next_r;
            x0 = x1;
            x1 = next_x;
        }

        x0 %= static_cast<i64>(m);
        if (x0 < 0)
            x0 += m;
        return static_cast<value_type>(x0);
    }

    // 法を素数冪に分解し、CRTの係数を準備する。O(sqrt(mod))。
    void factorize()
    {
        if (reinsirk::detail::is_prime_u32(modulus_))
        {
            factors_.emplace_back(modulus_, modulus_, 1);
            return;
        }
        if (modulus_ < 2) return;
        value_type remaining = modulus_;
        constexpr value_type primality_threshold = value_type{1} << 25;
        // 3・5は型に定数を持たせ、定数除算を保つ。
        const auto remove_factor = [&](auto divisor) {
            const value_type p = divisor;
            if (remaining % p != 0) return;
            u64 prime_power = 1;
            unsigned exponent = 0;
            do {
                remaining /= p;
                prime_power *= p;
                ++exponent;
            } while (remaining % p == 0);
            factors_.emplace_back(p, static_cast<value_type>(prime_power), exponent);
            if (remaining > primality_threshold && u64{p} * p < remaining &&
                reinsirk::detail::is_prime_u32(static_cast<value_type>(remaining))) {
                factors_.emplace_back(remaining, remaining, 1);
                remaining = 1;
            }
        };
        // 2の因子は末尾0ビット数で取り除く。
        if ((remaining & 1) == 0) {
            const auto exponent = static_cast<unsigned>(std::countr_zero(static_cast<value_type>(remaining)));
            const auto power = static_cast<value_type>(u64{1} << exponent);
            remaining >>= exponent;
            factors_.emplace_back(2, power, exponent);
            if (remaining > primality_threshold && reinsirk::detail::is_prime_u32(static_cast<value_type>(remaining))) {
                factors_.emplace_back(remaining, remaining, 1);
                remaining = 1;
            }
        }
        remove_factor(std::integral_constant<value_type, 3>{});
        remove_factor(std::integral_constant<value_type, 5>{});
        // 30と互いに素な候補を7,11,13,...の順に調べる。
        static constexpr unsigned steps[8] = {4, 2, 4, 2, 4, 6, 2, 6};
        unsigned step = 0;
        for (value_type p = 7; u64{p} * p <= remaining; p += steps[step], step = (step + 1) & 7) {
            if (remaining % p != 0) continue;
            u64 prime_power = 1;
            unsigned exponent = 0;
            do {
                remaining /= p;
                prime_power *= p;
                ++exponent;
            } while (remaining % p == 0);
            factors_.emplace_back(p, static_cast<value_type>(prime_power), exponent);
            if (remaining > primality_threshold && u64{p} * p < remaining &&
                reinsirk::detail::is_prime_u32(static_cast<value_type>(remaining))) {
                factors_.emplace_back(remaining, remaining, 1);
                remaining = 1;
                break;
            }
        }
        if (remaining > 1)
            factors_.emplace_back(remaining, remaining, 1);
        for (auto &factor : factors_)
        {
            const auto other = modulus_ / factor.modulus;
            const auto inv = inverse(other % factor.modulus, factor.modulus);
            factor.crt_weight = static_cast<value_type>(static_cast<u64>(other) * inv % modulus_);
        }
    }

    // 必要な添字まで、上限内でテーブルを概ね倍増させる。
    static value_type growth_target(value_type current, value_type required,
                                    value_type limit) noexcept
    {
        const u64 doubled = std::max<u64>(1, static_cast<u64>(current) * 2);
        return static_cast<value_type>(std::min<u64>(limit, std::max<u64>(required, doubled)));
    }

    // 大きい素数表を追加する。確保は呼び出し元で済ませる。
    static void extend_factorials_large(PrimePower &factor, std::size_t old_size, value_type target)
    {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        u64 i = old_size;
        // 2項積の差分は4i+6。法未満なら加減算1回で更新できる。
        if (static_cast<u64>(target) * 4 + 6 < factor.prime)
        {
            u64 pair = multiply(static_cast<value_type>(i), static_cast<value_type>(i + 1), factor);
            for (; i + 3 <= target; i += 4)
            {
                const auto previous = factor.fac[static_cast<std::size_t>(i - 1)];
                const auto p2 = static_cast<value_type>(pair);
                u64 next_pair = pair + 4 * i + 6;
                next_pair -= next_pair >= factor.prime ? factor.prime : 0;
                const auto q2 = static_cast<value_type>(next_pair);
                pair = next_pair + 4 * i + 14;
                pair -= pair >= factor.prime ? factor.prime : 0;
                const auto p3 = multiply(p2, static_cast<value_type>(i + 2), factor);
                const auto p4 = multiply(p2, q2, factor);
                factor.fac[static_cast<std::size_t>(i)] = multiply(previous, static_cast<value_type>(i), factor);
                factor.fac[static_cast<std::size_t>(i + 1)] = multiply(previous, p2, factor);
                factor.fac[static_cast<std::size_t>(i + 2)] = multiply(previous, p3, factor);
                factor.fac[static_cast<std::size_t>(i + 3)] = multiply(previous, p4, factor);
            }
            for (; i <= target; ++i)
                factor.fac[static_cast<std::size_t>(i)] = multiply(factor.fac[static_cast<std::size_t>(i - 1)], static_cast<value_type>(i), factor);
        }
        else
        {
            // 2項ずつ先頭値から求め、剰余乗算の依存を短くする。
            for (; i + 1 <= target; i += 2)
            {
                const auto previous = factor.fac[static_cast<std::size_t>(i - 1)];
                const auto pair = multiply(static_cast<value_type>(i), static_cast<value_type>(i + 1), factor);
                factor.fac[static_cast<std::size_t>(i)] = multiply(previous, static_cast<value_type>(i), factor);
                factor.fac[static_cast<std::size_t>(i + 1)] = multiply(previous, pair, factor);
            }
            if (i <= target)
                factor.fac[static_cast<std::size_t>(i)] = multiply(factor.fac[static_cast<std::size_t>(i - 1)], static_cast<value_type>(i), factor);
        }
#else
        for (u64 i = old_size; i <= target; ++i)
        {
            factor.fac[static_cast<std::size_t>(i)] =
                multiply(factor.fac[static_cast<std::size_t>(i - 1)],
                         static_cast<value_type>(i), factor);
        }
#endif
        // Wilsonの定理: ifac[k]=(-1)^(k+1)*fac[p-1-k]。小さい表は従来経路。
        if (target == factor.prime - 1)
        {
            u64 i = old_size;
            if (i & 1)
            {
                factor.ifac[static_cast<std::size_t>(i)] = factor.fac[static_cast<std::size_t>(target - i)];
                ++i;
            }
            for (; i < target; i += 2)
            {
                factor.ifac[static_cast<std::size_t>(i)] = factor.prime - factor.fac[static_cast<std::size_t>(target - i)];
                factor.ifac[static_cast<std::size_t>(i + 1)] = factor.fac[static_cast<std::size_t>(target - i - 1)];
            }
            if (i <= target)
                factor.ifac[static_cast<std::size_t>(i)] = factor.prime - factor.fac[static_cast<std::size_t>(target - i)];
            return;
        }
        factor.ifac[target] = power(factor.fac[target], factor.prime - 2, factor);
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        i = target;
        if (static_cast<u64>(target) * 4 + 6 < factor.prime)
        {
            u64 pair = multiply(static_cast<value_type>(i), static_cast<value_type>(i - 1), factor);
            for (; i > old_size + 3; i -= 4)
            {
                const auto previous = factor.ifac[static_cast<std::size_t>(i)];
                const auto p2 = static_cast<value_type>(pair);
                const u64 delta = 4 * i - 6;
                const u64 next_pair = pair + (pair < delta ? factor.prime : 0) - delta;
                const auto q2 = static_cast<value_type>(next_pair);
                const u64 next_delta = 4 * i - 14;
                pair = next_pair + (next_pair < next_delta ? factor.prime : 0) - next_delta;
                const auto p3 = multiply(p2, static_cast<value_type>(i - 2), factor);
                const auto p4 = multiply(p2, q2, factor);
                factor.ifac[static_cast<std::size_t>(i - 1)] = multiply(previous, static_cast<value_type>(i), factor);
                factor.ifac[static_cast<std::size_t>(i - 2)] = multiply(previous, p2, factor);
                factor.ifac[static_cast<std::size_t>(i - 3)] = multiply(previous, p3, factor);
                factor.ifac[static_cast<std::size_t>(i - 4)] = multiply(previous, p4, factor);
            }
            for (; i > old_size; --i)
                factor.ifac[static_cast<std::size_t>(i - 1)] = multiply(factor.ifac[static_cast<std::size_t>(i)], static_cast<value_type>(i), factor);
        }
        else
        {
            for (; i > old_size + 1; i -= 2)
            {
                const auto previous = factor.ifac[static_cast<std::size_t>(i)];
                const auto pair = multiply(static_cast<value_type>(i), static_cast<value_type>(i - 1), factor);
                factor.ifac[static_cast<std::size_t>(i - 1)] = multiply(previous, static_cast<value_type>(i), factor);
                factor.ifac[static_cast<std::size_t>(i - 2)] = multiply(previous, pair, factor);
            }
            if (i > old_size)
                factor.ifac[static_cast<std::size_t>(i - 1)] = multiply(factor.ifac[static_cast<std::size_t>(i)], static_cast<value_type>(i), factor);
        }
#else
        for (u64 i = target; i > old_size; --i)
        {
            factor.ifac[static_cast<std::size_t>(i - 1)] =
                multiply(factor.ifac[static_cast<std::size_t>(i)],
                         static_cast<value_type>(i), factor);
        }
#endif
    }

    // 素数用の階乗・逆階乗を拡張する。逆元計算は高々1回、確保失敗時はサイズを戻す。
    void ensure_factorials(PrimePower &factor, value_type required)
    {
        const auto current = static_cast<value_type>(factor.fac.size() - 1);
        if (required <= current)
            return;
        const auto limit = std::min(options_.table_limit, factor.prime - 1);
        const auto target = growth_target(current, required, limit);
        const auto old_size = factor.fac.size();
        if (static_cast<u64>(target) + 1 > std::numeric_limits<std::size_t>::max())
        {
            throw std::length_error("BinomialMod: table size exceeds size_t");
        }
        const auto new_size = static_cast<std::size_t>(static_cast<u64>(target) + 1);
        try
        {
            factor.fac.resize(new_size);
            factor.ifac.resize(new_size);
        }
        catch (...)
        {
            factor.fac.resize(old_size);
            factor.ifac.resize(old_size);
            throw;
        }
        if (target >= 4096)
        {
            extend_factorials_large(factor, old_size, target);
            return;
        }
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        // 2項ずつ先頭値から求め、剰余乗算の依存を短くする。
        u64 i = old_size;
        for (; i + 1 <= target; i += 2)
        {
            const auto previous = factor.fac[static_cast<std::size_t>(i - 1)];
            const auto pair = multiply(static_cast<value_type>(i), static_cast<value_type>(i + 1), factor);
            factor.fac[static_cast<std::size_t>(i)] = multiply(previous, static_cast<value_type>(i), factor);
            factor.fac[static_cast<std::size_t>(i + 1)] = multiply(previous, pair, factor);
        }
        if (i <= target)
            factor.fac[static_cast<std::size_t>(i)] = multiply(factor.fac[static_cast<std::size_t>(i - 1)], static_cast<value_type>(i), factor);
#else
        for (u64 i = old_size; i <= target; ++i)
        {
            factor.fac[static_cast<std::size_t>(i)] =
                multiply(factor.fac[static_cast<std::size_t>(i - 1)],
                         static_cast<value_type>(i), factor);
        }
#endif
        factor.ifac[target] = power(factor.fac[target], factor.prime - 2, factor);
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        i = target;
        for (; i > old_size + 1; i -= 2)
        {
            const auto previous = factor.ifac[static_cast<std::size_t>(i)];
            const auto pair = multiply(static_cast<value_type>(i), static_cast<value_type>(i - 1), factor);
            factor.ifac[static_cast<std::size_t>(i - 1)] = multiply(previous, static_cast<value_type>(i), factor);
            factor.ifac[static_cast<std::size_t>(i - 2)] = multiply(previous, pair, factor);
        }
        if (i > old_size)
            factor.ifac[static_cast<std::size_t>(i - 1)] = multiply(factor.ifac[static_cast<std::size_t>(i)], static_cast<value_type>(i), factor);
#else
        for (u64 i = target; i > old_size; --i)
        {
            factor.ifac[static_cast<std::size_t>(i - 1)] =
                multiply(factor.ifac[static_cast<std::size_t>(i)],
                         static_cast<value_type>(i), factor);
        }
#endif
    }

    // pを除いた累積積と逆累積積を拡張する。逆元は末尾で1回だけ求める。
    void ensure_prefix(PrimePower &factor, value_type required)
    {
        const auto current = static_cast<value_type>(factor.prefix.size() - 1);
        if (required <= current)
            return;
        const auto limit = std::min(options_.table_limit, factor.modulus - 1);
        const auto target = growth_target(current, required, limit);
        if (static_cast<u64>(target) + 1 > std::numeric_limits<std::size_t>::max())
            throw std::length_error("BinomialMod: table size exceeds size_t");
        const auto old_size = factor.prefix.size();
        const auto new_size = static_cast<std::size_t>(static_cast<u64>(target) + 1);
        try
        {
            factor.prefix.resize(new_size);
            factor.ifac.resize(new_size);
        }
        catch (...)
        {
            factor.prefix.resize(old_size);
            factor.ifac.resize(old_size);
            throw;
        }
        if (factor.prime == 2)
        {
            const u64 mask = factor.modulus - 1;
            for (u64 i = old_size; i <= target; ++i)
            {
                const auto previous = factor.prefix[static_cast<std::size_t>(i - 1)];
                factor.prefix[static_cast<std::size_t>(i)] = (i & 1)
                    ? static_cast<value_type>((static_cast<u64>(previous) * i) & mask) : previous;
            }
            factor.ifac[target] = inverse_binary(factor.prefix[target]) & static_cast<value_type>(mask);
            for (u64 i = target; i > current; --i)
            {
                const auto value = factor.ifac[static_cast<std::size_t>(i)];
                factor.ifac[static_cast<std::size_t>(i - 1)] = (i & 1)
                    ? static_cast<value_type>((static_cast<u64>(value) * i) & mask) : value;
            }
            return;
        }
        value_type residue = static_cast<value_type>(old_size % factor.prime);
        for (u64 i = old_size; i <= target; ++i)
        {
            const auto previous = factor.prefix[static_cast<std::size_t>(i - 1)];
            factor.prefix[static_cast<std::size_t>(i)] = residue
                ? multiply(previous, static_cast<value_type>(i), factor) : previous;
            if (++residue == factor.prime)
                residue = 0;
        }
        factor.ifac[target] = inverse(factor.prefix[target], factor.modulus);
        residue = target % factor.prime;
        for (u64 i = target; i > current; --i)
        {
            const auto value = factor.ifac[static_cast<std::size_t>(i)];
            factor.ifac[static_cast<std::size_t>(i - 1)] = residue
                ? multiply(value, static_cast<value_type>(i), factor) : value;
            if (residue == 0)
                residue = factor.prime;
            --residue;
        }
    }

    // 積の回数を残予算から差し引く。足りなければ length_error。
    static void consume_steps(u64 count, u64 &remaining)
    {
        if (count > remaining)
        {
            throw std::length_error("BinomialMod: multiplicative step limit exceeded; "
                                    "increase table_limit or max_multiplicative_steps");
        }
        remaining -= count;
    }

    // n<p の nCk。テーブル内はO(1)、上限外は小さい側のkを使って積で求める。
    template<bool Charge = true>
    value_type prime_digit(PrimePower &factor, value_type n, value_type k, [[maybe_unused]] u64 &budget)
    {
        k = std::min(k, n - k);
        if (k == 0)
            return 1;
        if (k == 1)
            return n;
        const bool table_eligible = n <= options_.table_limit;
        if constexpr (Charge)
            if (!table_eligible) consume_steps(k, budget);
        if (k == 2)
        {
            // nは32bitなのでn(n-1)はuint64_tに収まり、半分はINT64_MAX以下。
            const u64 product = (static_cast<u64>(n) * (n - 1u)) >> 1;
            return reduce(product, factor);
        }
        if (table_eligible)
        {
            ensure_factorials(factor, n);
            return multiply(multiply(factor.fac[n], factor.ifac[k], factor),
                            factor.ifac[n - k], factor);
        }
        value_type numerator = 1, denominator = 1;
        for (u64 i = 1; i <= k; ++i)
        {
            numerator = multiply(numerator, static_cast<value_type>(n - k + i), factor);
            denominator = multiply(denominator, static_cast<value_type>(i), factor);
        }
        return multiply(numerator, inverse(denominator, factor.prime), factor);
    }

    // 準備済みの小素数では、定数除算で桁を読みながら積を求める。
    template<value_type P>
    static value_type lucas_prepared(u64 n, u64 k, value_type ni, value_type ki,
                                     value_type next_n = 0, value_type next_k = 0) noexcept
    {
        static constexpr auto table = [] {
            struct Table { std::uint8_t value[P * P]{}; };
            Table t;
            for (value_type i = 0; i < P; ++i) {
                t.value[i * P] = 1;
                for (value_type j = 1; j <= i; ++j)
                    t.value[i * P + j] = static_cast<std::uint8_t>((t.value[(i-1)*P + j-1] + t.value[(i-1)*P + j]) % P);
            }
            return t;
        }();
        // int64_tの桁数と最上位桁の上限から、積全体がuint64_tに収まる。
        static_assert([] {
            u64 top = std::numeric_limits<i64>::max(), bound = 1;
            while (top >= P)
            {
                if (bound > std::numeric_limits<u64>::max() / (P - 1)) return false;
                bound *= P - 1;
                top /= P;
            }
            value_type leading = 1;
            for (value_type n = 0; n <= top; ++n)
                for (value_type k = 0; k <= n; ++k)
                    leading = std::max<value_type>(leading, table.value[n * P + k]);
            return bound <= std::numeric_limits<u64>::max() / leading;
        }());
        u64 result = static_cast<u64>(table.value[ni * P + ki]) * table.value[next_n * P + next_k];
        while (k)
        {
            const auto a = static_cast<value_type>(n % P);
            const auto b = static_cast<value_type>(k % P);
            if (b > a) return 0;
            result *= table.value[a * P + b];
            n /= P;
            k /= P;
        }
        return static_cast<value_type>(result % P);
    }

    // 他の法は全桁の0判定後に、準備済みの表か積を使う。
    template<unsigned Capacity>
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
#if defined(__GNUC__) && !defined(__clang__) && defined(__SIZEOF_INT128__)
    [[gnu::aligned(64)]]
#endif
    value_type lucas_digits(PrimePower &factor, u64 n, u64 k, u64 &budget,
                             u64 first)
    {
        struct Digit { value_type n, k; };
        Digit digits[Capacity];
        digits[0] = {static_cast<value_type>(first), static_cast<value_type>(first >> 32)};
        unsigned count = 1;
        u64 steps = 0;
        // 予算の費用は既存の桁走査へ組み込む。40桁×32bit以内で和は溢れない。
        auto add_steps = [&](value_type ni, value_type ki) {
            if (ni > options_.table_limit)
            {
                const auto cost = std::min(ki, ni - ki);
                if (cost >= 2) steps += cost;
            }
        };
        add_steps(digits[0].n, digits[0].k);
        while (k)
        {
            const auto nq = divide_prime(n, factor), kq = divide_prime(k, factor);
            const auto ni = static_cast<value_type>(n - nq * factor.prime);
            const auto ki = static_cast<value_type>(k - kq * factor.prime);
            if (ki > ni) return 0;
            digits[count++] = {ni, ki};
            add_steps(ni, ki);
            n = nq;
            k = kq;
        }
        // 数学的な0を全桁で判定してから、必要な予算を一度だけ差し引く。
        consume_steps(steps, budget);
        value_type result = 1;
        if (factor.fac.size() == factor.prime)
        {
#if defined(__GNUC__) && !defined(__clang__) && defined(__SIZEOF_INT128__)
            const auto first_n = digits[0].n, first_k = digits[0].k;
            result = multiply(multiply(factor.fac[first_n], factor.ifac[first_k], factor),
                              factor.ifac[first_n - first_k], factor);
            for (unsigned i = 1; i < count; ++i)
#else
            for (unsigned i = 0; i < count; ++i)
#endif
            {
                const auto n = digits[i].n, k = digits[i].k;
                const auto digit = multiply(multiply(factor.fac[n], factor.ifac[k], factor),
                                            factor.ifac[n-k], factor);
                result = multiply(result, digit, factor);
            }
        }
        else
        {
#if defined(__GNUC__) && !defined(__clang__) && defined(__SIZEOF_INT128__)
            result = prime_digit<false>(factor, digits[0].n, digits[0].k, steps);
            for (unsigned i = 1; i < count; ++i)
#else
            for (unsigned i = 0; i < count; ++i)
#endif
                result = multiply(result, prime_digit<false>(factor, digits[i].n, digits[i].k, steps), factor);
        }
        return result;
    }

    // 最初の桁で0になる照会は配列・表の分岐を使わず返す。
#if defined(__GNUC__) && !defined(__clang__) && defined(__SIZEOF_INT128__)
    [[gnu::aligned(32)]]
#endif
    value_type lucas_prime(PrimePower &factor, u64 n, u64 k, u64 &budget)
    {
        if (factor.prime == 2) return (k & ~n) == 0;
        if (k == 0) return 1;
        const auto nq = divide_prime(n, factor), kq = divide_prime(k, factor);
        const auto ni = static_cast<value_type>(n - nq * factor.prime);
        const auto ki = static_cast<value_type>(k - kq * factor.prime);
        if (ki > ni) return 0;
        if (factor.prime <= 31 && factor.fac.size() == factor.prime)
        {
            u64 rest_n = nq, rest_k = kq;
            value_type next_n = 0, next_k = 0;
            if (kq)
            {
                rest_n = divide_prime(nq, factor);
                rest_k = divide_prime(kq, factor);
                next_n = static_cast<value_type>(nq - rest_n * factor.prime);
                next_k = static_cast<value_type>(kq - rest_k * factor.prime);
                if (next_k > next_n) return 0;
            }
            switch (factor.prime)
            {
            case 3: return lucas_prepared<3>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 5: return lucas_prepared<5>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 7: return lucas_prepared<7>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 11: return lucas_prepared<11>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 13: return lucas_prepared<13>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 17: return lucas_prepared<17>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 19: return lucas_prepared<19>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 23: return lucas_prepared<23>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 29: return lucas_prepared<29>(rest_n, rest_k, ni, ki, next_n, next_k);
            case 31: return lucas_prepared<31>(rest_n, rest_k, ni, ki, next_n, next_k);
            default: break;
            }
        }
        // 先頭2値を1引数にまとめ、桁処理へ渡す引数を少なくする。
        const u64 first = static_cast<u64>(ni) | (static_cast<u64>(ki) << 32);
        // p>=37なら13桁、その他の奇素数でも40桁以内。
        if (factor.prime > 31)
            return lucas_digits<13>(factor, nq, kq, budget, first);
        return lucas_digits<40>(factor, nq, kq, budget, first);
    }

    // Kummerの繰り上がりを数え、素数冪で0になる時点で打ち切る。
    static u64 binomial_valuation(u64 n, u64 k, const PrimePower &factor) noexcept
    {
        u64 r = n - k;
        if (factor.prime == 2)
            // k+r=nの各桁の繰り上がりを、1回のpopcountで数える。
            return static_cast<u64>(std::popcount((k & r) | ((k | r) & ~n)));
        u64 result = 0;
        while (n)
        {
            n = divide_prime(n, factor);
            k = divide_prime(k, factor);
            r = divide_prime(r, factor);
            result += n - k - r;
            if (result >= factor.exponent) break;
        }
        return result;
    }

    // 2の冪の直接積。予算確認後に呼び、参照や例外を持ち込まない。
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
    static value_type binary_product(u64 n, u64 k, u64 valuation, value_type mask) noexcept
    {
        if (k == 2)
        {
            // 法は高々2^31。nの下位32bitからnC2の必要な下位桁を得る。
            const u64 low = static_cast<value_type>(n);
            return static_cast<value_type>((low * (low - 1)) >> 1) & mask;
        }
        value_type numerator0 = 1, numerator1 = 1, denominator0 = 1, denominator1 = 1;
        u64 i = 1;
        for (; i < k; i += 2)
        {
            u64 a = n - k + i, next_a = a + 1, b = i, next_b = i + 1;
            a >>= std::countr_zero(a);
            next_a >>= std::countr_zero(next_a);
            b >>= std::countr_zero(b);
            next_b >>= std::countr_zero(next_b);
            numerator0 = static_cast<value_type>(static_cast<u64>(numerator0) * static_cast<value_type>(a));
            numerator1 = static_cast<value_type>(static_cast<u64>(numerator1) * static_cast<value_type>(next_a));
            denominator0 = static_cast<value_type>(static_cast<u64>(denominator0) * static_cast<value_type>(b));
            denominator1 = static_cast<value_type>(static_cast<u64>(denominator1) * static_cast<value_type>(next_b));
        }
        if (i <= k)
        {
            u64 a = n - k + i, b = i;
            a >>= std::countr_zero(a);
            b >>= std::countr_zero(b);
            numerator0 = static_cast<value_type>(static_cast<u64>(numerator0) * static_cast<value_type>(a));
            denominator0 = static_cast<value_type>(static_cast<u64>(denominator0) * static_cast<value_type>(b));
        }
        const auto numerator = static_cast<value_type>(static_cast<u64>(numerator0) * numerator1);
        const auto denominator = static_cast<value_type>(static_cast<u64>(denominator0) * denominator1);
        const auto unit = static_cast<value_type>(static_cast<u64>(numerator) * inverse_binary(denominator));
        return static_cast<value_type>((static_cast<u64>(unit) << valuation) & mask);
    }

    // 分子・分母からpの因子を除いて積を求め、最後にp^valuationを戻す。
    static value_type prime_power_product(const PrimePower &factor, u64 n, u64 k,
                                          u64 valuation, u64 &budget)
    {
        consume_steps(k, budget);
        if (factor.prime == 2)
            return binary_product(n, k, valuation, factor.modulus - 1);
        value_type numerator = 1, denominator = 1;
        for (u64 i = 1; i <= k; ++i)
        {
            u64 a = n - k + i, b = i;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
            for (;;) {
                const auto q = divide_prime(a, factor);
                if (a - q * factor.prime != 0) break;
                a = q;
            }
            for (;;) {
                const auto q = divide_prime(b, factor);
                if (b - q * factor.prime != 0) break;
                b = q;
            }
#else
            while (a % factor.prime == 0) a /= factor.prime;
            while (b % factor.prime == 0) b /= factor.prime;
#endif
            numerator = multiply(numerator, reduce(a, factor), factor);
            denominator = multiply(denominator, reduce(b, factor), factor);
        }
        const auto unit = multiply(numerator, inverse(denominator, factor.modulus), factor);
        return multiply(unit, valuation_power(factor.prime, valuation), factor);
    }

    // 1個の素数冪でのnCkを、Lucas・累積積・直接の積から選んで求める。
#if defined(__GNUC__) && !defined(__clang__) && defined(__SIZEOF_INT128__)
    [[gnu::aligned(32)]]
#endif
    value_type choose_factor(PrimePower &factor, u64 n, u64 k, u64 &budget)
    {
        if (factor.exponent == 1)
            return lucas_prime(factor, n, k, budget);
        const u64 r = n - k;
        const auto current = static_cast<value_type>(factor.prefix.size() - 1);
        const bool ready = n <= current || current == factor.modulus - 1;
        if (factor.prime == 2)
        {
            const u64 valuation = binomial_valuation(n, k, factor);
            if (valuation >= factor.exponent)
                return 0;
            const u64 mask = factor.modulus - 1;
            if (!ready)
            {
                value_type required = 0;
                for (u64 nn = n, kk = k, rr = r; nn; nn >>= 1, kk >>= 1, rr >>= 1)
                {
                    required = std::max({required, static_cast<value_type>(nn & mask),
                                        static_cast<value_type>(kk & mask),
                                        static_cast<value_type>(rr & mask)});
                    if (required > options_.table_limit) break;
                }
                if (required > options_.table_limit)
                    return prime_power_product(factor, n, k, valuation, budget);
                ensure_prefix(factor, required);
            }
            // 2^eは2^32を割るため、途中はuint32_tの剰余で保持できる。
            value_type result = 1;
            for (u64 nn = n, kk = k, rr = r; kk || nn != rr;
                 nn >>= 1, kk >>= 1, rr >>= 1)
            {
                result = static_cast<value_type>(static_cast<u64>(result) * factor.prefix[static_cast<value_type>(nn & mask)]);
                result = static_cast<value_type>(static_cast<u64>(result) * factor.ifac[static_cast<value_type>(kk & mask)]);
                result = static_cast<value_type>(static_cast<u64>(result) * factor.ifac[static_cast<value_type>(rr & mask)]);
            }
            if (factor.exponent == 2 && ((valuation - ((n >> 1) - (k >> 1) - (r >> 1))) & 1))
                result = static_cast<value_type>(u64{0} - result);
            return static_cast<value_type>((static_cast<u64>(result) << valuation) & mask);
        }
        struct Level { u64 n, k, r; };
        Level levels[std::numeric_limits<i64>::digits];
        unsigned length = 0;
        u64 nn = n, kk = k, rr = r;
        u64 valuation = 0, low_valuation = 0;
        while (kk || nn != rr)
        {
            levels[length++] = {nn, kk, rr};
            nn = divide_prime(nn, factor);
            kk = divide_prime(kk, factor);
            rr = divide_prime(rr, factor);
            const auto carry = nn - kk - rr;
            valuation += carry;
            if (valuation >= factor.exponent)
                return 0;
            if (length < factor.exponent)
                low_valuation += carry;
        }
        if (!ready && factor.modulus - 1 > options_.table_limit &&
            reduce(n, factor) > options_.table_limit)
            return prime_power_product(factor, n, k, valuation, budget);
        const unsigned used = length;
        // 相殺後の商は同じ。剰余の参照分、未準備なら上限判定分まで補う。
        for (unsigned tail = 0; nn && (!ready || tail < factor.exponent); ++tail)
        {
            levels[length++] = {nn, 0, nn};
            nn = divide_prime(nn, factor);
        }
        auto remainders = [&](unsigned i)
        {
            const unsigned j = i + factor.exponent;
            const Level q = j < length ? levels[j] : Level{0, 0, 0};
            return Level{levels[i].n - q.n * factor.modulus,
                         levels[i].k - q.k * factor.modulus,
                         levels[i].r - q.r * factor.modulus};
        };
        if (!ready)
        {
            value_type required = 0;
            for (unsigned i = 0; i < length; ++i)
            {
                const auto rem = remainders(i);
                required = std::max({required, static_cast<value_type>(rem.n),
                                    static_cast<value_type>(rem.k),
                                    static_cast<value_type>(rem.r)});
                if (required > options_.table_limit) break;
            }
            if (required > options_.table_limit)
                return prime_power_product(factor, n, k, valuation, budget);
            ensure_prefix(factor, required);
        }
        // 三つの積を独立に進め、剰余乗算の依存を短くする。
        const auto first = remainders(0);
        value_type numerator = factor.prefix[static_cast<value_type>(first.n)];
        value_type inverse_k = factor.ifac[static_cast<value_type>(first.k)];
        value_type inverse_r = factor.ifac[static_cast<value_type>(first.r)];
        const unsigned quotient_end = length > factor.exponent ? length - factor.exponent : 0;
        const unsigned with_quotient = std::min(used, quotient_end);
        for (unsigned i = 1; i < with_quotient; ++i)
        {
            const auto &value = levels[i];
            const auto &q = levels[i + factor.exponent];
            numerator = multiply(numerator, factor.prefix[static_cast<value_type>(value.n - q.n * factor.modulus)], factor);
            inverse_k = multiply(inverse_k, factor.ifac[static_cast<value_type>(value.k - q.k * factor.modulus)], factor);
            inverse_r = multiply(inverse_r, factor.ifac[static_cast<value_type>(value.r - q.r * factor.modulus)], factor);
        }
        for (unsigned i = std::max(1u, with_quotient); i < used; ++i)
        {
            const auto &value = levels[i];
            numerator = multiply(numerator, factor.prefix[static_cast<value_type>(value.n)], factor);
            inverse_k = multiply(inverse_k, factor.ifac[static_cast<value_type>(value.k)], factor);
            inverse_r = multiply(inverse_r, factor.ifac[static_cast<value_type>(value.r)], factor);
        }
        value_type result = multiply(multiply(numerator, inverse_k, factor), inverse_r, factor);
        if ((valuation - low_valuation) & 1)
            result = factor.modulus - result;
        return multiply(result, valuation_power(factor.prime, valuation), factor);
    }

public:
    explicit BinomialMod(index_type modulus = 998244353,
                         index_type max_n = 5'000'000, BinomialOptions options = {})
        : modulus_(0), options_(options)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (modulus <= 0 || static_cast<u64>(modulus) > std::numeric_limits<value_type>::max())
        {
            throw std::invalid_argument("BinomialMod: modulus must be in [1, UINT32_MAX]");
        }
#endif
        modulus_ = static_cast<value_type>(modulus);
        factorize();
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        if (factors_.size() > 1)
        {
            const u64 reciprocal = std::numeric_limits<u64>::max() / modulus_ + 1;
            factors_[0].crt_reciprocal_part = static_cast<value_type>(reciprocal);
            factors_[1].crt_reciprocal_part = static_cast<value_type>(reciprocal >> 32);
        }
#endif
        prepare(max_n);
    }

    BinomialMod(index_type modulus, BinomialOptions options)
        : BinomialMod(modulus, 5'000'000, options) {}

    [[nodiscard]] value_type mod() const noexcept { return modulus_; }

    [[nodiscard]] bool is_prime_modulus() const noexcept
    {
        return factors_.size() == 1 && factors_[0].exponent == 1;
    }

    void prepare(index_type max_n = 5'000'000)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (max_n < 0)
            throw std::invalid_argument("BinomialMod: negative preparation bound");
#endif
        for (const auto &factor : factors_)
        {
            if (std::min<u64>(static_cast<u64>(max_n), factor.modulus - 1) > options_.table_limit)
            {
                throw std::length_error("BinomialMod: preparation exceeds table_limit");
            }
        }
        for (auto &factor : factors_)
        {
            const auto target = static_cast<value_type>(std::min<u64>(static_cast<u64>(max_n),
                                                                      factor.modulus - 1));
            if (factor.exponent == 1)
                ensure_factorials(factor, target);
            else
                ensure_prefix(factor, target);
        }
    }

#if defined(__GNUC__) && !defined(__clang__) && defined(__SIZEOF_INT128__)
    // GCCでは準備済み照会を短く保ち、汎用処理を分ける。
    [[nodiscard]] value_type COM(index_type n, index_type k)
    {
        if (modulus_ == 1)
            return 0;
        if (n < 0 || k < 0 || k > n)
            return 0;
        if (k == 0 || k == n)
            return 1;
        if (k == 1 || k == n - 1)
            return static_cast<value_type>(static_cast<u64>(n) % modulus_);
        const u64 nn = static_cast<u64>(n);
        if (factors_.size() == 1) {
            auto &factor = factors_[0];
            if (factor.exponent == 1 && nn < factor.fac.size())
                return multiply(multiply(factor.fac[static_cast<value_type>(nn)],
                                         factor.ifac[static_cast<value_type>(n - k)], factor),
                                factor.ifac[static_cast<value_type>(k)], factor);
            const u64 kk = static_cast<u64>(std::min(k, n - k));
            u64 budget = options_.max_multiplicative_steps;
            if (factor.exponent == 1)
            {
                if (nn < factor.prime)
                    return prime_digit(factor, static_cast<value_type>(nn),
                                       static_cast<value_type>(kk), budget);
                return lucas_prime(factor, nn, kk, budget);
            }
            return choose_factor(factor, nn, kk, budget);
        }
        return COM_crt(nn, static_cast<u64>(std::min(k, n - k)));
    }

private:
    [[gnu::noinline]]
    value_type COM_crt(u64 nn, u64 kk)
    {
        u64 budget = options_.max_multiplicative_steps;
        u64 result = 0;
        for (auto &factor : factors_)
        {
            const auto residue = choose_factor(factor, nn, kk, budget);
            // Σ(q-1)≤modulus-1より、CRTの積の和はuint64_tに収まる。
            result += static_cast<u64>(residue) * factor.crt_weight;
        }
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        const u64 reciprocal = static_cast<u64>(factors_[0].crt_reciprocal_part)
                               | (static_cast<u64>(factors_[1].crt_reciprocal_part) << 32);
        const u64 quotient = static_cast<u64>((static_cast<__uint128_t>(result) * reciprocal) >> 64);
        const u64 correction = quotient * modulus_;
        return static_cast<value_type>(result - correction + (result < correction ? modulus_ : 0));
#else
        return static_cast<value_type>(result % modulus_);
#endif
    }
public:

#else
    [[nodiscard]] value_type COM(index_type n, index_type k)
    {
        if (modulus_ == 1)
            return 0;
        if (n < 0 || k < 0 || k > n)
            return 0;
        if (k == 0 || k == n)
            return 1;
        if (k == 1 || k == n - 1)
            return static_cast<value_type>(static_cast<u64>(n) % modulus_);
        const u64 nn = static_cast<u64>(n);
        const u64 kk = static_cast<u64>(std::min(k, n - k));
        u64 budget = options_.max_multiplicative_steps;
        // 素数の通常範囲では Lucas の桁分解も CRT も不要。
        if (factors_.size() == 1)
        {
            auto &factor = factors_[0];
            if (factor.exponent == 1 && nn < factor.fac.size())
            {
                return multiply(multiply(factor.fac[static_cast<value_type>(nn)],
                                         factor.ifac[static_cast<value_type>(kk)], factor),
                                factor.ifac[static_cast<value_type>(nn - kk)], factor);
            }
            if (factor.exponent == 1 && nn < factor.prime)
            {
                return prime_digit(factor, static_cast<value_type>(nn),
                                   static_cast<value_type>(kk), budget);
            }
            return choose_factor(factor, nn, kk, budget);
        }
        u64 result = 0;
        for (auto &factor : factors_)
        {
            const auto residue = choose_factor(factor, nn, kk, budget);
            // Σ(q-1)≤modulus-1より、CRTの積の和はuint64_tに収まる。
            result += static_cast<u64>(residue) * factor.crt_weight;
        }
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        const u64 reciprocal = static_cast<u64>(factors_[0].crt_reciprocal_part)
                               | (static_cast<u64>(factors_[1].crt_reciprocal_part) << 32);
        const u64 quotient = static_cast<u64>((static_cast<__uint128_t>(result) * reciprocal) >> 64);
        const u64 correction = quotient * modulus_;
        return static_cast<value_type>(result - correction + (result < correction ? modulus_ : 0));
#else
        return static_cast<value_type>(result % modulus_);
#endif
    }

#endif

    [[nodiscard]] value_type Lucas(index_type n, index_type k)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (!is_prime_modulus())
        {
            throw std::domain_error("BinomialMod: Lucas requires a prime modulus; use COM");
        }
#endif
        return COM(n, k);
    }

    [[nodiscard]] value_type H(index_type n, index_type k)
    {
        if (n < 0 || k < 0)
            return 0;
        if (k == 0)
            return 1 % modulus_;
        if (n == 0 || modulus_ == 1)
            return 0;
        if (n - 1 > std::numeric_limits<index_type>::max() - k)
        {
            throw std::overflow_error("BinomialMod: n+k-1 exceeds int64_t");
        }
        return COM((n - 1) + k, k);
    }
};
#endif

#ifndef SIRKIT_MODINT_BINOMIAL_DEFINED
#define SIRKIT_MODINT_BINOMIAL_DEFINED
template <class Mint>
class ModIntBinomial {
  using u64 = std::uint64_t;
  using u32 = std::uint32_t;
  BinomialMod engine_;
  BinomialOptions options_;
  bool prime_;
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

  template <int> struct ConstantModulus {};
  static constexpr bool fixed_modulus_ = requires { typename ConstantModulus<Mint::mod()>; };

  u32 modulus() const noexcept {
    if constexpr (fixed_modulus_) return static_cast<u32>(Mint::mod());
    else return engine_.mod();
  }

  void check_modulus() const {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (!fixed_modulus_) {
      if (Mint::mod() != static_cast<int>(engine_.mod()))
        throw std::logic_error("ModIntBinomial: modulus changed");
    }
#endif
  }

  // 大表は2項・4項を並べて計算し、剰余乗算の依存を短くする。
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::noinline]]
#endif
  void extend_factorials_large(std::size_t old_size, u32 target) {
    const u64 p = modulus();
    u64 i = old_size;
    if (static_cast<u64>(target) * 4 + 6 < p) {
      u64 pair = (i * (i + 1)) % p;
      for (; i + 3 <= target; i += 4) {
        const auto previous = fac_[static_cast<std::size_t>(i - 1)];
        const auto p2 = Mint::raw(static_cast<int>(pair));
        u64 next_pair = pair + 4 * i + 6;
        next_pair -= next_pair >= p ? p : 0;
        const auto q2 = Mint::raw(static_cast<int>(next_pair));
        pair = next_pair + 4 * i + 14;
        pair -= pair >= p ? p : 0;
        const auto p3 = p2 * Mint::raw(static_cast<int>(i + 2));
        const auto p4 = p2 * q2;
        fac_[static_cast<std::size_t>(i)] = previous * Mint::raw(static_cast<int>(i));
        fac_[static_cast<std::size_t>(i + 1)] = previous * p2;
        fac_[static_cast<std::size_t>(i + 2)] = previous * p3;
        fac_[static_cast<std::size_t>(i + 3)] = previous * p4;
      }
    } else {
      for (; i + 1 <= target; i += 2) {
        const auto previous = fac_[static_cast<std::size_t>(i - 1)];
        const auto pair = Mint::raw(static_cast<int>(i)) * Mint::raw(static_cast<int>(i + 1));
        fac_[static_cast<std::size_t>(i)] = previous * Mint::raw(static_cast<int>(i));
        fac_[static_cast<std::size_t>(i + 1)] = previous * pair;
      }
    }
    for (; i <= target; ++i)
      fac_[static_cast<std::size_t>(i)] = fac_[static_cast<std::size_t>(i - 1)] * Mint::raw(static_cast<int>(i));
    // 全表ではWilsonの定理で逆元計算と逆階乗の乗算を省く。
    if constexpr (requires(Mint x) { -x; }) {
      if (target == p - 1) {
        i = old_size;
        if (i & 1) {
          ifac_[static_cast<std::size_t>(i)] = fac_[static_cast<std::size_t>(target - i)];
          ++i;
        }
        for (; i < target; i += 2) {
          ifac_[static_cast<std::size_t>(i)] = -fac_[static_cast<std::size_t>(target - i)];
          ifac_[static_cast<std::size_t>(i + 1)] = fac_[static_cast<std::size_t>(target - i - 1)];
        }
        if (i <= target) ifac_[static_cast<std::size_t>(i)] = -fac_[static_cast<std::size_t>(target - i)];
        return;
      }
    }
    ifac_[target] = fac_[target].inv();
    i = target;
    if (static_cast<u64>(target) * 4 + 6 < p) {
      u64 pair = (i * (i - 1)) % p;
      for (; i > old_size + 3; i -= 4) {
        const auto previous = ifac_[static_cast<std::size_t>(i)];
        const auto p2 = Mint::raw(static_cast<int>(pair));
        const u64 delta = 4 * i - 6;
        const u64 next_pair = pair + (pair < delta ? p : 0) - delta;
        const auto q2 = Mint::raw(static_cast<int>(next_pair));
        const u64 next_delta = 4 * i - 14;
        pair = next_pair + (next_pair < next_delta ? p : 0) - next_delta;
        const auto p3 = p2 * Mint::raw(static_cast<int>(i - 2));
        const auto p4 = p2 * q2;
        ifac_[static_cast<std::size_t>(i - 1)] = previous * Mint::raw(static_cast<int>(i));
        ifac_[static_cast<std::size_t>(i - 2)] = previous * p2;
        ifac_[static_cast<std::size_t>(i - 3)] = previous * p3;
        ifac_[static_cast<std::size_t>(i - 4)] = previous * p4;
      }
    } else {
      for (; i > old_size + 1; i -= 2) {
        const auto previous = ifac_[static_cast<std::size_t>(i)];
        const auto pair = Mint::raw(static_cast<int>(i)) * Mint::raw(static_cast<int>(i - 1));
        ifac_[static_cast<std::size_t>(i - 1)] = previous * Mint::raw(static_cast<int>(i));
        ifac_[static_cast<std::size_t>(i - 2)] = previous * pair;
      }
    }
    for (; i > old_size; --i)
      ifac_[static_cast<std::size_t>(i - 1)] = ifac_[static_cast<std::size_t>(i)] * Mint::raw(static_cast<int>(i));
  }

  // Mintの階乗・逆階乗を拡張する。確保失敗時はサイズを戻す。
  void ensure_factorials(u32 required) {
    if (required < fac_.size()) return;
    const u64 current = fac_.size() - 1;
    const u64 limit = std::min(options_.table_limit, modulus() - 1);
    const u64 target = std::min(limit, std::max<u64>(required, std::max<u64>(1, current * 2)));
    if (target + 1 > std::numeric_limits<std::size_t>::max()) {
      throw std::length_error("ModIntBinomial: table size exceeds size_t");
    }
    const auto old_size = fac_.size();
    const auto new_size = static_cast<std::size_t>(target + 1);
    try {
      grow_table(fac_, new_size);
      grow_table(ifac_, new_size);
    } catch (...) {
      fac_.resize(old_size);
      ifac_.resize(old_size);
      throw;
    }
    if (target + 1 - old_size >= 4096) {
      extend_factorials_large(old_size, static_cast<u32>(target));
      return;
    }
    for (u64 i = old_size; i <= target; ++i) {
      fac_[static_cast<std::size_t>(i)] =
          fac_[static_cast<std::size_t>(i - 1)] * Mint::raw(static_cast<int>(i));
    }
    ifac_[static_cast<std::size_t>(target)] = fac_[static_cast<std::size_t>(target)].inv();
    for (u64 i = target; i > old_size; --i) {
      ifac_[static_cast<std::size_t>(i - 1)] =
          ifac_[static_cast<std::size_t>(i)] * Mint::raw(static_cast<int>(i));
    }
  }

  // n<modのnCkを、Mintの階乗表または積で求める。
  Mint prime_digit(u32 n, u32 k, u64& budget) {
    k = std::min(k, n - k);
    if (k == 0) return Mint(1);
    if (k == 1) return Mint(n);
    if (n <= options_.table_limit) {
      ensure_factorials(n);
      return fac_[n] * ifac_[k] * ifac_[n - k];
    }
    if (k > budget) {
      throw std::length_error("ModIntBinomial: multiplicative step limit exceeded; "
                              "increase table_limit or max_multiplicative_steps");
    }
    budget -= k;
    if (k == 2) {
      const u64 product = (static_cast<u64>(n) * (n - 1u)) >> 1;
      return Mint::raw(static_cast<int>(product % modulus()));
    }
    Mint numerator(1), denominator(1);
    for (u64 i = 1; i <= k; ++i) {
      numerator *= Mint::raw(static_cast<int>(n - k + i));
      denominator *= Mint::raw(static_cast<int>(i));
    }
    return numerator * denominator.inv();
  }

  // 下位桁に続く全桁の0判定を済ませ、記録した桁を再利用する。
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::noinline]]
#endif
  Mint lucas_digits(u64 n, u64 k, u32 first_n, u32 first_k, u64& budget) {
    const u64 p = modulus();
    struct Digit { u32 n, k; };
    Digit digits[std::numeric_limits<std::int64_t>::digits];
    digits[0] = {first_n, first_k};
    unsigned count = 1;
    while (k) {
      const u64 nq = n / p, kq = k / p;
      const auto ni = static_cast<u32>(n - nq * p);
      const auto ki = static_cast<u32>(k - kq * p);
      if (ki > ni) return Mint(0);
      digits[count++] = {ni, ki};
      n = nq;
      k = kq;
    }
    Mint result = prime_digit(digits[0].n, digits[0].k, budget);
    for (unsigned i = 1; i < count; ++i)
      result *= prime_digit(digits[i].n, digits[i].k, budget);
    return result;
  }

  // 最初の桁で0なら、配列の確保や追加の呼び出しを行わない。
  Mint lucas_prime(u64 n, u64 k, u64& budget) {
    const u64 p = modulus();
    const u64 nq = n / p, kq = k / p;
    const auto ni = static_cast<u32>(n - nq * p);
    const auto ki = static_cast<u32>(k - kq * p);
    if (ki > ni) return Mint(0);
    if (kq == 0) return prime_digit(ni, ki, budget);
    return lucas_digits(nq, kq, ni, ki, budget);
  }

  // 準備済みテーブルで即答できない場合の計算を行う。
  Mint compute_uncached(std::int64_t n, std::int64_t k) {
    if (!prime_) return Mint::raw(static_cast<int>(engine_.COM(n, k)));
    if (n < 0 || k < 0 || k > n) return Mint(0);
    if (k == 0 || k == n) return Mint(1);
    if (k == 1 || k == n - 1) return Mint(n);
    // 全表がある法2・3はLucasの桁を整数だけで処理する。
    const auto p = modulus();
    if (p == 2 && fac_.size() == 2)
        return Mint::raw((static_cast<u64>(k) & ~static_cast<u64>(n)) == 0);
    if (p == 3 && fac_.size() == 3)
    {
        auto nn = static_cast<u64>(n), kk = static_cast<u64>(k);
        unsigned parity = 0;
        while (kk)
        {
            const auto nq = nn / 3, kq = kk / 3;
            const auto ni = nn - nq * 3, ki = kk - kq * 3;
            if (ki > ni) return Mint(0);
            parity ^= ni == 2 && ki == 1;
            nn = nq; kk = kq;
        }
        return Mint::raw(1 + parity);
    }
    u64 budget = options_.max_multiplicative_steps;
    const u64 kk = static_cast<u64>(std::min(k, n - k));
    if (static_cast<u64>(n) < modulus()) {
      return prime_digit(static_cast<u32>(n), static_cast<u32>(kk), budget);
    }
    return lucas_prime(static_cast<u64>(n), kk, budget);
  }

 public:
  explicit ModIntBinomial(std::int64_t max_n = 5'000'000, BinomialOptions options = {})
      : engine_(Mint::mod(), 0, options), options_(options), prime_(engine_.is_prime_modulus()) {
    prepare(max_n);
  }

  explicit ModIntBinomial(BinomialOptions options)
      : ModIntBinomial(5'000'000, options) {}

  void prepare(std::int64_t max_n = 5'000'000) {
    check_modulus();
#ifndef SKIP_RUNTIME_CHECKS
    if (max_n < 0)
      throw std::invalid_argument("ModIntBinomial: negative preparation bound");
#endif
    if (!prime_) { engine_.prepare(max_n); return; }
    const u64 target = std::min<u64>(static_cast<u64>(max_n), modulus() - 1);
    if (target > options_.table_limit) {
      throw std::length_error("ModIntBinomial: preparation exceeds table_limit");
    }
    ensure_factorials(static_cast<u32>(target));
  }

  [[nodiscard]] Mint COM(std::int64_t n, std::int64_t k) {
    check_modulus();
    // 通常の準備済み範囲では、Mint の乗算2回だけで答えを求める。
    if (prime_ && n >= 0 && k >= 0 && k <= n && static_cast<u64>(n) < fac_.size()) {
      return fac_[static_cast<std::size_t>(n)] * ifac_[static_cast<std::size_t>(k)]
           * ifac_[static_cast<std::size_t>(n - k)];
    }
    return compute_uncached(n, k);
  }

  [[nodiscard]] Mint Lucas(std::int64_t n, std::int64_t k) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!prime_)
      throw std::domain_error("ModIntBinomial: Lucas requires a prime modulus; use COM");
#endif
    return COM(n, k);
  }

  [[nodiscard]] Mint H(std::int64_t n, std::int64_t k) {
    check_modulus();
    if (n < 0 || k < 0) return Mint(0);
    if (k == 0) return Mint(1 % engine_.mod());
    if (n == 0 || engine_.mod() == 1) return Mint(0);
    if (n - 1 > std::numeric_limits<std::int64_t>::max() - k)
      throw std::overflow_error("ModIntBinomial: n+k-1 exceeds int64_t");
    return COM((n - 1) + k, k);
  }
};
#endif

#ifndef SIRKIT_PRIMALITY32_DEFINED
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
