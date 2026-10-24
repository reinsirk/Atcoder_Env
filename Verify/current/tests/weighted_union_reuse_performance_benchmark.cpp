// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_WUF_SOURCE
#define SIRKIT_WUF_SOURCE "../../../DataStructure/union_find_w.cpp"
#endif
#include SIRKIT_WUF_SOURCE
#include <cstdint>
namespace reinsirk::detail {
struct BenchmarkMod {
 std::uint32_t v=0;
 BenchmarkMod()=default;
 BenchmarkMod(std::int64_t x) { x%=998244353; if(x<0)x+=998244353;v=static_cast<std::uint32_t>(x); }
 BenchmarkMod& operator+=(BenchmarkMod x){v+=x.v;if(v>=998244353)v-=998244353;return *this;}
 friend BenchmarkMod operator+(BenchmarkMod a,BenchmarkMod b){return a+=b;}
 friend BenchmarkMod operator-(BenchmarkMod a,BenchmarkMod b){return BenchmarkMod(static_cast<std::int64_t>(a.v)-b.v);}
 friend BenchmarkMod operator-(BenchmarkMod a){return BenchmarkMod(-static_cast<std::int64_t>(a.v));}
 friend bool operator==(BenchmarkMod,BenchmarkMod)=default;
};
struct BenchmarkRing {
 std::int64_t v=0;
 BenchmarkRing()=default;BenchmarkRing(std::int64_t x):v(x){}
 BenchmarkRing& operator+=(BenchmarkRing b){v+=b.v;return *this;}
 friend BenchmarkRing operator+(BenchmarkRing a,BenchmarkRing b){return a+=b;}
 friend BenchmarkRing operator-(BenchmarkRing a,BenchmarkRing b){return BenchmarkRing(a.v-b.v);}
 friend BenchmarkRing operator-(BenchmarkRing a){return BenchmarkRing(-a.v);}
 friend bool operator==(BenchmarkRing,BenchmarkRing)=default;
};
template<class T>std::uint64_t checksum_value(const T&x){if constexpr(requires{x.v;})return static_cast<std::uint64_t>(x.v);else return static_cast<std::uint64_t>(x);}
}

#ifndef SIRKIT_WUF_BENCH_VALUE
#define SIRKIT_WUF_BENCH_VALUE std::int64_t
#endif

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
static const void* volatile escaped;
int main(int argc,char**argv){
 if(argc!=4)return 2;
 const int n=std::atoi(argv[1]),repeats=std::atoi(argv[3]);const std::string mode=argv[2];using T=SIRKIT_WUF_BENCH_VALUE;
 UnionFindW<T> target(n),source(n);
 for(int i=0;i+1<n;i+=2)source.merge(i,i+1,T(7));
 std::uint64_t checksum=0;
 const auto start=std::chrono::steady_clock::now();
 for(int r=0;r<repeats;++r){if(mode=="reset")target.reset(static_cast<std::size_t>(n));else if(mode=="copy")target=source;else return 2;escaped=&target;checksum+=static_cast<std::uint64_t>(target.component_count());if(n)checksum+=reinsirk::detail::checksum_value(target.weight(n-1))+target.size(0);}
 const auto stop=std::chrono::steady_clock::now();std::printf("%.6f %ju\n",std::chrono::duration<double,std::milli>(stop-start).count(),static_cast<std::uintmax_t>(checksum));
}
