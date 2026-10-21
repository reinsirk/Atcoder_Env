https://judge.yosupo.jp/submission/411011
```
// Library Codes are in: https://github.com/reinsirk/Atcoder_Env

#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
constexpr int64_t INF = static_cast<int64_t>(1) << 60;

template <class T>
inline bool chmin(T &a, T b);
template <class T>
inline bool chmax(T &a, T b);
// x^nを高速で計算する
inline uint64_t pow64i(uint64_t x, uint64_t n);
inline bool is_mul_overflow(int64_t a, int64_t b);
// cout << std::fixed << std::setprecision(15);

// AtCoder modint用・素数専用。他の二項係数ヘッダーへの依存なし。
// 準備済みのn<modはO(1)、n>=modはLucas。表外は積で計算する。
// dynamic_modintの法を変更したら、計算器も作り直す。
template <class Mint>
class PrimeBinomial {
  using i64 = std::int64_t;
  using u64 = std::uint64_t;
  const int modulus_;
  const u64 max_steps_;
  std::vector<Mint> fac_{Mint(1)}, ifac_{Mint(1)};

  // 素数かを試し割りで検査する。
  static bool is_prime(int p) {
    if (p < 2 || p % 2 == 0) return p == 2;
    for (int d = 3; d <= p / d; d += 2) if (p % d == 0) return false;
    return true;
  }
  // 古い法のテーブルを使わないようにする。
  void check_modulus() const {
    if (Mint::mod() != modulus_) throw std::logic_error("PrimeBinomial: modulus changed");
  }
  // 0<=k<=n<modのnCk。表外では小さい側のk回だけ積を取る。
  Mint digit(int n, int k, u64& remaining_steps) const {
    if (static_cast<std::size_t>(n) < fac_.size()) return fac_[n] * ifac_[k] * ifac_[n-k];
    k = std::min(k, n-k);
    if (k == 0) return Mint(1);
    if (k == 1) return Mint::raw(n);
    if (static_cast<u64>(k) > remaining_steps)
      throw std::length_error("PrimeBinomial: increase max_n or max_steps");
    remaining_steps -= k;
    Mint numerator(1), denominator(1);
    for (int i = 1; i <= k; ++i) {
      numerator *= Mint::raw(n-k+i);
      denominator *= Mint::raw(i);
    }
    return numerator * denominator.inv();
  }

 public:
  // 既定で500万まで前計算する。max_stepsは表外の積の回数上限。
  explicit PrimeBinomial(i64 max_n = 5'000'000, u64 max_steps = 1'000'000)
      : modulus_(Mint::mod()), max_steps_(max_steps) {
    if (!is_prime(modulus_)) throw std::domain_error("PrimeBinomial: prime modulus required");
    prepare(max_n);
  }

  // max_nまで前計算する。mod以上を指定した場合はmod-1まで。
  void prepare(i64 max_n = 5'000'000) {
    check_modulus();
    if (max_n < 0) throw std::invalid_argument("PrimeBinomial: negative bound");
    const auto target = static_cast<std::size_t>(std::min<i64>(max_n, modulus_-1));
    const auto old_size = fac_.size();
    if (target < old_size) return;
    try { fac_.resize(target+1); ifac_.resize(target+1); }
    catch (...) { fac_.resize(old_size); ifac_.resize(old_size); throw; }
    for (std::size_t i = old_size; i <= target; ++i)
      fac_[i] = fac_[i-1] * Mint::raw(static_cast<int>(i));
    ifac_[target] = fac_[target].inv();
    for (std::size_t i = target; i > old_size; --i)
      ifac_[i-1] = ifac_[i] * Mint::raw(static_cast<int>(i));
  }

  // nCk。n<0、k<0、k>nは0。表外の計算上限超過はlength_error。
  [[nodiscard]] Mint COM(i64 n, i64 k) const {
    check_modulus();
    if (n < 0 || k < 0 || k > n) return Mint(0);
    if (static_cast<u64>(n) < fac_.size()) return fac_[n] * ifac_[k] * ifac_[n-k];
    k = std::min(k, n-k);
    if (k == 0) return Mint(1);
    if (k == 1) return Mint(n);
    u64 remaining_steps = max_steps_;
    if (n < modulus_) return digit(static_cast<int>(n), static_cast<int>(k), remaining_steps);
    for (i64 nn = n, kk = k; kk; nn /= modulus_, kk /= modulus_)
      if (kk % modulus_ > nn % modulus_) return Mint(0);
    Mint result(1);
    while (k) {
      result *= digit(static_cast<int>(n % modulus_), static_cast<int>(k % modulus_), remaining_steps);
      n /= modulus_;
      k /= modulus_;
    }
    return result;
  }

  // COMが自動でLucasも処理するため、どちらを呼んでも同じ。
  [[nodiscard]] Mint Lucas(i64 n, i64 k) const { return COM(n, k); }
  
  // 重複組合せnHk。0H0=1、0Hk=0(k>0)。n+k-1の桁あふれは例外。
  [[nodiscard]] Mint H(i64 n, i64 k) const {
    check_modulus();
    if (n < 0 || k < 0) return Mint(0);
    if (k == 0) return Mint(1);
    if (n == 0) return Mint(0);
    if (n-1 > std::numeric_limits<i64>::max()-k) throw std::overflow_error("PrimeBinomial: n+k-1");
    return COM(n-1+k, k);
  }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int t, mod;
    std::cin >> t >> mod;

    using mint = atcoder::dynamic_modint<0>;
    mint::set_mod(mod);
    PrimeBinomial<mint> comb(9'999'999);

    while (t--) {
        std::int64_t n, k;
        std::cin >> n >> k;
        std::cout << comb.COM(n, k).val() << '\n';
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

https://atcoder.jp/contests/arc117/submissions/80086288
```
// Library Codes are in: https://github.com/reinsirk/Atcoder_Env

#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
constexpr int64_t INF = static_cast<int64_t>(1) << 60;

template <class T>
inline bool chmin(T &a, T b);
template <class T>
inline bool chmax(T &a, T b);
// x^nを高速で計算する
inline uint64_t pow64i(uint64_t x, uint64_t n);
inline bool is_mul_overflow(int64_t a, int64_t b);
// cout << std::fixed << std::setprecision(15);

using Graph = vector<vector<int>>;

// AtCoder modint用・素数専用。他の二項係数ヘッダーへの依存なし。
// 準備済みのn<modはO(1)、n>=modはLucas。表外は積で計算する。
// dynamic_modintの法を変更したら、計算器も作り直す。
template <class Mint>
class PrimeBinomial {
  using i64 = std::int64_t;
  using u64 = std::uint64_t;
  const int modulus_;
  const u64 max_steps_;
  std::vector<Mint> fac_{Mint(1)}, ifac_{Mint(1)};

  // 素数かを試し割りで検査する。
  static bool is_prime(int p) {
    if (p < 2 || p % 2 == 0) return p == 2;
    for (int d = 3; d <= p / d; d += 2) if (p % d == 0) return false;
    return true;
  }
  // 古い法のテーブルを使わないようにする。
  void check_modulus() const {
    if (Mint::mod() != modulus_) throw std::logic_error("PrimeBinomial: modulus changed");
  }
  // 0<=k<=n<modのnCk。表外では小さい側のk回だけ積を取る。
  Mint digit(int n, int k, u64& remaining_steps) const {
    if (static_cast<std::size_t>(n) < fac_.size()) return fac_[n] * ifac_[k] * ifac_[n-k];
    k = std::min(k, n-k);
    if (k == 0) return Mint(1);
    if (k == 1) return Mint::raw(n);
    if (static_cast<u64>(k) > remaining_steps)
      throw std::length_error("PrimeBinomial: increase max_n or max_steps");
    remaining_steps -= k;
    Mint numerator(1), denominator(1);
    for (int i = 1; i <= k; ++i) {
      numerator *= Mint::raw(n-k+i);
      denominator *= Mint::raw(i);
    }
    return numerator * denominator.inv();
  }

 public:
  // 既定で500万まで前計算する。max_stepsは表外の積の回数上限。
  explicit PrimeBinomial(i64 max_n = 5'000'000, u64 max_steps = 1'000'000)
      : modulus_(Mint::mod()), max_steps_(max_steps) {
    if (!is_prime(modulus_)) throw std::domain_error("PrimeBinomial: prime modulus required");
    prepare(max_n);
  }

  // max_nまで前計算する。mod以上を指定した場合はmod-1まで。
  void prepare(i64 max_n = 5'000'000) {
    check_modulus();
    if (max_n < 0) throw std::invalid_argument("PrimeBinomial: negative bound");
    const auto target = static_cast<std::size_t>(std::min<i64>(max_n, modulus_-1));
    const auto old_size = fac_.size();
    if (target < old_size) return;
    try { fac_.resize(target+1); ifac_.resize(target+1); }
    catch (...) { fac_.resize(old_size); ifac_.resize(old_size); throw; }
    for (std::size_t i = old_size; i <= target; ++i)
      fac_[i] = fac_[i-1] * Mint::raw(static_cast<int>(i));
    ifac_[target] = fac_[target].inv();
    for (std::size_t i = target; i > old_size; --i)
      ifac_[i-1] = ifac_[i] * Mint::raw(static_cast<int>(i));
  }

  // nCk。n<0、k<0、k>nは0。表外の計算上限超過はlength_error。
  [[nodiscard]] Mint COM(i64 n, i64 k) const {
    check_modulus();
    if (n < 0 || k < 0 || k > n) return Mint(0);
    if (static_cast<u64>(n) < fac_.size()) return fac_[n] * ifac_[k] * ifac_[n-k];
    k = std::min(k, n-k);
    if (k == 0) return Mint(1);
    if (k == 1) return Mint(n);
    u64 remaining_steps = max_steps_;
    if (n < modulus_) return digit(static_cast<int>(n), static_cast<int>(k), remaining_steps);
    for (i64 nn = n, kk = k; kk; nn /= modulus_, kk /= modulus_)
      if (kk % modulus_ > nn % modulus_) return Mint(0);
    Mint result(1);
    while (k) {
      result *= digit(static_cast<int>(n % modulus_), static_cast<int>(k % modulus_), remaining_steps);
      n /= modulus_;
      k /= modulus_;
    }
    return result;
  }

  // COMが自動でLucasも処理するため、どちらを呼んでも同じ。
  [[nodiscard]] Mint Lucas(i64 n, i64 k) const { return COM(n, k); }
  
  // 重複組合せnHk。0H0=1、0Hk=0(k>0)。n+k-1の桁あふれは例外。
  [[nodiscard]] Mint H(i64 n, i64 k) const {
    check_modulus();
    if (n < 0 || k < 0) return Mint(0);
    if (k == 0) return Mint(1);
    if (n == 0) return Mint(0);
    if (n-1 > std::numeric_limits<i64>::max()-k) throw std::overflow_error("PrimeBinomial: n+k-1");
    return COM(n-1+k, k);
  }
};


int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n;
    std::string s;
    std::cin >> n >> s;

    using mint = atcoder::static_modint<3>;
    PrimeBinomial<mint> comb;
    mint answer = 0;
    const std::string colors = "BWR";

    for (int i = 0; i < n; ++i) {
        const int color = static_cast<int>(colors.find(s[i]));
        answer += comb.Lucas(n - 1, i) * color;
    }

    if ((n - 1) % 2 != 0) answer = -answer;
    std::cout << colors[answer.val()] << '\n';
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