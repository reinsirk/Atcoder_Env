// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_ZETA_REQUIRE_EXTRA
#define SIRKIT_ZETA_REQUIRE_EXTRA
#endif
// AND/ORと4集合変換を、全要素対・包含判定による独立参照と照合する。
#include <algorithm>
#include <array>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#ifndef SIRKIT_ZETA_TEST_HEADER
#define SIRKIT_ZETA_TEST_HEADER "../../../Math/Zeta/Bitwise_Convolution.cpp"
#endif
#include SIRKIT_ZETA_TEST_HEADER
static std::uint64_t checks=0;
static void require_impl(bool ok,const char* expression,int line) { ++checks; if(!ok) { std::fprintf(stderr,"line %d: %s\n",line,expression); std::abort(); } }
#define require(...) require_impl((__VA_ARGS__),#__VA_ARGS__,__LINE__)
struct Mod {
    std::uint32_t x=0; static constexpr std::uint32_t modulus=998244353;
    Mod()=default; explicit Mod(std::uint64_t v):x(static_cast<std::uint32_t>(v%modulus)){}
    Mod& operator+=(const Mod& b) { x+=b.x;if(x>=modulus)x-=modulus;return *this; }
    Mod& operator-=(const Mod& b) { x=x>=b.x?x-b.x:x+modulus-b.x;return *this; }
    Mod& operator*=(const Mod& b) { x=static_cast<std::uint32_t>(std::uint64_t(x)*b.x%modulus);return *this; }
    friend bool operator==(const Mod&,const Mod&)=default;
};
struct Generic {
    std::int64_t x=0; Generic()=default; explicit Generic(std::int64_t v):x(v){}
    Generic& operator+=(const Generic& b) { x+=b.x;return *this; }
    Generic& operator-=(const Generic& b) { x-=b.x;return *this; }
    Generic& operator*=(const Generic& b) { x*=b.x;return *this; }
    friend bool operator==(const Generic&,const Generic&)=default;
};
template<class T> static T add(T a,const T& b) { a+=b;return a; }
template<class T> static T sub(T a,const T& b) { a-=b;return a; }
template<class T> static T product(const T& a,const T& b) {
    if constexpr(std::is_integral_v<T> && std::is_unsigned_v<T>)return static_cast<T>(std::uint64_t(a)*std::uint64_t(b));
    else { T result=a;result*=b;return result; }
}
template<class T> static std::vector<T> transform_reference(const std::vector<T>& f,bool subset,bool inverse) {
    std::vector<T> out(f.size());
    for(std::size_t s=0;s<f.size();++s)for(std::size_t t=0;t<f.size();++t) {
        if((subset && (t&s)==t)||(!subset && (t&s)==s)) {
            const bool negative=inverse && (std::popcount(s^t)&1);
            out[s]=negative?sub(out[s],f[t]):add(out[s],f[t]);
        }
    }
    return out;
}
template<class T> static std::vector<T> convolution_reference(const std::vector<T>& f,const std::vector<T>& g,bool is_and) {
    std::vector<T> out(f.size());
    for(std::size_t i=0;i<f.size();++i)for(std::size_t j=0;j<g.size();++j) {
        const auto index=is_and?(i&j):(i|j);out[index]=add(out[index],product(f[i],g[j]));
    }
    return out;
}
template<class T> static void check_case(std::vector<T> f,std::vector<T> g,bool convolution=true) {
    const auto original_f=f,original_g=g;
    auto a=f;superset_zeta_transform(a);require(a==transform_reference(f,false,false));
    superset_mobius_transform(a);require(a==f);
    a=f;subset_zeta_transform(a);require(a==transform_reference(f,true,false));
    subset_mobius_transform(a);require(a==f);
    a=f;superset_mobius_transform(a);require(a==transform_reference(f,false,true));
    a=f;subset_mobius_transform(a);require(a==transform_reference(f,true,true));
    if(convolution) {
        require(bitwise_and_convolution(f,g)==convolution_reference(f,g,true));
        require(bitwise_or_convolution(f,g)==convolution_reference(f,g,false));
        require(f==original_f && g==original_g);
        require(bitwise_and_convolution(f,f)==convolution_reference(f,f,true));
        require(bitwise_or_convolution(f,f)==convolution_reference(f,f,false));
#ifdef SIRKIT_ZETA_REQUIRE_EXTRA
        require(bitwise_and_square(f)==convolution_reference(f,f,true));
        require(bitwise_or_square(f)==convolution_reference(f,f,false));
        auto square_input=f;auto* square_storage=square_input.data();
        auto square_result=bitwise_and_square(std::move(square_input));
        require(square_result==convolution_reference(f,f,true));if(!f.empty())require(square_result.data()==square_storage);
        square_input=f;square_storage=square_input.data();square_result=bitwise_or_square(std::move(square_input));
        require(square_result==convolution_reference(f,f,false));if(!f.empty())require(square_result.data()==square_storage);
#endif
        auto copy=f;auto* allocation=copy.data();const auto expected=convolution_reference(copy,g,true);
        auto moved=bitwise_and_convolution(std::move(copy),g);require(moved==expected);
        if(!f.empty())require(moved.data()==allocation);
        copy=f;allocation=copy.data();auto other=g;auto result=bitwise_or_convolution(std::move(copy),std::move(other));
        require(result==convolution_reference(f,g,false));if(!f.empty())require(result.data()==allocation);
    }
}
template<class T> static void type_cases() {
    std::mt19937_64 rng(0x149e275adULL+sizeof(T));
    for(std::size_t n : std::array<std::size_t,10>{0,1,2,4,8,16,32,64,128,256})for(int trial=0;trial<6;++trial) {
        std::vector<T> a(n),b(n);
        for(std::size_t i=0;i<n;++i) {
            if constexpr(std::is_unsigned_v<T>) { a[i]=static_cast<T>(rng());b[i]=static_cast<T>(rng()); }
            else if constexpr(std::is_integral_v<T>) { a[i]=static_cast<T>(static_cast<int>(rng()%15)-7);b[i]=static_cast<T>(static_cast<int>(rng()%15)-7); }
            else if constexpr(std::is_same_v<T,Mod>) { a[i]=T(rng());b[i]=T(rng()); }
            else {a[i]=T(static_cast<int>(rng()%15)-7);b[i]=T(static_cast<int>(rng()%15)-7);}
        }
        bool convolution=true;
#ifndef SIRKIT_ZETA_REQUIRE_EXTRA
        if constexpr(std::is_same_v<T,std::uint16_t>)convolution=false; // 旧版の整数昇格UBは診断を別記。
#endif
        check_case(a,b,convolution);
    }
    // SIMD段階を跨ぐ大きさでも、疎な入力の独立全要素参照と一致する。
    constexpr std::size_t n=1024;std::vector<T> a(n),b(n);
    a[0]=T{2};a[17]=T{3};a[1023]=T{5};b[0]=T{7};b[256]=T{11};b[1023]=T{13};
    check_case(a,b);
}
#ifndef SKIP_RUNTIME_CHECKS
template<class F> static void invalid(F fn) { bool caught=false;try {fn();}catch(const std::invalid_argument&) {caught=true;}require(caught); }
static void boundary_checks() {
    for(std::size_t n:{3,5,6,7,9,15,17,31,33,63,65,127,129}) {
        std::vector<std::uint32_t> f(n,1);const auto old=f;
        invalid([&]{superset_zeta_transform(f);});require(f==old);
        invalid([&]{superset_mobius_transform(f);});require(f==old);
        invalid([&]{subset_zeta_transform(f);});require(f==old);
        invalid([&]{subset_mobius_transform(f);});require(f==old);
        invalid([&]{(void)bitwise_and_convolution(f,f);});require(f==old);
        invalid([&]{(void)bitwise_or_convolution(f,f);});require(f==old);
#ifdef SIRKIT_ZETA_REQUIRE_EXTRA
        invalid([&]{(void)bitwise_and_square(f);});require(f==old);
        invalid([&]{(void)bitwise_or_square(f);});require(f==old);
#endif
    }
    for(auto lengths:{std::pair<std::size_t,std::size_t>{0,1},{1,0},{2,4},{3,4},{4,3}}) {
        std::vector<std::uint32_t> a(lengths.first,2),b(lengths.second,3);const auto old_a=a,old_b=b;
        invalid([&]{(void)bitwise_and_convolution(a,b);});require(a==old_a&&b==old_b);
        invalid([&]{(void)bitwise_or_convolution(a,b);});require(a==old_a&&b==old_b);
    }
}
#endif
template<class T> static void signed_narrow_cases() {
    for(std::size_t n : std::array<std::size_t,10>{0,1,2,4,8,16,32,64,128,256}) {
        std::vector<T> a(n),b(n);
        if(n){a[0]=T{-2};b[n-1]=T{3};}if(n>1){a[n-1]=T{1};b[0]=T{-1};}
        check_case(a,b);
    }
    for(auto value:{std::numeric_limits<T>::min(),std::numeric_limits<T>::max()}) {
        std::vector<T> a{value},b{1};check_case(a,b,false);
        require(bitwise_and_convolution(a,b)==a);require(bitwise_or_convolution(a,b)==a);
    }
}
template<class T> static void complex_cases() {
    // 全途中結果が浮動小数点でも厳密に表せる小さい整数値で照合する。
    for(std::size_t n : {std::size_t{16},std::size_t{64},std::size_t{256}}) {
        std::vector<std::complex<T>> a(n),b(n);
        for(std::size_t i=0;i<n;++i) {
            a[i]={static_cast<T>(static_cast<int>(i%7)-3),static_cast<T>(static_cast<int>(i%5)-2)};
            b[i]={static_cast<T>(static_cast<int>(i%3)-1),static_cast<T>(static_cast<int>(i%7)-3)};
        }
        check_case(a,b);
    }
}
int main() {
    signed_narrow_cases<std::int8_t>();signed_narrow_cases<std::int16_t>();
    type_cases<std::uint8_t>();type_cases<std::uint16_t>();type_cases<std::uint32_t>();type_cases<std::uint64_t>();
    type_cases<std::int32_t>();type_cases<std::int64_t>();type_cases<Mod>();type_cases<Generic>();
    type_cases<float>();type_cases<double>();complex_cases<float>();complex_cases<double>();
    // 1要素では全中間値も型の範囲内である境界値。
    for(auto value:{std::numeric_limits<std::int64_t>::min(),std::numeric_limits<std::int64_t>::max()}) {
        std::vector<std::int64_t> a{value},b{1};require(bitwise_and_convolution(a,b)==a);require(bitwise_or_convolution(a,b)==a);
    }
#ifdef SIRKIT_ZETA_REQUIRE_EXTRA
    check_case(std::vector<std::uint16_t>{65535},std::vector<std::uint16_t>{65535});
#endif
#ifndef SKIP_RUNTIME_CHECKS
    boundary_checks();
#endif
    std::printf("PASS checks=%llu\n",static_cast<unsigned long long>(checks));
}
