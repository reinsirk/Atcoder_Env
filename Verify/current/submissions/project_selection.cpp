#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/typical90/tasks/typical90_an

// Library: Graph/Project_Selection.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://drken1215.hatenablog.com/entry/2023/11/24/034300
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/project_selection.md
//
// API一覧
// [TwoVariableSubmodularOpt]
// COSTは64bit以下の符号付き整数。true側を始点側とする最小カット。
// 分解途中の符号付きコスト・offsetもCOST範囲内が必要。
// 有限辺容量の総和はmake_unsigned_t<COST>の最大値未満。算術・容量超過は例外。
// V=補助頂点込み頂点数、E=辺数。追加は償却O(1)、集合項はO(|xs|)。
// ctor(n=0)/ctor(n,inf): n変数で構築。旧inf引数の値は不要(厳密制約は内部で表現)。
// init(n,inf=0)/clear(): 項を消去して再構築/変数数を維持して初期化、O(V+E)。clearは残る頂点の辺配列の領域を再利用、確保なし。
// add_single_cost(i,F,T): xi=false/trueのコストF/Tを追加。
// add_psp_constraint(i,j): xi=trueかつxj=falseを禁止。
// add_psp_penalty(i,j,C): xi=trueかつxj=falseに非負コストC。
// add_both_true_profit(i,j,P)/add_both_false_profit(i,j,P): 両方true/falseに非負利得P。
// add_submodular_function(i,j,A,B,C,D): FF/FT/TF/TTのコスト、B+C>=A+D。同一変数はA/Dのみ。
// add_all_true_profit(xs,P)/add_all_false_profit(xs,P): 全てtrue/falseに非負利得P。空集合も獲得、P=0は無操作。
// add_any_true_cost(xs,C)/add_any_false_cost(xs,C): 1個以上true/falseなら非負費用C。空集合は0、C=0は無操作。O(|xs|)。
// add_constant(C): 全割当に定数Cを加算、O(1)。
// force(i,value): xiをvalueに固定。矛盾はsolveでdomain_error。
// add_equality(i,j): xi=xjを強制。
// solve(): 最小コスト、Dinic法O(V^2 E)。再呼出しO(1)、追加後は残余網から再開。
// reconstruct(): 最適割当vector<bool>、未解決ならsolve後に復元、追加分O(n)。
// solve_with_assignment(): {最小コスト,最適割当}を返す。
// empty(): 変数数0ならtrue。O(1)。
// variable_count()/node_count()/edge_count(): 変数数/補助頂点込み頂点数/順方向辺数、O(1)。
// copy/move/swap(other)/swap(a,b): 独立コピー/移動(移動元は空の0変数)/定数時間交換。
// 項追加・init・コピー代入が失敗した場合は、項・変数数・最適値を変更しない（予約済み容量は変わり得る）。
// reserve_nodes(count): 補助頂点込みの頂点容量を予約、O(V)。辺容量の予約ではない。
// operator<<: 現在の順方向辺をfrom->to(flow/capacity)形式で出力、O(E)。

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

    static int checked_variable_count(int n) {
        if (n < 0 || n > std::numeric_limits<int>::max() - 2)
            throw std::length_error("Project Selection: invalid variable count");
        return n;
    }
    void reserve_incident(int vertex, std::size_t extra) {
        if (extra == 0) return;
        auto& edges = graph_[vertex];
        const auto maximum = std::min(edges.max_size(), static_cast<std::size_t>(std::numeric_limits<int>::max()));
        if (extra > maximum - edges.size())
            throw std::length_error("Project Selection: too many incident edges");
        const auto required = edges.size() + extra;
        if (required <= edges.capacity()) return;
        const auto capacity = edges.capacity();
        const auto growth = capacity > maximum - capacity ? maximum : capacity * 2;
        edges.reserve(std::max(required, growth));
    }
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
    void add_all_profit(const std::vector<int>& xs, COST profit, bool value, bool reward = true) {
        check_nonnegative(profit);
        const COST next = reward ? checked_sub(offset_, profit) : offset_;
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
        if (sent < limit) level_[v] = -1;
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
    explicit TwoVariableSubmodularOpt(int n)
        : n_(checked_variable_count(n)), source_(n_), sink_(n_ + 1), graph_(static_cast<std::size_t>(n_) + 2) {}
    TwoVariableSubmodularOpt(int n, COST /*inf*/) : TwoVariableSubmodularOpt(n) {}
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
    void clear() noexcept {
        graph_.resize(static_cast<std::size_t>(n_) + 2);
        for (auto& edges : graph_) edges.clear();
        offset_ = 0; finite_capacity_ = 0; total_flow_ = 0; edge_count_ = 0;
        solved_ = false; infeasible_ = false;
        level_.clear(); cursor_.clear(); queue_.clear(); stack_.clear();
    }
    bool empty() const noexcept { return n_ == 0; }
    friend void swap(TwoVariableSubmodularOpt& a, TwoVariableSubmodularOpt& b) noexcept { a.swap(b); }
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
        if (p != 0) {
            reserve_incident(source_, 1);
            reserve_incident(xi, xi == xj ? 1 : 2);
            if (xi != xj) reserve_incident(xj, 1);
        }
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
        if (p != 0) {
            reserve_incident(sink_, 1);
            reserve_incident(xj, xi == xj ? 1 : 2);
            if (xi != xj) reserve_incident(xi, 1);
        }
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
        // 複数辺の追加前に必要な容量を確保し、確保失敗時は項全体を未追加に保つ。
        reserve_incident(xi, std::size_t(one != 0) + std::size_t(cap != 0));
        reserve_incident(xj, std::size_t(two != 0) + std::size_t(cap != 0));
        reserve_incident(source_, std::size_t(one != 0 && first <= 0) + std::size_t(two != 0 && second <= 0));
        reserve_incident(sink_, std::size_t(one != 0 && first > 0) + std::size_t(two != 0 && second > 0));
        add_edge(first <= 0 ? source_ : xi, first <= 0 ? xi : sink_, one);
        add_edge(second <= 0 ? source_ : xj, second <= 0 ? xj : sink_, two);
        add_edge(xi, xj, cap);
        offset_ = next;
    }
    void add_all_true_profit(const std::vector<int>& xs, COST profit) { add_all_profit(xs, profit, true); }
    void add_all_false_profit(const std::vector<int>& xs, COST profit) { add_all_profit(xs, profit, false); }
    void add_any_true_cost(const std::vector<int>& xs, COST cost) { add_all_profit(xs, cost, false, false); }
    void add_any_false_cost(const std::vector<int>& xs, COST cost) { add_all_profit(xs, cost, true, false); }
    void force(int xi, bool value) {
        check_variable(xi);
        add_edge(value ? source_ : xi, value ? xi : sink_, hard_capacity, true);
    }
    void add_equality(int xi, int xj) {
        check_variable(xi); check_variable(xj);
        if (xi == xj) return;
        reserve_incident(xi, 2); reserve_incident(xj, 2);
        add_edge(xi, xj, hard_capacity, true);
        add_edge(xj, xi, hard_capacity, true);
    }
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


// SPDX-License-Identifier: CC0-1.0
// Verify I/O: Verify/current/common/fast_io.hpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef SIRKIT_VERIFY_FAST_IO_DEFINED
#define SIRKIT_VERIFY_FAST_IO_DEFINED

// VerifyInput(file=stdin): 非所有のFILEから読む。別の入力方式と混用しない。
// >> 整数: 64bit以下の十進整数を読む。検査省略時は値域内の正しい入力を前提とする。
// >> char/string: 空白を飛ばし、1文字／空白までの文字列を読む。
// >> 浮動小数点: 十進文字列を変換する。通常のCロケールで使う。
// bool / !: 直前までの読み取りが成功しているか／失敗したかを返す。
class VerifyInput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<unsigned char, buffer_size> buffer_;
    std::size_t position_ = 0, size_ = 0;
    bool good_ = true;

    int peek() {
        if (position_ == size_) {
            size_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            position_ = 0;
            if (size_ == 0) return EOF;
        }
        return buffer_[position_];
    }

    int skip_space() {
        int c;
        while ((c = peek()) != EOF && c <= ' ') ++position_;
        if (c == EOF) good_ = false;
        return c;
    }

public:
    explicit VerifyInput(std::FILE* file = stdin) : file_(file) {}
    VerifyInput(const VerifyInput&) = delete;
    VerifyInput& operator=(const VerifyInput&) = delete;

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyInput& operator>>(T& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        bool negative = false;
        if (c == '-' || c == '+') {
            negative = c == '-';
            ++position_;
            c = peek();
        }
        if (c == EOF) {
            good_ = false;
            return *this;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (c < '0' || c > '9') {
            good_ = false;
            return *this;
        }
        // 先頭の0を除けば、20桁以下の末尾直前の値はuint64_tに収まる。
        while (c == '0') {
            ++position_;
            c = peek();
        }
        std::uint64_t previous = 0;
        unsigned last_digit = 0;
        std::size_t digits = 0;
#endif
        std::uint64_t value = 0;
        while (c >= '0' && c <= '9') {
#ifndef SKIP_RUNTIME_CHECKS
            previous = value;
            last_digit = static_cast<unsigned>(c - '0');
            if (digits <= 20) ++digits;
#endif
            value = value * 10 + static_cast<unsigned>(c - '0');
            ++position_;
            c = peek();
        }
#ifndef SKIP_RUNTIME_CHECKS
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if ((c != EOF && c > ' ') || digits > 20
            || (digits == 20 && (previous > maximum / 10
                || (previous == maximum / 10 && last_digit > maximum % 10)))) {
            good_ = false;
            return *this;
        }
        std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max());
        if constexpr (std::is_signed_v<T>) limit += negative;
        else if (negative && value != 0) good_ = false;
        if (value > limit) good_ = false;
        if (!good_) return *this;
#endif
        if constexpr (std::is_signed_v<T>) {
            constexpr std::uint64_t minimum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<T>::max()) + 1;
            // -(最小値)は評価せず、絶対値が最大値+1のときだけ最小値を使う。
            result = negative
                ? (value == minimum_magnitude ? std::numeric_limits<T>::min()
                                  : static_cast<T>(-static_cast<T>(value)))
                : static_cast<T>(value);
        } else result = static_cast<T>(value);
        return *this;
    }

    VerifyInput& operator>>(char& result) {
        if (!good_) return *this;
        const int c = skip_space();
        if (c != EOF) {
            result = static_cast<char>(c);
            ++position_;
        }
        return *this;
    }

    VerifyInput& operator>>(std::string& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        result.clear();
        do {
            result.push_back(static_cast<char>(c));
            ++position_;
            c = peek();
        } while (c != EOF && c > ' ');
        return *this;
    }

    template<class T> requires std::is_floating_point_v<T>
    VerifyInput& operator>>(T& result) {
        std::string token;
        *this >> token;
        if (!good_) return *this;
#ifndef SKIP_RUNTIME_CHECKS
        char* end = nullptr;
        const long double value = std::strtold(token.c_str(), &end);
        if (end != token.c_str() + token.size()) good_ = false;
        else result = static_cast<T>(value);
#else
        result = static_cast<T>(std::strtold(token.c_str(), nullptr));
#endif
        return *this;
    }
};

// VerifyOutput(file=stdout): 非所有のFILEへ書く。破棄時にflushし、FILEは閉じない。
// << 整数/char/文字列/浮動小数点: 値を出力する。整数は64bit以下に対応する。
// fixed(digits): 浮動小数点を小数点以下digits桁で出力する（digits>=0）。
// flush(): バッファの内容をFILEへ渡す。bool / !で書き込み失敗を確認できる。
class VerifyOutput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<char, buffer_size> buffer_;
    std::size_t size_ = 0;
    int precision_ = 6;
    bool fixed_ = false, good_ = true;

    void write(std::string_view text) {
        while (!text.empty() && good_) {
            if (size_ == buffer_.size()) flush();
            const auto count = std::min(text.size(), buffer_.size() - size_);
            std::memcpy(buffer_.data() + size_, text.data(), count);
            size_ += count;
            text.remove_prefix(count);
        }
    }

public:
    explicit VerifyOutput(std::FILE* file = stdout) : file_(file) {}
    VerifyOutput(const VerifyOutput&) = delete;
    VerifyOutput& operator=(const VerifyOutput&) = delete;
    ~VerifyOutput() { flush(); }

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    VerifyOutput& fixed(int digits) {
        if (digits < 0) throw std::invalid_argument("negative output precision");
        fixed_ = true;
        precision_ = digits;
        return *this;
    }

    void flush() noexcept {
        std::size_t written = 0;
        while (written < size_ && good_) {
            const auto count = std::fwrite(buffer_.data() + written, 1, size_ - written, file_);
            if (count == 0) good_ = false;
            written += count;
        }
        size_ = 0;
    }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyOutput& operator<<(T value) {
        if constexpr (std::is_same_v<T, bool>) return *this << static_cast<char>('0' + value);
        else {
            char digits[std::numeric_limits<std::uint64_t>::digits10 + 3];
            const auto end = std::to_chars(digits, digits + sizeof(digits), value).ptr;
            write(std::string_view(digits, static_cast<std::size_t>(end - digits)));
            return *this;
        }
    }

    VerifyOutput& operator<<(char value) {
        if (size_ == buffer_.size()) flush();
        if (good_) buffer_[size_++] = value;
        return *this;
    }

    VerifyOutput& operator<<(std::string_view value) { write(value); return *this; }
    VerifyOutput& operator<<(const std::string& value) { write(value); return *this; }
    VerifyOutput& operator<<(const char* value) { write(value); return *this; }

    template<class T> requires std::is_floating_point_v<T>
    VerifyOutput& operator<<(T value) {
        char local[128];
        const char* format = fixed_ ? "%.*Lf" : "%.*Lg";
        const int count = std::snprintf(local, sizeof(local), format, precision_,
                                        static_cast<long double>(value));
        if (count < 0) good_ = false;
        else if (static_cast<std::size_t>(count) < sizeof(local))
            write(std::string_view(local, static_cast<std::size_t>(count)));
        else {
            std::vector<char> text(static_cast<std::size_t>(count) + 1);
            std::snprintf(text.data(), text.size(), format, precision_, static_cast<long double>(value));
            write(std::string_view(text.data(), static_cast<std::size_t>(count)));
        }
        return *this;
    }
};

inline VerifyInput verify_in;
inline VerifyOutput verify_out;
#endif

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
