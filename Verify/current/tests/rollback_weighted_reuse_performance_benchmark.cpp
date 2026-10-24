// SPDX-License-Identifier: CC0-1.0
// 再測定用。各反復の結果を集計し、計測外で状態を確認する。
#ifndef SIRKIT_RWUF_SOURCE
#define SIRKIT_RWUF_SOURCE "../../../DataStructure/rollback-union-findw.cpp"
#endif
#include SIRKIT_RWUF_SOURCE
#include <atcoder/modint>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
struct Ring{std::uint64_t x;explicit Ring(std::int64_t v=0):x(static_cast<std::uint64_t>(v)){}friend Ring operator+(Ring a,Ring b){a.x+=b.x;return a;}friend Ring operator-(Ring a,Ring b){a.x-=b.x;return a;}friend bool operator==(Ring,Ring)=default;};
#if SIRKIT_VALUE_TYPE==0
using Value=atcoder::modint998244353;
#elif SIRKIT_VALUE_TYPE==1
using Value=std::int64_t;
#else
using Value=Ring;
#endif
int main(int argc,char**argv){if(argc!=3)return 2;const int n=std::atoi(argv[1]),repeats=std::atoi(argv[2]);RollbackWeightedUnionFind<Value>uf(n);uf.reserve_history(100);std::uint64_t sum=0;const auto start=std::chrono::steady_clock::now();for(int r=0;r<repeats;++r){uf.reset(n);sum+=uf.component_count()+uf.get_state()+uf.size(n-1);}const auto stop=std::chrono::steady_clock::now();std::printf("%.6f %ju\n",std::chrono::duration<double,std::milli>(stop-start).count(),static_cast<std::uintmax_t>(sum));}
