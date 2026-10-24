// SPDX-License-Identifier: CC0-1.0
// 大量の包絡削除・再構築と所有操作を、全登録点の独立参照で照合する。
#ifndef SIRKIT_LC2D_SOURCE
#define SIRKIT_LC2D_SOURCE "../../../DataStructure/line-container-2d.cpp"
#endif
#include SIRKIT_LC2D_SOURCE
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <random>
#include <utility>
#include <vector>
using ld=long double;
using Point=std::pair<ld,ld>;
static std::uint64_t checks=0;
static void inspect(const LineContainer2D& h,const std::vector<Point>& points){
 assert(h.size()==points.size());
 for(std::int64_t a:{-8192,-4096,-1,0,1,4096,8192})
  for(std::int64_t b:{-8193,-4097,-4096,-4095,-2049,-2048,-2047,-2,-1,0,1,2,2047,2048,2049,4095,4096,4097,8193}){
   ld lo=std::numeric_limits<ld>::infinity(),hi=-lo;
   for(auto [x,y]:points){ld v=ld(a)*x+ld(b)*y;lo=std::min(lo,v);hi=std::max(hi,v);}
   assert(h.max_ld(a,b)==hi&&h.min_ld(a,b)==lo);
   auto mx=h.max_with_point(ld(a),ld(b)),mn=h.min_with_point(ld(a),ld(b));
   assert(mx.value==hi&&mn.value==lo);
   assert(std::find(points.begin(),points.end(),Point{mx.x,mx.y})!=points.end());
   assert(std::find(points.begin(),points.end(),Point{mn.x,mn.y})!=points.end());
   assert(ld(a)*mx.x+ld(b)*mx.y==hi&&ld(a)*mn.x+ld(b)*mn.y==lo);
   checks+=7;
  }
}
static void ownership(LineContainer2D& h,std::vector<Point>& points){
 const auto& readonly=h;inspect(readonly,points);
 LineContainer2D c=h;inspect(c,points);auto* self=&c;c=*self;inspect(c,points);
 LineContainer2D m=std::move(c);assert(c.empty());inspect(m,points);
 LineContainer2D other;other.add(3,5);std::vector<Point> op{{3,5}};
 m.swap(other);inspect(m,op);inspect(other,points);
 LineContainer2D assigned;assigned.add(-5,7);assigned=other;inspect(assigned,points);
 LineContainer2D moved;moved=std::move(assigned);assert(assigned.empty());inspect(moved,points);
 m.swap(other);h=std::move(m);assert(m.empty());inspect(h,points);
}
int main(){
 std::mt19937_64 rng(0x16fc4f1c49ccb6aULL);
 for(int sign:{-1,1})for(int order=0;order<3;++order){
  std::vector<int> ts;for(int t=-1024;t<=1024;++t)ts.push_back(t);
  if(order==1)std::reverse(ts.begin(),ts.end());
  if(order==2)std::shuffle(ts.begin(),ts.end(),rng);
  LineContainer2D h;std::vector<Point> points;
  for(int t:ts){Point p{ld(sign)*t*t/4096,t};h.add(p.first,p.second);points.push_back(p);if(points.size()%257==0)inspect(h,points);}
  ownership(h,points);
  for(Point p:std::vector<Point>{{ld(-sign)*4096,0},{ld(-sign)*16384,8192},{ld(-sign)*16384,-8192},{ld(-sign)*1048576,0}}){
   h.add(p.first,p.second);points.push_back(p);ownership(h,points);
  }
  for(int t=-1024;t<=1024;++t){Point p{ld(-sign)*1048576+ld(-sign)*t*t/4096,t};h.add(p.first,p.second);points.push_back(p);if((t+1024)%257==0)inspect(h,points);}
  ownership(h,points);h.clear();assert(h.empty());h.add(7,-11);points={{7,-11}};ownership(h,points);
 }
 std::printf("lc2d prune/reference checks: %" PRIu64 "\n",checks);
}
