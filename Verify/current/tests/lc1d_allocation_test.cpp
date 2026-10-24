// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LC1D_SOURCE
#define SIRKIT_LC1D_SOURCE "../../../DataStructure/line-container.cpp"
#endif
#include SIRKIT_LC1D_SOURCE
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <utility>
static std::ptrdiff_t countdown=-1;
static std::size_t live=0,checks=0,failures=0,assignment_empty_after_failure=0,assignment_other_after_failure=0;
[[gnu::noinline]]void*operator new(std::size_t n){if(countdown==0)throw std::bad_alloc();if(countdown>0)--countdown;if(void*p=std::malloc(n?n:1)){++live;return p;}throw std::bad_alloc();}
[[gnu::noinline]]void*operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]]static void free_block(void*p)noexcept{if(p)--live;std::free(p);}
void operator delete(void*p)noexcept{free_block(p);}void operator delete[](void*p)noexcept{free_block(p);}void operator delete(void*p,std::size_t)noexcept{free_block(p);}void operator delete[](void*p,std::size_t)noexcept{free_block(p);}
static void require_impl(bool ok,const char*expression,int line){++checks;if(!ok){std::fprintf(stderr,"require failed line %d after %ju checks: %s\n",line,static_cast<std::uintmax_t>(checks),expression);std::abort();}}
#define require(ok) require_impl(bool(ok),#ok,__LINE__)
using Hull=MinLineContainer<std::int64_t>;
static Hull make(){Hull h;for(int i=-32;i<=32;++i)h.add(i,i*i);return h;}
static void inspect(const Hull&h){require(h.size()==65);for(int x=-80;x<=80;++x){std::int64_t best=1000000;for(int i=-32;i<=32;++i)best=std::min(best,std::int64_t(i)*x+i*i);require(h.query(x)==best);auto[k,m]=h.query_line(x);require(m==k*k&&k>=-32&&k<=32);require(k*x+m==best);if constexpr(requires{h.query_with_line(0);h.query_with_line_optional(0);}){const auto result=h.query_with_line(x);require(result.value==best&&result.k*x+result.m==best);const auto optional=h.query_with_line_optional(x);require(optional.has_value()&&optional->value==best);}}}
int main(){const auto beginning=live;
 {const auto source=make();const auto source_live=live;countdown=0;inspect(source);countdown=-1;require(live==source_live);
  for(int n=0;n<100;++n){bool failed=false;countdown=n;try{Hull copy(source);countdown=-1;inspect(copy);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==source_live);inspect(source);if(!failed)break;}
  for(int n=0;n<100;++n){Hull dest;dest.add(0,9);bool failed=false;countdown=n;try{dest=source;countdown=-1;inspect(dest);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;if(dest.empty())++assignment_empty_after_failure;else ++assignment_other_after_failure;dest.clear();require(live==source_live);dest.add(0,7);require(dest.query(99)==7);}inspect(source);if(!failed)break;}
  auto h=source;const auto before=live;countdown=0;bool failed=false;try{h.add(0,-1000000);countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}if(failed){require(live==before);inspect(h);h.add(0,-1000000);}require(h.query(99)==-1000000);
  h.clear();require(live==source_live);h.add(0,3);require(h.query(99)==3);
 }
 require(live==beginning);std::printf("{\"status\":\"PASS\",\"checks\":%zu,\"failure_points\":%zu,\"live_delta\":%td,\"copy_assignment_empty_after_failure\":%zu,\"copy_assignment_other_after_failure\":%zu}\n",checks,failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(beginning),assignment_empty_after_failure,assignment_other_after_failure);
}
