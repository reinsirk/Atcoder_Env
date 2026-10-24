// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_SEGMENT_TEST_HEADER
#define SIRKIT_SEGMENT_TEST_HEADER "../../../Geometry/Line_Segment.cpp"
#endif
#include SIRKIT_SEGMENT_TEST_HEADER
#include <cstdint>
#include <cstdlib>
#include <iostream>

template<class C> void check(std::int64_t base) {
    using S=geo::Segment<std::int64_t,C>;
    const S s{{base,base},{base+1,base+64}};
    const S outside{{base,base+1000},{base,base+1000}};
    const S endpoint{s.b,s.b};
    // Cでx座標が丸められても、非整数計算型では両軸の範囲を維持する。
    if(geo::intersects(s,outside)||geo::intersects(outside,s))std::abort();
    if(!geo::intersects(s,endpoint)||!geo::intersects(endpoint,s))std::abort();
}
int main(){check<float>(1000000000);check<double>(std::int64_t{1}<<53);
    std::cout<<"PASS mixed calculation types\n";}
