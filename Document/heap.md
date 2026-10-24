# 最小ヒープ

`DataStructure/heap.cpp`の`Heap`は、`int`を所有する最小ヒープです。外部ライブラリに依存せず貼り付けられます。使用例は[heap_example.cpp](../Examples/heap_example.cpp)です。

## API

| 操作 | 意味・前提 | 計算量 |
| --- | --- | --- |
| `Heap()` | 空で構築 | O(1) |
| `Heap(vector<int>)` | 配列を受け取って構築。ムーブで所有権を渡せる | O(N) |
| `Heap({値...})`, `Heap(first,last)` | 値をコピーして構築 | O(N) |
| `empty()`, `size()` | 空判定・要素数 | O(1) |
| `top()`, `top_or(fallback)` | 最小値。空ならそれぞれ`-1`・`fallback` | O(1) |
| `push(value)` | 値を追加 | 償却O(log N) |
| `pop()` | 最小値を削除。空なら何もしない | O(log N) |
| `pop_value()` | 最小値を削除して返す | O(log N) |
| `replace_top(value)` | 最小値を置換し、旧値を返す | O(log N) |
| `push_pop(value)` | 追加後の最小値を削除して返す。空なら`value`を返す | O(log N) |
| `clear()` | 空にする。確保済み容量は保持 | O(1) |
| `reserve(n)` | `n`要素分の容量を確保 | 再確保時O(N) |

`pop_value()`と`replace_top()`は、空なら`std::out_of_range`を送出します。`SKIP_RUNTIME_CHECKS`を定義するとこの検査を除き、非空が前提となります。`top()`の`-1`は空の目印と値の区別がつかないため、必要なら`empty()`を併用します。

コピーは独立した配列を持ち、ムーブは配列の所有権を移します。負値・重複値・`int`の両端値も格納できます。

## 構造

連続した配列に二分ヒープを格納します。添字`i`の子は`2*i+1`と`2*i+2`です。追加は親を下へ移しながら挿入位置を求め、根の削除・置換は小さい子を上へ移して修復します。構築には標準の`std::make_heap`を使います。

## Verify

[ABC234 D - Prefix K-th Max](https://atcoder.jp/contests/abc234/tasks/abc234_d)では、先頭から追加し、要素数がKを超えたら最小値を削除します。[提出用コード](../Verify/current/submissions/heap_priority_queue.cpp)には共通の高速入出力と検査省略の設定を含めています。

[ローカル検査](../Verify/current/tests/heap_test.cpp)では、オンライン問題に含まれない置換・重複・負値・整数端点・コピー/ムーブ・空の扱いを`std::multiset`と照合します。判定・測定の証拠は`Audit/`に置きます。
