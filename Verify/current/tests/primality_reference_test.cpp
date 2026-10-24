// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PRIMALITY_REQUIRE_EXTRA
#define SIRKIT_PRIMALITY_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_PRIMALITY_SOURCE
#define SIRKIT_PRIMALITY_SOURCE "../../../Math/prime_judge(Miller-Rabin).cpp"
#endif
#include SIRKIT_PRIMALITY_SOURCE
#include <boost/multiprecision/cpp_int.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <ranges>
#include <type_traits>
#include <vector>
using Big=boost::multiprecision::cpp_int;
static std::size_t checks=0,sieve_cases=0,wide_cases=0,base_cases=0;
static void require_impl(bool ok,const char*expr,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s (%zu checks)\n",line,expr,checks);std::abort();}}
#define require(x) require_impl(bool(x),#x,__LINE__)
static Big power(Big base,std::uint64_t exp,const Big&n){Big result=1;while(exp){if(exp&1)result=result*base%n;exp>>=1;if(exp)base=base*base%n;}return result;}
template<class V>static bool reference(std::uint64_t n,const V&bases){if(n<2)return false;if(!(n&1))return n==2;auto odd=n-1;unsigned shifts=0;while(!(odd&1)){odd>>=1;++shifts;}const Big mod=n;for(auto base:bases){Big value=base;value%=mod;if(value<0)value+=mod;if(value==0)continue;value=power(value,odd,mod);if(value==1||value==mod-1)continue;bool found=false;for(unsigned j=1;j<shifts;++j){value=value*value%mod;if(value==mod-1){found=true;break;}}if(!found)return false;}return true;}
static constexpr std::array<std::uint64_t,16> trial_bases{2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53};
static bool independent_prime(std::uint64_t n){return reference(n,trial_bases);}
static void inspect(std::uint64_t n){++wide_cases;const bool expected=independent_prime(n);if(is_prime(n)!=expected){std::fprintf(stderr,"n=%ju expected=%d\n",static_cast<std::uintmax_t>(n),int(expected));require(false);}require(is_prime(n)==expected);if(n<=std::uint64_t(std::numeric_limits<std::int64_t>::max()))require(is_prime(static_cast<std::int64_t>(n))==expected);}
template<class I>static void typed(){static_assert(std::is_integral_v<I>);const std::array<std::uint64_t,13> small{0,1,2,3,4,5,7,11,49,61,97,121,127};for(auto n:small)if(n<=static_cast<std::uint64_t>(std::numeric_limits<I>::max())){require(is_prime(static_cast<I>(n))==independent_prime(n));require(MillerRabin(static_cast<I>(n),trial_bases)==independent_prime(n));}if constexpr(std::is_signed_v<I>){require(!is_prime(I{-1}));require(!is_prime(std::numeric_limits<I>::min()));require(!MillerRabin(std::numeric_limits<I>::min(),trial_bases));}const auto m=std::numeric_limits<I>::max();require(is_prime(m)==independent_prime(static_cast<std::uint64_t>(m)));}
struct BasesRange{std::array<std::int64_t,8> values;const auto*begin()const noexcept{return values.data();}const auto*end()const noexcept{return values.data()+values.size();}};
template<class Bases>static void custom(const Bases&bases){for(auto n:std::array<std::uint64_t,24>{0,1,2,3,4,5,9,17,25,61,341,561,1105,1729,2047,3215031751ULL,4294967291ULL,4759123141ULL,341550071728321ULL,3825123056546413051ULL,4611686018427387903ULL,9223372036854775807ULL,18446744073709551557ULL,18446744073709551615ULL}){++base_cases;require(MillerRabin(n,bases)==reference(n,bases));}}
#ifdef SIRKIT_PRIMALITY_REQUIRE_EXTRA
struct RangeFailure {};
struct ThrowRange {
    enum Site {none,begin_call,end_call,dereference,increment,comparison,destruction};
    Site site;
    struct Iterator {
        Site site; int index;
        int operator*()const {if(site==dereference)throw RangeFailure{};return 2;}
        Iterator&operator++(){if(site==increment)throw RangeFailure{};++index;return*this;}
        friend bool operator!=(const Iterator&a,int end){if(a.site==comparison)throw RangeFailure{};return a.index!=end;}
        ~Iterator() noexcept(false){if(site==destruction)throw RangeFailure{};}
    };
    Iterator begin()const {if(site==begin_call)throw RangeFailure{};return {site,0};}
    int end()const {if(site==end_call)throw RangeFailure{};return 1;}
};
struct MixedRange {
    std::array<int,1> data{2};
    struct Iterator {int operator*()const{return 2;}Iterator&operator++(){return*this;}friend bool operator!=(const Iterator&,int){return false;}};
    Iterator begin()const {throw RangeFailure{};}int end()const noexcept{return 1;}
    friend const int*begin(const MixedRange&r)noexcept{return r.data.data();}
    friend const int*end(const MixedRange&r)noexcept{return r.data.data()+1;}
};
struct AdlRange {
    std::array<int,2> values{2,3};
    friend const int*begin(const AdlRange&r)noexcept{return r.values.data();}
    friend const int*end(const AdlRange&r)noexcept{return r.values.data()+2;}
};
struct CopyRange {
    struct Iterator {
        using value_type=int;using difference_type=std::ptrdiff_t;using iterator_concept=std::input_iterator_tag;
        int index=0;Iterator()noexcept=default;Iterator(const Iterator&){throw RangeFailure{};}
        int operator*()const noexcept{return 2;}Iterator&operator++()noexcept{++index;return*this;}void operator++(int)noexcept{++*this;}
        friend bool operator==(const Iterator&a,int end)noexcept{return a.index==end;}
    };
    Iterator iterator;
    const Iterator&begin()const noexcept{return iterator;}int end()const noexcept{return 1;}
};
template<ThrowRange::Site Site>
struct ProperRange {
    struct Iterator {
        using value_type=int;using difference_type=std::ptrdiff_t;using iterator_concept=std::input_iterator_tag;
        int index=0;
        int operator*()const noexcept(Site!=ThrowRange::dereference){if constexpr(Site==ThrowRange::dereference)throw RangeFailure{};return 2;}
        Iterator&operator++()noexcept(Site!=ThrowRange::increment){if constexpr(Site==ThrowRange::increment)throw RangeFailure{};++index;return*this;}
        void operator++(int)noexcept(Site!=ThrowRange::increment){++*this;}
        friend bool operator==(const Iterator&a,int end)noexcept(Site!=ThrowRange::comparison){if constexpr(Site==ThrowRange::comparison)throw RangeFailure{};return a.index==end;}
    };
    Iterator begin()const noexcept(Site!=ThrowRange::begin_call){if constexpr(Site==ThrowRange::begin_call)throw RangeFailure{};return {};}
    int end()const noexcept(Site!=ThrowRange::end_call){if constexpr(Site==ThrowRange::end_call)throw RangeFailure{};return 1;}
};
template<ThrowRange::Site Site>static void proper_range(){
    static_assert(std::ranges::range<const ProperRange<Site>>);
    static_assert(!noexcept(MillerRabin(17,ProperRange<Site>{})));
    bool threw=false;try{(void)MillerRabin(17,ProperRange<Site>{});}catch(const RangeFailure&){threw=true;}require(threw);
}
template<class I>static void nearest_typed(){
    const auto value=std::numeric_limits<I>::max();const auto n=static_cast<std::uint64_t>(value);
    auto hi=n;while(!independent_prime(hi)&&hi<std::numeric_limits<std::uint64_t>::max())++hi;
    auto lo=n;while(lo>=2&&!independent_prime(lo))--lo;
    const auto next=next_prime(value),prev=prev_prime(value);
    require(bool(next)==independent_prime(hi));if(next)require(*next==hi);require(bool(prev)==(lo>=2));if(prev)require(*prev==lo);
    if constexpr(std::is_signed_v<I>){require(next_prime(std::numeric_limits<I>::min())==2);require(!prev_prime(std::numeric_limits<I>::min()));}
}
static void extras(){
    static_assert(noexcept(MillerRabin(17,std::array<int,2>{2,3})));
    static_assert(!noexcept(MillerRabin(17,ThrowRange{ThrowRange::none})));
    for(auto site:{ThrowRange::begin_call,ThrowRange::end_call,ThrowRange::dereference,ThrowRange::increment,ThrowRange::comparison,ThrowRange::destruction}){
        bool threw=false;try{(void)MillerRabin(17,ThrowRange{site});}catch(const RangeFailure&){threw=true;}require(threw);
    }
    static_assert(noexcept(MillerRabin(17,ProperRange<ThrowRange::none>{})));
    proper_range<ThrowRange::begin_call>();proper_range<ThrowRange::end_call>();proper_range<ThrowRange::dereference>();proper_range<ThrowRange::increment>();proper_range<ThrowRange::comparison>();
    static_assert(std::ranges::range<const MixedRange>);
    static_assert(!noexcept(MillerRabin(17,MixedRange{})));
    bool mixed_threw=false;try{(void)MillerRabin(17,MixedRange{});}catch(const RangeFailure&){mixed_threw=true;}require(mixed_threw);
    static_assert(!noexcept(MillerRabin(17,CopyRange{})));
    bool copy_threw=false;try{(void)MillerRabin(17,CopyRange{});}catch(const RangeFailure&){copy_threw=true;}require(copy_threw);
    static_assert(!noexcept(MillerRabin(17,AdlRange{})));require(MillerRabin(17,AdlRange{}));require(!MillerRabin(2047,AdlRange{}));
    static_assert(noexcept(MillerRabin(17,std::declval<const std::vector<int>&>())));
    const int c_bases[]{2,3};static_assert(noexcept(MillerRabin(17,c_bases)));require(MillerRabin(17,c_bases));
    nearest_typed<bool>();nearest_typed<char>();nearest_typed<wchar_t>();nearest_typed<char8_t>();nearest_typed<char16_t>();nearest_typed<char32_t>();typed<wchar_t>();typed<char8_t>();typed<char16_t>();typed<char32_t>();nearest_typed<signed char>();nearest_typed<unsigned char>();nearest_typed<short>();nearest_typed<unsigned short>();nearest_typed<int>();nearest_typed<unsigned>();nearest_typed<std::int64_t>();nearest_typed<std::uint64_t>();
    constexpr std::uint32_t bound=11000;std::vector<bool>sieve(bound+1,true);sieve[0]=sieve[1]=false;for(std::uint32_t d=2;d<=bound/d;++d)if(sieve[d])for(auto k=d*d;k<=bound;k+=d)sieve[k]=false;
    for(std::uint32_t n=0;n<=10000;++n){auto next=n;while(!sieve[next])++next;auto previous=n;while(previous>=2&&!sieve[previous])--previous;auto a=next_prime(n),b=prev_prime(n);require(a&&*a==next);require(bool(b)==(previous>=2));if(b)require(*b==previous);}
    for(std::uint64_t center:{(std::uint64_t{1}<<32),(std::uint64_t{1}<<62),(std::uint64_t{1}<<63),std::numeric_limits<std::uint64_t>::max()})for(std::uint64_t delta=0;delta<15;++delta){
        const auto n=center-delta;auto wanted_prev=n;while(wanted_prev>=2&&!independent_prime(wanted_prev))--wanted_prev;auto wanted_next=n;while(!independent_prime(wanted_next)&&wanted_next<std::numeric_limits<std::uint64_t>::max())++wanted_next;const auto a=next_prime(n),b=prev_prime(n);require(bool(a)==independent_prime(wanted_next));if(a)require(*a==wanted_next);require(b&&*b==wanted_prev);
    }
    const auto max=std::numeric_limits<std::uint64_t>::max();require(!next_prime(max));require(!next_prime(max-1));require(next_prime(max-58)==max-58);require(next_prime(max-59)==max-58);require(!next_prime(max-57));require(prev_prime(max)==max-58);require(!prev_prime(0));require(!prev_prime(1));require(next_prime(std::numeric_limits<std::int64_t>::min())==2);require(!prev_prime(std::numeric_limits<std::int64_t>::min()));require(next_prime(-1)==2);require(!prev_prime(-1));require(next_prime(false)==2);require(next_prime(true)==2);require(!prev_prime(true));
}
#endif
int main(){
 // 剰余表105通りを通常の剰余で照合し、64bit側の切替境界も調べる。
 for(unsigned r=0;r<105;++r) {
  const bool expected=r%3!=0 && r%5!=0 && r%7!=0;
  require(bool((reinsirk::detail::primality_coprime_105[r>>6]>>(r&63))&1u)==expected);
 }
 for(auto start:std::array<std::uint64_t,5>{(std::uint64_t{1}<<32)-128,4759123141ULL-128,(std::uint64_t{1}<<62)-128,(std::uint64_t{1}<<63)-128,std::numeric_limits<std::uint64_t>::max()-255})
  for(std::uint64_t delta=0;delta<256;++delta)inspect(start+delta);

 constexpr std::uint32_t bound=2000000;std::vector<bool> sieve(bound+1,true);sieve[0]=sieve[1]=false;for(std::uint32_t p=2;p<=bound/p;++p)if(sieve[p])for(auto k=p*p;k<=bound;k+=p)sieve[k]=false;for(std::uint32_t n=0;n<=bound;++n){require(is_prime(n)==sieve[n]);++sieve_cases;}
 typed<bool>();typed<char>();typed<wchar_t>();typed<char8_t>();typed<char16_t>();typed<char32_t>();typed<signed char>();typed<unsigned char>();typed<short>();typed<unsigned short>();typed<int>();typed<unsigned>();typed<std::int64_t>();typed<std::uint64_t>();
 for(auto n:std::array<std::uint64_t,35>{0,1,2,3,4,341,561,1105,1729,2465,2821,6601,2047,1373653,25326001,3215031751ULL,2152302898747ULL,3474749660383ULL,341550071728321ULL,3825123056546413051ULL,4294967291ULL,18446744073709551557ULL,18446744073709551533ULL,2305843009213693951ULL,18446744073709551615ULL,167772161,469762049,754974721,998244353,1000000007,1000000009,1004535809,1224736769,2013265921,4759123141ULL})inspect(n);
 const std::array<std::uint64_t,9> boundaries{std::uint64_t{1}<<31,std::uint64_t{1}<<32,4759123141ULL,std::uint64_t{1}<<62,std::uint64_t{1}<<63,std::numeric_limits<std::uint64_t>::max(),std::uint64_t{1}<<16,std::uint64_t{1}<<33,std::uint64_t{1}<<61};for(auto center:boundaries)for(std::uint64_t d=0;d<100;++d){if(center>=d)inspect(center-d);if(center<=std::numeric_limits<std::uint64_t>::max()-d)inspect(center+d);}
 std::mt19937_64 random(6102227);for(unsigned i=0;i<5000;++i)inspect(random());for(unsigned i=0;i<1000;++i)inspect(static_cast<std::uint32_t>(random()));
 // 素数の二乗と積は、Miller-Rabinの底選択によらず合成数。
 for(std::uint64_t p:std::array<std::uint64_t,13>{13,61,97,65537,99991,1000003,10000019,99999989,1000000007,2147483647,4294967291ULL,4294967279ULL,4294967231ULL}){require(independent_prime(p));require(!is_prime(p*p));inspect(p*p);}
 custom(std::array<std::uint64_t,0>{});custom(std::vector<int>{2,3,7,61});custom(std::array<std::int16_t,2>{std::numeric_limits<std::int16_t>::min(),std::numeric_limits<std::int16_t>::max()});custom(std::array<std::int32_t,2>{std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()});custom(std::array<std::uint16_t,2>{0,std::numeric_limits<std::uint16_t>::max()});custom(std::array<std::uint32_t,2>{0,std::numeric_limits<std::uint32_t>::max()});custom(std::array<std::uint64_t,9>{0,1,2,3,5,7,61,4759123141ULL,std::numeric_limits<std::uint64_t>::max()});custom(std::array<std::int64_t,7>{0,-1,-2,-7,-61,std::numeric_limits<std::int64_t>::min(),std::numeric_limits<std::int64_t>::max()});custom(std::array<signed char,4>{-128,-3,2,127});custom(std::array<unsigned char,3>{0,2,255});custom(std::array<bool,2>{false,true});custom(BasesRange{{0,-1,-2,-3,-7,-61,std::numeric_limits<std::int64_t>::min(),std::numeric_limits<std::int64_t>::max()}});
 require(MillerRabin(2047,{2}));require(!MillerRabin(2047,{2,3}));require(!is_prime(2047));require(MillerRabin(3,{0,3,6}));require(MillerRabin(9,std::array<int,0>{}));require(!MillerRabin(4,std::array<int,0>{}));
 #ifdef SIRKIT_PRIMALITY_REQUIRE_EXTRA
 extras();
 #endif
 std::printf("PASS checks=%zu sieve=%zu wide=%zu bases=%zu\n",checks,sieve_cases,wide_cases,base_cases);
}
