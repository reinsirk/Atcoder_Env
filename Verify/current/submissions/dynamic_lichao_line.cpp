#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/line_add_get_min

// Library: DataStructure/dynamic-li-chao-tree.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/segment-tree/dynamic-li-chao-tree.hpp

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_DYNAMIC_LI_CHAO_TREE_DEFINED
#define REINSIRK_DYNAMIC_LI_CHAO_TREE_DEFINED
// 整数座標の閉区間で直線・線分の最小値を管理する。Tは符号付き64ビット以下の整数。
// 追加する線の対象区間では、数学的なa*x+bがTの範囲内かつINF未満であること。
// Wを座標数とすると直線追加・取得はO(log W)、線分追加はO(log² W)。
// queryは読み取り専用。SKIP_RUNTIME_CHECKSで座標・端点値の検査を除く。
template <typename T, T INF>
class DynamicLiChaoTree {
  // DynamicLiChaoTree(xmin, xmax): 整数の閉区間[xmin, xmax]で空の木を作る。
  // update(a, b) / add_line(a, b): 定義域全体にy=a*x+bを追加する。
  // update_line_segment(a, b, left, right) / add_segment(...): 閉区間[left, right]に線分を追加。left>rightは無操作。
  // query(x): xでの最小値を返す。被覆する線がなければINF。
  // query_line(x): 最小値を与えるLineを返す。同値時は任意、非被覆ならnullopt。
  // empty(): 追加した直線・空でない線分がなければtrue。O(1)。
  // clear(): 全ての線を消し、確保した容量を再利用する。O(格納ノード数)。
  // reserve(n): nノード分を事前確保する。直線本数とは異なる。
  // stored_nodes(): 現在の格納ノード数を返す。取得操作では増えない。O(1)。
  // domain_min() / domain_max(): 定義域の左端・右端を返す。O(1)。
 public:
  struct Line {
    // Line(a, b): y=a*x+b。既定値は空を表す定数INF。
    // get(x): a*x+bを返す。値がTで表現可能であること。
    // over(other, x): xでotherより小さい値を与えるときtrue。
    T slope = 0;
    T intercept = INF;
    Line() = default;
    Line(T a, T b) : slope(a), intercept(b) {}
    T get(T x) const noexcept {
      using U = std::make_unsigned_t<T>;
      // 中間積がTを超えても、最終値が表現可能なら正確に復元できる。
      const U value = static_cast<U>(std::uint64_t(static_cast<U>(slope)) *
                                         std::uint64_t(static_cast<U>(x)) +
                                     std::uint64_t(static_cast<U>(intercept)));
      return std::bit_cast<T>(value);
    }
    bool over(const Line& other, T x) const noexcept { return get(x) < other.get(x); }
  };

 private:
  static_assert(std::is_integral_v<T> && std::is_signed_v<T> &&
                sizeof(T) <= sizeof(std::uint64_t) &&
                std::is_same_v<T, std::remove_cv_t<T>>);
  static_assert(INF > 0);
  using Index = std::uint32_t;
  static constexpr Index no_child = std::numeric_limits<Index>::max();
  struct Node {
    Line line;
    Index left = no_child;
    Index right = no_child;
  };
  T xmin_, xmax_;
  std::vector<Node> nodes_;

#ifndef SKIP_RUNTIME_CHECKS
  void check_coordinate(T x) const {
    if (x < xmin_ || xmax_ < x) throw std::out_of_range("Li Chao coordinate");
  }
  static void check_value(const Line& line, T x) {
    using U = std::make_unsigned_t<T>;
    const U a = line.slope < 0 ? U{} - U(line.slope) : U(line.slope);
    const U v = x < 0 ? U{} - U(x) : U(x);
    // 積そのものを作らず、64ビット全域の値と相殺を含むa*x+bを検査する。
    const auto product_at_most = [a, v](U limit) {
      return a == 0 || v <= limit / a;
    };
    bool valid;
    if ((line.slope < 0) != (x < 0)) {
      valid = product_at_most(U(line.intercept) - U(std::numeric_limits<T>::min())) &&
              (line.intercept < INF || !product_at_most(U(line.intercept) - U(INF)));
    } else {
      valid = line.intercept < INF &&
              product_at_most(U(INF - 1) - U(line.intercept));
    }
    if (!valid) throw std::overflow_error("Li Chao value must fit T and be below INF");
  }
  static void check_line(const Line& line, T left, T right) {
    check_value(line, left);
    if (left != right) check_value(line, right);
  }
#endif

  Index make_node() {
    if (nodes_.size() >= no_child || nodes_.size() == nodes_.max_size())
      throw std::length_error("Li Chao node limit");
    const Index result = static_cast<Index>(nodes_.size());
    nodes_.emplace_back();
    return result;
  }
  Index child(Index index, bool right) {
    Index next = right ? nodes_[index].right : nodes_[index].left;
    if (next == no_child) {
      next = make_node();
      if (right) nodes_[index].right = next;
      else nodes_[index].left = next;
    }
    return next;
  }
  void insert_line(Index index, T left, T right, Line line) {
    for (;;) {
      Node& node = nodes_[index];
      if (node.line.intercept == INF && node.line.slope == 0) {
        node.line = line;
        return;
      }
      const bool better_left = line.over(node.line, left);
      const bool better_right = line.over(node.line, right);
      if (better_left == better_right) {
        if (better_left) node.line = line;
        return;
      }
      const T middle = std::midpoint(left, right);
      const bool better_middle = line.over(node.line, middle);
      if (better_middle) std::swap(node.line, line);
      if (better_left != better_middle) {
        index = child(index, false);
        right = middle;
      } else {
        index = child(index, true);
        left = static_cast<T>(middle + 1);
      }
    }
  }
  void insert_segment(Index index, T left, T right, T qleft, T qright,
                      const Line& line) {
    if (qleft <= left && right <= qright) {
      insert_line(index, left, right, line);
      return;
    }
    const T middle = std::midpoint(left, right);
    if (qleft <= middle)
      insert_segment(child(index, false), left, middle, qleft, qright, line);
    if (middle < qright)
      insert_segment(child(index, true), static_cast<T>(middle + 1), right,
                     qleft, qright, line);
  }
  Line best_line(T x) const noexcept {
    Line best;
    if (nodes_.empty()) return best;
    T left = xmin_, right = xmax_;
    Index index = 0;
    T value = INF;
    for (;;) {
      const Node& node = nodes_[index];
      const T candidate = node.line.get(x);
      if (candidate < value) {
        value = candidate;
        best = node.line;
      }
      if (left == right) return best;
      const T middle = std::midpoint(left, right);
      if (x <= middle) {
        index = node.left;
        right = middle;
      } else {
        index = node.right;
        left = static_cast<T>(middle + 1);
      }
      if (index == no_child) return best;
    }
  }

 public:
  DynamicLiChaoTree(T xmin, T xmax) : xmin_(xmin), xmax_(xmax) {
#ifndef SKIP_RUNTIME_CHECKS
    if (xmax < xmin) throw std::invalid_argument("Li Chao domain");
#endif
  }
  void update(T a, T b) {
    const Line line(a, b);
#ifndef SKIP_RUNTIME_CHECKS
    check_line(line, xmin_, xmax_);
#endif
    if (nodes_.empty()) make_node();
    insert_line(0, xmin_, xmax_, line);
  }
  void add_line(T a, T b) { update(a, b); }
  void update_line_segment(T a, T b, T left, T right) {
    if (right < left) return;
    const Line line(a, b);
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(left);
    check_coordinate(right);
    check_line(line, left, right);
#endif
    if (nodes_.empty()) make_node();
    insert_segment(0, xmin_, xmax_, left, right, line);
  }
  void add_segment(T a, T b, T left, T right) {
    update_line_segment(a, b, left, right);
  }
  T query(T x) const {
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(x);
#endif
    return best_line(x).get(x);
  }
  std::optional<Line> query_line(T x) const {
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(x);
#endif
    Line result = best_line(x);
    if (result.slope == 0 && result.intercept == INF) return std::nullopt;
    return result;
  }
  bool empty() const noexcept { return nodes_.empty(); }
  void clear() noexcept { nodes_.clear(); }
  void reserve(std::size_t count) {
    if (count > no_child || count > nodes_.max_size())
      throw std::length_error("Li Chao node limit");
    nodes_.reserve(count);
  }
  std::size_t stored_nodes() const noexcept { return nodes_.size(); }
  T domain_min() const noexcept { return xmin_; }
  T domain_max() const noexcept { return xmax_; }
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

#ifndef REINSIRK_VERIFY_FAST_IO_DEFINED
#define REINSIRK_VERIFY_FAST_IO_DEFINED

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

#include <cstdint>
#include <limits>

int main() {
    int n{}, q{};
    if (!(verify_in >> n >> q)) return 0;
    constexpr std::int64_t infinity = std::numeric_limits<std::int64_t>::max();
    // 問題の直線値は約±2e18まで届くので、共通INF=2^60は空の印に使わない。
    DynamicLiChaoTree<std::int64_t, infinity> tree(-1'000'000'000, 1'000'000'000);
    auto read_line = [&] {
        std::int64_t a{}, b{}; verify_in >> a >> b; tree.add_line(a, b);
    };
    while (n--) read_line();
    while (q--) {
        int type{}; verify_in >> type;
        if (type == 0) read_line();
        else {
            std::int64_t x{}; verify_in >> x;
            auto answer = tree.query(x);
            if (answer == infinity) verify_out << "INFINITY\n";
            else verify_out << answer << '\n';
        }
    }
}
