// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>
#ifndef SIRKIT_TEST_TOPOLOGICAL_SOURCE
#define SIRKIT_TEST_TOPOLOGICAL_SOURCE "../../../Graph/Topological.cpp"
#endif
#include SIRKIT_TEST_TOPOLOGICAL_SOURCE
static std::uint64_t cases=0,checks=0;
static void require(bool b){++checks;if(!b)throw std::runtime_error("topological reference mismatch");}
// 出力集合から次の頂点を選び、各辺の残存を毎回直接照合する独立参照。
static std::vector<int> slow_lex(const Graph& g){std::vector<int> order;std::vector<bool> used(g.size());while(order.size()<g.size()){int next=-1;for(int v=0;v<static_cast<int>(g.size());++v)if(!used[v]){bool blocked=false;for(int u=0;u<static_cast<int>(g.size());++u)if(!used[u])for(int to:g[u])if(to==v)blocked=true;if(!blocked){next=v;break;}}if(next<0)break;used[next]=true;order.push_back(next);}return order;}
static TopologicalResult reference(const Graph& g,bool lex){std::vector<std::size_t>degree(g.size());for(const auto& es:g)for(int to:es)++degree[to];std::deque<int>fifo;std::set<int>ready;for(int i=0;i<static_cast<int>(g.size());++i)if(!degree[i]){fifo.push_back(i);ready.insert(i);}TopologicalResult r;r.position.assign(g.size(),-1);while(lex?!ready.empty():!fifo.empty()){if((lex?ready.size():fifo.size())>1)r.unique=false;int v;if(lex){v=*ready.begin();ready.erase(ready.begin());}else{v=fifo.front();fifo.pop_front();}r.position[v]=static_cast<int>(r.order.size());r.order.push_back(v);for(int to:g[v])if(--degree[to]==0){if(lex)ready.insert(to);else fifo.push_back(to);}}r.acyclic=r.order.size()==g.size();r.unique=r.unique&&r.acyclic;return r;}
static void test(const Graph& g,bool slow=false){++cases;const auto before=g;for(bool lex:{false,true}){auto expected=reference(g,lex);auto actual=TopologicalSortDetails(g,lex);require(actual.order==expected.order);require(actual.position==expected.position);require(actual.acyclic==expected.acyclic);require(actual.unique==expected.unique);auto order=lex?TopologicalSortLexicographical(g):TopologicalSort(g);require(order==(expected.acyclic?expected.order:std::vector<int>{}));if(lex&&slow)require(expected.order==slow_lex(g));}require(IsDAG(g)==reference(g,false).acyclic);require(g==before);}
int main(){try{
 for(int n=0;n<=4;++n)for(std::uint32_t mask=0;mask<(std::uint32_t{1}<<(n*n));++mask){Graph g(n);for(int i=0;i<n;++i)for(int j=0;j<n;++j)if((mask>>(i*n+j))&1)g[i].push_back(j);test(g,true);}
 std::mt19937 rng(0x81FA22);for(int trial=0;trial<1000;++trial){int n=1+static_cast<int>(rng()%40);Graph g(n);int edges=static_cast<int>(rng()%(n*6+1));for(int e=0;e<edges;++e)g[rng()%n].push_back(static_cast<int>(rng()%n));test(g,true);}
 for(int n:{1,2,31,32,63,64,65,255,256,257,4096,200000,262145})for(int kind=0;kind<7;++kind){Graph g(n);if(kind==1)for(int i=1;i<n;++i)g[i-1].push_back(i);if(kind==2)for(int i=n-1;i>0;--i)g[i].push_back(i-1);if(kind==3)for(int i=1;i<n;++i)g[0].push_back(i);if(kind==4)for(int i=1;i<n;++i)g[i].push_back(0);if(kind==5)for(int i=1;i<n;++i){g[i-1].push_back(i);g[i-1].push_back(i);}if(kind==6)for(int i=0;i<n;++i)g[i].push_back((i+1)%n);test(g);}
 // 最初の新規候補が旧最小値より小さく、後続候補も増える場合。
 test(Graph{{},{},{},{0,1,2},{0,2}});test(Graph{{},{},{},{2,0,1},{}});
#ifndef SKIP_RUNTIME_CHECKS
 for(int n:{1,2,65})for(int to:{-1,n}){Graph g(n);g.back().push_back(to);for(int api=0;api<5;++api){bool caught=false;try{if(api==0)(void)TopologicalSort(g);if(api==1)(void)TopologicalSortLexicographical(g);if(api==2)(void)TopologicalSortDetails(g);if(api==3)(void)TopologicalSortDetails(g,true);if(api==4)(void)IsDAG(g);}catch(const std::out_of_range&){caught=true;}require(caught);}}
#endif
 std::cout<<"{\"cases\":"<<cases<<",\"checks\":"<<checks<<"}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
