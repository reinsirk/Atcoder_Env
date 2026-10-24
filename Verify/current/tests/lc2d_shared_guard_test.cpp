// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LC2D_SOURCE
#define SIRKIT_LC2D_SOURCE "../../../DataStructure/line-container-2d.cpp"
#endif
#ifndef SIRKIT_LC1D_SOURCE
#define SIRKIT_LC1D_SOURCE "../../../DataStructure/line-container.cpp"
#endif
#ifdef SIRKIT_LC2D_FIRST
#include SIRKIT_LC2D_SOURCE
#include SIRKIT_LC1D_SOURCE
#else
#include SIRKIT_LC1D_SOURCE
#include SIRKIT_LC2D_SOURCE
#endif
#include SIRKIT_LC2D_SOURCE
#include SIRKIT_LC1D_SOURCE
#include <cstdint>
#include <cstdio>
#include <cstdlib>
static std::size_t checks=0;
static void require(bool ok){++checks;if(!ok)std::abort();}
int main(){
 MinLineContainer<std::int64_t> mn;MaxLineContainer<std::int64_t> mx;
 for(auto[k,m]:{std::pair<std::int64_t,std::int64_t>{-3,7},{5,-11},{0,100},{5,-12}}){mn.add(k,m);mx.add(k,m);}
 LineContainer2D two;two.add(-3,7);two.add(5,-11);two.add(0,100);two.add(5,-12);
 for(std::int64_t x=-100;x<=100;++x){auto expected_min=std::min({-3*x+7,5*x-11,std::int64_t{100},5*x-12});auto expected_max=std::max({-3*x+7,5*x-11,std::int64_t{100},5*x-12});require(mn.query(x)==expected_min);require(mx.query(x)==expected_max);require(two.min_rounded_ll(x,1)==expected_min);require(two.max_rounded_ll(x,1)==expected_max);}
 bool overflow=false;MinLineContainer<std::int64_t> large;large.add(std::numeric_limits<std::int64_t>::max(),1);try{(void)large.query(2);}catch(const std::overflow_error&){overflow=true;}require(overflow);
 require(lc_div<std::int64_t>(-5,2)==-3);require(lc_div<std::int64_t>(5,-2)==-3);
 std::printf("PASS %zu shared LineContainer guard/order checks\n",checks);
}
