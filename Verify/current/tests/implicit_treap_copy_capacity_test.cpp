// SPDX-License-Identifier: CC0-1.0
#include "../../../DataStructure/implicit_treap.cpp"
#include <utility>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>
static bool block_allocation=false;
void* operator new(std::size_t n){if(block_allocation)throw std::bad_alloc();if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
int main(){
 using Tree=lazy_implicit_treap<>;
 std::vector<std::int64_t> a,b;for(int i=0;i<250;++i)a.push_back(i);for(int i=0;i<50;++i)b.push_back(i+1000);
 Tree source(a),target(b);target.reserve(500);
 source.apply(10,220,3);for(int i=10;i<220;++i)a[i]+=3;source.reverse(0,250);std::reverse(a.begin(),a.end());
 block_allocation=true;target=source;block_allocation=false;
 assert(target.to_vector()==a);
 Tree small(b);block_allocation=true;bool failed=false;try{small=source;}catch(const std::bad_alloc&){failed=true;}block_allocation=false;
 assert(failed&&small.to_vector()==b);small.erase_at(3);b.erase(b.begin()+3);small.insert_at(8,12);b.insert(b.begin()+8,12);assert(small.to_vector()==b);
 std::cout<<"nothrow existing-capacity copy used no allocation; growing-copy allocation failure preserved the destination\n";
}
