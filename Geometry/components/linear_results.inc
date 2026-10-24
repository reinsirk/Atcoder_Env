#ifndef SIRKIT_GEO_LINEAR_RESULTS_DEFINED
#define SIRKIT_GEO_LINEAR_RESULTS_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [geo::直線・線分の交差型]
// LineIntersectionKind: None/Point/Coincident。LineIntersection{kind,point}: 交点をoptional<RealPoint>で所有。
// SegmentIntersectionKind: None/Point/Overlap。SegmentIntersection{kind,points}: 近似交点、重なりは辞書順の両端点を所有。
// SIRKIT_API_END
enum class LineIntersectionKind { None, Point, Coincident };
enum class SegmentIntersectionKind { None, Point, Overlap };
struct LineIntersection { LineIntersectionKind kind = LineIntersectionKind::None; std::optional<RealPoint> point; };
struct SegmentIntersection { SegmentIntersectionKind kind = SegmentIntersectionKind::None; std::vector<RealPoint> points; };
}
#endif
