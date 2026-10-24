// SPDX-License-Identifier: CC0-1.0
#include "../../../DataStructure/implicit_treap.cpp"
#include <utility>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
struct Value {
 std::string text;
 static inline int remaining=-1;
 static void copied(){if(remaining==0)throw std::runtime_error("copy injected");if(remaining>0)--remaining;}
 explicit Value(std::string x):text(std::move(x)){}
 Value(const Value& other):text(other.text){copied();}
 Value& operator=(const Value& other){copied();text=other.text;return *this;}
 Value(Value&&)=default;Value& operator=(Value&&)=default;
};
struct Action {int shift; explicit Action(int x):shift(x){} };
Value op(Value a,Value b){a.text+=b.text;return a;}
Value e(){return Value("");}
Value mapping(Action f,Value a){for(auto& c:a.text)c=char('a'+(c-'a'+f.shift)%26);return a;}
Action compose(Action a,Action b){return Action((a.shift+b.shift)%26);}
Action id(){return Action(0);}
using Treap=lazy_implicit_treap<Value,op,e,Action,mapping,compose,id>;
void apply(std::vector<std::string>& v,int l,int r,int shift){for(int i=l;i<r;++i)for(auto& c:v[i])c=char('a'+(c-'a'+shift)%26);}
void check(Treap& tree,const std::vector<std::string>& ref){assert(tree.size()==int(ref.size()));std::string s;for(auto& x:ref)s+=x;assert(tree.all_prod().text==s);auto v=tree.to_vector();assert(v.size()==ref.size());for(std::size_t i=0;i<v.size();++i)assert(v[i].text==ref[i]);}
int main(){
 static_assert(!std::is_nothrow_copy_constructible_v<Value>);
 static_assert(!std::is_nothrow_copy_assignable_v<Value>);
 std::vector<Value> a,b;std::vector<std::string> ar,br;
 for(int i=0;i<150;++i){auto x=std::string(1,char('a'+i%26));a.emplace_back(x);ar.push_back(x);if(i<50){b.emplace_back(x);br.push_back(x);}}
 Treap source(a),original(b);original.reserve(500);
 source.apply(15,130,Action(3));apply(ar,15,130,3);source.reverse(1,149);std::reverse(ar.begin()+1,ar.begin()+149);
 original.erase_at(8);br.erase(br.begin()+8);original.insert_at(20,Value("b"));br.insert(br.begin()+20,"b");
 original.apply(3,48,Action(5));apply(br,3,48,5);original.reverse(0,50);std::reverse(br.begin(),br.end());
 int failures=0,successes=0;
 for(int budget=0;budget<470;++budget){
  Value::remaining=-1;Treap target=original;target.reserve(500);
  bool failed=false;Value::remaining=budget;
  try{target=source;}catch(const std::runtime_error&){failed=true;}
  Value::remaining=-1;
  auto expected=failed?br:ar;check(target,expected);
  if(failed)++failures;else++successes;
  target.apply(0,target.size(),Action(4));apply(expected,0,int(expected.size()),4);
  target.reverse(2,target.size()-1);std::reverse(expected.begin()+2,expected.end()-1);
  target.erase_at(7);expected.erase(expected.begin()+7);target.insert_at(3,Value("c"));expected.insert(expected.begin()+3,"c");check(target,expected);
 }
 Value::remaining=0;auto* self=&original;original=*self;Value::remaining=-1;check(original,br);check(source,ar);
 assert(failures>0&&successes>0);
 std::cout<<failures<<" injected failures preserved the destination; "<<successes<<" successful copies and self-assignment passed\n";
}
