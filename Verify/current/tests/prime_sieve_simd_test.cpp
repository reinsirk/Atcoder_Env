// SPDX-License-Identifier: CC0-1.0
#ifndef SIEVE_SOURCE
#define SIEVE_SOURCE "../../../Math/Eratosthenes.cpp"
#endif
#include SIEVE_SOURCE
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
namespace {
std::size_t checks=0;
void require(bool test){++checks;if(!test)throw std::runtime_error("PrimeSieve independent comparison failed");}
std::vector<std::uint64_t> reference(std::size_t n){
 std::vector<bool> prime(n+1,true);prime[0]=false;if(n)prime[1]=false;
 for(std::size_t d=2;d<=n/d;++d)if(prime[d])for(std::size_t x=d*d;x<=n;x+=d)prime[x]=false;
 std::vector<std::uint64_t> result;for(std::size_t x=2;x<=n;++x)if(prime[x])result.push_back(x);return result;
}
void sieve_cases(){
 auto all=reference(1000000);std::vector<std::uint64_t> limits;
 for(std::uint64_t n=0;n<=1024;++n)limits.push_back(n);
 for(std::uint64_t n:{3967,3968,3969,3970,8191,8192,8193,16255,16256,16257,16258,16383,16384,16385,32767,32768,32769,100000,1000000})limits.push_back(n);
 for(auto n:limits){PrimeSieve s(n);const auto end=std::upper_bound(all.begin(),all.end(),n);const std::vector<std::uint64_t> expected(all.begin(),end);
  require(s.limit()==n);require(s.prime_count()==expected.size());require(s.primes()==expected);std::vector<std::uint64_t> appended{99};s.append_to(appended);require(appended.size()==expected.size()+1);require(std::equal(expected.begin(),expected.end(),appended.begin()+1));
  for(std::uint64_t x=0;x<=std::min(n,std::uint64_t{1024});++x)require(s.is_prime(x)==std::binary_search(expected.begin(),expected.end(),x));
  PrimeSieve copied(s);require(copied.prime_count()==s.prime_count());PrimeSieve moved(std::move(copied));require(copied.limit()==0&&copied.prime_count()==0);require(moved.primes()==expected);copied=std::move(moved);require(moved.limit()==0&&moved.prime_count()==0);require(copied.primes()==expected);
#ifndef SKIP_RUNTIME_CHECKS
  bool thrown=false;try{(void)s.is_prime(n+1);}catch(const std::out_of_range&){thrown=true;}require(thrown);
#endif
 }
}
void word_cases(){
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
 if(!__builtin_cpu_supports("avx2"))return;
 std::mt19937_64 gen(0x551EC0DE);std::vector<std::uint64_t> data(65536+8);
 const std::array<std::size_t,24> lengths{0,1,2,3,4,5,7,8,15,16,31,32,63,64,127,128,129,255,256,511,512,4095,4096,65536};
 for(unsigned pattern=0;pattern<5;++pattern){
  for(std::size_t i=0;i<data.size();++i)data[i]=pattern==0?0:pattern==1?~std::uint64_t{0}:pattern==2?UINT64_C(0xaaaaaaaaaaaaaaaa):pattern==3?UINT64_C(0x0101010101010101)*(i%256):gen();
  for(std::size_t offset=0;offset<4;++offset)for(auto n:lengths){std::uint64_t expected=0;for(std::size_t i=0;i<n;++i)expected+=std::popcount(data[i+offset]);require(reinsirk::detail::popcount_words_avx2(data.data()+offset,n)==expected);}
 }
#endif
}
}
int main(){sieve_cases();word_cases();std::cout<<"prime_sieve_simd_test: "<<checks<<" checks passed\n";}
