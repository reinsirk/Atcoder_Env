# Project Selection

`Graph/Project_Selection.cpp` の `TwoVariableSubmodularOpt<COST>` は、二値変数に対する劣モジュラなコストの総和を最小化する。`true` を最小カットの始点側、`false` を終点側として表す。公開 API はファイル先頭、実行例は `Examples/project_selection_example.cpp` を参照。

## 項と制約

`add_single_cost(i,F,T)` は変数が false/true のとき F/T を加える。`add_psp_penalty(i,j,C)` は `xi=true, xj=false` に非負費用 C を加え、`add_psp_constraint(i,j)` は同じ組を禁止する。`force` と `add_equality` で固定値・等値制約も指定できる。

`add_submodular_function(i,j,A,B,C,D)` は FF/FT/TF/TT のコストを追加する。異なる変数では `B+C >= A+D` が必要。同じ変数には A/D だけを使用する。条件の判定と辺への分解では、加算の前に差を取り、符号付き整数の未定義オーバーフローを避ける。

`add_both_true_profit` / `add_both_false_profit` は二変数がともに true/false の場合の非負利得、`add_all_true_profit(xs,P)` / `add_all_false_profit(xs,P)` は集合全体の非負利得を表す。空集合では利得 P を常に得る。集合内の重複は許される。

`add_any_true_cost(xs,C)` / `add_any_false_cost(xs,C)` は、少なくとも一つが true/false の場合の非負費用 C を表す。空集合の費用は 0。例えば、利用した設備の種類ごとに一度だけかかる費用を、複数の変数をまとめて指定できる。これらは補助頂点で直接表現し、定数加算と利得の二段階操作による一時的な offset の超過を避ける。

```cpp
TwoVariableSubmodularOpt<std::int64_t> model(3);
model.add_single_cost(0, 0, -8); // 選ぶと8の利得
model.add_psp_constraint(0, 1);  // 0を選ぶなら1も選ぶ
model.add_any_true_cost({0, 1}, 3);
model.add_any_false_cost({0, 1, 2}, 2);
auto [cost, assignment] = model.solve_with_assignment();
```

集合項の費用・利得が 0 の場合は無操作で、未参照の添字は検査しない。`SKIP_RUNTIME_CHECKS` を定義すると、添字・非負性・劣モジュラ性の入力検査を除く。変数数・内部添字・容量予算・算術の上限は省略時も維持する。不正入力を未参照部分まで一括検査する API ではない。

## 数値と解法

`COST` は 64 bit 以下の符号付き整数型。入力値だけでなく、分解途中の符号付きコスト・offset・最適値もこの型の範囲に収まる必要がある。有限辺の容量総和は `std::make_unsigned_t<COST>` の最大値未満に制限する。差は符号なし演算で正確に表し、厳密制約の容量にはその最大値を使用する。有限容量の総和を超えるフローが流れたら制約矛盾として `std::domain_error` を送出する。容量・数値の超過は `std::overflow_error`。旧コンストラクタの `inf` 引数は互換性のために受け付け、値は使用しない。

残余網を隣接配列で保持し、Dinic 法で解く。再帰は固定の深さまでに制限し、それより長い増加路は明示的なスタックで処理する。同じ層グラフで使い切った頂点を無効にし、再訪を減らす。標準 C++23 だけで実装し、専用の SIMD・128 bit 整数経路は持たない。

補助頂点込みの頂点数を V、辺数を E とすると、解法は `O(V²E)`、保存領域は `O(V+E)`。単一項の追加は償却 `O(1)`、集合項は `O(|xs|)`。`solve()` は変更がなければ `O(1)` で、項の追加後は残余網から再開する。定数の追加はフローを再計算しない。`reconstruct()` は必要に応じて解いてから、追加 `O(n)` で最適割当を返す。同点時の割当は固定しない。

## 再利用・所有権・例外

`clear()` は変数数を維持して項と解を消去する。補助頂点を取り除き、残る頂点の辺配列と作業配列の容量を再利用する。確保を行わず `noexcept`、計算量は `O(V+E)`。`init(n)` は隣接配列を作り直すため、旧隣接配列の領域を解放する。作業配列の容量は保持する。`reserve_nodes(count)` は頂点配列の予約で、辺配列の予約ではない。

コピーは独立した所有、移動後の元は空の 0 変数モデル。`empty()` は変数数が 0 かを返す。メンバ `swap` と ADL の `swap(a,b)` は定数時間で所有権を交換する。

項の追加・`init`・コピー代入が失敗しても、変数数・追加済みの項・最適値は変えない。ただし確保済み容量は変わり得る。複数辺を追加する操作は先に必要な容量を予約し、集合項は失敗時に追加分だけ巻き戻す。`solve()` の確保失敗後も再試行できる。制約矛盾は項追加では解消できないので `clear` / `init` で作り直す。最適値の型超過はフロー計算後に判定し、定数を調整して収まる場合は再取得できる。

## 検証と資料

Verify は [AtCoder 典型90 040](https://atcoder.jp/contests/typical90/tasks/typical90_an)。main は入力を単一項・含意制約に変換して解くだけとする。公開サンプルに加え、全割当列挙と独立な行列形式の Edmonds–Karp 法を使った補足入力を生成する。この問題で扱わない集合項などは、`Verify/current/tests/project_selection_reference_test.cpp` の独立エネルギー表・境界試験で照合する。

アルゴリズムの参考は [drken の解説](https://drken1215.hatenablog.com/entry/2023/11/24/034300)。由来とライセンス、測定条件・不採用案、今回の検証結果は `Audit/*-project-selection-2026-10-22.json` に記録する。ローカル検証とオンライン提出の状態は区別する。
