// SPDX-License-Identifier: CC0-1.0
#include <atcoder/modint>
#include <iostream>
#include <stdexcept>

int main() {
    const auto require=[](bool ok) { if(!ok)throw std::runtime_error("binomial reference mismatch"); };
    using Mint=atcoder::dynamic_modint<1>;
    for(int mod=1;mod<=160;++mod) {
        Mint::set_mod(mod);BinomialMod integer(mod,8);ModIntBinomial<Mint> wrapped(8);
        std::vector row(101,std::vector<std::uint32_t>(101));row[0][0]=1%mod;
        for(int n=1;n<=100;++n) {
            row[n][0]=1%mod;
            for(int k=1;k<=n;++k)row[n][k]=(row[n-1][k-1]+row[n-1][k])%mod;
        }
        for(int n=0;n<=70;++n)for(int k=-1;k<=n+1;++k) {
            const auto want=k<0||k>n?0:row[n][k];require(integer.COM(n,k)==want);require(wrapped.COM(n,k).val()==static_cast<int>(want));
        }
        integer.prepare(80);wrapped.prepare(80);
        for(int n=0;n<=35;++n)for(int k=0;k<=35;++k) {
            const auto want=k==0?1%mod:(n==0?0:row[n+k-1][k]);
            require(integer.H(n,k)==static_cast<unsigned>(want));require(wrapped.H(n,k).val()==static_cast<int>(want));
        }
        if(integer.is_prime_modulus()) {
            PrimeBinomial<Mint> prime(8);
            for(int n=0;n<=100;++n)for(int k=0;k<=n;++k) {
                require(prime.COM(n,k).val()==static_cast<int>(row[n][k]));
                require(prime.Lucas(n,k).val()==static_cast<int>(row[n][k]));
                require(integer.Lucas(n,k)==row[n][k]);require(wrapped.Lucas(n,k).val()==static_cast<int>(row[n][k]));
            }
            prime.prepare(120);require(prime.H(10,15).val()==static_cast<int>(row[24][15]));
        }
    }
    std::cout<<"OK\n";
}
