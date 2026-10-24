// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_GEOMETRY_TEST_HEADER
#define SIRKIT_GEOMETRY_TEST_HEADER "../../../Geometry/geometory.cpp"
#endif
#include SIRKIT_GEOMETRY_TEST_HEADER
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>
struct Item {geo::PointI a,b,c,d;};
int main(int argc,char**argv){
 const std::string mode=argc>1?argv[1]:"contains";std::mt19937_64 gen(777); const int n=mode=="circle"?262144:524288;
 std::vector<Item> items(n); for(auto& v:items){auto point=[&](){return geo::PointI{std::int64_t(gen()%2000001)-1000000,std::int64_t(gen()%2000001)-1000000};};v={point(),point(),point(),point()};}
 const std::string shape=argc>2?argv[2]:"random";
 if(mode=="segments")for(auto&v:items){
  if(shape=="crossing"){v.a={-1000000,-1000000};v.b={1000000,1000000};v.c={-1000000,1000000};v.d={1000000,-1000000};}
  if(shape=="separated"){v.c.x+=10000000;v.d.x+=10000000;}
  if(shape=="collinear")v.a.y=v.b.y=v.c.y=v.d.y=0;
  if(shape=="touching")v.c=v.b;
  if(shape=="degenerate"){v.b=v.a;v.d=v.c;}
 }
 if(mode=="distance")for(auto&v:items){
  if(shape=="interior"){v.a={0,0};v.b={2000000,0};v.c.x=1000000;}
  if(shape=="endpoint"){v.a={0,0};v.b={2000000,0};v.c.x=-2000000;}
 }
 std::vector<geo::Circle> circles; if(mode=="circle")for(int i=0;i<n;++i)circles.push_back({geo::to_real(items[i].a),static_cast<long double>(1+gen()%1000000)});
 if(mode=="circle")for(std::size_t i=0;i<circles.size();++i){
  if(shape=="coincident")circles[i]={{0,0},5};
  if(shape=="tangent")circles[i]={{static_cast<long double>(i%2)*10,0},5};
  if(shape=="tiny")circles[i]={{circles[i].center.x*1e-200L,circles[i].center.y*1e-200L},circles[i].radius*1e-200L};
  if(shape=="huge")circles[i]={{circles[i].center.x*1e200L,circles[i].center.y*1e200L},circles[i].radius*1e200L};
 }
 std::vector<geo::PointI> poly; const bool zig=argc>2&&std::string(argv[2])=="zig";
 if(mode=="contains"||mode=="area") {for(int i=0;i<100000;++i)poly.push_back({i,(zig?(i%2)*200000:200000)});for(int i=99999;i>=0;--i)poly.push_back({i,-100000});}
 std::int64_t checksum=0;long double real=0;
 const auto start=std::chrono::steady_clock::now();
 if(mode=="contains")for(int k=0;k<100;++k)checksum+=int(geo::contains(poly,geo::PointI{std::int64_t((k*977)%120000)-10000,std::int64_t((k*15427)%500000)-200000}));
 else if(mode=="orientation")for(const auto&v:items)checksum+=geo::orientation(v.a,v.b,v.c);
 else if(mode=="segments")for(const auto&v:items)checksum+=geo::intersects(geo::Segment<std::int64_t>{v.a,v.b},geo::Segment<std::int64_t>{v.c,v.d});
 else if(mode=="distance")for(const auto&v:items)real+=geo::distance(v.c,geo::Segment<std::int64_t>{v.a,v.b});
 else if(mode=="circle")for(int i=0;i<n;++i){auto hit=geo::intersection(circles[i],circles[(i+17)%n]);checksum+=int(hit.kind)+std::int64_t(hit.points.size());if(!hit.points.empty())real+=hit.points.front().x;}
 else if(mode=="area")for(int k=0;k<100;++k){poly[0].x=k;real+=geo::area(poly);}
 const auto end=std::chrono::steady_clock::now();
 std::cout<<std::chrono::duration<double,std::milli>(end-start).count()<<' '<<checksum<<' '<<std::setprecision(20)<<real<<'\n';
}
