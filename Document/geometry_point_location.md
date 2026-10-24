# 多角形と点の内外判定

`Geometry/Polygon.cpp`を貼り付ける。`geo::contains(polygon,p,eps)`は単純多角形に対する`Outside`・`Boundary`・`Inside`を返す。閉じるために先頭頂点を末尾へ重複させる必要はない。空は外、1・2頂点では境界か外となる。時間O(N)、追加領域O(1)。

辺が照会点の高さをまたぐ向きと、辺に対する点の向きから巻き数を数える。座標型と計算型`C`がともに整数なら、必要な差・積が`C`に収まる範囲で厳密に判定する。整数座標でも浮動計算型を指定した場合は、型変換の丸めを含む。整数の場合は照会点の高さより上下に完全に外れる辺を先に除く。一直線上の辺は座標の範囲比較で境界を判定する。

浮動小数点座標は`eps`を使う従来の判定を用いる。高さの比較そのものは座標値の比較であり、すべての比較へ許容幅を適用するわけではない。単純性を検査するための全辺対照合は行わない。

```cpp
std::vector<geo::PointI> polygon{{0,0}, {4,0}, {4,3}, {0,3}};
auto inside = geo::contains(polygon, geo::PointI{2,1}); // Inside
auto edge = geo::contains(polygon, geo::PointI{4,1});   // Boundary
```

Verifyは[098 - Polygon and Point](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_bu)。辺上・退化・浮動入力は問題の対象外であり、独立ローカル検査で補う。

`Verify/current/tests/geometry_contains_reference_test.cpp`では、多倍長整数の有理数ray交差を独立参照とし、境界・退化・整数端・二進有理数・許容幅を確認する。
