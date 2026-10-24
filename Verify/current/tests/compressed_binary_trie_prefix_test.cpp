// SPDX-License-Identifier: CC0-1.0
#ifndef TRIE_SOURCE
#define TRIE_SOURCE "../../../DataStructure/Compressed_Binary_Trie.cpp"
#endif
#include TRIE_SOURCE
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>
static std::uint64_t checks=0;
static void require(bool ok){++checks;if(!ok)std::abort();}
template<class T,int L>void run() {
    using U=std::make_unsigned_t<T>;
    constexpr U mask=[] {if constexpr(L+1==std::numeric_limits<U>::digits)return std::numeric_limits<U>::max();else return static_cast<U>((U{1}<<(L+1))-1);}();
    constexpr U high=U{1}<<L;
    for(int prefix_bits:{0,1,2,3,5}) {
        const U lowmask=static_cast<U>(mask>>std::min(L,prefix_bits));
        const U prefix=static_cast<U>(mask&static_cast<U>(~lowmask));
        CompressedBinaryTrie<T,L> tree;std::vector<std::pair<U,int>> reference;
        const int distinct=std::min(64,static_cast<int>(std::min<std::uint64_t>(lowmask,63))+1);
        auto add=[&](U key,int id){tree.add(static_cast<T>(key),id);reference.emplace_back(key,id);};
        for(int i=0;i<distinct;++i){const U key=static_cast<U>(prefix|static_cast<U>(i));add(key,i);if(i%3==0)add(key,-1);if(i%5==0)add(key,i);}
        auto check=[&](U key) {
            std::size_t count=0;std::multiset<int> ids;
            for(auto [value,id]:reference)if(value==key){++count;if(id!=-1)ids.insert(id);}
            const auto found=tree.find(static_cast<T>(key));require(found.first==count);require(tree.count(static_cast<T>(key))==count);require(tree.contains(static_cast<T>(key))==(count!=0));require(std::multiset<int>(found.second.begin(),found.second.end())==ids);
        };
        auto validate=[&] {
            require(tree.size()==reference.size());require(tree.empty()==reference.empty());
            for(int i=0;i<256;++i)check(static_cast<U>(static_cast<U>(i)&mask));
            for(auto [key,id]:reference){static_cast<void>(id);check(key);check(static_cast<U>(key^high));check(static_cast<U>(key^mask));}
            std::vector<U> sorted;for(auto [key,id]:reference){static_cast<void>(id);sorted.push_back(key);}std::sort(sorted.begin(),sorted.end());
            for(std::size_t k=0;k<sorted.size();++k)require(static_cast<U>(tree.get_kth(static_cast<std::int64_t>(k+1)).first)==sorted[k]);
        };
        validate();
        // 偶数の代表値を消しても、内点の共通接頭辞は残る奇数に対して有効。
        for(int i=0;i<distinct;i+=2) {
            const U key=static_cast<U>(prefix|static_cast<U>(i));
            require(tree.del(static_cast<T>(key),i));
            reference.erase(std::find(reference.begin(),reference.end(),std::pair<U,int>{key,i}));
            if(i%5==0){require(tree.del(static_cast<T>(key),i));reference.erase(std::find(reference.begin(),reference.end(),std::pair<U,int>{key,i}));}
            if(i%3==0){require(tree.del(static_cast<T>(key)));reference.erase(std::find(reference.begin(),reference.end(),std::pair<U,int>{key,-1}));}
        }
        validate();
        for(U delta:{U{0},high,mask,static_cast<U>(mask/3)}) {
            tree.operate_xor(static_cast<T>(delta));for(auto&entry:reference)entry.first^=delta;validate();
            // 共通接頭辞の外で削除を試し、個数とIDが不変であることを確かめる。
            for(U query:{U{0},high,mask,static_cast<U>(prefix^high)}) {
                if(std::none_of(reference.begin(),reference.end(),[&](auto entry){return entry.first==query;})) {
                    const auto before=tree.size();require(!tree.del(static_cast<T>(query)));require(!tree.del(static_cast<T>(query),7));require(tree.size()==before);
                }
            }
        }
        while(!reference.empty()) {
            const auto [key,id]=reference.back();require(tree.del(static_cast<T>(key),id));reference.pop_back();
        }
        require(tree.empty());require(tree.count(0)==0);require(!tree.contains(0));require(!tree.del(0));
        for(int i=0;i<distinct;++i)tree.add(static_cast<T>(static_cast<U>(i)&mask),i);
        tree.reserve(1024);require(tree.size()==static_cast<std::size_t>(distinct));
        for(int i=0;i<distinct;++i)require(tree.count(static_cast<T>(static_cast<U>(i)&mask))==1);
    }
}
int main() {
    run<std::uint8_t,7>();run<std::int8_t,6>();run<std::uint16_t,15>();run<std::uint32_t,29>();run<std::int64_t,62>();run<std::uint64_t,63>();run<unsigned,0>();
    std::printf("PASS %llu compressed prefix/stale representative/lazy XOR/deletion/reuse checks\n",static_cast<unsigned long long>(checks));
}
