// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_ZETA_REQUIRE_EXTRA
#define SIRKIT_ZETA_REQUIRE_EXTRA
#endif
// 破壊的変換の例外は途中状態、値渡しの畳み込みは呼出元を保持する。
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#ifndef SIRKIT_ZETA_TEST_HEADER
#define SIRKIT_ZETA_TEST_HEADER "../../../Math/Zeta/Bitwise_Convolution.cpp"
#endif
#include SIRKIT_ZETA_TEST_HEADER
static volatile std::ptrdiff_t countdown=-1;
static volatile std::size_t live=0,calls=0;
static std::size_t checks=0,allocation_failures=0,value_failures=0,arithmetic_failures=0;
[[gnu::noinline]] void* operator new(std::size_t n) {
    if(countdown==0)throw std::bad_alloc();
    if(countdown>0)countdown=countdown-1;
    if(void* p=std::malloc(n?n:1)){live=live+1;calls=calls+1;return p;}throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]] static void release(void* p) noexcept {if(p)live=live-1;std::free(p);}
void operator delete(void* p)noexcept{release(p);}void operator delete[](void* p)noexcept{release(p);}
void operator delete(void* p,std::size_t)noexcept{release(p);}void operator delete[](void* p,std::size_t)noexcept{release(p);}
static void require_impl(bool ok,const char* expression,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s\n",line,expression);std::abort();}}
#define require(...) require_impl((__VA_ARGS__),#__VA_ARGS__,__LINE__)
struct Value {
    std::int64_t x=0;static inline int alive=0,copy_budget=-1,arithmetic_budget=-1;
    static void event(int& budget){if(budget==0)throw std::runtime_error("injected");if(budget>0)--budget;}
    Value(){++alive;}explicit Value(std::int64_t n):x(n){++alive;}
    Value(const Value& rhs):x(rhs.x){event(copy_budget);++alive;}Value(Value&& rhs)noexcept:x(rhs.x){++alive;}
    Value& operator=(const Value& rhs){event(copy_budget);x=rhs.x;return *this;}
    Value& operator=(Value&& rhs)noexcept{x=rhs.x;return *this;}
    ~Value(){--alive;}
    Value& operator+=(const Value& rhs){event(arithmetic_budget);x+=rhs.x;return *this;}
    Value& operator-=(const Value& rhs){event(arithmetic_budget);x-=rhs.x;return *this;}
    Value& operator*=(const Value& rhs){event(arithmetic_budget);x*=rhs.x;return *this;}
    friend bool operator==(const Value&,const Value&)=default;
};
// コピーは自明でも、演算が送出する型。GCCの展開経路を実際に通す。
struct TrivialValue {
    std::int64_t x=0;
    static inline int budget=-1;
    static void event(){if(budget==0)throw std::runtime_error("arithmetic");if(budget>0)--budget;}
    TrivialValue& operator+=(const TrivialValue& b){event();x+=b.x;return *this;}
    TrivialValue& operator-=(const TrivialValue& b){event();x-=b.x;return *this;}
    TrivialValue& operator*=(const TrivialValue& b){event();x*=b.x;return *this;}
    friend bool operator==(const TrivialValue&,const TrivialValue&)=default;
};
static_assert(std::is_trivially_copyable_v<TrivialValue> && sizeof(TrivialValue)<=8);
template<class F>static void trivial_value_exception(F fn){
    std::vector<TrivialValue> a(32),b(32);
    for(std::size_t i=0;i<a.size();++i){a[i].x=static_cast<std::int64_t>(i%7);b[i].x=static_cast<std::int64_t>(i%5);}
    const auto old_a=a,old_b=b;bool success=false;
    for(int k=0;k<300;++k){const auto owned=live;TrivialValue::budget=k;
        try{auto out=fn(a,b);TrivialValue::budget=-1;require(out.size()==a.size());success=true;}
        catch(const std::runtime_error&){TrivialValue::budget=-1;++arithmetic_failures;}
        require(live==owned);require(a==old_a&&b==old_b);if(success)break;
    }
    require(success);
}
template<class F>static void trivial_inplace_exception(F fn){
    std::vector<TrivialValue> a(32);for(std::size_t i=0;i<a.size();++i)a[i].x=static_cast<std::int64_t>(i+1);
    const auto original=a;bool success=false,partial=false;
    for(int k=0;k<100;++k){a=original;const auto owned=live;TrivialValue::budget=k;
        try{fn(a);TrivialValue::budget=-1;success=true;}
        catch(const std::runtime_error&){TrivialValue::budget=-1;++arithmetic_failures;partial|=a!=original;}
        require(live==owned&&a.size()==original.size());if(success)break;
    }
    require(success&&partial);
}
static std::vector<Value> values(){std::vector<Value> f;f.reserve(8);for(int i=0;i<8;++i)f.emplace_back(i+1);return f;}
template<class F>static void allocation_checks(F fn){
    std::vector<std::int64_t>a{1,2,3,4,5,6,7,8},b{2,3,5,7,11,13,17,19};const auto old_a=a,old_b=b;
    bool success=false;for(int k=0;k<8;++k){const auto before=live;countdown=k;try{auto out=fn(a,b);countdown=-1;require(out.size()==8);success=true;}catch(const std::bad_alloc&){countdown=-1;++allocation_failures;}
        require(live==before);require(a==old_a&&b==old_b);if(success)break;}require(success);
}
template<class F>static void value_checks(F fn){
    auto a=values(),b=values();const auto old_a=a,old_b=b;bool success=false;
    for(int k=0;k<40;++k){const auto before=Value::alive;Value::copy_budget=k;try{auto out=fn(a,b);Value::copy_budget=-1;require(out.size()==8);success=true;}catch(const std::runtime_error&){Value::copy_budget=-1;++value_failures;}
        require(Value::alive==before);require(a==old_a&&b==old_b);if(success)break;}require(success);
    success=false;
    for(int k=0;k<80;++k){const auto before=Value::alive;Value::arithmetic_budget=k;try{auto out=fn(a,b);Value::arithmetic_budget=-1;require(out.size()==8);success=true;}catch(const std::runtime_error&){Value::arithmetic_budget=-1;++arithmetic_failures;}
        require(Value::alive==before);require(a==old_a&&b==old_b);if(success)break;}require(success);
}
template<class F>static void inplace_exception(F fn){
    auto input=values();const auto original=input;bool observed_partial=false,success=false;
    for(int k=0;k<30;++k){input=original;const auto before=Value::alive;Value::arithmetic_budget=k;try{fn(input);Value::arithmetic_budget=-1;success=true;}catch(const std::runtime_error&){Value::arithmetic_budget=-1;++arithmetic_failures;observed_partial|=input!=original;}
        require(input.size()==original.size());require(Value::alive==before);if(success)break;}
    require(success&&observed_partial);
}
int main(){const auto starting_live=live;
    const auto and_fn=[](const auto&a,const auto&b){return bitwise_and_convolution(a,b);};
    const auto or_fn=[](const auto&a,const auto&b){return bitwise_or_convolution(a,b);};
    allocation_checks(and_fn);allocation_checks(or_fn);value_checks(and_fn);value_checks(or_fn);
    trivial_value_exception(and_fn);trivial_value_exception(or_fn);
#ifdef SIRKIT_ZETA_REQUIRE_EXTRA
    const auto and_square=[](const auto&a,const auto&){return bitwise_and_square(a);};
    const auto or_square=[](const auto&a,const auto&){return bitwise_or_square(a);};
    allocation_checks(and_square);allocation_checks(or_square);value_checks(and_square);value_checks(or_square);
    trivial_value_exception(and_square);trivial_value_exception(or_square);
#endif
    inplace_exception([](auto&f){superset_zeta_transform(f);});inplace_exception([](auto&f){superset_mobius_transform(f);});
    inplace_exception([](auto&f){subset_zeta_transform(f);});inplace_exception([](auto&f){subset_mobius_transform(f);});
    trivial_inplace_exception([](auto&f){superset_zeta_transform(f);});
    trivial_inplace_exception([](auto&f){superset_mobius_transform(f);});
    trivial_inplace_exception([](auto&f){subset_zeta_transform(f);});
    trivial_inplace_exception([](auto&f){subset_mobius_transform(f);});
    {std::vector<std::uint64_t> a(1024,2),b(1024,3);const auto prior=calls;countdown=0;
        superset_zeta_transform(a);superset_mobius_transform(a);subset_zeta_transform(a);subset_mobius_transform(a);
        auto* storage=a.data();auto result=bitwise_and_convolution(std::move(a),std::move(b));countdown=-1;
        require(result.data()==storage);require(calls==prior);require(result.size()==1024);
#ifdef SIRKIT_ZETA_REQUIRE_EXTRA
        countdown=0;storage=result.data();auto squared=bitwise_or_square(std::move(result));countdown=-1;require(squared.data()==storage);require(calls==prior);
#endif
    }
    require(Value::alive==0);require(live==starting_live);
    std::printf("PASS checks=%zu allocation_failures=%zu value_failures=%zu arithmetic_failures=%zu live_delta=%td\n",checks,allocation_failures,value_failures,arithmetic_failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(starting_live));
}
