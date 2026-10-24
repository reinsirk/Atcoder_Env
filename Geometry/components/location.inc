#ifndef SIRKIT_GEO_LOCATION_DEFINED
#define SIRKIT_GEO_LOCATION_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [geo::Location]
// Location: containsの戻り値。Outside/Boundary/Insideは外部/境界/内部。
// SIRKIT_API_END
enum class Location { Outside, Boundary, Inside };
}
#endif
