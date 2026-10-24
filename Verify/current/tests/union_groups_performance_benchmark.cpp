// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_UNION_BENCH_HEADER
#ifdef SIRKIT_BENCH_WEIGHTED
#define SIRKIT_UNION_BENCH_HEADER "../../../DataStructure/union_find_w.cpp"
#else
#define SIRKIT_UNION_BENCH_HEADER "../../../DataStructure/union_find.cpp"
#endif
#endif
#include SIRKIT_UNION_BENCH_HEADER
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#ifdef SIRKIT_BENCH_WEIGHTED
using UF=UnionFindW<std::int64_t>;
#else
using UF=UnionFind;
#endif
int main(int argc,char**argv){if(argc!=4)return 2;const int n=std::atoi(argv[1]),components=std::atoi(argv[2]),repeats=std::atoi(argv[3]);UF uf(n);
 for(int stride=1;stride<n/components;stride*=2)for(int i=0;i+stride<n;i+=stride*2){
#ifdef SIRKIT_BENCH_WEIGHTED
 uf.unite(i,i+stride,stride);
#else
 uf.unite(i,i+stride);
#endif
 }
 std::uint64_t checksum=0;auto start=std::chrono::steady_clock::now();for(int r=0;r<repeats;++r){auto groups=uf.groups();checksum+=groups.size();for(const auto&g:groups)checksum+=g.size()+g.front()+g.back();}auto stop=std::chrono::steady_clock::now();std::printf("%.6f %ju\n",std::chrono::duration<double,std::milli>(stop-start).count(),static_cast<std::uintmax_t>(checksum));}
