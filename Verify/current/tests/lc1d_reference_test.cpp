// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LC1D_REQUIRE_EXTRA
#define SIRKIT_LC1D_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_LC1D_SOURCE
#define SIRKIT_LC1D_SOURCE "../../../DataStructure/line-container.cpp"
#endif
#include SIRKIT_LC1D_SOURCE
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>
using Big=boost::multiprecision::cpp_int;
using High=boost::multiprecision::number<boost::multiprecision::cpp_bin_float<256>>;
static std::uint64_t checks=0,queries=0,overflow_queries=0;
static void require_impl(bool ok,const char*expression,int line){++checks;if(!ok){std::fprintf(stderr,"require failed line %d after %ju checks: %s\n",line,static_cast<std::uintmax_t>(checks),expression);std::abort();}}
#define require(ok) require_impl(bool(ok),#ok,__LINE__)
template<class Exception,class F>static void throws(F&&f){bool caught=false;try{f();}catch(const Exception&){caught=true;}require(caught);}
template<class T>static Big big(T x){return Big(static_cast<std::int64_t>(x));}
template<class T>static T sample(std::mt19937_64&rng){
    using U=std::make_unsigned_t<T>;
    const U bits=static_cast<U>(rng());
    if(bits<=static_cast<U>(std::numeric_limits<T>::max()))return static_cast<T>(bits);
    return static_cast<T>(-T{1}-static_cast<T>(static_cast<U>(~bits)));
}
template<class T>static bool contains(const std::vector<std::pair<T,T>>&lines,T k,T m){return std::find(lines.begin(),lines.end(),std::pair<T,T>{k,m})!=lines.end();}
template<class T,Objective O>static void inspect(const LineContainer<T,O>&h,const std::vector<std::pair<T,T>>&lines,T x){
    ++queries;require(!lines.empty());Big best=big(lines.front().first)*big(x)+big(lines.front().second);
    for(auto[k,m]:lines){const Big value=big(k)*big(x)+big(m);if constexpr(O==MINIMIZE){if(value<best)best=value;}else if(value>best)best=value;}
    const auto selected=h.query_line(x);require(contains(lines,selected.first,selected.second));require(big(selected.first)*big(x)+big(selected.second)==best);
    const auto optional_line=h.query_line_optional(x);require(optional_line.has_value());require(big(optional_line->first)*big(x)+big(optional_line->second)==best);
    const bool in_range=best>=big(std::numeric_limits<T>::min())&&best<=big(std::numeric_limits<T>::max());
    if(in_range){const T expected=best.convert_to<T>();require(h.query(x)==expected);require(h.query_optional(x)==expected);
        if constexpr(requires{h.query_with_line(x);h.query_with_line_optional(x);}){
            const auto result=h.query_with_line(x);require(result.value==expected);require(contains(lines,result.k,result.m));require(big(result.k)*big(x)+big(result.m)==best);
            const auto optional=h.query_with_line_optional(x);require(optional.has_value());require(optional->value==expected);require(contains(lines,optional->k,optional->m));require(big(optional->k)*big(x)+big(optional->m)==best);
        }
    }else{++overflow_queries;throws<std::overflow_error>([&]{(void)h.query(x);});throws<std::overflow_error>([&]{(void)h.query_optional(x);});
        if constexpr(requires{h.query_with_line(x);h.query_with_line_optional(x);}){throws<std::overflow_error>([&]{(void)h.query_with_line(x);});throws<std::overflow_error>([&]{(void)h.query_with_line_optional(x);});}
    }
}
template<class T,Objective O>static void empty_checks(const LineContainer<T,O>&h){require(h.empty());require(h.size()==0);require(!h.query_optional(0));require(!h.query_line_optional(0));
    if constexpr(requires{h.query_with_line_optional(T{});})require(!h.query_with_line_optional(0));
#ifndef SKIP_RUNTIME_CHECKS
    throws<std::out_of_range>([&]{(void)h.query(0);});throws<std::out_of_range>([&]{(void)h.query_line(0);});
    if constexpr(requires{h.query_with_line(T{});})throws<std::out_of_range>([&]{(void)h.query_with_line(0);});
#endif
}
template<class T,Objective O>static void sequence(const std::vector<std::pair<T,T>>&input,const std::vector<T>&xs){
    LineContainer<T,O>h;std::vector<std::pair<T,T>>seen;empty_checks(h);
    for(auto[k,m]:input){h.add(k,m);seen.emplace_back(k,m);require(!h.empty());require(h.size()<=seen.size());
        for(T x:xs)inspect(h,seen,x);
        for(const auto&line:h)require(contains(seen,line.k,line.m));
        if(h.size()>1){auto previous=h.begin();for(auto it=std::next(previous);it!=h.end();++it){if constexpr(O==MINIMIZE)require(previous->k>it->k);else require(previous->k<it->k);require(previous->p<it->p);previous=it;}}
        require(std::prev(h.end())->p==lc_inf<T>());
    }
    auto copy=h;for(T x:xs)inspect(copy,seen,x);
    auto moved=std::move(copy);for(T x:xs)inspect(moved,seen,x);copy.clear();copy.add(0,3);require(copy.query(0)==3);
    LineContainer<T,O>assigned;assigned.add(1,-2);assigned=h;for(T x:xs)inspect(assigned,seen,x);
    auto*self=&assigned;assigned=*self;for(T x:xs)inspect(assigned,seen,x);
    auto*move_self=&moved;moved=std::move(*move_self);moved.clear();moved.add(0,-4);require(moved.query(0)==-4);
    assigned.swap(moved);require(assigned.query(0)==-4);for(T x:xs)inspect(moved,seen,x);
    h.clear();empty_checks(h);for(T x:xs)inspect(moved,seen,x);
}
template<class T,Objective O>static void integer_suite(){
    constexpr T lo=std::numeric_limits<T>::min(),hi=std::numeric_limits<T>::max();std::mt19937_64 rng(719621+sizeof(T)*19+O);
    const std::vector<T>ends={lo,static_cast<T>(lo+1),static_cast<T>(lo+2),static_cast<T>(lo/2),T{-2},T{-1},T{0},T{1},T{2},static_cast<T>(hi/2),static_cast<T>(hi-2),static_cast<T>(hi-1),hi};
    const std::vector<std::vector<std::pair<T,T>>>special={
        {{0,0},{0,0},{0,hi},{0,lo},{1,0},{-1,0}},
        {{1,hi},{0,lo},{-1,hi}},{{0,lo},{1,0},{2,hi}},
        {{hi,hi},{lo,lo},{hi,lo},{lo,hi},{0,0},{1,-1},{-1,1}},
        {{0,lo},{1,0}},{{0,lo},{1,-1}},{{0,hi},{1,-1}},
        {{hi,lo}},{{lo,hi}},{{hi,hi}},{{lo,lo}},
        {{2,-1},{-2,-1},{0,-1}},{{1,0},{3,1},{-2,4},{1,-5},{1,5}}
    };
    for(auto lines:special){sequence<T,O>(lines,ends);std::reverse(lines.begin(),lines.end());sequence<T,O>(lines,ends);}
    for(int round=0;round<12;++round){std::vector<std::pair<T,T>>lines;std::vector<T>xs=ends;
        for(int i=0;i<10;++i)xs.push_back(sample<T>(rng));
        for(int i=0;i<40;++i){T k=sample<T>(rng),m=sample<T>(rng);if(i%7==0)k=0;if(i%11==0)m=lo;if(i%13==0)m=hi;lines.emplace_back(k,m);}
        sequence<T,O>(lines,xs);
    }
    if constexpr(sizeof(T)==1){std::vector<T>xs;for(int x=-128;x<=127;++x)xs.push_back(static_cast<T>(x));sequence<T,O>({{lo,hi},{hi,lo},{0,lo},{-1,hi},{1,-1},{-1,0},{0,hi}},xs);}
    for(T a:ends)for(T b:ends){if(b==0){throws<std::domain_error>([&]{(void)lc_div<T>(a,b);});continue;}if(a==lo&&b==-1){throws<std::overflow_error>([&]{(void)lc_div<T>(a,b);});continue;}Big q=big(a)/big(b);const Big rem=big(a)%big(b);if((a<0)!=(b<0)&&rem!=0)--q;require(big(lc_div<T>(a,b))==q);}
    require(lc_inf<T>()==hi);
}
// 早期棄却の本数境界と、大包絡から小包絡へ戻った後の追加を確認する。
template<class T,Objective O>static void threshold_suite(){
    LineContainer<T,O> h;std::vector<std::pair<T,T>>lines;
    auto append=[&](T k,T m){h.add(k,m);lines.emplace_back(k,m);};
    const std::vector<T>xs={std::numeric_limits<T>::min(),T{-2000},T{-701},T{-700},T{-3},T{-1},T{0},T{1},T{3},T{700},T{701},T{2000},std::numeric_limits<T>::max()};
    for(int i=-350;i<=350;++i){append(static_cast<T>(i),static_cast<T>((O==MINIMIZE?1:-1)*i*i));const auto n=lines.size();require(h.size()==n);if(n==254||n==255||n==256||n==257||n==258||n==511||n==701)for(T x:xs)inspect(h,lines,x);}
    append(0,O==MINIMIZE?T{-1000000000}:T{1000000000});require(h.size()==3);for(T x:xs)inspect(h,lines,x);
    for(int i=-340;i<=340;i+=4){append(static_cast<T>(i),static_cast<T>((O==MINIMIZE?1:-1)*i*i));require(h.size()==3);}
    append(350,O==MINIMIZE?std::numeric_limits<T>::max():std::numeric_limits<T>::min());
    append(-350,O==MINIMIZE?std::numeric_limits<T>::min():std::numeric_limits<T>::max());
    append(std::numeric_limits<T>::min(),0);append(std::numeric_limits<T>::max(),0);
    for(T x:xs)inspect(h,lines,x);
    auto copy=h;h.clear();for(T x:xs)inspect(copy,lines,x);
}
template<class T,Objective O>static void inspect_float(const LineContainer<T,O>&h,const std::vector<std::pair<T,T>>&lines,T x,bool exact){
    High best=High(lines.front().first)*High(x)+High(lines.front().second);for(auto[k,m]:lines){const High value=High(k)*High(x)+High(m);if constexpr(O==MINIMIZE){if(value<best)best=value;}else if(value>best)best=value;}
    const auto[k,m]=h.query_line(x);require(contains(lines,k,m));const High achieved=High(k)*High(x)+High(m);const High tolerance=exact?High{0}:(abs(best)+1)*High(std::numeric_limits<T>::epsilon())*256;require(abs(achieved-best)<=tolerance);
    const T value=h.query(x);require(std::isfinite(value));require(abs(High(value)-best)<=tolerance);require(h.query_optional(x).has_value());require(h.query_line_optional(x).has_value());
    if constexpr(requires{h.query_with_line(x);h.query_with_line_optional(x);}){const auto result=h.query_with_line(x);require(contains(lines,result.k,result.m));require(abs(High(result.value)-best)<=tolerance);require(result.value==result.k*x+result.m);require(h.query_with_line_optional(x).has_value());}
}
template<class T,Objective O>static void floating_suite(){
    LineContainer<T,O>h;empty_checks(h);std::vector<std::pair<T,T>>lines;std::mt19937_64 rng(9671+O+sizeof(T));
    for(int i=0;i<80;++i){const T k=T(static_cast<int>(rng()%129)-64)/4,m=T(static_cast<int>(rng()%257)-128)/16;lines.emplace_back(k,m);h.add(k,m);for(int j=-17;j<=17;++j)inspect_float(h,lines,T(j)/8,true);}
    const T large=std::numeric_limits<T>::max()*T{0.75};
    const std::vector<std::vector<std::pair<T,T>>>cases={{{large,-large},{-large,large}},{{large,0},{-large,1}},{{0,large},{1,-large}},{{large,large/T{2}},{large/T{2},-large/T{2}}}};
    for(const auto&input:cases){h.clear();for(const auto&[k,m]:input)h.add(k,m);for(T x:{T{0},T{0.125},T{-0.125},T{0.25},T{-0.25}})inspect_float(h,input,x,false);}
    h.clear();h.add(1,2);auto copy=h;auto moved=std::move(copy);require(moved.query(3)==5);copy.clear();copy.add(0,-2);require(copy.query(3)==-2);h.swap(copy);require(h.query(9)==-2&&copy.query(9)==11);
#ifndef SKIP_RUNTIME_CHECKS
    const T inf=std::numeric_limits<T>::infinity(),nan=std::numeric_limits<T>::quiet_NaN();
    for(T bad:{inf,-inf,nan}){throws<std::invalid_argument>([&]{h.add(bad,1);});throws<std::invalid_argument>([&]{h.add(1,bad);});throws<std::invalid_argument>([&]{(void)h.query(bad);});throws<std::invalid_argument>([&]{(void)h.query_line(bad);});throws<std::invalid_argument>([&]{(void)h.query_optional(bad);});throws<std::invalid_argument>([&]{(void)h.query_line_optional(bad);});
        if constexpr(requires{h.query_with_line(bad);}){throws<std::invalid_argument>([&]{(void)h.query_with_line(bad);});throws<std::invalid_argument>([&]{(void)h.query_with_line_optional(bad);});}}
    h.clear();throws<std::invalid_argument>([&]{(void)h.query_optional(nan);});throws<std::invalid_argument>([&]{(void)h.query_line_optional(inf);});if constexpr(requires{h.query_with_line_optional(nan);})throws<std::invalid_argument>([&]{(void)h.query_with_line_optional(nan);});
#endif
    require(std::isinf(lc_inf<T>()));throws<std::domain_error>([]{(void)lc_div<T>(1,0);});require(lc_div<T>(T{-5},T{2})==T{-2.5});
}
#ifdef SIRKIT_LC1D_REQUIRE_EXTRA
static_assert(requires(const MinLineContainer<std::int64_t>&h){h.query_with_line(0);h.query_with_line_optional(0);});
static_assert(std::is_same_v<decltype(std::declval<MinLineContainer<std::int64_t>>().query_with_line(0).value),std::int64_t>);
#endif
int main(){integer_suite<std::int8_t,MAXIMIZE>();integer_suite<std::int8_t,MINIMIZE>();integer_suite<std::int16_t,MAXIMIZE>();integer_suite<std::int16_t,MINIMIZE>();integer_suite<std::int32_t,MAXIMIZE>();integer_suite<std::int32_t,MINIMIZE>();integer_suite<std::int64_t,MAXIMIZE>();integer_suite<std::int64_t,MINIMIZE>();threshold_suite<std::int32_t,MAXIMIZE>();threshold_suite<std::int32_t,MINIMIZE>();threshold_suite<std::int64_t,MAXIMIZE>();threshold_suite<std::int64_t,MINIMIZE>();floating_suite<float,MAXIMIZE>();floating_suite<float,MINIMIZE>();floating_suite<double,MAXIMIZE>();floating_suite<double,MINIMIZE>();floating_suite<long double,MAXIMIZE>();floating_suite<long double,MINIMIZE>();std::printf("PASS %ju checks; %ju exact integer queries; %ju out-of-range queries; ld_digits=%d\n",static_cast<std::uintmax_t>(checks),static_cast<std::uintmax_t>(queries),static_cast<std::uintmax_t>(overflow_queries),std::numeric_limits<long double>::digits);}
