# 2次元の内積最適化

`LineContainer2D`は点を追加し、方向`(a,b)`に対する`a*x+b*y`の最大値・最小値と、その点を照会します。削除には対応しません。[使用例](../Examples/line_container_2d_example.cpp)を参照してください。

点`(x,y)`を直線`y*t+x`として、最小・最大の2つの包絡に保存します。`a`が非零なら`t=b/a`から選択点を求め、元の係数で内積を評価します。軸方向は座標の極値を使います。包絡の選択点を固定容量でキャッシュし、範囲外では木の検索に戻ります。照会でキャッシュは変更しません。

以下、hは両包絡に保持する点数の合計です。保持領域はO(h)、追加は償却O(log h)、照会はO(log h)です。多数の点を包絡から除く1回の追加にはO(h)かかる場合があります。

## API

| API | 動作 |
| --- | --- |
| `add(x,y)` | 有限の点を追加する。重複も登録件数に含める。 |
| `max_ld(a,b)` / `min_ld(a,b)` | 最大・最小の内積値を`long double`で返す。非空が前提。 |
| `max_ld(整数,整数)` / `min_ld(整数,整数)` | 型全体がint64_t範囲内の整数型では、型から保証できる有限性・比の検査を省く。計算は浮動小数点。 |
| `max_ll(a,b)` / `min_ll(a,b)` | int64_t係数の照会後、値を±4.1e18に制限し、0方向へ切り捨ててint64_tで返す。 |
| `max_rounded_ll(a,b)` / `min_rounded_ll(a,b)` | 同じ範囲制限後に最も近い整数へ丸める。半整数は0から遠い側。 |
| `argmax(a,b)` / `argmin(a,b)` | 選択した元の点を`pair<long double,long double>`で返す。非空が前提。 |
| `max_with_point(a,b)` / `min_with_point(a,b)` | 点を1回選択し、`Result{value,x,y}`を返す。係数はlong double、非空が前提。 |
| `max_optional(a,b)` / `min_optional(a,b)` | 空ならnullopt、それ以外は内積値を返す。 |
| `argmax_optional(a,b)` / `argmin_optional(a,b)` | 空ならnullopt、それ以外は選択点を返す。 |
| `size()` / `empty()` | 重複込みの追加点数・空判定。O(1)。有効な包絡点数とは異なる。 |
| `clear()` / `swap(other)` | 全削除して空にする / 全状態を交換する。O(h) / O(1)。 |

同点で選ばれる点は規定しません。`Result`は3つの値を所有し、表の要素への参照を持ちません。内積値と選択点が両方必要な場合に使います。
`value`は返した点の内積です。浮動計算の順序により、別に呼んだ`max_ld`・`min_ld`とビット単位で一致する保証はありません。

```cpp
LineContainer2D points;
points.add(1, 2);
points.add(3, -1);
auto result = points.max_with_point(2, 1);
// result.value == 5, result.x == 3, result.y == -1
```

## 数値と入力の前提

座標と浮動係数は有限値が前提です。型本来の`long double`精度と既定の丸めモードで使います。内積・交点には丸め誤差があり、整数引数やll版でも厳密な整数内積を保証しません。丸め前に精度を失った値は、rounded版でも復元できません。ゼロの符号は規定しません。

非空の照会で、有限な非零`a`に対して`b/a`が表現範囲を超える場合は、`SKIP_RUNTIME_CHECKS`の有無によらず`overflow_error`を投げます。内積が浮動小数点範囲を超える場合は無限大になり得ます。巨大な有限積同士の相殺でNaNになった場合は、指数を調整して再評価します。ll版は内積を±4.1e18へ制限するため、範囲外の数学的な値をそのまま返すAPIではありません。

検査有効時は非有限の入力を拒否し、非optional照会の空表に`out_of_range`を投げます。`SKIP_RUNTIME_CHECKS`を定義すると、これらの入力検査を除外します。追加点数の上限、比の表現範囲、相殺の回復は維持します。`SIRKIT_LINE_CONTAINER_FORCE_PORTABLE`は同梱1D部品の整数演算の標準代替経路を指定するもので、2D演算を厳密整数へ切り替える設定ではありません。

## 所有権と公開メンバー

コピーは独立した包絡を持ち、内部の選択点参照も組み直します。ムーブ後の元は空で再利用できます。自己代入は内容を保持します。追加途中の確保失敗では、両包絡と登録件数を空に戻して例外を伝えます。コピー代入は一時コピーが完成してから交換します。

公開の`minlc`・`maxlc`・`Xmin`・`Xmax`・`Ymin`・`Ymax`は読み取り用です。包絡の直線は元点の`k=y,m=x`を保持します。外から書き換えたり、包絡だけに追加・削除したりすると、件数・座標範囲・内部参照の不変条件が壊れます。

提出先は[Library Checker Line Add Get Min](https://judge.yosupo.jp/problem/line_add_get_min)です。直線`a*x+b`を点`(a,b)`、照会を方向`(x,1)`へ変換します。全方向・浮動係数・追加API・所有権はローカル検査で補います。出典とライセンスの調査根拠は[監査記録](../Audit/provenance-line-container-2d-min-2026-10-22.json)に置きます。
