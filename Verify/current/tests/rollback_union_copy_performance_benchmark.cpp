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
int main(int argc,char**argv){if(argc!=4)return 2;const int n=std::atoi(argv[1]),extra=std::atoi(argv[2]),repeats=std::atoi(argv[3]);RollbackUnionFind source(n),target(n);for(int i=1;i<n;++i)source.unite(i-1,i);source.snapshot();for(int i=0;i<extra;++i)source.unite(0,n-1);target=source;std::uint64_t sum=0;const auto start=std::chrono::steady_clock::now();for(int i=0;i<repeats;++i){target=source;sum+=target.get_state()+target.size(0);}const auto stop=std::chrono::steady_clock::now();target.rollback();if(target.get_state()!=n-1||source.get_state()!=n-1+extra)return 3;std::printf("%.6f %ju\n",std::chrono::duration<double,std::milli>(stop-start).count(),static_cast<std::uintmax_t>(sum));}
