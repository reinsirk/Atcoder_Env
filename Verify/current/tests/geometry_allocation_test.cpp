// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_GEOMETRY_TEST_HEADER
#define SIRKIT_GEOMETRY_TEST_HEADER "../../../Geometry/geometory.cpp"
#endif
#include SIRKIT_GEOMETRY_TEST_HEADER
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
std::size_t allocations=0;
void* operator new(std::size_t bytes) { ++allocations; if(void* p=std::malloc(bytes==0?1:bytes)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
int main(){const geo::Circle a{{0,0},5},b{{6,0},5};const auto before=allocations;assert(circle_intersection_kind(a,b)==geo::CircleIntersectionKind::TwoPoints);assert(triangle_area(geo::PointI{0,0},geo::PointI{4,0},geo::PointI{0,3})==6);assert(allocations==before);{const auto hit=geo::intersection(a,b);assert(hit.points.size()==2);assert(allocations>before);}std::cout<<"allocation-free convenience APIs passed\n";}
