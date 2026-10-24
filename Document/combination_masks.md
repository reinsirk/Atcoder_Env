# 同じ要素数の部分集合マスク

`Algorithm/next_combination.cpp` は、`bool` を除く標準の符号なし整数をマスクとして扱う。ビットが1の位置を選んだ要素とみなし、同じ個数を選ぶ組合せをマスクの数値昇順に列挙する。

| 操作 | 結果 |
|---|---|
| `next_combination(mask)` | 同じ個数の1を持つ次のマスク。0や型内に次がない場合は0 |
| `next_combination(mask, n)` | 下位nビット内の次に更新。次がなければfalseを返し、maskは不変 |
| `first_combination<UInt>(k)` | 下位kビットが1の最小マスク |
| `last_combination<UInt>(n, k)` | nビット内で1がk個の最大マスク |
| `for_each_combination<UInt>(n, k, visit)` | 全マスクを `visit(mask)` へ渡す。k=0も1回呼ぶ |

`UInt` の省略時は `std::uint64_t`。一引数版には既存の `int` オーバーロードもあり、非負を前提とし、次が正の `int` に収まらなければ0を返す。それ以外は `0 <= k <= n <= 型幅` が前提。全幅を使う場合も全幅シフトを起こさない。入力検査は既定で有効で、`SKIP_RUNTIME_CHECKS` により除外できる。

次のマスクは、最下位の1を上へ繰り上げ、その下の1を末尾へ詰めて求める。最低ビットまでの距離は `std::countr_zero` で求め、除算を使わない。一回の操作はO(1)時間・追加O(1)領域。

列挙は最終マスクを開始時に計算し、各回の幅の検査を終端の比較へ置き換える。終端以外では次が型内に存在するため、ゼロや桁あふれの判定を繰り返さない。1個だけ選ぶ場合は左シフトで更新し、2個以上では右シフトを1回にまとめる。この一段化は単独の`next_combination`には使わない。1個だけ選ぶマスクでは、シフト量が型幅と等しくなる場合があるためである。

訪問処理を除いてO(C(n,k))時間・追加O(1)領域である。コールバックは昇順に各マスクを訪問し、訪問後に次を計算する。例外は呼出元へ伝わる。

列挙の数が大きくなり得る点に注意する。ライブラリはコールバックの回数制限を設けない。

[使用例](../Examples/next_combination_example.cpp)・[小幅の全列挙と境界テスト](../Verify/current/tests/combination_reference_test.cpp)・[32／64ビットの位置配列による独立照合](../Verify/current/tests/combination_wide_reference_test.cpp)・[constexprの検査](../Verify/current/tests/combination_constexpr_test.cpp)を参照。
