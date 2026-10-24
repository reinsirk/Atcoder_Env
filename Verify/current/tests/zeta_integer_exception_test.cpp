// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_ZETA_INTEGER_EXTRA
#define SIRKIT_ZETA_INTEGER_EXTRA
#endif
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta_Mebius.cpp"
#endif
#include SOURCE
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>
namespace failure {
inline int allocation=-1,copy=-1,arithmetic=-1;inline std::int64_t live=0,values=0;inline std::uint64_t checks=0;
void check(bool x){++checks;if(!x)std::abort();}
void hit(int& budget){if(budget==0)throw std::runtime_error("injected value failure");if(budget>0)--budget;}
struct alignas(std::max_align_t) Header{};
}
[[gnu::noinline]] void* operator new(std::size_t n){if(failure::allocation==0)throw std::bad_alloc();if(failure::allocation>0)--failure::allocation;if(n>std::numeric_limits<std::size_t>::max()-sizeof(failure::Header))throw std::bad_alloc();auto*p=static_cast<failure::Header*>(std::malloc(n+sizeof(failure::Header)));if(!p)throw std::bad_alloc();++failure::live;return p+1;}
[[gnu::noinline]] void operator delete(void*p)noexcept{if(p){--failure::live;std::free(static_cast<failure::Header*>(p)-1);}}
[[gnu::noinline]] void operator delete(void*p,std::size_t)noexcept{::operator delete(p);}
[[gnu::noinline]] void* operator new[](std::size_t n){return ::operator new(n);}[[gnu::noinline]] void operator delete[](void*p)noexcept{::operator delete(p);}[[gnu::noinline]] void operator delete[](void*p,std::size_t)noexcept{::operator delete(p);}
namespace {
struct Value{std::int64_t v=0;Value(){++failure::values;}Value(std::int64_t x):v(x){++failure::values;}Value(const Value&o):v(o.v){failure::hit(failure::copy);++failure::values;}Value(Value&&o)noexcept:v(o.v){++failure::values;}~Value(){--failure::values;}Value&operator=(const Value&o){failure::hit(failure::copy);v=o.v;return *this;}Value&operator=(Value&&o)noexcept{v=o.v;return *this;}Value&operator+=(const Value&o){failure::hit(failure::arithmetic);v+=o.v;return *this;}Value&operator-=(const Value&o){failure::hit(failure::arithmetic);v-=o.v;return *this;}Value&operator*=(const Value&o){failure::hit(failure::arithmetic);v*=o.v;return *this;}friend Value operator+(Value a,const Value&b){return a+=b;}friend Value operator*(Value a,const Value&b){return a*=b;}friend bool operator==(const Value&a,const Value&b){return a.v==b.v;}};
void reset(){failure::allocation=failure::copy=failure::arithmetic=-1;}
}
int main(){using failure::check;const auto initial=failure::live;int allocation_sites=0,value_sites=0,math_sites=0;
{
 IntegerSieve sieve(40);std::vector<Value>a(33),b(33);for(std::size_t i=0;i<a.size();++i){a[i].v=static_cast<std::int64_t>(i%3);b[i].v=static_cast<std::int64_t>(i%4);}
 const auto original_a=a,original_b=b;
 for(int op=0;op<8;++op){
#ifndef SIRKIT_ZETA_INTEGER_EXTRA
  if(op>=4)continue;
#endif
  for(int kind=0;kind<3;++kind){bool success=false;for(int at=0;at<10000&&!success;++at){const auto live=failure::live,values=failure::values;reset();if(kind==0)failure::allocation=at;else if(kind==1)failure::copy=at;else failure::arithmetic=at;try{std::vector<Value>out;switch(op){case 0:out=gcd_convolution(a,b,sieve);break;case 1:out=lcm_convolution(a,b,sieve);break;case 2:out=gcd_convolution(a,b);break;case 3:out=lcm_convolution(a,b);break;
#ifdef SIRKIT_ZETA_INTEGER_EXTRA
 case 4:out=gcd_square(a,sieve);break;case 5:out=lcm_square(a,sieve);break;case 6:out=gcd_square(a);break;case 7:out=lcm_square(a);break;
#endif
 }success=true;}catch(const std::bad_alloc&){check(kind==0);++allocation_sites;}catch(const std::runtime_error&){check(kind!=0);if(kind==1)++value_sites;else ++math_sites;}reset();check(a==original_a&&b==original_b);check(failure::live==live&&failure::values==values);}check(success);}
 }
 for(int op=0;op<4;++op){auto f=a;const auto values=failure::values;failure::arithmetic=2;bool threw=false;try{switch(op){case 0:multiple_zeta_transform(f,sieve);break;case 1:multiple_mobius_transform(f,sieve);break;case 2:divisor_zeta_transform(f,sieve);break;case 3:divisor_mobius_transform(f,sieve);break;}}catch(const std::runtime_error&){threw=true;}reset();check(threw);check(failure::values==values);}
}
for(int op=0;op<2;++op){IntegerSieve kept(100);const auto lpf=kept.least_prime_factor,primes=kept.prime_list;bool success=false;for(int at=0;at<100&&!success;++at){const auto live=failure::live;failure::allocation=at;try{if(op==0){IntegerSieve x(4000);check(x.is_prime(3989));}else kept=IntegerSieve(4000);success=true;}catch(const std::bad_alloc&){++allocation_sites;}reset();if(!success)check(kept.least_prime_factor==lpf&&kept.prime_list==primes);if(op==0||!success)check(failure::live==live);}check(success);}
check(failure::live==initial&&failure::values==0);std::cout<<"PASS checks="<<failure::checks<<" allocation_failures="<<allocation_sites<<" copy_failures="<<value_sites<<" arithmetic_failures="<<math_sites<<" live_delta="<<failure::live-initial<<'\n';}
