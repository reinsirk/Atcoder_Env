# 直線の追加と最適値照会

`LineContainer<T, objective>`は直線`y=k*x+m`を追加し、指定した`x`での最小値または最大値を返します。傾きや照会順の制約はありません。`MinLineContainer<T>`と`MaxLineContainer<T>`を使うと目的を省略できます。[使用例](../Examples/line_container_example.cpp)を参照してください。

直線を傾き順に保持し、最適になる区間の右端`p`を保存します。追加した直線と隣接する直線の交点を更新し、不要になった直線を除きます。整数で包絡が小さい場合は、隣接する直線から不要と判断できる追加の確保を省き、残す場合は計算した交点を再利用します。照会では`x<=p`となる最初の直線を探します。以下、hは包絡に保持する直線数です。領域はO(h)、追加は償却O(log h)、照会はO(log h)です。1回の追加で多数の直線を除く場合はO(h)かかります。

## API

| API | 動作 |
| --- | --- |
| `add(k,m)` | 直線を追加する。既存直線より常に劣るものや重複は包絡から除かれ得る。 |
| `query(x)` | 最適値をTで返す。非空が前提。 |
| `query_optional(x)` | 空ならnullopt、それ以外はqueryと同じ。 |
| `query_line(x)` | 選択した元係数の組`pair<T,T>{k,m}`を返す。非空が前提。値の評価は行わない。 |
| `query_line_optional(x)` | 空ならnullopt、それ以外はquery_lineと同じ。 |
| `query_with_line(x)` | 1回の探索で`Result{value,k,m}`を返す。非空が前提。 |
| `query_with_line_optional(x)` | 空ならnullopt、それ以外はquery_with_lineと同じ。 |
| `size()` / `empty()` | 保持する直線数・空判定。O(1)。追加回数とは異なる。 |
| `clear()` / `swap(other)` | 全削除 / 同じ型の包絡を交換する。O(h) / O(1)。 |
| `lc_div(a,b)` | 整数では床除算、浮動小数点では通常の除算。 |
| `lc_inf<T>()` | 整数ではTの最大値、浮動小数点では正の無限大。 |

同じ値を与える直線の選択は未規定です。`Result`は値と係数を所有し、コンテナ内の要素への参照を持ちません。値と直線が両方必要な場合に使います。

```cpp
MinLineContainer<std::int64_t> lines;
lines.add(2, 1);
lines.add(-1, 4);
auto result = lines.query_with_line(3);
// result.value == 1, result.k == -1, result.m == 4
```

## 数値と入力の前提

整数Tは64bit以下の符号付き型です。傾き・切片・問い合わせはTに収めます。交点は整数の床除算で求め、型の範囲へ制限した右端を保持します。係数差の符号付きオーバーフローを避けて計算し、型の最小値ちょうどで選択される直線も扱います。

整数の最適値は数学的な`k*x+m`を評価し、Tの範囲外なら`SKIP_RUNTIME_CHECKS`の有無によらず`overflow_error`を投げます。積だけがTを超えても、切片との相殺で最終値がTに収まれば返します。利用可能なGCC/Clangでは128bit整数拡張を使い、`SIRKIT_LINE_CONTAINER_FORCE_PORTABLE`で標準C++の代替計算へ切り替えられます。拡張型がない環境も代替経路を使います。`query_line`系は最適値の評価をしないため、この値域検査を行いません。

浮動Tでは有限の係数・問い合わせを渡します。交点や値には丸め誤差があり、厳密な実数の最適値を保証しません。値の表現範囲を超える計算は無限大やNaNになり得ます。整数版と異なり、浮動値の計算結果を範囲外例外へ変換しません。

検査有効時は非有限の浮動入力を`invalid_argument`、非optional照会の空表を`out_of_range`で拒否します。`SKIP_RUNTIME_CHECKS`はこれらを除外します。`lc_div`のゼロ除算は`domain_error`、整数の商がTに収まらない場合は`overflow_error`を常に投げます。

## 所有権と参照

包絡のノードは`std::multiset`が所有します。コピーは独立した包絡を持ち、ムーブ後の元も有効な状態で再利用できます。追加の確保失敗では元の包絡を保持します。コピー代入の例外保証は基底の`std::multiset`に従い、失敗時の元内容保持は保証しません。

公開の`begin()`・`end()`・`lower_bound(x)`は包絡の参照に使います。最小化でも`k,m`は元の符号で保存し、傾き順は最大化が昇順、最小化が降順です。整数の`p`は型の範囲へ制限されるため、真の交点そのものとは限りません。追加で削除された直線へのiteratorや参照は無効になります。

基底の`insert`・`erase`やmutable係数を直接変更すると、隣接交点との整合性が壊れます。更新には`add`と`clear`を使います。`insect(x,y)`は旧互換の内部交点更新で、利用者が通常操作として呼ぶ関数ではありません。

同梱の1D部品は2次元版と`SIRKIT_LINE_CONTAINER_DEFINED`を共有し、どちらを先に含めても重複定義しません。提出先は[Library Checker Line Add Get Min](https://judge.yosupo.jp/problem/line_add_get_min)です。最大化・浮動係数・追加API・整数端・所有権は独立ローカル検査で補います。出典とライセンスの調査根拠は[監査記録](../Audit/provenance-line-container-min-2026-10-22.json)に置きます。
