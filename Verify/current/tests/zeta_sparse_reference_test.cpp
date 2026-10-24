// SPDX-License-Identifier: CC0-1.0
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta/Sparse_Transform.cpp"
#endif
#include SOURCE
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <vector>
namespace {std::uint64_t checks=0;void check(bool x){++checks;if(!x)std::abort();}int mu(std::uint64_t n){int sign=1;for(std::uint64_t p=2;p<=n/p;++p)if(n%p==0){n/=p;sign=-sign;if(n%p==0)return 0;}if(n>1)sign=-sign;return sign;}
struct Mod{std::uint32_t v=0;Mod()=default;Mod(std::int64_t x){x%=998244353;if(x<0)x+=998244353;v=static_cast<std::uint32_t>(x);}Mod&operator+=(Mod b){v+=b.v;if(v>=998244353)v-=998244353;return *this;}Mod&operator-=(Mod b){if(v<b.v)v+=998244353;v-=b.v;return *this;}friend bool operator==(Mod,Mod)=default;};
template<class Key,class Value>void run_type(){IntegerSieve sieve(100000);std::mt19937_64 rng(61022);for(int t=0;t<100;++t){std::set<int>keys;if(t<10){for(int i=1;i<=t*7;++i)keys.insert(i);}else{for(int k=0;k<3;++k){const int n=1+static_cast<int>(rng()%120);for(int d=1;d<=n;++d)if(n%d==0)keys.insert(d);}}std::vector<std::pair<Key,Value>>a;for(int k:keys)a.emplace_back(static_cast<Key>(k),Value(rng()%9));if(t%3==0)std::shuffle(a.begin(),a.end(),rng);else if(t%3==1)std::reverse(a.begin(),a.end());auto sorted=a;std::sort(sorted.begin(),sorted.end(),[](const auto&a,const auto&b){return a.first<b.first;});for(bool inv:{false,true}){auto expected=sorted;for(std::size_t i=0;i<expected.size();++i){Value sum{};for(std::size_t j=0;j<sorted.size();++j)if(sorted[j].first%sorted[i].first==0){const auto c=inv?mu(static_cast<std::uint64_t>(sorted[j].first/sorted[i].first)):1;if(c==1)sum+=sorted[j].second;else if(c==-1)sum-=sorted[j].second;}expected[i].second=sum;}auto actual=a;if(inv)sparse_multiple_mobius_transform(actual,sieve);else sparse_multiple_zeta_transform(actual,sieve);check(actual==expected);}auto actual=a;sparse_multiple_zeta_transform(actual,sieve);sparse_multiple_mobius_transform(actual,sieve);check(actual==sorted);}
 for(int n:{1,2,32,128})for(int shape=0;shape<4;++shape){std::vector<std::pair<Key,Value>>a;for(int k=1;k<=n;++k)a.emplace_back(static_cast<Key>(k+(shape==3&&n>1&&k==n)),Value(k%7));if(shape==1)std::reverse(a.begin(),a.end());if(shape==2)std::shuffle(a.begin(),a.end(),rng);auto original=a;std::sort(original.begin(),original.end(),[](const auto&a,const auto&b){return a.first<b.first;});sparse_multiple_zeta_transform(a,sieve);sparse_multiple_mobius_transform(a,sieve);check(a==original);}
}
struct MoveOnly{std::unique_ptr<std::int64_t>p;explicit MoveOnly(std::int64_t v=0):p(std::make_unique<std::int64_t>(v)){}MoveOnly(MoveOnly&&)=default;MoveOnly&operator=(MoveOnly&&)=default;MoveOnly&operator+=(const MoveOnly&b){*p+=*b.p;return *this;}MoveOnly&operator-=(const MoveOnly&b){*p-=*b.p;return *this;}};
void large_dense(){constexpr int n=20000;IntegerSieve sieve(n);std::vector<std::pair<int,std::int64_t>>a;for(int k=1;k<=n;++k)a.emplace_back(k,k%13-6);auto original=a,expected=a;for(int k=1;k<=n;++k){std::int64_t sum=0;for(int j=k;j<=n;j+=k)sum+=original[j-1].second;expected[k-1].second=sum;}sparse_multiple_zeta_transform(a,sieve);check(a==expected);sparse_multiple_mobius_transform(a,sieve);check(a==original);
 auto almost=original;std::swap(almost[almost.size()-3],almost[almost.size()-2]);sparse_multiple_zeta_transform(almost,sieve);check(almost==expected);sparse_multiple_mobius_transform(almost,sieve);check(almost==original);
 std::vector<std::pair<int,MoveOnly>>m;for(int i=1;i<=128;++i)m.emplace_back(i,MoveOnly(i%7));sparse_multiple_zeta_transform(m,sieve);sparse_multiple_mobius_transform(m,sieve);for(int i=1;i<=128;++i)check(*m[i-1].second.p==i%7);std::reverse(m.begin(),m.end());sparse_multiple_zeta_transform(m,sieve);sparse_multiple_mobius_transform(m,sieve);for(int i=1;i<=128;++i)check(*m[i-1].second.p==i%7);
#ifndef SKIP_RUNTIME_CHECKS
 for(auto keys:std::vector<std::vector<int>>{{0},{-1,1},{1,1,3},{1,3,6}}){std::vector<std::pair<int,int>>bad;for(auto k:keys)bad.emplace_back(k,1);bool caught=false;try{sparse_multiple_zeta_transform(bad,sieve);}catch(const std::invalid_argument&){caught=true;}catch(const std::out_of_range&){caught=true;}check(caught);}std::vector<std::pair<int,int>>one{{1,7}};bool caught=false;try{sparse_multiple_zeta_transform(one,IntegerSieve(0));}catch(const std::out_of_range&){caught=true;}check(caught);
#endif
}
}
int main(){run_type<std::int16_t,std::uint8_t>();run_type<std::uint16_t,std::uint16_t>();run_type<std::int32_t,std::uint64_t>();run_type<std::int64_t,std::int64_t>();run_type<std::int32_t,Mod>();large_dense();std::cout<<"PASS checks="<<checks<<'\n';}
