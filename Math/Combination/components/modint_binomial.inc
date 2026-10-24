// SIRKIT_API_BEGIN
// [ModIntBinomial<Mint>]
// AtCoder modint用。Mintは利用側で用意し、動的な法を変えたら計算器も作り直す。
// ModIntBinomial(max_n=5'000'000,options={}): Mint::mod()を法として構築・前計算。合成数にも対応。
// ModIntBinomial(options): 指定した上限で500万まで前計算する。
// prepare(max_n=5'000'000): 上限内で表を準備。制限・例外はBinomialModと同じ。
// COM(n,k): nCkをMintで返す。n<0,k<0,k>nと法1は0。素数でn<modかつ準備済みならO(1)。
// Lucas(n,k): 素数の法でCOMと同じ結果。合成数・法1ではCOMを使う。
// H(n,k): nHkをMintで返す。0H0=1（法1は0）、0Hk=0（k>0）。int64_t超過はoverflow_error。
// SIRKIT_API_END
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
