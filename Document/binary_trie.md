# 整数の二分トライ

[BinaryTrie](../DataStructure/Binary_Trie.cpp)と[CompressedBinaryTrie](../DataStructure/Compressed_Binary_Trie.cpp)は、非負整数の多重集合を扱います。全要素へのXOR、順位、XOR値の最小・最大を照会できます。[通常版の使用例](../Examples/binary_trie_example.cpp)と[圧縮版の使用例](../Examples/compressed_binary_trie_example.cpp)を参照してください。

## 型と範囲

`T`は`bool`・`const`・`volatile`を除く整数型です。`MAX_LOG`は最高位ビット番号で、B=`MAX_LOG+1`とします。キーとXORマスクは`0 <= x < 2^B`の範囲とし、符号付きTでは非負の範囲に収めます。例えば`std::uint32_t,29`は30bit、`std::uint64_t,63`は64bitすべてを使えます。

`MAX_LOG`は省略でき、既定値は`std::numeric_limits<T>::digits - 1`です。両クラスとも、符号なし型では全ビット、符号付き型では符号ビットを除く非負の範囲を扱います。第2テンプレート引数による手動指定もできます。

```cpp
BinaryTrie<std::uint32_t> full;                  // 32bit（MAX_LOG=31）。
BinaryTrie<std::uint32_t, 29> limited;           // 手動で30bitに限定。
CompressedBinaryTrie<std::uint64_t> compressed; // 64bit（MAX_LOG=63）。
CompressedBinaryTrie<std::uint64_t, 29> small;   // 手動で30bitに限定。
```

通常版はビットごとに木をたどるため、値の範囲が分かっている場合は`MAX_LOG`を小さく指定すると、探索する段数と必要なノード数を抑えられます。

`NODES`は根を含む未圧縮の二分トライのノード上限です。圧縮版も同じ制限を使い、圧縮した実ノードの個数だけで判定するものではありません。どちらもコピー・ムーブはできません。ノードとID配列はライブラリが所有し、再確保や破棄に利用者の処理は不要です。

## 操作

以下は両方に共通します。`size_type`は`std::size_t`、ID一覧は`std::vector<int>`への読み取り専用参照です。

| API | 結果 |
| --- | --- |
| `size()` / `empty()` | 重複込みの要素数 / 空か |
| `add(x,id=-1)` | xを1個追加する |
| `insert_unique(x,id=-1)` | 未登録時だけ追加してtrue。登録済みなら個数・IDを変えずfalse |
| `del(x,id=-1)` | 1個削除してtrue。値や指定IDがなければfalse |
| `find(x)` | `{xの個数, xのID一覧}`。未登録なら`{0, 空配列}` |
| `count(x)` / `contains(x)` | xの個数 / 存在 |
| `min_element()` / `max_element()` | `{最小値/最大値, その値のID一覧}`。空でないこと |
| `get_kth(std::int64_t k)` | 重複込み・1始まりの`{k番目の値, ID一覧}`。`1 <= k <= size()` |
| `operate_xor(x)` | 全要素にXORを適用する。以後`add(y)`で追加する公開値はy |
| `count_less(upper)` | upper未満の要素数。整数型のupperが0以下なら0、キー範囲を超えればsize() |
| `min_xor(x)` / `max_xor(x)` | `{x ^ yの最小値/最大値, 対応するyのID一覧}`。空でないこと |
| `count_xor_less(x,upper)` | `x ^ y < upper`となる要素数。upperの扱いはcount_lessと同じ |

IDは重複でき、`-1`は匿名要素を表します。匿名要素はID一覧に含めません。ID省略の削除は匿名要素を優先し、匿名がなければ任意のID付き要素を1個削除します。一覧の順序は規定せず、返された参照は次の更新または破棄まで有効です。

`size`・`empty`・`operate_xor`はO(1)、検索・順位・XOR照会・削除はO(B)です。指定IDの削除には、その値のID数に比例する探索も加わります。通常版の追加は償却O(B)、圧縮版の追加は償却O(B)で、ノード配列の再配置時には保持ノード数に比例する処理も発生します。ID追加は配列の拡張を伴うことがあります。

## 記憶領域と圧縮

通常版は各ビットに1段のノードを置きます。最初の最大4096ノードを構築時に確保し、その後は最大8192ノードずつ増やします。削除で空いたノードは再利用し、ブロックの拡張でもノードのアドレスを保ちます。

圧縮版は値が分かれるビットだけを内点に持ちます。非空でD種類の値を持つ場合、生存する実ノードは`2D-1`個です。確保済みの領域は削除で縮めず再利用するため、保持量は過去の確保量にも依存します。`reserve(n)`は異なる値n個分の圧縮ノード領域を上限内で予約します。再配置時は保持ノード数に比例し、未圧縮の`NODES`制限は緩和しません。

圧縮版の内点のキーは、分岐ビットより上の共通接頭辞を表します。保存時のキーは公開値と遅延XORのXORです。検索や削除では、まず根の共通接頭辞と比較し、不一致なら子をたどらず不存在を返します。代表として記録した値が削除されても、内点が残る限り、その上位接頭辞は残る全要素に共通です。葉の負の分岐番号ではシフトせず、64bit最上位の分岐でも型幅以上のシフトを使いません。

## 検査と失敗

既定では負のキー、キー範囲外、空集合の最小・最大照会、不正な順位で例外を投げます。`SKIP_RUNTIME_CHECKS`を定義するとこれらの入力検査を除き、利用者が前提を満たす必要があります。ノード容量・添字・要素数のオーバーフロー保護は維持します。

追加に失敗した場合も値・個数・IDを保持しますが、確保済み容量が増える場合はあります。存在しない値やIDの削除はfalseを返し、内容を変えません。

提出先は[Library Checker Set Xor-Min](https://judge.yosupo.jp/problem/set_xor_min)です。重複・ID・遅延XOR・順位・容量境界・確保失敗は[多重集合参照](../Verify/current/tests/binary_trie_reference_test.cpp)、[所有権と確保失敗](../Verify/current/tests/binary_trie_allocation_test.cpp)、[圧縮接頭辞の境界](../Verify/current/tests/compressed_binary_trie_prefix_test.cpp)で補います。測定条件と検証・出典の記録は[Audit](../Audit/)に置きます。
