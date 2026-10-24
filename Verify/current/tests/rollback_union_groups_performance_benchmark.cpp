// SPDX-License-Identifier: CC0-1.0
// 再測定用。各反復の結果を集計し、計測外で状態を確認する。
#ifndef SIRKIT_RUF_SOURCE
#define SIRKIT_RUF_SOURCE "../../../DataStructure/rollback-union-find.cpp"
#endif
#include SIRKIT_RUF_SOURCE
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#ifdef SIRKIT_WEIGHTED_BENCHMARK
using UF=RollbackWeightedUnionFind<std::int64_t>;
#else
using UF=RollbackUnionFind;
#endif
int main(int argc,char**argv){if(argc!=4)return 2;const int n=std::atoi(argv[1]),components=std::atoi(argv[2]),repeats=std::atoi(argv[3]);UF uf(n);for(int stride=1;stride<n/components;stride*=2)for(int i=0;i+stride<n;i+=stride*2){
#ifdef SIRKIT_WEIGHTED_BENCHMARK
uf.unite(i,i+stride,stride);
#else
uf.unite(i,i+stride);
#endif
}std::uint64_t sum=0;const auto start=std::chrono::steady_clock::now();for(int r=0;r<repeats;++r){auto groups=uf.groups();sum+=groups.size();for(const auto&g:groups)sum+=g.size()+g.front()+g.back();}const auto stop=std::chrono::steady_clock::now();std::printf("%.6f %ju\n",std::chrono::duration<double,std::milli>(stop-start).count(),static_cast<std::uintmax_t>(sum));}
