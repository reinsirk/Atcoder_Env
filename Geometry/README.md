# 幾何ライブラリの選び方

各 `.cpp` は標準ヘッダー以外の依存がなく、そのまま単独で貼り付けられます。
複数ファイルの併用・重複した貼り付けにも部品ごとのガードで対応します。
全機能が必要なら従来の `geometory.cpp` を使ってください。呼び出し名は従来どおり `geo::` です。

| ファイル | 主な用途・関数 |
| --- | --- |
| `Point.cpp` | `Point`、内積・外積・距離・回転・偏角順、`orientation`、`to_real` |
| `Line_Segment.cpp` | `Line/Segment`、`on_segment/intersects`、射影・反射・最近点、距離、交点 |
| `Polygon.cpp` | 面積・周長・内外判定・凸包・重心、`convex_cut`、整列済み点列用の`::convex_hull_sorted` |
| `Circle.cpp` | `Circle`、`circumcircle`、円盤の内外判定、円と直線・線分・円の交点 |

Pointは全ファイルに含まれます。Polygonは必要な線分判定とLine型、Circleは必要な線分判定を含みます。
Polygonだけで直線間の交点計算も使う場合はLine_Segmentを併用してください。
各型・関数群の先頭にAPI一覧と計算量を記載しています。全機能を使う例は `../Examples/geometry_example.cpp` にあります。

## 最小例

```cpp
#include "Polygon.cpp"
#include <cassert>
#include <vector>

int main() {
    std::vector<geo::PointI> points{{0, 0}, {4, 0}, {0, 3}, {1, 1}};
    auto hull = geo::convex_hull(points);
    assert(geo::area(hull) == 6);
    assert(geo::contains(hull, geo::PointI{1, 1}) == geo::Location::Inside);
}
```

## 型と前提

- `Point<T,C>` の座標型はT、内積・外積・差・積の計算型はCです。整数座標では途中計算もCに収まる必要があります。
- `PointI/PointD/PointLD` はそれぞれ `int64_t/double/long double` 座標、交点・重心は `RealPoint`（long double）です。
- 整数座標のCは、利用できればGCC／Clangの128ビット拡張、その他ではint64_tです。`SIRKIT_FORCE_STANDARD_ARITHMETIC` または既存の `GEO_DISABLE_INT128` で後者を選べます。
- 座標・通常の座標演算結果は有限、`Epsilon` のabs/relは非負有限が前提です。整数の向き・線分判定は厳密、実数の判定は指定の許容幅を使います。全座標を検査する走査は行いません。
- `Line` は異なる2点、`Segment` は同一点でも使えます。円の中心は有限、半径は非負有限です。円と直線・線分の交点APIには `Line<long double>/Segment<long double>` を渡します。
- `contains(poly,p)`・面積重心は単純多角形、`convex_cut(poly,line)` は順に並べた凸多角形が前提です。cutは有向直線の左側を境界ごと残します。
- `arg_cmp` は零ベクトルを先頭に偏角・距離順で並べます。浮動小数点では丸めた偏角が等しいものを距離で比較します。
- 円の交点は円周の交差です。円盤の重なり全体を返すものではありません。許容幅内の接触は近似した接点になり、浮動小数点計算の精度に依存します。

## 編集と生成

`components/` が共通部品の編集元です。配布用の5ファイルは直接編集せず、次で同期します。

```sh
python3 Scripts/generate_geometry.py
python3 Scripts/generate_geometry.py --check
```

生成器が部品の依存を展開するため、利用時にcomponentsや生成器は不要です。
現行実装・部品・生成器はCC0-1.0で提供します。旧版の出典・許諾に関する記録は監査資料に保持しています。

## 全機能版と追加ライブラリ

| ファイル | 用途 |
| --- | --- |
| [geometory.cpp](geometory.cpp) | 従来の全機能版 |
| [Nearest_Two_points.cpp](Nearest_Two_points.cpp) | 最近点対 |
| [偏角ソート.cpp](偏角ソート.cpp) | 偏角順の比較・ソート |

現在版の提出コード・検査一覧は[Verify](../Verify/current/README.md)を参照してください。

凸包の方式・整数演算・整列済み入力の扱いは[凸包の説明](../Document/convex_hull.md)を参照してください。

偏角ソートの提出コードは[angle_sort版](../Verify/current/submissions/angle_sort.cpp)と[比較関数を直接渡すstd::sort版](../Verify/current/submissions/angle_sort_std.cpp)があります。両方とも同じ入出力で比較できます。

整数の最近点対と整列済み入力の扱いは[説明](../Document/nearest_two_points.md)と[使用例](../Examples/nearest_two_points_example.cpp)を参照してください。

[円の交差](../Document/geometry_circle_intersection.md)・[内外判定](../Document/geometry_point_location.md)・[方向判定](../Document/geometry_orientation.md)・[点線分距離](../Document/geometry_point_segment_distance.md)・[線分交差](../Document/geometry_segment_intersection.md)・[面積](../Document/geometry_area.md)の前提と使用例は各説明を参照してください。
