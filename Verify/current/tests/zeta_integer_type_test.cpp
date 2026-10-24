// SPDX-License-Identifier: CC0-1.0
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta_Mebius.cpp"
#endif
#include SOURCE
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <vector>
namespace {std::uint64_t checks=0;void check(bool x){++checks;if(!x)std::abort();}
template<class T>void signed_suite(){IntegerSieve sieve(300);for(std::size_t n:{1U,2U,3U,32U,129U,257U})for(std::size_t k=0;k<20;++k){std::vector<T>a(n),b(n);const auto i=(k*31)%n,j=(k*57+3)%n;a[i]=k%2?T{-1}:T{1};b[j]=k%3?T{1}:T{-1};for(bool gcd:{false,true}){std::vector<T>expected(n);const auto index=gcd?std::gcd(i,j):std::lcm(i,j);if(index<n)expected[index]=static_cast<T>(a[i]*b[j]);check((gcd?gcd_convolution(a,b,sieve):lcm_convolution(a,b,sieve))==expected);std::fill(expected.begin(),expected.end(),T{});const auto self=gcd?i:std::lcm(i,i);if(self<n)expected[self]=T{1};check((gcd?gcd_square(a,sieve):lcm_square(a,sieve))==expected);}auto f=a;multiple_zeta_transform(f,sieve);multiple_mobius_transform(f,sieve);check(f==a);f=a;divisor_zeta_transform(f,sieve);divisor_mobius_transform(f,sieve);check(f==a);}
 for(T value:{std::numeric_limits<T>::min(),std::numeric_limits<T>::max()}){std::vector<T>a{value},b{1};check(gcd_convolution(a,b)==a);check(lcm_convolution(a,b)==a);}}
struct Matrix{std::array<std::int64_t,4>v{};Matrix&operator+=(const Matrix&b){for(int i=0;i<4;++i)v[i]+=b.v[i];return *this;}Matrix&operator-=(const Matrix&b){for(int i=0;i<4;++i)v[i]-=b.v[i];return *this;}Matrix&operator*=(const Matrix&b){Matrix out;for(int r=0;r<2;++r)for(int c=0;c<2;++c)for(int k=0;k<2;++k)out.v[r*2+c]+=v[r*2+k]*b.v[k*2+c];return *this=out;}friend Matrix operator+(Matrix a,const Matrix&b){return a+=b;}friend Matrix operator*(Matrix a,const Matrix&b){return a*=b;}friend bool operator==(const Matrix&,const Matrix&)=default;};
void matrix_suite(){IntegerSieve sieve(65);std::mt19937 rng(61022);for(std::size_t n:{0U,1U,2U,7U,32U,65U})for(int rep=0;rep<20;++rep){std::vector<Matrix>a(n),b(n);for(auto*v:{&a,&b})for(auto&x:*v)for(auto&e:x.v)e=static_cast<int>(rng()%5)-2;for(bool gcd:{false,true}){std::vector<Matrix>expected(n),self(n);for(std::size_t i=0;i<n;++i)for(std::size_t j=0;j<n;++j){const auto k=gcd?std::gcd(i,j):std::lcm(i,j);if(k<n){expected[k]+=a[i]*b[j];self[k]+=a[i]*a[j];}}check((gcd?gcd_convolution(a,b,sieve):lcm_convolution(a,b,sieve))==expected);check((gcd?gcd_square(a,sieve):lcm_square(a,sieve))==self);}}}
}
int main(){signed_suite<std::int8_t>();signed_suite<std::int16_t>();signed_suite<std::int32_t>();signed_suite<std::int64_t>();matrix_suite();std::cout<<"PASS checks="<<checks<<'\n';}
