// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LCA_SOURCE
#define SIRKIT_LCA_SOURCE "../../../Graph/LCA.cpp"
#endif
#include SIRKIT_LCA_SOURCE
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>
static std::uint64_t checks=0;
static void require(bool ok){++checks;if(!ok)std::abort();}
using Graph=std::vector<std::vector<int>>;
static void test(int n,int width) {
    Graph graph(n);
    for(int v=0;v<n;++v)if(v%width){const int p=v/width*width;graph[p].push_back(v);graph[v].push_back(p);}
    const int requested=n?(n-1)/width*width:0;
    LCA tree(graph,requested);
    require(tree.size()==static_cast<std::size_t>(n));
    require(tree.component_count()==static_cast<std::size_t>((n+width-1)/width));
    std::vector<int> used(n);
    for(int v=0;v<n;++v) {
        const int root=v/width*width;const int depth=v==root?0:1;
        require(tree.parent(v)==(v==root?-1:root));require(tree.depth_of(v)==depth);require(tree.root_of(v)==root);
        require(tree.kth_ancestor(v,0)==v);require(tree.kth_ancestor(v,1)==(depth?root:-1));require(tree.kth_ancestor(v,2)==-1);require(tree.kth_ancestor(v,UINT64_MAX)==-1);
        const auto [entry,end]=tree.subtree_range(v);require(entry>=0&&end<=n&&entry<end);++used[entry];
        require(tree.subtree_size(v)==(depth?1:std::min(width,n-root)));
        for(const int u:{v,root,(v+1)%n,n-1}) {
            const bool same=u/width==v/width;const int answer=!same?-1:u==v?v:root;
            require(tree.get(u,v)==answer);require(tree.get_distance(u,v)==(!same?-1:u==v?0:(u==root||v==root)?1:2));
            require(tree.get(u,v,root)==answer);require(tree.get(v,v,u)==(same?v:-1));
            require(tree.is_ancestor(u,v)==(u==v||(same&&u==root)));
        }
    }
    for(const int x:used)require(x==1);
    LCA copy=tree;tree.clear();require(tree.empty());
    LCA moved=std::move(copy);require(copy.empty());
    tree.build(graph,requested);require(tree.component_count()==moved.component_count());
    for(int v=0;v<n;++v)require(tree.parent(v)==moved.parent(v));
    // 星の葉を根にすると深さ2となり、一般構築との境界を通る。
    if(n>=3&&width>=3){LCA leaf_root(graph,1);require(leaf_root.parent(0)==1);require(leaf_root.depth_of(2)==2);require(leaf_root.get(1,2)==1);require(leaf_root.get(0,2)==0);}
}
int main() {
    for(int n:{0,1,2,3,31,32,33,63,64,65,257,65536,200000})
        for(int width:{1,2,3,31,32,33,257,200000})test(n,width);
    std::printf("PASS %llu shallow construction checks; isolated, matching, block stars, root changes, copy/move/rebuild\n",static_cast<unsigned long long>(checks));
}
