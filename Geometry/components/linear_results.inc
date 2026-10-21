#ifndef SIRKIT_GEO_LINEAR_RESULTS_DEFINED
#define SIRKIT_GEO_LINEAR_RESULTS_DEFINED
namespace geo {
// 直線・線分の交差種別。交点は RealPoint、重なりは辞書順の両端点。
enum class LineIntersectionKind { None, Point, Coincident };
enum class SegmentIntersectionKind { None, Point, Overlap };
struct LineIntersection { LineIntersectionKind kind = LineIntersectionKind::None; std::optional<RealPoint> point; };
struct SegmentIntersection { SegmentIntersectionKind kind = SegmentIntersectionKind::None; std::vector<RealPoint> points; };
}
#endif
