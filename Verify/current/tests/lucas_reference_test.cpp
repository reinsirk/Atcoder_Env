// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#include <cassert>
#include <iostream>
#include <random>
#include <boost/multiprecision/cpp_int.hpp>

template<int Fixed>struct TestMint {
    inline static int dynamic_modulus=3;
    static constexpr int mod()requires(Fixed!=0){return Fixed;}
    static int mod()requires(Fixed==0){return dynamic_modulus;}
    static void set_mod(int value)requires(Fixed==0){dynamic_modulus=value;}
    int value=0;
    TestMint()=default;
    TestMint(std::int64_t x){x%=mod();if(x<0)x+=mod();value=static_cast<int>(x);}
    static TestMint raw(int v){TestMint r;r.value=v;return r;}
    int val()const{return value;}
    TestMint& operator+=(TestMint other){value+=other.value;if(value>=mod())value-=mod();return *this;}
    friend TestMint operator+(TestMint a,TestMint b){return a+=b;}
    TestMint operator-()const{return raw(value?mod()-value:0);}
    TestMint& operator*=(TestMint other){value=static_cast<int>(static_cast<std::uint64_t>(value)*other.value%mod());return *this;}
    friend TestMint operator*(TestMint a,TestMint b){return a*=b;}
    TestMint inv()const{assert(value!=0);TestMint a=*this,result=1;for(int e=mod()-2;e;e>>=1,a*=a)if(e&1)result*=a;return result;}
};
#ifndef LUCAS_HEADER
#define LUCAS_HEADER "../../../Math/Combination/Prime_Binomial.cpp"
#endif
#include LUCAS_HEADER
#include LUCAS_HEADER

std::size_t checks=0;
int reference_lucas(std::int64_t n,std::int64_t k,int p){
    if(n<0||k<0||k>n)return 0;
    int result=1;
    while(k){int a=static_cast<int>(n%p),b=static_cast<int>(k%p);if(b>a)return 0;std::uint64_t value=1;for(int i=1;i<=b;++i)value=value*(a-b+i)/i;result=result*static_cast<int>(value%p)%p;n/=p;k/=p;}
    return result;
}
int reference_big(std::int64_t n,int k,int p){
    using boost::multiprecision::cpp_int;cpp_int value=1;
    for(int i=1;i<=k;++i)value=value*(n-k+i)/i;
    return static_cast<int>(value%p);
}
template<class Mint>void small(int limit){
    const int p=Mint::mod();std::vector<std::vector<int>> triangle(limit+1,std::vector<int>(limit+1));triangle[0][0]=1;
    for(int n=1;n<=limit;++n){triangle[n][0]=1;for(int k=1;k<=n;++k)triangle[n][k]=(triangle[n-1][k-1]+triangle[n-1][k])%p;}
    for(int bound:{0,1,2,3,7}){
        PrimeBinomial<Mint> comb(bound,10000);
        for(int n=0;n<=limit;++n)for(int k=-1;k<=n+1;++k){int want=k<0||k>n?0:triangle[n][k];assert(comb.COM(n,k).val()==want);assert(comb.Lucas(n,k).val()==want);++checks;}
        comb.prepare(limit);for(int n=0;n<=limit;++n)for(int k=0;k<=n;++k){assert(comb.COM(n,k).val()==triangle[n][k]);++checks;}
        auto copied=comb;auto moved=std::move(copied);assert(moved.COM(limit,limit/2).val()==triangle[limit][limit/2]);++checks;copied.prepare(limit);assert(copied.COM(limit,limit/2).val()==triangle[limit][limit/2]);++checks;
        for(int n=0;n<=limit/2;++n)for(int k=0;k<=limit/2;++k){int want=k==0?1:(n==0?0:triangle[n+k-1][k]);assert(comb.H(n,k).val()==want);++checks;}
    }
}
template<class Mint>void full_width(){
    constexpr auto top=std::numeric_limits<std::int64_t>::max();PrimeBinomial<Mint> comb(2,0);std::mt19937_64 rng(130991);
    auto check=[&](std::int64_t n,std::int64_t k){assert(comb.COM(n,k).val()==reference_lucas(n,k,3));assert(comb.Lucas(n,k).val()==reference_lucas(n,k,3));++checks;};
    for(std::int64_t power=1;power<=top/3;power*=3){
        for(std::int64_t n:{power-1,power,power+1})for(std::int64_t k:std::initializer_list<std::int64_t>{0,1,2,n/2,n-1,n})check(n,k);
    }
    for(std::int64_t n=top-128;n<top;++n)for(int k:{0,1,2,3,7,17,31,63}){check(n,k);assert(comb.COM(n,k).val()==reference_big(n,k,3));++checks;}
    for(int i=0;i<100000;++i){auto n=static_cast<std::int64_t>(rng()>>1);auto k=static_cast<std::int64_t>(rng()%(static_cast<std::uint64_t>(n)+1));check(n,k);check(n,n-k);}
    check(top,top/2);assert(comb.H(0,0).val()==1);assert(comb.H(0,1).val()==0);assert(comb.COM(-1,0).val()==0);
    bool overflow=false;try{(void)comb.H(top,2);}catch(const std::overflow_error&){overflow=true;}assert(overflow);++checks;
}
int main(){
    small<TestMint<2>>(100);small<TestMint<3>>(600);small<TestMint<5>>(100);small<TestMint<7>>(100);small<TestMint<97>>(100);full_width<TestMint<3>>();
    using D=TestMint<0>;for(int p:{2,3,5,7,97}){D::set_mod(p);small<D>(p==3?300:100);}D::set_mod(3);full_width<D>();
    for(int bound:{0,1,2})for(std::uint64_t budget:{0,1,2,100}){
        PrimeBinomial<TestMint<3>> comb(bound,budget);
        for(std::int64_t n:{0,1,2,3,4,8,9,80,81,242,243,10000})for(std::int64_t k=0;k<=std::min<std::int64_t>(n,30);++k){assert(comb.COM(n,k).val()==reference_lucas(n,k,3));++checks;}
    }
    {
        PrimeBinomial<TestMint<97>> limited(0,0);bool error=false;try{(void)limited.COM(80,31);}catch(const std::length_error&){error=true;}assert(error);
        assert(limited.COM(97*97+2*97+1,3*97+2).val()==0);++checks;
    }
#ifndef SKIP_RUNTIME_CHECKS
    for(int p:{1,4,9,15}){D::set_mod(p);bool error=false;try{PrimeBinomial<D> invalid(0);}catch(const std::domain_error&){error=true;}assert(error);++checks;}
    D::set_mod(3);PrimeBinomial<D> old(2);D::set_mod(5);bool changed=false;try{(void)old.COM(10,2);}catch(const std::logic_error&){changed=true;}assert(changed);++checks;
    bool negative=false;try{PrimeBinomial<TestMint<3>> invalid(-1);}catch(const std::invalid_argument&){negative=true;}assert(negative);++checks;
#endif
    std::cout<<"PASS Pascal, cpp_int, full-width Lucas and budget checks="<<checks<<'\n';
}
