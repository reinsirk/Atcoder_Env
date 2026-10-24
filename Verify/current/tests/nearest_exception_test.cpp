// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_NEAREST_REQUIRE_EXTRA
#define SIRKIT_NEAREST_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_NEAREST_SOURCE
#define SIRKIT_NEAREST_SOURCE "../../../Geometry/Nearest_Two_points.cpp"
#endif
#include SIRKIT_NEAREST_SOURCE
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <new>
#include <utility>
#include <vector>
static std::ptrdiff_t countdown=-1;
static std::size_t live=0,checks=0,allocation_failures=0,read_failures=0;
[[gnu::noinline]]void*operator new(std::size_t n){if(countdown==0)throw std::bad_alloc();if(countdown>0)--countdown;if(void*p=std::malloc(n?n:1)){++live;return p;}throw std::bad_alloc();}
[[gnu::noinline]]void*operator new[](std::size_t n){return::operator new(n);}
[[gnu::noinline]]static void free_block(void*p)noexcept{if(p)--live;std::free(p);}
void operator delete(void*p)noexcept{free_block(p);}void operator delete[](void*p)noexcept{free_block(p);}void operator delete(void*p,std::size_t)noexcept{free_block(p);}void operator delete[](void*p,std::size_t)noexcept{free_block(p);}
static void require_impl(bool ok,const char*expression,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s (%ju checks)\n",line,expression,static_cast<std::uintmax_t>(checks));std::abort();}}
#define require(ok) require_impl(bool(ok),#ok,__LINE__)
static std::vector<P>make_points(){std::vector<P>v;for(std::int32_t i=0;i<129;++i)v.emplace_back(i*1000,10000000+(i*i*77)%100000);v.emplace_back(12345,67890);v.emplace_back(12346,67890);return v;}
static void inspect(const std::optional<NearestTwoPointsResult>&r,const std::vector<P>&v){require(bool(r));require(r->distance_squared==1);require(r->first_index<r->second_index&&r->second_index<v.size());const auto a=v[r->first_index],b=v[r->second_index];const std::int64_t x=std::int64_t(a.first)-b.first,y=std::int64_t(a.second)-b.second;require(x*x+y*y==1);}
template<class V>static auto invoke(const V&v,int mode){
 if(mode==0)return nearest_two_points(v);
 if(mode==1)return nearest_two_points(v.begin(),static_cast<std::ptrdiff_t>(v.size()));
 if constexpr(requires{nearest_two_points_sorted(v);})return nearest_two_points_sorted(v);
 else return nearest_two_points(v.begin(),v.end());
}
struct ReadFailure{};
struct ReadState{const std::vector<P>*v=nullptr;std::size_t index=0;std::size_t fail_index=0;bool increment=false;};
struct ReadEnd{};
struct ReadIterator{
 using value_type=P;using difference_type=std::ptrdiff_t;using iterator_concept=std::input_iterator_tag;using iterator_category=std::input_iterator_tag;
 std::shared_ptr<ReadState>state;
 const P&operator*()const{if(!state->increment&&state->index==state->fail_index)throw ReadFailure{};return(*state->v)[state->index];}
 ReadIterator&operator++(){if(state->increment&&state->index==state->fail_index)throw ReadFailure{};++state->index;return*this;}void operator++(int){++*this;}
 friend bool operator==(const ReadIterator&i,ReadEnd){return!i.state||i.state->index==i.state->v->size();}
};
static_assert(std::input_iterator<ReadIterator> && std::sentinel_for<ReadEnd,ReadIterator>);
template<class Iterator>static auto invoke_iterator(Iterator first,ReadEnd last,bool sorted){if constexpr(requires{nearest_two_points_sorted(first,last);}){if(sorted)return nearest_two_points_sorted(first,last);}return nearest_two_points(first,last);}
int main(){const auto beginning=live;
 {const std::vector<P>empty;countdown=0;require(!nearest_two_points(empty));countdown=-1;
#ifdef SIRKIT_NEAREST_REQUIRE_EXTRA
 countdown=0;require(!nearest_two_points_sorted(empty));countdown=-1;
#endif
 }
 require(live==beginning);
 for(int mode=0;mode<3;++mode){auto v=make_points();if(mode==2)std::sort(v.begin(),v.end());const auto original=v;const auto retained=live;bool succeeded=false;
  for(int budget=0;budget<64;++budget){bool failed=false;countdown=budget;try{const auto r=invoke(v,mode);countdown=-1;inspect(r,v);}catch(const std::bad_alloc&){countdown=-1;failed=true;++allocation_failures;}
   require(v==original);require(live==retained);if(!failed){succeeded=true;break;}}
  require(succeeded);
 }
 require(live==beginning);
 for(bool sorted:{false,true})for(bool increment:{false,true})for(const std::size_t at:{std::size_t{0},std::size_t{1},std::size_t{2},std::size_t{15},std::size_t{63},std::size_t{130}}){
  auto v=make_points();if(sorted)std::sort(v.begin(),v.end());const auto original=v;auto state=std::make_shared<ReadState>();state->v=&v;state->fail_index=at;state->increment=increment;const auto retained=live;bool failed=false;
  try{(void)invoke_iterator(ReadIterator{state},ReadEnd{},sorted);}catch(const ReadFailure&){failed=true;++read_failures;}
  require(failed);require(state->index<=v.size());require(v==original);require(live==retained);
 }
 require(live==beginning);
 std::optional<NearestTwoPointsResult>saved;
 {auto v=make_points();saved=nearest_two_points(v);inspect(saved,v);}
 require(saved&&saved->distance_squared==1);require(live==beginning);
 std::printf("PASS %ju checks; allocation_failures=%ju read_failures=%ju live_delta=%jd\n",static_cast<std::uintmax_t>(checks),static_cast<std::uintmax_t>(allocation_failures),static_cast<std::uintmax_t>(read_failures),static_cast<std::intmax_t>(live)-static_cast<std::intmax_t>(beginning));
}
