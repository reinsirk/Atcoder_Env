// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LC2D_SOURCE
#define SIRKIT_LC2D_SOURCE "../../../DataStructure/line-container-2d.cpp"
#endif
#include SIRKIT_LC2D_SOURCE
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <vector>
using ld=long double;
using ll=std::int64_t;
static std::uint64_t checks=0,values=0;
static const char* phase="start";
static void require(bool ok,ld input,ll got,ll expected){++checks;if(!ok){std::fprintf(stderr,"rounding failure phase=%s input=%La got=%" PRId64 " expected=%" PRId64 " check=%" PRIu64 "\n",phase,input,got,expected,checks);std::abort();}}
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
static ll reference(ld value){return std::llround(std::clamp(value,-LineContainer2D::INF,LineContainer2D::INF));}
static void check(ld value){
 if(!std::isfinite(value))std::abort();
 LineContainer2D t;t.add(value,0);++values;
 const ll positive=reference(value),negative=reference(-value);
 for(bool minimum:{false,true}){ll got=minimum?t.min_rounded_ll(1,0):t.max_rounded_ll(1,0);require(got==positive,value,got,positive);got=minimum?t.min_rounded_ll(-1,0):t.max_rounded_ll(-1,0);require(got==negative,-value,got,negative);}
}
static void neighbours(ld value){const ld inf=std::numeric_limits<ld>::infinity();if(std::isfinite(value))check(value);ld below=std::nextafter(value,-inf),above=std::nextafter(value,inf);if(std::isfinite(below))check(below);if(std::isfinite(above))check(above);}
int main(){
 phase="zeros and exponent extremes";
 for(ld v:{0.L,-0.L,std::numeric_limits<ld>::denorm_min(),-std::numeric_limits<ld>::denorm_min(),std::numeric_limits<ld>::min(),-std::numeric_limits<ld>::min(),std::numeric_limits<ld>::max(),-std::numeric_limits<ld>::max()})neighbours(v);
 phase="half integer neighbourhoods";
 for(int n=0;n<=4096;++n){neighbours(ld(n)+.5L);neighbours(-ld(n)-.5L);}
 phase="powers of two and nearby half integers";
 for(int exponent=-8;exponent<=65;++exponent){ld power=std::ldexp(1.L,exponent);for(ld offset:{-1.5L,-1.L,-.5L,0.L,.5L,1.L,1.5L}){neighbours(power+offset);neighbours(-power+offset);}}
 phase="clamp neighbourhoods";
 for(ld sign:{-1.L,1.L}){ld v=sign*LineContainer2D::INF;neighbours(v);ld lower=v,upper=v;for(int i=0;i<128;++i){lower=std::nextafter(lower,-std::numeric_limits<ld>::infinity());upper=std::nextafter(upper,std::numeric_limits<ld>::infinity());check(lower);check(upper);}for(int e=-8;e<20;++e){neighbours(v+std::ldexp(1.L,e));neighbours(v-std::ldexp(1.L,e));}}
 phase="random significand and rounding exponents";
 std::mt19937_64 rng(UINT64_C(0x5713e9acda28d5a1));
 for(int i=0;i<150000;++i){std::uint64_t significand=rng();int exponent=static_cast<int>(rng()%153)-80;ld v=std::ldexp(static_cast<ld>(significand),exponent-63);if(rng()&1)v=-v;check(v);if(i%8==0)neighbours(v);}
 phase="random significand and full finite exponents";
 for(int i=0;i<30000;++i){std::uint64_t significand=rng();const int range=2*std::numeric_limits<ld>::max_exponent;int exponent=static_cast<int>(rng()%static_cast<unsigned>(range))-std::numeric_limits<ld>::max_exponent;ld v=std::ldexp(static_cast<ld>(significand),exponent-63);if(rng()&1)v=-v;if(std::isfinite(v))check(v);}
 std::printf("PASS %" PRIu64 " rounding checks; %" PRIu64 " finite ld values; digits=%d; reference=llround(clamp)\n",checks,values,std::numeric_limits<ld>::digits);
}
