// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PROJECT_TEST_HEADER
#define SIRKIT_PROJECT_TEST_HEADER "../../../Graph/Project_Selection.cpp"
#endif
#ifndef SIRKIT_PROJECT_REQUIRE_EXTRA
#define SIRKIT_PROJECT_REQUIRE_EXTRA 1
#endif
#ifndef SIRKIT_PROJECT_REQUIRE_STRONG
#define SIRKIT_PROJECT_REQUIRE_STRONG 1
#endif
#include SIRKIT_PROJECT_TEST_HEADER
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <optional>
#include <utility>
std::int64_t allocation_budget=-1;
std::size_t live_allocations=0,live_bytes=0,peak_bytes=0,allocation_calls=0;
struct AllocationHeader{void* raw;std::size_t bytes;};
void* allocate_test(std::size_t bytes,std::size_t alignment){if(allocation_budget==0)throw std::bad_alloc();if(allocation_budget>0)--allocation_budget;alignment=std::max(alignment,alignof(std::max_align_t));const std::size_t extra=sizeof(AllocationHeader)+alignment-1;if(bytes>std::numeric_limits<std::size_t>::max()-extra)throw std::bad_alloc();void*raw=std::malloc((bytes==0?1:bytes)+extra);if(!raw)throw std::bad_alloc();const auto address=(reinterpret_cast<std::uintptr_t>(raw)+sizeof(AllocationHeader)+alignment-1)&~(std::uintptr_t(alignment)-1);auto*header=reinterpret_cast<AllocationHeader*>(address)-1;header->raw=raw;header->bytes=bytes;++live_allocations;++allocation_calls;live_bytes+=bytes;peak_bytes=std::max(peak_bytes,live_bytes);return reinterpret_cast<void*>(address);}
void free_test(void* pointer)noexcept{if(!pointer)return;auto*header=static_cast<AllocationHeader*>(pointer)-1;--live_allocations;live_bytes-=header->bytes;std::free(header->raw);}
void*operator new(std::size_t n){return allocate_test(n,alignof(std::max_align_t));}void*operator new[](std::size_t n){return allocate_test(n,alignof(std::max_align_t));}void*operator new(std::size_t n,std::align_val_t a){return allocate_test(n,std::size_t(a));}void*operator new[](std::size_t n,std::align_val_t a){return allocate_test(n,std::size_t(a));}
void operator delete(void*p)noexcept{free_test(p);}void operator delete[](void*p)noexcept{free_test(p);}void operator delete(void*p,std::size_t)noexcept{free_test(p);}void operator delete[](void*p,std::size_t)noexcept{free_test(p);}void operator delete(void*p,std::align_val_t)noexcept{free_test(p);}void operator delete[](void*p,std::align_val_t)noexcept{free_test(p);}void operator delete(void*p,std::size_t,std::align_val_t)noexcept{free_test(p);}void operator delete[](void*p,std::size_t,std::align_val_t)noexcept{free_test(p);}
using Model=TwoVariableSubmodularOpt<std::int64_t>;
Model seed(int op=0){Model model(4);const int count=op==5?7:8;for(int k=0;k<count;++k){model.add_psp_penalty(0,2,1);model.add_psp_penalty(1,3,1);}for(int k=0;k<8;++k){model.add_single_cost(2,0,1);model.add_single_cost(3,1,0);}return model;}
void apply(Model&model,int op){switch(op){case 0:model.add_single_cost(0,0,-5);break;case 1:model.force(0,true);break;case 2:model.add_both_true_profit(0,1,5);break;case 3:model.add_both_false_profit(0,1,5);break;case 4:model.add_submodular_function(0,1,-3,2,4,1);break;case 5:model.add_equality(0,1);break;case 6:model.add_all_true_profit({0,1,2,3,0,1,2,3,0},5);break;case 7:model.add_all_false_profit({0,1,2,3,0,1,2,3,0},5);break;
#if SIRKIT_PROJECT_REQUIRE_EXTRA
case 8:model.add_any_true_cost({0,1,2,3,0,1,2,3,0},5);break;case 9:model.add_any_false_cost({0,1,2,3,0,1,2,3,0},5);break;
#endif
}}
int main(){const auto initial_live=live_allocations;std::size_t injected=0,partial_failures=0;for(int op=0;op<(SIRKIT_PROJECT_REQUIRE_EXTRA?10:8);++op){bool succeeded=false;for(int fail=0;fail<100&&!succeeded;++fail){auto model=seed(op);const auto value=model.solve();const auto edges=model.edge_count();const auto nodes=model.node_count();bool failed=false;allocation_budget=fail;try{apply(model,op);}catch(const std::bad_alloc&){failed=true;}allocation_budget=-1;if(failed){++injected;const bool unchanged=model.edge_count()==edges&&model.node_count()==nodes&&model.solve()==value;if(!unchanged)++partial_failures;
#if SIRKIT_PROJECT_REQUIRE_STRONG
assert(unchanged);
#endif
}else succeeded=true;}assert(succeeded);}
 for(int kind=0;kind<6;++kind){bool succeeded=false;for(int fail=0;fail<100&&!succeeded;++fail){auto source=seed();const auto value=source.solve();const auto edges=source.edge_count();Model target(2);target.add_constant(-13);assert(target.solve()==-13);std::optional<Model> constructed;bool failed=false;allocation_budget=fail;try{if(kind==0)constructed.emplace(source);if(kind==1)target=source;if(kind==2)constructed.emplace(std::move(source));if(kind==3)target=std::move(source);if(kind==4)source.init(20);if(kind==5)source.reserve_nodes(1000);}catch(const std::bad_alloc&){failed=true;}allocation_budget=-1;if(failed){++injected;assert(source.variable_count()==4&&source.edge_count()==edges&&source.solve()==value);assert(target.variable_count()==2&&target.solve()==-13);}else{if(kind<=3){Model&copied=(kind==0||kind==2)?*constructed:target;assert(copied.solve()==value&&copied.variable_count()==4);if(kind==2||kind==3)assert(source.variable_count()==0&&source.node_count()==2&&source.solve()==0);else{source.add_constant(1);assert(copied.solve()==value);}}if(kind==4)assert(source.variable_count()==20&&source.solve()==0);if(kind==5)assert(source.variable_count()==4&&source.solve()==value);succeeded=true;}}assert(succeeded);}
 for(int fail=0;fail<20;++fail){auto source=seed();auto copy=source;const auto expected=copy.solve();bool failed=false;allocation_budget=fail;try{(void)source.solve();}catch(const std::bad_alloc&){failed=true;}allocation_budget=-1;if(failed)++injected;assert(source.solve()==expected);if(!failed)break;}
#if SIRKIT_PROJECT_REQUIRE_EXTRA
 {auto model=seed();model.add_all_true_profit({0,1,2},1);allocation_budget=0;model.clear();allocation_budget=-1;assert(model.variable_count()==4&&model.node_count()==6&&model.edge_count()==0&&model.solve()==0);auto*alias=&model;allocation_budget=0;model=*alias;model=std::move(*alias);allocation_budget=-1;Model other(0);other.add_constant(3);static_assert(noexcept(swap(model,other)));allocation_budget=0;swap(model,other);allocation_budget=-1;assert(model.empty()&&model.solve()==3&&other.variable_count()==4&&other.solve()==0);}
#endif
 assert(live_allocations==initial_live);assert(live_bytes==0);std::cout<<injected<<" injected failures, "<<partial_failures<<" partial additions, ownership/exception checks passed\n";}
