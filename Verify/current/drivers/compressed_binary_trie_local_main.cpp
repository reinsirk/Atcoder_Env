// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>

int main() {
    const auto require=[](bool ok) { if(!ok)throw std::runtime_error("trie reference mismatch"); };
    std::mt19937 random(57623);
    CompressedBinaryTrie<std::uint16_t,7> trie;std::multiset<std::uint16_t> model;
    for(int step=0;step<5000;++step) {
        const std::uint16_t x=random()%256;
        switch(random()%4) {
        case 0: case 1:trie.add(x);model.insert(x);break;
        case 2: {
            const auto it=model.find(x);const bool found=it!=model.end();require(trie.del(x)==found);
            if(found)model.erase(it);break;
        }
        default: {
            trie.operate_xor(x);std::multiset<std::uint16_t> next;
            for(auto y:model)next.insert(y^x);model.swap(next);break;
        }
        }
        require(trie.size()==model.size() && trie.empty()==model.empty());
        require(trie.count(x)==model.count(x) && trie.contains(x)==model.contains(x));
        require(trie.find(x).first==model.count(x));
        const int upper=static_cast<int>(random()%260)-2;
        require(trie.count_less(upper)==static_cast<std::size_t>(std::count_if(model.begin(),model.end(),[&](auto y){return y<upper;})));
        require(trie.count_xor_less(x,upper)==static_cast<std::size_t>(std::count_if(model.begin(),model.end(),[&](auto y){return (x^y)<upper;})));
        if(!model.empty()) {
            require(trie.min_element().first==*model.begin());require(trie.max_element().first==*model.rbegin());
            const std::size_t k=random()%model.size();require(trie.get_kth(k+1).first==*std::next(model.begin(),k));
            std::uint16_t lo=255,hi=0;for(auto y:model) {lo=std::min<std::uint16_t>(lo,x^y);hi=std::max<std::uint16_t>(hi,x^y);}
            require(trie.min_xor(x).first==lo && trie.max_xor(x).first==hi);
        }
    }
    CompressedBinaryTrie<std::uint32_t,31> ids;ids.add(7,10);ids.add(7,10);ids.add(7,20);ids.add(7);
    require(ids.count(7)==4 && ids.find(7).second.size()==3);
    require(ids.del(7,10) && ids.count(7)==3 && ids.find(7).second.size()==2);
    ids.operate_xor(0xffffffffU);require(ids.contains(0xfffffff8U));
    require(ids.count_less(std::uint64_t{1}<<32)==3);
    require(ids.count_xor_less(0xffffffffU,std::uint64_t{1}<<32)==3);
    CompressedBinaryTrie<std::uint32_t, 29> unique;
    unique.reserve(8);
    require(unique.insert_unique(9, 42));
    require(!unique.insert_unique(9, 99));
    require(unique.size() == 1 && unique.find(9).second == std::vector<int>{42});
    std::cout<<"OK\n";
}
