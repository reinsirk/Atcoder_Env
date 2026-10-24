// SPDX-License-Identifier: CC0-1.0
// groups単独と、平坦化後のfindを分けて計測する。
#ifndef SIRKIT_DUF_SOURCE
#define SIRKIT_DUF_SOURCE "../../../DataStructure/dynamic-union-find.cpp"
#endif
#include SIRKIT_DUF_SOURCE
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
static int key(int i) { return i * 7919 - 900000000; }
static DynamicUnionFind prepared(int n, int components, int reverse) {
    DynamicUnionFind uf(n);
    for (int i = 0; i < n; ++i) uf.add(key(i));
    const auto index = [=](int i) { return reverse ? n - 1 - i : i; };
    for (int stride = 1; stride < n / components; stride *= 2)
        for (int i = 0; i + stride < n; i += stride * 2)
            uf.unite(key(index(i)), key(index(i + stride)));
    return uf;
}
int main(int argc, char** argv) {
    if (argc != 6) return 2;
    const int n=std::atoi(argv[1]), components=std::atoi(argv[2]), repeats=std::atoi(argv[3]), mode=std::atoi(argv[4]), reverse=std::atoi(argv[5]);
    if(n<1||n>131072||components<1||components>n||repeats<1||mode<0||mode>1||reverse<0||reverse>1)return 2;
    auto source=prepared(n,components,reverse); std::uint64_t checksum=0; double group_ms=0,query_ms=0;
    if(mode==0){const auto start=std::chrono::steady_clock::now();for(int r=0;r<repeats;++r){auto groups=source.groups();checksum+=groups.size();for(const auto&g:groups)checksum+=g.size()+static_cast<std::uint32_t>(g.front())+static_cast<std::uint32_t>(g.back());}const auto stop=std::chrono::steady_clock::now();group_ms=std::chrono::duration<double,std::milli>(stop-start).count();}
    else for(int r=0;r<repeats;++r){auto uf=source;const auto start=std::chrono::steady_clock::now();auto groups=uf.groups();const auto middle=std::chrono::steady_clock::now();checksum+=groups.size();for(const auto&g:groups)checksum+=g.size()+static_cast<std::uint32_t>(g.front())+static_cast<std::uint32_t>(g.back());const auto query_start=std::chrono::steady_clock::now();for(int q=0;q<2;++q)for(int i=0;i<n;++i)checksum+=static_cast<std::uint32_t>(uf.find(key(i)));const auto stop=std::chrono::steady_clock::now();group_ms+=std::chrono::duration<double,std::milli>(middle-start).count();query_ms+=std::chrono::duration<double,std::milli>(stop-query_start).count();}
    std::printf("%.6f %.6f %ju\n",group_ms,query_ms,static_cast<std::uintmax_t>(checksum));
}
