#ifndef SIRKIT_GEO_LINEAR_TYPES_DEFINED
#define SIRKIT_GEO_LINEAR_TYPES_DEFINED
namespace geo {
// Segment{a,b} は閉線分（零長可）、Line{a,b} は相異なる2点で表す直線。
template<class T, class C = DefaultCalc<T>> struct Segment { Point<T, C> a, b; };
template<class T, class C = DefaultCalc<T>> struct Line { Point<T, C> a, b; };
}
#endif
