// SPDX-License-Identifier: CC0-1.0
#include <array>
#include <iostream>
#include <stdexcept>

int main() {
    const auto require=[](bool ok) {if(!ok)throw std::runtime_error("primality reference mismatch");};
    for(std::uint64_t bound:{0,1,2,3,63,64,65,127,128,129,10000}) {
        PrimeSieve sieve(bound);std::vector<std::uint64_t> expected;
        for(std::uint64_t n=0;n<=bound;++n) {
            bool prime=n>=2;for(std::uint64_t d=2;d<=n/d;++d)if(n%d==0){prime=false;break;}
            require(sieve.is_prime(n)==prime);require(is_prime(n)==prime);if(prime)expected.push_back(n);
        }
        require(sieve.prime_count()==expected.size() && sieve.primes()==expected && eratosthenes(bound)==expected);
        std::vector<std::uint64_t> appended{123};eratosthenes(bound,appended);expected.insert(expected.begin(),123);require(appended==expected);
        const PrimeSieve copied=sieve;PrimeSieve moved=std::move(sieve);require(copied.primes()==moved.primes());require(sieve.limit()==0);
    }
    for(auto n:std::array<std::uint64_t,6>{2,998244353,4294967291ULL,2305843009213693951ULL,18446744073709551557ULL,18446744073709551533ULL}) require(is_prime(n));
    for(auto n:std::array<std::uint64_t,8>{0,1,341,2047,3215031751ULL,341550071728321ULL,3825123056546413051ULL,18446744073709551615ULL}) require(!is_prime(n));
    require(!is_prime(-1));require(MillerRabin(2047,{2}));require(!MillerRabin(2047,{2,3}));
    std::cout<<"OK\n";
}
