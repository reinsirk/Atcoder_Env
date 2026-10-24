#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/persistent_unionfind
// Submission: https://judge.yosupo.jp/submission/412482

// Library: DataStructure/rollback-union-find.cpp
// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/rollback-union-find.hpp.html
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/rollback_union_find.md
//
// API一覧
// [RollbackUnionFind]
// RollbackUnionFind(n=0): n要素を構築。O(n+1)時間・領域。コピーはO(n+h+1)時間・領域で独立（hは履歴長）、移動元は空。
// find(i)/root(i)/leader(i): 代表、same(a,b)/issame(a,b): 同一成分か、size(i): 成分サイズ。O(log(n+1))。
// unite(a,b)/merge(a,b): 新規結合ならtrue。falseでも履歴は1回分増える。O(log(n+1))償却時間。
// undo(): 結合呼出し1回を戻す。O(1)。履歴が必要。get_state(): 現在の呼出し回数、O(1)。
// snapshot(): 現在位置を保存。rollback(state=-1): 指定位置/保存位置まで戻す。O(1+戻す回数)。
// size()/component_count(): 全要素数/成分数、O(1)。groups(): 成分別の頂点昇順一覧、O(1+n log(n+1))。
// swap(other)/swap(a,b): 履歴を含めて内容を交換、O(1)、noexcept。
// reset(n)/reset(): 指定/現在の頂点数で全成分を独立にし履歴・保存位置を消去。O(旧n+旧h+n+1)上界。容量再利用。
// clear()/empty(): 頂点と履歴を空にして容量保持/空か。clearはO(n+h+1)上界、いずれもnoexcept。
// コピー代入とresetの失敗時は論理状態を保持（予約容量は変わり得る）。
// reserve_history(k): k回分の履歴容量を確保。履歴・代表の直接書換えは不可。
// clear_history(): 森林を保ち履歴と保存位置を消去。容量は保持し、旧stateは無効。O(h+1)上界、noexcept。

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef SIRKIT_ROLLBACK_UNION_FIND_DEFINED
#define SIRKIT_ROLLBACK_UNION_FIND_DEFINED
class RollbackUnionFind {
    struct Change { int x, y, dx, dy; };
    std::vector<int> data;
    std::vector<Change> history;
    int inner_snap = 0;
    int components_ = 0;
    static std::size_t checked_size(std::size_t n) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("RollbackUnionFind: too many vertices");
        return n;
    }
    void check(int i) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (static_cast<std::size_t>(i) >= data.size()) throw std::out_of_range("RollbackUnionFind: vertex");
#else
        (void)i;
#endif
    }
    int leader_unchecked(int i) const { while (data[i] >= 0) i = data[i]; return i; }
    void undo_last() noexcept {
        const auto c = history.back(); history.pop_back();
        data[c.x] = c.dx; data[c.y] = c.dy; components_ += (c.x != c.y);
    }
    std::vector<std::vector<int>> trivial_groups() const {
        std::vector<std::vector<int>> result;
        result.reserve(static_cast<std::size_t>(components_));
        if (components_ == size()) {
            for (int v = 0; v < size(); ++v) result.emplace_back(std::size_t{1}, v);
        } else {
            result.emplace_back(data.size());
            for (int v = 0; v < size(); ++v) result[0][v] = v;
        }
        return result;
    }
public:
    explicit RollbackUnionFind(std::size_t n = 0)
        : data(checked_size(n), -1), components_(static_cast<int>(n)) { history.reserve(std::min(n, std::size_t{64})); }
    RollbackUnionFind(const RollbackUnionFind&) = default;
    RollbackUnionFind& operator=(const RollbackUnionFind& other) {
        if (this != &other) {
            data.reserve(other.data.size()); history.reserve(other.history.size());
            data.clear(); history.clear();
            data.insert(data.end(), other.data.begin(), other.data.end());
            history.insert(history.end(), other.history.begin(), other.history.end());
            inner_snap = other.inner_snap; components_ = other.components_;
        }
        return *this;
    }
    void swap(RollbackUnionFind& other) noexcept {
        data.swap(other.data); history.swap(other.history);
        std::swap(inner_snap, other.inner_snap); std::swap(components_, other.components_);
    }
    RollbackUnionFind(RollbackUnionFind&& other) noexcept
        : data(std::move(other.data)), history(std::move(other.history)),
          inner_snap(std::exchange(other.inner_snap, 0)), components_(std::exchange(other.components_, 0)) {
        other.data.clear(); other.history.clear();
    }
    RollbackUnionFind& operator=(RollbackUnionFind&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data); history = std::move(other.history);
            inner_snap = std::exchange(other.inner_snap, 0); components_ = std::exchange(other.components_, 0);
            other.data.clear(); other.history.clear();
        }
        return *this;
    }
    void reset(std::size_t n) {
        const auto count = checked_size(n);
        data.reserve(count); history.reserve(std::min(count, std::size_t{64}));
        data.clear(); history.clear();
        data.resize(count, -1);
        inner_snap = 0; components_ = static_cast<int>(count);
    }
    void reset() { reset(data.size()); }
    void clear() noexcept { data.clear(); history.clear(); inner_snap = 0; components_ = 0; }
    bool empty() const noexcept { return data.empty(); }
    friend void swap(RollbackUnionFind& a, RollbackUnionFind& b) noexcept { a.swap(b); }
    int find(int i) const { check(i); return leader_unchecked(i); }
    int root(int i) const { return find(i); }
    int leader(int i) const { return find(i); }
    bool unite(int x, int y) {
        check(x); check(y); x = leader_unchecked(x); y = leader_unchecked(y);
        if (history.size() == static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("RollbackUnionFind: history overflow");
        history.push_back({x, y, data[x], data[y]});
        if (x == y) return false;
        if (data[x] > data[y]) std::swap(x, y);
        data[x] += data[y]; data[y] = x; --components_; return true;
    }
    bool merge(int a, int b) { return unite(a, b); }
    bool same(int a, int b) const { check(a); check(b); return leader_unchecked(a) == leader_unchecked(b); }
    bool issame(int a, int b) const { return same(a, b); }
    int size(int i) const { return -data[find(i)]; }
    int size() const noexcept { return static_cast<int>(data.size()); }
    int component_count() const noexcept { return components_; }
    void undo() {
#ifndef SKIP_RUNTIME_CHECKS
        if (history.empty()) throw std::out_of_range("RollbackUnionFind: empty history");
#endif
        const auto c = history.back(); history.pop_back();
        if (c.x != c.y) {
            data[c.x] = c.dx; data[c.y] = c.dy; ++components_;
        }
        if (inner_snap > get_state()) inner_snap = get_state();
    }
    void snapshot() noexcept { inner_snap = get_state(); }
    int get_state() const noexcept { return static_cast<int>(history.size()); }
    void rollback(int state = -1) {
        if (state == -1) state = inner_snap;
#ifndef SKIP_RUNTIME_CHECKS
        if (state < 0 || state > get_state()) throw std::out_of_range("RollbackUnionFind: state");
#endif
        while (get_state() > state) undo_last();
        if (inner_snap > state) inner_snap = state;
    }
    void clear_history() noexcept { history.clear(); inner_snap = 0; }
    void reserve_history(std::size_t count) {
        if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("RollbackUnionFind: history overflow");
        history.reserve(count);
    }
    std::vector<std::vector<int>> groups() const {
        if (components_ == size() || components_ == 1) return trivial_groups();
        std::vector<int> leaders(data.size()), counts(data.size(), 0), slot(data.size());
        for (int i = 0; i < size(); ++i) ++counts[leaders[i] = leader_unchecked(i)];
        std::vector<std::vector<int>> result; result.reserve(components_);
        for (int i = 0; i < size(); ++i) if (counts[i]) {
            slot[i] = static_cast<int>(result.size()); result.emplace_back(); result.back().reserve(counts[i]);
        }
        for (int i = 0; i < size(); ++i) result[slot[leaders[i]]].push_back(i);
        return result;
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

#include <array>
#include <vector>
int main() {
    int n,q;verify_in>>n>>q;
    std::vector<std::array<int,3>> queries(q+1);
    std::vector<std::vector<int>> children(q+1);
    for(int i=1;i<=q;++i){int k;auto& a=queries[i];verify_in>>a[0]>>k>>a[1]>>a[2];children[k+1].push_back(i);}
    RollbackUnionFind uf(n);
    std::vector<int> answers(q+1,-1);
    struct Frame{int vertex;bool exit;};std::vector<Frame> stack{{0,false}};
    while(!stack.empty()){
        auto [i,exit]=stack.back();stack.pop_back();
        if(exit){if(i!=0&&queries[i][0]==0)uf.undo();continue;}
        if(i!=0){const auto& a=queries[i];if(a[0]==0)uf.unite(a[1],a[2]);else answers[i]=uf.same(a[1],a[2]);}
        stack.push_back({i,true});for(int child:children[i])stack.push_back({child,false});
    }
    for(int i=1;i<=q;++i)if(answers[i]!=-1)verify_out<<answers[i]<<'\n';
}
