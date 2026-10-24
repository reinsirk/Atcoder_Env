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
template<class T>static void random_cases(std::uint32_t seed){
 std::mt19937 rng(seed);using UF=RollbackWeightedUnionFind<T>;using R=typename UF::MergeResult;
 for(int trial=0;trial<20;++trial){int n=trial%14;UF uf(n);Ref<T>r(n);uf.reserve_history(512);observe(uf,r);
  for(int step=0;step<180;++step){++operations;int op=static_cast<int>(rng()%12);if(n==0)op=9;
   if(op<=4){int a=static_cast<int>(rng()%static_cast<unsigned>(n)),b=static_cast<int>(rng()%static_cast<unsigned>(n));T w=from_big<T>(Big(static_cast<int>(rng()%5)-2));if(op==0){const auto old=r.diff(a,b);bool merged=uf.unite(a,b,w);require(merged==!old);r.merge(a,b,w);}else{const int expected=r.merge(a,b,w);R got=uf.unite_checked(a,b,w);require(got==(expected==0?R::Merged:expected==1?R::AlreadyConsistent:R::Contradiction));}}
   else if(op==5&&!r.history.empty()){uf.undo();r.undo();}
   else if(op==6){int s=static_cast<int>(rng()%(r.history.size()+1));uf.rollback(s);r.rollback(s);}
   else if(op==7){uf.snapshot();r.snap=static_cast<int>(r.history.size());}
   else if(op==8){uf.rollback();r.rollback();}
   else if(op==9){auto copy=uf;observe(copy,r);auto moved=std::move(copy);require(copy.empty()&&copy.get_state()==0);observe(moved,r);UF target(1);target=moved;observe(target,r);auto*alias=&target;target=*alias;target=std::move(*alias);observe(target,r);UF empty;empty.swap(target);observe(empty,r);require(target.empty());}
   else if(op==10){uf.reserve_history(512);}
   else if(op==11){
#ifdef SIRKIT_RWUF_REQUIRE_EXTRA
    if(step%2){uf.clear_history();r.history.clear();r.snap=0;}
    else
#endif
    {uf.clear();r=Ref<T>();n=0;uf.reset(trial%14);n=trial%14;r=Ref<T>(n);}
   }
   if(step%7==0)observe(uf,r);
  }observe(uf,r);
 }
}
template<class T>static void signed_boundaries(){
 using UF=RollbackWeightedUnionFind<T>;using R=typename UF::MergeResult;constexpr T lo=std::numeric_limits<T>::min(),hi=std::numeric_limits<T>::max();
 const std::vector<T>values={lo,static_cast<T>(lo+1),static_cast<T>(lo/2),T(-1),T(0),T(1),static_cast<T>(hi/2),static_cast<T>(hi-1),hi};
 for(T a:values)for(T b:values){UF u(3);require(u.unite(0,1,a));require(u.unite(0,2,b));const Big difference=Big(b)-Big(a);const bool fits=difference>=Big(lo)&&difference<=Big(hi);if(fits){const T expected=difference.template convert_to<T>();require(u.diff(1,2)==expected);require(u.try_diff(1,2)==expected);require(u.consistent(1,2,expected));require(u.unite_checked(1,2,expected)==R::AlreadyConsistent);}else{throws<std::overflow_error>([&]{(void)u.diff(1,2);});throws<std::overflow_error>([&]{(void)u.try_diff(1,2);});require(!u.consistent(1,2,T(0)));require(u.unite_checked(1,2,T(0))==R::Contradiction);}require(u.get_state()==3);require(!u.unite(1,2,T(0)));require(u.get_state()==4);u.rollback(0);require(u.component_count()==3);}
 {UF u(3);u.unite(0,1,T(0));throws<std::overflow_error>([&]{u.unite(2,0,lo);});require(u.get_state()==1&&!u.same(0,2));require(u.unite(2,0,hi));require(u.diff(2,0)==hi);}
}
static void boundaries(){
 using T=std::int64_t;using UF=RollbackWeightedUnionFind<T>;using R=UF::MergeResult;constexpr T lo=std::numeric_limits<T>::min(),hi=std::numeric_limits<T>::max();
 {UF u(6);require(u.unite(0,1,hi));require(u.unite(0,2,hi-1));require(u.diff(1,2)==-1);require(u.consistent(1,2,-1));require(u.unite_checked(1,2,-1)==R::AlreadyConsistent);require(u.unite_checked(1,2,hi)==R::Contradiction);require(u.get_state()==4);u.undo();u.rollback(2);require(u.unite(3,4,lo));require(u.diff(3,4)==lo);throws<std::overflow_error>([&]{(void)u.diff(4,3);});require(!u.consistent(4,3,lo));const int s=u.get_state();throws<std::overflow_error>([&]{u.unite(1,5,1);});require(u.get_state()==s&&!u.same(1,5));u.rollback(0);require(u.component_count()==6);}
 {UF u(8);require(u.unite(0,1,hi));require(u.unite(0,6,0));require(u.unite(0,7,0));require(u.unite(2,3,hi));require(u.unite(4,5,-hi));require(u.unite(2,4,hi));require(u.unite(0,2,1));require(u.diff(0,5)==1);require(u.diff(1,3)==1);require(u.try_diff(0,5)==1);require(u.consistent(0,5,1));throws<std::overflow_error>([&]{(void)u.get_weight(3);});throws<std::overflow_error>([&]{(void)u.root_with_weight(3);});throws<std::overflow_error>([&]{(void)u.diff(0,3);});require(u.diff(3,5)==-hi);require(u.unite_checked(0,5,1)==R::AlreadyConsistent);u.undo();require(u.get_state()==7);u.rollback(3);require(u.component_count()==5);}
 {UF u(4);require(u.unite(0,1,1));u.snapshot();for(int i=0;i<4000;++i)require(u.unite_checked(0,1,i%2?1:2)==(i%2?R::AlreadyConsistent:R::Contradiction));require(u.get_state()==4001);u.rollback(2001);u.snapshot();u.undo();require(u.get_state()==2000);u.rollback();require(u.get_state()==2000);u.rollback(1);require(u.diff(0,1)==1);u.unite(2,3,-4);u.rollback();require(u.get_state()==1&&!u.same(2,3));}
 {UF u(0);require(u.groups().empty());throws<std::length_error>([&]{u.reserve_history(static_cast<std::size_t>(std::numeric_limits<int>::max())+1);});
#ifndef SKIP_RUNTIME_CHECKS
  throws<std::invalid_argument>([]{UF bad(-1);});throws<std::out_of_range>([&]{u.undo();});throws<std::out_of_range>([&]{u.rollback(1);});throws<std::out_of_range>([&]{u.rollback(-2);});
  for(int x:{-1,0,1}){throws<std::out_of_range>([&]{(void)u.find(x);});throws<std::out_of_range>([&]{(void)u.size(x);});throws<std::out_of_range>([&]{(void)u.same(x,x);});throws<std::out_of_range>([&]{(void)u.root_with_weight(x);});throws<std::out_of_range>([&]{(void)u.get_weight(x);});throws<std::out_of_range>([&]{(void)u.try_diff(x,x);});throws<std::out_of_range>([&]{(void)u.diff(x,x);});throws<std::out_of_range>([&]{(void)u.consistent(x,x,0);});throws<std::out_of_range>([&]{(void)u.unite(x,x,0);});throws<std::out_of_range>([&]{(void)u.unite_checked(x,x,0);});}
  UF a(2);throws<std::invalid_argument>([&]{(void)a.diff(0,1);});for(int x:{-1,2}){throws<std::out_of_range>([&]{(void)a.same(0,x);});throws<std::out_of_range>([&]{(void)a.unite(0,x,0);});throws<std::out_of_range>([&]{(void)a.try_diff(x,0);});}const int s=a.get_state();throws<std::invalid_argument>([&]{a.reset(-1);});require(a.size()==2&&a.get_state()==s);
#endif
 }
#ifdef SIRKIT_RWUF_REQUIRE_EXTRA
 {UF u(4);u.unite(0,1,hi);u.snapshot();u.unite(2,3,-7);u.clear_history();require(u.get_state()==0&&u.component_count()==2&&u.diff(0,1)==hi);u.rollback();require(u.get_state()==0&&u.diff(2,3)==-7);u.unite(1,2,-hi);require(u.diff(0,2)==0);u.undo();require(!u.same(0,2)&&u.same(2,3));static_assert(noexcept(u.clear_history()));}
#endif
 {RollbackWeightedUnionFind<AddOnly>u(3);require(u.unite(0,1,AddOnly(4)));require(u.unite(1,2,AddOnly(-7)));require(u.diff(0,2).value==-3);require(u.try_diff(0,2)->value==-3);require(u.get_weight(1).value==4);u.undo();require(!u.same(1,2));auto copy=u;copy.rollback(0);require(copy.component_count()==3&&u.component_count()==2);}
}
int main(){random_cases<std::int8_t>(1);random_cases<std::int16_t>(2);random_cases<std::int32_t>(3);random_cases<std::int64_t>(4);random_cases<std::uint8_t>(5);random_cases<std::uint64_t>(6);random_cases<Modular>(7);signed_boundaries<std::int8_t>();signed_boundaries<std::int16_t>();signed_boundaries<std::int32_t>();signed_boundaries<std::int64_t>();boundaries();std::printf("PASS checks=%zu states=%zu operations=%zu\n",checks,states,operations);}
