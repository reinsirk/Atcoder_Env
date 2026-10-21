// SPDX-License-Identifier: CC0-1.0
// 参考: https://drken1215.hatenablog.com/entry/2023/11/24/034300

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_TWO_VARIABLE_SUBMODULAR_OPT_DEFINED
#define SIRKIT_TWO_VARIABLE_SUBMODULAR_OPT_DEFINED

// COSTは64bit以下の符号付き整数。true側を始点側とする最小カット。
// 分解途中の符号付きコスト・offsetもCOST範囲内が必要。
// 有限辺容量の総和はmake_unsigned_t<COST>の最大値未満。算術・容量超過は例外。
// V=補助頂点込み頂点数、E=辺数。追加は償却O(1)、集合項はO(|xs|)。
// ctor(n=0)/ctor(n,inf): n変数で構築。旧inf引数の値は不要(厳密制約は内部で表現)。
// init(n,inf=0)/clear(): 項を消去して再構築/変数数を維持して初期化、O(V+E)。
// add_single_cost(i,F,T): xi=false/trueのコストF/Tを追加。
// add_psp_constraint(i,j): xi=trueかつxj=falseを禁止。
// add_psp_penalty(i,j,C): xi=trueかつxj=falseに非負コストC。
// add_both_true_profit(i,j,P)/add_both_false_profit(i,j,P): 両方true/falseに非負利得P。
// add_submodular_function(i,j,A,B,C,D): FF/FT/TF/TTのコスト、B+C>=A+D。同一変数はA/Dのみ。
// add_all_true_profit(xs,P)/add_all_false_profit(xs,P): 全てtrue/falseに非負利得P。空集合も獲得、P=0は無操作。
// add_constant(C): 全割当に定数Cを加算、O(1)。
// force(i,value): xiをvalueに固定。矛盾はsolveでdomain_error。
// add_equality(i,j): xi=xjを強制。
// solve(): 最小コスト、Dinic法O(V^2 E)。再呼出しO(1)、追加後は残余網から再開。
// reconstruct(): 最適割当vector<bool>、未解決ならsolve後に復元、追加分O(n)。
// solve_with_assignment(): {最小コスト,最適割当}を返す。
// variable_count()/node_count()/edge_count(): 変数数/補助頂点込み頂点数/順方向辺数、O(1)。
// copy/move/swap(other): 独立コピー/移動(移動元は空の0変数)/定数時間交換。
// reserve_nodes(count): 補助頂点込みの頂点容量を予約、O(V)。辺容量の予約ではない。
// operator<<: 現在の順方向辺をfrom->to(flow/capacity)形式で出力、O(E)。
template<class COST = std::int64_t>
class TwoVariableSubmodularOpt {
    static_assert(std::is_integral_v<COST> && std::is_signed_v<COST> && sizeof(COST) <= 8,
                  "COST must be a signed integer of at most 64 bits");
    using Capacity = std::make_unsigned_t<COST>;
    static constexpr Capacity hard_capacity = std::numeric_limits<Capacity>::max();
    struct Edge { int to; int reverse; Capacity capacity; };
    struct Frame { int vertex; Capacity limit; Capacity sent; };
    int n_ = 0;
    int source_ = 0;
    int sink_ = 1;
    COST offset_ = 0;
    Capacity finite_capacity_ = 0;
    Capacity total_flow_ = 0;
    std::size_t edge_count_ = 0;
    bool solved_ = false;
    bool infeasible_ = false;
    std::vector<std::vector<Edge>> graph_{2};
    std::vector<int> level_, cursor_, queue_;
    std::vector<Frame> stack_;

    static COST checked_add(COST a, COST b) {
        if ((b > 0 && a > std::numeric_limits<COST>::max() - b) ||
            (b < 0 && a < std::numeric_limits<COST>::min() - b))
            throw std::overflow_error("Project Selection: cost overflow");
        return static_cast<COST>(a + b);
    }
    static COST checked_sub(COST a, COST b) {
        if ((b > 0 && a < std::numeric_limits<COST>::min() + b) ||
            (b < 0 && a > std::numeric_limits<COST>::max() + b))
            throw std::overflow_error("Project Selection: cost overflow");
        return static_cast<COST>(a - b);
    }
    static Capacity difference(COST high, COST low) {
        return static_cast<Capacity>(static_cast<Capacity>(high) - static_cast<Capacity>(low));
    }
    void check_variable(int i) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (i < 0 || i >= n_) throw std::out_of_range("Project Selection: variable index");
#else
        (void)i;
#endif
    }
    static void check_nonnegative(COST value) {
#ifndef SKIP_RUNTIME_CHECKS
        if (value < 0) throw std::invalid_argument("Project Selection: negative penalty/profit");
#else
        (void)value;
#endif
    }
    void check_budget(Capacity extra) const {
        if (extra >= hard_capacity - finite_capacity_)
            throw std::overflow_error("Project Selection: finite capacity budget exceeded");
    }
    void add_edge(int from, int to, Capacity cap, bool hard = false) {
        if (!cap || from == to) return;
        if (graph_[from].size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            graph_[to].size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("Project Selection: too many incident edges");
        if (!hard) check_budget(cap);
        const int a = static_cast<int>(graph_[from].size());
        const int b = static_cast<int>(graph_[to].size());
        graph_[from].push_back({to, b, cap});
        try { graph_[to].push_back({from, ~a, 0}); }
        catch (...) { graph_[from].pop_back(); throw; }
        if (!hard) finite_capacity_ += cap;
        ++edge_count_;
        solved_ = false;
    }
    int add_node() {
        if (graph_.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("Project Selection: too many vertices");
        const int result = static_cast<int>(graph_.size());
        graph_.emplace_back();
        solved_ = false;
        return result;
    }
    void add_all_profit(const std::vector<int>& xs, COST profit, bool value) {
        check_nonnegative(profit);
        const COST next = checked_sub(offset_, profit);
        if (xs.empty() || profit == 0) { offset_ = next; return; }
        check_budget(static_cast<Capacity>(profit));
        const bool was_solved = solved_;
        const Capacity old_budget = finite_capacity_;
        const std::size_t old_count = edge_count_;
        const int y = add_node();
        try {
            if (value) add_edge(source_, y, static_cast<Capacity>(profit));
            else add_edge(y, sink_, static_cast<Capacity>(profit));
            for (int x : xs) {
                check_variable(x);
                if (value) add_edge(y, x, hard_capacity, true);
                else add_edge(x, y, hard_capacity, true);
            }
        } catch (...) {
            // 通常処理中に検査し、失敗時だけこの集合項の辺を巻き戻す。
            for (auto it = graph_[y].rbegin(); it != graph_[y].rend(); ++it)
                graph_[it->to].pop_back();
            graph_.pop_back();
            finite_capacity_ = old_budget; edge_count_ = old_count; solved_ = was_solved;
            throw;
        }
        offset_ = next;
    }
    bool bfs() {
        std::fill(level_.begin(), level_.end(), -1);
        level_[source_] = 0;
        queue_.clear();
        queue_.push_back(source_);
        for (std::size_t head = 0; head < queue_.size(); ++head) {
            const int v = queue_[head];
            for (const auto& e : graph_[v]) {
                if (e.capacity == 0 || level_[e.to] >= 0) continue;
                level_[e.to] = level_[v] + 1;
                if (e.to == sink_) return true;
                queue_.push_back(e.to);
            }
        }
        return false;
    }
    Capacity iterative_flow(int start, Capacity limit) {
        stack_.clear();
        stack_.push_back({start, limit, 0});
        while (!stack_.empty()) {
            Frame& frame = stack_.back();
            const int v = frame.vertex;
            if (v == sink_) frame.sent = frame.limit;
            if (frame.sent == frame.limit || cursor_[v] == static_cast<int>(graph_[v].size())) {
                const Capacity sent = frame.sent;
                stack_.pop_back();
                if (stack_.empty()) return sent;
                Frame& parent = stack_.back();
                Edge& edge = graph_[parent.vertex][cursor_[parent.vertex]];
                edge.capacity -= sent;
                graph_[edge.to][edge.reverse < 0 ? ~edge.reverse : edge.reverse].capacity += sent;
                parent.sent += sent;
                if (parent.sent != parent.limit) ++cursor_[parent.vertex];
                continue;
            }
            Edge& edge = graph_[v][cursor_[v]];
            if (edge.capacity == 0 || level_[edge.to] != level_[v] + 1) {
                ++cursor_[v];
                continue;
            }
            stack_.push_back({edge.to, std::min(static_cast<Capacity>(frame.limit - frame.sent), edge.capacity), 0});
        }
        return 0;
    }
    Capacity bounded_flow(int v, Capacity limit, unsigned depth = 64) {
        if (v == sink_) return limit;
        if (depth == 0) [[unlikely]] return iterative_flow(v, limit);
        Capacity sent = 0;
        for (int& i = cursor_[v]; i < static_cast<int>(graph_[v].size()); ++i) {
            Edge& edge = graph_[v][i];
            if (edge.capacity == 0 || level_[edge.to] != level_[v] + 1) continue;
            const Capacity flow = bounded_flow(edge.to, std::min(static_cast<Capacity>(limit - sent), edge.capacity), depth - 1);
            edge.capacity -= flow;
            graph_[edge.to][edge.reverse < 0 ? ~edge.reverse : edge.reverse].capacity += flow;
            sent += flow;
            if (sent == limit) break;
        }
        return sent;
    }
    COST answer() const {
        if (offset_ >= 0) {
            if (total_flow_ > static_cast<Capacity>(std::numeric_limits<COST>::max() - offset_))
                throw std::overflow_error("Project Selection: optimum is out of COST range");
            return static_cast<COST>(offset_ + static_cast<COST>(total_flow_));
        }
        const Capacity magnitude = static_cast<Capacity>(Capacity{0} - static_cast<Capacity>(offset_));
        if (total_flow_ >= magnitude) {
            const Capacity positive = total_flow_ - magnitude;
            if (positive > static_cast<Capacity>(std::numeric_limits<COST>::max()))
                throw std::overflow_error("Project Selection: optimum is out of COST range");
            return static_cast<COST>(positive);
        }
        const Capacity negative = magnitude - total_flow_;
        if (negative == static_cast<Capacity>(std::numeric_limits<COST>::max()) + Capacity{1})
            return std::numeric_limits<COST>::min();
        return static_cast<COST>(-static_cast<COST>(negative));
    }

public:
    TwoVariableSubmodularOpt() = default;
    TwoVariableSubmodularOpt(const TwoVariableSubmodularOpt&) = default;
    TwoVariableSubmodularOpt& operator=(const TwoVariableSubmodularOpt& other) {
        if (this != &other) { TwoVariableSubmodularOpt copy(other); swap(copy); }
        return *this;
    }
    TwoVariableSubmodularOpt(TwoVariableSubmodularOpt&& other) : TwoVariableSubmodularOpt() { swap(other); }
    TwoVariableSubmodularOpt& operator=(TwoVariableSubmodularOpt&& other) {
        if (this != &other) { TwoVariableSubmodularOpt moved(std::move(other)); swap(moved); }
        return *this;
    }
    void swap(TwoVariableSubmodularOpt& other) noexcept {
        using std::swap;
        swap(n_, other.n_); swap(source_, other.source_); swap(sink_, other.sink_);
        swap(offset_, other.offset_); swap(finite_capacity_, other.finite_capacity_);
        swap(total_flow_, other.total_flow_); swap(edge_count_, other.edge_count_);
        swap(solved_, other.solved_); swap(infeasible_, other.infeasible_);
        graph_.swap(other.graph_); level_.swap(other.level_); cursor_.swap(other.cursor_);
        queue_.swap(other.queue_); stack_.swap(other.stack_);
    }
    explicit TwoVariableSubmodularOpt(int n) { init(n); }
    TwoVariableSubmodularOpt(int n, COST /*inf*/) { init(n); }
    void init(int n, COST /*inf*/ = 0) {
        if (n < 0 || n > std::numeric_limits<int>::max() - 2)
            throw std::length_error("Project Selection: invalid variable count");
        std::vector<std::vector<Edge>> fresh(static_cast<std::size_t>(n) + 2);
        graph_.swap(fresh);
        n_ = n; source_ = n; sink_ = n + 1;
        offset_ = 0; finite_capacity_ = 0; total_flow_ = 0; edge_count_ = 0;
        solved_ = false; infeasible_ = false;
        level_.clear(); cursor_.clear(); queue_.clear(); stack_.clear();
    }
    void clear() { init(n_); }
    void add_constant(COST cost) { offset_ = checked_add(offset_, cost); }
    void add_single_cost(int xi, COST false_cost, COST true_cost) {
        check_variable(xi);
        const COST small = std::min(false_cost, true_cost);
        const COST next_offset = checked_add(offset_, small);
        const Capacity cap = false_cost >= true_cost ? difference(false_cost, true_cost) : difference(true_cost, false_cost);
        if (false_cost >= true_cost) add_edge(source_, xi, cap);
        else add_edge(xi, sink_, cap);
        offset_ = next_offset;
    }
    void add_psp_constraint(int xi, int xj) {
        check_variable(xi); check_variable(xj);
        add_edge(xi, xj, hard_capacity, true);
    }
    void add_psp_penalty(int xi, int xj, COST cost) {
        check_variable(xi); check_variable(xj); check_nonnegative(cost);
        add_edge(xi, xj, static_cast<Capacity>(cost));
    }
    void add_both_true_profit(int xi, int xj, COST profit) {
        check_variable(xi); check_variable(xj); check_nonnegative(profit);
        const COST next_offset = checked_sub(offset_, profit);
        const Capacity p = static_cast<Capacity>(profit);
        if (xi == xj) check_budget(p);
        else { check_budget(p); if (p >= hard_capacity - finite_capacity_ - p) throw std::overflow_error("Project Selection: finite capacity budget exceeded"); }
        add_edge(source_, xi, p);
        add_edge(xi, xj, p);
        offset_ = next_offset;
    }
    void add_both_false_profit(int xi, int xj, COST profit) {
        check_variable(xi); check_variable(xj); check_nonnegative(profit);
        const COST next_offset = checked_sub(offset_, profit);
        const Capacity p = static_cast<Capacity>(profit);
        if (xi == xj) check_budget(p);
        else { check_budget(p); if (p >= hard_capacity - finite_capacity_ - p) throw std::overflow_error("Project Selection: finite capacity budget exceeded"); }
        add_edge(xj, sink_, p);
        add_edge(xi, xj, p);
        offset_ = next_offset;
    }
    void add_submodular_function(int xi, int xj, COST A, COST B, COST C, COST D) {
        check_variable(xi); check_variable(xj);
        if (xi == xj) { add_single_cost(xi, A, D); return; }
        // 2項ずつ差を取ることで、B+CやA+Dの符号付きoverflowを避ける。
        const COST first = checked_sub(D, B);
        const COST second = checked_sub(B, A);
        // capacity = (C-A) - (D-B)。正負別にunsignedで正確に求める。
        Capacity cap = 0;
        if (C >= A) {
            cap = difference(C, A);
            if (first >= 0) {
#ifndef SKIP_RUNTIME_CHECKS
                if (cap < static_cast<Capacity>(first)) throw std::invalid_argument("Project Selection: not submodular");
#endif
                cap -= static_cast<Capacity>(first);
            } else {
                const Capacity extra = static_cast<Capacity>(Capacity{0} - static_cast<Capacity>(first));
                if (cap > hard_capacity - extra) throw std::overflow_error("Project Selection: submodular capacity overflow");
                cap += extra;
            }
        } else {
#ifndef SKIP_RUNTIME_CHECKS
            if (first >= 0) throw std::invalid_argument("Project Selection: not submodular");
#endif
            const Capacity positive = static_cast<Capacity>(Capacity{0} - static_cast<Capacity>(first));
            const Capacity negative = difference(A, C);
#ifndef SKIP_RUNTIME_CHECKS
            if (positive < negative) throw std::invalid_argument("Project Selection: not submodular");
#endif
            cap = positive - negative;
        }
        COST next = checked_add(offset_, A);
        next = checked_add(next, std::min(COST{0}, first));
        next = checked_add(next, std::min(COST{0}, second));
        const Capacity one = first < 0 ? static_cast<Capacity>(Capacity{0} - static_cast<Capacity>(first)) : static_cast<Capacity>(first);
        const Capacity two = second < 0 ? static_cast<Capacity>(Capacity{0} - static_cast<Capacity>(second)) : static_cast<Capacity>(second);
        check_budget(cap);
        if (one >= hard_capacity - finite_capacity_ - cap || two >= hard_capacity - finite_capacity_ - cap - one)
            throw std::overflow_error("Project Selection: finite capacity budget exceeded");
        add_edge(first <= 0 ? source_ : xi, first <= 0 ? xi : sink_, one);
        add_edge(second <= 0 ? source_ : xj, second <= 0 ? xj : sink_, two);
        add_edge(xi, xj, cap);
        offset_ = next;
    }
    void add_all_true_profit(const std::vector<int>& xs, COST profit) { add_all_profit(xs, profit, true); }
    void add_all_false_profit(const std::vector<int>& xs, COST profit) { add_all_profit(xs, profit, false); }
    void force(int xi, bool value) {
        check_variable(xi);
        add_edge(value ? source_ : xi, value ? xi : sink_, hard_capacity, true);
    }
    void add_equality(int xi, int xj) { add_psp_constraint(xi, xj); add_psp_constraint(xj, xi); }
    COST solve() {
        if (infeasible_) throw std::domain_error("Project Selection: contradictory constraints");
        if (!solved_) {
            level_.resize(graph_.size()); cursor_.resize(graph_.size());
            queue_.reserve(graph_.size()); stack_.reserve(graph_.size());
            while (bfs()) {
                std::fill(cursor_.begin(), cursor_.end(), 0);
                const Capacity remaining = static_cast<Capacity>(finite_capacity_ - total_flow_ + Capacity{1});
                const Capacity added = bounded_flow(source_, remaining);
                total_flow_ += added;
                if (total_flow_ > finite_capacity_) {
                    infeasible_ = true;
                    throw std::domain_error("Project Selection: contradictory constraints");
                }
            }
            solved_ = true;
        }
        return answer();
    }
    std::vector<bool> reconstruct() {
        (void)solve();
        std::vector<bool> result(static_cast<std::size_t>(n_));
        for (int i = 0; i < n_; ++i) result[i] = level_[i] >= 0;
        return result;
    }
    std::pair<COST, std::vector<bool>> solve_with_assignment() {
        COST value = solve();
        return {value, reconstruct()};
    }
    int variable_count() const noexcept { return n_; }
    int node_count() const noexcept { return static_cast<int>(graph_.size()); }
    std::size_t edge_count() const noexcept { return edge_count_; }
    void reserve_nodes(std::size_t count) {
        if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("Project Selection: too many vertices");
        graph_.reserve(count);
    }
    friend std::ostream& operator<<(std::ostream& out, const TwoVariableSubmodularOpt& optimizer) {
        // reverseの符号で順方向辺を識別し、補助的な辺一覧の保存を省く。
        for (int from = 0; from < static_cast<int>(optimizer.graph_.size()); ++from) {
            for (const Edge& edge : optimizer.graph_[from]) {
                if (edge.reverse < 0) continue;
                const Edge& reverse = optimizer.graph_[edge.to][edge.reverse < 0 ? ~edge.reverse : edge.reverse];
                out << from << "->" << edge.to << '(' << static_cast<std::uint64_t>(reverse.capacity) << '/';
                // 逆辺も含めた対の合計は、追加時の容量に等しい。
                out << static_cast<std::uint64_t>(static_cast<Capacity>(edge.capacity + reverse.capacity)) << ")\n";
            }
        }
        return out;
    }
};
#endif
