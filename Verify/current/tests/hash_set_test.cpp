// SPDX-License-Identifier: CC0-1.0
#ifndef HEADER
#define HEADER "../../../DataStructure/Hash_set.cpp"
#endif
#include HEADER
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
struct Collision { std::uint64_t operator()(int) const noexcept { return 0; } };
struct ThrowHash { static bool armed;std::uint64_t operator()(int x)const {if(armed && x==17)throw std::runtime_error("hash");return GenericHash(123)(x);} };bool ThrowHash::armed=false;
struct Bomb {
 static int budget;int value=0;
 Bomb()=default;explicit Bomb(int x):value(x){}
 Bomb(const Bomb& x):value(x.value){if(budget==0)throw std::runtime_error("copy");if(budget>0)--budget;}
 Bomb(Bomb&& x)noexcept(false):value(x.value){x.value=-1;}
 bool operator==(const Bomb& x)const noexcept{return value==x.value;}
};int Bomb::budget=-1;
struct BombHash {std::uint64_t operator()(const Bomb& x)const noexcept{return GenericHash(5)(x.value);}};
struct MoveKey {std::unique_ptr<int> value;explicit MoveKey(int x):value(std::make_unique<int>(x)){};MoveKey(MoveKey&&)noexcept=default;bool operator==(const MoveKey& x)const noexcept{return *value==*x.value;}};
struct MoveHash {std::uint64_t operator()(const MoveKey& x)const noexcept{return GenericHash(9)(*x.value);}};
template<class Key,class Hash>void random_case(const std::vector<Key>& keys,Hash hash){
 HashSet<Key,Hash> table(0,hash);std::set<Key> ref;std::mt19937_64 rng(1234);
 for(int i=0;i<100000;++i){const auto& key=keys[rng()%keys.size()];switch(rng()%12){
 case 0:case 1:{auto got=table.insert_checked(key);auto want=ref.insert(key);assert(got.second==want.second&&*got.first==*want.first);break;}
 case 2:table.erase(key,false);ref.erase(key);break;
 case 3:{auto got=table.find(key);assert((got!=table.end())==ref.contains(key));if(got!=table.end())assert(*got==key);break;}
 case 4:{auto copy=table;assert(copy==table);HashSet<Key,Hash> moved(std::move(copy));assert(copy.empty());copy=moved;assert(copy==table);break;}
 case 5:if(i%200==0)table.rehash(rng()%400);break;
 case 6:if(i%300==0){table.clear();ref.clear();}break;
 case 7:if(i%100==0){int count=0;auto got=table.erase_if([&](const Key& x){++count;return x==key;});assert(count==int(ref.size())&&got==ref.erase(key));}break;
 case 8:{auto got=table.erase(key);ref.erase(key);if(got!=table.end())assert(ref.contains(*got));break;}
 case 9:assert(table.count(key)==int(ref.count(key)));break;
 case 10:assert(table.contains(key)==ref.contains(key));break;
 case 11:if(i%200==0)table.reserve(rng()%400);break;
 }
 assert(table.size()==ref.size());if(i%100==0){assert(std::set<Key>(table.begin(),table.end())==ref);if(!table.empty()){auto it=table.end();--it;assert(ref.contains(*it));}}
 }
}
int main(){
 std::vector<int> keys;for(int i=-100;i<=100;++i)keys.push_back(i);random_case(keys,GenericHash(3));random_case(keys,Collision{});
 std::vector<std::string> words;for(int i:keys)words.push_back("prefix"+std::to_string(i));random_case(words,GenericHash(3));
 HashSet<int> empty(1u<<18,GenericHash(5)),other(0,GenericHash(5));const auto capacity=empty.capacity();assert(empty.begin()==empty.end()&&empty==other&&!empty.contains(17));int calls=0;assert(empty.erase_if([&](int){++calls;return true;})==0&&calls==0);empty.clear();empty.clear();assert(empty.capacity()==capacity);empty.insert(17);empty.erase(17,false);assert(empty.begin()==empty.end()&&!empty.contains(17)&&empty==other);assert(empty.erase_if([&](int){++calls;return true;})==0&&calls==0);empty.clear();empty.insert(9);assert(empty.contains(9)&&empty.size()==1);
 HashSet<Bomb,BombHash> bombs;for(int i=0;i<50;++i)bombs.emplace(i);auto cap=bombs.capacity();Bomb::budget=10;bool threw=false;try{bombs.reserve(1000);}catch(const std::runtime_error&){threw=true;}Bomb::budget=-1;assert(threw&&bombs.capacity()==cap&&bombs.size()==50);for(int i=0;i<50;++i)assert(bombs.contains(Bomb(i)));
 HashSet<int,ThrowHash> risky;for(int i=0;i<50;++i)risky.insert(i);ThrowHash::armed=true;threw=false;try{risky.reserve(1000);}catch(const std::runtime_error&){threw=true;}ThrowHash::armed=false;assert(threw&&risky.size()==50);for(int i=0;i<50;++i)assert(risky.contains(i));
 HashSet<MoveKey,MoveHash> moveonly;for(int i=0;i<50;++i)moveonly.emplace(i);moveonly.reserve(1000);for(int i=0;i<50;++i)assert(moveonly.contains(MoveKey(i)));auto moved=std::move(moveonly);assert(moveonly.empty());moveonly.emplace(80);assert(moveonly.contains(MoveKey(80))&&moved.size()==50);
 HashSet<const int> cv;cv.insert(3);auto copy=cv;assert(copy.contains(3));
#ifndef SKIP_RUNTIME_CHECKS
 auto it=empty.begin();empty.clear();threw=false;try{(void)*it;}catch(const std::out_of_range&){threw=true;}assert(threw);
#endif
 std::cout<<"PASS 300000 independent HashSet operations; empty/reserved/tombstones/copy/move/throwing keys\n";
}
