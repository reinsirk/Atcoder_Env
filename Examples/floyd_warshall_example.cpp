#include "../Graph/Floyd-Warshall.cpp"
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    std::vector initial(5, std::vector<std::int64_t>(5, INF));
    for (int i = 0; i < 5; ++i) initial[i][i] = 0;
    initial[0][1] = 4;
    initial[1][2] = -2;
    initial[0][2] = 10;
    initial[2][3] = 7;
    auto distance = initial;
    std::vector<std::vector<int>> next;
    std::cout << FloydWarshall(distance, next) << '\n'; // 0
    std::cout << distance[0][2] << ' ' << (distance[0][4] == INF) << '\n'; // 2 1
    for (int vertex : FloydWarshallPath(next, 0, 2)) std::cout << vertex << ' ';
    std::cout << '\n'; // 0 1 2

    auto negative = initial;
    negative[2][1] = 1; // 1→2→1が負閉路。
    std::cout << FloydWarshall(negative) << '\n'; // 1
    std::cout << (negative[0][3] == -INF) << ' ' << (negative[4][0] == INF)
              << ' ' << negative[4][4] << '\n'; // 1 1 0
}
