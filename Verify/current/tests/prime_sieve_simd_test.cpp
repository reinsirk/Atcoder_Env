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
#include <limits>
#include <random>
#include <source_location>
#include <stdexcept>
#include <utility>
#include <vector>
#ifdef SIRKIT_TEST_REQUIRE_FOR_EACH_PRIME
static_assert(requires(const PrimeSieve& sieve) {
 sieve.for_each_prime([](std::uint64_t) {});
});
#endif
namespace {
std::size_t checks=0;
void require(bool test, std::source_location where = std::source_location::current()) {
 ++checks;
 if (!test) {
  std::cerr << "PrimeSieve check " << checks << ", line " << where.line() << '\n';
  throw std::runtime_error("PrimeSieve independent comparison failed");
 }
}
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
void extended_sieve_cases() {
 const auto all = reference(4'200'000);
 std::mt19937_64 random(0xE2A7057EULL);
 std::vector<std::uint64_t> limits;
 for (std::uint64_t boundary : {64,128,256,512,3969,8192,16384,32768,
      65536,131072,262144,524288,1048576,2097152,4194304}) {
  for (int delta = -3; delta <= 3; ++delta)
   limits.push_back(static_cast<std::uint64_t>(static_cast<std::int64_t>(boundary) + delta));
 }
 for (int i = 0; i < 48; ++i) limits.push_back(random() % 2'000'001);
 for (auto n : limits) {
  const auto end = std::upper_bound(all.begin(), all.end(), n);
  const std::vector<std::uint64_t> expected(all.begin(), end);
  PrimeSieve sieve(n);
  require(sieve.limit() == n);
  require(sieve.prime_count() == expected.size());
  require(sieve.primes() == expected);
  std::vector<std::uint64_t> appended{0, 101, 101};
  sieve.append_to(appended);
  require(appended.size() == expected.size() + 3);
  require(appended[0] == 0 && appended[1] == 101 && appended[2] == 101);
  require(std::equal(expected.begin(), expected.end(), appended.begin() + 3));
  for (unsigned i = 0; i < 512; ++i) {
   const auto x = random() % (n + 1);
   require(sieve.is_prime(x) == std::binary_search(expected.begin(), expected.end(), x));
  }
  for (auto x = n > 128 ? n - 128 : 0; x <= n; ++x)
   require(sieve.is_prime(x) == std::binary_search(expected.begin(), expected.end(), x));
  if (n <= 32768) {
   require(eratosthenes(n) == expected);
   std::vector<std::uint64_t> wrapped{123};
   eratosthenes(n, wrapped);
   require(wrapped.size() == expected.size() + 1 && wrapped.front() == 123);
   require(std::equal(expected.begin(), expected.end(), wrapped.begin() + 1));
  }
 }
 // コピー代入の容量再利用と自己代入、空になった移動元の再利用。
 for (std::uint64_t n : {0,1,2,3,127,128,129,65535,65536,65537}) {
  PrimeSieve source(n), destination(100000);
  const auto expected = source.primes();
  destination = source;
  require(destination.limit() == n && destination.primes() == expected);
  auto* alias = &destination;
  destination = *alias;
  require(destination.primes() == expected);
  destination = std::move(*alias);
  require(destination.limit() == n && destination.primes() == expected);
  PrimeSieve moved(std::move(destination));
  require(destination.limit() == 0 && destination.prime_count() == 0);
  require(destination.primes().empty() && !destination.is_prime(0));
#ifndef SKIP_RUNTIME_CHECKS
  bool thrown = false;
  try { (void)destination.is_prime(1); } catch (const std::out_of_range&) { thrown = true; }
  require(thrown);
#endif
  destination = PrimeSieve(17);
  require(destination.prime_count() == 7 && destination.is_prime(17));
  require(moved.primes() == expected);
 }
 for (auto [n, count] : std::array<std::pair<std::uint64_t, std::uint64_t>, 2>{
      {{10'000'000, 664579}, {100'000'000, 5761455}}}) {
  PrimeSieve sieve(n);
  require(sieve.prime_count() == count);
  require(!sieve.is_prime(n));
 }
}
template<class Sieve>
void callback_cases() {
 if constexpr (requires(const Sieve& sieve) {
     sieve.for_each_prime([](std::uint64_t) {});
 }) {
  struct Counter {
   std::uint64_t previous = 0;
   std::size_t count = 0;
   Counter() = default;
   Counter(const Counter&) = delete;
   void operator()(std::uint64_t value) {
    require(previous < value);
    previous = value;
    ++count;
   }
  };
  struct CallbackFailure {};
  for (std::uint64_t n : {0,1,2,3,7,127,128,129,65535,65536,65537}) {
   const Sieve sieve(n);
   const auto expected = reference(static_cast<std::size_t>(n));
   std::vector<std::uint64_t> collected{99};
   sieve.for_each_prime([&](std::uint64_t value) { collected.push_back(value); });
   require(collected.size() == expected.size() + 1 && collected.front() == 99);
   require(std::equal(expected.begin(), expected.end(), collected.begin() + 1));
   Counter counter;
   sieve.for_each_prime(counter);
   require(counter.count == expected.size());
   if (!expected.empty()) require(counter.previous == expected.back());
   if (expected.size() >= 3) {
    std::size_t calls = 0;
    bool thrown = false;
    try {
     sieve.for_each_prime([&](std::uint64_t) {
      if (++calls == 3) throw CallbackFailure{};
     });
    } catch (const CallbackFailure&) { thrown = true; }
    require(thrown && calls == 3);
    require(sieve.limit() == n && sieve.prime_count() == expected.size());
    require(sieve.primes() == expected);
   }
  }
 }
}
}
int main(){sieve_cases();word_cases();extended_sieve_cases();callback_cases<PrimeSieve>();std::cout<<"prime_sieve_simd_test: "<<checks<<" checks passed\n";}
