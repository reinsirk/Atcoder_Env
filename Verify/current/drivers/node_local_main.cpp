// SPDX-License-Identifier: CC0-1.0
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("Node reference mismatch");};
    for(std::size_t n=0;n<80;++n){
        auto head=make_chain(n);check(chain_size(head)==n);check(!has_cycle(head));
        std::vector<std::shared_ptr<Node>> reference;for(auto p=head;p;p=p->n)reference.push_back(p);
        for(std::size_t i=0;i<=n;++i)check(chain_nth(head,i)==(i<n?reference[i]:nullptr));
        check(chain_tail(head)==(n?reference.back():nullptr));
        head=reverse_chain(head);for(std::size_t i=0;i<n;++i)check(chain_nth(head,i)==reference[n-1-i]);
        push_front_node(head);check(chain_size(head)==n+1);check(pop_front_node(head));
        clear_chain(head);check(!head);reference.clear();
    }
    auto head=make_chain(50);auto tail=chain_tail(head);auto entry=chain_nth(head,17);tail->n=entry;
    check(has_cycle(head));check(cycle_entry(head)==entry.get());check(break_cycle(head));check(!has_cycle(head));
    check(chain_size(head)==50);clear_chain(head);entry.reset();tail.reset();
    {auto long_chain=make_chain(300000);check(chain_size(long_chain)==300000);}
    std::shared_ptr<Node> empty;check(!pop_front_node(empty));check(!break_cycle(empty));
    std::cout<<"OK\n";
}
