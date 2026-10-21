// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
int main() {
    std::uint64_t n,a,b;verify_in>>n>>a>>b;
    PrimeSieve sieve(n);const auto primes=sieve.primes();
    const auto size=primes.size();const auto x=size>b?(size-1-b)/a+1:0;
    verify_out<<sieve.prime_count()<<' '<<x<<'\n';
    for(std::uint64_t i=b;i<size;i+=a) verify_out<<primes[i]<<' ';
    verify_out<<'\n';
}
