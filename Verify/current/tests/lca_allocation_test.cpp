// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LCA_REQUIRE_EXTRA
#define SIRKIT_LCA_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_LCA_SOURCE
#define SIRKIT_LCA_SOURCE "../../../Graph/LCA.cpp"
#endif
#include SIRKIT_LCA_SOURCE
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

static std::ptrdiff_t failure_countdown = -1;
static std::size_t live_allocations = 0, checks = 0, failures = 0;
[[gnu::noinline]] void* operator new(std::size_t n) {
    if (failure_countdown == 0) throw std::bad_alloc();
    if (failure_countdown > 0) --failure_countdown;
    if (void* p = std::malloc(n ? n : 1)) { ++live_allocations; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] static void free_block(void* p) noexcept { if (p) --live_allocations; std::free(p); }
void operator delete(void* p) noexcept { free_block(p); }
void operator delete[](void* p) noexcept { free_block(p); }
void operator delete(void* p, std::size_t) noexcept { free_block(p); }
void operator delete[](void* p, std::size_t) noexcept { free_block(p); }
static void require(bool ok) { ++checks; if (!ok) std::abort(); }
using Graph = std::vector<std::vector<int>>;
static Graph make_graph(int n, bool forest) {
    Graph g(n); for(int v=1;v<n;++v) if(!forest || v%7) {int p=(v-1)/2;g[v].push_back(p);g[p].push_back(v);}return g;
}
template<class Tree> static void append_extra(std::vector<int>& out,const Tree& tree,int v) {
    if constexpr(requires { tree.subtree_size(v);tree.subtree_range(v);tree.order_index(v); }) {
        out.push_back(tree.subtree_size(v));out.push_back(tree.order_index(v));
        const auto [first,last]=tree.subtree_range(v);out.push_back(first);out.push_back(last);
    }
}
static std::vector<int> snapshot(const LCA& tree) {
    std::vector<int> result{static_cast<int>(tree.size()),tree.root(),static_cast<int>(tree.component_count())};
    for (int v=0;v<static_cast<int>(tree.size());++v) {
        result.push_back(tree.parent(v));result.push_back(tree.depth_of(v));result.push_back(tree.root_of(v));result.push_back(tree.component_of(v));
        result.push_back(tree.kth_ancestor(v,UINT64_MAX));append_extra(result,tree,v);
        for(int u=0;u<static_cast<int>(tree.size());++u) {result.push_back(tree.get(v,u));result.push_back(tree.get_distance(v,u));result.push_back(tree.is_ancestor(v,u));result.push_back(tree.kth_on_path(v,u,1));}
    }
    return result;
}
static void constructor_failures(const Graph& g,int root) {
    LCA reference(g,root);const auto expected=snapshot(reference);bool succeeded=false;
    for(std::ptrdiff_t point=0;point<256;++point) {
        const auto before=live_allocations;bool caught=false;
        try {failure_countdown=point;LCA tree(g,root);failure_countdown=-1;require(snapshot(tree)==expected);succeeded=true;}
        catch(const std::bad_alloc&) {failure_countdown=-1;caught=true;++failures;}
        require(live_allocations==before);
        if(!caught)break;
    }
    require(succeeded);
}
static void copy_failures(const LCA& source) {
    const auto expected=snapshot(source);bool succeeded=false;
    for(std::ptrdiff_t point=0;point<256;++point) {
        const auto before=live_allocations;bool caught=false;
        try {failure_countdown=point;LCA copy(source);failure_countdown=-1;require(snapshot(copy)==expected);succeeded=true;}
        catch(const std::bad_alloc&) {failure_countdown=-1;caught=true;++failures;}
        require(snapshot(source)==expected);require(live_allocations==before);if(!caught)break;
    }
    require(succeeded);
}
static void assignment_failures(const LCA& source,const Graph& original,int root) {
    const auto expected=snapshot(source);bool succeeded=false;
    for(std::ptrdiff_t point=0;point<256;++point) {
        const auto outside=live_allocations;bool caught=false;
        {
            LCA target(original,root);const auto previous=snapshot(target);const auto before=live_allocations;
            try {failure_countdown=point;target=source;failure_countdown=-1;require(snapshot(target)==expected);succeeded=true;}
            catch(const std::bad_alloc&) {failure_countdown=-1;caught=true;++failures;require(snapshot(target)==previous);require(live_allocations==before);}
            require(snapshot(source)==expected);
        }
        require(live_allocations==outside);if(!caught)break;
    }
    require(succeeded);
}
static void build_failures(const Graph& next,int root,const Graph& original) {
    LCA reference(next,root);const auto expected=snapshot(reference);bool succeeded=false;
    for(std::ptrdiff_t point=0;point<256;++point) {
        const auto outside=live_allocations;bool caught=false;
        {
            LCA target(original);const auto previous=snapshot(target);const auto before=live_allocations;
            try {failure_countdown=point;target.build(next,root);failure_countdown=-1;require(snapshot(target)==expected);succeeded=true;}
            catch(const std::bad_alloc&) {failure_countdown=-1;caught=true;++failures;require(snapshot(target)==previous);require(live_allocations==before);target.build(original);require(snapshot(target)==previous);}
        }
        require(live_allocations==outside);if(!caught)break;
    }
    require(succeeded);
}
int main() {
    static_assert(std::is_nothrow_move_constructible_v<LCA> && std::is_nothrow_move_assignable_v<LCA>);
    const auto initial=live_allocations;
    {
        const Graph empty,one(1),original=make_graph(9,false),forest=make_graph(73,true);
        Graph star(73),matching(74);
        for(int v=1;v<73;++v){star[0].push_back(v);star[v].push_back(0);}
        for(int v=0;v<74;v+=2){matching[v].push_back(v+1);matching[v+1].push_back(v);}
        constructor_failures(empty,0);constructor_failures(one,0);constructor_failures(forest,41);
        constructor_failures(star,0);constructor_failures(star,1);constructor_failures(matching,72);
        build_failures(star,0,forest);build_failures(forest,41,star);build_failures(matching,72,star);
        LCA shallow(star);copy_failures(shallow);assignment_failures(shallow,forest,41);
        LCA source(forest,41);copy_failures(source);assignment_failures(source,original,3);build_failures(forest,41,original);
        const auto expected=snapshot(source);const auto before=live_allocations;
        failure_countdown=0;
        LCA moved(std::move(source));source=std::move(moved);source.swap(source);
        failure_countdown=-1;require(snapshot(source)==expected);require(live_allocations==before);
        failure_countdown=0;source.clear();source.clear();failure_countdown=-1;require(source.empty()&&source.root()==-1);
        source.build(forest,41);require(snapshot(source)==expected);
    }
    require(live_allocations==initial);
    std::printf("{\"status\":\"PASS\",\"checks\":%zu,\"allocation_failure_points\":%zu,\"live_delta\":%zu}\n",checks,failures,live_allocations-initial);
}
