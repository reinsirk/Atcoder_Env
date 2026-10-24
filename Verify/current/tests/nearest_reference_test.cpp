// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_NEAREST_REQUIRE_EXTRA
#define SIRKIT_NEAREST_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_NEAREST_SOURCE
#define SIRKIT_NEAREST_SOURCE "../../../Geometry/Nearest_Two_points.cpp"
#endif
#include SIRKIT_NEAREST_SOURCE
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
using Big=boost::multiprecision::cpp_int;
using I=std::int32_t;
static std::size_t checks=0,cases=0,pair_checks=0,overflow_cases=0;
static void require_impl(bool ok,const char*expression,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s (%ju checks)\n",line,expression,static_cast<std::uintmax_t>(checks));std::abort();}}
#define require(ok) require_impl(bool(ok),#ok,__LINE__)
template<class A,class B>static Big squared(const A&a,const B&b){const Big dx=Big(a.first)-Big(b.first),dy=Big(a.second)-Big(b.second);return dx*dx+dy*dy;}
template<class V>static std::optional<Big> reference(const V&v){std::optional<Big>best;for(std::size_t i=0;i<v.size();++i)for(std::size_t j=0;j<i;++j){const Big d=squared(v[i],v[j]);++pair_checks;if(!best||d<*best)best=d;}return best;}
template<class V,class Function>static void inspect(const V&v,const std::optional<Big>&expected,Function fn){
 const bool over=expected&&*expected>std::numeric_limits<std::uint64_t>::max();
 if(over){bool threw=false;try{(void)fn();}catch(const std::overflow_error&){threw=true;}require(threw);++overflow_cases;return;}
 const auto got=fn();require(bool(got)==bool(expected));if(!got)return;
 require(got->first_index<got->second_index);require(got->second_index<v.size());require(Big(got->distance_squared)==*expected);require(squared(v[got->first_index],v[got->second_index])==*expected);
}
#ifdef SIRKIT_NEAREST_REQUIRE_EXTRA
static_assert(requires(const std::vector<P>&v){nearest_two_points_sorted(v);nearest_two_points_sorted(v.begin(),v.end());nearest_two_points_sorted(v.begin(),std::ptrdiff_t{});});
#endif
template<class V>static void inspect_sorted(const V&v,const std::optional<Big>&expected){
 if constexpr(requires{nearest_two_points_sorted(v);}){
  auto sorted=v;std::sort(sorted.begin(),sorted.end());const auto copy=sorted;
  inspect(sorted,expected,[&]{return nearest_two_points_sorted(sorted);});
  inspect(sorted,expected,[&]{return nearest_two_points_sorted(sorted.begin(),sorted.end());});
  inspect(sorted,expected,[&]{return nearest_two_points_sorted(sorted.begin(),static_cast<std::ptrdiff_t>(sorted.size()));});
  require(sorted==copy);
 }
}
template<class Point>static void check(const std::vector<Point>&v,bool all=true){
 ++cases;const auto copy=v;const auto expected=reference(v);
 inspect(v,expected,[&]{return nearest_two_points(v);});inspect(v,expected,[&]{return nearest_two_points(v.cbegin(),v.cend());});
 if(all){inspect(v,expected,[&]{return nearest_two_points(v.cbegin(),static_cast<std::ptrdiff_t>(v.size()));});
  const bool over=expected&&*expected>std::numeric_limits<std::uint64_t>::max();bool threw=false;
  try{const auto got=closest_pair(v.cbegin(),static_cast<int>(v.size()));require(!over);require(got==(expected?expected->template convert_to<std::uint64_t>():static_cast<std::uint64_t>(std::numeric_limits<I>::max())));}catch(const std::overflow_error&){threw=true;}
  require(threw==over);if(over)++overflow_cases;
 }
 inspect_sorted(v,expected);require(v==copy);
}
struct InputState{const std::vector<P>*v=nullptr;std::size_t index=0;};
struct InputEnd{};
struct Input{
 using value_type=P;using difference_type=std::ptrdiff_t;using iterator_concept=std::input_iterator_tag;using iterator_category=std::input_iterator_tag;
 std::shared_ptr<InputState>state;
 const P&operator*()const{return(*state->v)[state->index];}Input&operator++(){++state->index;return*this;}void operator++(int){++*this;}
 friend bool operator==(const Input&i,InputEnd){return!i.state||i.state->index==i.state->v->size();}
};
static_assert(std::input_iterator<Input> && std::sentinel_for<InputEnd,Input>);
#ifdef SIRKIT_NEAREST_REQUIRE_EXTRA
struct MoveInput{
 using value_type=P;using difference_type=std::ptrdiff_t;using iterator_concept=std::input_iterator_tag;using iterator_category=std::input_iterator_tag;
 std::shared_ptr<InputState>state;
 MoveInput()=default;explicit MoveInput(std::shared_ptr<InputState>s):state(std::move(s)){}
 MoveInput(MoveInput&&)=default;MoveInput&operator=(MoveInput&&)=default;MoveInput(const MoveInput&)=delete;MoveInput&operator=(const MoveInput&)=delete;
 const P&operator*()const{return(*state->v)[state->index];}MoveInput&operator++(){++state->index;return*this;}void operator++(int){++*this;}
 friend bool operator==(const MoveInput&i,InputEnd){return!i.state||i.state->index==i.state->v->size();}
};
static_assert(std::input_iterator<MoveInput> && std::sentinel_for<InputEnd,MoveInput>);
#endif
struct HugeEnd{};
struct Huge{
 using value_type=P;using difference_type=std::ptrdiff_t;using iterator_concept=std::input_iterator_tag;using iterator_category=std::input_iterator_tag;
 std::ptrdiff_t index=0;P operator*()const{return{1,1};}Huge&operator++(){++index;return*this;}void operator++(int){++*this;}
 friend bool operator==(Huge i,HugeEnd){return i.index==std::numeric_limits<std::ptrdiff_t>::max();}
 friend std::ptrdiff_t operator-(HugeEnd,Huge i){return std::numeric_limits<std::ptrdiff_t>::max()-i.index;}
 friend std::ptrdiff_t operator-(Huge i,HugeEnd){return i.index-std::numeric_limits<std::ptrdiff_t>::max();}
};
static_assert(std::input_iterator<Huge> && std::sized_sentinel_for<HugeEnd,Huge>);
static void iterator_cases(){
 for(const std::vector<P>&v: {std::vector<P>{},std::vector<P>{{1,2}},std::vector<P>{{9,7},{-3,2},{4,5},{5,5}},std::vector<P>{{std::numeric_limits<I>::min(),std::numeric_limits<I>::min()},{std::numeric_limits<I>::max(),std::numeric_limits<I>::max()}}}){
  const auto expected=reference(v);auto state=std::make_shared<InputState>();state->v=&v;inspect(v,expected,[&]{return nearest_two_points(Input{state},InputEnd{});});
  require(state->index<=v.size());
  auto state2=std::make_shared<InputState>();state2->v=&v;inspect(v,expected,[&]{return nearest_two_points(Input{state2},static_cast<std::ptrdiff_t>(v.size()));});
#ifdef SIRKIT_NEAREST_REQUIRE_EXTRA
  auto move_state=std::make_shared<InputState>();move_state->v=&v;inspect(v,expected,[&]{return nearest_two_points(MoveInput{move_state},InputEnd{});});
  auto count_state=std::make_shared<InputState>();count_state->v=&v;inspect(v,expected,[&]{return nearest_two_points(MoveInput{count_state},static_cast<std::ptrdiff_t>(v.size()));});
  auto legacy_state=std::make_shared<InputState>();legacy_state->v=&v;const bool over=expected&&*expected>std::numeric_limits<std::uint64_t>::max();bool threw=false;
  try{const auto distance=closest_pair(MoveInput{legacy_state},static_cast<int>(v.size()));require(!over);require(distance==(expected?expected->convert_to<std::uint64_t>():static_cast<std::uint64_t>(std::numeric_limits<I>::max())));}catch(const std::overflow_error&){threw=true;}
  require(threw==over);
  auto sorted=v;std::sort(sorted.begin(),sorted.end());auto sorted_state=std::make_shared<InputState>();sorted_state->v=&sorted;inspect(sorted,expected,[&]{return nearest_two_points_sorted(MoveInput{sorted_state},InputEnd{});});
  auto sorted_count=std::make_shared<InputState>();sorted_count->v=&sorted;inspect(sorted,expected,[&]{return nearest_two_points_sorted(MoveInput{sorted_count},static_cast<std::ptrdiff_t>(sorted.size()));});
#endif
  std::deque<P>q(v.begin(),v.end());inspect(v,expected,[&]{return nearest_two_points(q.begin(),q.end());});
 }
 bool capacity=false;try{(void)nearest_two_points(Huge{},HugeEnd{});}catch(const std::length_error&){capacity=true;}require(capacity);
#ifdef SIRKIT_NEAREST_REQUIRE_EXTRA
 bool sorted_capacity=false;try{(void)nearest_two_points_sorted(Huge{},HugeEnd{});}catch(const std::length_error&){sorted_capacity=true;}require(sorted_capacity);
#endif
}
static void invalid_cases(){
#ifndef SKIP_RUNTIME_CHECKS
 const std::vector<P>v{{1,2},{4,7}};
#ifdef SIRKIT_NEAREST_REQUIRE_EXTRA
 for(const auto&unsorted:std::vector<std::vector<P>>{{{2,0},{1,0}},{{0,2},{0,1}},{{1,0},{0,0},{1,0}}}){bool bad=false;try{(void)nearest_two_points_sorted(unsorted);}catch(const std::invalid_argument&){bad=true;}require(bad);}
 for(int kind=0;kind<2;++kind){bool bad=false;try{if(kind==0)(void)nearest_two_points_sorted(v.end(),v.begin());else(void)nearest_two_points_sorted(v.begin(),std::ptrdiff_t{-1});}catch(const std::invalid_argument&){bad=true;}require(bad);}
#endif
 for(int kind=0;kind<3;++kind){bool bad=false;try{if(kind==0)(void)nearest_two_points(v.end(),v.begin());else if(kind==1)(void)nearest_two_points(v.begin(),std::ptrdiff_t{-1});else(void)closest_pair(v.begin(),-1);}catch(const std::invalid_argument&){bad=true;}require(bad);}
 for(const auto value:{std::int64_t(std::numeric_limits<I>::min())-1,std::int64_t(std::numeric_limits<I>::max())+1,std::numeric_limits<std::int64_t>::min(),std::numeric_limits<std::int64_t>::max()}){
  for(bool second:{false,true}){std::vector<std::pair<std::int64_t,std::int64_t>>p{{second?0:value,second?value:0}};bool bad=false;try{(void)nearest_two_points(p);}catch(const std::out_of_range&){bad=true;}require(bad);}
 }
 for(const auto value:{std::uint64_t{2147483648},std::numeric_limits<std::uint64_t>::max()}){const std::vector<std::pair<std::uint64_t,std::uint64_t>>p{{value,0}};bool bad=false;try{(void)nearest_two_points(p);}catch(const std::out_of_range&){bad=true;}require(bad);}
#endif
}
int main(){
 const I low=std::numeric_limits<I>::min(),high=std::numeric_limits<I>::max();
 for(const auto& v:std::vector<std::vector<P>>{{},{{0,0}},{{0,0},{0,0}},{{low,low},{high,high}},{{low,0},{high,92681}},{{low,0},{high,92682}},{{low,0},{high,92682},{high,92882}},{{low,low},{0,0}},{{low,low},{high,high},{high-1,high}},{{low,low},{high,high},{low,high},{high,low}},{{0,0},{1,0},{0,1},{1,1}},{{1,0},{-1,0},{0,1},{0,-1}}})check(v);
 std::mt19937_64 rng(825173);
 for(int n=0;n<=90;++n)for(int mode=0;mode<6;++mode){std::vector<P>v;v.reserve(n);for(int i=0;i<n;++i){I x{},y{};
  if(mode==0){x=static_cast<I>(std::int64_t(rng()%UINT64_C(4294967296))-INT64_C(2147483648));y=static_cast<I>(std::int64_t(rng()%UINT64_C(4294967296))-INT64_C(2147483648));}
  if(mode==1){x=static_cast<I>(rng()%31)-15;y=static_cast<I>(rng()%31)-15;}
  if(mode==2){x=i;y=17;}
  if(mode==3){x=0;y=i%7;}
  if(mode==4){x=i%11;y=i/11;}
  if(mode==5){x=(i%2)?high-i:low+i;y=(i%3)?high-i:low+i;}
  v.emplace_back(x,y);
 }check(v);}
 for(const int n:{127,128,129,255,256,257,513}){std::vector<P>v;for(int i=0;i<n;++i)v.emplace_back(i*31,(i*i*17)%65521);check(v);std::reverse(v.begin(),v.end());check(v);}
 for(const int n:{2047,2048,2049}){std::vector<P>v;v.reserve(n);for(int i=0;i<n;++i){
  if(n==2047)v.emplace_back(static_cast<I>(std::int64_t(rng()%UINT64_C(4294967296))-INT64_C(2147483648)),static_cast<I>(std::int64_t(rng()%UINT64_C(4294967296))-INT64_C(2147483648)));
  else if(n==2048)v.emplace_back((i%64-32)*INT32_C(16777216),i*65536);
  else v.emplace_back(-17,(i*1009)%4093);
 }check(v);}
 check(std::vector<std::pair<std::int8_t,std::int16_t>>{{-127,321},{126,-324},{0,0},{-1,2}});
 check(std::vector<std::pair<std::int64_t,std::int64_t>>{{low,high},{high,low},{0,0},{1,0}});
 check(std::vector<std::pair<std::uint64_t,std::uint64_t>>{{0,0},{2147483647,2147483647},{2147483646,2147483647}});
 check(std::vector<std::pair<std::uint32_t,std::uint32_t>>{{0,0},{2147483647,2147483647},{1,1}});
 check(std::vector<std::pair<bool,bool>>{{false,false},{true,false},{true,true}});
 iterator_cases();invalid_cases();
 std::printf("PASS %ju checks; %ju cases; %ju exact pair evaluations; %ju overflow calls\n",static_cast<std::uintmax_t>(checks),static_cast<std::uintmax_t>(cases),static_cast<std::uintmax_t>(pair_checks),static_cast<std::uintmax_t>(overflow_cases));
}
