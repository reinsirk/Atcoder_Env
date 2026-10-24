# データ構造

各ライブラリは必要な標準ヘッダーを含み、単独で貼り付けられます。

`HashMap`と`HashSet`の既定のハッシュ関数は、利用者も直接使える`GenericHash`です。
内部専用のハッシュ表・比較関数・循環検査は`reinsirk::detail`に置きます。
HashMapの削除とiteratorの計算量は[説明](../Document/hash_map.md)、使用例は[こちら](../Examples/hash_map_example.cpp)を参照してください。
可変長列の集約・遅延作用・所有の扱いは[暗黙Treapの説明](../Document/implicit_treap.md)を参照してください。
点の追加と内積の最適化は[2次元LineContainerの説明](../Document/line_container_2d.md)と[使用例](../Examples/line_container_2d_example.cpp)を参照してください。
直線の追加と最小値・最大値の照会は[LineContainerの説明](../Document/line_container.md)と[使用例](../Examples/line_container_example.cpp)を参照してください。

| ファイル | 対応する検証 |
| --- | --- |
| [Binary_Trie.cpp](Binary_Trie.cpp) | [Set Xor-Min](../Verify/current/submissions/binary_trie.cpp) |
| [Compressed_Binary_Trie.cpp](Compressed_Binary_Trie.cpp) | [Set Xor-Min（圧縮版）](../Verify/current/submissions/compressed_binary_trie.cpp) |
| [Hash_map.cpp](Hash_map.cpp) | [Associative Array](../Verify/current/submissions/hash_map.cpp) |
| [Hash_set.cpp](Hash_set.cpp) | [整数追加・検索](../Verify/current/submissions/hash_set_membership.cpp)・[追加・削除](../Verify/current/submissions/hash_set_toggle.cpp)・[文字列](../Verify/current/submissions/hash_set_strings.cpp) |
| [Node.cpp](Node.cpp) | [Node 独立ローカル検査](../Verify/current/submissions/node_local.cpp) |
| [dynamic-binary-indexed-tree-2d.cpp](dynamic-binary-indexed-tree-2d.cpp) | [Point Add Rectangle Sum](../Verify/current/submissions/dynamic_fenwick_2d_rectangle.cpp) |
| [dynamic-binary-indexed-tree.cpp](dynamic-binary-indexed-tree.cpp) | [Point Add Range Sum](../Verify/current/submissions/dynamic_fenwick_range.cpp) |
| [dynamic-li-chao-tree.cpp](dynamic-li-chao-tree.cpp) | [Segment Add Get Min](../Verify/current/submissions/dynamic_lichao_segment.cpp) |
| [dynamic-union-find.cpp](dynamic-union-find.cpp) | [Disjoint Set: Union Find Tree](../Verify/current/submissions/dynamic_union_find.cpp) |
| [erasable-priority-queue.cpp](erasable-priority-queue.cpp) | [Double-Ended Priority Queue](../Verify/current/submissions/erasable_priority_queue_double.cpp) |
| [heap.cpp](heap.cpp) | [Prefix K-th Max](../Verify/current/submissions/heap_priority_queue.cpp)・[説明](../Document/heap.md) |
| [implicit_treap.cpp](implicit_treap.cpp) | [区間アフィン作用・和](../Verify/current/submissions/implicit_treap_affine.cpp)・[両端操作・関数合成](../Verify/current/submissions/implicit_treap_deque.cpp) |
| [line-container-2d.cpp](line-container-2d.cpp) | [Line Add Get Min](../Verify/current/submissions/line_container_2d_min.cpp) |
| [line-container.cpp](line-container.cpp) | [Line Add Get Min](../Verify/current/submissions/line_container_min.cpp) |
| [persistent-union-find.cpp](persistent-union-find.cpp) | [Persistent Unionfind](../Verify/current/submissions/persistent_union_find.cpp) |
| [rollback-union-find.cpp](rollback-union-find.cpp) | [Persistent Unionfind](../Verify/current/submissions/rollback_union_find.cpp) |
| [rollback-union-findw.cpp](rollback-union-findw.cpp) | [Unionfind with Potential](../Verify/current/submissions/rollback_weighted_union_find.cpp) |
| [union_find.cpp](union_find.cpp) | [Unionfind](../Verify/current/submissions/union_find.cpp) |
| [union_find_w.cpp](union_find_w.cpp) | [Unionfind with Potential](../Verify/current/submissions/weighted_union_find.cpp) |

問題URL、追加APIの補足検査、現在版の判定状況は[Verify一覧](../Verify/current/README.md)を参照してください。

## 二分トライの選択

`BinaryTrie`はビットごとのトライ、`CompressedBinaryTrie`は分岐しない経路を省く版です。どちらも重複・ID・順位照会・遅延XORを扱えます。
`insert_unique(x,id=-1)`は未登録時だけ追加してtrueを返し、`reserve(n)`で領域を予約できます。
方式・APIの前提は[説明](../Document/binary_trie.md)、使用例は[通常版](../Examples/binary_trie_example.cpp)・[圧縮版](../Examples/compressed_binary_trie_example.cpp)を参照してください。

## 動的BIT

[1次元](../Document/dynamic_fenwick.md)・[2次元](../Document/dynamic_fenwick_2d.md)の説明と、[1次元](../Examples/dynamic_fenwick_example.cpp)・[2次元](../Examples/dynamic_fenwick_2d_example.cpp)の使用例を参照してください。
現在の提出コードと判定の対応は[Verifyのmanifest](../Verify/current/manifest.json)で確認できます。

永続Union Findと永続配列の版・所有権・2点更新は[説明](../Document/persistent_union_find.md)と[使用例](../Examples/persistent_union_find_example.cpp)を参照してください。

巻き戻しUnion Findの履歴・snapshot・履歴消去は[説明](../Document/rollback_union_find.md)と[使用例](../Examples/rollback_union_find_example.cpp)を参照してください。

重み付き巻き戻しUnion Findの差・整合性・整数範囲は[説明](../Document/rollback_weighted_union_find.md)と[使用例](../Examples/rollback_weighted_union_find_example.cpp)を参照してください。

Union Findの代表・経路圧縮・初期化は[説明](../Document/union_find.md)と[使用例](../Examples/union_find_example.cpp)を参照してください。

重み付きUnion Findの差・整合性・型の前提は[説明](../Document/weighted_union_find.md)と[使用例](../Examples/union_find_w_example.cpp)を参照してください。
