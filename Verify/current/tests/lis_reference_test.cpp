// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LIS_REQUIRE_EXTRA
#define SIRKIT_LIS_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_LIS_SOURCE
#define SIRKIT_LIS_SOURCE "../../../Algorithm/LIS.cpp"
#endif
#include SIRKIT_LIS_SOURCE
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
static std::uint64_t checks=0,cases=0;
static void require_impl(bool ok,int line){++checks;if(!ok){std::fprintf(stderr,"line %d failed after %ju checks\n",line,static_cast<std::uintmax_t>(checks));std::abort();}}
#define require(x) require_impl(bool(x),__LINE__)
template<class T> static auto key(const T& value){if constexpr(requires{value.value;})return value.value;else return value;}
struct NoDefault {
 int value;NoDefault()=delete;explicit NoDefault(int v):value(v){}
 friend bool operator<(const NoDefault&a,const NoDefault&b){return a.value<b.value;}
};
struct MoveOnly {
 int value;MoveOnly()=delete;explicit MoveOnly(int v):value(v){}
 MoveOnly(const MoveOnly&)=delete;MoveOnly&operator=(const MoveOnly&)=delete;
 MoveOnly(MoveOnly&&)=default;MoveOnly&operator=(MoveOnly&&)=default;
 friend bool operator<(const MoveOnly&a,const MoveOnly&b){return a.value<b.value;}
};
template<bool Strict,class T>static std::vector<std::size_t> reference(const std::vector<T>& values){
 std::vector<std::size_t> dp(values.size(),1),prefix(values.size());std::size_t best=0;
 for(std::size_t i=0;i<values.size();++i){for(std::size_t j=0;j<i;++j){const auto a=key(values[j]),b=key(values[i]);if(Strict?a<b:!(b<a))dp[i]=std::max(dp[i],dp[j]+1);}best=std::max(best,dp[i]);prefix[i]=best;}return prefix;
}
template<bool Strict,class T>static void check_indices(const std::vector<T>& values,const std::vector<std::size_t>& indices,std::size_t length){
 require(indices.size()==length);for(std::size_t i=0;i<indices.size();++i){require(indices[i]<values.size());if(i){require(indices[i-1]<indices[i]);const auto a=key(values[indices[i-1]]),b=key(values[indices[i]]);require(Strict?a<b:!(b<a));}}
}
template<bool Strict,class T>static void check_extra(const std::vector<T>& values,const std::vector<std::size_t>& expected){
#ifdef SIRKIT_LIS_REQUIRE_EXTRA
 static_assert(requires{LISEndingLengths<Strict>(values);});
#endif
 if constexpr(requires{LISEndingLengths<Strict>(values);}){
  static_assert(std::is_same_v<decltype(LISEndingLengths<Strict>(values)),std::vector<std::size_t>>);
  const auto ending=LISEndingLengths<Strict>(values);require(ending.size()==values.size());
  // 1024超は単調列・定数列だけで、末尾長と接頭辞長が一致する。
  if(values.size()>1024){require(ending==expected);return;}
  std::vector<std::size_t> dp(values.size(),1);std::size_t best=0;
  for(std::size_t i=0;i<values.size();++i){for(std::size_t j=0;j<i;++j){const auto a=key(values[j]),b=key(values[i]);if(Strict?a<b:!(b<a))dp[i]=std::max(dp[i],dp[j]+1);}require(ending[i]==dp[i]);best=std::max(best,ending[i]);require(best==expected[i]);}
 }
}
template<bool Strict,class T>static void inspect(const std::vector<T>& values,const std::vector<std::size_t>& expected){
 ++cases;const std::size_t length=expected.empty()?0:expected.back();
 const auto prefix=LIS<Strict>(values);require(prefix==expected);require(LISLength<Strict>(values)==length);
 const auto indices=LISIndices<Strict>(values);check_indices<Strict>(values,indices,length);
 const auto sequence=LISSequence<Strict>(values);require(sequence.size()==length);std::size_t position=0;
 for(std::size_t i=0;i<sequence.size();++i){if(i){const auto a=key(sequence[i-1]),b=key(sequence[i]);require(Strict?a<b:!(b<a));}while(position<values.size()&&key(values[position])!=key(sequence[i]))++position;require(position<values.size());++position;}
 check_extra<Strict>(values,expected);
}
template<class T>static void inspect_both(const std::vector<T>& values){inspect<true>(values,reference<true>(values));inspect<false>(values,reference<false>(values));}
template<class T>static void ending_ownership(std::vector<T> values){
 if constexpr(requires{LISEndingLengths(values);}){const auto ending=LISEndingLengths(values);require(ending==std::vector<std::size_t>({1,2,3,1}));values.clear();require(ending==std::vector<std::size_t>({1,2,3,1}));}
}
static void small_exhaustive(){
 for(std::size_t n=0,power=1;n<=7;++n,power*=3){for(std::size_t mask=0;mask<power;++mask){std::vector<int> v(n);auto number=mask;for(auto&x:v){x=int(number%3)-1;number/=3;}inspect_both(v);}}
 // 小さい列は全ての部分集合も参照し、DPの参照自体を確認する。
 for(unsigned mask=0;mask<729;++mask){std::vector<int> v(6);auto code=mask;for(auto&x:v){x=int(code%3)-1;code/=3;}for(bool strict:{false,true}){std::size_t best=0;for(unsigned chosen=0;chosen<(1u<<v.size());++chosen){bool okay=true,first=true;int previous=0;std::size_t count=0;for(std::size_t i=0;i<v.size();++i)if(chosen>>i&1){if(!first&&(strict?previous>=v[i]:previous>v[i]))okay=false;first=false;previous=v[i];++count;}if(okay)best=std::max(best,count);}const auto dp=strict?reference<true>(v):reference<false>(v);require(dp.back()==best);}}
}
template<class T>static void numeric_edges(){
 using L=std::numeric_limits<T>;for(std::size_t n:{15u,16u,17u,31u,32u,33u,63u,64u,65u,127u,128u,129u}){std::vector<T> medium(n);for(std::size_t i=0;i<n;++i)medium[i]=T(int((i*73)%31)-15)/T{4};inspect_both(medium);}
 std::vector<T> v{L::lowest(),L::max(),T{0},L::lowest(),L::max(),T{-1},T{1},T{0},T{0}};inspect_both(v);
 if constexpr(std::is_floating_point_v<T>){inspect_both(std::vector<T>{-L::infinity(),T{0},L::infinity(),-L::infinity(),L::infinity(),T{0}});for(T x:{T{-1},T{0},T{1},L::min(),L::max(),L::lowest()}){const auto before=std::nextafter(x,L::lowest()),after=std::nextafter(x,L::max());inspect_both(std::vector<T>{before,x,after,x,before,after});}inspect_both(std::vector<T>{-T{0},T{0},L::denorm_min(),-L::denorm_min(),T{0}});}
}
template<class T>static void unsigned_edges(){
 using L=std::numeric_limits<T>;const T high=T{1}<<(L::digits-1);inspect_both(std::vector<T>{T{0},L::max(),high,T{1},T(L::max()-1),high,T{0}});
 std::vector<T> v(129);for(std::size_t i=0;i<v.size();++i)v[i]=i%3==0?L::max():i%3==1?T(i):T(high+T(i));inspect_both(v);
}
static void random_and_capacity(){
 std::mt19937_64 rng(517823);
 for(int test=0;test<1200;++test){const std::size_t n=rng()%81;std::vector<std::int64_t> v(n);for(auto&x:v)x=std::int64_t(rng()%41)-20;inspect_both(v);}
 for(std::size_t n:{0u,1u,2u,15u,16u,17u,31u,32u,33u,63u,64u,65u,127u,128u,129u,255u,256u,257u,511u,512u,513u}){
  std::vector<int> v(n);for(std::size_t i=0;i<n;++i)v[i]=int(i%2?i:2*n-i);inspect_both(v);std::shuffle(v.begin(),v.end(),rng);inspect_both(v);
 }
 for(std::size_t n:{4097u,65537u})for(int shape=0;shape<3;++shape){std::vector<std::int64_t> v(n);std::vector<std::size_t> strict(n),wide(n);for(std::size_t i=0;i<n;++i){v[i]=shape==0?std::int64_t(i):shape==1?-std::int64_t(i):0;strict[i]=shape==0?i+1:1;wide[i]=shape==1?1:i+1;}inspect<true>(v,strict);inspect<false>(v,wide);}
}
template<class F>static void expect_invalid(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}require(rejected);}
template<class T>static void nan_checks(){
#if defined(SIRKIT_LIS_REQUIRE_EXTRA) && !defined(SKIP_RUNTIME_CHECKS)
 for(std::size_t n:{1u,129u})for(std::size_t at:{std::size_t{0},n/2,n-1}){
  std::vector<T> v(n);for(std::size_t i=0;i<n;++i)v[i]=T(i);v[at]=std::numeric_limits<T>::quiet_NaN();
  expect_invalid([&]{(void)LIS<true>(v);});expect_invalid([&]{(void)LIS<false>(v);});expect_invalid([&]{(void)LISLength<true>(v);});expect_invalid([&]{(void)LISLength<false>(v);});expect_invalid([&]{(void)LISIndices<true>(v);});expect_invalid([&]{(void)LISIndices<false>(v);});expect_invalid([&]{(void)LISSequence<true>(v);});expect_invalid([&]{(void)LISSequence<false>(v);});
  expect_invalid([&]{(void)LISEndingLengths<true>(v);});expect_invalid([&]{(void)LISEndingLengths<false>(v);});
 }
#endif
}
static void generic_and_ownership(){
 inspect_both(std::vector<bool>{true,false,true,true,false,true,false});inspect_both(std::vector<std::string>{"ab","a","b","b","","zz","x"});
 const std::vector<int> raw{3,1,2,2,4,-5,-5,0,7};std::vector<NoDefault> v;for(int x:raw)v.emplace_back(x);inspect_both(v);
 std::vector<MoveOnly> movable;for(int x:raw)movable.emplace_back(x);check_indices<true>(movable,LISIndices(movable),reference<true>(raw).back());check_indices<false>(movable,LISIndices<false>(movable),reference<false>(raw).back());
 std::vector<int> input{3,1,2,2,4};const auto original=input;auto prefix=LIS<true>(input);auto indices=LISIndices(input);auto sequence=LISSequence<false>(input);require(input==original);input.assign(100,999);require(prefix==std::vector<std::size_t>({1,1,2,2,3}));require(indices.size()==3);require(sequence==std::vector<int>({1,2,2,4}));
 require(LISLength(original)==LISLength<true>(original));check_indices<true>(original,LISIndices(original),3);check_indices<true>(original,indices,3);
}
int main(){small_exhaustive();random_and_capacity();numeric_edges<std::int8_t>();numeric_edges<std::int16_t>();numeric_edges<std::int32_t>();numeric_edges<std::int64_t>();unsigned_edges<std::uint8_t>();unsigned_edges<std::uint16_t>();unsigned_edges<std::uint32_t>();unsigned_edges<std::uint64_t>();numeric_edges<float>();numeric_edges<double>();numeric_edges<long double>();generic_and_ownership();ending_ownership(std::vector<int>{1,2,3,0});nan_checks<float>();nan_checks<double>();nan_checks<long double>();std::printf("PASS %ju checks; %ju API cases; independent DP/subset reference\n",static_cast<std::uintmax_t>(checks),static_cast<std::uintmax_t>(cases));}
