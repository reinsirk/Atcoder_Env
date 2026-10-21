#ifndef SIRKIT_GEO_CIRCLE_TYPES_DEFINED
#define SIRKIT_GEO_CIRCLE_TYPES_DEFINED
namespace geo {
// 円の交差種別と近似交点。一致する正半径円には交点を格納しない。
enum class CircleIntersectionKind { None, Tangent, TwoPoints, Coincident };
struct CircleIntersection { CircleIntersectionKind kind = CircleIntersectionKind::None; std::vector<RealPoint> points; };
// Circle{center,radius}: 有限の中心と非負の有限半径。半径0は点。
struct Circle { RealPoint center; long double radius = 0; };
}
#endif
