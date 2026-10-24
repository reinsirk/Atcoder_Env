# 点と閉線分の距離

`Geometry/Line_Segment.cpp`を貼り付ける。`geo::distance(p,segment)`は閉線分上の最近点までの距離を`long double`で返す。零長線分も扱う。`geo::closest_point(p,segment)`は最近点そのものを返す。ともに時間O(1)、動的確保なし。

座標を`long double`へ変換して方向を単位化し、内積により最近点が端点か内部かを判定する。内部の場合、距離は点と方向ベクトルの外積の絶対値から直接求める。最近点を一度生成してから再び距離を取る必要がない。64ビット以下の整数座標では、方向の二乗和が`long double`の指数範囲に収まることを型から確認し、直接平方根で方向長を求める。それ以外は成分を最大絶対値でスケーリングし、方向の二乗和がそのままでは表せない場合にも対応する。

入力座標は有限で、変換後の差や内積・外積等の途中結果が表せることを前提とする。丸め誤差があるため、ほぼ一直線上の点や巨大な座標の小さな距離について絶対精度を保証するものではない。直線を受ける距離・射影は、変換後も非零長の直線を要求する。

```cpp
geo::Segment<std::int64_t> segment{{0,0}, {4,0}};
auto d = geo::distance(geo::PointI{2,3}, segment); // 3
```

Verifyは[033 - Distance](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ae)。問題の3点は相異なる。零長、極端な大きさ、端点・内部の分岐は多倍長による独立ローカル検査で補う。

`Verify/current/tests/geometry_distance_reference_test.cpp`では、多倍長の投影判定・内外積による距離と比較し、整数端・反転・平行移動・浮動座標・極端スケールを確認する。
