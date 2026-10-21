#include "../DataStructure/implicit_treap.cpp"
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

using SumTreap = lazy_implicit_treap<
    RangeAddSum::S, RangeAddSum::op, RangeAddSum::e, RangeAddSum::F,
    RangeAddSum::mapping, RangeAddSum::composition, RangeAddSum::id>;

int main() {
    SumTreap sequence(std::vector<RangeAddSum::S>{{1, 1}, {2, 1}, {3, 1}, {4, 1}});
    sequence.apply(1, 3, 10);
    sequence.reverse(0, 4);
    sequence.set(1, {5, 1});
    sequence.insert_at(2, {7, 1});
    sequence.erase_at(4);
    sequence.rotate(0, 1, 4); // [4,5,7,12] -> [5,7,12,4]。
    std::cout << sequence.prod(0, 3).sum << ' ' << sequence.all_prod().sum << '\n'; // 24 28
    std::cout << sequence.max_right(0, [](RangeAddSum::S x) { return x.sum <= 12; }) << '\n'; // 2
    std::cout << sequence.min_left(4, [](RangeAddSum::S x) { return x.sum <= 16; }) << '\n'; // 2
    for (auto x : sequence.to_vector()) std::cout << x.sum << ' '; // 5 7 12 4
    std::cout << '\n';
    SumTreap copy = sequence;
    SumTreap moved = std::move(copy);
    std::cout << copy.empty() << ' ' << moved.size() << '\n'; // 1 4

    lazy_implicit_treap<> ordered;
    ordered.reserve(10);
    for (std::int64_t x : {3, 1, 3, 2}) ordered.insert_sorted(x);
    std::cout << ordered.get_rank(3) << ' ' << ordered.count(3) << ' ' << ordered.contains(2) << '\n'; // 2 2 1
    std::cout << ordered.lower_bound(3) << ' ' << ordered.upper_bound(3) << '\n'; // 2 4
    ordered.erase_val(3);
    ordered.erase_max();
    std::cout << ordered.size() << ' ' << ordered.all_prod() << '\n'; // 2 1
}
