// SPDX-License-Identifier: CC0-1.0
#include <iostream>
#include <string>
int main(){
    std::ios::sync_with_stdio(false);std::cin.tie(nullptr);
    int q;std::cin>>q;Trie trie;
    while(q--){std::string op,s;std::cin>>op>>s;if(op=="insert")trie.insert(s);else std::cout<<(trie.contains(s)?"yes":"no")<<'\n';}
}
