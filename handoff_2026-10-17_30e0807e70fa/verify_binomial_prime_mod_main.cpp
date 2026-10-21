#include <cstdint>
#include <iostream>
#include <atcoder/modint>

// ここにPrimeBinomialのライブラリを貼り付ける。

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
