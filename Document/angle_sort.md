# 整数ベクトルの偏角ソート

`Geometry/偏角ソート.cpp`を貼り付けて使います。点は`std::pair<std::int64_t,std::int64_t>`の`{x,y}`で表し、各座標の全域に対応します。関数と比較器はグローバルに定義されます。

偏角は+x方向から反時計回りの`[0,2π)`です。昇順では零ベクトルを先頭、降順では末尾に置きます。同じ方向の長さによる順序は定めません。零ベクトルは零ベクトルとのみ同方向です。

| API | 動作 |
| --- | --- |
| `angle_cross_sign(a,b)` | 外積の符号を-1/0/1で返す。 |
| `same_angle(a,b)` | 同じ方向なら`true`を返す。 |
| `arccmp_less(a,b)`、`arccmp_g(a,b)` | 偏角の厳密な`a<b`、`a>b`を返す。 |
| `arccmp_less_eq(a,b)`、`arccmp_g_eq(a,b)` | 非厳密な`a<=b`、`a>=b`を返す。 |
| `AngleLess`、`AngleGreater` | 厳密な昇順・降順の比較器。`std::sort`にも渡せる。 |
| `angle_sort(p,descending=false)` | `p`をその場で昇順に並べ替える。`true`なら降順。 |
| `angle_sort_indices(p,descending=false)` | `p`を変えず、偏角順の0始まり添字列を返す。`true`なら降順。 |

比較と方向判定はO(1)、ソートはO(N log N)です。添字列の作成はO(N)の追加領域を使い、同じ偏角の点同士の順序は保証しません。`arccmp_less_eq`と`arccmp_g_eq`は厳密な比較ではないため、`std::sort`の比較器には使えません。

整数の積を安全な範囲で比較し、必要な場合だけ広い演算を使います。GCC/Clangの128ビット整数拡張が利用できる環境では、その拡張を使います。標準C++の型ではありません。`SIRKIT_FORCE_STANDARD_ARITHMETIC`を定義するか拡張が利用できない場合は、符号と符号なし整数の分割積による標準C++の経路へ切り替わります。

有効な座標型の全域を扱うため入力検査は不要で、`SKIP_RUNTIME_CHECKS`によって動作は変わりません。

```cpp
std::vector<std::pair<std::int64_t, std::int64_t>> points{
    {0, -1}, {1, 0}, {0, 0}, {0, 1}
};
angle_sort(points); // {0,0}, {1,0}, {0,1}, {0,-1}
auto order = angle_sort_indices(points, true); // 入力を変えず降順の添字を得る
```
