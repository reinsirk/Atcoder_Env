# 内部部品

各ライブラリは必要な標準ヘッダーを含み、単独で貼り付けられます。

| ファイル | 対応する検証 |
| --- | --- |
| [Nyaan_hash_map_min.cpp](Nyaan_hash_map_min.cpp) | [Associative Array](../Verify/current/submissions/internal_hash_map.cpp) |

内部用`reinsirk::detail::InternalHashMap`です。利用者向けの汎用表は[HashMap](../DataStructure/Hash_map.cpp)を使ってください。

問題URL、追加APIの補足検査、現在版の判定状況は[Verify一覧](../Verify/current/README.md)を参照してください。

方式とAPIの前提は[内部整数ハッシュ表](../Document/internal_hash_map.md)、使用方法は[使用例](../Examples/internal_hash_map_example.cpp)を参照してください。
