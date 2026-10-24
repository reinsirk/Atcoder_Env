// Verify: ローカル独立検査

// Library: String/Trie.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/trie.md
//
// API一覧
// [TrieNode]
// TrieNode(): 空で生成。TrieNode(children): 子mapを受け取り、コピーは子を共有する。
// children: 指すノード自体を所有する子（別対象の所有を借りるaliasingは対象外）。
// 破棄は反復処理。循環所有は解消しない。指定初期化は不可。
//
// [Trie]
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
// count/contains/prefix系/longest_prefixは長さLに対しO(16L)、size系はO(1)。
// insertは償却O(16L+256)、再確保時は保持ノード/索引枠数ぶんのコピーも発生。
// erase/erase_allはO(16L+256)。count_less/kthは語長L/戻す長さKに対しO(256L)/O(256K)。
// lower_boundはO(256(L+K))。itemsは接頭辞探索＋訪問ノード数＋返す文字列総長。
// 保持ノード数N・保持索引数TとしてコピーはO(N+256T)、破棄・clearはO(N+T)。
// reserve_nodesは最悪O(N)。使用メモリはノード枠数＋256×索引枠数に比例。
// 個数・ノード枠の上限超過は検査省略時もlength_error。確保失敗はbad_alloc。

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

#ifndef SIRKIT_TRIE_NODE_DEFINED
#define SIRKIT_TRIE_NODE_DEFINED
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

#ifndef SIRKIT_TRIE_DEFINED
#define SIRKIT_TRIE_DEFINED
// 子はソート済み兄弟リスト。次数16で256要素の索引を追加、8未満で回収。
// 破棄・clear・列挙は再帰しない。64文字までの追加経路は固定長の作業配列へ記録。
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
        // 新規ノードだけの末尾は単一路としてまとめて接続する。
        if (needed != 0 && free_head_ == none) {
            const std::size_t first = fresh;
            for (std::size_t i = matched; i < word.size(); ++i) {
                auto& node = nodes_[fresh++];
                node.byte = static_cast<unsigned char>(word[i]);
                node.subtree = copies;
                if (i + 1 != word.size()) {
                    node.first_child = fresh;
                    node.degree = 1;
                }
            }
            const unsigned char byte = static_cast<unsigned char>(word[matched]);
            std::size_t* link = &nodes_[p].first_child;
            while (*link != none && nodes_[*link].byte < byte)
                link = &nodes_[*link].next_sibling;
            nodes_[first].next_sibling = *link;
            *link = first;
            ++nodes_[p].degree;
            if (nodes_[p].table != no_table) tables_[nodes_[p].table].entries[byte] = first;
            live_ += needed;
            p = fresh - 1;
        } else for (std::size_t i = matched; i < word.size(); ++i) {
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
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("Trie reference mismatch");};
    auto less=[](const std::string&a,const std::string&b){return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end(),[](char x,char y){return static_cast<unsigned char>(x)<static_cast<unsigned char>(y);});};
    std::map<std::string,std::size_t,decltype(less)> ref(less);Trie trie;std::mt19937 rng(918273);
    for(int step=0;step<1200;++step){std::string s;for(unsigned j=0,len=rng()%6;j<len;++j)s+=static_cast<char>(rng()%256);
        const unsigned mode=rng()%4,copies=rng()%4;auto it=ref.find(s);std::size_t count=it==ref.end()?0:it->second;
        if(mode==0){const auto result=trie.insert(s,copies);check(result==count+copies);if(copies)ref[s]+=copies;}
        else if(mode==1){check(trie.erase(s,copies)==(count>=copies));if(count>=copies&&copies){if(count==copies)ref.erase(s);else ref[s]-=copies;}}
        else if(mode==2){check(trie.erase_all(s)==count);ref.erase(s);}
        check(trie.count(s)==(ref.contains(s)?ref.at(s):0));check(trie.contains(s)==ref.contains(s));
        std::vector<std::string> sorted;std::vector<std::pair<std::string,std::size_t>> items;
        for(auto&[word,c]:ref){items.push_back({word,c});for(std::size_t j=0;j<c;++j)sorted.push_back(word);}
        check(trie.size()==sorted.size());check(trie.distinct_size()==ref.size());check(trie.items()==items);
        std::size_t below=0,prefix=0;for(auto&w:sorted){below+=less(w,s);prefix+=w.starts_with(s);}
        check(trie.count_less(s)==below);check(trie.prefix_count(s)==prefix);check(trie.starts_with(s)==(prefix!=0));
        const auto lb=trie.lower_bound(s);check(lb.has_value()==(below<sorted.size()));if(lb)check(*lb==sorted[below]);
        for(std::size_t j=0;j<sorted.size();++j)check(trie.kth(j)==sorted[j]);check(!trie.kth(sorted.size()));
        std::optional<std::size_t> longest;for(auto&[word,c]:ref)if(s.starts_with(word)&&(!longest||word.size()>*longest))longest=word.size();
        check(trie.longest_prefix(s)==longest);
    }
    Trie copy=trie;copy.clear();check(copy.empty());check(copy.size()==0);check(trie.size()!=0);
    Trie moved=std::move(trie);check(trie.empty());trie.insert("reuse");check(trie.contains("reuse"));
    moved.reserve_nodes(10000);moved.clear();check(moved.empty());
    {TrieNode root;root.children['x']=std::make_shared<TrieNode>();TrieNode copy=root;check(copy.children['x']==root.children['x']);}
    std::cout<<"OK\n";
}
