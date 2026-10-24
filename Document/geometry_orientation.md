# 三点の向き

`Geometry/Point.cpp`を貼り付ける。`geo::orientation(a,b,c,eps)`は左折1、一直線0、右折-1を返す。時間O(1)、動的確保なし。`a.orient(b,c)`は符号ではなく外積値、`a.ccw(b,c)`はその厳密な符号を返す。

整数座標の`orientation`は、差と2つの積が計算型`C`に収まることを前提に、積同士を比較する。外積の減算を行わない。`a.orient`は減算も`C`に収まる必要がある。GCC/Clangの128ビット整数拡張が利用できる環境では、既定の整数計算型として拡張型を使う。`SIRKIT_FORCE_STANDARD_ARITHMETIC`または従来の`GEO_DISABLE_INT128`を定義すると標準の`std::int64_t`へ切り替わる。代替経路では入力をその演算範囲へ制限する。

浮動小数点座標は2つの積の絶対値和を尺度とし、`eps.abs + eps.rel * scale`の許容幅で符号を判定する。有限入力と、計算型に収まる途中演算が必要。

```cpp
int turn = geo::orientation(geo::PointI{0,0}, geo::PointI{4,0}, geo::PointI{0,3}); // 1
```

Verifyは[C - Convex Quadrilateral](https://atcoder.jp/contests/abc266/tasks/abc266_c)。同問題だけで整数全域や許容幅による浮動述語を検証したことにはならない。

独立参照は`Verify/current/tests/geometry_orientation_test.cpp`。整数の差・積・符号を多倍長整数で照合し、二進有理数の座標と許容幅では整数だけで閾値を計算します。計算型の範囲外になる入力は、APIを呼ばず検証対象から除きます。
