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
int main() {
  // 単純路の有限距離と、ちょうどINF/-INFになる距離を区別する。
  for (std::int64_t last : {std::int64_t{0},std::int64_t{1},std::int64_t{2}}) {
    check({{0,INF-2,INF},{INF,0,last},{INF,INF,0}});
    check({{0,-INF+2,INF},{INF,0,-last},{INF,INF,0}});
  }
  for (int n : {2,7,8,9,15,16,17,31,32,33,63,64,65}) {
    Matrix a(n,std::vector<std::int64_t>(n,INF));
    for(int i=0;i<n;++i) a[i][i]=0;
    for(int i=0;i+1<n;++i) a[i][i+1]=0;
    check(a);
    for(int i=0;i<n;++i) for(int j=0;j<n;++j) {
      if(i!=j) a[i][j]=1+((i+j)%7)+std::int64_t(j-i)*11;
    }
    check(a);
    a.assign(n,std::vector<std::int64_t>(n,INF));
    for(int i=0;i<n;++i) a[i][i]=0;
    a[0][1]=-3;a[1][0]=2;
    if(n>3){a[2][0]=4;a[1][3]=5;}
    if(n>6)a[5][6]=-3;
    check(a);
  }
#ifndef SKIP_RUNTIME_CHECKS
  auto invalid = [](auto function,auto tag) {
    bool caught=false;
    try{function();}catch(const decltype(tag)&){caught=true;}
    assert(caught);
  };
  invalid([]{Matrix a{{0,0},{0,INF+1}};FloydWarshall(a);},std::out_of_range(""));
  invalid([]{Matrix a{{0,-INF},{INF,0}};std::vector<std::vector<int>> p;FloydWarshall(a,p);},std::out_of_range(""));
  invalid([]{Matrix a{{0,1},{2}};std::vector<std::vector<int>> p;FloydWarshall(a,p);},std::invalid_argument(""));
  invalid([]{FloydWarshallPath({{0}},-1,0);},std::out_of_range(""));
  invalid([]{FloydWarshallPath({{0}},0,1);},std::out_of_range(""));
  invalid([]{FloydWarshallPath({{0,1},{1}},1,0);},std::invalid_argument(""));
  invalid([]{FloydWarshallPath({{0,2},{1,1}},0,1);},std::out_of_range(""));
#endif
  std::cout << "45 boundary/SCC graphs + checked invalid arguments passed\n";
}
