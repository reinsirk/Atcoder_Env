// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RWUF_REQUIRE_EXTRA
#define SIRKIT_RWUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_RWUF_SOURCE
#define SIRKIT_RWUF_SOURCE "../../../DataStructure/rollback-union-findw.cpp"
#endif
#include SIRKIT_RWUF_SOURCE
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <random>
#include <string>
#include <tuple>

using Big = boost::multiprecision::cpp_int;
static std::size_t checks=0,states=0,operations=0;
static void require_impl(bool ok,const char*what,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s\n",line,what);std::abort();}}
#define require(x) require_impl(bool(x),#x,__LINE__)
template<class E,class F>static void throws(F&&f){bool yes=false;try{f();}catch(const E&){yes=true;}require(yes);}
struct Modular {
 std::int64_t value=0;
 Modular(std::int64_t x=0):value((x%1000000007+1000000007)%1000000007){}
 friend Modular operator+(Modular a,Modular b){return Modular(a.value+b.value);}
 friend Modular operator-(Modular a,Modular b){return Modular(a.value-b.value);}
 friend bool operator==(Modular,Modular)=default;
};
struct AddOnly {
 std::int64_t value;
 AddOnly()=delete;explicit AddOnly(std::int64_t x):value(x){}
 friend AddOnly operator+(AddOnly a,AddOnly b){return AddOnly(a.value+b.value);}
 friend AddOnly operator-(AddOnly a,AddOnly b){return AddOnly(a.value-b.value);}
};
template<class T>static Big modulus(){if constexpr(std::is_integral_v<T>&&std::is_unsigned_v<T>)return Big(1)<<std::numeric_limits<T>::digits;else if constexpr(std::is_same_v<T,Modular>)return 1000000007;else return 0;}
template<class T>static Big norm(Big v){const auto mod=modulus<T>();if(mod!=0){v%=mod;if(v<0)v+=mod;}return v;}
template<class T>static T from_big(const Big&v){if constexpr(std::is_same_v<T,Modular>)return Modular(norm<T>(v).template convert_to<std::int64_t>());else return norm<T>(v).template convert_to<T>();}
template<class T>static Big as_big(const T&v){if constexpr(std::is_same_v<T,Modular>)return v.value;else return Big(v);}
struct Edge{int a,b;Big w;};
template<class T>struct Ref {
 int n,snap=0;std::vector<Edge>edges;std::vector<std::size_t>history;
 explicit Ref(int count=0):n(count){}
 std::vector<std::optional<Big>> from(int x)const{
  std::vector<std::optional<Big>>d(static_cast<std::size_t>(n));d[x]=0;std::deque<int>q;q.push_back(x);
  while(!q.empty()){int a=q.front();q.pop_front();for(const auto&e:edges){int b=-1;Big w;if(e.a==a){b=e.b;w=e.w;}else if(e.b==a){b=e.a;w=-e.w;}if(b>=0&&!d[b]){d[b]=norm<T>(*d[a]+w);q.push_back(b);}}}return d;
 }
 std::optional<Big> diff(int x,int y)const{return from(x)[y];}
 int merge(int x,int y,const T&w){const auto d=diff(x,y);history.push_back(edges.size());if(d)return *d==norm<T>(as_big(w))?1:2;edges.push_back({x,y,norm<T>(as_big(w))});return 0;}
 void undo(){edges.resize(history.back());history.pop_back();snap=std::min(snap,static_cast<int>(history.size()));}
 void rollback(int state=-1){if(state<0)state=snap;while(static_cast<int>(history.size())>state)undo();snap=std::min(snap,state);}
 int count()const{std::vector<bool>seen(static_cast<std::size_t>(n));int c=0;for(int i=0;i<n;++i)if(!seen[i]){++c;const auto d=from(i);for(int j=0;j<n;++j)if(d[j])seen[j]=true;}return c;}
 std::vector<std::vector<int>>groups()const{std::vector<std::vector<int>>v;std::vector<bool>seen(static_cast<std::size_t>(n));for(int i=0;i<n;++i)if(!seen[i]){v.emplace_back();const auto d=from(i);for(int j=0;j<n;++j)if(d[j]){seen[j]=true;v.back().push_back(j);}}return v;}
};
template<class T>static void observe(const RollbackWeightedUnionFind<T>&uf,const Ref<T>&r){
 ++states;require(uf.size()==r.n&&uf.empty()==(r.n==0));require(uf.get_state()==static_cast<int>(r.history.size()));require(uf.component_count()==r.count());
 auto groups=uf.groups();std::sort(groups.begin(),groups.end());require(groups==r.groups());
 for(int i=0;i<r.n;++i){const int root=uf.find(i);require(root>=0&&root<r.n);require(uf.find(root)==root);const auto d=r.from(i);int size=0;for(const auto&v:d)size+=v.has_value();require(uf.size(i)==size);const auto rw=uf.root_with_weight(i);require(rw.first==root);require(as_big(rw.second)==norm<T>(*r.diff(root,i)));require(as_big(uf.get_weight(i))==as_big(rw.second));
  for(int j=0;j<r.n;++j){require(uf.same(i,j)==d[j].has_value());const auto diff=uf.try_diff(i,j);require(diff.has_value()==d[j].has_value());if(d[j]){require(as_big(*diff)==norm<T>(*d[j]));require(as_big(uf.diff(i,j))==norm<T>(*d[j]));require(uf.consistent(i,j,from_big<T>(*d[j])));const T other=from_big<T>(*d[j]+1);require(!uf.consistent(i,j,other));}else require(uf.consistent(i,j,T(0)));}
 }
}

// 上位桁を含む多倍長整数の全体をBFS参照と照合する。
int main() {
 using UF = RollbackWeightedUnionFind<Big>;
 using R = UF::MergeResult;
 std::mt19937 rng(0x5107abU);
 const Big high=(Big(1)<<1024)+(Big(1)<<513)+(Big(1)<<256);
 for(int trial=0;trial<16;++trial){
  UF uf(12); Ref<Big> ref(12);
  auto add=[&](int a,int b,const Big&w,bool checked){
   ++operations;
   const int expected=ref.merge(a,b,w);
   if(checked){const auto status=uf.unite_checked(a,b,w);require(status==(expected==0?R::Merged:expected==1?R::AlreadyConsistent:R::Contradiction));}
   else require(uf.unite(a,b,w)==(expected==0));
   observe(uf,ref);
  };
  for(int i=0;i<12;i+=2)add(i,i+1,high+trial+i,true);
  add(0,2,-high*3+17,true);add(4,6,high*7-9,false);add(8,10,-high*11-5,true);
  add(0,4,high*13+3,true);add(0,8,-high*17+trial,true);
  const Big connected=*ref.diff(3,11);
  require(connected>high || connected< -high);
  add(3,11,connected,true);add(3,11,connected+1,true);
  uf.snapshot();ref.snap=static_cast<int>(ref.history.size());
  add(5,9,*ref.diff(5,9)+1,true);add(5,9,*ref.diff(5,9),false);
  uf.undo();ref.undo();observe(uf,ref);
  uf.rollback();ref.rollback();observe(uf,ref);
  uf.rollback(4);ref.rollback(4);observe(uf,ref);
  for(int step=0;step<120;++step){
   const unsigned op=rng()%9;
   if(op<5){
    int a=static_cast<int>(rng()%12),b=static_cast<int>(rng()%12);
    Big w=high*(static_cast<int>(rng()%31)-15)+(static_cast<int>(rng()%71)-35);
    if(op>=3){auto d=ref.diff(a,b);if(d)w=*d+(op==4?1:0);}
    add(a,b,w,op!=0);
   }else if(op==5&&!ref.history.empty()){uf.undo();ref.undo();}
   else if(op==6){int s=static_cast<int>(rng()%(ref.history.size()+1));uf.rollback(s);ref.rollback(s);}
   else if(op==7){uf.snapshot();ref.snap=static_cast<int>(ref.history.size());}
   else{uf.rollback();ref.rollback();}
   observe(uf,ref);
  }
  uf.rollback(0);ref.rollback(0);observe(uf,ref);
 }
 std::printf("PASS full cpp_int BFS checks=%zu states=%zu operations=%zu\n",checks,states,operations);
}
