// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("Trie reference mismatch");};
    auto less=[](const std::string&a,const std::string&b){return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end(),[](char x,char y){return static_cast<unsigned char>(x)<static_cast<unsigned char>(y);});};
    std::map<std::string,std::size_t,decltype(less)> ref(less);Trie trie;std::mt19937 rng(918273);
    for(int step=0;step<1200;++step){std::string s;for(unsigned j=0,len=rng()%6;j<len;++j)s+=static_cast<char>(rng()%256);
        const unsigned mode=rng()%4,copies=rng()%4;auto it=ref.find(s);std::size_t count=it==ref.end()?0:it->second;
        if(mode==0){const auto result=trie.insert(s,copies);check(result==count+copies);if(copies)ref[s]+=copies;}
        else if(mode==1){check(trie.erase(s,copies)==(count>=copies));if(count>=copies&&copies){if(count==copies)ref.erase(s);else ref[s]-=copies;}}
        else if(mode==2){check(trie.erase_all(s)==count);ref.erase(s);}
        check(trie.count(s)==(ref.contains(s)?ref.at(s):0));check(trie.contains(s)==ref.contains(s));
        std::vector<std::string> sorted;std::vector<std::pair<std::string,std::size_t>> items;
        for(auto&[word,c]:ref){items.push_back({word,c});for(std::size_t j=0;j<c;++j)sorted.push_back(word);}
        check(trie.size()==sorted.size());check(trie.distinct_size()==ref.size());check(trie.items()==items);
        std::size_t below=0,prefix=0;for(auto&w:sorted){below+=less(w,s);prefix+=w.starts_with(s);}
        check(trie.count_less(s)==below);check(trie.prefix_count(s)==prefix);check(trie.starts_with(s)==(prefix!=0));
        const auto lb=trie.lower_bound(s);check(lb.has_value()==(below<sorted.size()));if(lb)check(*lb==sorted[below]);
        for(std::size_t j=0;j<sorted.size();++j)check(trie.kth(j)==sorted[j]);check(!trie.kth(sorted.size()));
        std::optional<std::size_t> longest;for(auto&[word,c]:ref)if(s.starts_with(word)&&(!longest||word.size()>*longest))longest=word.size();
        check(trie.longest_prefix(s)==longest);
    }
    Trie copy=trie;copy.clear();check(copy.empty());check(copy.size()==0);check(trie.size()!=0);
    Trie moved=std::move(trie);check(trie.empty());trie.insert("reuse");check(trie.contains("reuse"));
    moved.reserve_nodes(10000);moved.clear();check(moved.empty());
    {TrieNode root;root.children['x']=std::make_shared<TrieNode>();TrieNode copy=root;check(copy.children['x']==root.children['x']);}
    std::cout<<"OK\n";
}
