// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LC2D_SOURCE
#define SIRKIT_LC2D_SOURCE "../../../DataStructure/line-container-2d.cpp"
#endif
#include SIRKIT_LC2D_SOURCE
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>
static std::ptrdiff_t countdown=-1;
static std::size_t live=0,checks=0,failures=0;
[[gnu::noinline]] void* operator new(std::size_t n){if(countdown==0)throw std::bad_alloc();if(countdown>0)--countdown;if(void*p=std::malloc(n?n:1)){++live;return p;}throw std::bad_alloc();}
[[gnu::noinline]] void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]] static void free_block(void*p)noexcept{if(p)--live;std::free(p);}
void operator delete(void*p)noexcept{free_block(p);}void operator delete[](void*p)noexcept{free_block(p);}void operator delete(void*p,std::size_t)noexcept{free_block(p);}void operator delete[](void*p,std::size_t)noexcept{free_block(p);}
static void require(bool ok){++checks;if(!ok)std::abort();}
static LineContainer2D make(){LineContainer2D t;for(int i=-32;i<=32;++i)t.add(i,i*i);return t;}
template<class Table>static void inspect(const Table&t){require(t.size()==65);for(int x=-20;x<=20;++x){std::int64_t best=1000000;for(int i=-32;i<=32;++i)best=std::min(best,std::int64_t(i)*x+i*i);require(t.min_rounded_ll(x,1)==best);if constexpr(requires{t.min_with_point(0.L,0.L);}){auto result=t.min_with_point(static_cast<long double>(x),1.L);require(result.value==best&&result.y==result.x*result.x&&result.x>=-32&&result.x<=32);}}}
static void inspect_first_add_failures(){
 for(int budget=0;budget<=2;++budget){const auto before=live;
  {LineContainer2D t;bool failed=false;countdown=budget;
   try{t.add(7,-9);countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}
   require(failed==(budget<2));
   if(failed){
    require(t.empty()&&t.size()==0&&t.minlc.empty()&&t.maxlc.empty());require(live==before);
    const auto inf=std::numeric_limits<long double>::infinity();
    require(t.Xmax==-inf&&t.Xmin==inf&&t.Ymax==-inf&&t.Ymin==inf);
    require(!t.min_optional(1,1)&&!t.max_optional(1,1));
    require(!t.argmin_optional(1,1)&&!t.argmax_optional(1,1));
    t.add(7,-9);
   }
   require(t.size()==1&&t.minlc.size()==1&&t.maxlc.size()==1);
   require(t.Xmin==7&&t.Xmax==7&&t.Ymin==-9&&t.Ymax==-9);
   require(t.min_rounded_ll(3,2)==3&&t.max_rounded_ll(3,2)==3);
   require((t.argmax(3,2)==std::pair<long double,long double>{7,-9}));
   t.add(2,6);require(t.size()==2&&t.min_rounded_ll(3,2)==3&&t.max_rounded_ll(3,2)==18);
   t.clear();require(t.empty()&&live==before);
   t.add(-13,17);require(t.size()==1&&t.min_ld(3,2)==-5&&t.max_ld(3,2)==-5);
  }
  require(live==before);
 }
}
int main(){const auto beginning=live;
 inspect_first_add_failures();require(live==beginning);
 {auto src=make();const auto source_live=live;countdown=0;inspect(src);countdown=-1;require(live==source_live);
  for(int n=0;n<200;++n){bool failed=false;countdown=n;try{LineContainer2D copy(src);countdown=-1;inspect(copy);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==source_live);inspect(src);if(!failed)break;}
  for(int n=0;n<200;++n){LineContainer2D dest;dest.add(1,4);const auto before=live;bool failed=false;countdown=n;try{dest=src;countdown=-1;inspect(dest);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;require(dest.size()==1&&dest.max_ld(3,2)==11);require(live==before);}inspect(src);if(!failed)break;}
 }
 require(live==beginning);
 for(int n=0;n<3;++n){auto t=make();const auto retained=live;bool failed=false;countdown=n;try{t.add(0,-1000000);countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;require(t.empty());require(!t.min_optional(1,1));require(!t.argmax_optional(1,1));require(t.Xmax==-std::numeric_limits<long double>::infinity());require(t.Xmin==std::numeric_limits<long double>::infinity());}
  if(failed){require(live<retained);t.add(7,-9);require(t.size()==1&&t.min_rounded_ll(3,2)==3);}
 }
 require(live==beginning);std::printf("{\"status\":\"PASS\",\"checks\":%zu,\"allocation_failure_points\":%zu,\"live_delta\":%td}\n",checks,failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(beginning));
}
