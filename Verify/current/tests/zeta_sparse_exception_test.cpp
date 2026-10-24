// SPDX-License-Identifier: CC0-1.0
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta/Sparse_Transform.cpp"
#endif
#include SOURCE
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <source_location>
#include <cstdio>
#include <stdexcept>
namespace track {
inline int allocation=-1, arithmetic=-1;
inline std::atomic<std::size_t> live{0}, requests{0};
inline std::size_t values=0;
inline std::uint64_t checks=0;
void check(bool b,std::source_location at=std::source_location::current()){++checks;if(!b){std::fprintf(stderr,"failure at line %u, live=%zu values=%zu allocation=%d arithmetic=%d\n",at.line(),live.load(),values,allocation,arithmetic);throw std::runtime_error("sparse exception ownership mismatch");}}
void hit(int& b){if(b==0)throw std::runtime_error("injected arithmetic failure");if(b>0)--b;}
}
#if defined(__GNUC__) || defined(__clang__)
#define SIRKIT_TEST_NOINLINE [[gnu::noinline]]
#else
#define SIRKIT_TEST_NOINLINE
#endif
SIRKIT_TEST_NOINLINE void* operator new(std::size_t n){++track::requests;if(track::allocation==0)throw std::bad_alloc();if(track::allocation>0)--track::allocation;void*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();++track::live;return p;}
SIRKIT_TEST_NOINLINE void operator delete(void*p)noexcept{if(p){--track::live;std::free(p);}}
SIRKIT_TEST_NOINLINE void operator delete(void*p,std::size_t)noexcept{::operator delete(p);}
SIRKIT_TEST_NOINLINE void* operator new[](std::size_t n){return ::operator new(n);}
SIRKIT_TEST_NOINLINE void operator delete[](void*p)noexcept{::operator delete(p);}
SIRKIT_TEST_NOINLINE void operator delete[](void*p,std::size_t)noexcept{::operator delete(p);}
#undef SIRKIT_TEST_NOINLINE
struct Value{
 std::int64_t value=0;Value(){++track::values;}explicit Value(std::int64_t x):value(x){++track::values;}
 Value(const Value&o):value(o.value){++track::values;}Value(Value&&o)noexcept:value(o.value){++track::values;}
 Value&operator=(const Value&)=default;Value&operator=(Value&&)noexcept=default;~Value(){--track::values;}
 Value&operator+=(const Value&o){track::hit(track::arithmetic);value+=o.value;return *this;}
 Value&operator-=(const Value&o){track::hit(track::arithmetic);value-=o.value;return *this;}
};
int main(){using track::check;const auto initial=track::live.load();std::size_t zero_allocation_calls=0,failed_allocations=0,failed_arithmetic=0;
 {
  IntegerSieve sieve(400000);
  // 入力と篩の準備後、prefix経路で確保しないことを調べる。
  for(int n:{0,1,2,32,4096,20000}){std::vector<std::pair<int,std::int64_t>> f;for(int k=1;k<=n;++k)f.emplace_back(k,k%11);const auto original=f;
   const auto before=track::requests.load();track::allocation=0;sparse_multiple_zeta_transform(f,sieve);sparse_multiple_mobius_transform(f,sieve);track::allocation=-1;check(track::requests.load()==before);check(f==original);zero_allocation_calls+=2;
  }
  for(int shape=0;shape<3;++shape){std::vector<std::pair<int,Value>> original;if(shape<2){for(int k=1;k<=256;++k)original.emplace_back(k,Value(k%7));if(shape==1)std::reverse(original.begin(),original.end());}else{for(int k=1;k<=360360;++k)if(360360%k==0)original.emplace_back(k,Value(k%7));std::reverse(original.begin(),original.end());}
   for(bool inverse:{false,true})for(int kind=0;kind<2;++kind){bool success=false;for(int at=0;at<10000&&!success;++at){const auto before=track::live.load(), values=track::values;
    {auto f=original;const auto during=track::live.load(), objects=track::values;track::allocation=kind==0?at:-1;track::arithmetic=kind==1?at:-1;
     try{if(inverse)sparse_multiple_mobius_transform(f,sieve);else sparse_multiple_zeta_transform(f,sieve);success=true;}
     catch(const std::bad_alloc&){check(kind==0);++failed_allocations;}
     catch(const std::runtime_error&){check(kind==1);++failed_arithmetic;}
     track::allocation=track::arithmetic=-1;if(track::live.load()!=during||track::values!=objects||f.size()!=original.size())std::fprintf(stderr,"shape=%d inv=%d kind=%d at=%d during=%zu objects=%zu n=%zu expectedn=%zu\n",shape,inverse,kind,at,during,objects,f.size(),original.size());check(track::live.load()==during);check(track::values==objects);check(f.size()==original.size());
    }
    check(track::live.load()==before);check(track::values==values);
   }check(success);}
  }
 }
 check(track::live.load()==initial);check(track::values==0);
 std::cout<<"PASS checks="<<track::checks<<" zero_allocation_calls="<<zero_allocation_calls<<" failed_allocations="<<failed_allocations<<" failed_arithmetic="<<failed_arithmetic<<" live_delta="<<track::live.load()-initial<<'\n';
}
