# バイト列の多重集合

`String/Trie.cpp` の `Trie` は文字列の重複を数え、接頭辞の照会と辞書順の操作を提供する。空文字列とNULを含む全256種類のバイトを扱い、辞書順は符号なしバイトの順とする。入力の文字種を制限しない。

| 操作 | 結果 |
|---|---|
| `insert(word, copies=1)` | 個数を追加し、その語の個数を返す。0個なら照会のみ |
| `erase(word, copies=1)` / `erase_all(word)` | 指定個数を削除できたか / すべて削除した個数 |
| `count(word)` / `contains(word)` | その語の個数 / 存在 |
| `prefix_count(prefix)` / `starts_with(prefix)` | 接頭辞を持つ語の総個数 / 存在 |
| `size()` / `distinct_size()` / `empty()` | 総個数 / 異なる語数 / 空判定 |
| `longest_prefix(text)` | 登録語のうちtextの最長接頭辞長。なければnullopt |
| `count_less(word)` | word未満の語の総個数 |
| `kth(k)` / `lower_bound(word)` | 重複込みの0始まりk番目 / word以上の最小語。なければnullopt |
| `items(prefix="", limit=SIZE_MAX)` | 接頭辞を持つ異なる語と個数を辞書順に最大limit件 |
| `node_count()` / `capacity()` | rootを含む使用ノード数 / 確保済みノード枠数 |
| `clear()` / `reserve_nodes(n)` / `swap(other)` | 全削除して容量を保持 / ノード枠の予約 / 内容と容量を交換 |

子はソート済みの兄弟リストで保持し、次数16で256要素の直接索引を追加、8未満で回収する。新しく連続確保した末尾は単一路として接続し、再利用する空きノードがある場合は空きリストを使う。削除したノードと索引の枠は再利用される。深い語の破棄・削除・列挙で再帰しない。

語長Lに対し登録・削除・通常の照会はO(16L+256)、総個数などはO(1)。登録の再確保では保持しているノード枠・索引枠のコピーも発生する。`count_less` はO(256L)、長さKの `kth` はO(256K)、`lower_bound` はO(256(L+K))。列挙は接頭辞の探索に加え、訪問したノード数と返す文字列の総長に比例する。コピーは保持ノード枠数N・索引枠数TとしてO(N+256T)、破棄・全削除はO(N+T)。

コピーは独立し、ムーブ元は空で再利用できる。個数不足の削除はfalseで不変。登録の確保失敗とコピー代入の失敗では論理的な内容を保持するが、確保済み容量が増える場合はある。個数と容量の上限は `SKIP_RUNTIME_CHECKS` にかかわらず保護する。

従来の `TrieNode` も別の公開型として残す。`children` のコピーは子の所有権を共有する。子が指すノード自体を所有する使い方を対象とし、別対象を所有するaliasingポインタと循環所有は対象外。こちらの破棄も反復処理で行う。

[使用例](../Examples/trie_example.cpp)と[独立した順序付き辞書との照合](../Verify/current/tests/trie_reference_test.cpp)を参照。

## Verify

提出用コードは[AtCoder C - gacha](https://atcoder.jp/contests/abc164/tasks/abc164_c)で、登録・存在確認・重複語と接頭辞を共有する語を扱う。英小文字・長さ10以下の問題なので、全APIの検証にはならない。

全バイト・空語・長い語・削除と再利用・重複個数・接頭辞照会・辞書順操作・所有権は、独立した順序付き辞書を使う `trie_reference_test.cpp` で照合する。索引の追加・回収時の確保失敗とコピー代入の状態保持は `radix_trie_allocation_test.cpp` で補う。
