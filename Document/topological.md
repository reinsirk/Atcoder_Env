# トポロジカルソート

`Graph/Topological.cpp` と `Graph/Topological_sort.cpp` は、同じ公開APIと共有ガードを持つ。どちらか一方を単独で貼り付けて使える。`Graph` は `std::vector<std::vector<int>>`。

| API | 戻り値・用途 |
| --- | --- |
| `TopologicalSort(graph)` | 入次数0の頂点をFIFOで処理した順序。閉路があれば空列。 |
| `TopologicalSortLexicographical(graph)` | 辞書順最小の順序。閉路があれば空列。 |
| `TopologicalSortDetails(graph, lexicographical=false)` | 取り出せた頂点列・各頂点の位置・DAG判定・順序の一意性。 |
| `IsDAG(graph)` | 有向閉路がなければ `true`。 |

空グラフはDAGであり、順序は空、一意性は `true`。閉路があるとき、`Details` は処理できた部分の順序を残すが、一意性は `false`。位置配列の未処理頂点は `-1`。部分順序は、閉路の下流にある頂点を含まない場合がある。

FIFO版は `O(V+E)` 時間、辞書順版は `O(E+V log(V+1))` 時間の上限を持つ。どちらも追加領域は `O(V)`。入力グラフを変更せず、戻り値の配列は呼出しごとに所有する。既存の `Details` と `IsDAG` が用途を補うため、追加の公開APIは設けていない。

辞書順版は、候補の最小頂点を直接保持し、残りを64ビット単位の階層ビット集合で管理する。下位の語が非空かどうかを上位のビットで表すため、空の区間を線形に探さない。頂点数の制限下では階層は高々6段。候補が1個しかない間は集合への追加・削除を行わず、64頂点を超える集合用の領域も、複数候補が初めて現れるまで確保しない。候補の大小は入力分布にかかわらず頂点番号で比較する。

次の最小候補は、直前の最小頂点と同じ64ビット語に残っていれば、その語から直接取り出す。残っていなければ階層をたどる。GCC／Clangのx86でAVX2に対応し、4096頂点以上なら、初期入次数のゼロ判定を64頂点ずつまとめ、下位語から上位語を構築する。端数は有効な頂点だけを処理する。AVX2非対応時と小規模入力は標準C++23の登録処理を使う。貼り付ける前に`SIRKIT_DISABLE_SIMD`を定義すると、SIMD専用処理とCPU判定を除く。

頂点数は `INT_MAX` 以下、各入次数は `UINT32_MAX` 以下。容量超過は検査省略時も例外とする。既定では入次数を数える走査に終点の範囲検査を組み込み、取り出し時には繰り返さない。`SKIP_RUNTIME_CHECKS` を定義した場合、辺の終点がすべて `[0,V)` に入ることは利用側の前提になる。

FIFO版は戻り値の配列をキューとして使う。大きなグラフで候補が十分にあり、到達できる頂点も増えている場合は、確保済みの配列を頂点数まで広げて添字で追加する。入次数が0になったかによる分岐を減らし、実際に追加した長さだけを結果として残す。鎖のように候補が少ない場合や、閉路のため処理が進まない場合は通常の追加を続け、未使用領域の初期化を避ける。FIFO順と部分順序の扱いは共通で、入力の頂点番号の並びには依存しない。

```cpp
const Graph graph{{3}, {2}, {}, {}};
const auto fifo = TopologicalSort(graph); // 0,1,3,2
const auto lex = TopologicalSortLexicographical(graph); // 0,1,2,3
const auto details = TopologicalSortDetails(graph, true);
// details.acyclic == true, details.unique == false
const bool cyclic = !IsDAG(Graph{{1}, {0}});
```

Verifyは通常版に [DP G](https://atcoder.jp/contests/dp/tasks/dp_g)、辞書順版に [ABC223 D](https://atcoder.jp/contests/abc223/tasks/abc223_d) を使う。閉路・一意性・部分順序・入力検査はローカルの独立参照で補う。提出履歴と現在版のローカル検証は別に記録する。

アルゴリズムの参考: [競プロのための標準 C++](https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/topological-sort)。
