#include "../Algorithm/LIS.cpp"
#include <iostream>
#include <vector>
int main() {
    const std::vector<int> a{3, 1, 2, 2, 4};
    for (auto length : LIS<true>(a)) std::cout << length << ' '; // 1 1 2 2 3
    std::cout << '\n' << LISLength<true>(a) << ' ' << LISLength<false>(a) << '\n'; // 3 4
    for (auto i : LISIndices(a)) std::cout << i << ' '; // 例: 1 3 4
    std::cout << '\n';
    for (int value : LISSequence<false>(a)) std::cout << value << ' '; // 1 2 2 4
    std::cout << '\n';
}
