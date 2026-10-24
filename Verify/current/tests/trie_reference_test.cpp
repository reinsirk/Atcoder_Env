// SPDX-License-Identifier: CC0-1.0
#ifndef TRIE_HEADER
#define TRIE_HEADER "../../../String/Trie.cpp"
#endif
#include TRIE_HEADER
#include TRIE_HEADER
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <vector>

struct UnsignedLex {
    bool operator()(std::string_view a, std::string_view b) const {
        const std::size_t n = std::min(a.size(), b.size());
        for (std::size_t i = 0; i < n; ++i) {
            const auto x = static_cast<unsigned char>(a[i]);
            const auto y = static_cast<unsigned char>(b[i]);
            if (x != y) return x < y;
        }
        return a.size() < b.size();
    }
};
using Oracle = std::map<std::string, std::size_t, UnsignedLex>;
using Entry = std::pair<std::string, std::size_t>;
static bool prefix(std::string_view s, std::string_view p) {
    return s.size() >= p.size() && s.substr(0, p.size()) == p;
}
static std::size_t weight(const Oracle& ref) {
    std::size_t n = 0;
    for (const auto& [s, c] : ref) { (void)s; n += c; }
    return n;
}
static std::vector<Entry> oracle_items(const Oracle& ref, std::string_view p, std::size_t limit) {
    std::vector<Entry> out;
    for (const auto& [s, c] : ref) if (prefix(s, p) && out.size() < limit) out.emplace_back(s, c);
    return out;
}
static std::optional<std::string> oracle_kth(const Oracle& ref, std::size_t k) {
    for (const auto& [s, c] : ref) { if (k < c) return s; k -= c; }
    return std::nullopt;
}
static void query(const Trie& trie, const Oracle& ref, const std::string& q) {
    const auto before = trie.node_count();
    const auto cap = trie.capacity();
    const auto found = ref.find(q);
    const std::size_t c = found == ref.end() ? 0 : found->second;
    assert(trie.count(q) == c);
    assert(trie.contains(q) == (c != 0));
    std::size_t pc = 0, smaller = 0;
    std::optional<std::size_t> longest;
    for (const auto& [s, copies] : ref) {
        if (prefix(s, q)) pc += copies;
        if (UnsignedLex{}(s, q)) smaller += copies;
        if (prefix(q, s) && (!longest || s.size() > *longest)) longest = s.size();
    }
    assert(trie.prefix_count(q) == pc);
    assert(trie.starts_with(q) == (pc != 0));
    assert(trie.longest_prefix(q) == longest);
    assert(trie.count_less(q) == smaller);
    const auto bound = ref.lower_bound(q);
    const auto expected_bound = bound == ref.end() ? std::nullopt : std::optional<std::string>(bound->first);
    assert(trie.lower_bound(q) == expected_bound);
    for (const std::size_t limit : {std::size_t{0}, std::size_t{1}, std::size_t{4}, std::numeric_limits<std::size_t>::max()}) {
        assert(trie.items(q, limit) == oracle_items(ref, q, limit));
    }
    assert(trie.node_count() == before);
    assert(trie.capacity() == cap);
}
static void full(const Trie& trie, const Oracle& ref) {
    const std::size_t n = weight(ref);
    assert(trie.size() == n);
    assert(trie.empty() == ref.empty());
    assert(trie.distinct_size() == ref.size());
    std::set<std::string, UnsignedLex> prefixes;
    prefixes.emplace();
    for (const auto& [s, c] : ref) {
        assert(c != 0);
        for (std::size_t i = 1; i <= s.size(); ++i) prefixes.insert(s.substr(0, i));
    }
    assert(trie.node_count() == prefixes.size());
    assert(trie.capacity() >= trie.node_count() || trie.empty());
    assert(trie.items() == oracle_items(ref, "", std::numeric_limits<std::size_t>::max()));
    query(trie, ref, "");
    assert(trie.kth(n) == std::nullopt);
    assert(trie.kth(std::numeric_limits<std::size_t>::max()) == std::nullopt);
    if (n < 160) for (std::size_t i = 0; i < n; ++i) assert(trie.kth(i) == oracle_kth(ref, i));
    else for (const auto k : {std::size_t{0}, n / 3, n / 2, n - 1}) assert(trie.kth(k) == oracle_kth(ref, k));
}
static std::string random_word(std::mt19937_64& gen, std::size_t max_len = 9) {
    std::string s(gen() % (max_len + 1), '\0');
    for (auto& ch : s) ch = static_cast<char>(gen() % 256);
    return s;
}
static void deterministic() {
    Trie t;
    Oracle ref;
    full(t, ref);
    assert(t.insert("missing", 0) == 0);
    assert(t.erase("missing", 0));
    assert(!t.erase("missing"));
    assert(t.erase_all("missing") == 0);
    full(t, ref);
    std::vector<std::string> words = {"", "a", "ab", "abc", "abd", "b", std::string("a\0", 2), std::string("\0", 1), std::string("\0\0", 2)};
    for (unsigned i = 0; i < 256; ++i) {
        std::string s(1, static_cast<char>(i));
        words.push_back(s);
        words.push_back(s + "a");
        words.push_back("a" + s);
    }
    for (std::size_t i = 0; i < words.size(); ++i) {
        const auto copies = i % 5 + 1;
        ref[words[i]] += copies;
        assert(t.insert(words[i], copies) == ref[words[i]]);
    }
    full(t, ref);
    for (const auto& s : words) {
        query(t, ref, s);
        query(t, ref, s.substr(0, s.size() / 2));
        query(t, ref, s + std::string("\0\xff", 2));
    }
    Trie copy(t);
    full(copy, ref);
    copy.insert("unique-copy-word");
    full(t, ref);
    Trie assigned;
    assigned.insert("preexisting");
    assigned = t;
    full(assigned, ref);
    Trie moved(std::move(copy));
    assert(copy.empty() && copy.node_count() == 1);
    copy.insert("reuse");
    assert(copy.count("reuse") == 1);
    assert(moved.count("unique-copy-word") == 1);
    Trie move_assigned;
    move_assigned.insert("old");
    move_assigned = std::move(assigned);
    full(move_assigned, ref);
    assert(assigned.empty());
    assigned.insert("again", 2);
    assert(assigned.count("again") == 2);
    t.swap(assigned);
    full(assigned, ref);
    assert(t.count("again") == 2);
    t.swap(assigned);
    full(t, ref);
    const std::size_t capacity = t.capacity();
    t.reserve_nodes(capacity + 100);
    assert(t.capacity() >= capacity + 100);
    full(t, ref);
    for (const auto& s : words) {
        const auto it = ref.find(s);
        if (it == ref.end()) continue;
        const auto c = it->second;
        assert(!t.erase(s, c + 1));
        assert(t.erase(s, 0));
        assert(t.insert(s, 0) == c);
        query(t, ref, s);
        if (c > 1) { assert(t.erase(s, c - 1)); it->second = 1; }
        assert(t.erase_all(s) == it->second);
        ref.erase(it);
    }
    full(t, ref);
    t.clear();
    full(t, ref);
    for (unsigned i = 0; i < 256; ++i) {
        std::string s(500, static_cast<char>(i));
        const auto old_cap = t.capacity();
        t.insert(s);
        const auto new_cap = t.capacity();
        assert(new_cap >= old_cap);
        assert(t.node_count() == 501);
        t.erase_all(s);
        assert(t.node_count() == 1);
        if (i > 0) assert(new_cap == old_cap);
    }
}

static void degree_transitions() {
    for (const std::string& base : {std::string(""), std::string("p"), std::string("pq")}) {
        Trie t;
        Oracle ref;
        unsigned active = 0;
        for (const unsigned target : {7u, 8u, 15u, 16u, 17u, 256u, 17u, 16u, 15u, 8u, 7u, 0u, 17u, 7u, 16u, 0u, 256u, 0u}) {
            while (active < target) {
                const std::string word = base + static_cast<char>(active);
                const auto copies = active % 3 + 1;
                ref[word] += copies;
                assert(t.insert(word, copies) == ref[word]);
                ++active;
            }
            while (active > target) {
                --active;
                const std::string word = base + static_cast<char>(active);
                assert(t.erase_all(word) == ref.at(word));
                ref.erase(word);
            }
            full(t, ref);
            for (unsigned i = 0; i < 256; ++i) query(t, ref, base + static_cast<char>(i));
            Trie copied(t);
            full(copied, ref);
            Trie assigned;
            assigned.insert("replaced");
            assigned = t;
            full(assigned, ref);
            const Trie& alias = assigned;
            assigned = alias;
            full(assigned, ref);
            Trie moved(std::move(copied));
            full(moved, ref);
            assert(copied.empty());
            copied.insert("move reuse");
            Trie other;
            t.swap(other);
            assert(t.empty());
            full(other, ref);
            t.swap(other);
            moved.clear();
            full(moved, Oracle{});
        }
    }
}

static void overflow() {
    const auto maximum = std::numeric_limits<std::size_t>::max();
    Trie t;
    assert(t.insert("", maximum) == maximum);
    assert(t.size() == maximum);
    assert(t.distinct_size() == 1);
    assert(t.prefix_count("") == maximum);
    assert(t.count_less("a") == maximum);
    assert(t.kth(maximum - 1) == std::optional<std::string>(""));
    assert(t.kth(maximum) == std::nullopt);
    assert(t.insert("", 0) == maximum);
    for (const std::string& word : {std::string(""), std::string("new")}) {
        bool threw = false;
        try { (void)t.insert(word); } catch (const std::overflow_error&) { threw = true; } catch (const std::length_error&) { threw = true; }
        assert(threw);
        assert(t.size() == maximum && t.count("") == maximum && t.distinct_size() == 1 && t.node_count() == 1);
    }
    assert(t.erase("", maximum - 1));
    assert(t.size() == 1 && t.count("") == 1);
    assert(t.erase_all("") == 1);
    assert(t.empty());
    assert(t.insert("", maximum) == maximum);
    assert(t.erase_all("") == maximum);
    assert(t.empty());
    bool threw = false;
    try { t.reserve_nodes(maximum); } catch (const std::exception&) { threw = true; }
    assert(threw && t.empty() && t.node_count() == 1);
    t.insert("usable");
    assert(t.contains("usable"));
    t.clear();
    assert(t.insert("a", maximum - 1) == maximum - 1);
    assert(t.insert("b") == 1);
    assert(t.size() == maximum && t.distinct_size() == 2);
    assert(t.kth(maximum - 2) == std::optional<std::string>("a"));
    assert(t.kth(maximum - 1) == std::optional<std::string>("b"));
    assert(t.count_less("b") == maximum - 1);
    assert(t.count_less("c") == maximum);
    threw = false;
    try { (void)t.insert("a"); } catch (const std::overflow_error&) { threw = true; } catch (const std::length_error&) { threw = true; }
    assert(threw);
    assert(t.count("a") == maximum - 1 && t.count("b") == 1 && t.size() == maximum);
    assert(!t.erase("a", maximum));
    assert(t.count("a") == maximum - 1);
    assert(t.erase_all("a") == maximum - 1);
    assert(t.size() == 1 && t.contains("b"));
}
static void randomized() {
    std::mt19937_64 gen(0xa1b2c3d4e5f60718ULL);
    for (unsigned round = 0; round < 4; ++round) {
        Trie t;
        Oracle ref;
        std::vector<std::string> pool = {"", "a", "aa", "aaa", "ab", "b", std::string("\0", 1), std::string("\xff", 1)};
        for (unsigned i = 0; i < 240; ++i) pool.push_back(random_word(gen));
        for (unsigned step = 0; step < 3500; ++step) {
            const std::string& word = pool[gen() % pool.size()];
            const auto copies = gen() % 6;
            const auto op = gen() % 13;
            auto it = ref.find(word);
            const auto previous = it == ref.end() ? 0 : it->second;
            if (op < 5) {
                const auto result = t.insert(word, copies);
                if (copies) ref[word] += copies;
                assert(result == previous + copies);
            } else if (op < 8) {
                assert(t.erase(word, copies) == (previous >= copies));
                if (copies && previous >= copies) {
                    if (previous == copies) ref.erase(word);
                    else ref[word] -= copies;
                }
            } else if (op == 8) {
                assert(t.erase_all(word) == previous);
                ref.erase(word);
            } else if (op == 9) {
                t.reserve_nodes(t.node_count() + gen() % 30);
            } else if (op == 10 && step % 73 == 0) {
                Trie other(t);
                full(other, ref);
                t = std::move(other);
                assert(other.empty());
                other.insert("temporary");
            } else if (op == 11 && step % 127 == 0) {
                t.clear();
                ref.clear();
            }
            query(t, ref, word);
            query(t, ref, word.substr(0, word.size() / 2));
            const auto q = random_word(gen);
            query(t, ref, q);
            if (step % 17 == 0) full(t, ref);
        }
        full(t, ref);
    }
}
static void deep() {
    const std::string word(160000, 'z');
    Trie t;
    t.insert(word, 3);
    assert(t.node_count() == word.size() + 1);
    assert(t.count(word) == 3);
    assert(t.longest_prefix(word + "tail") == std::optional<std::size_t>(word.size()));
    assert(t.prefix_count(std::string_view(word).substr(0, 150000)) == 3);
    assert(t.kth(2) == std::optional<std::string>(word));
    assert(t.lower_bound("a") == std::optional<std::string>(word));
    assert((t.items() == std::vector<Entry>{{word, 3}}));
    Trie copied = t;
    assert(copied.count(word) == 3);
    assert(copied.erase(word, 2));
    assert(copied.erase_all(word) == 1);
    assert(copied.empty() && copied.node_count() == 1);
    { Trie destroyed = t; assert(destroyed.size() == 3); }
    t.clear();
    assert(t.empty() && t.node_count() == 1);
    t.insert(word);
    assert(t.erase(word));
    assert(t.empty() && t.node_count() == 1);
}

static std::shared_ptr<TrieNode> compatibility_chain(std::size_t length, std::weak_ptr<TrieNode>& terminal) {
    auto root = std::make_shared<TrieNode>();
    terminal = root;
    for (std::size_t i = 0; i < length; ++i) {
        auto next = std::make_shared<TrieNode>();
        next->children.emplace('x', root);
        root = std::move(next);
    }
    return root;
}
static void compatibility() {
    {
        auto leaf = std::make_shared<TrieNode>();
        const std::weak_ptr<TrieNode> weak = leaf;
        std::unordered_map<char, std::shared_ptr<TrieNode>> source;
        source.emplace('a', leaf);
        source.emplace(static_cast<char>(255), leaf);
        source.emplace('\0', nullptr);
        TrieNode from_map(source);
        assert(from_map.children.size() == 3 && from_map.children.at('a') == leaf);
        TrieNode copied(from_map);
        assert(copied.children.at('a') == from_map.children.at('a'));
        auto extra = std::make_shared<TrieNode>();
        copied.children.at('a')->children.emplace('q', extra);
        assert(from_map.children.at('a')->children.at('q') == extra);
        TrieNode assigned;
        assigned = from_map;
        assert(assigned.children.at('a') == leaf);
        TrieNode moved(std::move(copied));
        assert(moved.children.at('a') == leaf);
        copied.children.emplace('r', extra);
        assert(copied.children.at('r') == extra);
        TrieNode move_assigned;
        move_assigned = std::move(assigned);
        assert(move_assigned.children.at('a') == leaf);
        assigned.children.emplace('s', extra);
        assert(assigned.children.at('s') == extra);
        source.clear();
        leaf.reset();
        assert(!weak.expired());
    }
    {
        std::weak_ptr<TrieNode> terminal;
        auto root = compatibility_chain(160000, terminal);
        assert(!terminal.expired());
        root.reset();
        assert(terminal.expired());
    }
    {
        std::weak_ptr<TrieNode> terminal;
        auto root = compatibility_chain(90000, terminal);
        TrieNode stack_root;
        stack_root.children.emplace('x', root);
        root.reset();
        assert(!terminal.expired());
        stack_root.children.clear();
        assert(terminal.expired());
    }
    {
        auto root = std::make_shared<TrieNode>();
        std::weak_ptr<TrieNode> last_leaf;
        for (std::size_t i = 0; i < 70000; ++i) {
            auto next = std::make_shared<TrieNode>();
            auto leaf = std::make_shared<TrieNode>();
            last_leaf = leaf;
            next->children.emplace('x', root);
            next->children.emplace('y', leaf);
            root = std::move(next);
        }
        assert(!last_leaf.expired());
        root.reset();
        assert(last_leaf.expired());
    }
    {
        auto suffix = std::make_shared<TrieNode>();
        const std::weak_ptr<TrieNode> terminal = suffix;
        auto root = suffix;
        for (std::size_t i = 0; i < 100000; ++i) {
            auto next = std::make_shared<TrieNode>();
            next->children.emplace('a', root);
            next->children.emplace('b', root);
            root = std::move(next);
        }
        TrieNode copy(*root);
        root.reset();
        assert(!terminal.expired());
        copy.children.clear();
        assert(!terminal.expired());
        assert(suffix->children.empty());
        suffix.reset();
        assert(terminal.expired());
    }
    {
        std::weak_ptr<TrieNode> terminal;
        auto suffix = compatibility_chain(50000, terminal);
        auto root = std::make_shared<TrieNode>();
        root->children.emplace('a', suffix);
        root->children.emplace('b', suffix);
        const std::weak_ptr<TrieNode> suffix_observer = suffix;
        root.reset();
        assert(!suffix_observer.expired() && !terminal.expired());
        suffix->children.emplace('z', std::make_shared<TrieNode>());
        suffix.reset();
        assert(suffix_observer.expired() && terminal.expired());
    }
}

int main() {
    deterministic();
    degree_transitions();
    overflow();
    randomized();
    deep();
    compatibility();
    std::cout << "trie independent tests passed\n";
}
