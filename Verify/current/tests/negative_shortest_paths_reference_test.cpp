// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
#ifndef SIRKIT_TEST_NEGATIVE_PATH_SOURCE
#define SIRKIT_TEST_NEGATIVE_PATH_SOURCE "../../../Graph/Bellman-Ford.cpp"
#endif
#ifndef SIRKIT_TEST_NEGATIVE_PATH_API
#define SIRKIT_TEST_NEGATIVE_PATH_API BellmanFord
#endif
#include SIRKIT_TEST_NEGATIVE_PATH_SOURCE
static std::uint64_t checks=0,cases=0;
static void require(bool b){++checks;if(!b)throw std::runtime_error("negative path reference mismatch");}
// Floyd-Warshallと到達関係による独立参照。
static std::vector<std::int64_t> reference(const WGraph&g,int s){int n=static_cast<int>(g.size());std::vector<std::vector<std::int64_t>> d(n,std::vector<std::int64_t>(n,INF));for(int i=0;i<n;++i){d[i][i]=0;for(auto e:g[i])d[i][e.to]=std::min(d[i][e.to],e.w);}for(int k=0;k<n;++k)for(int i=0;i<n;++i)if(d[i][k]!=INF)for(int j=0;j<n;++j)if(d[k][j]!=INF)d[i][j]=std::min(d[i][j],std::max(-INF/4,d[i][k]+d[k][j]));auto result=d[s];for(int k=0;k<n;++k)if(d[s][k]!=INF&&d[k][k]<0)for(int j=0;j<n;++j)if(d[k][j]!=INF)result[j]=-INF;return result;}
static void test(const WGraph&g,int s,const std::vector<std::int64_t>*known=nullptr){++cases;auto expected=known?*known:reference(g,s);std::vector<std::int64_t>d{123,456};bool negative=SIRKIT_TEST_NEGATIVE_PATH_API(g,d,s);require(d==expected);require(negative==(std::find(expected.begin(),expected.end(),-INF)!=expected.end()));
#if defined(SIRKIT_TEST_NEGATIVE_PATH_KIND)
 if(g.size()<=100){
#if SIRKIT_TEST_NEGATIVE_PATH_KIND == 1
  for(bool bounded:{false,true}){d={999};bool neg=bounded?reinsirk::detail::BellmanFordImpl<true,true>(g,d,s):reinsirk::detail::BellmanFordImpl<true,false>(g,d,s);require(d==expected);require(neg==negative);}
#else
  d={999};bool neg=reinsirk::detail::SPFARun<false,false>(g,d,s);require(d==expected);require(neg==negative);
  if(g.size()<=64){d={999};neg=reinsirk::detail::SPFARun<true,false>(g,d,s);require(d==expected);require(neg==negative);}
#endif
 }
#endif
}
int main(){try{
 std::mt19937 rng(0x91B122);for(int trial=0;trial<5000;++trial){int n=1+static_cast<int>(rng()%25);WGraph g(n);int m=static_cast<int>(rng()%(n*5+1));for(int e=0;e<m;++e){std::int64_t w=static_cast<int>(rng()%21)-10;if(trial%5==0)w*=100000000;g[rng()%n].emplace_back(static_cast<int>(rng()%n),w);}test(g,static_cast<int>(rng()%n));}
 // 65頂点以上の前進経路と、途中停止後の通常処理への引継ぎ。
 for(int trial=0;trial<400;++trial){int n=65+static_cast<int>(rng()%25),start=trial%2?0:n/3;WGraph g(n);g[start].emplace_back(n-1,-1);for(int e=0;e<6*n;++e){int u=static_cast<int>(rng()%n),v=static_cast<int>(rng()%n);if(u!=start)g[u].emplace_back(v,static_cast<int>(rng()%7)-3);}test(g,start);}
 for(int n:{1,2,63,64,65,127,4095,4096,4097,20000}){
  WGraph isolated(n);std::vector<std::int64_t>want(n,INF);want[0]=0;test(isolated,0,&want);if(n==1)continue;
  WGraph forward(n),reverse(n);for(int i=1;i<n;++i){forward[i-1].emplace_back(i,-1000000000);reverse[i].emplace_back(i-1,-1000000000);}for(int i=0;i<n;++i)want[i]=-std::int64_t{i}*1000000000;test(forward,0,&want);for(int i=0;i<n;++i)want[i]=-std::int64_t{n-1-i}*1000000000;test(reverse,n-1,&want);
  // 負閉路とその下流だけを-INFへ変換する。
  WGraph cycle(n);for(int i=1;i<n-1;++i)cycle[i-1].emplace_back(i,1);int seed=(n-2)/2;cycle[seed].emplace_back(seed,-1);for(int i=0;i<n;++i)want[i]=i<seed?i:(i<n-1?-INF:INF);test(cycle,0,&want);
 }
#ifndef SKIP_RUNTIME_CHECKS
 for(int n:{0,1,64,65,4096}){
  WGraph g(n);for(int start:{-1,n}){std::vector<std::int64_t>d{321,654};bool threw=false;try{(void)SIRKIT_TEST_NEGATIVE_PATH_API(g,d,start);}catch(const std::out_of_range&){threw=true;}require(threw&&d==std::vector<std::int64_t>({321,654}));}
  if(n==0)continue;
  for(int kind=0;kind<4;++kind){WGraph bad(n);if(n>1)bad[0].emplace_back(n-1,0);bad[n-1].emplace_back(kind==0?-1:(kind==1?n:0),kind==2?-1000000001:(kind==3?1000000001:0));std::vector<std::int64_t>d{321,654};bool threw=false;try{(void)SIRKIT_TEST_NEGATIVE_PATH_API(bad,d,0);}catch(const std::out_of_range&){threw=true;}require(threw&&d==std::vector<std::int64_t>({321,654}));}
  if(n>1){WGraph ignored(n);ignored[n-1].emplace_back(n,1000000001);std::vector<std::int64_t>want(n,INF);want[0]=0;test(ignored,0,&want);}
 }
#endif
 std::cout<<"{\"cases\":"<<cases<<",\"checks\":"<<checks<<"}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
