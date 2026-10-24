# 円周同士の交差

`Geometry/Circle.cpp`を貼り付ける。全機能版は`Geometry/geometory.cpp`。既存の呼び出しは`geo::`内にある。

`geo::Circle{center,radius}`は有限の中心と非負の有限半径を持つ。半径0は点を表す。`geo::intersection(a,b,eps)`は非交差・接点・2交点・一致を分類し、必要な交点を`long double`座標で返す。正半径で一致する円には交点を格納しない。半径0の同一点同士は接点1個となる。

`circle_intersection_kind(a,b,eps)`は同じ判定を用い、種別だけを返す。交点用の動的確保を行わない。いずれも時間O(1)。既存の`intersection`は交点の所有権を戻り値の`vector`で持つ。

中心間距離と半径の和・差で交差を分類する。2交点の場合は中心を結ぶ方向への射影と垂直方向の高さから求める。通常の大きさの中心差には直接平方根を使い、二乗によるオーバーフローや距離のアンダーフローを避けるため、極端な大きさでは`hypot`を使う。境界付近の判定は`eps.abs + eps.rel * scale`による近似であり、厳密な実数述語ではない。ほぼ同心の円は、交点の高さが大きい場合に接点へまとめない。

入力検査は既定で有効。`SKIP_RUNTIME_CHECKS`は円の検査を除くが、公開関数`geo::validate(c)`を直接呼ぶと設定にかかわらず検査する。有限の入力であっても、中心差や交点計算の途中結果が`long double`で表せない場合の有限な答えは保証しない。

```cpp
geo::Circle a{{0, 0}, 5}, b{{6, 0}, 5};
auto kind = circle_intersection_kind(a, b); // TwoPoints、交点用の確保なし
auto hit = geo::intersection(a, b);        // (3,4), (3,-4)
```

Verifyは[035 - Two Circles](https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ag)。問題の出力は交点座標を含まないため、交点残差・零半径・極端な大きさ・ほぼ同心・許容幅の境界は独立ローカル検査で補う。オンライン提出の記録とは区別する。整数入力でも近い非接触を区別するため、このVerifyでは`geo::Epsilon{0,0}`を指定する。

`Verify/current/tests/geometry_circle_reference_test.cpp`では、多倍長の距離二乗と半径和・差の二乗で種別を参照し、交点残差・許容幅・極端スケール・入力検査を確認する。
