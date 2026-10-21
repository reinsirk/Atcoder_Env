#include "../Graph/Bellman-Ford_SFPA.cpp"
#include <iostream>

int main()
{
    WGraph graph(5);
    graph[0].emplace_back(1, 2);
    graph[1].emplace_back(2, -3);
    graph[2].emplace_back(1, 1); // 負閉路。
    graph[2].emplace_back(3, 4);
    std::vector<std::int64_t> distances;
    std::cout << SPFA(graph, distances, 0) << '\n'; // 1
    for (const auto distance : distances)
    {
        if (distance == INF)
            std::cout << "unreachable\n";
        else if (distance == -INF)
            std::cout << "negative cycle\n";
        else
            std::cout << distance << '\n';
    }
}
