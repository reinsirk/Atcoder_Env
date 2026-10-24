// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PRIMALITY_SOURCE
#define SIRKIT_PRIMALITY_SOURCE "../../../Math/prime_judge(Miller-Rabin).cpp"
#endif
#ifndef SIRKIT_PRIMALITY_COMBINATION_SOURCE
#define SIRKIT_PRIMALITY_COMBINATION_SOURCE "../../../Math/Combination/Prime_Binomial.cpp"
#endif
#ifndef SIRKIT_PRIMALITY_SIEVE_SOURCE
#define SIRKIT_PRIMALITY_SIEVE_SOURCE "../../../Math/Eratosthenes.cpp"
#endif
#ifdef SIRKIT_PRIMALITY_COMBINATION_FIRST
#include SIRKIT_PRIMALITY_COMBINATION_SOURCE
#include SIRKIT_PRIMALITY_SOURCE
#else
#include SIRKIT_PRIMALITY_SOURCE
#include SIRKIT_PRIMALITY_COMBINATION_SOURCE
#endif
#include SIRKIT_PRIMALITY_SOURCE
#include SIRKIT_PRIMALITY_SIEVE_SOURCE
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
static std::size_t checks=0;
static void require(bool ok){++checks;if(!ok)std::abort();}
static bool trial(std::uint32_t n){if(n<2)return false;if(!(n&1))return n==2;for(std::uint32_t d=3;d<=n/d;d+=2)if(n%d==0)return false;return true;}
int main(){PrimeSieve sieve(10000);for(std::uint32_t n=0;n<=10000;++n){require(is_prime(n)==trial(n));require(is_prime(n)==sieve.is_prime(n));}std::mt19937 random(771022);for(unsigned k=0;k<2000;++k){const auto n=static_cast<std::uint32_t>(random());require(reinsirk::detail::is_prime_u32(n)==trial(n));require(is_prime(n)==trial(n));}require(is_prime(std::uint64_t{18446744073709551557ULL}));std::printf("PASS guard checks=%zu\n",checks);}
