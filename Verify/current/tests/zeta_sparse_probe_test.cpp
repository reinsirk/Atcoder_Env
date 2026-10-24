// SPDX-License-Identifier: CC0-1.0
// 疎な約数閉包、検索の衝突、既定構築・コピーを禁じた値型を直接定義と照合する。
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta/Sparse_Transform.cpp"
#endif
namespace review_stats {inline std::size_t fallback=0,probe7=0,probe8=0;}
#include SOURCE
static std::size_t checks=0,fixtures=0;
static void need(bool ok){++checks;if(!ok)std::abort();}
static int mu(int n){int result=1;for(int p=2;p<=n/p;++p)if(n%p==0){n/=p;result=-result;if(n%p==0)return 0;}if(n>1)result=-result;return result;}
static std::vector<int> divisors(int n){std::vector<int> keys;for(int d=1;d<=n;++d)if(n%d==0)keys.push_back(d);return keys;}
struct MoveOnly{std::unique_ptr<std::int64_t> value;MoveOnly()=delete;explicit MoveOnly(std::int64_t x):value(std::make_unique<std::int64_t>(x)){}MoveOnly(const MoveOnly&)=delete;MoveOnly&operator=(const MoveOnly&)=delete;MoveOnly(MoveOnly&&)=default;MoveOnly&operator=(MoveOnly&&)=default;MoveOnly&operator+=(const MoveOnly& other){*value+=*other.value;return *this;}MoveOnly&operator-=(const MoveOnly& other){*value-=*other.value;return *this;}};
template<class Val>static Val make(int x){return Val(x);}
template<class Val>static std::int64_t read(const Val& x){if constexpr(std::is_same_v<Val,MoveOnly>)return *x.value;else return static_cast<std::int64_t>(x);}
template<class Key,class Val>static void run(const std::vector<int>& keys,const IntegerSieve& sieve){
 if(static_cast<std::uintmax_t>(keys.back())>static_cast<std::uintmax_t>(std::numeric_limits<Key>::max()))return;
 std::mt19937 rng(61024);for(int shape=0;shape<3;++shape)for(bool inverse:{false,true}){std::vector<std::pair<Key,Val>> actual;for(int k:keys)actual.emplace_back(Key(k),make<Val>(k%11));if(shape==1)std::reverse(actual.begin(),actual.end());if(shape==2)std::shuffle(actual.begin(),actual.end(),rng);
 std::vector<Val> expected;for(int k:keys){Val value=make<Val>(0);for(int j:keys)if(j%k==0){int c=inverse?mu(j/k):1;if(c==1)value+=make<Val>(j%11);if(c==-1)value-=make<Val>(j%11);}expected.push_back(std::move(value));}
 if(inverse)sparse_multiple_mobius_transform(actual,sieve);else sparse_multiple_zeta_transform(actual,sieve);
 need(actual.size()==keys.size());for(std::size_t i=0;i<keys.size();++i){need(static_cast<std::uintmax_t>(actual[i].first)==static_cast<std::uintmax_t>(keys[i]));need(read(actual[i].second)==read(expected[i]));}++fixtures;
 }
}
int main(){IntegerSieve sieve(400000);std::vector<std::vector<int>> all{{1,89,233,733,877,911,1021,1453,1487,1597},divisors(120),divisors(240),divisors(360360)};auto boundary=divisors(40);boundary.push_back(3);std::sort(boundary.begin(),boundary.end());all.push_back(boundary);boundary.push_back(7);std::sort(boundary.begin(),boundary.end());all.push_back(boundary);boundary.push_back(11);std::sort(boundary.begin(),boundary.end());all.push_back(boundary);
 for(const auto& keys:all){for(int key:keys)for(int d=1;d<=key/d;++d)if(key%d==0){need(std::binary_search(keys.begin(),keys.end(),d));need(std::binary_search(keys.begin(),keys.end(),key/d));}
 run<std::int8_t,std::uint8_t>(keys,sieve);run<std::uint8_t,std::uint16_t>(keys,sieve);run<std::int16_t,std::uint64_t>(keys,sieve);run<std::uint16_t,std::int64_t>(keys,sieve);run<std::int32_t,MoveOnly>(keys,sieve);run<std::int64_t,std::int64_t>(keys,sieve);run<std::uint64_t,MoveOnly>(keys,sieve);
 }
#ifndef SKIP_RUNTIME_CHECKS
 for(std::uint64_t key:{UINT64_C(400001),UINT64_C(4294967297),std::numeric_limits<std::uint64_t>::max()})for(bool inverse:{false,true}){std::vector<std::pair<std::uint64_t,int>>bad{{1,0},{key,1}};bool caught=false;try{if(inverse)sparse_multiple_mobius_transform(bad,sieve);else sparse_multiple_zeta_transform(bad,sieve);}catch(const std::out_of_range&){caught=true;}need(caught);}
#endif
#ifdef INSTRUMENTED
 need(review_stats::fallback>0);need(review_stats::probe7>0);need(review_stats::probe8>0);
#endif
 std::printf("PASS checks=%zu fixtures=%zu fallback=%zu probe7=%zu probe8=%zu\n",checks,fixtures,review_stats::fallback,review_stats::probe7,review_stats::probe8);
}
