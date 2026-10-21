https://judge.yosupo.jp/submission/411012
```
// Library Codes are in: https://github.com/reinsirk/Atcoder_Env

#include <bits/stdc++.h>
// #include <atcoder/all>
using namespace std;
// using namespace atcoder;
constexpr int64_t INF = static_cast<int64_t>(1) << 60;

template <class T>
inline bool chmin(T &a, T b);
template <class T>
inline bool chmax(T &a, T b);
// x^nを高速で計算する
inline uint64_t pow64i(uint64_t x, uint64_t n);
inline bool is_mul_overflow(int64_t a, int64_t b);
// cout << std::fixed << std::setprecision(15);

// 事前計算と、積による代替計算の上限。
struct BinomialOptions {
  // 各テーブルの最大添字。0なら事前計算を省く。
  std::uint32_t table_limit = 5'100'000;

  // 1回の問い合わせで許す積のループ回数。超過時は length_error。
  std::uint64_t max_multiplicative_steps = 1'000'000;
};

// 二項係数（C++17）。法は1〜UINT32_MAX、n・kはint64_t。
// n<0、k<0、k>n、法1は0。Lucasは素数専用。
// テーブル上限外は積で代替。積の回数上限も超えると length_error。
// 参考: https://dms.umontreal.ca/~andrew/Binomial/intro.html
class BinomialMod {
 public:
  using value_type = std::uint32_t;
  using index_type = std::int64_t;

 private:
  using u64 = std::uint64_t;
  using i64 = std::int64_t;

  struct PrimePower {
    value_type prime;
    value_type modulus;
    unsigned exponent;
    value_type crt_weight = 0;
    std::vector<value_type> fac{1};
    std::vector<value_type> ifac{1};
    std::vector<value_type> prefix{1};

    // 素数p、その冪m=p^e、指数eを保持する。
    PrimePower(value_type p, value_type m, unsigned e)
        : prime(p), modulus(m), exponent(e) {}
  };

  value_type modulus_;
  BinomialOptions options_;
  std::vector<PrimePower> factors_;

  // a*b mod m。uint64_tで乗算する。
  static value_type multiply(value_type a, value_type b, value_type m) noexcept {
    return static_cast<value_type>(static_cast<u64>(a) * b % m);
  }

  // 二分累乗で a^e mod m を求める。
  static value_type power(value_type a, u64 e, value_type m) noexcept {
    value_type result = 1;
    while (e) {
      if (e & 1) result = multiply(result, a, m);
      a = multiply(a, a, m);
      e >>= 1;
    }
    return result;
  }

  // 拡張ユークリッド互除法で逆元を求める。存在しなければ logic_error。
  static value_type inverse(value_type a, value_type m) {
    i64 r0 = a, r1 = m, x0 = 1, x1 = 0;
    while (r1) {
      const i64 q = r0 / r1;
      const i64 next_r = r0 - q * r1;
      const i64 next_x = x0 - q * x1;
      r0 = r1;
      r1 = next_r;
      x0 = x1;
      x1 = next_x;
    }
    if (r0 != 1) throw std::logic_error("BinomialMod: inverse does not exist");
    x0 %= static_cast<i64>(m);
    if (x0 < 0) x0 += m;
    return static_cast<value_type>(x0);
  }

  // 法を素数冪に分解し、CRTの係数を準備する。O(sqrt(mod))。
  void factorize() {
    u64 remaining = modulus_;
    for (u64 p = 2; p <= remaining / p; p += (p == 2 ? 1 : 2)) {
      if (remaining % p != 0) continue;
      u64 prime_power = 1;
      unsigned exponent = 0;
      do {
        remaining /= p;
        prime_power *= p;
        ++exponent;
      } while (remaining % p == 0);
      factors_.emplace_back(static_cast<value_type>(p),
                            static_cast<value_type>(prime_power), exponent);
    }
    if (remaining > 1) {
      factors_.emplace_back(static_cast<value_type>(remaining),
                            static_cast<value_type>(remaining), 1);
    }
    for (auto& factor : factors_) {
      const auto other = modulus_ / factor.modulus;
      const auto inv = inverse(other % factor.modulus, factor.modulus);
      factor.crt_weight = static_cast<value_type>(static_cast<u64>(other) * inv % modulus_);
    }
  }

  // 必要な添字まで、上限内でテーブルを概ね倍増させる。
  static value_type growth_target(value_type current, value_type required,
                                  value_type limit) noexcept {
    const u64 doubled = std::max<u64>(1, static_cast<u64>(current) * 2);
    return static_cast<value_type>(std::min<u64>(limit, std::max<u64>(required, doubled)));
  }

  // 素数用の階乗・逆階乗を拡張する。逆元計算は1回、確保失敗時はサイズを戻す。
  void ensure_factorials(PrimePower& factor, value_type required) {
    const auto current = static_cast<value_type>(factor.fac.size() - 1);
    if (required <= current) return;
    const auto limit = std::min(options_.table_limit, factor.prime - 1);
    const auto target = growth_target(current, required, limit);
    const auto old_size = factor.fac.size();
    if (static_cast<u64>(target) + 1 > std::numeric_limits<std::size_t>::max()) {
      throw std::length_error("BinomialMod: table size exceeds size_t");
    }
    const auto new_size = static_cast<std::size_t>(static_cast<u64>(target) + 1);
    try {
      factor.fac.resize(new_size);
      factor.ifac.resize(new_size);
    } catch (...) {
      factor.fac.resize(old_size);
      factor.ifac.resize(old_size);
      throw;
    }
    for (u64 i = old_size; i <= target; ++i) {
      factor.fac[static_cast<std::size_t>(i)] =
          multiply(factor.fac[static_cast<std::size_t>(i - 1)],
                   static_cast<value_type>(i), factor.prime);
    }
    factor.ifac[target] = power(factor.fac[target], factor.prime - 2, factor.prime);
    for (u64 i = target; i > old_size; --i) {
      factor.ifac[static_cast<std::size_t>(i - 1)] =
          multiply(factor.ifac[static_cast<std::size_t>(i)],
                   static_cast<value_type>(i), factor.prime);
    }
  }

  // pの倍数を除いた累積積を拡張する（素数冪用）。
  void ensure_prefix(PrimePower& factor, value_type required) {
    const auto current = static_cast<value_type>(factor.prefix.size() - 1);
    if (required <= current) return;
    const auto limit = std::min(options_.table_limit, factor.modulus - 1);
    const auto target = growth_target(current, required, limit);
    if (static_cast<u64>(target) + 1 > std::numeric_limits<std::size_t>::max()) {
      throw std::length_error("BinomialMod: table size exceeds size_t");
    }
    factor.prefix.resize(static_cast<std::size_t>(static_cast<u64>(target) + 1));
    for (u64 i = static_cast<u64>(current) + 1; i <= target; ++i) {
      factor.prefix[static_cast<std::size_t>(i)] = factor.prefix[static_cast<std::size_t>(i - 1)];
      if (i % factor.prime != 0) {
        factor.prefix[static_cast<std::size_t>(i)] =
            multiply(factor.prefix[static_cast<std::size_t>(i)],
                     static_cast<value_type>(i), factor.modulus);
      }
    }
  }

  // 積の回数を残予算から差し引く。足りなければ length_error。
  static void consume_steps(u64 count, u64& remaining) {
    if (count > remaining) {
      throw std::length_error("BinomialMod: multiplicative step limit exceeded; "
                              "increase table_limit or max_multiplicative_steps");
    }
    remaining -= count;
  }

  // n<p の nCk。テーブル内はO(1)、上限外は小さい側のkを使って積で求める。
  value_type prime_digit(PrimePower& factor, value_type n, value_type k, u64& budget) {
    k = std::min(k, n - k);
    if (k == 0) return 1;
    if (k == 1) return n;
    if (n <= options_.table_limit) {
      ensure_factorials(factor, n);
      return multiply(multiply(factor.fac[n], factor.ifac[k], factor.prime),
                      factor.ifac[n - k], factor.prime);
    }
    consume_steps(k, budget);
    value_type numerator = 1, denominator = 1;
    for (u64 i = 1; i <= k; ++i) {
      numerator = multiply(numerator, static_cast<value_type>(n - k + i), factor.prime);
      denominator = multiply(denominator, static_cast<value_type>(i), factor.prime);
    }
    return multiply(numerator, inverse(denominator, factor.prime), factor.prime);
  }

  // Lucasの定理で各桁の二項係数を掛ける。先に全桁の0判定を行う。
  value_type lucas_prime(PrimePower& factor, u64 n, u64 k, u64& budget) {
    for (u64 nn = n, kk = k; kk; nn /= factor.prime, kk /= factor.prime) {
      if (kk % factor.prime > nn % factor.prime) return 0;
    }
    value_type result = 1;
    while (k) {
      const auto ni = static_cast<value_type>(n % factor.prime);
      const auto ki = static_cast<value_type>(k % factor.prime);
      if (ki > ni) return 0;
      result = multiply(result, prime_digit(factor, ni, ki, budget), factor.prime);
      n /= factor.prime;
      k /= factor.prime;
    }
    return result;
  }

  // n!に含まれる素因数pの個数を求める。O(log_p(n+1))。
  static u64 factorial_valuation(u64 n, value_type p) noexcept {
    u64 result = 0;
    while (n) {
      n /= p;
      result += n;
    }
    return result;
  }

  // 素数冪用テーブルで参照する最大添字を求める。
  static value_type required_prefix(u64 n, const PrimePower& factor) noexcept {
    value_type result = 0;
    while (n) {
      result = std::max(result, static_cast<value_type>(n % factor.modulus));
      n /= factor.prime;
    }
    return result;
  }

  // n!からpの因子を除いた積。周期積は通常-1、p=2かつe>=3なら+1。
  static value_type unit_factorial(u64 n, const PrimePower& factor) noexcept {
    value_type result = 1;
    const bool negative_cycle = !(factor.prime == 2 && factor.exponent >= 3);
    while (n) {
      if (negative_cycle && ((n / factor.modulus) & 1)) {
        result = factor.modulus - result;
      }
      result = multiply(result, factor.prefix[static_cast<value_type>(n % factor.modulus)],
                        factor.modulus);
      n /= factor.prime;
    }
    return result;
  }

  // 分子・分母からpの因子を除いて積を求め、最後にp^valuationを戻す。
  static value_type prime_power_product(const PrimePower& factor, u64 n, u64 k,
                                       u64 valuation, u64& budget) {
    consume_steps(k, budget);
    value_type numerator = 1, denominator = 1;
    for (u64 i = 1; i <= k; ++i) {
      u64 a = n - k + i, b = i;
      while (a % factor.prime == 0) a /= factor.prime;
      while (b % factor.prime == 0) b /= factor.prime;
      numerator = multiply(numerator, static_cast<value_type>(a % factor.modulus), factor.modulus);
      denominator = multiply(denominator, static_cast<value_type>(b % factor.modulus), factor.modulus);
    }
    const auto unit = multiply(numerator, inverse(denominator, factor.modulus), factor.modulus);
    return multiply(unit, power(factor.prime, valuation, factor.modulus), factor.modulus);
  }

  // 1個の素数冪でのnCkを、Lucas・累積積・直接の積から選んで求める。
  value_type choose_factor(PrimePower& factor, u64 n, u64 k, u64& budget) {
    if (factor.exponent == 1) return lucas_prime(factor, n, k, budget);
    const u64 valuation = factorial_valuation(n, factor.prime)
                        - factorial_valuation(k, factor.prime)
                        - factorial_valuation(n - k, factor.prime);
    if (valuation >= factor.exponent) return 0;
    const auto required = std::max({required_prefix(n, factor), required_prefix(k, factor),
                                    required_prefix(n - k, factor)});
    if (required > options_.table_limit) {
      return prime_power_product(factor, n, k, valuation, budget);
    }
    ensure_prefix(factor, required);
    const auto numerator = unit_factorial(n, factor);
    const auto denominator = multiply(unit_factorial(k, factor),
                                      unit_factorial(n - k, factor), factor.modulus);
    const auto unit = multiply(numerator, inverse(denominator, factor.modulus), factor.modulus);
    return multiply(unit, power(factor.prime, valuation, factor.modulus), factor.modulus);
  }

 public:
  // 法を指定して構築する。既定では500万まで前計算。範囲外の法は invalid_argument。
  explicit BinomialMod(index_type modulus = 998244353,
                       index_type max_n = 5'000'000, BinomialOptions options = {})
      : modulus_(0), options_(options) {
    if (modulus <= 0 || static_cast<u64>(modulus) > std::numeric_limits<value_type>::max()) {
      throw std::invalid_argument("BinomialMod: modulus must be in [1, UINT32_MAX]");
    }
    modulus_ = static_cast<value_type>(modulus);
    factorize();
    prepare(max_n);
  }

  // オプションだけ指定した場合も、既定では500万まで前計算する。
  BinomialMod(index_type modulus, BinomialOptions options)
      : BinomialMod(modulus, 5'000'000, options) {}

  // 使用中の法を返す。
  [[nodiscard]] value_type mod() const noexcept { return modulus_; }

  // 法が素数ならtrue（法1はfalse）。
  [[nodiscard]] bool is_prime_modulus() const noexcept {
    return factors_.size() == 1 && factors_[0].exponent == 1;
  }

  // max_nまでの問い合わせ用テーブルを準備する。問い合わせ時の自動拡張も可。
  // 必要な添字がtable_limitを超えると length_error。max_n<0は invalid_argument。
  void prepare(index_type max_n = 5'000'000) {
    if (max_n < 0) throw std::invalid_argument("BinomialMod: negative preparation bound");
    for (const auto& factor : factors_) {
      if (std::min<u64>(static_cast<u64>(max_n), factor.modulus - 1) > options_.table_limit) {
        throw std::length_error("BinomialMod: preparation exceeds table_limit");
      }
    }
    for (auto& factor : factors_) {
      const auto target = static_cast<value_type>(std::min<u64>(static_cast<u64>(max_n),
                                                               factor.modulus - 1));
      if (factor.exponent == 1) ensure_factorials(factor, target);
      else ensure_prefix(factor, target);
    }
  }

  // nCk mod mod()。素数でn<mod・準備済みならO(1)。
  [[nodiscard]] value_type COM(index_type n, index_type k) {
    if (n < 0 || k < 0 || k > n || modulus_ == 1) return 0;
    if (k == 0 || k == n) return 1;
    if (k == 1 || k == n - 1) return static_cast<value_type>(static_cast<u64>(n) % modulus_);
    const u64 nn = static_cast<u64>(n);
    const u64 kk = static_cast<u64>(std::min(k, n - k));
    u64 budget = options_.max_multiplicative_steps;
    // 素数の通常範囲では Lucas の桁分解も CRT も不要。
    if (factors_.size() == 1) {
      auto& factor = factors_[0];
      if (factor.exponent == 1 && nn < factor.prime) {
        return prime_digit(factor, static_cast<value_type>(nn),
                           static_cast<value_type>(kk), budget);
      }
      return choose_factor(factor, nn, kk, budget);
    }
    u64 result = 0;
    for (auto& factor : factors_) {
      const auto residue = choose_factor(factor, nn, kk, budget);
      result = (result + static_cast<u64>(residue) * factor.crt_weight % modulus_) % modulus_;
    }
    return static_cast<value_type>(result);
  }

  // 素数の法でnCkを求める。合成数・法1では domain_error（COMを使う）。
  [[nodiscard]] value_type Lucas(index_type n, index_type k) {
    if (!is_prime_modulus()) {
      throw std::domain_error("BinomialMod: Lucas requires a prime modulus; use COM");
    }
    return COM(n, k);
  }

  // 重複組合せnHk=(n+k-1)Ck。0H0=1（法1は0）、0Hk=0（k>0）。
  // n+k-1がint64_tを超えると overflow_error。
  [[nodiscard]] value_type H(index_type n, index_type k) {
    if (n < 0 || k < 0) return 0;
    if (k == 0) return 1 % modulus_;
    if (n == 0 || modulus_ == 1) return 0;
    if (n - 1 > std::numeric_limits<index_type>::max() - k) {
      throw std::overflow_error("BinomialMod: n+k-1 exceeds int64_t");
    }
    return COM((n - 1) + k, k);
  }
};



int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int t;
    std::uint32_t mod;
    std::cin >> t >> mod;
    BinomialMod comb(mod);

    while (t--) {
        std::int64_t n, k;
        std::cin >> n >> k;
        std::cout << comb.COM(n, k) << '\n';
    }
}
template <class T>
inline bool chmin(T &a, T b)
{
    if (a > b)
    {
        a = b;
        return true;
    }
    return false;
}
template <class T>
inline bool chmax(T &a, T b)
{
    if (a < b)
    {
        a = b;
        return true;
    }
    return false;
}
inline uint64_t pow64i(uint64_t x, uint64_t n)
{
    uint64_t ret = 1;
    while (n > 0)
    {
        if (n & 1)
            ret *= x; // n の最下位bitが 1 ならば x^(2^i) をかける
        x *= x;
        n >>= 1; // n を1bit 左にずらす
    }
    return ret;
}
inline bool is_mul_overflow(int64_t a, int64_t b)
{
#ifdef _MSC_VER
    long long high, low = _mul128(a, b, &high);
    return !((high == 0 && low >= 0) || (high == -1 && low < 0));
#else
    long long low;
    return __builtin_smulll_overflow(a, b, &low);
#endif
}
```