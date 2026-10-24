// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LIS_REQUIRE_EXTRA
#define SIRKIT_LIS_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_LIS_SOURCE
#define SIRKIT_LIS_SOURCE "../../../Algorithm/LIS.cpp"
#endif
#include SIRKIT_LIS_SOURCE
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <utility>
#include <vector>
static std::ptrdiff_t allocation_budget=-1;
static std::size_t live_blocks=0,checks=0,allocation_failures=0,copy_failures=0,comparison_failures=0;
[[gnu::noinline]]void*operator new(std::size_t n){if(allocation_budget==0)throw std::bad_alloc();if(allocation_budget>0)--allocation_budget;if(void*p=std::malloc(n?n:1)){++live_blocks;return p;}throw std::bad_alloc();}
[[gnu::noinline]]void*operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]]static void free_block(void*p)noexcept{if(p)--live_blocks;std::free(p);}
void operator delete(void*p)noexcept{free_block(p);}void operator delete[](void*p)noexcept{free_block(p);}void operator delete(void*p,std::size_t)noexcept{free_block(p);}void operator delete[](void*p,std::size_t)noexcept{free_block(p);}
static void require_impl(bool ok,int line){++checks;if(!ok){std::fprintf(stderr,"line %d failed after %ju checks\n",line,static_cast<std::uintmax_t>(checks));std::abort();}}
#define require(x) require_impl(bool(x),__LINE__)
struct Failure{};
struct Value {
 int value;static inline std::ptrdiff_t copy_budget=-1,compare_budget=-1;static inline int alive=0;
 Value()=delete;explicit Value(int v):value(v){++alive;}
 static void before_copy(){if(copy_budget==0)throw Failure{};if(copy_budget>0)--copy_budget;}
 Value(const Value&o):value(o.value){before_copy();++alive;}
 Value(Value&&o)noexcept(false):value(o.value){before_copy();++alive;}
 Value&operator=(const Value&o){before_copy();value=o.value;return *this;}
 Value&operator=(Value&&o)noexcept(false){before_copy();value=o.value;return *this;}
 ~Value(){--alive;}
 friend bool operator<(const Value&a,const Value&b){if(compare_budget==0)throw Failure{};if(compare_budget>0)--compare_budget;return a.value<b.value;}
};
static bool ascending_input=false;
static void inspect_input(const std::vector<Value>& input){require(input.size()==129);for(std::size_t i=0;i<input.size();++i)require(input[i].value==(ascending_input?int(i):int((i*73)%131)));}
template<class F>static void inject(const std::vector<Value>& input,F action,bool copyable){
 const auto retained=live_blocks;const auto alive=Value::alive;inspect_input(input);
 for(std::ptrdiff_t budget=0;budget<200;++budget){bool failed=false;allocation_budget=budget;try{action();allocation_budget=-1;}catch(const std::bad_alloc&){allocation_budget=-1;failed=true;++allocation_failures;}require(live_blocks==retained);require(Value::alive==alive);inspect_input(input);if(!failed)break;require(budget<199);}
 if(copyable)for(std::ptrdiff_t budget=0;budget<2000;++budget){bool failed=false;Value::copy_budget=budget;try{action();Value::copy_budget=-1;}catch(const Failure&){Value::copy_budget=-1;failed=true;++copy_failures;}require(live_blocks==retained);require(Value::alive==alive);inspect_input(input);if(!failed)break;require(budget<1999);}
 for(std::ptrdiff_t budget=0;budget<2000;++budget){bool failed=false;Value::compare_budget=budget;try{action();Value::compare_budget=-1;}catch(const Failure&){Value::compare_budget=-1;failed=true;++comparison_failures;}require(live_blocks==retained);require(Value::alive==alive);inspect_input(input);if(!failed)break;require(budget<1999);}
}
template<bool Strict,class T>static void extra(const std::vector<T>& input){
#ifdef SIRKIT_LIS_REQUIRE_EXTRA
 static_assert(requires{LISEndingLengths<Strict>(input);});
#endif
 if constexpr(requires{LISEndingLengths<Strict>(input);})inject(input,[&]{const auto r=LISEndingLengths<Strict>(input);require(r.size()==input.size());},true);
}
template<bool Strict>static void suite(const std::vector<Value>& input){
 inject(input,[&]{const auto r=LIS<Strict>(input);require(r.size()==input.size());},true);
 inject(input,[&]{const auto r=LISLength<Strict>(input);require(r>0);},true);
 inject(input,[&]{const auto r=LISIndices<Strict>(input);require(!r.empty());},false);
 inject(input,[&]{const auto r=LISSequence<Strict>(input);require(!r.empty());},true);extra<Strict>(input);
}
template<class T>static void empty_extra(const std::vector<T>& input){if constexpr(requires{LISEndingLengths(input);})require(LISEndingLengths(input).empty());}
int main(){const auto beginning=live_blocks;require(Value::alive==0);
 for(bool ascending:{false,true}){ascending_input=ascending;std::vector<Value> input;input.reserve(129);for(int i=0;i<129;++i)input.emplace_back(ascending?i:(i*73)%131);suite<true>(input);suite<false>(input);}
 require(Value::alive==0);require(live_blocks==beginning);
 {std::vector<int> empty;allocation_budget=0;require(LIS<true>(empty).empty());require(LISLength(empty)==0);require(LISIndices(empty).empty());require(LISSequence(empty).empty());empty_extra(empty);allocation_budget=-1;}
 require(live_blocks==beginning);std::printf("PASS %ju checks; allocation failures=%ju copy failures=%ju comparison failures=%ju live_delta=%jd value_alive=%d\n",static_cast<std::uintmax_t>(checks),static_cast<std::uintmax_t>(allocation_failures),static_cast<std::uintmax_t>(copy_failures),static_cast<std::uintmax_t>(comparison_failures),static_cast<std::intmax_t>(live_blocks)-static_cast<std::intmax_t>(beginning),Value::alive);
}
