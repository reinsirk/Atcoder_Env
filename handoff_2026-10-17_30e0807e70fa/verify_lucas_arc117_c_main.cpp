#include <iostream>
#include <string>
#include <atcoder/modint>

// ここにPrimeBinomialのライブラリを貼り付ける。

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
