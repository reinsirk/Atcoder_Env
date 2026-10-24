// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <string>
#include <type_traits>
#include <vector>
#ifndef TRIE_SOURCE
#define TRIE_SOURCE "../../../DataStructure/Compressed_Binary_Trie.cpp"
#endif
#include TRIE_SOURCE
#ifndef TRIE_CLASS
#define TRIE_CLASS CompressedBinaryTrie
#endif
static std::uint64_t checks=0, operations=0, capacity_failures=0;
#define REQUIRE(x) do { ++checks; if(!(x)) { std::cerr << "failed line " << __LINE__ << " : " << #x << '\n'; std::abort(); } }while(false)
template<class T,int L,std::size_t N> void test(std::uint64_t seed,int steps=1800) {
 using U=std::make_unsigned_t<T>; using Tr=TRIE_CLASS<T,L,N>; using Entry=std::pair<U,int>;
 static_assert(!std::is_copy_constructible_v<Tr> && !std::is_move_constructible_v<Tr>);
 Tr tr; std::vector<Entry> m; U lazy=0; std::mt19937_64 rng(seed);
 constexpr U mask=[] { if constexpr(L+1==std::numeric_limits<U>::digits) return std::numeric_limits<U>::max(); else return static_cast<U>((U{1}<<(L+1))-1); }();
 auto ids=[&](U x) { std::multiset<int> a;for(auto [v,id]:m) if(v==x && id!=-1)a.insert(id);return a;};
 auto verify_ids=[&](U x,const auto& got){REQUIRE(std::multiset<int>(got.begin(),got.end())==ids(x));};
 auto less=[&](U x,std::uint64_t bound) {std::size_t c=0;for(auto[v,id]:m)c+=static_cast<std::uint64_t>(v^x)<bound;return c;};
 auto validate=[&] {
  REQUIRE(tr.size()==m.size());REQUIRE(tr.empty()==m.empty());
  std::vector<U> keys{0,mask,static_cast<U>(rng()&mask)}; for(auto[v,id]:m)keys.push_back(v);
  for(U x:keys){std::size_t c=0;for(auto[v,id]:m)c+=v==x;const auto r=tr.find(static_cast<T>(x));REQUIRE(r.first==c);verify_ids(x,r.second);REQUIRE(tr.count(static_cast<T>(x))==c);REQUIRE(tr.contains(static_cast<T>(x))==(c!=0));}
  for(U x:{U{0},mask,static_cast<U>(rng()&mask)}) {
   for(std::uint64_t b:{std::uint64_t{0},std::uint64_t{1},std::uint64_t{2},static_cast<std::uint64_t>(mask),std::numeric_limits<std::uint64_t>::max()}){REQUIRE(tr.count_xor_less(static_cast<T>(x),b)==less(x,b)); if(!x)REQUIRE(tr.count_less(b)==less(x,b));}
   REQUIRE(tr.count_xor_less(static_cast<T>(x),-7)==0);REQUIRE(tr.count_xor_less(static_cast<T>(x),0)==0);
   if(!m.empty()) {
    U mn=mask,mx=0;for(auto[v,id]:m){mn=std::min(mn,static_cast<U>(v^x));mx=std::max(mx,static_cast<U>(v^x));}
    auto a=tr.min_xor(static_cast<T>(x));auto b=tr.max_xor(static_cast<T>(x));REQUIRE(static_cast<U>(a.first)==mn);REQUIRE(static_cast<U>(b.first)==mx);verify_ids(mn^x,a.second);verify_ids(mx^x,b.second);
   }
  }
  REQUIRE(tr.count_less(-1)==0);REQUIRE(tr.count_less(false)==0);REQUIRE(tr.count_less(true)==less(0,1));
  if(!m.empty()){
   std::vector<U> sorted;for(auto[v,id]:m)sorted.push_back(v);std::sort(sorted.begin(),sorted.end());
   auto a=tr.min_element();auto b=tr.max_element();REQUIRE(static_cast<U>(a.first)==sorted.front());REQUIRE(static_cast<U>(b.first)==sorted.back());verify_ids(sorted.front(),a.second);verify_ids(sorted.back(),b.second);
   for(std::size_t k:{std::size_t{1},m.size(),std::size_t{1}+rng()%m.size()}){auto r=tr.get_kth(static_cast<std::int64_t>(k));REQUIRE(static_cast<U>(r.first)==sorted[k-1]);verify_ids(sorted[k-1],r.second);}
  }
 };
 auto logical_nodes=[&](U add){std::set<std::pair<int,U>> prefixes;for(auto[v,id]:m) for(int i=0;i<=L;++i)prefixes.emplace(i,static_cast<U>((v^lazy)>>i));for(int i=0;i<=L;++i)prefixes.emplace(i,static_cast<U>((add^lazy)>>i));return prefixes.size()+1;};
 for(int step=0;step<steps;++step){++operations;const int op=rng()%8;U x=static_cast<U>(rng()&mask);if(!m.empty() && rng()%2)x=m[rng()%m.size()].first;int id=rng()%3 ? static_cast<int>(rng()%11)-5 : -1;
  if(op<=2 && m.size()<100){const bool expected_failure=logical_nodes(x)>N;bool threw=false;try{tr.add(static_cast<T>(x),id);}catch(const std::length_error&){threw=true;++capacity_failures;}REQUIRE(threw==expected_failure);if(!threw)m.emplace_back(x,id);}
  else if(op<=5){if(op==5)id=-1;auto it=std::find_if(m.begin(),m.end(),[&](Entry e){return e.first==x && (id==-1 || e.second==id);});const bool expect=it!=m.end();auto before=ids(x);bool hadanon=std::any_of(m.begin(),m.end(),[&](Entry e){return e.first==x&&e.second==-1;});REQUIRE(tr.del(static_cast<T>(x),id)==expect);if(expect){if(id!=-1){m.erase(it);}else if(hadanon){m.erase(std::find(m.begin(),m.end(),Entry{x,-1}));}else{auto got=tr.find(static_cast<T>(x));std::multiset<int>after(got.second.begin(),got.second.end());std::vector<int>difference;std::set_difference(before.begin(),before.end(),after.begin(),after.end(),std::back_inserter(difference));REQUIRE(difference.size()==1);m.erase(std::find(m.begin(),m.end(),Entry{x,difference[0]}));}}}
  else if(op==6){tr.operate_xor(static_cast<T>(x));for(auto&e:m)e.first^=x;lazy^=x;}
  else if constexpr(requires { tr.insert_unique(static_cast<T>(x),id); }) {bool present=std::any_of(m.begin(),m.end(),[&](Entry e){return e.first==x;});bool limit=!present && logical_nodes(x)>N;bool threw=false;bool inserted=false;try{inserted=tr.insert_unique(static_cast<T>(x),id);}catch(const std::length_error&){threw=true;++capacity_failures;}REQUIRE(threw==limit);if(!threw){REQUIRE(inserted==!present);if(inserted)m.emplace_back(x,id);}}
  if(step%13==0)validate();
 }
 validate();while(!m.empty()){U x=m.back().first;REQUIRE(tr.del(static_cast<T>(x)));auto it=std::find_if(m.begin(),m.end(),[&](Entry e){return e.first==x&&e.second==-1;});if(it==m.end()){auto got=tr.find(static_cast<T>(x));auto before=ids(x);std::multiset<int>after(got.second.begin(),got.second.end());std::vector<int>d;std::set_difference(before.begin(),before.end(),after.begin(),after.end(),std::back_inserter(d));REQUIRE(d.size()==1);it=std::find(m.begin(),m.end(),Entry{x,d[0]});}m.erase(it);}validate();
 for(int rep=0;rep<100;++rep){U x=static_cast<U>(rng()&mask);if(L+2<=N){tr.add(static_cast<T>(x),rep);REQUIRE(tr.count(static_cast<T>(x))==1);REQUIRE(tr.del(static_cast<T>(x),rep));}}
#ifndef SKIP_RUNTIME_CHECKS
 auto throws=[&](auto fn){bool ok=false;try{fn();}catch(const std::out_of_range&){ok=true;}REQUIRE(ok);};
 throws([&]{(void)tr.min_element();});throws([&]{(void)tr.max_element();});throws([&]{(void)tr.min_xor(0);});throws([&]{(void)tr.max_xor(0);});throws([&]{(void)tr.get_kth(0);});throws([&]{(void)tr.get_kth(1);});
 if constexpr(std::is_signed_v<T>){throws([&]{tr.add(T{-1});});throws([&]{(void)tr.find(T{-1});});throws([&]{tr.operate_xor(T{-1});});}
 if constexpr(L+1<std::numeric_limits<T>::digits){T bad=static_cast<T>(U{1}<<(L+1));throws([&]{tr.add(bad);});throws([&]{(void)tr.count(bad);});throws([&]{(void)tr.contains(bad);});throws([&]{(void)tr.del(bad);});throws([&]{(void)tr.min_xor(bad);});throws([&]{(void)tr.count_xor_less(bad,5);});}
#endif
}
int main(){
 test<signed char,6,1024>(1);test<unsigned char,7,1024>(2);test<char,5,1024>(3);test<short,14,2048>(4);test<unsigned short,15,2048>(5);test<int,30,4096>(6);test<unsigned,31,4096>(7);test<long,std::numeric_limits<long>::digits-1,8192>(8);test<unsigned long,std::numeric_limits<unsigned long>::digits-1,8192>(9);test<std::int64_t,62,8192>(10);test<std::uint64_t,63,8192>(11);test<wchar_t,7,1024>(12);test<char8_t,7,1024>(13);test<char16_t,15,2048>(14);test<char32_t,31,4096>(15);
 test<unsigned,0,1>(16,300);test<unsigned,0,2>(17,300);test<unsigned,0,3>(18,300);test<unsigned,3,5>(19);test<unsigned,3,9>(20);test<unsigned,3,17>(21);test<unsigned,3,31>(22);test<std::uint64_t,63,64>(23,300);test<std::uint64_t,63,65>(24,300);test<unsigned,31,33>(25,300);test<unsigned,31,34>(26,300);test<unsigned,3,1000>(27,6000);
 std::cout<<"{\"checks\":"<<checks<<",\"operations\":"<<operations<<",\"capacity_failures\":"<<capacity_failures<<"}\n";
}
