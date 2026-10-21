#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/abc164/tasks/abc164_c

// Library: String/Trie.cpp
// SPDX-License-Identifier: CC0-1.0
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef REINSIRK_TRIE_NODE_DEFINED
#define REINSIRK_TRIE_NODE_DEFINED
// TrieNode(): 空で生成。TrieNode(children): 子mapを受け取り、コピーは子を共有する。
// children: 指すノード自体を所有する子（別対象の所有を借りるaliasingは対象外）。
// 破棄は反復処理。循環所有は解消しない。指定初期化は不可。
struct TrieNode {
    std::unordered_map<char, std::shared_ptr<TrieNode>> children;
    TrieNode() = default;
    explicit TrieNode(std::unordered_map<char, std::shared_ptr<TrieNode>> values)
        : children(std::move(values)) {}
    TrieNode(const TrieNode&) = default;
    TrieNode(TrieNode&&) noexcept = default;
    TrieNode& operator=(const TrieNode&) = default;
    TrieNode& operator=(TrieNode&&) noexcept = default;
    ~TrieNode() {
        std::shared_ptr<TrieNode> pending;
        detach_children(*this, pending);
        while (pending) {
            auto current = std::move(pending);
            pending = std::move(current->pending_);
            detach_children(*current, pending);
        }
    }
private:
    std::shared_ptr<TrieNode> pending_;
    static void detach_children(TrieNode& node, std::shared_ptr<TrieNode>& pending) noexcept {
        while (!node.children.empty()) {
            auto it = node.children.begin();
            auto child = std::move(it->second);
            node.children.erase(it);
            if (child && child.use_count() == 1) {
                child->pending_ = std::move(pending);
                pending = std::move(child);
            }
        }
    }
};
#endif

#ifndef REINSIRK_TRIE_DEFINED
#define REINSIRK_TRIE_DEFINED
// 全バイトを扱う多重集合。辞書順は符号なしバイト順、空文字列も登録可。
// Trie(): 空で生成。コピーは独立、move元は空で再利用可。
// insert(word, copies=1): copies個追加し、その語の個数を返す。0なら照会のみ。
// erase(word, copies=1): copies個だけ削除。個数不足ならfalseで不変、0ならtrue。
// erase_all(word): その語を全削除し、削除した個数を返す。
// count(word)/contains(word): その語の個数/存在を返す。
// prefix_count(prefix)/starts_with(prefix): 接頭辞を持つ語の総個数/存在を返す。
// size()/distinct_size()/empty(): 総個数/異なる語数/空かを返す。
// node_count()/capacity(): rootを含む使用ノード数/確保済みノード枠を返す。
// longest_prefix(text): 登録語のうちtextの最長接頭辞長。該当なしはnullopt。
// count_less(word): word未満の語の総個数を返す（重複込み）。
// kth(k): 0始まりのk番目の語。重複込み、範囲外はnullopt。
// lower_bound(word): word以上の最小登録語。該当なしはnullopt。
// items(prefix="", limit=SIZE_MAX): prefixを持つ異なる語と個数を辞書順に最大limit件返す。
// clear(): 全削除し、確保枠を保持。reserve_nodes(n): 少なくともn枠を確保。
// swap(other): 内容と確保枠を定数時間で交換。
// 子はソート済み兄弟リスト。次数16で256要素の索引を追加、8未満で回収。
// count/contains/prefix系/longest_prefixは長さLに対しO(16L)、size系はO(1)。
// insertは償却O(16L+256)、再確保時は保持ノード/索引枠数ぶんのコピーも発生。
// erase/erase_allはO(16L+256)。count_less/kthは語長L/戻す長さKに対しO(256L)/O(256K)。
// lower_boundはO(256(L+K))。itemsは接頭辞探索＋訪問ノード数＋返す文字列総長。
// 保持ノード数N・保持索引数TとしてコピーはO(N+256T)、破棄・clearはO(N+T)。
// reserve_nodesは最悪O(N)。使用メモリはノード枠数＋256×索引枠数に比例。
// 破棄・clear・列挙は再帰しない。64文字までの追加経路は固定長の作業配列へ記録。
// 個数・ノード枠の上限超過は検査省略時もlength_error。確保失敗はbad_alloc。
class Trie {
    static constexpr std::size_t none = std::numeric_limits<std::size_t>::max();
    static constexpr std::uint32_t no_table = std::numeric_limits<std::uint32_t>::max();
    static constexpr std::uint16_t index_threshold = 16;
    struct Node {
        std::size_t subtree = 0;
        std::size_t terminal = 0;
        std::size_t first_child = none;
        std::size_t next_sibling = none;
        unsigned char byte = 0;
        std::uint16_t degree = 0;
        std::uint32_t table = no_table;
    };
    struct Table {
        std::array<std::size_t, 256> entries;
        Table() noexcept { entries.fill(none); }
    };
    std::vector<Node> nodes_{1};
    std::vector<Table> tables_;
    std::uint32_t free_table_ = no_table;
    std::size_t free_head_ = none;
    std::size_t live_ = 1;
    std::size_t distinct_ = 0;

    std::size_t child(std::size_t parent, unsigned char byte) const noexcept {
        if (nodes_[parent].table != no_table)
            return tables_[nodes_[parent].table].entries[byte];
        std::size_t p = nodes_[parent].first_child;
        while (p != none && nodes_[p].byte < byte) p = nodes_[p].next_sibling;
        return p != none && nodes_[p].byte == byte ? p : none;
    }
    std::size_t find(std::string_view word) const noexcept {
        if (nodes_.empty()) return none;
        std::size_t p = 0;
        for (char value : word) {
            const unsigned char byte = static_cast<unsigned char>(value);
            p = child(p, byte);
            if (p == none) return none;
        }
        return p;
    }
    void release_table(std::size_t p) noexcept {
        if (nodes_[p].table == no_table) return;
        tables_[nodes_[p].table].entries[0] = free_table_;
        free_table_ = nodes_[p].table;
        nodes_[p].table = no_table;
    }
    std::size_t allocate_node(unsigned char byte, std::size_t& fresh) noexcept {
        std::size_t p;
        if (free_head_ != none) {
            p = free_head_;
            free_head_ = nodes_[p].first_child;
            nodes_[p] = Node{};
        } else {
            p = fresh++;
        }
        nodes_[p].byte = byte;
        ++live_;
        return p;
    }
    void remove_known(std::string_view word, std::size_t copies) noexcept {
        nodes_[0].subtree -= copies;
        std::size_t parent = 0;
        for (char value : word) {
            const unsigned char byte = static_cast<unsigned char>(value);
            std::size_t p = child(parent, byte);
            nodes_[p].subtree -= copies;
            if (nodes_[p].subtree == 0) {
                std::size_t* link = &nodes_[parent].first_child;
                while (*link != p) link = &nodes_[*link].next_sibling;
                *link = nodes_[p].next_sibling;
                if (nodes_[parent].table != no_table)
                    tables_[nodes_[parent].table].entries[byte] = none;
                if (--nodes_[parent].degree < index_threshold / 2) release_table(parent);
                // 消えた語だけからなる単一路を空きリストへ戻す。
                while (p != none) {
                    const std::size_t next = nodes_[p].first_child;
                    release_table(p);
                    nodes_[p].first_child = free_head_;
                    free_head_ = p;
                    --live_;
                    p = next;
                }
                --distinct_;
                return;
            }
            parent = p;
        }
        nodes_[parent].terminal -= copies;
        if (nodes_[parent].terminal == 0) --distinct_;
    }
public:
    Trie() = default;
    Trie(const Trie&) = default;
    Trie& operator=(const Trie& other) {
        if (this != &other) {
            Trie copied(other);
            swap(copied);
        }
        return *this;
    }
    Trie(Trie&& other) noexcept : nodes_(std::move(other.nodes_)),
        tables_(std::move(other.tables_)), free_table_(std::exchange(other.free_table_, no_table)),
        free_head_(std::exchange(other.free_head_, none)),
        live_(std::exchange(other.live_, 1)), distinct_(std::exchange(other.distinct_, 0)) {
        other.nodes_.clear();
        other.tables_.clear();
    }
    Trie& operator=(Trie&& other) noexcept {
        if (this != &other) {
            nodes_ = std::move(other.nodes_);
            tables_ = std::move(other.tables_);
            free_table_ = std::exchange(other.free_table_, no_table);
            free_head_ = std::exchange(other.free_head_, none);
            live_ = std::exchange(other.live_, 1);
            distinct_ = std::exchange(other.distinct_, 0);
            other.nodes_.clear();
            other.tables_.clear();
        }
        return *this;
    }
    std::size_t insert(std::string_view word, std::size_t copies = 1) {
        if (copies == 0) return count(word);
        if (copies > none - size()) throw std::length_error("Trie: count overflow");
        std::array<std::size_t, 64> path;
        std::size_t matched = 0, p = 0;
        if (!nodes_.empty()) {
            while (matched < word.size()) {
                const std::size_t next = child(p, static_cast<unsigned char>(word[matched]));
                if (next == none) break;
                p = next;
                if (matched < path.size()) path[matched] = p;
                ++matched;
            }
        }
        const std::size_t needed = word.size() - matched;
        if (needed > nodes_.max_size() - live_)
            throw std::length_error("Trie: node capacity overflow");
        const std::size_t old_size = nodes_.size();
        const std::size_t free_count = old_size == 0 ? 0 : old_size - live_;
        const std::size_t additional = needed > free_count ? needed - free_count : 0;
        const std::size_t root_needed = old_size == 0 ? 1 : 0;
        const std::size_t branch = p;
        const bool needs_index = needed != 0 && old_size != 0 &&
            nodes_[branch].table == no_table && nodes_[branch].degree + 1 >= index_threshold;
        bool appended_table = false;
        // 全確保を先に済ませ、例外時に内容を変えない。
        if (needs_index && free_table_ == no_table) {
            if (tables_.size() >= no_table) throw std::length_error("Trie: index capacity overflow");
            tables_.emplace_back();
            appended_table = true;
        }
        try {
            nodes_.resize(old_size + root_needed + additional);
        } catch (...) {
            if (appended_table) tables_.pop_back();
            throw;
        }
        if (needs_index) {
            std::uint32_t table;
            if (appended_table) {
                table = static_cast<std::uint32_t>(tables_.size() - 1);
            } else {
                table = free_table_;
                free_table_ = static_cast<std::uint32_t>(tables_[table].entries[0]);
                tables_[table].entries.fill(none);
            }
            nodes_[branch].table = table;
            for (std::size_t c = nodes_[branch].first_child; c != none; c = nodes_[c].next_sibling)
                tables_[table].entries[nodes_[c].byte] = c;
        }
        std::size_t fresh = old_size + root_needed;
        p = 0;
        nodes_[0].subtree += copies;
        for (std::size_t i = 0; i < matched; ++i) {
            p = i < path.size() ? path[i] : child(p, static_cast<unsigned char>(word[i]));
            nodes_[p].subtree += copies;
        }
        for (std::size_t i = matched; i < word.size(); ++i) {
            const unsigned char byte = static_cast<unsigned char>(word[i]);
            const std::size_t next = allocate_node(byte, fresh);
            nodes_[next].subtree = copies;
            std::size_t* link = &nodes_[p].first_child;
            while (*link != none && nodes_[*link].byte < byte)
                link = &nodes_[*link].next_sibling;
            nodes_[next].next_sibling = *link;
            *link = next;
            ++nodes_[p].degree;
            if (nodes_[p].table != no_table) tables_[nodes_[p].table].entries[byte] = next;
            p = next;
        }
        if (nodes_[p].terminal == 0) ++distinct_;
        return nodes_[p].terminal += copies;
    }
    bool erase(std::string_view word, std::size_t copies = 1) noexcept {
        if (copies == 0) return true;
        if (count(word) < copies) return false;
        remove_known(word, copies);
        return true;
    }
    std::size_t erase_all(std::string_view word) noexcept {
        const std::size_t copies = count(word);
        if (copies != 0) remove_known(word, copies);
        return copies;
    }
    std::size_t count(std::string_view word) const noexcept {
        const std::size_t p = find(word);
        return p == none ? 0 : nodes_[p].terminal;
    }
    bool contains(std::string_view word) const noexcept { return count(word) != 0; }
    std::size_t prefix_count(std::string_view prefix) const noexcept {
        const std::size_t p = find(prefix);
        return p == none ? 0 : nodes_[p].subtree;
    }
    bool starts_with(std::string_view prefix) const noexcept { return prefix_count(prefix) != 0; }
    std::size_t size() const noexcept { return nodes_.empty() ? 0 : nodes_[0].subtree; }
    std::size_t distinct_size() const noexcept { return distinct_; }
    bool empty() const noexcept { return size() == 0; }
    std::size_t node_count() const noexcept { return live_; }
    std::size_t capacity() const noexcept { return nodes_.capacity(); }
    std::optional<std::size_t> longest_prefix(std::string_view text) const noexcept {
        if (nodes_.empty()) return std::nullopt;
        std::optional<std::size_t> result;
        std::size_t p = 0;
        if (nodes_[0].terminal != 0) result = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            p = child(p, static_cast<unsigned char>(text[i]));
            if (p == none) break;
            if (nodes_[p].terminal != 0) result = i + 1;
        }
        return result;
    }
    std::size_t count_less(std::string_view word) const noexcept {
        if (nodes_.empty()) return 0;
        std::size_t answer = 0, p = 0;
        for (char value : word) {
            const unsigned char byte = static_cast<unsigned char>(value);
            answer += nodes_[p].terminal;
            std::size_t next = nodes_[p].first_child;
            while (next != none && nodes_[next].byte < byte) {
                answer += nodes_[next].subtree;
                next = nodes_[next].next_sibling;
            }
            if (next == none || nodes_[next].byte != byte) return answer;
            p = next;
        }
        return answer;
    }
    std::optional<std::string> kth(std::size_t k) const {
        if (k >= size()) return std::nullopt;
        std::string answer;
        std::size_t p = 0;
        for (;;) {
            if (k < nodes_[p].terminal) return answer;
            k -= nodes_[p].terminal;
            p = nodes_[p].first_child;
            while (k >= nodes_[p].subtree) {
                k -= nodes_[p].subtree;
                p = nodes_[p].next_sibling;
            }
            answer.push_back(static_cast<char>(nodes_[p].byte));
        }
    }
    std::optional<std::string> lower_bound(std::string_view word) const {
        return kth(count_less(word));
    }
    std::vector<std::pair<std::string, std::size_t>> items(
        std::string_view prefix = {}, std::size_t limit = none) const {
        std::vector<std::pair<std::string, std::size_t>> result;
        if (limit == 0) return result;
        const std::size_t p = find(prefix);
        if (p == none || nodes_[p].subtree == 0) return result;
        std::string word(prefix);
        if (nodes_[p].terminal != 0) {
            result.emplace_back(word, nodes_[p].terminal);
            if (result.size() == limit) return result;
        }
        std::vector<std::size_t> pending{nodes_[p].first_child};
        while (!pending.empty()) {
            const std::size_t next = pending.back();
            if (next == none) {
                pending.pop_back();
                if (!pending.empty()) word.pop_back();
                continue;
            }
            pending.back() = nodes_[next].next_sibling;
            word.push_back(static_cast<char>(nodes_[next].byte));
            if (nodes_[next].terminal != 0) {
                result.emplace_back(word, nodes_[next].terminal);
                if (result.size() == limit) return result;
            }
            pending.push_back(nodes_[next].first_child);
        }
        return result;
    }
    void clear() noexcept {
        if (!nodes_.empty()) {
            nodes_.resize(1);
            nodes_[0] = Node{};
        }
        tables_.clear();
        free_table_ = no_table;
        free_head_ = none;
        live_ = 1;
        distinct_ = 0;
    }
    void reserve_nodes(std::size_t n) { nodes_.reserve(n); }
    void swap(Trie& other) noexcept {
        nodes_.swap(other.nodes_);
        tables_.swap(other.tables_);
        std::swap(free_table_, other.free_table_);
        std::swap(free_head_, other.free_head_);
        std::swap(live_, other.live_);
        std::swap(distinct_, other.distinct_);
    }
    friend void swap(Trie& a, Trie& b) noexcept { a.swap(b); }
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

#include <string>

int main() {
    int n{}; verify_in >> n;
    Trie dictionary;
    int answer = 0;
    while (n--) {
        std::string word; verify_in >> word;
        if (!dictionary.contains(word)) {
            dictionary.insert(word);
            ++answer;
        }
    }
    verify_out << answer << '\n';
}
