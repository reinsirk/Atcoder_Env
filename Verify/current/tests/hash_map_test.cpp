// SPDX-License-Identifier: CC0-1.0
#ifndef HEADER
#define HEADER "../../../DataStructure/Hash_map.cpp"
#endif
#include HEADER
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>
#include <map>
#include <random>
#include <string>
#include <iostream>
#include <memory>

struct Collision { std::uint64_t operator()(int) const noexcept { return 0; } };
struct ThrowHash {
 static bool armed;
 std::uint64_t operator()(int x) const { if(armed && x==17) throw std::runtime_error("hash"); return GenericHash(123)(x); }
}; bool ThrowHash::armed=false;
struct Bomb {
 static int allowance;
 int value=0;
 Bomb()=default; explicit Bomb(int x):value(x){}
 Bomb(const Bomb& x):value(x.value) { if(allowance==0)throw std::runtime_error("copy");if(allowance>0)--allowance; }
 Bomb(Bomb&& x) noexcept(false):value(x.value) { x.value=-1; }
 Bomb& operator=(const Bomb&)=default;
}; int Bomb::allowance=-1;

template<class Key,class Hash> void random_case(const std::vector<Key>& keys,Hash hash) {
 HashMap<Key,int,Hash> table(0,hash);
 std::map<Key,int> ref;
 std::mt19937_64 rng(513);
 for(int rep=0;rep<100000;++rep) {
  const auto& k=keys[rng()%keys.size()];
  switch(rng()%13) {
   case 0:case 1:case 2:{int v=int(rng()%1000);table[k]=v;ref[k]=v;break;}
   case 3:{auto it=table.find(k);auto want=ref.find(k);assert((it==table.end())==(want==ref.end()));if(it!=table.end())assert(it->second==want->second);break;}
   case 4:{table.erase(k);ref.erase(k);break;}
   case 5:{auto got=table.try_emplace(k,7);auto want=ref.try_emplace(k,7);assert(got.second==want.second && got.first->second==want.first->second);break;}
   case 6:{auto got=table.insert_or_assign(k,9);auto want=ref.insert_or_assign(k,9);assert(got.second==want.second);break;}
   case 7:{auto it=table.find(k);if(it!=table.end()){table.erase(it,false);ref.erase(k);}break;}
   case 8:{auto copy=table;assert(copy.size()==ref.size());for(auto& [key,v]:ref)assert(copy.at(key)==v);HashMap<Key,int,Hash> other(std::move(copy));assert(copy.empty());copy=other;break;}
   case 9: if(rep%300==0)table.rehash(rng()%300);break;
   case 10:if(rep%500==0){table.clear();ref.clear();}break;
   case 11:assert(table.get_or_default(k,77)==(ref.contains(k)?ref.at(k):77));break;
   case 12: {
    const auto old_size=table.size();
    const auto removed=ref.erase(k);
    assert(table.erase(k,false)==table.end());
    assert(table.size()==old_size-removed && !table.contains(k));
    break;
   }
  }
  assert(table.size()==ref.size());
  if(rep%100==0){std::map<Key,int> got;for(auto& v:table)got.insert(v);assert(got==ref);table.reserve(rng()%300);}
 }
}
int main(){
 std::vector<int> keys;for(int i=-100;i<=100;++i)keys.push_back(i);
 random_case(keys,GenericHash(123));random_case(keys,Collision{});
 std::vector<std::string> texts;for(auto k:keys)texts.push_back("prefix"+std::to_string(k));random_case(texts,GenericHash(123));
 HashMap<int,Bomb> bombs(0,GenericHash(11));for(int i=0;i<50;++i)bombs.try_emplace(i,i+3);
 const auto cap=bombs.capacity();Bomb::allowance=10;bool threw=false;try{bombs.reserve(1000);}catch(const std::runtime_error&){threw=true;}assert(threw&&bombs.capacity()==cap&&bombs.size()==50);Bomb::allowance=-1;for(int i=0;i<50;++i)assert(bombs.at(i).value==i+3);
 HashMap<int,std::unique_ptr<int>,ThrowHash> movable;for(int i=0;i<50;++i)movable.try_emplace(i,std::make_unique<int>(i));ThrowHash::armed=true;threw=false;try{movable.reserve(1000);}catch(const std::runtime_error&){threw=true;}assert(threw&&movable.size()==50);ThrowHash::armed=false;for(int i=0;i<50;++i)assert(*movable.at(i)==i);
 HashMap<int,int> empty;
 assert(empty.erase(5,false)==empty.end() && empty.empty() && empty.capacity()==0);
 HashMap<int,int> sparse(1u<<18,GenericHash(123));
 const auto sparse_capacity=sparse.capacity();
 for(int key : {-11,0,17,50}) sparse[key]=key*3;
 const auto& sparse_ref=sparse;
 assert(sparse_ref.at(17)==51 && sparse_ref.find(0)->second==0);
 assert(sparse.erase(17,false)==sparse.end() && sparse.size()==3 && !sparse.contains(17));
 assert(sparse.erase(17,false)==sparse.end() && sparse.size()==3);
 assert(sparse_ref.at(-11)==-33 && sparse_ref.at(50)==150);
 for(int key : {-11,0,50}) sparse.erase(key,false);
 assert(sparse.empty() && sparse.capacity()==sparse_capacity);
 sparse[17]=99;assert(sparse.size()==1 && sparse.at(17)==99);
#ifndef SKIP_RUNTIME_CHECKS
 HashMap<int,int> checked;checked[0]=1;auto it=checked.begin();checked.rehash(100);threw=false;try{(void)*it;}catch(const std::out_of_range&){threw=true;}assert(threw);
#endif
 std::cout<<"PASS randomized integer/string/collision, copy/move/erase/tombstones, throwing copy/hash\n";
}
