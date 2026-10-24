// SPDX-License-Identifier: CC0-1.0
#include "../Graph/Project_Selection.cpp"
#include <cassert>
#include <cstdint>
#include <utility>
#include <iostream>

int main() {
    TwoVariableSubmodularOpt<std::int64_t> model(3);
    model.reserve_nodes(10);
    model.add_single_cost(0, 0, -5); // x0を選ぶと利得5。
    model.add_single_cost(1, 0, 2);  // x1を選ぶと費用2。
    model.add_single_cost(2, 0, -3);
    model.add_psp_constraint(0, 1); // x0を選ぶならx1も選ぶ。
    model.force(2, false);
    auto [cost, selected] = model.solve_with_assignment();
    std::cout << cost << '\n'; // -3
    for (bool value : selected) std::cout << value << ' ';
    std::cout << '\n'; // 1 1 0
    std::cout << model.solve() << '\n'; // -3（繰り返しも同じ値）
    model.add_single_cost(1, 0, 10); // 解いた後にも項を追加できる。
    std::cout << model.solve() << '\n'; // 0
    model.clear();
    model.add_equality(0, 1);
    model.add_constant(7);
    model.add_all_true_profit({}, 2); // 空集合の条件は常に成立。
    std::cout << model.solve() << ' ' << model.variable_count() << '\n'; // 5 3
    model.clear(); // 変数と辺配列の領域を再利用。
    model.add_any_true_cost({0, 1}, 4);
    model.add_any_false_cost({1, 2}, 3);
    model.force(0, true);
    model.force(2, false);
    std::cout << model.solve() << '\n'; // 7
    TwoVariableSubmodularOpt<std::int64_t> other;
    swap(model, other);
    assert(model.empty() && other.variable_count() == 3);
}
