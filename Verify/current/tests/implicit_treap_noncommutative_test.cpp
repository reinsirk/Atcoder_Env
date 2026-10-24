// SPDX-License-Identifier: CC0-1.0
#include "../../../DataStructure/implicit_treap.cpp"
#include <utility>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <string>
#include <vector>
struct Value {std::string text; explicit Value(std::string x):text(std::move(x)){} };
struct Action {int shift; explicit Action(int x):shift(x){} };
Value op(Value a,Value b){a.text+=b.text;return a;}
Value e(){return Value("");}
Value mapping(Action f,Value a){for(auto& c:a.text)c=char('a'+(c-'a'+f.shift)%26);return a;}
Action compose(Action a,Action b){return Action((a.shift+b.shift)%26);}
Action id(){return Action(0);}
using Treap=lazy_implicit_treap<Value,op,e,Action,mapping,compose,id>;
int main(){
 std::mt19937_64 rng(187612);
 for(int rep=0;rep<40;++rep){
  std::vector<std::string> ref;
  std::vector<Value> init;
  for(int i=0;i<32;++i){auto v=std::string(1,char('a'+rng()%26));ref.push_back(v);init.emplace_back(v);}
  Treap tree(init);
  for(int z=0;z<2500;++z){
   int n=int(ref.size()),l=int(rng()%(n+1)),r=int(rng()%(n+1));if(l>r)std::swap(l,r);
   int kind=int(rng()%9);
   if(kind==0||n==0){auto v=std::string(1,char('a'+rng()%26));ref.insert(ref.begin()+l,v);tree.insert_at(l,Value(v));}
   else if(kind==1){int pos=int(rng()%n);ref.erase(ref.begin()+pos);tree.erase_at(pos);}
   else if(kind==2){if(z%7==0){l=0;r=n;}else if(z%7==1)l=0;else if(z%7==2)r=n;std::reverse(ref.begin()+l,ref.begin()+r);tree.reverse(l,r);}
   else if(kind==3){int f=int(rng()%26);for(int i=l;i<r;++i)for(auto& c:ref[i])c=char('a'+(c-'a'+f)%26);tree.apply(l,r,Action(f));}
   else if(kind==4){int pos=int(rng()%n);auto v=std::string(1,char('a'+rng()%26));ref[pos]=v;tree.set(pos,Value(v));}
   else if(kind==5){std::string s;for(int i=l;i<r;++i)s+=ref[i];assert(tree.prod(l,r).text==s);}
   else if(kind==6){int m=l+int(rng()%(r-l+1));std::rotate(ref.begin()+l,ref.begin()+m,ref.begin()+r);tree.rotate(l,m,r);}
   else if(kind==7){int pos=int(rng()%n);assert(tree.get(pos).text==ref[pos]);}
   else {if(z%17==0){ref.erase(ref.begin()+l,ref.begin()+r);tree.erase(l,r);}else{Treap copy=tree;auto values=copy.to_vector();assert(values.size()==ref.size());for(int i=0;i<n;++i)assert(values[i].text==ref[i]);}}
   assert(tree.size()==int(ref.size()));
   if(z%101==0){
    int start=int(rng()%(ref.size()+1)),finish=int(rng()%(ref.size()+1)),limit=int(rng()%20);
    auto predicate=[&](Value x){return x.text.size()<=std::size_t(limit);};
    assert(tree.max_right(start,predicate)==std::min(int(ref.size()),start+limit));
    assert(tree.min_left(finish,predicate)==std::max(0,finish-limit));
   }
   if(z%23==0){std::string s;for(auto& x:ref)s+=x;assert(tree.all_prod().text==s);auto values=tree.to_vector();assert(values.size()==ref.size());for(std::size_t i=0;i<ref.size();++i)assert(values[i].text==ref[i]);}
  }
 }
 std::cout<<"100000 independent noncommutative/lazy/reverse/insert/erase mixed operations passed\n";
}
