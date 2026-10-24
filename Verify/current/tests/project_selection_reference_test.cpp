// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PROJECT_TEST_HEADER
#define SIRKIT_PROJECT_TEST_HEADER "../../../Graph/Project_Selection.cpp"
#endif
#ifndef SIRKIT_PROJECT_REQUIRE_EXTRA
#define SIRKIT_PROJECT_REQUIRE_EXTRA 1
#endif
#include SIRKIT_PROJECT_TEST_HEADER
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
std::uint64_t checks=0;
void require(bool okay){++checks;if(!okay){std::cerr<<"project selection failed "<<checks<<'\n';std::abort();}}
template<class Exception,class Function> void throws(Function&& function){bool caught=false;try{function();}catch(const Exception&){caught=true;}require(caught);}
template<class Cost>void trials(std::uint64_t seed,int count,int steps){std::mt19937_64 random(seed);for(int trial=0;trial<count;++trial){const int n=1+int(random()%(sizeof(Cost)==1?5:8));TwoVariableSubmodularOpt<Cost> model(n);std::vector<std::int64_t> energy(std::size_t{1}<<n);std::vector<bool> valid(energy.size(),true);
 auto verify=[&](auto& candidate){std::int64_t best=std::numeric_limits<std::int64_t>::max();for(std::size_t mask=0;mask<energy.size();++mask)if(valid[mask])best=std::min(best,energy[mask]);if(best==std::numeric_limits<std::int64_t>::max()){throws<std::domain_error>([&]{(void)candidate.solve();});throws<std::domain_error>([&]{(void)candidate.reconstruct();});return;}const auto actual=candidate.solve();require(actual==best);require(candidate.solve()==best);const auto assignment=candidate.reconstruct();require(assignment.size()==std::size_t(n));std::size_t mask=0;for(int i=0;i<n;++i)if(assignment[i])mask|=std::size_t{1}<<i;require(valid[mask]&&energy[mask]==best);const auto together=candidate.solve_with_assignment();require(together.first==best&&together.second==assignment);};
 for(int step=0;step<steps;++step){int op=int(random()%13);if((trial&1)==0&&(op==9||op==10))op=3;const int i=int(random()%n),j=int(random()%n);const auto signed_value=[&](){return sizeof(Cost)==1?std::int64_t(random()%2):std::int64_t(random()%7)-3;};const Cost a=Cost(signed_value()),b=Cost(signed_value()),d=Cost(signed_value()),p=Cost(random()%3);const Cost c=Cost(std::int64_t(a)+d-b+random()%2);std::vector<int> subset;for(int k=0;k<n;++k)if(random()%3==0)subset.push_back(k);if(!subset.empty()&&random()%3==0)subset.push_back(subset.front());const bool value=(random()&1)!=0;
  switch(op){case 0:model.add_single_cost(i,a,b);break;case 1:model.add_constant(a);break;case 2:model.add_psp_constraint(i,j);break;case 3:model.add_psp_penalty(i,j,p);break;case 4:model.add_both_true_profit(i,j,p);break;case 5:model.add_both_false_profit(i,j,p);break;case 6:model.add_submodular_function(i,j,a,b,c,d);break;case 7:model.add_all_true_profit(subset,p);break;case 8:model.add_all_false_profit(subset,p);break;case 9:model.force(i,value);break;case 10:model.add_equality(i,j);break;case 11:
#if SIRKIT_PROJECT_REQUIRE_EXTRA
 model.add_any_true_cost(subset,p);
#else
 model.add_constant(p);model.add_all_false_profit(subset,p);
#endif
 break;case 12:
#if SIRKIT_PROJECT_REQUIRE_EXTRA
 model.add_any_false_cost(subset,p);
#else
 model.add_constant(p);model.add_all_true_profit(subset,p);
#endif
 break;}
  for(std::size_t mask=0;mask<energy.size();++mask){const bool x=((mask>>i)&1)!=0,y=((mask>>j)&1)!=0;bool all_true=true,all_false=true;for(int k:subset){all_true=all_true&&((mask>>k)&1)!=0;all_false=all_false&&((mask>>k)&1)==0;}switch(op){case 0:energy[mask]+=x?b:a;break;case 1:energy[mask]+=a;break;case 2:valid[mask]=valid[mask]&&(!x||y);break;case 3:if(x&&!y)energy[mask]+=p;break;case 4:if(x&&y)energy[mask]-=p;break;case 5:if(!x&&!y)energy[mask]-=p;break;case 6:energy[mask]+=x?(y?d:c):(y?b:a);break;case 7:if(all_true)energy[mask]-=p;break;case 8:if(all_false)energy[mask]-=p;break;case 9:valid[mask]=valid[mask]&&(x==value);break;case 10:valid[mask]=valid[mask]&&(x==y);break;case 11:if(!all_false)energy[mask]+=p;break;case 12:if(!all_true)energy[mask]+=p;break;}}
  verify(model);if(step%7==0){auto copied=model;verify(copied);auto moved=std::move(copied);require(copied.variable_count()==0&&copied.node_count()==2&&copied.solve()==0);verify(moved);TwoVariableSubmodularOpt<Cost> assigned(1);assigned=std::move(moved);require(moved.variable_count()==0&&moved.solve()==0);verify(assigned);auto* alias=&assigned;assigned=*alias;verify(assigned);assigned=std::move(*alias);verify(assigned);}
 }
 model.clear();require(model.variable_count()==n&&model.node_count()==n+2&&model.edge_count()==0&&model.solve()==0);model.init(0);require(model.variable_count()==0&&model.node_count()==2&&model.solve()==0);
#if SIRKIT_PROJECT_REQUIRE_EXTRA
 require(model.empty());TwoVariableSubmodularOpt<Cost> other(2);other.add_constant(Cost(3));swap(model,other);require(model.variable_count()==2&&model.solve()==3&&other.empty());
#endif
}}
template<class Cost>void boundaries(){using Model=TwoVariableSubmodularOpt<Cost>;const Cost minimum=std::numeric_limits<Cost>::min(),maximum=std::numeric_limits<Cost>::max();Model equal(1);equal.add_single_cost(0,minimum,minimum);require(equal.solve()==minimum);equal.add_constant(maximum);require(equal.solve()==-1);equal.clear();equal.add_single_cost(0,0,maximum);equal.force(0,true);equal.add_constant(1);throws<std::overflow_error>([&]{(void)equal.solve();});equal.add_constant(-1);require(equal.solve()==maximum);
 Model endpoint(1);endpoint.add_single_cost(0,minimum,0);endpoint.force(0,true);require(endpoint.solve()==0&&endpoint.reconstruct()[0]);Model full(1);full.add_single_cost(0,Cost(maximum-1),minimum);require(full.solve()==minimum);const auto old_edges=full.edge_count();throws<std::overflow_error>([&]{full.add_psp_penalty(0,0,1);full.add_single_cost(0,0,1);});require(full.edge_count()==old_edges&&full.solve()==minimum);full.force(0,false);require(full.solve()==maximum-1);full.force(0,true);throws<std::domain_error>([&]{(void)full.solve();});auto infeasible_copy=full;throws<std::domain_error>([&]{(void)infeasible_copy.solve();});infeasible_copy.clear();require(infeasible_copy.solve()==0);
 Model invalid(2);throws<std::overflow_error>([&]{invalid.add_single_cost(0,maximum,minimum);});require(invalid.edge_count()==0&&invalid.solve()==0);throws<std::overflow_error>([&]{invalid.add_submodular_function(0,1,0,minimum,0,maximum);});require(invalid.edge_count()==0&&invalid.solve()==0);throws<std::length_error>([&]{invalid.init(-1);});throws<std::length_error>([&]{invalid.reserve_nodes(std::numeric_limits<std::size_t>::max());});require(invalid.variable_count()==2&&invalid.solve()==0);
#ifndef SKIP_RUNTIME_CHECKS
 throws<std::out_of_range>([&]{invalid.add_single_cost(2,0,0);});throws<std::invalid_argument>([&]{invalid.add_psp_penalty(0,1,-1);});throws<std::invalid_argument>([&]{invalid.add_submodular_function(0,1,1,0,0,1);});const auto nodes=invalid.node_count();throws<std::out_of_range>([&]{invalid.add_all_true_profit({0,2},1);});require(invalid.node_count()==nodes&&invalid.edge_count()==0&&invalid.solve()==0);
#endif

#if SIRKIT_PROJECT_REQUIRE_EXTRA
 Model any(2);any.add_constant(maximum);any.add_any_true_cost({},1);any.add_any_false_cost({},1);any.add_any_true_cost({0,1},1);require(any.solve()==maximum);any.force(0,true);throws<std::overflow_error>([&]{(void)any.solve();});any.clear();any.add_constant(maximum);any.add_any_false_cost({0,1},1);require(any.solve()==maximum);any.force(1,false);throws<std::overflow_error>([&]{(void)any.solve();});
#endif
 Model dump(2);dump.add_single_cost(0,0,-2);dump.add_single_cost(1,0,1);dump.add_psp_constraint(0,1);require(dump.solve()==-1);std::ostringstream stream;stream<<dump;require(!stream.str().empty());dump.add_constant(3);require(dump.solve()==2);}
int main(){trials<std::int8_t>(71,150,10);trials<std::int16_t>(72,150,25);trials<std::int32_t>(73,200,35);trials<std::int64_t>(74,300,40);boundaries<std::int8_t>();boundaries<std::int16_t>();boundaries<std::int32_t>();boundaries<std::int64_t>();
 TwoVariableSubmodularOpt<std::int64_t> long_path(100000);long_path.add_single_cost(0,0,1);long_path.add_single_cost(99999,0,-1);for(int i=1;i<100000;++i)long_path.add_psp_constraint(i,i-1);require(long_path.solve()==0);long_path.force(99999,true);require(long_path.solve()==0);long_path.force(0,false);throws<std::domain_error>([&]{(void)long_path.solve();});std::cout<<checks<<" independent project selection checks passed\n";}
