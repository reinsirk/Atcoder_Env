// SPDX-License-Identifier: CC0-1.0
// 非整数型のコピー代入で確保失敗後も読み取り・再利用できることを確認する。
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#endif
#include FENWICK_SOURCE
#include <cstdio>
#include <cstdlib>
#include <new>
#include <source_location>
static int remaining=-1;
static std::size_t allocations=0,checks=0,failures=0;
[[gnu::noinline]]void* operator new(std::size_t n){++allocations;if(remaining==0)throw std::bad_alloc();if(remaining>0)--remaining;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
[[gnu::noinline]]void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]]static void release(void*p)noexcept{std::free(p);}
void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}
void operator delete(void*p,std::size_t)noexcept{release(p);}void operator delete[](void*p,std::size_t)noexcept{release(p);}
static void require(bool ok,std::source_location loc=std::source_location::current()){++checks;if(!ok){std::fprintf(stderr,"failed line %u\n",loc.line());std::abort();}}
using Tree=DynamicFenwickTree<int,double>;
static Tree build(int n,int count){Tree bit(n);for(int i=0;i<count;++i)bit.add(i,double(i%8)/8);bit.add(n-1,3);return bit;}
static void failure_case(int source_n,int source_count,int destination_n){
 int local_failures=0,successes=0;
 for(int allowed=0;allowed<10;++allowed){
  auto source=build(source_n,source_count);Tree destination(destination_n);destination.add(0,7);destination.add(destination_n-1,13);
  const double expected=source.sum_all();const auto nodes=source.stored_nodes();
  bool failed=false;remaining=allowed;
  try{destination=source;}catch(const std::bad_alloc&){failed=true;}
  remaining=-1;
  require(source.size()==source_n&&source.sum_all()==expected&&source.stored_nodes()==nodes);
  if(failed){
   ++failures;++local_failures;
   // 基本保証の零状態と、強い保証による代入前の状態の両方を許す。
   if(destination.stored_nodes()==0){require(destination.sum_all()==0);if(destination.size())require(destination.get(destination.size()-1)==0);}
   else{require(destination.size()==destination_n);require(destination.sum_all()==20&&destination.get(0)==7&&destination.get(destination_n-1)==13);}
   destination.clear();destination.reset(destination_n);destination.add(destination_n-1,17);require(destination.sum_all()==17&&destination.get(destination_n-1)==17);
   destination=source;
  }else ++successes;
  require(destination.size()==source_n&&destination.sum_all()==expected&&destination.stored_nodes()==nodes);
  require(destination.get(source_n-1)==source.get(source_n-1));
  destination.add(source_n-1,1);require(destination.sum_all()==expected+1&&source.sum_all()==expected);
 }
 require(local_failures>0&&successes>0);
}
static std::size_t capacity_reuse(int n,int count){
 auto source=build(n,count),destination=build(n,count);
 const auto before=allocations;destination=source;const auto used=allocations-before;
 require(destination.size()==n&&destination.sum_all()==source.sum_all());
 const auto self_before=allocations;auto* alias=&destination;destination=*alias;require(destination.sum_all()==source.sum_all());
 // 自己代入は値が不変ならよく、確保回数の保証はAPIに課さない。
 static_cast<void>(self_before);
 return used;
}
int main(){
 failure_case(64,1,32);failure_case(5000,1000,32);failure_case(64,1,1000001);failure_case(1000001,2,64);
 const auto tiny=capacity_reuse(64,1),dense=capacity_reuse(5000,1000);
 std::printf("{\"checks\":%zu,\"allocation_failures\":%zu,\"copy_allocations_tiny_with_capacity\":%zu,\"copy_allocations_dense_with_capacity\":%zu,\"status\":\"pass\"}\n",checks,failures,tiny,dense);
}
