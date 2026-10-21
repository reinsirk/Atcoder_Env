#include "../DataStructure/Node.cpp"
#include <iostream>
#include <utility>

int main() {
    auto head = make_chain(3);
    push_front_node(head);
    std::cout << chain_size(head) << '\n'; // 4
    pop_front_node(head);
    std::cout << (chain_nth(head, 2) == chain_tail(head)) << '\n'; // 1

    auto copy = clone_chain(head); // ノードを独立に複製する。
    copy = reverse_chain(std::move(copy)); // 循環なしが前提。
    std::cout << chain_size(copy) << '\n'; // 3

    auto tail = chain_tail(head);
    auto joined = make_chain(2, tail); // 末尾を共有する。
    clear_chain(head);
    std::cout << chain_size(joined) << '\n'; // 3
    clear_chain(joined); // tailからの所有は残る。

    auto cyclic = make_chain(3);
    auto entry = chain_nth(cyclic, 1);
    chain_tail(cyclic)->n = entry;
    std::cout << has_cycle(cyclic) << ' '
              << (cycle_entry(cyclic) == entry.get()) << '\n'; // 1 1
    break_cycle(cyclic); // 共有所有の循環を切ってから解放する。
    clear_chain(cyclic);
}
