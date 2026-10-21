#include <cstdint>
#include <iostream>

// ここにBinomialModのライブラリを貼り付ける。

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
