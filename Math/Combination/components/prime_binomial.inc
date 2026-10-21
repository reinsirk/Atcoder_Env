// AtCoder modint用・素数専用（C++20以降）。他の二項係数ヘッダーへの依存なし。
// 準備済みのn<modはO(1)、n>=modはLucas。表外は積で計算する。
// dynamic_modintの法を変更したら、計算器も作り直す。
// SKIP_RUNTIME_CHECKSで入力検査を省略する。ACLは利用側で読み込む。
#ifndef REINSIRK_PRIME_BINOMIAL_DEFINED
#define REINSIRK_PRIME_BINOMIAL_DEFINED
template <class Mint>
class PrimeBinomial
{
    // PrimeBinomial(max_n=5'000'000, max_steps=1'000'000): Mintの素数の法で構築し、前計算する。
    // prepare(max_n=5'000'000): min(max_n, mod-1)まで階乗・逆階乗を準備する。
    // COM(n, k): nCkをMintで返す。n<0、k<0、k>nは0。
    // Lucas(n, k): COMと同じ。n>=modの処理もCOMが自動で行う。
    // H(n, k): 重複組合せnHkをMintで返す。0H0=1、0Hk=0（k>0）。
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
        u64 remaining_steps = max_steps_;
        if (n < modulus())
            return digit(static_cast<int>(n), static_cast<int>(k), remaining_steps);
        return lucas(n, k, remaining_steps);
    }

public:
    // 既定で500万まで前計算する。max_stepsは表外の積の回数上限。
    explicit PrimeBinomial(i64 max_n = 5'000'000, u64 max_steps = 1'000'000)
        : modulus_(Mint::mod()), max_steps_(max_steps)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (modulus_ < 2 || !reinsirk::detail::is_prime_u32(static_cast<std::uint32_t>(modulus_)))
            throw std::domain_error("PrimeBinomial: prime modulus required");
#endif
        prepare(max_n);
    }

    // max_nまで前計算する。mod以上を指定した場合はmod-1まで。
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
        const auto old_size = fac_.size();
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

    // nCk。n<0、k<0、k>nは0。表外の計算上限超過はlength_error。
    [[nodiscard]] Mint COM(i64 n, i64 k) const
    {
        check_modulus();
        if (n < 0 || k < 0 || k > n)
            return Mint(0);
        if (static_cast<u64>(n) < fac_.size())
            return fac_[n] * ifac_[k] * ifac_[n - k];
        return compute_uncached(n, k);
    }

    // COMが自動でLucasも処理するため、どちらを呼んでも同じ。
    [[nodiscard]] Mint Lucas(i64 n, i64 k) const { return COM(n, k); }

    // 重複組合せnHk。0H0=1、0Hk=0(k>0)。n+k-1の桁あふれは例外。
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
