# HashMap

`DataStructure/Hash_map.cpp`は、キーと値を所有する開番地法のハッシュ表です。`GenericHash`を独立に利用でき、独自の同値関係を使う場合は対応するHasherとKeyEqualを渡します。

検索・追加・次要素を探さないキー削除は期待償却O(1)です。ハッシュ計算の費用は別に必要で、列のキーでは列長に比例します。衝突が多い入力では時間が増えます。削除済みバケットを墓石として残し、再配置の際に除きます。

## 削除と列挙

`erase(key)`は対象を削除し、次の登録済み要素のiteratorを返します。`erase(iterator)`も同じです。空きバケットを飛ばすため、削除一回の最悪時間はO(capacity())です。返されたiteratorを順に使って全要素を削除する場合は、表全体の走査がO(capacity())に収まります。

次要素が不要なら`erase(key, false)`または`erase(iterator, false)`を使います。キー版は期待O(1)、有効なiteratorを渡す版はO(1)で削除し、`end()`を返します。未登録キーや`end()`の削除は何もしません。

空表の`begin()`は、予約容量によらずO(1)で`end()`を返します。`clear()`は登録要素も墓石もなければ配列の走査を省きます。全要素を削除しただけの表には墓石が残るため、次の`clear()`では通常の初期化を行います。

```cpp
HashMap<int, int> counts;
counts[7] = 3;
counts.erase(7, false);
for (auto it = counts.begin(); it != counts.end();) it = counts.erase(it);
```

## 所有と前提

コピーは独立した表を作ります。再配置・clear・swap・ムーブでiteratorと要素参照が無効になります。再配置を伴う追加に、同じ表の要素参照を渡す場合は先にコピーしてください。

`SKIP_RUNTIME_CHECKS`を定義するとiteratorの所有元・世代・有効性の検査を除きます。容量オーバーフローの保護と要素の破棄は維持します。

使用例は`Examples/hash_map_example.cpp`、提出用コードは`Verify/current/submissions/hash_map.cpp`です。Associative Arrayでは主に追加・更新・検索を検証し、削除・例外・所有操作は`Verify/current/tests/hash_map_test.cpp`と`hash_map_boundary_test.cpp`、`hash_map_storage_test.cpp`で独立に照合します。

## 再配置とSIMD

GCCのx86では、自明にコピー・破棄できる要素の未使用バケットを未構築のまま保持し、登録時に値を構築します。配列の確保時には、値の構築前に領域の一部を順に書き、最初のランダムな書き込みの負担を抑えます。Clangとその他の対象環境では従来の初期化を使います。

GCCのSSE2対応x86では、自明にコピー・破棄できる要素の表が64バケット以上ある場合、再配置時の登録状態を16バケットずつ判定します。キーと値は登録順序に依存しない通常の再配置で移し、ハッシュが例外を送出しても元の表を保持します。

小さい表、非自明な要素、Clang、その他の対象環境では標準C++のループを使います。`SIRKIT_DISABLE_SIMD`を定義するとSIMD専用コードを除外できます。GCC向けの未使用値の構築省略は維持します。検索・追加・削除の通常の探索手順は共通です。

## HashSet

値を持たない集合は[HashSetの説明](hash_set.md)を参照してください。
