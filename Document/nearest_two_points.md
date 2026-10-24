# 最近点対

整数座標の点列から、距離が最も短い2点と距離の二乗を返します。入力の順序は変更しません。座標は各値が`int32_t`に収まる整数です。[使用例](../Examples/nearest_two_points_example.cpp)を参照してください。

点をx座標順に分け、左右を再帰的に解きます。小区間は全対を比較し、大きな区間は2本の配列を交互に使ってy座標順にマージします。分割線の近くにある点だけを比較します。未整列版の初回整列は、小入力では比較sort、大入力では32bitのx座標の基数整列を使います。距離0の対が見つかれば、それ以上の距離探索は不要です。N点にO(N log N)時間、O(N)追加領域を使います。

2048点以上の未整列入力で先頭2点が同じなら、入力コピーを終えてから距離0の対を返し、整列を省きます。この場合はO(N)時間です。通常の入力検査と単一パスiteratorの読み取りは、同じ位置で全入力に対して行います。

## API

| API | 動作 |
| --- | --- |
| `nearest_two_points(first,last)` | `[first,last)`の整数pair-like列から`optional<NearestTwoPointsResult>`を返す。未整列でよい。 |
| `nearest_two_points(first,count)` | `first`から`std::ptrdiff_t`型の`count`個を処理する。同じ戻り値。 |
| `nearest_two_points(points)` | `vector`全体を処理する。 |
| `nearest_two_points_sorted(first,last)` | `(x,y)`の辞書順で非減少の範囲を処理し、最初のsortを省く。 |
| `nearest_two_points_sorted(first,count)` | 同じ順序の列を指定個数だけ処理する。 |
| `nearest_two_points_sorted(points)` | 同じ順序の`vector`全体を処理する。 |
| `closest_pair(first,n)` | 互換呼出。`int`個数の点から最短距離の二乗を`uint64_t`で返す。 |

`P`は`pair<int32_t,int32_t>`の別名です。ほかの点型も、`.first`と`.second`が整数なら範囲内の値を渡せます。範囲版は`std::input_iterator`に対応し、move-only iteratorも移動して渡せます。個数版では負でない個数を渡し、その個数の要素を参照できることが前提です。

`NearestTwoPointsResult`は`distance_squared`、`first_index`、`second_index`を持ちます。添字は渡した列の0始まりで、`first_index < second_index`です。同じ最短距離を与える対が複数ある場合、選択は規定しません。sorted版も添字は渡した整列済み列の位置で、整列前の別の列の添字を復元するAPIではありません。

前提を満たす点列が2点未満ならoptional版はnullopt、互換の`closest_pair`は`INT32_MAX`を`uint64_t`に変換して返します。空かどうかを明示したい場合はoptional版を使います。

```cpp
const std::vector<P> points{{4, 5}, {5, 5}, {9, 7}};
auto answer = nearest_two_points_sorted(points);
// answer->distance_squared == 1
// answer->first_index == 0, answer->second_index == 1
```

## 距離と入力検査

距離計算は整数で行います。int32座標同士の差は最大`2^32-1`で、二乗和は65bit必要になる場合があります。途中の遠い対が`uint64_t`を超えても探索を続け、最小距離の二乗自体が`uint64_t`を超える場合だけ`overflow_error`を投げます。この判定は`SKIP_RUNTIME_CHECKS`でも維持します。

GCC/Clangの128bit整数拡張が利用可能なら内部計算に使います。これは標準C++の型ではありません。`SIRKIT_FORCE_STANDARD_ARITHMETIC`を定義すると標準C++の代替経路へ切り替わります。

既定では通常の入力コピー中に座標範囲を確認し、範囲外を`out_of_range`で拒否します。sorted版は同じ処理で辞書順の非減少を確認し、違反なら`invalid_argument`を投げます。sized sentinelの逆順と負個数も`invalid_argument`で拒否します。検査だけの追加走査は行いません。例外で処理が止まった場合、残りの入力まで検査したとは限りません。

`SKIP_RUNTIME_CHECKS`を定義するとこれらの入力検査と検査専用状態を除外します。座標範囲、sorted版の整列条件、iterator範囲、個数は利用側の前提になります。内部の容量対策、距離の桁上がり対策、距離0の判定は維持します。

## 所有権

点の座標と添字を作業領域へコピーし、2本の`std::vector`で所有します。戻り値は距離と添字を所有し、入力への参照やiteratorを保持しません。確保やiterator操作の例外は局所領域を破棄して伝えます。単一パスiteratorの読み取り位置を巻き戻す保証はありません。

来歴とライセンスの根拠は[監査記録](../Audit/provenance-nearest-two-points-2026-10-22.json)を参照してください。

## Verify

提出先は[Closest Pair of Points](https://judge.yosupo.jp/problem/closest_pair)。元入力の点番号を返す未整列版を使用する。sorted版・単一パス/値返しiterator・全int32座標域・65bit二乗和・不正入力と確保/読取失敗は、`Verify/current/tests/nearest_*_test.cpp`の独立ローカル検査で補う。
