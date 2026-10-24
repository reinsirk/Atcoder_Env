// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_GEOMETRY_TEST_HEADER
#define SIRKIT_GEOMETRY_TEST_HEADER "../../../Geometry/geometory.cpp"
#endif
#include SIRKIT_GEOMETRY_TEST_HEADER
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>
using Big = boost::multiprecision::cpp_int;
using Decimal = boost::multiprecision::cpp_dec_float_100;
using P = geo::PointI;
using S = geo::Segment<std::int64_t>;
std::uint64_t checks = 0;
void require(bool value) { ++checks; if (!value) { std::cerr << "failed check " << checks << '\n'; std::abort(); } }
int sign(const Big& value) { return (value > 0) - (value < 0); }
Big turn(P a,P b,P c) { return (Big(b.x)-a.x)*(Big(c.y)-a.y)-(Big(b.y)-a.y)*(Big(c.x)-a.x); }
bool between(P p,S s) { return turn(s.a,s.b,p)==0 && std::min(s.a.x,s.b.x)<=p.x && p.x<=std::max(s.a.x,s.b.x) && std::min(s.a.y,s.b.y)<=p.y && p.y<=std::max(s.a.y,s.b.y); }
bool crossing(S s,S t) { const int a=sign(turn(s.a,s.b,t.a)),b=sign(turn(s.a,s.b,t.b)),c=sign(turn(t.a,t.b,s.a)),d=sign(turn(t.a,t.b,s.b)); return (a*b<0&&c*d<0)||between(s.a,t)||between(s.b,t)||between(t.a,s)||between(t.b,s); }
geo::Location inside(const std::vector<P>& polygon,P p) {
 if(polygon.empty())return geo::Location::Outside;
 if(polygon.size()==1)return p==polygon[0]?geo::Location::Boundary:geo::Location::Outside;
 bool parity=false;
 for(std::size_t i=0;i<polygon.size();++i){const P a=polygon[i],b=polygon[(i+1)%polygon.size()]; if(between(p,{a,b}))return geo::Location::Boundary;
  if((a.y>p.y)!=(b.y>p.y)){const Big left=(Big(p.x)-a.x)*(Big(b.y)-a.y),right=(Big(b.x)-a.x)*(Big(p.y)-a.y);if((b.y>a.y&&left<right)||(b.y<a.y&&left>right))parity=!parity;}}
 return parity?geo::Location::Inside:geo::Location::Outside;
}
Decimal decimal(long double x){std::ostringstream stream;stream.precision(std::numeric_limits<long double>::max_digits10);stream<<x;return Decimal(stream.str());}
long double distance_reference(geo::RealPoint p,geo::RealPoint a,geo::RealPoint b) {
 const Decimal dx=decimal(b.x)-decimal(a.x),dy=decimal(b.y)-decimal(a.y),ux=decimal(p.x)-decimal(a.x),uy=decimal(p.y)-decimal(a.y); const Decimal den=dx*dx+dy*dy;
 if(den==0)return sqrt(ux*ux+uy*uy).convert_to<long double>();
 Decimal t=(ux*dx+uy*dy)/den;t=std::max(Decimal(0),std::min(Decimal(1),t));const Decimal x=ux-t*dx,y=uy-t*dy;return sqrt(x*x+y*y).convert_to<long double>();
}
void distances(std::mt19937_64& random) {
 for(int i=0;i<5000;++i){auto point=[&](){return geo::RealPoint{static_cast<long double>(std::int64_t(random()%2000000001)-1000000000),static_cast<long double>(std::int64_t(random()%2000000001)-1000000000)};};const auto p=point(),a=point(),b=i%19==0?a:point();const long double expected=distance_reference(p,a,b),actual=geo::distance(p,geo::Segment<long double>{a,b});require(std::isfinite(actual)&&std::abs(actual-expected)<=1e-8L+1e-14L*expected);}
 for(long double scale: {1e-200L,1e-150L,1.0L,1e150L,1e200L}) {const geo::RealPoint a{0,0},b{3*scale,4*scale},p{-4*scale,3*scale};const auto actual=geo::distance(p,geo::Segment<long double>{a,b});require(std::abs(actual/scale-5)<1e-15L);}
 const long double large=std::numeric_limits<long double>::max()/4;
 require(std::abs(geo::distance(geo::RealPoint{0,1},geo::Segment<long double>{{-large,0},{large,0}})-1)<1e-15L);
 require(geo::distance(geo::RealPoint{1,0},geo::Segment<long double>{{0,0},{2,0}})==0);
 require(std::abs(geo::distance(P{4,7},S{{1,3},{1,3}})-5)<1e-15L);
}
void circles(std::mt19937_64& random) {
 for(int i=0;i<7000;++i){auto circle=[&](){return geo::Circle{{static_cast<long double>(std::int64_t(random()%20001)-10000),static_cast<long double>(std::int64_t(random()%20001)-10000)},static_cast<long double>(random()%10001)};};const auto a=circle(),b=i%23==0?a:circle();const Decimal dx=decimal(a.center.x)-decimal(b.center.x),dy=decimal(a.center.y)-decimal(b.center.y),r=decimal(a.radius),s=decimal(b.radius),d2=dx*dx+dy*dy;geo::CircleIntersectionKind expected;
  if(d2==0&&r==s)expected=r==0?geo::CircleIntersectionKind::Tangent:geo::CircleIntersectionKind::Coincident;
  else if(d2>(r+s)*(r+s)||d2<(r-s)*(r-s))expected=geo::CircleIntersectionKind::None;
  else if(d2==(r+s)*(r+s)||d2==(r-s)*(r-s))expected=geo::CircleIntersectionKind::Tangent;
  else expected=geo::CircleIntersectionKind::TwoPoints;
  const auto hit=geo::intersection(a,b);require(hit.kind==expected);
#ifdef SIRKIT_CIRCLE_INTERSECTION_KIND_DEFINED
  require(circle_intersection_kind(a,b)==hit.kind);
#endif
  require(hit.points.size()==(hit.kind==geo::CircleIntersectionKind::Tangent?1:hit.kind==geo::CircleIntersectionKind::TwoPoints?2:0));
  for(const auto& p:hit.points){require(std::abs(p.dist(a.center)-a.radius)<=1e-7L);require(std::abs(p.dist(b.center)-b.radius)<=1e-7L);}}
 for(long double scale:{1e-200L,1e-150L,1.0L,1e150L,1e200L}){const geo::Circle a{{0,0},5*scale},b{{6*scale,0},5*scale};const auto hit=geo::intersection(a,b,geo::Epsilon{0,0});require(hit.kind==geo::CircleIntersectionKind::TwoPoints);require(hit.points.size()==2);for(const auto&p:hit.points){require(std::abs(p.x/scale-3)<1e-15L);require(std::abs(std::abs(p.y/scale)-4)<1e-15L);}}
 const geo::Circle a{{0,0},1},point{{0,0},0};require(geo::intersection(point,point).kind==geo::CircleIntersectionKind::Tangent);require(geo::intersection(a,geo::Circle{{1e-15L,0},1}).kind==geo::CircleIntersectionKind::TwoPoints);
 for(long double offset:{-2e-11L,-1e-12L,0.0L,1e-12L,2e-11L}){auto hit=geo::intersection(a,geo::Circle{{2+offset,0},1});require(hit.kind==(offset>3e-12L?geo::CircleIntersectionKind::None:offset>=-3e-12L?geo::CircleIntersectionKind::Tangent:geo::CircleIntersectionKind::TwoPoints));}
#ifndef SKIP_RUNTIME_CHECKS
 bool threw=false;try{(void)geo::intersection(a,geo::Circle{{0,0},-1});}catch(const std::domain_error&){threw=true;}require(threw);
 threw=false;try{(void)geo::intersection(a,geo::Circle{{std::numeric_limits<long double>::infinity(),0},1});}catch(const std::domain_error&){threw=true;}require(threw);
#endif
 bool validate_threw=false;try{geo::validate(geo::Circle{{0,0},-1});}catch(const std::domain_error&){validate_threw=true;}require(validate_threw);
}
int main(){std::mt19937_64 random(881151);
 std::vector<P> grid;for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)grid.push_back({x,y});
 for(P a:grid)for(P b:grid)for(P c:grid)for(P d:grid){const S first{a,b},second{c,d};const bool expected=crossing(first,second);require(geo::intersects(first,second)==expected);require(geo::intersects(geo::Segment<long double>{geo::to_real(a),geo::to_real(b)},geo::Segment<long double>{geo::to_real(c),geo::to_real(d)},geo::Epsilon{0,0})==expected);const auto hit=geo::intersection(first,second);require((hit.kind!=geo::SegmentIntersectionKind::None)==expected);for(const auto& q:hit.points){require(geo::on_segment(q,geo::Segment<long double>{geo::to_real(a),geo::to_real(b)},geo::Epsilon{1e-15L,1e-15L}));require(geo::on_segment(q,geo::Segment<long double>{geo::to_real(c),geo::to_real(d)},geo::Epsilon{1e-15L,1e-15L}));}}
#if defined(__SIZEOF_INT128__) && !defined(GEO_DISABLE_INT128) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
 for(int k=0;k<10000;++k){auto point=[&](){return P{std::int64_t(random()%6000000000000000001ULL)-std::int64_t{3000000000000000000},std::int64_t(random()%6000000000000000001ULL)-std::int64_t{3000000000000000000}};};const P a=point(),b=point(),c=point(),d=point();require(geo::orientation(a,b,c)==sign(turn(a,b,c)));require(geo::intersects(S{a,b},S{c,d})==crossing({a,b},{c,d}));}
#endif
for(int i=0;i<70000;++i){auto point=[&](){return P{std::int64_t(random()%2000000001)-1000000000,std::int64_t(random()%2000000001)-1000000000};};const P a=point(),b=point(),c=point(),d=i%17==0?a:point();require(geo::orientation(a,b,c)==sign(turn(a,b,c)));
#ifdef SIRKIT_TRIANGLE_AREA_DEFINED
require(decimal(triangle_area(a,b,c))==Decimal(abs(turn(a,b,c)).convert_to<std::string>())/2);
require(Big(signed_triangle_area2(a,b,c))==turn(a,b,c));
#endif
require(geo::intersects(S{a,b},S{c,d})==crossing({a,b},{c,d}));require(geo::on_segment(c,S{a,b})==between(c,{a,b}));}
 std::vector<std::vector<P>> polygons{{},{{1,2}},{{0,0},{3,3}},{{0,0},{0,0}},{{0,0},{6,0},{6,6},{4,6},{4,2},{2,2},{2,6},{0,6}}};
 for(int k=0;k<200;++k){std::vector<P> polygon;const int n=2+int(random()%30);for(int x=0;x<n;++x)polygon.push_back({x*3,10+std::int64_t(random()%20)});for(int x=n-1;x>=0;--x)polygon.push_back({x*3,-10-std::int64_t(random()%20)});polygons.push_back(std::move(polygon));}
 for(auto polygon:polygons){for(int rev=0;rev<2;++rev){Big sum=0;for(std::size_t j=0;j<polygon.size();++j){const auto a=polygon[j],b=polygon[(j+1)%polygon.size()];sum+=Big(a.x)*b.y-Big(a.y)*b.x;}require(decimal(geo::area(polygon))==Decimal(abs(sum).convert_to<std::string>())/2);for(int k=0;k<100;++k){P p{std::int64_t(random()%101)-20,std::int64_t(random()%81)-40};require(geo::contains(polygon,p)==inside(polygon,p));}std::reverse(polygon.begin(),polygon.end());}}
#if defined(__SIZEOF_INT128__) && !defined(GEO_DISABLE_INT128) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
 const P a{std::numeric_limits<std::int64_t>::min(),0},b{std::numeric_limits<std::int64_t>::max(),0};require(geo::on_segment(P{0,0},S{a,b}));require(geo::intersects(S{a,b},S{{0,0},{0,0}}));
#endif
 distances(random);circles(random);std::cout<<checks<<" independent geometry checks passed\n";}
