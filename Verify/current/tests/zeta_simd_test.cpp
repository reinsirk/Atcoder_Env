// SPDX-License-Identifier: CC0-1.0
#ifndef ZETA_SOURCE
#define ZETA_SOURCE "../../../Math/Zeta/Bitwise_Convolution.cpp"
#endif
#include ZETA_SOURCE
#include <iostream>
#include <random>
#include <stdexcept>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>
struct Box {std::int64_t x=0;Box()=default;Box(std::int64_t v):x(v){};Box& operator+=(Box a){x+=a.x;return *this;}Box& operator-=(Box a){x-=a.x;return *this;}Box& operator*=(Box a){x*=a.x;return *this;}friend bool operator==(Box,Box)=default;};
std::uint64_t checks=0;
void require(bool v){++checks;if(!v)throw std::runtime_error("mismatch");}
template<class T>void test(){
 std::mt19937_64 g(931);
 for(unsigned lg=0;lg<=12;++lg){
  const std::size_t n=std::size_t{1}<<lg;
  for(unsigned r=0;r<10;++r){
   std::vector<T>a(n);for(auto&x:a)x=T(g()%7);
   if constexpr(std::is_integral_v<T> && sizeof(T)==1){for(auto&x:a)x=T{};a[g()%n]=T(3);}
   auto sub=a,sup=a,refsub=a,refsup=a;
   for(std::size_t k=1;k<n;k*=2)for(std::size_t s=0;s<n;++s)if(s&k)refsub[s]+=refsub[s^k];
   for(std::size_t k=1;k<n;k*=2)for(std::size_t s=0;s<n;++s)if(!(s&k))refsup[s]+=refsup[s|k];
   subset_zeta_transform(sub);superset_zeta_transform(sup);require(sub==refsub);require(sup==refsup);
   subset_mobius_transform(sub);superset_mobius_transform(sup);require(sub==a);require(sup==a);
  }
 }
 std::vector<T>empty;subset_zeta_transform(empty);superset_zeta_transform(empty);subset_mobius_transform(empty);superset_mobius_transform(empty);require(empty.empty());
#ifndef SKIP_RUNTIME_CHECKS
 for(unsigned op=0;op<4;++op){std::vector<T>bad(3);bool threw=false;try{switch(op){case 0:subset_zeta_transform(bad);break;case 1:subset_mobius_transform(bad);break;case 2:superset_zeta_transform(bad);break;case 3:superset_mobius_transform(bad);break;}}catch(const std::invalid_argument&){threw=true;}require(threw);}
#endif
}
template<class T>void test_convolution(){
 std::mt19937_64 g(781);
 for(unsigned lg=0;lg<=6;++lg)for(unsigned r=0;r<5;++r){
  const std::size_t n=std::size_t{1}<<lg;std::vector<T>a(n),b(n),want_and(n),want_or(n);
  for(auto&x:a)x=T(g()%3);
  for(auto&x:b)x=T(g()%3);
  for(std::size_t i=0;i<n;++i)for(std::size_t j=0;j<n;++j){T product=a[i];product*=b[j];want_and[i&j]+=product;want_or[i|j]+=product;}
  require(bitwise_and_convolution(a,b)==want_and);require(bitwise_or_convolution(a,b)==want_or);
 }
 std::vector<T>e;require(bitwise_and_convolution(e,e).empty());require(bitwise_or_convolution(e,e).empty());
#ifndef SKIP_RUNTIME_CHECKS
 bool threw=false;try{(void)bitwise_and_convolution(std::vector<T>(2),std::vector<T>(4));}catch(const std::invalid_argument&){threw=true;}require(threw);
 threw=false;try{(void)bitwise_or_convolution(std::vector<T>(2),std::vector<T>(4));}catch(const std::invalid_argument&){threw=true;}require(threw);
#endif
}
int main(){test_convolution<std::uint32_t>();test_convolution<std::uint64_t>();test_convolution<std::int64_t>();test_convolution<float>();test_convolution<Box>();test<std::uint8_t>();test<std::int8_t>();test<std::uint16_t>();test<std::int16_t>();test<std::uint32_t>();test<std::int32_t>();test<std::uint64_t>();test<std::int64_t>();test<float>();test<double>();test<Box>();std::cout << "zeta_simd_test: " << checks << " checks passed\n";}
