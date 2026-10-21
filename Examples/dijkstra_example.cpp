#include "../Graph/Dijkstra.cpp"
#include <iostream>

int main()
{
    WGraph graph(5);
    graph[0].emplace_back(1, 2);
    graph[0].emplace_back(2, 7);
    graph[1].emplace_back(2, 3);
    graph[2].emplace_back(3, 0);
    std::vector<std::int64_t> distances;
    std::vector<int> path;
    Dijkstra(graph, distances, 0, 3, path);
    for (const auto distance : distances)
    {
        if (distance == INF)
            std::cout << "INF\n";
        else
            std::cout << distance << '\n';
    }
    for (std::size_t i = 0; i < path.size(); ++i)
        std::cout << (i == 0 ? "" : " ") << path[i];
    std::cout << '\n'; // 0 1 2 3
    Dijkstra(graph, distances, 0); // 距離のみ。経路用配列の確保・更新を省く。
}
