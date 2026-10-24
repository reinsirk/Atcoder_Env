# グラフ

各ライブラリは必要な標準ヘッダーを含み、単独で貼り付けられます。

| ファイル | 対応する検証 |
| --- | --- |
| [Bellman-Ford.cpp](Bellman-Ford.cpp) | [E - Coins Respawn](../Verify/current/submissions/bellman_ford.cpp) |
| [Bellman-Ford_SFPA.cpp](Bellman-Ford_SFPA.cpp) | [E - Coins Respawn](../Verify/current/submissions/spfa.cpp) |
| [Dijkstra.cpp](Dijkstra.cpp) | [Shortest Path](../Verify/current/submissions/dijkstra.cpp) |
| [Floyd-Warshall.cpp](Floyd-Warshall.cpp) | [D - バスと避けられない運命](../Verify/current/submissions/floyd_warshall.cpp) |
| [LCA.cpp](LCA.cpp) | [Lowest Common Ancestor](../Verify/current/submissions/lca.cpp) |
| [Project_Selection.cpp](Project_Selection.cpp) | [040 - Get More Money（★7）](../Verify/current/submissions/project_selection.cpp) |
| [Topological.cpp](Topological.cpp) | [G - Longest Path](../Verify/current/submissions/topological.cpp) |
| [Topological_sort.cpp](Topological_sort.cpp) | [D - Restricted Permutation](../Verify/current/submissions/topological_lexicographical.cpp) |

問題URL、追加APIの補足検査、現在版の判定状況は[Verify一覧](../Verify/current/README.md)を参照してください。

## Dijkstra

[アルゴリズムと前提](../Document/dijkstra.md)・[使用例](../Examples/dijkstra_example.cpp)を参照してください。
重みは`[0, 1e9]`です。経路付きの呼び出しでも全頂点の最短距離を計算します。
現在のソースと判定の対応は[Verifyのmanifest](../Verify/current/manifest.json)で確認できます。

## LCA

[アルゴリズムと前提](../Document/lca.md)・[使用例](../Examples/lca_example.cpp)を参照してください。

[トポロジカルソートの説明](../Document/topological.md)に通常順と辞書順の前提・使用例をまとめています。

[Bellman–FordとSPFAの説明](../Document/negative_shortest_paths.md)に負閉路判定・未到達値・入力の前提をまとめています。

[Project Selectionの説明](../Document/project_selection.md)に費用・利得の表現、解の復元、再利用の前提をまとめています。
