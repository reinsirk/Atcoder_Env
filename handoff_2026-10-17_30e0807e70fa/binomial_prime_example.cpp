// SPDX-License-Identifier: CC0-1.0
#ifndef REINSIRK_PRIME_BINOMIAL_DEFINED
#define REINSIRK_PRIME_BINOMIAL_DEFINED
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

// AtCoder modint用。法は素数、prepareの引数は非負を前提とする。
// 準備済みのn<modはO(1)、n>=modはLucas。表外は積で計算する。
// dynamic_modintの法を変更したら、計算器も作り直す。
template <class Mint>
class PrimeBinomial {
  using i64 = std::int64_t;
  using u64 = std::uint64_t;
  const int modulus_;
  const u64 max_steps_;
  std::vector<Mint> fac_{Mint(1)}, ifac_{Mint(1)};

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
    prepare(max_n);
  }
  // max_nまで前計算する。mod以上を指定した場合はmod-1まで。
  void prepare(i64 max_n = 5'000'000) {
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
  // 重複組合せnHk。0H0=1、0Hk=0(k>0)。n+k-1はint64_t内を前提とする。
  [[nodiscard]] Mint H(i64 n, i64 k) const {
    if (n < 0 || k < 0) return Mint(0);
    if (k == 0) return Mint(1);
    if (n == 0) return Mint(0);
    return COM(n-1+k, k);
  }
};
#endif

#include <atcoder/modint>
#include <iostream>

using mint = atcoder::modint998244353;

int main() {
  PrimeBinomial<mint> comb;  // 既定で500万まで前計算。
  std::cout << comb.COM(10, 3).val() << '\n';  // 120
  std::cout << comb.H(3, 4).val() << '\n';     // 15

  // 小さな素数では全桁のテーブルが小さく、巨大なnにもLucasで対応。
  PrimeBinomial<atcoder::static_modint<7>> small;
  std::cout << small.Lucas(100, 20).val() << '\n';  // 0
}
