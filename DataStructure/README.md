# データ構造

各ライブラリは必要な標準ヘッダーを含み、単独で貼り付けられます。

`HashMap`と`HashSet`の既定のハッシュ関数は、利用者も直接使える`GenericHash`です。
内部専用のハッシュ表・比較関数・循環検査は`reinsirk::detail`に置きます。

| ファイル | 対応する検証 |
| --- | --- |
| [Binary_Trie.cpp](Binary_Trie.cpp) | [Set Xor-Min](../Verify/current/submissions/binary_trie.cpp) |
| [Compressed_Binary_Trie.cpp](Compressed_Binary_Trie.cpp) | [Set Xor-Min（圧縮版）](../Verify/current/submissions/compressed_binary_trie.cpp) |
| [Hash_map.cpp](Hash_map.cpp) | [Associative Array](../Verify/current/submissions/hash_map.cpp) |
| [Hash_set.cpp](Hash_set.cpp) | [A + B + HashSet independent unit test](../Verify/current/submissions/hash_set_unit.cpp) |
| [Node.cpp](Node.cpp) | [Node 独立ローカル検査](../Verify/current/submissions/node_local.cpp) |
| [dynamic-binary-indexed-tree-2d.cpp](dynamic-binary-indexed-tree-2d.cpp) | [Point Add Rectangle Sum](../Verify/current/submissions/dynamic_fenwick_2d_rectangle.cpp) |
| [dynamic-binary-indexed-tree.cpp](dynamic-binary-indexed-tree.cpp) | [Point Add Range Sum](../Verify/current/submissions/dynamic_fenwick_range.cpp) |
| [dynamic-li-chao-tree.cpp](dynamic-li-chao-tree.cpp) | [Segment Add Get Min](../Verify/current/submissions/dynamic_lichao_segment.cpp) |
| [dynamic-union-find.cpp](dynamic-union-find.cpp) | [Disjoint Set: Union Find Tree](../Verify/current/submissions/dynamic_union_find.cpp) |
| [erasable-priority-queue.cpp](erasable-priority-queue.cpp) | [Double-Ended Priority Queue](../Verify/current/submissions/erasable_priority_queue_double.cpp) |
| [heap.cpp](heap.cpp) | [Priority Queue](../Verify/current/submissions/heap_priority_queue.cpp) |
| [implicit_treap.cpp](implicit_treap.cpp) | [Dynamic Sequence Range Affine Range Sum](../Verify/current/submissions/implicit_treap_affine.cpp) |
| [line-container-2d.cpp](line-container-2d.cpp) | [Largest Triangle](../Verify/current/submissions/line_container_2d_triangle.cpp) |
| [line-container.cpp](line-container.cpp) | [Largest Triangle](../Verify/current/submissions/line_container_triangle.cpp) |
| [persistent-union-find.cpp](persistent-union-find.cpp) | [Persistent Unionfind](../Verify/current/submissions/persistent_union_find.cpp) |
| [rollback-union-find.cpp](rollback-union-find.cpp) | [Persistent Unionfind](../Verify/current/submissions/rollback_union_find.cpp) |
| [rollback-union-findw.cpp](rollback-union-findw.cpp) | [Weighted Union Find Trees](../Verify/current/submissions/rollback_weighted_union_find.cpp) |
| [union_find.cpp](union_find.cpp) | [Unionfind](../Verify/current/submissions/union_find.cpp) |
| [union_find_w.cpp](union_find_w.cpp) | [Weighted Union Find Trees](../Verify/current/submissions/weighted_union_find.cpp) |

問題URL、追加APIの補足検査、現在版の判定状況は[Verify一覧](../Verify/current/README.md)を参照してください。

## 二分トライの選択

`BinaryTrie`はビットごとのトライ、`CompressedBinaryTrie`は分岐しない経路を省く版です。どちらも重複・ID・順位照会・遅延XORを扱えます。
`insert_unique(x,id=-1)`は未登録時だけ追加してtrueを返し、`reserve(n)`で領域を予約できます。
使用例は[通常版](../Examples/binary_trie_example.cpp)・[圧縮版](../Examples/compressed_binary_trie_example.cpp)を参照してください。

## 動的BIT

[1次元](../Document/dynamic_fenwick.md)・[2次元](../Document/dynamic_fenwick_2d.md)の説明と、[1次元](../Examples/dynamic_fenwick_example.cpp)・[2次元](../Examples/dynamic_fenwick_2d_example.cpp)の使用例を参照してください。
現在の提出コードと判定の対応は[Verifyのmanifest](../Verify/current/manifest.json)で確認できます。
