# 現在版のVerify

## 使い方

1. 下表の問題を開き、リンク先の提出コードをそのまま貼り付けます。ライブラリ本体を埋め込み済みです。
2. C++23を選びます。提出コード先頭には速度確認用の`#define SKIP_RUNTIME_CHECKS`を定義しています。
3. 提出URLはコードの`Submission`コメントに最新版だけを記載し、[manifest](manifest.json)でソースと判定の対応を確認します。

オンラインACはソースが一致する版だけに対応付けます。ローカル検査はオンライン判定と区別します。

## mainの書き方

[共通高速入出力](common/fast_io.hpp)を使い、mainは初期化・問題形式とAPIの変換・素直なAPI呼び出し・結果出力に留めます。照会のバッチ化・並べ替え・入力分布による実装切り替えは加えません。入出力内部のバッファリングは使用します。
使用例は読みやすい`cin/cout`を基本とします。本体の速度比較は同じ問題・main・入出力・コンパイラ設定・検査設定で行います。

## 共通入出力の扱い

- `verify_in >> 値`、`verify_out << 値`を使います。入力変数に未初期化の警告が出る箇所は値初期化しています。
- `verify_out.fixed(12)`で固定小数出力を指定します。出力バッファは終了時にflushします。
- `SKIP_RUNTIME_CHECKS`時は型に収まる正しい入力が前提です。入力検査を除いても、EOF確認・バッファ境界・符号付き最小値の安全な変換は維持します。
- 生成器が共通ヘッダーを展開するため、提出コードは別ファイルのincludeに依存しません。標準の`cin/cout`や別の入出力方式とは混用しません。
- [manifest](manifest.json)の`driver_sha256`・`io_mode`・`io_source_hashes`で、計算手順と入出力の比較条件を記録します。
- [入出力の境界検査](tests/fast_io_test.cpp)は、リポジトリ直下から`g++ -std=c++23 -O2 Verify/current/tests/fast_io_test.cpp -o /tmp/verify_io_test`でビルドできます。検査省略時は`-DSKIP_RUNTIME_CHECKS`を追加します。

## 検証範囲

Verify先はLibrary Checkerを第一候補とし、対応問題がなければAtCoderを使います。AOJは時間分解能のため提出先に選びません。対応問題のないAPIは独立参照や入力例のローカル検査で補います。
検証するAPI・入力範囲は[manifest](manifest.json)の`coverage`と`limitations`を参照してください。
偏角ソートには[angle_sort版](submissions/angle_sort.cpp)と[std::sortへ比較関数を渡す版](submissions/angle_sort_std.cpp)を用意しています。
整数二項係数の素数法問題では、提出コード側で`table_limit=9'999'999`、前計算範囲を`min(modulus-1,9'999'999)`に設定しています。設定方法は[二項係数の説明](../../Math/Combination/README.md)を参照してください。

## 提出環境と依存

- 提出用コードは提出先のACLインストールに依存しません。二項係数のMintには[公式ACL](https://github.com/atcoder/ac-library)のmodintと必要な内部ヘッダーを展開済みです。
- 使用したACLの版は[vendor/ACL_COMMIT](vendor/ACL_COMMIT)、ライセンスは[vendor/ACL_LICENSE](vendor/ACL_LICENSE)に保存しています。他の提出コードは標準ヘッダーだけを使います。
- 旧AOJ・Yukicoderの入力例はローカル回帰として保持しています。現在のオンライン提出先やAC判定には数えません。
- 全コードを512 KiB以下に制限しています。上限は各提出ファイル単位です。
- `local`のコードは提出用の問題を持たない補足検査です。HashSetと削除可能PQのA+Bは原典のunit test形式に合わせ、独立照合を実行してからA+Bを出力します。A+Bの入力自体がデータ構造を検証するものではありません。

## ライブラリ別の入口

| ライブラリ | 主な提出コード／補足検査 |
| --- | --- |
| [Graph/Bellman-Ford.cpp](../../Graph/Bellman-Ford.cpp) | [E - Coins Respawn](submissions/bellman_ford.cpp) |
| [Graph/Bellman-Ford_SFPA.cpp](../../Graph/Bellman-Ford_SFPA.cpp) | [E - Coins Respawn](submissions/spfa.cpp) |
| [Graph/Dijkstra.cpp](../../Graph/Dijkstra.cpp) | [Shortest Path](submissions/dijkstra.cpp) |
| [Graph/Floyd-Warshall.cpp](../../Graph/Floyd-Warshall.cpp) | [D - バスと避けられない運命](submissions/floyd_warshall.cpp) |
| [Graph/LCA.cpp](../../Graph/LCA.cpp) | [Lowest Common Ancestor](submissions/lca.cpp) |
| [Graph/Project_Selection.cpp](../../Graph/Project_Selection.cpp) | [040 - Get More Money（★7）](submissions/project_selection.cpp) |
| [Graph/Topological.cpp](../../Graph/Topological.cpp) | [G - Longest Path](submissions/topological.cpp) |
| [Graph/Topological_sort.cpp](../../Graph/Topological_sort.cpp) | [D - Restricted Permutation](submissions/topological_lexicographical.cpp) |
| [DataStructure/Binary_Trie.cpp](../../DataStructure/Binary_Trie.cpp) | [Set Xor-Min](submissions/binary_trie.cpp) |
| [DataStructure/Compressed_Binary_Trie.cpp](../../DataStructure/Compressed_Binary_Trie.cpp) | [Set Xor-Min](submissions/compressed_binary_trie.cpp) |
| [DataStructure/Hash_map.cpp](../../DataStructure/Hash_map.cpp) | [Associative Array](submissions/hash_map.cpp) |
| [DataStructure/Hash_set.cpp](../../DataStructure/Hash_set.cpp) | [A + B + HashSet independent unit test](submissions/hash_set_unit.cpp) |
| [DataStructure/Node.cpp](../../DataStructure/Node.cpp) | [Node 独立ローカル検査](submissions/node_local.cpp) |
| [DataStructure/dynamic-binary-indexed-tree-2d.cpp](../../DataStructure/dynamic-binary-indexed-tree-2d.cpp) | [Point Add Rectangle Sum](submissions/dynamic_fenwick_2d_rectangle.cpp) |
| [DataStructure/dynamic-binary-indexed-tree.cpp](../../DataStructure/dynamic-binary-indexed-tree.cpp) | [Point Add Range Sum](submissions/dynamic_fenwick_range.cpp) |
| [DataStructure/dynamic-li-chao-tree.cpp](../../DataStructure/dynamic-li-chao-tree.cpp) | [Segment Add Get Min](submissions/dynamic_lichao_segment.cpp) |
| [DataStructure/dynamic-union-find.cpp](../../DataStructure/dynamic-union-find.cpp) | [Unionfind](submissions/dynamic_union_find.cpp) |
| [DataStructure/erasable-priority-queue.cpp](../../DataStructure/erasable-priority-queue.cpp) | [Double-Ended Priority Queue](submissions/erasable_priority_queue_double.cpp) |
| [DataStructure/heap.cpp](../../DataStructure/heap.cpp) | [D - Prefix K-th Max](submissions/heap_priority_queue.cpp) |
| [DataStructure/implicit_treap.cpp](../../DataStructure/implicit_treap.cpp) | [Dynamic Sequence Range Affine Range Sum](submissions/implicit_treap_affine.cpp) |
| [DataStructure/line-container-2d.cpp](../../DataStructure/line-container-2d.cpp) | [Line Add Get Min](submissions/line_container_2d_min.cpp) |
| [DataStructure/line-container.cpp](../../DataStructure/line-container.cpp) | [Line Add Get Min](submissions/line_container_min.cpp) |
| [DataStructure/persistent-union-find.cpp](../../DataStructure/persistent-union-find.cpp) | [Persistent Unionfind](submissions/persistent_union_find.cpp) |
| [DataStructure/rollback-union-find.cpp](../../DataStructure/rollback-union-find.cpp) | [Persistent Unionfind](submissions/rollback_union_find.cpp) |
| [DataStructure/rollback-union-findw.cpp](../../DataStructure/rollback-union-findw.cpp) | [Unionfind with Potential](submissions/rollback_weighted_union_find.cpp) |
| [DataStructure/union_find.cpp](../../DataStructure/union_find.cpp) | [Unionfind](submissions/union_find.cpp) |
| [DataStructure/union_find_w.cpp](../../DataStructure/union_find_w.cpp) | [Unionfind with Potential](submissions/weighted_union_find.cpp) |
| [Math/Combination/Binomial_Mod.cpp](../../Math/Combination/Binomial_Mod.cpp) | [Binomial Coefficient](submissions/binomial_integer_split.cpp) |
| [Math/Combination/ModInt_Binomial.cpp](../../Math/Combination/ModInt_Binomial.cpp) | [Binomial Coefficient](submissions/binomial_modint_split.cpp) |
| [Math/Combination/Prime_Binomial.cpp](../../Math/Combination/Prime_Binomial.cpp) | [Binomial Coefficient (Prime Mod)](submissions/binomial_prime_split.cpp) |
| [Math/Combination_AtCoderMod.cpp](../../Math/Combination_AtCoderMod.cpp) | [Binomial Coefficient (Prime Mod)](submissions/binomial_prime.cpp) |
| [Math/Combination_Mod.cpp](../../Math/Combination_Mod.cpp) | [Binomial Coefficient](submissions/binomial_integer.cpp) |
| [Math/Eratosthenes.cpp](../../Math/Eratosthenes.cpp) | [Enumerate Primes](submissions/eratosthenes.cpp) |
| [Math/Zeta/Bitwise_Convolution.cpp](../../Math/Zeta/Bitwise_Convolution.cpp) | [Bitwise And Convolution](submissions/zeta_and_split.cpp) |
| [Math/Zeta/Divisor_Multiple_Transform.cpp](../../Math/Zeta/Divisor_Multiple_Transform.cpp) | [GCD Convolution](submissions/zeta_gcd_transform_split.cpp) |
| [Math/Zeta/Gcd_Lcm_Convolution.cpp](../../Math/Zeta/Gcd_Lcm_Convolution.cpp) | [GCD Convolution](submissions/zeta_gcd_conv_split.cpp) |
| [Math/Zeta/Set_Transform.cpp](../../Math/Zeta/Set_Transform.cpp) | [Bitwise And Convolution](submissions/zeta_set_as_and_split.cpp) |
| [Math/Zeta/Sieve.cpp](../../Math/Zeta/Sieve.cpp) | [D - Div Game](submissions/zeta_sieve_factorize_split.cpp) |
| [Math/Zeta/Sparse_Transform.cpp](../../Math/Zeta/Sparse_Transform.cpp) | [GCD Convolution](submissions/zeta_sparse_gcd_split.cpp) |
| [Math/Zeta_Mebius.cpp](../../Math/Zeta_Mebius.cpp) | [Bitwise And Convolution](submissions/zeta_and_full.cpp) |
| [Math/prime_judge(Miller-Rabin).cpp](../../Math/prime_judge(Miller-Rabin).cpp) | [Primality Test](submissions/primality.cpp) |
| [String/RollingHash.cpp](../../String/RollingHash.cpp) | [Z Algorithm](submissions/rolling_hash.cpp) |
| [String/Trie.cpp](../../String/Trie.cpp) | [C - gacha](submissions/trie_dictionary.cpp) |
| [Geometry/Circle.cpp](../../Geometry/Circle.cpp) | [035 - Two Circles](submissions/geometry_circle_relation_split.cpp) |
| [Geometry/Line_Segment.cpp](../../Geometry/Line_Segment.cpp) | [037 - Intersection](submissions/geometry_segment_intersection_atcoder_split.cpp) |
| [Geometry/Nearest_Two_points.cpp](../../Geometry/Nearest_Two_points.cpp) | [Closest Pair of Points](submissions/nearest_two_points.cpp) |
| [Geometry/Point.cpp](../../Geometry/Point.cpp) | [C - Convex Quadrilateral](submissions/geometry_orientation_atcoder_split.cpp) |
| [Geometry/Polygon.cpp](../../Geometry/Polygon.cpp) | [Static Convex Hull](submissions/geometry_convex_hull_split.cpp) |
| [Geometry/geometory.cpp](../../Geometry/geometory.cpp) | [Static Convex Hull](submissions/geometry_convex_hull_full.cpp) |
| [Geometry/偏角ソート.cpp](../../Geometry/偏角ソート.cpp) | [Sort Points by Argument](submissions/angle_sort.cpp) |
| [Algorithm/LIS.cpp](../../Algorithm/LIS.cpp) | [Longest Increasing Subsequence](submissions/lis.cpp) |
| [Algorithm/Radix_sort.cpp](../../Algorithm/Radix_sort.cpp) | [C - Sum of Numbers Greater Than Me](submissions/radix_sort.cpp) |
| [Algorithm/next_combination.cpp](../../Algorithm/next_combination.cpp) | [E - Modulo MST](submissions/next_combination.cpp) |
| [Algorithm/rotate.cpp](../../Algorithm/rotate.cpp) | [B - Coloring Matrix](submissions/matrix_rotate.cpp) |
| [Internal/Nyaan_hash_map_min.cpp](../../Internal/Nyaan_hash_map_min.cpp) | [Associative Array](submissions/internal_hash_map.cpp) |

## 提出用コード一覧

| ID・コード | 提出先 | 主な対象 |
| --- | --- | --- |
| [angle_sort](submissions/angle_sort.cpp) | [Sort Points by Argument](https://judge.yosupo.jp/problem/sort_points_by_argument) | 昇順angle_sort・零ベクトル |
| [angle_sort_std](submissions/angle_sort_std.cpp) | [Sort Points by Argument](https://judge.yosupo.jp/problem/sort_points_by_argument) | std::sortへarccmp_lessを直接渡す昇順・零ベクトル |
| [binary_trie](submissions/binary_trie.cpp) | [Set Xor-Min](https://judge.yosupo.jp/problem/set_xor_min) | insert_unique・重複抑止・削除・最小XOR。共通高速入出力。 |
| [binomial_integer](submissions/binomial_integer.cpp) | [Binomial Coefficient](https://judge.yosupo.jp/problem/binomial_coefficient) | 任意法（素数版は素数法）のCOM。 |
| [binomial_integer_prime_mod](submissions/binomial_integer_prime_mod.cpp) | [Binomial Coefficient (Prime Mod)](https://judge.yosupo.jp/problem/binomial_coefficient_prime_mod) | BinomialModの素数法COM。提出側でtable_limit=9999999、max_n=min(modulus-1,9999999)を指定。 |
| [binomial_integer_prime_mod_split](submissions/binomial_integer_prime_mod_split.cpp) | [Binomial Coefficient (Prime Mod)](https://judge.yosupo.jp/problem/binomial_coefficient_prime_mod) | BinomialModの素数法COM。提出側でtable_limit=9999999、max_n=min(modulus-1,9999999)を指定。 |
| [binomial_integer_split](submissions/binomial_integer_split.cpp) | [Binomial Coefficient](https://judge.yosupo.jp/problem/binomial_coefficient) | 任意法（素数版は素数法）のCOM。 |
| [binomial_modint](submissions/binomial_modint.cpp) | [Binomial Coefficient](https://judge.yosupo.jp/problem/binomial_coefficient) | 任意法（素数版は素数法）のCOM。 |
| [binomial_modint_prime_mod](submissions/binomial_modint_prime_mod.cpp) | [Binomial Coefficient (Prime Mod)](https://judge.yosupo.jp/problem/binomial_coefficient_prime_mod) | ModIntBinomial<dynamic_modint>の素数法COM。提出側でtable_limit=9999999、max_n=min(modulus-1,9999999)を指定。 |
| [binomial_modint_prime_mod_split](submissions/binomial_modint_prime_mod_split.cpp) | [Binomial Coefficient (Prime Mod)](https://judge.yosupo.jp/problem/binomial_coefficient_prime_mod) | ModIntBinomial<dynamic_modint>の素数法COM。提出側でtable_limit=9999999、max_n=min(modulus-1,9999999)を指定。 |
| [binomial_modint_split](submissions/binomial_modint_split.cpp) | [Binomial Coefficient](https://judge.yosupo.jp/problem/binomial_coefficient) | 任意法（素数版は素数法）のCOM。 |
| [binomial_prime](submissions/binomial_prime.cpp) | [Binomial Coefficient (Prime Mod)](https://judge.yosupo.jp/problem/binomial_coefficient_prime_mod) | 任意法（素数版は素数法）のCOM。 |
| [binomial_prime_split](submissions/binomial_prime_split.cpp) | [Binomial Coefficient (Prime Mod)](https://judge.yosupo.jp/problem/binomial_coefficient_prime_mod) | 任意法（素数版は素数法）のCOM。 |
| [compressed_binary_trie](submissions/compressed_binary_trie.cpp) | [Set Xor-Min](https://judge.yosupo.jp/problem/set_xor_min) | 圧縮版のinsert_unique・重複抑止・削除・最小XOR。共通高速入出力。 |
| [dijkstra](submissions/dijkstra.cpp) | [Shortest Path](https://judge.yosupo.jp/problem/shortest_path) | 距離と経路復元、未到達。 |
| [dynamic_fenwick_2d_rectangle](submissions/dynamic_fenwick_2d_rectangle.cpp) | [Point Add Rectangle Sum](https://judge.yosupo.jp/problem/point_add_rectangle_sum) | 点加算・半開矩形和・オフライン座標圧縮による疎座標対応 |
| [dynamic_fenwick_range](submissions/dynamic_fenwick_range.cpp) | [Point Add Range Sum](https://judge.yosupo.jp/problem/point_add_range_sum) | add・sum(l,r) |
| [dynamic_fenwick_rectangle](submissions/dynamic_fenwick_rectangle.cpp) | [Point Add Rectangle Sum](https://judge.yosupo.jp/problem/point_add_rectangle_sum) | 点加算・半開矩形和・オフライン座標圧縮による疎座標対応 |
| [dynamic_lichao_line](submissions/dynamic_lichao_line.cpp) | [Line Add Get Min](https://judge.yosupo.jp/problem/line_add_get_min) | add_line・query・空の照会 |
| [dynamic_lichao_segment](submissions/dynamic_lichao_segment.cpp) | [Segment Add Get Min](https://judge.yosupo.jp/problem/segment_add_get_min) | add_segment・query・空の照会 |
| [dynamic_union_find](submissions/dynamic_union_find.cpp) | [Unionfind](https://judge.yosupo.jp/problem/unionfind) | unite・same・未登録要素の自動登録 |
| [erasable_priority_queue_double](submissions/erasable_priority_queue_double.cpp) | [Double-Ended Priority Queue](https://judge.yosupo.jp/problem/double_ended_priority_queue) | push・pop・top・erase・重複要素・最小化/最大化 |
| [erasable_priority_queue_unit](submissions/erasable_priority_queue_unit.cpp) | [A + B + ErasablePriorityQueue independent unit test](https://judge.yosupo.jp/problem/aplusb) | 各最小化/最大化の固定種10000更新をstd::multisetと独立照合・vector一括構築/push/emplace/erase/top/pop・try_top/extract_top/size/empty・copy/move/clear |
| [eratosthenes](submissions/eratosthenes.cpp) | [Enumerate Primes](https://judge.yosupo.jp/problem/enumerate_primes) | PrimeSieveの構築、素数列挙・個数（上限5億）。 |
| [geometry_convex_hull_full](submissions/geometry_convex_hull_full.cpp) | [Static Convex Hull](https://judge.yosupo.jp/problem/static_convex_hull) | convex_hull: 整数点の最小凸包、反時計回り・重複除去・辺上中間点除去・空/1/2点。 |
| [geometry_convex_hull_split](submissions/geometry_convex_hull_split.cpp) | [Static Convex Hull](https://judge.yosupo.jp/problem/static_convex_hull) | convex_hull: 整数点の最小凸包、反時計回り・重複除去・辺上中間点除去・空/1/2点。 |
| [hash_map](submissions/hash_map.cpp) | [Associative Array](https://judge.yosupo.jp/problem/associative_array) | operator[]・64ビット整数キー/値・未登録時の零初期値 |
| [hash_set_unit](submissions/hash_set_unit.cpp) | [A + B + HashSet independent unit test](https://judge.yosupo.jp/problem/aplusb) | 固定種5000更新をstd::setと独立照合・insert/insert_checked/erase/find/contains/count/size/empty/iterator・reserve/clear/copy/move/erase_if・整数・pair・string・vectorキー |
| [implicit_treap_affine](submissions/implicit_treap_affine.cpp) | [Dynamic Sequence Range Affine Range Sum](https://judge.yosupo.jp/problem/dynamic_sequence_range_affine_range_sum) | 線形一括構築・insert_at・erase_at・reverse・apply・prod・非自明な作用の合成 |
| [internal_hash_map](submissions/internal_hash_map.cpp) | [Associative Array](https://judge.yosupo.jp/problem/associative_array) | operator[]・64ビット整数キー/値・未登録時の零初期値 |
| [lca](submissions/lca.cpp) | [Lowest Common Ancestor](https://judge.yosupo.jp/problem/lca) | 構築・get |
| [line_container_2d_min](submissions/line_container_2d_min.cpp) | [Line Add Get Min](https://judge.yosupo.jp/problem/line_add_get_min) | 点追加・整数方向(x,1)のmin_rounded_ll |
| [line_container_min](submissions/line_container_min.cpp) | [Line Add Get Min](https://judge.yosupo.jp/problem/line_add_get_min) | 整数add・整数query |
| [lis](submissions/lis.cpp) | [Longest Increasing Subsequence](https://judge.yosupo.jp/problem/longest_increasing_subsequence) | LISIndices<true> |
| [nearest_two_points](submissions/nearest_two_points.cpp) | [Closest Pair of Points](https://judge.yosupo.jp/problem/closest_pair) | 最近点対と元入力添字 |
| [persistent_union_find](submissions/persistent_union_find.cpp) | [Persistent Unionfind](https://judge.yosupo.jp/problem/persistent_unionfind) | unite(指定版)・same(指定版)・get_root・PersistentArray内部更新 |
| [primality](submissions/primality.cpp) | [Primality Test](https://judge.yosupo.jp/problem/primality_test) | is_prime、1〜10^18。 |
| [rollback_union_find](submissions/rollback_union_find.cpp) | [Persistent Unionfind](https://judge.yosupo.jp/problem/persistent_unionfind) | unite・same・undo |
| [rollback_weighted_union_find](submissions/rollback_weighted_union_find.cpp) | [Unionfind with Potential](https://judge.yosupo.jp/problem/unionfind_with_potential) | unite_checked・try_diff・mod998244353の加法群・矛盾・冗長制約 |
| [rolling_hash](submissions/rolling_hash.cpp) | [Z Algorithm](https://judge.yosupo.jp/problem/zalgorithm) | RollingHash構築・size・lcp（同じ文字列の接尾辞）・lcp内部の区間ハッシュ比較 |
| [union_find](submissions/union_find.cpp) | [Unionfind](https://judge.yosupo.jp/problem/unionfind) | unite・same |
| [weighted_union_find](submissions/weighted_union_find.cpp) | [Unionfind with Potential](https://judge.yosupo.jp/problem/unionfind_with_potential) | constrain・try_diff・mod998244353の加法群・矛盾・冗長制約 |
| [zeta_and_full](submissions/zeta_and_full.cpp) | [Bitwise And Convolution](https://judge.yosupo.jp/problem/bitwise_and_convolution) | AND畳み込み |
| [zeta_and_split](submissions/zeta_and_split.cpp) | [Bitwise And Convolution](https://judge.yosupo.jp/problem/bitwise_and_convolution) | AND畳み込み |
| [zeta_gcd_conv_full](submissions/zeta_gcd_conv_full.cpp) | [GCD Convolution](https://judge.yosupo.jp/problem/gcd_convolution) | GCD畳み込み |
| [zeta_gcd_conv_split](submissions/zeta_gcd_conv_split.cpp) | [GCD Convolution](https://judge.yosupo.jp/problem/gcd_convolution) | GCD畳み込み |
| [zeta_gcd_transform_full](submissions/zeta_gcd_transform_full.cpp) | [GCD Convolution](https://judge.yosupo.jp/problem/gcd_convolution) | multipleゼータ・メビウス変換とSieve |
| [zeta_gcd_transform_split](submissions/zeta_gcd_transform_split.cpp) | [GCD Convolution](https://judge.yosupo.jp/problem/gcd_convolution) | multipleゼータ・メビウス変換とSieve |
| [zeta_lcm_conv_full](submissions/zeta_lcm_conv_full.cpp) | [LCM Convolution](https://judge.yosupo.jp/problem/lcm_convolution) | LCM畳み込み |
| [zeta_lcm_conv_split](submissions/zeta_lcm_conv_split.cpp) | [LCM Convolution](https://judge.yosupo.jp/problem/lcm_convolution) | LCM畳み込み |
| [zeta_lcm_transform_full](submissions/zeta_lcm_transform_full.cpp) | [LCM Convolution](https://judge.yosupo.jp/problem/lcm_convolution) | divisorゼータ・メビウス変換とSieve |
| [zeta_lcm_transform_split](submissions/zeta_lcm_transform_split.cpp) | [LCM Convolution](https://judge.yosupo.jp/problem/lcm_convolution) | divisorゼータ・メビウス変換とSieve |
| [zeta_or_as_and_full](submissions/zeta_or_as_and_full.cpp) | [Bitwise And Convolution](https://judge.yosupo.jp/problem/bitwise_and_convolution) | 添字の補集合を用いたOR畳み込み |
| [zeta_or_as_and_split](submissions/zeta_or_as_and_split.cpp) | [Bitwise And Convolution](https://judge.yosupo.jp/problem/bitwise_and_convolution) | 添字の補集合を用いたOR畳み込み |
| [zeta_set_as_and_full](submissions/zeta_set_as_and_full.cpp) | [Bitwise And Convolution](https://judge.yosupo.jp/problem/bitwise_and_convolution) | 上位集合ゼータ・メビウス変換 |
| [zeta_set_as_and_split](submissions/zeta_set_as_and_split.cpp) | [Bitwise And Convolution](https://judge.yosupo.jp/problem/bitwise_and_convolution) | 上位集合ゼータ・メビウス変換 |
| [zeta_sparse_gcd_full](submissions/zeta_sparse_gcd_full.cpp) | [GCD Convolution](https://judge.yosupo.jp/problem/gcd_convolution) | 疎変換の公開APIによるGCD畳み込み（全キーを使用）。 |
| [zeta_sparse_gcd_split](submissions/zeta_sparse_gcd_split.cpp) | [GCD Convolution](https://judge.yosupo.jp/problem/gcd_convolution) | 疎変換の公開APIによるGCD畳み込み（全キーを使用）。 |
| [bellman_ford](submissions/bellman_ford.cpp) | [E - Coins Respawn](https://atcoder.jp/contests/abc137/tasks/abc137_e) | 負辺を含む単一始点最短路、終点へ伝播する-INFと終点に無関係な負閉路。 |
| [binomial_lucas](submissions/binomial_lucas.cpp) | [C - Tricolor Pyramid](https://atcoder.jp/contests/arc117/tasks/arc117_c) | n>=modのLucas。 |
| [binomial_lucas_split](submissions/binomial_lucas_split.cpp) | [C - Tricolor Pyramid](https://atcoder.jp/contests/arc117/tasks/arc117_c) | n>=modのLucas。 |
| [floyd_warshall](submissions/floyd_warshall.cpp) | [D - バスと避けられない運命](https://atcoder.jp/contests/abc012/tasks/abc012_4) | FloydWarshallによる全点間距離、グラフの中心。 |
| [geometry_circle_relation_full](submissions/geometry_circle_relation_full.cpp) | [035 - Two Circles](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ag) | intersection(Circle,Circle).kind: 非交差・接触・2交点・一致の分類。中心間距離から内包/外接を問題の5分類へ変換。 |
| [geometry_circle_relation_split](submissions/geometry_circle_relation_split.cpp) | [035 - Two Circles](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ag) | intersection(Circle,Circle).kind: 非交差・接触・2交点・一致の分類。中心間距離から内包/外接を問題の5分類へ変換。 |
| [geometry_contains_atcoder_full](submissions/geometry_contains_atcoder_full.cpp) | [098 - Polygon and Point](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_bu) | contains: 整数座標の一般単純多角形の内外（1照会、O(N)）。 |
| [geometry_contains_atcoder_split](submissions/geometry_contains_atcoder_split.cpp) | [098 - Polygon and Point](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_bu) | contains: 整数座標の一般単純多角形の内外（1照会、O(N)）。 |
| [geometry_orientation_atcoder_full](submissions/geometry_orientation_atcoder_full.cpp) | [C - Convex Quadrilateral](https://atcoder.jp/contests/abc266/tasks/abc266_c) | orientation: 非退化四角形の各連続3頂点の向き（整数座標）。 |
| [geometry_orientation_atcoder_split](submissions/geometry_orientation_atcoder_split.cpp) | [C - Convex Quadrilateral](https://atcoder.jp/contests/abc266/tasks/abc266_c) | orientation: 非退化四角形の各連続3頂点の向き（整数座標）。 |
| [geometry_point_segment_distance_full](submissions/geometry_point_segment_distance_full.cpp) | [033 - Distance](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ae) | distance(Point,Segment): 整数入力の点と閉線分の最短距離。 |
| [geometry_point_segment_distance_split](submissions/geometry_point_segment_distance_split.cpp) | [033 - Distance](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ae) | distance(Point,Segment): 整数入力の点と閉線分の最短距離。 |
| [geometry_segment_intersection_atcoder_full](submissions/geometry_segment_intersection_atcoder_full.cpp) | [037 - Intersection](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ah) | intersects: 整数閉線分の交差判定（接触・重なり含む）。 |
| [geometry_segment_intersection_atcoder_split](submissions/geometry_segment_intersection_atcoder_split.cpp) | [037 - Intersection](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ah) | intersects: 整数閉線分の交差判定（接触・重なり含む）。 |
| [geometry_triangle_area_full](submissions/geometry_triangle_area_full.cpp) | [C - 直訴](https://atcoder.jp/contests/abc002/tasks/abc002_3) | area: 整数座標の非退化三角形の面積。 |
| [geometry_triangle_area_split](submissions/geometry_triangle_area_split.cpp) | [C - 直訴](https://atcoder.jp/contests/abc002/tasks/abc002_3) | area: 整数座標の非退化三角形の面積。 |
| [heap_priority_queue](submissions/heap_priority_queue.cpp) | [D - Prefix K-th Max](https://atcoder.jp/contests/abc234/tasks/abc234_d) | push・pop_value・top・最大50万要素 |
| [matrix_rotate](submissions/matrix_rotate.cpp) | [B - Coloring Matrix](https://atcoder.jp/contests/abc298/tasks/abc298_b) | rotate_90r(vector<vector<int>>) |
| [next_combination](submissions/next_combination.cpp) | [E - Modulo MST](https://atcoder.jp/contests/abc328/tasks/abc328_e) | for_each_combination・first_combination内部・next_combination内部 |
| [project_selection](submissions/project_selection.cpp) | [040 - Get More Money（★7）](https://atcoder.jp/contests/typical90/tasks/typical90_an) | 単項費用・厳密な含意制約・solve |
| [radix_sort](submissions/radix_sort.cpp) | [C - Sum of Numbers Greater Than Me](https://atcoder.jp/contests/abc331/tasks/abc331_c) | RadixSortによる最大20万要素の非負整数整列、重複を含む数列。 |
| [spfa](submissions/spfa.cpp) | [E - Coins Respawn](https://atcoder.jp/contests/abc137/tasks/abc137_e) | 負辺を含む単一始点最短路、終点へ伝播する-INFと終点に無関係な負閉路。 |
| [topological](submissions/topological.cpp) | [G - Longest Path](https://atcoder.jp/contests/dp/tasks/dp_g) | TopologicalSort・得られた順序に沿うDAG最長路 |
| [topological_lexicographical](submissions/topological_lexicographical.cpp) | [D - Restricted Permutation](https://atcoder.jp/contests/abc223/tasks/abc223_d) | TopologicalSortLexicographical・閉路時の空結果 |
| [trie_dictionary](submissions/trie_dictionary.cpp) | [C - gacha](https://atcoder.jp/contests/abc164/tasks/abc164_c) | insert・contains・重複語と接頭辞を共有する語 |
| [zeta_sieve_factorize_full](submissions/zeta_sieve_factorize_full.cpp) | [D - Div Game](https://atcoder.jp/contests/abc169/tasks/abc169_d) | Sieve構築・1〜10^12の素因数分解と指数。 |
| [zeta_sieve_factorize_split](submissions/zeta_sieve_factorize_split.cpp) | [D - Div Game](https://atcoder.jp/contests/abc169/tasks/abc169_d) | Sieve構築・1〜10^12の素因数分解と指数。 |

## 独立参照による補足検査

| コード | 対象 |
| --- | --- |
| [dynamic_hash_local](submissions/dynamic_hash_local.cpp) | BIT検索・reset/clear/copy・64bit疎領域・2D BITのget/set/resetと10億寸法の疎操作・動的UFの負/INT端キー・size/groups/callback・HashMap複合キー/erase/copy/move、GenericHashの符号付き0と集合順不同・内部HashMap初期値/find/enumerate/clear・Li Chao区間の全点直接評価・query_line・読み取り非確保・2e18超の値・LineContainer整数と2D内積を全直線/点走査で照合・小数点・argmax・Heap置換/push_popをmultisetと照合・Treapの列操作をvectorと照合・順序集合・境界探索・非可換文字列 |
| [graph_local](submissions/graph_local.cpp) | 独立Floyd参照、負閉路の-INF伝播、経路復元、Dijkstra 256/257境界・radix経路 |
| [binary_trie_local](submissions/binary_trie_local.cpp) | multisetとの5000操作照合、XOR・count・順序統計・ID・キー上端・insert_uniqueの重複時ID不変・更新との混在 |
| [binomial_local](submissions/binomial_local.cpp) | Pascal三角形による法1〜160・Lucas・H・prepare・Mintラッパー照合 |
| [primality_sieve_local](submissions/primality_sieve_local.cpp) | 試し割り、ビット境界、uint64上端の既知例、指定底・コピー移動・列挙の照合 |
| [compressed_binary_trie_local](submissions/compressed_binary_trie_local.cpp) | multiset参照、XOR・count・順序統計・ID・キー上端、insert_unique・reserve |
| [zeta_set_reference_full](submissions/zeta_set_reference_full.cpp) | 集合4変換をO(n²)定義と照合。空列・長さ1を含む。 |
| [zeta_set_reference_split](submissions/zeta_set_reference_split.cpp) | 集合4変換をO(n²)定義と照合。空列・長さ1を含む。 |
| [zeta_sparse_reference_full](submissions/zeta_sparse_reference_full.cpp) | 除数閉包の疎入力で倍数和を二重ループと照合。疎・密の両経路、未整列・空列・逆変換。 |
| [zeta_sparse_reference_split](submissions/zeta_sparse_reference_split.cpp) | 除数閉包の疎入力で倍数和を二重ループと照合。疎・密の両経路、未整列・空列・逆変換。 |
| [zeta_sieve_reference_full](submissions/zeta_sieve_reference_full.cpp) | 最小素因数利用・素数判定・因数分解・約数・phi/mu・べき乗表・コピー/移動/交換。小範囲の独立定義に照合。 |
| [zeta_sieve_reference_split](submissions/zeta_sieve_reference_split.cpp) | 最小素因数利用・素数判定・因数分解・約数・phi/mu・べき乗表・コピー/移動/交換。小範囲の独立定義に照合。 |
| [geometry_reference_full](submissions/geometry_reference_full.cpp) | 収録部品ごとの独立既知値。重複・一直線・重なり・零長線分・円の非交差/接触/一致・重心など。 |
| [geometry_reference_point](submissions/geometry_reference_point.cpp) | 収録部品ごとの独立既知値。重複・一直線・重なり・零長線分・円の非交差/接触/一致・重心など。 |
| [geometry_reference_line](submissions/geometry_reference_line.cpp) | 収録部品ごとの独立既知値。重複・一直線・重なり・零長線分・円の非交差/接触/一致・重心など。 |
| [geometry_reference_polygon](submissions/geometry_reference_polygon.cpp) | 収録部品ごとの独立既知値。重複・一直線・重なり・零長線分・円の非交差/接触/一致・重心など。 |
| [geometry_reference_circle](submissions/geometry_reference_circle.cpp) | 収録部品ごとの独立既知値。重複・一直線・重なり・零長線分・円の非交差/接触/一致・重心など。 |
| [radix_sort_reference](submissions/radix_sort_reference.cpp) | 8整数型全域をstd::stable_sortと照合。小配列閾値の前後、昇降順、作業領域、安定添字順。 |
| [project_selection_reference](submissions/project_selection_reference.cpp) | 一般2変数劣モジュラ関数・集合利得・復元・再solve・追加・独立コピーを全割当参照で照合。 |
| [nearest_two_points_reference](submissions/nearest_two_points_reference.cpp) | O(n²)距離二乗・返却添字・全オーバーロード・空/1点・重複・uint64超過を照合。 |
| [angle_sort_reference](submissions/angle_sort_reference.cpp) | 小整数の独立atan2参照・厳密/非厳密比較・昇降順・添字列とint64境界の既知外積。 |
| [node_local](submissions/node_local.cpp) | make_node/make_chain・size/tail/nth・push/pop/reverse・cycle detection/break・共有状態・30万ノード反復破棄 |
| [trie_local](submissions/trie_local.cpp) | 空文字/全バイト・insert/count/contains・erase/erase_all・prefix/longest_prefix・kth/count_less/lower_bound/items・copy/move/clear/reserve・TrieNode共有 |
| [combination_masks_local](submissions/combination_masks_local.cpp) | 全n<=14・全k列挙・first/next/for_each・幅64・0・次なし |
| [rollback_weighted_local](submissions/rollback_weighted_local.cpp) | 冗長/矛盾追加の履歴・undo/snapshot/rollback・copy/clear/reset・差の相殺とINT64_MAX |
| [lca_local](submissions/lca_local.cpp) | 80個の森・BFS親列によるLCA/距離/根変更参照・祖先/パス上k番目・copy/move/clear |
| [topological_local](submissions/topological_local.cpp) | 0〜4頂点の全有向グラフ・全順列によるDAG/一意性/辞書順参照・Details位置・両ファイルの共有ガード |
| [rolling_hash_local](submissions/rolling_hash_local.cpp) | 二進加算の剰余積によるget/whole参照・全バイト/空/底の補正・concat/equal/lcp/compare・copy/move/swap |
| [matrix_transform_local](submissions/matrix_transform_local.cpp) | 全回転・転置・上下左右反転/inplace・vector<int>/vector<bool>/文字盤面・空/幅0/矩形/32境界・INT_MIN/INT_MAX回転数 |
| [lis_local](submissions/lis_local.cpp) | 400列・各接頭辞の全部分集合参照・狭義/広義・長さ/接頭辞長/添字復元/値列復元 |

## 旧入力例のローカル回帰

旧問題の入力例・出力例を手元で照合します。独立アルゴリズムによる照合や現在版のオンラインACとは区別します。幾何の未掲載APIは上の独立参照検査も併用します。

| コード | 対象 |
| --- | --- |
| [line_container_triangle](submissions/line_container_triangle.cpp) | 浮動小数点の直線追加・最小/最大照会・原点と2点の外積最大値 |
| [line_container_2d_triangle](submissions/line_container_2d_triangle.cpp) | 点の追加と全方向の内積最大/最小・原点と2点の外積最大値 |
| [geometry_orientation_full](submissions/geometry_orientation_full.cpp) | ローカル回帰: orientation、内積、距離二乗 |
| [geometry_orientation_split](submissions/geometry_orientation_split.cpp) | ローカル回帰: orientation、内積、距離二乗 |
| [geometry_projection_full](submissions/geometry_projection_full.cpp) | ローカル回帰: 直線への射影 |
| [geometry_projection_split](submissions/geometry_projection_split.cpp) | ローカル回帰: 直線への射影 |
| [geometry_reflection_full](submissions/geometry_reflection_full.cpp) | ローカル回帰: 直線に関する反射 |
| [geometry_reflection_split](submissions/geometry_reflection_split.cpp) | ローカル回帰: 直線に関する反射 |
| [geometry_segment_intersection_full](submissions/geometry_segment_intersection_full.cpp) | ローカル回帰: 閉線分交差判定 |
| [geometry_segment_intersection_split](submissions/geometry_segment_intersection_split.cpp) | ローカル回帰: 閉線分交差判定 |
| [geometry_segment_distance_full](submissions/geometry_segment_distance_full.cpp) | ローカル回帰: 閉線分間距離 |
| [geometry_segment_distance_split](submissions/geometry_segment_distance_split.cpp) | ローカル回帰: 閉線分間距離 |
| [geometry_area_full](submissions/geometry_area_full.cpp) | ローカル回帰: 多角形面積 |
| [geometry_area_split](submissions/geometry_area_split.cpp) | ローカル回帰: 多角形面積 |
| [geometry_contains_full](submissions/geometry_contains_full.cpp) | ローカル回帰: 多角形内外判定 |
| [geometry_contains_split](submissions/geometry_contains_split.cpp) | ローカル回帰: 多角形内外判定 |
| [geometry_convex_cut_full](submissions/geometry_convex_cut_full.cpp) | ローカル回帰: 凸多角形切断 |
| [geometry_convex_cut_split](submissions/geometry_convex_cut_split.cpp) | ローカル回帰: 凸多角形切断 |
| [geometry_circle_line_full](submissions/geometry_circle_line_full.cpp) | ローカル回帰: 円・直線交点（問題は交点あり） |
| [geometry_circle_line_split](submissions/geometry_circle_line_split.cpp) | ローカル回帰: 円・直線交点（問題は交点あり） |
| [geometry_circle_circle_full](submissions/geometry_circle_circle_full.cpp) | ローカル回帰: 円同士の交点（問題は交点あり） |
| [geometry_circle_circle_split](submissions/geometry_circle_circle_split.cpp) | ローカル回帰: 円同士の交点（問題は交点あり） |

## 動的2次元BIT

[提出コード](submissions/dynamic_fenwick_2d_rectangle.cpp)・[アルゴリズムの説明](../../Document/dynamic_fenwick_2d.md)を参照してください。
現在のソースとオンライン判定の対応は[manifest](manifest.json)で確認できます。
