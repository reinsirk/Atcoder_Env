// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LC2D_REQUIRE_EXTRA
#define SIRKIT_LC2D_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_LC2D_SOURCE
#define SIRKIT_LC2D_SOURCE "../../../DataStructure/line-container-2d.cpp"
#endif
#include SIRKIT_LC2D_SOURCE
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

using ld = long double;
using ll = std::int64_t;
using Point = std::pair<ld,ld>;
using Big = boost::multiprecision::cpp_int;
using High = boost::multiprecision::number<boost::multiprecision::cpp_bin_float<256>>;
#ifdef SIRKIT_LC2D_REQUIRE_EXTRA
static_assert(requires(const LineContainer2D& t){t.max_with_point(0.L,0.L);t.min_with_point(0.L,0.L);});
static_assert(std::is_same_v<decltype(std::declval<const LineContainer2D&>().max_with_point(0.L,0.L)),LineContainer2D::Result>);
static_assert(std::is_same_v<decltype(std::declval<LineContainer2D::Result>().value),ld>);
static_assert(std::is_same_v<decltype(std::declval<LineContainer2D::Result>().x),ld>);
static_assert(std::is_same_v<decltype(std::declval<LineContainer2D::Result>().y),ld>);
#endif
static std::uint64_t checks=0, queries=0;
static const char* phase="start";
static void require(bool ok) { ++checks; if(!ok){std::fprintf(stderr,"failure phase=%s check=%" PRIu64 " query=%" PRIu64 "\n",phase,checks,queries);std::abort();} }
template<class E,class F> static void throws(F f) { bool caught=false;try { f(); } catch(const E&) { caught=true; }require(caught); }
static ld dot(Point p,ld a,ld b){return p.first*a+p.second*b;}
static bool member(Point p,const std::vector<Point>& points){return std::find(points.begin(),points.end(),p)!=points.end();}
static ll rounded(ld x){x=std::clamp(x,-LineContainer2D::INF,LineContainer2D::INF);return static_cast<ll>(std::round(x));}
static ll trunced(ld x){x=std::clamp(x,-LineContainer2D::INF,LineContainer2D::INF);return static_cast<ll>(x);}
template<class Table>static void extra_exact(const Table&t,const std::vector<Point>&points,ll a,ll b,ld hi,ld lo){
 if constexpr(requires{t.max_with_point(ld{},ld{});t.min_with_point(ld{},ld{});}){
  auto mx=t.max_with_point(ld(a),ld(b)),mn=t.min_with_point(ld(a),ld(b));
  require(mx.value==hi&&mn.value==lo);require(member({mx.x,mx.y},points)&&member({mn.x,mn.y},points));
  require(dot({mx.x,mx.y},ld(a),ld(b))==mx.value);require(dot({mn.x,mn.y},ld(a),ld(b))==mn.value);
 }
}
template<class Table>static void extra_high(const Table&t,const std::vector<Point>&points,ld a,ld b,const High&hi,const High&lo,const High&tol){
 if constexpr(requires{t.max_with_point(ld{},ld{});t.min_with_point(ld{},ld{});}){
  for(bool minimum:{false,true}){auto got=minimum?t.min_with_point(a,b):t.max_with_point(a,b);High target=minimum?lo:hi;require(member({got.x,got.y},points));High selected=High(a)*High(got.x)+High(b)*High(got.y);require(abs(selected-target)<=tol);
   if(abs(target)>High(std::numeric_limits<ld>::max()))require(std::isinf(got.value)&&std::signbit(got.value)==(target<0));
   else require(std::isfinite(got.value)&&abs(High(got.value)-target)<=tol);
   if(std::isfinite(a*got.x)&&std::isfinite(b*got.y))require(got.value==a*got.x+b*got.y);
  }
 }
}
template<class Table>static void extra_empty(const Table&t){
#ifndef SKIP_RUNTIME_CHECKS
 if constexpr(requires{t.max_with_point(ld{},ld{});t.min_with_point(ld{},ld{});}){throws<std::out_of_range>([&]{(void)t.max_with_point(0.L,0.L);});throws<std::out_of_range>([&]{(void)t.min_with_point(0.L,0.L);});}
#else
 (void)t;
#endif
}
template<class Table>static void extra_invalid(const Table&t,ld a,ld b){
#ifndef SKIP_RUNTIME_CHECKS
 if constexpr(requires{t.max_with_point(ld{},ld{});t.min_with_point(ld{},ld{});}){throws<std::invalid_argument>([&]{(void)t.max_with_point(a,b);});throws<std::invalid_argument>([&]{(void)t.min_with_point(a,b);});}
#else
 (void)t;(void)a;(void)b;
#endif
}
template<class Table>static void extra_ratio(const Table&t,ld a,ld b){if constexpr(requires{t.max_with_point(ld{},ld{});t.min_with_point(ld{},ld{});}){throws<std::overflow_error>([&]{(void)t.max_with_point(a,b);});throws<std::overflow_error>([&]{(void)t.min_with_point(a,b);});}}
static void inspect_exact(const LineContainer2D& t,const std::vector<Point>& points,ll a,ll b){
 ++queries;require(t.size()==points.size());require(t.empty()==points.empty());
 if(points.empty()) {require(!t.max_optional(ld(a),ld(b)));require(!t.min_optional(ld(a),ld(b)));require(!t.argmax_optional(ld(a),ld(b)));require(!t.argmin_optional(ld(a),ld(b)));return;}
 ld hi=-std::numeric_limits<ld>::infinity(),lo=-hi;
 for(auto p:points){ld v=dot(p,ld(a),ld(b));hi=std::max(hi,v);lo=std::min(lo,v);}
 extra_exact(t,points,a,b,hi,lo);
 require(t.max_ld(ld(a),ld(b))==hi);require(t.min_ld(ld(a),ld(b))==lo);
 require(t.max_ld(a,b)==hi);require(t.min_ld(a,b)==lo);
 require(t.max_ll(a,b)==trunced(hi));require(t.min_ll(a,b)==trunced(lo));
 require(t.max_rounded_ll(a,b)==rounded(hi));require(t.min_rounded_ll(a,b)==rounded(lo));
 require(t.max_optional(ld(a),ld(b))==hi);require(t.min_optional(ld(a),ld(b))==lo);
 Point maxp=t.argmax(ld(a),ld(b)),minp=t.argmin(ld(a),ld(b));
 require(member(maxp,points)&&dot(maxp,ld(a),ld(b))==hi);require(member(minp,points)&&dot(minp,ld(a),ld(b))==lo);
 auto maxopt=t.argmax_optional(ld(a),ld(b)),minopt=t.argmin_optional(ld(a),ld(b));
 require(maxopt&&member(*maxopt,points)&&dot(*maxopt,ld(a),ld(b))==hi);
 require(minopt&&member(*minopt,points)&&dot(*minopt,ld(a),ld(b))==lo);
}
static void random_exact(){
 phase="random binary-fraction hulls";std::mt19937_64 rng(81273951);
 for(int trial=0;trial<100;++trial){LineContainer2D t;std::vector<Point> points;
  for(int step=0;step<120;++step){
   ld x=ld(static_cast<int>(rng()%2000001)-1000000)/4,y=ld(static_cast<int>(rng()%2000001)-1000000)/4;
   if(step%9==0){x=0;y=0;}if(step%13==0&&!points.empty()){x=points.back().first;y=points.back().second;}
   t.add(x,y);points.emplace_back(x,y);
   for(int q=0;q<3;++q){ll a=static_cast<ll>(rng()%2000001)-1000000,b=static_cast<ll>(rng()%2000001)-1000000;if(q==0)a=0;if(q==1)b=0;inspect_exact(t,points,a,b);}
   if(step%31==0){auto copy=t;auto original=points;t.clear();require(t.empty());inspect_exact(copy,original,-37,59);t=copy;copy.clear();inspect_exact(t,points,93,-19);auto moved=std::move(t);require(t.empty());t=std::move(moved);require(moved.empty());inspect_exact(t,points,-123,811);}
  }
  auto before=t.max_ld(17,29);auto* self=&t;t=*self;require(t.max_ld(17,29)==before);t=std::move(*self);require(t.max_ld(17,29)==before);
  LineContainer2D other;other.add(7,-13);std::vector<Point> pother{{7,-13}};t.swap(other);inspect_exact(t,pother,31,-17);inspect_exact(other,points,31,-17);t.swap(t);inspect_exact(t,pother,31,-17);
  t.clear();require(t.empty()&&t.size()==0);t.add(-1.5L,.5L);inspect_exact(t,{{-1.5L,.5L}},1,0);
 }
}
static void boundaries(){
 phase="hull boundaries";
 for(int form=0;form<6;++form){LineContainer2D t;std::vector<Point> points;
  for(int i=-80;i<=80;++i){ld x=i,y=form==0?-i*i:(form==1?i*i:form==2?0:form==3?ld(i):form==4?ld(i%3):ld(i*i*i)/4);if(form==2)x=0;t.add(x,y);points.emplace_back(x,y);}
  for(ll a=-200;a<=200;++a)for(ll b:{ll{-2},ll{-1},ll{0},ll{1},ll{2}})inspect_exact(t,points,a,b);
  // 交点の直前・直後は高精度参照と絶対誤差で比較する。
  for(ld x:{-1.L,-.5L,0.L,.5L,1.L})for(ld c:{std::nextafter(x,-std::numeric_limits<ld>::infinity()),x,std::nextafter(x,std::numeric_limits<ld>::infinity())}){
   ld hi=-std::numeric_limits<ld>::infinity(),lo=-hi;for(auto p:points){hi=std::max(hi,dot(p,1,c));lo=std::min(lo,dot(p,1,c));}
   ld maxv=t.max_ld(1.L,c),minv=t.min_ld(1.L,c);ld tol=256*std::numeric_limits<ld>::epsilon()*(1+std::max(std::abs(hi),std::abs(lo)));
   require(std::abs(maxv-hi)<=tol);require(std::abs(minv-lo)<=tol);
  }
 }
 phase="half integer rounding and clamping";
 for(ld x:{-4.1e18L,-4.1e18L-8,-3.5L,-2.5L,-1.5L,-.5L,0.L,.5L,1.5L,2.5L,3.5L,4.1e18L,4.1e18L+8}){LineContainer2D t;t.add(x,-x);inspect_exact(t,{{x,-x}},1,0);inspect_exact(t,{{x,-x}},0,1);inspect_exact(t,{{x,-x}},-1,0);}
}
static void exact_integer_reference(){
 phase="cpp_int exact bounded integer";std::mt19937_64 rng(137);
 for(int trial=0;trial<30;++trial){LineContainer2D t;std::vector<std::pair<ll,ll>> points;
  for(int i=0;i<200;++i){ll x=static_cast<ll>(rng()%2000000001)-1000000000,y=static_cast<ll>(rng()%2000000001)-1000000000;t.add(x,y);points.emplace_back(x,y);}
  for(int q=0;q<200;++q){ll a=static_cast<ll>(rng()%2000000001)-1000000000,b=static_cast<ll>(rng()%2000000001)-1000000000;Big hi,lo;bool first=true;for(auto[x,y]:points){Big v=Big(x)*a+Big(y)*b;if(first){hi=lo=v;first=false;}else{hi=std::max(hi,v);lo=std::min(lo,v);}}
   require(t.max_rounded_ll(a,b)==hi.convert_to<ll>());require(t.min_rounded_ll(a,b)==lo.convert_to<ll>());
  }
 }
}
static void check_high(const LineContainer2D& t,const std::vector<Point>& points,ld a,ld b){
 High hi,lo,scale=0;bool first=true;for(auto[x,y]:points){High p=High(a)*High(x),q=High(b)*High(y),v=p+q;if(first){hi=lo=v;first=false;}else{if(v>hi)hi=v;if(v<lo)lo=v;}High s=abs(p)+abs(q);if(s>scale)scale=s;}
 High tol=(scale+1)*High(std::numeric_limits<ld>::epsilon())*256;
 extra_high(t,points,a,b,hi,lo,tol);
 for(bool minimum:{false,true}){ld value=minimum?t.min_ld(a,b):t.max_ld(a,b);High target=minimum?lo:hi;
  if(abs(target)>High(std::numeric_limits<ld>::max())){require(std::isinf(value)&&std::signbit(value)==(target<0));}
  else {require(std::isfinite(value)&&abs(High(value)-target)<=tol);if(target==0 && scale>High(std::numeric_limits<ld>::max()))require(value==0);}
  auto selected=minimum?t.argmin(a,b):t.argmax(a,b);require(member(selected,points));High selected_value=High(a)*High(selected.first)+High(b)*High(selected.second);require(abs(selected_value-target)<=tol);
 }
}
static void unit_boundary_cases(){
 phase="two-line unit boundary paths";
 for(ld sign:{-1.L,1.L}){LineContainer2D t;std::vector<Point> ps{{sign,0},{0,1}};for(auto[x,y]:ps)t.add(x,y);
  for(ld a:{-3.L,-1.L,-.5L,.5L,1.L,3.L,std::numeric_limits<ld>::min()})
   for(ld c:{std::nextafter(sign,-std::numeric_limits<ld>::infinity()),sign,std::nextafter(sign,std::numeric_limits<ld>::infinity()),0.L}){
    ld b=a*c;check_high(t,ps,a,b);auto mx=t.max_optional(a,b),mn=t.min_optional(a,b);require(mx&&mn);const ld tolerance=256*std::numeric_limits<ld>::epsilon()*(1+std::abs(a)+std::abs(b));require(std::abs(*mx-t.max_ld(a,b))<=tolerance);require(std::abs(*mn-t.min_ld(a,b))<=tolerance);
   }
  const ld tiny=std::numeric_limits<ld>::denorm_min(),large=std::numeric_limits<ld>::max();
  throws<std::overflow_error>([&]{(void)t.min_ld(tiny,large);});throws<std::overflow_error>([&]{(void)t.max_ld(-tiny,large);});
 }
}
static void high_precision_boundaries(){
 phase="high precision int64 endpoints";const ll low=std::numeric_limits<ll>::min(),high=std::numeric_limits<ll>::max();LineContainer2D t;std::vector<Point> ps;
 for(auto[x,y]:std::vector<std::pair<ll,ll>>{{low,low},{low,high},{high,low},{high,high},{high,high-1},{high-1,high},{0,0}}){t.add(x,y);ps.emplace_back(ld(x),ld(y));}
 for(auto[a,b]:std::vector<std::pair<ll,ll>>{{1,0},{0,1},{1,1},{1,-1},{high,high},{low,low},{high,low},{low,high}})check_high(t,ps,ld(a),ld(b));
 phase="finite huge cancellation";const ld huge=std::ldexp(1.L,std::numeric_limits<ld>::max_exponent/2+4);
 for(Point p:std::vector<Point>{{huge,huge},{huge,-huge},{-huge,huge},{-huge,-huge}}){LineContainer2D one;one.add(p.first,p.second);for(auto[a,b]:std::vector<Point>{{huge,huge},{huge,-huge},{-huge,huge},{-huge,-huge}})check_high(one,{p},a,b);}
 phase="integer central-hull overflow cancellation";
 const ld central_scale=std::numeric_limits<ld>::max()/16;const ll ia=std::numeric_limits<ll>::max()-1,ib=ia/2;
 for(int direction:{-1,1}){LineContainer2D ct;std::vector<Point> cp{{direction*central_scale,-direction*2*central_scale},{-direction*3*central_scale,4*central_scale},{-direction*3*central_scale,-4*central_scale}};for(auto[x,y]:cp)ct.add(x,y);check_high(ct,cp,ld(ia),ld(ib));
  if(direction==1){require(ct.max_ld(ia,ib)==0);require(ct.max_ll(ia,ib)==0);require(ct.max_rounded_ll(ia,ib)==0);require(ct.min_ll(ia,ib)==-LineContainer2D::INTEGER_LIMIT);}
  else{require(ct.min_ld(ia,ib)==0);require(ct.min_ll(ia,ib)==0);require(ct.min_rounded_ll(ia,ib)==0);require(ct.max_ll(ia,ib)==LineContainer2D::INTEGER_LIMIT);}
 }
 phase="huge two-point intersections";
 const ld largest=std::numeric_limits<ld>::max()/4;
 for(const auto& ps2:std::vector<std::vector<Point>>{{{largest,largest},{-largest,-largest}},{{largest,-largest},{-largest,largest}},{{largest,0},{-largest,0},{0,largest},{0,-largest}}}){LineContainer2D ht;for(auto[x,y]:ps2)ht.add(x,y);for(auto[a,b]:std::vector<Point>{{1,1},{1,-1},{-1,1},{-1,-1},{0,1},{1,0}})check_high(ht,ps2,a,b);}
 phase="ratio overflow";LineContainer2D one;one.add(1,2);const ld tiny=std::numeric_limits<ld>::denorm_min(),large=std::numeric_limits<ld>::max();
 throws<std::overflow_error>([&]{(void)one.max_ld(tiny,large);});throws<std::overflow_error>([&]{(void)one.min_ld(tiny,large);});throws<std::overflow_error>([&]{(void)one.argmax(tiny,large);});throws<std::overflow_error>([&]{(void)one.argmin_optional(tiny,large);});extra_ratio(one,tiny,large);
}
static void validation(){
 phase="empty, nonfinite, finite coefficients";LineContainer2D t;inspect_exact(t,{},0,0);extra_empty(t);const ld inf=std::numeric_limits<ld>::infinity(),nan=std::numeric_limits<ld>::quiet_NaN();
#ifndef SKIP_RUNTIME_CHECKS
 throws<std::out_of_range>([&]{(void)t.max_ld(0,0);});throws<std::out_of_range>([&]{(void)t.min_ld(0.L,0.L);});throws<std::out_of_range>([&]{(void)t.argmax(0,0);});throws<std::out_of_range>([&]{(void)t.max_ll(0,0);});
 throws<std::invalid_argument>([&]{t.add(inf,0);});throws<std::invalid_argument>([&]{t.add(0,nan);});require(t.empty());throws<std::invalid_argument>([&]{(void)t.max_optional(inf,0);});
 t.add(1,2);for(auto[a,b]:std::vector<Point>{{inf,1},{1,inf},{nan,1},{1,nan},{0,inf},{inf,0}}){throws<std::invalid_argument>([&]{(void)t.max_ld(a,b);});throws<std::invalid_argument>([&]{(void)t.min_ld(a,b);});throws<std::invalid_argument>([&]{(void)t.argmax(a,b);});extra_invalid(t,a,b);}
#else
 (void)inf;(void)nan;t.add(1,2);
#endif
 require(t.max_ld(std::uint32_t{2},std::uint32_t{3})==8);require(t.min_ld(std::int8_t{-2},std::int16_t{-3})==-8);require(t.max_ld(true,false)==1);
}
int main(){validation();random_exact();boundaries();exact_integer_reference();unit_boundary_cases();high_precision_boundaries();std::cout<<"PASS "<<checks<<" independent LineContainer2D checks; "<<queries<<" exact-direction queries; ld_digits="<<std::numeric_limits<ld>::digits<<'\n';}
