#ifndef SIRKIT_GEO_CIRCLE_TYPES_DEFINED
#define SIRKIT_GEO_CIRCLE_TYPES_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [geo::円の型]
// CircleIntersectionKind: None/Tangent/TwoPoints/Coincident。CircleIntersection{kind,points}: 近似交点を所有。
// 一致する正半径円には交点を格納しない。半径0同士の同一点はTangentと1交点。
// Circle{center,radius}: 有限の中心と非負の有限半径。半径0は点。
// SIRKIT_API_END
enum class CircleIntersectionKind { None, Tangent, TwoPoints, Coincident };
struct CircleIntersection { CircleIntersectionKind kind = CircleIntersectionKind::None; std::vector<RealPoint> points; };
struct Circle { RealPoint center; long double radius = 0; };
}
#endif
