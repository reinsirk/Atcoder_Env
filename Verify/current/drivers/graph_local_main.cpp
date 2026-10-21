// SPDX-License-Identifier: CC0-1.0
#include <iostream>
#include <random>
#include <stdexcept>

int main() {
    const auto require=[](bool ok) { if(!ok) throw std::runtime_error("graph reference mismatch"); };
    std::mt19937 random(219381);
    for(int test=0;test<500;++test) {
        const int n=1+random()%14;
        WGraph graph(n);
        std::vector ref(n,std::vector<std::int64_t>(n,INF));
        std::vector reach(n,std::vector<bool>(n));
        for(int i=0;i<n;++i) { ref[i][i]=0;reach[i][i]=true; }
        for(int i=0;i<n*n/2;++i) {
            const int u=random()%n,v=random()%n;
            const std::int64_t w=static_cast<int>(random()%31)-10;
            graph[u].emplace_back(v,w);ref[u][v]=std::min(ref[u][v],w);reach[u][v]=true;
        }
        const auto initial=ref;
        for(int k=0;k<n;++k) for(int i=0;i<n;++i) for(int j=0;j<n;++j) {
            reach[i][j]=reach[i][j] || (reach[i][k]&&reach[k][j]);
            if(ref[i][k]!=INF && ref[k][j]!=INF) ref[i][j]=std::min(ref[i][j],ref[i][k]+ref[k][j]);
        }
        std::vector negative(n,std::vector<bool>(n));bool any=false;
        for(int k=0;k<n;++k) if(ref[k][k]<0) {
            any=true;
            for(int i=0;i<n;++i) for(int j=0;j<n;++j)
                if(reach[i][k]&&reach[k][j]) negative[i][j]=true;
        }
        auto expected=ref;
        for(int i=0;i<n;++i) for(int j=0;j<n;++j) if(negative[i][j]) expected[i][j]=-INF;
        auto distance=initial;require(FloydWarshall(distance)==any);require(distance==expected);
        std::vector<std::vector<int>> next;
        distance=initial;require(FloydWarshall(distance,next)==any);require(distance==expected);
        for(int s=0;s<n;++s) {
            const bool cycle=std::any_of(negative[s].begin(),negative[s].end(),[](bool x){return x;});
            std::vector<std::int64_t> actual;
            require(BellmanFord(graph,actual,s)==cycle);require(actual==expected[s]);
            require(SPFA(graph,actual,s)==cycle);require(actual==expected[s]);
            for(int t=0;t<n;++t) if(!negative[s][t]) {
                const auto path=FloydWarshallPath(next,s,t);
                if(!reach[s][t]) {require(path.empty());continue;}
                require(!path.empty() && path.front()==s && path.back()==t);
                std::int64_t sum=0;
                for(std::size_t i=1;i<path.size();++i) sum+=initial[path[i-1]][path[i]];
                require(sum==expected[s][t]);
            }
        }
    }
    // 256頂点をまたぎ、小規模ヒープ・大規模radix heapの両経路を使う。
    for(int n:{1,2,255,256,257,600}) {
        WGraph graph(n);
        for(int u=0;u<n;++u) {
            if(u+1<n)graph[u].emplace_back(u+1,0);
            for(int e=0;e<15;++e)graph[u].emplace_back(random()%n,random()%1'000'000'001);
        }
        for(int s:{0,n/2,n-1}) {
            std::vector<std::int64_t> expected(n,INF);expected[s]=0;
            for(int i=1;i<n;++i) {
                bool change=false;
                for(int u=0;u<n;++u) if(expected[u]!=INF) for(const auto& e:graph[u])
                    if(expected[e.to]>expected[u]+e.w) { expected[e.to]=expected[u]+e.w;change=true; }
                if(!change)break;
            }
            std::vector<std::int64_t> actual;Dijkstra(graph,actual,s);require(actual==expected);
            for(int t:{0,n/2,n-1}) {
                std::vector<int> path;Dijkstra(graph,actual,s,t,path);require(actual==expected);
                if(expected[t]==INF) {require(path.empty());continue;}
                require(path.front()==s && path.back()==t && path.size()<=static_cast<std::size_t>(n));
                std::int64_t sum=0;
                for(std::size_t i=1;i<path.size();++i) {
                    auto w=INF;
                    for(const auto& e:graph[path[i-1]]) if(e.to==path[i])w=std::min(w,e.w);
                    require(w!=INF);sum+=w;
                }
                require(sum==expected[t]);
            }
        }
    }
    std::cout<<"OK\n";
}
