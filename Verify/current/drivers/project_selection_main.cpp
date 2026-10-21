// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    int n{}; std::int64_t fee{}; verify_in >> n >> fee;
    TwoVariableSubmodularOpt<std::int64_t> model(n);
    for (int i = 0; i < n; ++i) {
        std::int64_t reward{}; verify_in >> reward;
        model.add_single_cost(i, 0, fee - reward);
    }
    for (int i = 0; i < n; ++i) {
        int k{}; verify_in >> k;
        while (k--) { int target{}; verify_in >> target; model.add_psp_constraint(target - 1, i); }
    }
    verify_out << -model.solve() << '\n';
}
