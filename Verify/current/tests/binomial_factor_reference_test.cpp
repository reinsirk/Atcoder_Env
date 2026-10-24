// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>
#ifndef BINOMIAL_SOURCE
#define BINOMIAL_SOURCE "../../../Math/Combination/Binomial_Mod.cpp"
#endif
#include BINOMIAL_SOURCE
#include BINOMIAL_SOURCE
using boost::multiprecision::cpp_int;
static std::uint64_t checks=0, queries=0, limit_failures=0;
#define REQUIRE(x) do { ++checks; if(!(x)){std::cerr<<"failed line "<<__LINE__<<" "<<#x<<'\n';std::abort();} }while(false)
static bool prime(std::uint32_t m){if(m<2)return false;for(std::uint64_t p=2;p*p<=m;++p)if(m%p==0)return false;return true;}
static std::uint32_t exact(std::int64_t n,std::int64_t k,std::uint32_t m){if(n<0||k<0||k>n||m==1)return 0;k=std::min(k,n-k);cpp_int a=1;for(std::int64_t i=1;i<=k;++i){a*=n-k+i;a/=i;}return static_cast<std::uint32_t>(a%m);}
static std::vector<std::uint32_t> moduli(){std::vector<std::uint32_t> v;for(std::uint32_t i=1;i<=96;++i)v.push_back(i);for(auto m:{97u,101u,127u,128u,243u,256u,343u,512u,625u,729u,1024u,2187u,2401u,3125u,4096u,6561u,65536u,531441u,1048576u,16777216u,1073741824u,2147483648u,1162261467u,1220703125u,1977326743u,2357947691u,815730721u,2147483647u,4294967291u,4294967295u,4294967294u,4000000000u,1000000000u,1000000007u,998244353u,360u,999999937u})v.push_back(m);for(auto m:{131042u,131074u,2097146u,33554426u,536870798u,805306197u,1342176995u,3758096363u,4294967287u,1000000006u,1000000008u,2147483646u,2000000014u,3000000021u,4292870399u,4293001441u,4292739361u,3215031751u})v.push_back(m);std::mt19937 rng(419650);for(int i=0;i<20;++i)v.push_back(rng());return v;}
int main(){const auto mods=moduli();
 for(auto m:mods){std::array<std::array<std::uint32_t,130>,130> row{};row[0][0]=1%m;for(std::size_t n=1;n<row.size();++n){row[n][0]=1%m;for(std::size_t k=1;k<=n;++k)row[n][k]=static_cast<std::uint32_t>((static_cast<std::uint64_t>(row[n-1][k-1])+row[n-1][k])%m);}
  for(std::uint32_t table:{0u,1u,2u,7u,31u,96u,10000u}){BinomialOptions opt;opt.table_limit=table;opt.max_multiplicative_steps=4096;BinomialMod b(m,0,opt);REQUIRE(b.mod()==m);REQUIRE(b.is_prime_modulus()==prime(m));
   for(int n=0;n<=72;++n)for(int k=-1;k<=n+1;++k){auto want=k<0||k>n?0u:row[n][k];REQUIRE(b.COM(n,k)==want);++queries;if(b.is_prime_modulus()){REQUIRE(b.Lucas(n,k)==want);++queries;}}
   for(int n=0;n<=24;++n)for(int k=0;k<=24;++k){const auto want=k==0?1%m:(n==0?0:row[n+k-1][k]);REQUIRE(b.H(n,k)==want);++queries;}
   REQUIRE(b.COM(-1,0)==0);REQUIRE(b.COM(7,-1)==0);REQUIRE(b.H(-1,0)==0);REQUIRE(b.H(2,-1)==0);
   const auto before=b.COM(60,17);b.prepare(std::min<std::uint32_t>(table,96));REQUIRE(b.COM(60,17)==before);b.prepare(0);REQUIRE(b.COM(60,17)==before);
   bool exceeds=false;try{b.prepare(static_cast<std::int64_t>(table)+1);}catch(const std::length_error&){exceeds=true;++limit_failures;}
   bool should=false;std::uint32_t rem=m;for(std::uint32_t p=2;static_cast<std::uint64_t>(p)*p<=rem;++p)if(rem%p==0){std::uint64_t pp=1;while(rem%p==0){rem/=p;pp*=p;}if(std::min<std::uint64_t>(table+1ull,pp-1)>table)should=true;}if(rem>1&&std::min<std::uint64_t>(table+1ull,rem-1)>table)should=true;
   REQUIRE(exceeds==should);REQUIRE(b.COM(60,17)==before);
  }
  BinomialOptions large_options;large_options.table_limit=31;large_options.max_multiplicative_steps=4096;BinomialMod large(m,0,large_options);
  const std::int64_t top=std::numeric_limits<std::int64_t>::max();
  for(std::int64_t n:{top,top-1,top-31,top-1024,std::int64_t{4294967295},std::int64_t{4294967296},std::int64_t{1000000000000}})for(std::int64_t k:{0,1,2,3,5,10,17,31,63}){REQUIRE(large.COM(n,k)==exact(n,k,m));++queries;REQUIRE(large.COM(n,n-k)==exact(n,k,m));++queries;if(large.is_prime_modulus()){REQUIRE(large.Lucas(n,k)==exact(n,k,m));++queries;}}
  for(std::int64_t n:{0,1,2,3,7,10,31,63,100})for(std::int64_t k:{0,1,2,5,10,17}){const auto want=k==0?1%m:(n==0?0:exact(n+k-1,k,m));REQUIRE(large.H(n,k)==want);++queries;}
  REQUIRE(large.H(top,1)==static_cast<std::uint64_t>(top)%m);REQUIRE(large.H(1,top)==1%m);REQUIRE(large.H(0,top)==0);
  bool overflow=false;try{(void)large.H(top,2);}catch(const std::overflow_error&){overflow=true;}REQUIRE(overflow==(m!=1));
#ifndef SKIP_RUNTIME_CHECKS
  if(!large.is_prime_modulus()){bool domain=false;try{(void)large.Lucas(10,3);}catch(const std::domain_error&){domain=true;}REQUIRE(domain);}
#endif
 }
 // Product budgets remain a limit even with input checks disabled.
 for(auto [m,n,k,need]:std::array<std::array<std::uint32_t,4>,4>{{{{101,10,2,2}},{{8,8,2,2}},{{72,10,2,2}},{{36,14,2,4}}}}){for(std::uint32_t limit=0;limit<=need+1;++limit){BinomialOptions opt;opt.table_limit=0;opt.max_multiplicative_steps=limit;BinomialMod b(m,0,opt);bool threw=false;std::uint32_t got=0;try{got=b.COM(n,k);}catch(const std::length_error&){threw=true;++limit_failures;}REQUIRE(threw==(limit<need));if(!threw)REQUIRE(got==exact(n,k,m));REQUIRE(b.COM(n,0)==1%m);REQUIRE(b.COM(n,1)==n%m);}}
 for(auto m:{8u,16u,27u,72u,360u}){BinomialOptions o;o.table_limit=512;o.max_multiplicative_steps=0;BinomialMod b(m,0,o);for(int n=0;n<=60;++n)for(int k=0;k<=n;++k){REQUIRE(b.COM(n,k)==exact(n,k,m));++queries;}b.prepare(512);REQUIRE(b.COM(60,29)==exact(60,29,m));}
 {BinomialOptions o;o.table_limit=0;o.max_multiplicative_steps=0;BinomialMod a(8,0,o);REQUIRE(a.COM(8,3)==0);BinomialMod b(101,0,o);REQUIRE(b.COM(101,2)==0);}
#ifndef SKIP_RUNTIME_CHECKS
 for(std::int64_t invalid:{std::int64_t{-1},std::int64_t{0},std::int64_t{4294967296},std::numeric_limits<std::int64_t>::max()}){bool threw=false;try{BinomialMod b(invalid,0);}catch(const std::invalid_argument&){threw=true;}REQUIRE(threw);}
 {BinomialMod b(8,0);bool threw=false;try{b.prepare(-1);}catch(const std::invalid_argument&){threw=true;}REQUIRE(threw);}
#endif
 std::cout<<"{\"moduli\":"<<mods.size()<<",\"checks\":"<<checks<<",\"queries\":"<<queries<<",\"limit_failures\":"<<limit_failures<<"}\n";
}
