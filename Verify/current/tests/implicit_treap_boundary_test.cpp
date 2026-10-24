// SPDX-License-Identifier: CC0-1.0
#ifndef HEADER
#define HEADER "../../../DataStructure/implicit_treap.cpp"
#endif
#include HEADER
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Text {
    std::string value;
    explicit Text(std::string v) : value(std::move(v)) {}
};
struct CharAffine {
    int a, b;
    explicit CharAffine(int multiplier, int offset) : a(multiplier), b(offset) {}
};
Text concatenate(Text a, Text b) { a.value += b.value; return a; }
Text empty_text() { return Text(""); }
Text transform(CharAffine f, Text x) {
    for (char& c : x.value) c = static_cast<char>('a' + (f.a * (c - 'a') + f.b) % 26);
    return x;
}
CharAffine compose(CharAffine f, CharAffine g) {
    return CharAffine(f.a * g.a % 26, (f.a * g.b + f.b) % 26);
}
CharAffine identity() { return CharAffine(1, 0); }
using TextTree = lazy_implicit_treap<Text, concatenate, empty_text,
                                     CharAffine, transform, compose, identity>;
static std::string forbidden = "ab";
bool no_pattern(Text x) { return x.value.find(forbidden) == std::string::npos; }

void verify_text(TextTree& tree, const std::vector<std::string>& expected) {
    assert(tree.size() == static_cast<int>(expected.size()));
    auto actual = tree.to_vector();
    std::string total;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        assert(actual[i].value == expected[i]);
        total += expected[i];
    }
    assert(tree.all_prod().value == total);
}
void verify_boundaries(TextTree& tree, const std::vector<std::string>& expected) {
    const int n = static_cast<int>(expected.size());
    for (int start = 0; start <= n; ++start) {
        int right = start;
        std::string joined;
        while (right < n && (joined + expected[right]).find(forbidden) == std::string::npos)
            joined += expected[right++];
        const auto predicate = [](Text x) { return no_pattern(std::move(x)); };
        assert(tree.max_right(start, predicate) == right);
        assert(tree.max_right<no_pattern>(start) == right);
    }
    for (int finish = 0; finish <= n; ++finish) {
        int left = finish;
        std::string joined;
        while (left > 0 && (expected[left - 1] + joined).find(forbidden) == std::string::npos)
            joined = expected[--left] + joined;
        const auto predicate = [](Text x) { return no_pattern(std::move(x)); };
        assert(tree.min_left(finish, predicate) == left);
        assert(tree.min_left<no_pattern>(finish) == left);
    }
}
void text_operations() {
    std::mt19937_64 rng(20261022);
    std::vector<std::string> ref{"a", "b", "", "ab", "c", "a", "b", "b", "a"};
    std::vector<Text> initial;
    for (const auto& x : ref) initial.emplace_back(x);
    TextTree tree(initial);
    tree.reserve(256);
    verify_boundaries(tree, ref);
    for (int step = 0; step < 2400; ++step) {
        int n = static_cast<int>(ref.size());
        int l = static_cast<int>(rng() % (n + 1)), r = static_cast<int>(rng() % (n + 1));
        if (l > r) std::swap(l, r);
        switch (rng() % 10) {
        case 0: {
            const auto value = std::string(static_cast<std::size_t>(rng() % 3), static_cast<char>('a' + rng() % 3));
            ref.insert(ref.begin() + l, value); tree.insert_at(l, Text(value)); break;
        }
        case 1:
            if (n) { int pos = static_cast<int>(rng() % n); ref.erase(ref.begin() + pos); tree.erase_at(pos); }
            break;
        case 2:
            std::reverse(ref.begin() + l, ref.begin() + r); tree.reverse(l, r); break;
        case 3: {
            const int m = l + static_cast<int>(rng() % (r - l + 1));
            std::rotate(ref.begin() + l, ref.begin() + m, ref.begin() + r); tree.rotate(l, m, r); break;
        }
        case 4: {
            const CharAffine f(static_cast<int>(rng() % 26), static_cast<int>(rng() % 26));
            for (int i = l; i < r; ++i) ref[i] = transform(f, Text(ref[i])).value;
            tree.apply(l, r, f); break;
        }
        case 5:
            if (n) { int pos = static_cast<int>(rng() % n); ref[pos] = "abc"; tree.set(pos, Text("abc")); }
            break;
        case 6:
            ref.erase(ref.begin() + l, ref.begin() + r); tree.erase(l, r); break;
        case 7: {
            TextTree copy(tree); TextTree assigned; assigned = copy;
            TextTree moved(std::move(copy)); assert(copy.empty()); copy.push_back(Text("x"));
            TextTree target; target.push_back(Text("discard")); target = std::move(assigned);
            assert(assigned.empty()); assigned.push_front(Text("y"));
            verify_text(moved, ref); verify_text(target, ref);
            swap(target, moved); verify_text(moved, ref); break;
        }
        case 8:
            ref.insert(ref.begin(), "a"); tree.push_front(Text("a"));
            ref.push_back("b"); tree.push_back(Text("b"));
            tree.pop_front(); ref.erase(ref.begin()); tree.pop_back(); ref.pop_back(); break;
        default: {
            std::string joined;
            for (int i = l; i < r; ++i) joined += ref[i];
            assert(tree.prod(l, r).value == joined); break;
        }
        }
        if (step % 7 == 0) { forbidden = (step % 2 == 0 ? "ab" : "ba"); verify_boundaries(tree, ref); }
        if (step % 19 == 0) verify_text(tree, ref);
        if (step % 251 == 0) {
            tree.clear(); ref.clear();
            for (int i = 0; i < 32; ++i) { ref.push_back(i % 2 ? "a" : "b"); tree.push_back(Text(ref.back())); }
        }
    }
    verify_text(tree, ref); verify_boundaries(tree, ref);
}


void endpoint_operations() {
    for (int n : {0, 1, 2, 3, 7, 16, 31, 64, 257}) {
        std::vector<std::string> ref;
        std::vector<Text> initial;
        for (int i = 0; i < n; ++i) {
            ref.push_back(std::string(1 + i % 3, static_cast<char>('a' + i % 26)));
            initial.emplace_back(ref.back());
        }
        TextTree tree(initial);
        tree.reserve(static_cast<std::size_t>(n + 64));
        for (int round = 0; round < 24; ++round) {
            int count = static_cast<int>(ref.size());
            const CharAffine first(3, 2), second(7, 11);
            tree.apply(0, count, first);
            for (auto& value : ref) value = transform(first, Text(value)).value;
            tree.apply(count / 3, count, second);
            for (int i = count / 3; i < count; ++i) ref[i] = transform(second, Text(ref[i])).value;
            tree.reverse(0, count); std::reverse(ref.begin(), ref.end());
            tree.rotate(0, count / 2, count); std::rotate(ref.begin(), ref.begin() + count / 2, ref.end());
            // 遅延作用・反転を残したまま端点を削除する。
            if (round % 2) {
                tree.pop_front(); if (!ref.empty()) ref.erase(ref.begin());
                tree.pop_back(); if (!ref.empty()) ref.pop_back();
            } else {
                tree.pop_back(); if (!ref.empty()) ref.pop_back();
                tree.pop_front(); if (!ref.empty()) ref.erase(ref.begin());
            }
            verify_text(tree, ref);
            const auto middle = static_cast<int>(ref.size() / 2);
            tree.insert_at(middle, Text("xy")); ref.insert(ref.begin() + middle, "xy");
            tree.push_front(Text("z")); ref.insert(ref.begin(), "z");
            // 削除スロットを再利用した直後にも作用・反転を繰り返す。
            verify_text(tree, ref);
        }
        tree.reverse(0, tree.size()); std::reverse(ref.begin(), ref.end());
        bool front = false;
        while (!ref.empty()) {
            if (front) { tree.pop_front(); ref.erase(ref.begin()); }
            else { tree.pop_back(); ref.pop_back(); }
            front = !front;
            if (ref.size() % 7 == 0) verify_text(tree, ref);
        }
        assert(tree.empty()); tree.pop_front(); tree.pop_back();
        tree.push_back(Text("again")); verify_text(tree, {"again"});
    }
    lazy_implicit_treap<> sorted;
    std::vector<std::int64_t> values;
    for (int i = 0; i < 129; ++i) { sorted.insert_sorted(i / 3); values.push_back(i / 3); }
    sorted.apply(0, sorted.size(), 5); for (auto& value : values) value += 5;
    while (!values.empty()) {
        sorted.erase_max(); values.pop_back();
        assert(sorted.to_vector() == values);
    }
    sorted.erase_max(); sorted.insert_sorted(9); assert(sorted.get(0) == 9);
}

struct Ordered {
    int key, payload;
    friend bool operator<(Ordered a, Ordered b) { return a.key < b.key; }
    friend bool operator==(Ordered a, Ordered b) { return a.key == b.key && a.payload == b.payload; }
};
Ordered ordered_op(Ordered a, Ordered b) { return b < a ? b : a; }
Ordered ordered_e() { return {std::numeric_limits<int>::max(), 0}; }
Ordered ordered_map(int, Ordered x) { return x; }
int add_action(int a, int b) { return a + b; }
int no_action() { return 0; }
using OrderedTree = lazy_implicit_treap<Ordered, ordered_op, ordered_e, int,
                                        ordered_map, add_action, no_action>;
struct WithoutEqual {
    int key;
    friend bool operator<(WithoutEqual a, WithoutEqual b) { return a.key < b.key; }
};
WithoutEqual weak_op(WithoutEqual a, WithoutEqual b) { return b < a ? b : a; }
WithoutEqual weak_e() { return {std::numeric_limits<int>::max()}; }
WithoutEqual weak_map(int, WithoutEqual x) { return x; }
using WeakTree = lazy_implicit_treap<WithoutEqual, weak_op, weak_e, int,
                                     weak_map, add_action, no_action>;
void sorted_operations() {
    std::mt19937 rng(175031);
    OrderedTree tree; std::vector<Ordered> ref;
    const auto before = [](Ordered a, Ordered b) { return a < b; };
    for (int step = 0; step < 6000; ++step) {
        const Ordered value{static_cast<int>(rng() % 31), static_cast<int>(rng() % 11)};
        auto first = std::lower_bound(ref.begin(), ref.end(), value, before);
        auto last = std::upper_bound(ref.begin(), ref.end(), value, before);
        const int lower = static_cast<int>(first - ref.begin()), upper = static_cast<int>(last - ref.begin());
        const bool found = first != ref.end() && *first == value;
        assert(tree.lower_bound(value) == lower && tree.upper_bound(value) == upper);
        assert(tree.get_rank(value) == lower && tree.count(value) == upper - lower);
        assert(tree.contains(value) == found);
        switch (rng() % 5) {
        case 0: case 1: ref.insert(first, value); tree.insert_sorted(value); break;
        case 2: if (found) ref.erase(first); tree.erase_val(value); break;
        case 3: if (!ref.empty()) ref.pop_back(); tree.erase_max(); break;
        default: break;
        }
        auto actual = tree.to_vector();
        assert(actual == ref);
    }
    // ==の一致はlower_bound位置に対して判定する既存契約。
    OrderedTree distinct(std::vector<Ordered>{{1, 10}, {1, 20}, {2, 30}});
    assert(distinct.count({1, 99}) == 2);
    assert(distinct.contains({1, 10}) && !distinct.contains({1, 20}));
    distinct.erase_val({1, 20}); assert(distinct.size() == 3);
    distinct.erase_val({1, 10}); assert(distinct.contains({1, 20}));
    WeakTree weak;
    for (int x : {2, 1, 2, 3, 1}) weak.insert_sorted({x});
    assert(weak.lower_bound({2}) == 2 && weak.upper_bound({2}) == 4);
    assert(weak.count({2}) == 2 && weak.contains({2}) && !weak.contains({9}));
    weak.erase_val({2}); weak.erase_max(); assert(weak.size() == 3 && weak.count({2}) == 1);
}

template<class Exception, class Action> void throws(Action action) {
    bool caught = false;
    try { action(); } catch (const Exception&) { caught = true; }
    assert(caught);
}
void checked_boundaries() {
    lazy_implicit_treap<> tree;
    tree.pop_front(); tree.pop_back(); tree.erase_max(); tree.erase(0, 0);
    tree.apply(0, 0, 3); tree.reverse(0, 0); tree.rotate(0, 0, 0);
    assert(tree.prod(0, 0) == RangeAddMin::e());
    assert(tree.max_right(0, [](std::int64_t) { return true; }) == 0);
    assert(tree.min_left(0, [](std::int64_t) { return true; }) == 0);
    const auto capacity = static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1;
    throws<std::length_error>([&] { tree.reserve(capacity); });
#ifndef SKIP_RUNTIME_CHECKS
    throws<std::out_of_range>([&] { (void)tree.get(0); });
    throws<std::out_of_range>([&] { tree.set(0, 2); });
    throws<std::out_of_range>([&] { tree.erase_at(0); });
    throws<std::out_of_range>([&] { tree.insert_at(-1, 2); });
    throws<std::out_of_range>([&] { tree.insert_at(1, 2); });
    throws<std::out_of_range>([&] { tree.apply(0, 1, 2); });
    throws<std::out_of_range>([&] { (void)tree.prod(1, 0); });
    throws<std::out_of_range>([&] { tree.reverse(-1, 0); });
    throws<std::out_of_range>([&] { tree.rotate(0, 1, 0); });
    throws<std::out_of_range>([&] { (void)tree.max_right(1, [](std::int64_t) { return true; }); });
    throws<std::out_of_range>([&] { (void)tree.min_left(-1, [](std::int64_t) { return true; }); });
    throws<std::invalid_argument>([&] { (void)tree.max_right(0, [](std::int64_t) { return false; }); });
    throws<std::invalid_argument>([&] { (void)tree.min_left(0, [](std::int64_t) { return false; }); });
    tree.erase(-1, 0); tree.erase(0, 1); tree.erase(1, 0); assert(tree.empty());
#endif
    tree.push_back(5); tree.push_front(2); tree.pop_front(); tree.pop_back();
    assert(tree.empty());
}
int main() {
    checked_boundaries(); text_operations(); endpoint_operations(); sorted_operations();
    std::cout << "PASS checked/empty/capacity, order-sensitive boundary overloads, affine text/rotation/reuse/ownership, sorted comparison contracts\n";
}
