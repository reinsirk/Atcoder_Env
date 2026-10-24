// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PROJECT_TEST_HEADER
#define SIRKIT_PROJECT_TEST_HEADER "../../../Graph/Project_Selection.cpp"
#endif
#include SIRKIT_PROJECT_TEST_HEADER
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>
using Model = TwoVariableSubmodularOpt<std::int64_t>;
int main(int argc,char**argv){const std::string mode=argc>1?argv[1]:"typical_sparse";std::mt19937_64 random(334411);int n=100,repeats=2000;if(mode=="typical_dense"||mode=="reuse")repeats=1000;if(mode=="chain"){n=100000;repeats=5;}if(mode=="bipartite"){n=4000;repeats=5;}if(mode=="general"){n=20000;repeats=2;}if(mode=="sets"||mode=="submod"||mode=="incremental"){n=1000;repeats=20;}
 if(mode=="unary_large"){n=100000;repeats=10;}if(mode=="empty"){n=0;repeats=100000;}if(mode=="one"){n=1;repeats=100000;}
 std::vector<std::int64_t> costs(n);for(auto&v:costs)v=std::int64_t(random()%2001)-1000;
 std::vector<std::pair<int,int>> links;
 if(mode=="chain"){std::fill(costs.begin(),costs.end(),0);costs.front()=1;costs.back()=-1;for(int i=1;i<n;++i)links.push_back({i,i-1});}
 else if(mode=="bipartite"){for(int i=0;i<n;++i)costs[i]=i<n/2?-1:1;for(int i=0;i<200000;++i)links.push_back({int(random()%(n/2)),n/2+int(random()%(n/2))});}
 else if(mode=="general"){for(int i=0;i<100000;++i)links.push_back({int(random()%n),int(random()%n)});}
 else if(mode!="sets"&&mode!="submod"&&mode!="incremental"&&mode!="unary_large"&&mode!="empty"&&mode!="one"){for(int i=0;i<n;++i)for(int j=i+1;j<n;++j)if(mode=="typical_dense"||mode=="reuse"||random()%15==0)links.push_back({j,i});}
 std::vector<std::vector<int>> subsets;for(int i=0;i<(n==0?0:1000);++i){std::vector<int> xs;for(int k=0;k<10;++k)xs.push_back(int(random()%n));subsets.push_back(std::move(xs));}
 std::int64_t checksum=0;const auto start=std::chrono::steady_clock::now();
 auto work=[&](Model& model){for(int i=0;i<n;++i)model.add_single_cost(i,0,costs[i]);for(auto[i,j]:links)model.add_psp_constraint(i,j);
  if(mode=="sets")for(int i=0;i<1000;++i){if(i&1)model.add_all_true_profit(subsets[i],3);else model.add_all_false_profit(subsets[i],3);}
  if(mode=="submod")for(int i=0;i<10000;++i)model.add_submodular_function(i%n,(i*17+11)%n,-1,2,3,1);
  if(mode=="incremental")for(int i=0;i<1000;++i){model.add_psp_penalty(i%n,(i*17+11)%n,3);if(i%50==0)checksum+=model.solve();}
  checksum+=model.solve();
 };
 if(mode=="reuse"){Model model(n);for(int rep=0;rep<repeats;++rep){model.clear();work(model);}}
 else for(int rep=0;rep<repeats;++rep){Model model(n);work(model);}
 const auto stop=std::chrono::steady_clock::now();std::cout<<std::chrono::duration<double,std::milli>(stop-start).count()<<' '<<checksum<<'\n';}
