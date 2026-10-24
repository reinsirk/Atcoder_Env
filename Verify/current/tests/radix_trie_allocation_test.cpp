// SPDX-License-Identifier: CC0-1.0
#ifndef RADIX_HEADER
#define RADIX_HEADER "../../../Algorithm/Radix_sort.cpp"
#endif
#ifndef TRIE_HEADER
#define TRIE_HEADER "../../../String/Trie.cpp"
#endif
#include RADIX_HEADER
#include TRIE_HEADER
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>

namespace { int remaining=-1; }
void* operator new(std::size_t bytes){
    if(remaining==0)throw std::bad_alloc();
    if(remaining>0)--remaining;
    if(void* p=std::malloc(bytes?bytes:1))return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes){return ::operator new(bytes);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

template<class Fn>bool allocation_failure(int budget,Fn fn){
    remaining=budget;bool failed=false;
    try{fn();}catch(const std::bad_alloc&){failed=true;}
    remaining=-1;return failed;
}
int main(){
    std::size_t checks=0;
    std::vector<std::int64_t> original(1000);
    for(std::size_t i=0;i<original.size();++i)original[i]=static_cast<std::int64_t>(i*137%1000);
    {
        auto a=original;RadixSortWorkspace<std::int64_t> workspace;
        assert(allocation_failure(0,[&]{workspace.sort(a);}));assert(a==original);++checks;
        workspace.reserve(a.size());assert(!allocation_failure(0,[&]{workspace.sort(a);}));assert(std::is_sorted(a.begin(),a.end()));++checks;
        const auto capacity=workspace.capacity();assert(allocation_failure(0,[&]{workspace.reserve(capacity+10000);}));assert(workspace.capacity()==capacity);++checks;
        assert(allocation_failure(0,[&]{(void)RadixSortOrder(original);}));++checks;
    }
    for(int budget=0;budget<4;++budget){
        Trie trie;
        for(unsigned i=0;i<15;++i)trie.insert(std::string(1,static_cast<char>(i)));
        const auto saved=trie.items();const auto size=trie.size(),nodes=trie.node_count();
        const std::string word=std::string(1,static_cast<char>(15))+std::string(1000,'z');
        bool failed=allocation_failure(budget,[&]{trie.insert(word,3);});
        if(failed){assert(trie.items()==saved&&trie.size()==size&&trie.node_count()==nodes);++checks;}
        else{assert(trie.count(word)==3);++checks;}
    }
    {
        Trie trie;trie.insert("abcdef");trie.erase_all("abcdef");
        assert(!allocation_failure(0,[&]{trie.insert("uvwxyz");}));assert(trie.count("uvwxyz")==1);++checks;
    }
    for(int budget=0;budget<3;++budget){
        Trie source;for(unsigned i=0;i<32;++i)source.insert(std::string(1,static_cast<char>(i))+"suffix");
        Trie target;target.insert("old",4);const auto saved=target.items();
        bool failed=allocation_failure(budget,[&]{target=source;});
        if(failed)assert(target.items()==saved);else assert(target.items()==source.items());++checks;
        assert(source.distinct_size()==32);
    }
    std::cout<<"PASS allocation/strong ownership checks="<<checks<<'\n';
}
