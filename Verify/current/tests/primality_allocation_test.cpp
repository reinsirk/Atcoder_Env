// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PRIMALITY_REQUIRE_EXTRA
#define SIRKIT_PRIMALITY_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_PRIMALITY_SOURCE
#define SIRKIT_PRIMALITY_SOURCE "../../../Math/prime_judge(Miller-Rabin).cpp"
#endif
#include SIRKIT_PRIMALITY_SOURCE
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
static bool armed=false;static std::size_t calls=0,checks=0;
[[gnu::noinline]]void*operator new(std::size_t n){if(armed){++calls;throw std::bad_alloc();}if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
[[gnu::noinline]]void*operator new[](std::size_t n){return::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
static void require(bool ok){++checks;if(!ok)std::abort();}
int main(){
 static_assert(noexcept(is_prime(std::uint64_t{2})));static_assert(noexcept(MillerRabin(2,{2,3})));
 const std::array<std::uint64_t,13> primes{2,3,13,61,65537,998244353,1000000007,4294967291ULL,4759123129ULL,2305843009213693951ULL,9223372036854775783ULL,18446744073709551557ULL,18446744073709551533ULL};
 const std::array<std::uint64_t,13> composites{0,1,4,341,561,2047,3215031751ULL,4759123141ULL,341550071728321ULL,3825123056546413051ULL,4611686018427387903ULL,9223372036854775807ULL,18446744073709551615ULL};
 armed=true;
 for(int rep=0;rep<16;++rep){for(auto n:primes)require(is_prime(n));for(auto n:composites)require(!is_prime(n));require(!is_prime(std::int64_t{-1}));require(MillerRabin(2047,{2}));require(!MillerRabin(2047,{2,3}));require(MillerRabin(std::uint64_t{18446744073709551557ULL},std::array<std::int64_t,3>{-2,-7,-61}));}
 #ifdef SIRKIT_PRIMALITY_REQUIRE_EXTRA
 require(next_prime(std::uint64_t{18446744073709551556ULL})==18446744073709551557ULL);
 require(!next_prime(std::numeric_limits<std::uint64_t>::max()));
 require(prev_prime(std::numeric_limits<std::uint64_t>::max())==18446744073709551557ULL);
 require(next_prime(-5)==2);require(!prev_prime(-5));
 #endif
 armed=false;require(calls==0);std::printf("PASS allocation_calls=%zu checks=%zu\n",calls,checks);
}
