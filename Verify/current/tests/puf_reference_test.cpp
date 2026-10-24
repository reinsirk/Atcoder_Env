// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PUF_REQUIRE_EXTRA
#define SIRKIT_PUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_PUF_SOURCE
#define SIRKIT_PUF_SOURCE "../../../DataStructure/persistent-union-find.cpp"
#endif
#include SIRKIT_PUF_SOURCE
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <random>
#include <type_traits>
#include <string>
static std::size_t checks=0,versions_tested=0,operations=0;
static void demand(bool value,const char* expression,int line){++checks;if(!value){std::fprintf(stderr,"line %d: %s\n",line,expression);std::abort();}}
#define require(x) demand(bool(x),#x,__LINE__)
template<class E,class F>static void expect(F f){bool caught=false;try{f();}catch(const E&){caught=true;}require(caught);}
struct Model {
    std::vector<int> label;
    explicit Model(int n=0):label(static_cast<std::size_t>(n)){std::iota(label.begin(),label.end(),0);}
    bool same(int a,int b)const{return label[a]==label[b];}
    int size(int a)const{return static_cast<int>(std::count(label.begin(),label.end(),label[a]));}
    bool merge(int a,int b){const auto from=label[b],to=label[a];if(from==to)return false;for(auto& x:label)if(x==from)x=to;return true;}
    std::vector<std::vector<int>>groups()const{std::vector<std::vector<int>>g(label.size());for(std::size_t i=0;i<label.size();++i)g[label[i]].push_back(static_cast<int>(i));g.erase(std::remove_if(g.begin(),g.end(),[](const auto&v){return v.empty();}),g.end());std::sort(g.begin(),g.end());return g;}
};
static void inspect(const PersistentUnionFind& uf,PersistentUnionFind::Node* version,const Model& model,std::mt19937_64& random){
    ++versions_tested;const auto n=static_cast<int>(model.label.size());require(uf.size()==n);require(uf.component_count(version)==static_cast<int>(model.groups().size()));
    auto groups=uf.groups(version);for(const auto&g:groups)require(std::is_sorted(g.begin(),g.end()));std::sort(groups.begin(),groups.end());require(groups==model.groups());
    for(int i=0;i<n;++i){auto[root,negative]=uf.find_with_size(i,version);require(0<=root&&root<n);require(model.same(i,root));require(negative==-model.size(i));require(uf.find(i,version)==root);require(uf.root_of(i,version)==root);require(uf.leader(i,version)==root);require(uf.size(i,version)==model.size(i));}
    for(int i=0;i<128&&n;++i){const auto a=static_cast<int>(random()%n),b=static_cast<int>(random()%n);require(uf.same(a,b,version)==model.same(a,b));}
}
static void uf_cases(){
    std::mt19937_64 random(2026102201);
    for(const int n:{0,1,2,15,16,17,31,64,129,257}){
        PersistentUnionFind uf(static_cast<std::size_t>(n));auto initial=uf.get_root();std::vector<PersistentUnionFind::Node*>roots{initial};std::vector<Model>models{Model(n)};std::size_t current=0;
        inspect(uf,initial,models[0],random);
        for(int step=0;step<320&&n;++step){
            const auto base=static_cast<std::size_t>(random()%roots.size());const auto a=static_cast<int>(random()%n),b=static_cast<int>(random()%n);auto expected=models[base];const bool changed=expected.merge(a,b);const auto old=uf.get_root();bool result;
            switch(step%4){case 0:result=uf.unite(a,b,roots[base]);break;case 1:result=uf.merge(a,b,roots[base]);break;case 2:uf.restore(roots[base]);result=uf.unite(a,b);break;default:uf.restore(roots[base]);result=uf.merge(a,b);break;}
            require(result==changed);if(!changed)require(uf.get_root()==roots[base]);require(uf.root==uf.arr.root);require(old!=nullptr);roots.push_back(uf.get_root());models.push_back(expected);current=roots.size()-1;++operations;
            if(step%17==0){inspect(uf,roots.back(),expected,random);const auto previous=static_cast<std::size_t>(random()%roots.size());inspect(uf,roots[previous],models[previous],random);}
            if(step%23==0){auto copy=uf;const auto stored=uf.get_root();const bool actual=copy.unite(a,(b+1)%n);auto em=models[current];require(actual==em.merge(a,(b+1)%n));require(uf.get_root()==stored);inspect(copy,copy.get_root(),em,random);inspect(uf,stored,models[current],random);}
        }
        uf.restore(roots[current]);require(uf.get_root()==roots[current]);auto current_groups=uf.groups();std::sort(current_groups.begin(),current_groups.end());require(current_groups==models[current].groups());require(uf.component_count()==static_cast<int>(current_groups.size()));
        for(int i=0;i<n;++i){require(uf.find(i)==uf.find(i,uf.root));require(uf.root_of(i)==uf.find(i));require(uf.leader(i)==uf.find(i));require(uf.size(i)==models[current].size(i));require(uf.find_with_size(i)==uf.find_with_size(i,uf.root));require(uf.same(i,(i+1)%n)==models[current].same(i,(i+1)%n));}
        auto moved=std::move(uf);require(uf.size()==0&&uf.get_root()==nullptr);require(uf.groups().empty()&&uf.component_count()==0);inspect(moved,initial,models[0],random);
        PersistentUnionFind assigned(3);assigned=std::move(moved);require(moved.size()==0&&moved.root==nullptr);inspect(assigned,roots[current],models[current],random);auto& self_alias=assigned;assigned=self_alias;inspect(assigned,initial,models[0],random);
        PersistentUnionFind survivor;PersistentUnionFind::Node* retained;
        {PersistentUnionFind local(7);local.unite(2,5);retained=local.get_root();survivor=local;}require(survivor.same(2,5,retained));survivor.restore(retained);require(survivor.size(2)==2);
#ifndef SKIP_RUNTIME_CHECKS
        expect<std::out_of_range>([&]{(void)assigned.find(-1,initial);});expect<std::out_of_range>([&]{(void)assigned.find(n,initial);});
        PersistentUnionFind foreign(static_cast<std::size_t>(n));expect<std::invalid_argument>([&]{assigned.restore(foreign.root);});expect<std::invalid_argument>([&]{(void)assigned.component_count(foreign.root);});expect<std::invalid_argument>([&]{(void)assigned.groups(foreign.root);});
        if(n){expect<std::invalid_argument>([&]{assigned.restore(nullptr);});expect<std::invalid_argument>([&]{(void)assigned.same(0,0,nullptr);});expect<std::invalid_argument>([&]{(void)assigned.unite(0,0,nullptr);});}
        else{assigned.restore(nullptr);require(assigned.component_count()==0);require(assigned.groups().empty());}
#endif
    }
    expect<std::length_error>([]{PersistentUnionFind uf(static_cast<std::size_t>(std::numeric_limits<int>::max())+1);});
    PersistentUnionFind giant(static_cast<std::size_t>(std::numeric_limits<int>::max()));require(giant.size()==std::numeric_limits<int>::max());require(giant.size(giant.size()-1)==1);auto root=giant.root;require(giant.unite(0,giant.size()-1));require(giant.same(0,giant.size()-1));require(!giant.same(0,giant.size()-1,root));require(giant.size(0)==2);
}
struct NonDefault {int value;NonDefault()=delete;explicit NonDefault(int x):value(x){};friend bool operator==(const NonDefault&,const NonDefault&)=default;};
#ifdef SIRKIT_PUF_REQUIRE_EXTRA
struct CopyOnly {int value;CopyOnly()=delete;explicit CopyOnly(int x):value(x){};CopyOnly(const CopyOnly&)=default;CopyOnly(CopyOnly&&)=delete;CopyOnly&operator=(const CopyOnly&)=default;CopyOnly&operator=(CopyOnly&&)=delete;};
#endif
template<int shift>static void array_cases(){
    std::mt19937_64 random(20261022+shift);using Array=PersistentArray<int,shift>;const int n=(1<<shift)+3;Array array(n,-17);std::vector<typename Array::Node*>roots{array.root};std::vector<std::vector<int>> models{std::vector<int>(n,-17)};
    for(int step=0;step<160;++step){const auto base=static_cast<std::size_t>(random()%roots.size());const int k=static_cast<int>(random()%n),value=static_cast<int>(random()%20000)-10000;auto expected=models[base];expected[k]=value;
        if(step%3==0){array.restore(roots[base]);array.update(k,value);}else array.update(roots[base],k,value);
        roots.push_back(array.root);models.push_back(expected);for(int j=0;j<n;++j)require(array.get(j)==expected[j]);const auto old=static_cast<std::size_t>(random()%roots.size());for(int j=0;j<n;++j)require(array.get(roots[old],j)==models[old][j]);++operations;
    }
#ifdef SIRKIT_PUF_REQUIRE_EXTRA
    for(int step=0;step<80;++step){const auto base=static_cast<std::size_t>(random()%roots.size());const int a=static_cast<int>(random()%n),b=step%5==0?a:static_cast<int>(random()%n),va=10000+step,vb=-10000-step;auto expected=models[base];expected[a]=va;expected[b]=vb;
        if(step%2){array.restore(roots[base]);array.update_two(a,va,b,vb);}else array.update_two(roots[base],a,va,b,vb);
        roots.push_back(array.root);models.push_back(expected);for(int j=0;j<n;++j)require(array.get(j)==expected[j]);for(int j=0;j<n;++j)require(array.get(roots[base],j)==models[base][j]);++operations;
    }
    array.update_two(nullptr,0,101,n-1,102);require(array.get(0)==101&&array.get(n-1)==102);array.update_two(0,103,0,104);require(array.get(0)==104);
#ifndef SKIP_RUNTIME_CHECKS
    const auto pair_before=array.root;expect<std::out_of_range>([&]{array.update_two(0,1,n,2);});require(array.root==pair_before);expect<std::out_of_range>([&]{array.update_two(n,1,0,2);});require(array.root==pair_before);
#endif
    array.restore(roots.back());
#endif
    auto old=array.root;array.update(nullptr,n-1,13);require(array.get(n-1)==13&&array.get(0)==-17);require(array.get(old,0)==models.back()[0]);array.restore(nullptr);require(array.root==nullptr&&array.get(n-1)==-17);
    auto copy=array;copy.update(0,45);require(array.get(0)==-17);auto retained=copy.root;array=copy;copy=Array(0,-17);require(array.get(retained,0)==45);
    auto moved=std::move(array);require(array.size()==0&&array.root==nullptr);Array assigned;assigned=std::move(moved);require(moved.size()==0&&moved.root==nullptr);require(assigned.get(retained,0)==45);
    const std::vector<int> input{3,1,4,1,5,9};Array from_values(input,-2);for(std::size_t i=0;i<input.size();++i)require(from_values.get(static_cast<std::int64_t>(i))==input[i]);
#ifdef SIRKIT_PUF_REQUIRE_EXTRA
    PersistentArray<CopyOnly,shift> copy_only(4,CopyOnly(-8));const auto copy_only_old=copy_only.root;const CopyOnly copy_only_value(6);copy_only.update(1,copy_only_value);require(copy_only.get(1).value==6&&copy_only.get(copy_only_old,1).value==-8);copy_only.update_two(0,copy_only_value,2,copy_only_value);require(copy_only.get(0).value==6&&copy_only.get(2).value==6);std::vector<CopyOnly> copy_values;copy_values.emplace_back(1);copy_values.emplace_back(2);PersistentArray<CopyOnly,shift> copy_constructed(copy_values,CopyOnly(-4));require(copy_constructed.get(0).value==1&&copy_constructed.get(1).value==2);
#endif
    std::vector<NonDefault> values;values.emplace_back(8);values.emplace_back(9);PersistentArray<NonDefault,shift> generic(values,NonDefault(-1));auto generic_old=generic.root;generic.update(0,NonDefault(4));require(generic.get(0).value==4&&generic.get(generic_old,0).value==8);
#ifdef SIRKIT_PUF_REQUIRE_EXTRA
    generic.update_two(generic_old,0,NonDefault(14),1,NonDefault(15));require(generic.get(0).value==14&&generic.get(1).value==15&&generic.get(generic_old,0).value==8);generic.update_two(0,NonDefault(16),0,NonDefault(17));require(generic.get(0).value==17);
#endif
    generic.update(nullptr,1,NonDefault(3));require(generic.get(0).value==-1&&generic.get(1).value==3);
    Array huge(std::numeric_limits<std::int64_t>::max(),-3);const std::array<std::int64_t,7> indexes{0,1,(std::int64_t{1}<<31)-1,std::int64_t{1}<<31,(std::int64_t{1}<<62)-1,std::int64_t{1}<<62,std::numeric_limits<std::int64_t>::max()-1};auto before=huge.root;
#ifdef SIRKIT_PUF_REQUIRE_EXTRA
    huge.update_two(indexes.front(),18,indexes.back(),19);require(huge.get(indexes.front())==18&&huge.get(indexes.back())==19);require(huge.get(before,indexes.front())==-3&&huge.get(before,indexes.back())==-3);huge.restore(before);
#endif
    for(std::size_t i=0;i<indexes.size();++i){huge.update(indexes[i],static_cast<int>(i));}
    for(std::size_t i=0;i<indexes.size();++i){require(huge.get(indexes[i])==static_cast<int>(i));require(huge.get(before,indexes[i])==-3);}require(huge.get(nullptr,indexes.back())==-3);
    expect<std::length_error>([]{Array negative(-1);});
#ifndef SKIP_RUNTIME_CHECKS
    expect<std::out_of_range>([&]{(void)assigned.get(-1);});expect<std::out_of_range>([&]{(void)assigned.get(n);});expect<std::out_of_range>([&]{assigned.update(n,3);});Array foreign(n);expect<std::invalid_argument>([&]{assigned.restore(foreign.root);});expect<std::invalid_argument>([&]{(void)assigned.get(foreign.root,0);});expect<std::invalid_argument>([&]{assigned.update(foreign.root,0,3);});
#endif
}
int main(){uf_cases();array_cases<1>();array_cases<2>();array_cases<3>();array_cases<4>();array_cases<5>();array_cases<6>();array_cases<7>();array_cases<8>();std::printf("PASS checks=%zu versions=%zu operations=%zu\n",checks,versions_tested,operations);}
