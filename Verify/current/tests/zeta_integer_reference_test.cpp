// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_ZETA_INTEGER_EXTRA
#define SIRKIT_ZETA_INTEGER_EXTRA
#endif
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta_Mebius.cpp"
#endif
#include SOURCE
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>
namespace {
std::uint64_t checks=0;
void check(bool v){++checks;if(!v)std::abort();}
struct Mod {
 std::uint32_t v=0;Mod()=default;Mod(std::int64_t x){x%=998244353;if(x<0)x+=998244353;v=static_cast<std::uint32_t>(x);}
 Mod&operator+=(Mod b){v+=b.v;if(v>=998244353)v-=998244353;return *this;}Mod&operator-=(Mod b){if(v<b.v)v+=998244353;v-=b.v;return *this;}Mod&operator*=(Mod b){v=std::uint64_t(v)*b.v%998244353;return *this;}
 friend Mod operator+(Mod a,Mod b){return a+=b;}friend Mod operator*(Mod a,Mod b){return a*=b;}friend bool operator==(Mod,Mod)=default;
 Mod pow(std::int64_t k)const{Mod a=*this,r(1);for(;k;k>>=1,a*=a)if(k&1)r*=a;return r;}
};
int mobius(std::size_t n){int ans=1;for(std::size_t p=2;p<=n/p;++p)if(n%p==0){n/=p;ans=-ans;if(n%p==0)return 0;while(n%p==0)n/=p;}if(n>1)ans=-ans;return ans;}
template<class T>T product(T a,T b){if constexpr(std::is_unsigned_v<T>)return static_cast<T>(std::uint64_t(a)*std::uint64_t(b));else return a*b;}
template<class T>void suite(){std::mt19937_64 rng(61022);IntegerSieve sieve(300);
 for(const std::size_t n: {0U,1U,2U,3U,5U,17U,64U,127U,128U,129U,257U})for(int rep=0;rep<4;++rep){
  std::vector<T>a(n),b(n);for(auto&v:a){if constexpr(std::is_same_v<T,Mod>)v=Mod(rng()%998244353);else if constexpr(std::is_unsigned_v<T>){
#ifndef SIRKIT_ZETA_INTEGER_EXTRA
   if constexpr(sizeof(T)==2)v=static_cast<T>(rng()%8);else
#endif
   v=static_cast<T>(rng());}else v=static_cast<T>(static_cast<int>(rng()%9)-4);}
  for(auto&v:b){if constexpr(std::is_same_v<T,Mod>)v=Mod(rng()%998244353);else if constexpr(std::is_unsigned_v<T>){
#ifndef SIRKIT_ZETA_INTEGER_EXTRA
   if constexpr(sizeof(T)==2)v=static_cast<T>(rng()%8);else
#endif
   v=static_cast<T>(rng());}else v=static_cast<T>(static_cast<int>(rng()%9)-4);}
  if(rep%2==0&&n){a[0]=T{};b[0]=T{};}
  for(bool multiples:{false,true})for(bool inverse:{false,true}){
   auto expected=a,actual=a;for(std::size_t i=1;i<n;++i){T sum{};for(std::size_t j=1;j<n;++j){const bool related=multiples?j%i==0:i%j==0;if(!related)continue;const auto q=multiples?j/i:i/j;const auto coefficient=inverse?mobius(q):1;if(coefficient==1)sum+=a[j];else if(coefficient==-1)sum-=a[j];}expected[i]=sum;}
   if(multiples){if(inverse)multiple_mobius_transform(actual,sieve);else multiple_zeta_transform(actual,sieve);}else{if(inverse)divisor_mobius_transform(actual,sieve);else divisor_zeta_transform(actual,sieve);}check(actual==expected);
   actual=a;if(multiples){if(inverse)multiple_mobius_transform(actual);else multiple_zeta_transform(actual);}else{if(inverse)divisor_mobius_transform(actual);else divisor_zeta_transform(actual);}check(actual==expected);
  }
  for(bool gcd:{false,true}){std::vector<T>expected(n);for(std::size_t i=0;i<n;++i)for(std::size_t j=0;j<n;++j){const auto k=gcd?std::gcd(i,j):std::lcm(i,j);if(k<n)expected[k]+=product(a[i],b[j]);}auto result=gcd?gcd_convolution(a,b,sieve):lcm_convolution(a,b,sieve);check(result==expected);check((gcd?gcd_convolution(a,b):lcm_convolution(a,b))==expected);
#ifdef SIRKIT_ZETA_INTEGER_EXTRA
   std::vector<T>self(n);for(std::size_t i=0;i<n;++i)for(std::size_t j=0;j<n;++j){const auto k=gcd?std::gcd(i,j):std::lcm(i,j);if(k<n)self[k]+=product(a[i],a[j]);}check((gcd?gcd_square(a,sieve):lcm_square(a,sieve))==self);check((gcd?gcd_square(a):lcm_square(a))==self);
#endif
  }
 }
}
void sieve_suite(){for(int n=0;n<1000;++n){IntegerSieve sieve(n);std::vector<int>primes;for(int x=0;x<=n;++x){int least=0;if(x>=2){least=x;for(int p=2;p<=x/p;++p)if(x%p==0){least=p;break;}}check(sieve.least_prime_factor[x]==least);check(sieve.is_prime(x)==(x>=2&&least==x));if(x>=2&&least==x)primes.push_back(x);}check(sieve.prime_list==primes);auto copy=sieve;IntegerSieve moved(std::move(copy));check(copy.max_value()==-1);check(moved.least_prime_factor==sieve.least_prime_factor);copy=sieve;check(copy.prime_list==sieve.prime_list);copy=std::move(moved);check(moved.max_value()==-1);check(copy.least_prime_factor==sieve.least_prime_factor);}
 IntegerSieve sieve(1000);auto mu=sieve.mobius_table();auto phi=sieve.totient_table();for(int i=1;i<=1000;++i){check(mu[i]==mobius(i));int expected=0;for(int j=1;j<=i;++j)expected+=std::gcd(i,j)==1;check(phi[i]==expected);auto divisors=sieve.sorted_divisors(i);std::vector<int>ds;for(int j=1;j<=i;++j)if(i%j==0)ds.push_back(j);check(divisors==ds);auto facts=sieve.prime_factorization(i);int v=1;for(auto[p,e]:facts)for(int k=0;k<e;++k)v*=p;check(v==i);auto totients=sieve.divisor_totients(i);for(auto[d,totient]:totients)check(totient==phi[d]);}
 for(int k=0;k<8;++k){auto powers=sieve.power_table<Mod>(k,1000);for(int i=0;i<=1000;++i)check(powers[i]==Mod(i).pow(k));}
#ifndef SKIP_RUNTIME_CHECKS
 bool caught=false;try{IntegerSieve bad(-1);}catch(const std::exception&){caught=true;}check(caught);std::vector<int>a(3),b(4);caught=false;try{(void)gcd_convolution(a,b,sieve);}catch(const std::invalid_argument&){caught=true;}check(caught);caught=false;try{multiple_zeta_transform(a,IntegerSieve(1));}catch(const std::out_of_range&){caught=true;}check(caught);
#endif
}
}
int main(){suite<std::uint8_t>();suite<std::uint16_t>();suite<std::uint32_t>();suite<std::uint64_t>();suite<std::int64_t>();suite<Mod>();sieve_suite();std::cout<<"PASS checks="<<checks<<'\n';}
