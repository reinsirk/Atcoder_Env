// SPDX-License-Identifier: CC0-1.0
#include <atomic>
#include <cstdlib>
#include <new>
#include <cstdint>
#include <iostream>
#include <vector>
#include <algorithm>
#include <type_traits>
#ifndef TRIE_SOURCE
#define TRIE_SOURCE "../../../DataStructure/Compressed_Binary_Trie.cpp"
#endif
static std::int64_t fail_after=-1;
static std::uint64_t allocation_attempts=0;
static std::atomic<std::size_t> live_allocations{0}, requests{0};
[[gnu::noinline]] static void* obtain(std::size_t n){++requests;if(fail_after>=0){++allocation_attempts;if(fail_after--==0)throw std::bad_alloc();}if(auto*p=std::malloc(n?n:1)){++live_allocations;return p;}throw std::bad_alloc();}
void* operator new(std::size_t n){return obtain(n);} void* operator new[](std::size_t n){return obtain(n);}
[[gnu::noinline]]static void release(void*p)noexcept{if(p)--live_allocations;std::free(p);}
void operator delete(void*p)noexcept{release(p);} void operator delete[](void*p)noexcept{release(p);}
void operator delete(void*p,std::size_t)noexcept{release(p);} void operator delete[](void*p,std::size_t)noexcept{release(p);}
#include TRIE_SOURCE
#ifndef TRIE_CLASS
#define TRIE_CLASS CompressedBinaryTrie
#endif
static std::uint64_t checks=0,exceptions=0,successes=0;
#define REQUIRE(x)do{++checks;if(!(x)){std::cerr<<"failed "<<__LINE__<<" "<<#x<<'\n';std::abort();}}while(false)
int main(){for(std::uint32_t initial:{0u,1u,2u,2040u,2047u,2048u,4095u,4096u})for(std::uint32_t key:{0u,initial+1,0x80000000u})for(int mode=0;mode<3;++mode)for(int budget=0;budget<9;++budget){
 const auto before_live=live_allocations.load();{
 TRIE_CLASS<std::uint32_t,31,1000000>tr;for(std::uint32_t i=0;i<initial;++i)tr.add(i);if(mode>0){tr.add(key,5);if(mode==2){tr.add(key,6);tr.add(key,7);tr.add(key,8);}}
 const auto oldsize=tr.size();auto oldids=tr.find(key).second;auto oldcount=tr.count(key);bool threw=false;
 fail_after=budget;try{tr.add(key,11);}catch(const std::bad_alloc&){threw=true;}fail_after=-1;
 if(threw){++exceptions;REQUIRE(tr.size()==oldsize);REQUIRE(tr.count(key)==oldcount);REQUIRE(tr.find(key).second==oldids);for(std::uint32_t i=0;i<initial;++i)REQUIRE(tr.count(i)==(i==key?oldcount:1));tr.add(key,11);}
 else ++successes;
 REQUIRE(tr.size()==oldsize+1);REQUIRE(tr.count(key)==oldcount+1);oldids.push_back(11);auto now=tr.find(key).second;std::sort(now.begin(),now.end());std::sort(oldids.begin(),oldids.end());REQUIRE(now==oldids);REQUIRE(tr.del(key,11));REQUIRE(tr.size()==oldsize);
 const auto before_requests=requests.load();fail_after=0;
 REQUIRE(tr.count(0)==tr.find(0).first);REQUIRE(tr.contains(0)==(tr.count(0)!=0));REQUIRE(!tr.del(0xffffffffu));
 if(!tr.empty()){static_cast<void>(tr.min_xor(0));static_cast<void>(tr.max_element());static_cast<void>(tr.get_kth(1));}
 fail_after=-1;REQUIRE(requests.load()==before_requests);
 }REQUIRE(live_allocations.load()==before_live);
 }
 REQUIRE(live_allocations.load()==0);
 std::cout<<"{\"checks\":"<<checks<<",\"injected_bad_alloc\":"<<exceptions<<",\"successful_add\":"<<successes<<",\"allocation_attempts\":"<<allocation_attempts<<",\"live_delta\":"<<live_allocations.load()<<"}\n";
}
