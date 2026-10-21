// SPDX-License-Identifier: CC0-1.0
#ifndef FLOYD_SOURCE
#define FLOYD_SOURCE "../../../Graph/Floyd-Warshall.cpp"
#endif
#include FLOYD_SOURCE
#include <cassert>
#include <iostream>
#include <random>
#include <queue>
using Matrix=std::vector<std::vector<std::int64_t>>;
struct Answer { Matrix d; bool negative=false; bool overflow=false; };
Answer reference(Matrix a) {
  const int n=a.size(); Answer ans{Matrix(n,std::vector<std::int64_t>(n,INF))};
  for(int s=0;s<n;++s) {
    const __int128_t big=__int128_t(1)<<115; std::vector<__int128_t>d(n,big); d[s]=0;
    for(int pass=1;pass<n;++pass) { bool changed=false;
      for(int u=0;u<n;++u) if(d[u]!=big) for(int v=0;v<n;++v) if(a[u][v]!=INF && d[v]>d[u]+a[u][v]) {d[v]=d[u]+a[u][v];changed=true;}
      if(!changed)break;
    }
    std::vector<bool> neg(n); std::queue<int>q;
    for(int u=0;u<n;++u) if(d[u]!=big)for(int v=0;v<n;++v)if(a[u][v]!=INF && d[v]>d[u]+a[u][v]&&!neg[v]){neg[v]=true;q.push(v);}
    while(!q.empty()){int u=q.front();q.pop();for(int v=0;v<n;++v)if(a[u][v]!=INF&&!neg[v]){neg[v]=true;q.push(v);}}
    for(int v=0;v<n;++v) {
      if(neg[v]){ans.d[s][v]=-INF;ans.negative=true;}
      else if(d[v]==big)ans.d[s][v]=INF;
      else if(d[v]<=-INF||d[v]>=INF)ans.overflow=true;
      else ans.d[s][v]=static_cast<std::int64_t>(d[v]);
    }
  } return ans;
}
void check(Matrix input){
 auto ref=reference(input);auto a=input,b=input;std::vector<std::vector<int>>next;
 bool overflow=false,negative=false;try{negative=FloydWarshall(a,next);}catch(const std::overflow_error&){overflow=true;}
 if(overflow!=ref.overflow){std::cerr<<"overflow mismatch n="<<input.size()<<"\n";std::abort();}
 if(overflow)return;
 assert(negative==ref.negative);if(a!=ref.d){std::cerr<<"distance mismatch\n";std::abort();}
 assert(FloydWarshall(b)==negative);assert(a==b);
 for(int u=0;u<int(a.size());++u)for(int v=0;v<int(a.size());++v){
  auto path=FloydWarshallPath(next,u,v);
  if(a[u][v]==INF||a[u][v]==-INF){assert(path.empty());continue;}
  assert(!path.empty()&&path.front()==u&&path.back()==v);__int128_t cost=0;
  for(std::size_t i=1;i<path.size();++i){assert(input[path[i-1]][path[i]]!=INF);cost+=input[path[i-1]][path[i]];}
  assert(cost==a[u][v]);
 }
}
int main(){
 check({});check({{INF}});check({{0}});check({{-1}});check({{INF,INF},{INF,INF}});check({{0,0},{0,0}});
 // 大きな有限距離と負辺による相殺、全組が範囲内になる頂点順序の例。
 check({{0, INF-1, INF-2},{INF,0,-INF+2},{INF,INF,0}});
 // 10本以上の大きな正辺/負辺、広幅経路と負閉路。
 Matrix large(24,std::vector<std::int64_t>(24,INF));for(int i=0;i<24;++i)large[i][i]=0;
 for(int i=0;i<23;++i){large[i][i+1]=INF-1;}
 check(large);
 for(int i=0;i<23;++i){large[i][i+1]=-INF+1;}
 check(large);large[23][0]=1;check(large);
 // 途中のINF超えを保持し、後から現れる経路で全有限距離が範囲内へ戻る。
 Matrix cancellation(7,std::vector<std::int64_t>(7,INF));
 for(int i=0;i<7;++i)cancellation[i][i]=0;
 cancellation[0][1]=3*(INF/4);cancellation[1][2]=3*(INF/4);
 cancellation[0][3]=INF/4;cancellation[3][2]=INF/4;cancellation[2][4]=-3*(INF/4);
 check(cancellation);
 std::mt19937_64 rng(123984);
 for(int trial=0;trial<30000;++trial){
  const int n=rng()%17;
  Matrix a(n,std::vector<std::int64_t>(n,INF));
  for(int i=0;i<n;++i){
   for(int j=0;j<n;++j){
    if(rng()%5<2){
     auto w=std::int64_t(rng()%101)-50;
     if(trial%3==0)w=(std::int64_t(rng()%7)-3)*(INF/4);
     a[i][j]=w;
    }
   }
  }
  check(a);
 }
 // SIMD境界をまたぎ、疎・密・非負・負閉路・DAG・広幅演算を確認する。
 for(int n:{31,32,33,35,63,64,65})for(int kind=0;kind<6;++kind)for(int trial=0;trial<3;++trial){
  Matrix a(n,std::vector<std::int64_t>(n,INF));
  for(int i=0;i<n;++i){a[i][i]=0;for(int j=0;j<n;++j)if(i!=j&&(kind!=1||rng()%10==0)){
   auto w=static_cast<std::int64_t>(rng()%101);
   if(kind==2)w-=30;
   if(kind==3)w+=static_cast<std::int64_t>(j-i)*100;
   if(kind==4)w=(static_cast<std::int64_t>(rng()%7)-3)*(INF/4);
   if(kind==5&&j<i)continue;
   a[i][j]=w;
  }}
  check(a);
 }
#ifndef SKIP_RUNTIME_CHECKS
 bool threw=false;try{Matrix a{{0,1},{2}};FloydWarshall(a);}catch(const std::invalid_argument&){threw=true;}assert(threw);
 threw=false;try{Matrix a{{-INF}};FloydWarshall(a);}catch(const std::out_of_range&){threw=true;}assert(threw);
 threw=false;try{FloydWarshallPath({{0}},1,0);}catch(const std::out_of_range&){threw=true;}assert(threw);
#endif
 bool cyclic_threw=false;try{FloydWarshallPath({{0,0},{1,1}},0,1);}catch(const std::invalid_argument&){cyclic_threw=true;}assert(cyclic_threw);
 std::cout<<"30126 randomized graphs + boundaries passed\n";
}
