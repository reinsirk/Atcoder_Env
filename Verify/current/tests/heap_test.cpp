// SPDX-License-Identifier: CC0-1.0
#ifndef HEADER
#define HEADER "../../../DataStructure/heap.cpp"
#endif
#include HEADER
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

void verify_top(const Heap& heap, const std::multiset<int>& reference) {
    assert(heap.size() == reference.size());
    assert(heap.empty() == reference.empty());
    assert(heap.top() == (reference.empty() ? -1 : *reference.begin()));
    assert(heap.top_or(873) == (reference.empty() ? 873 : *reference.begin()));
}

void verify_all(Heap heap, const std::multiset<int>& reference) {
    verify_top(heap, reference);
    for (int value : reference) {
        assert(heap.top() == value);
        assert(heap.pop_value() == value);
    }
    assert(heap.empty());
    heap.pop();
    assert(heap.top() == -1);
}

int reference_push_pop(std::multiset<int>& reference, int value) {
    reference.insert(value);
    const auto first = reference.begin();
    const int result = *first;
    reference.erase(first);
    return result;
}

void boundaries() {
    constexpr int low = std::numeric_limits<int>::min();
    constexpr int high = std::numeric_limits<int>::max();
    Heap empty;
    verify_all(empty, {});
    empty.reserve(0);
    empty.reserve(64);
    empty.clear();
    assert(empty.push_pop(low) == low);
    assert(empty.push_pop(high) == high);
    assert(empty.empty());
#ifndef SKIP_RUNTIME_CHECKS
    bool caught = false;
    try { (void)empty.pop_value(); } catch (const std::out_of_range&) { caught = true; }
    assert(caught && empty.empty());
    caught = false;
    try { (void)empty.replace_top(3); } catch (const std::out_of_range&) { caught = true; }
    assert(caught && empty.empty());
#endif
    Heap initialized{low, high, 0, low, -1, 1};
    verify_all(initialized, {low, high, 0, low, -1, 1});
    const std::array values{high, low, 7, 7};
    Heap iterator(values.begin(), values.end());
    verify_all(iterator, {high, low, 7, 7});
    // 単一パスの入力イテレータからも構築する。
    std::istringstream input("-2147483648 2147483647 0 -1 0");
    Heap stream{std::istream_iterator<int>(input), std::istream_iterator<int>()};
    verify_all(stream, {low, high, 0, -1, 0});
    bool capacity_caught = false;
    try { initialized.reserve(std::numeric_limits<std::size_t>::max()); }
    catch (const std::length_error&) { capacity_caught = true; }
    assert(capacity_caught);
    verify_all(initialized, {low, high, 0, low, -1, 1});
}

void exhaustive() {
    constexpr std::array probes{std::numeric_limits<int>::min(), -2, -1, 0, 1, 2,
                                std::numeric_limits<int>::max()};
    std::size_t possibilities = 1;
    for (std::size_t count = 0; count <= 8; ++count) {
        for (std::size_t code = 0; code < possibilities; ++code) {
            std::size_t encoded = code;
            std::vector<int> values(count);
            for (int& value : values) { value = static_cast<int>(encoded % 3) - 1; encoded /= 3; }
            const std::multiset<int> reference(values.begin(), values.end());
            Heap constructed(values);
            verify_all(constructed, reference);
            Heap pushed;
            pushed.reserve(count);
            for (int value : values) pushed.push(value);
            verify_all(pushed, reference);
            for (int value : probes) {
                Heap edited = constructed;
                auto expected = reference;
                assert(edited.push_pop(value) == reference_push_pop(expected, value));
                verify_all(edited, expected);
                if (!reference.empty()) {
                    edited = constructed;
                    expected = reference;
                    const int old = *expected.begin();
                    expected.erase(expected.begin());
                    expected.insert(value);
                    assert(edited.replace_top(value) == old);
                    verify_all(edited, expected);
                }
            }
        }
        possibilities *= 3;
    }
}

void random_operations(std::uint32_t seed) {
    std::mt19937 random(seed);
    Heap heap;
    std::multiset<int> reference;
    for (std::size_t step = 0; step < 150'000; ++step) {
        const auto operation = random() % 12;
        int value = static_cast<int>(random() % 4001) - 2000;
        if (step % 97 == 0) value = std::numeric_limits<int>::min();
        if (step % 101 == 0) value = std::numeric_limits<int>::max();
        if (operation < 5) {
            heap.push(value); reference.insert(value);
        } else if (operation == 5) {
            heap.pop();
            if (!reference.empty()) reference.erase(reference.begin());
        } else if (operation == 6 && !reference.empty()) {
            assert(heap.pop_value() == *reference.begin());
            reference.erase(reference.begin());
        } else if (operation == 7 && !reference.empty()) {
            assert(heap.replace_top(value) == *reference.begin());
            reference.erase(reference.begin()); reference.insert(value);
        } else if (operation == 8) {
            assert(heap.push_pop(value) == reference_push_pop(reference, value));
        } else if (operation == 9 && step % 257 == 0) {
            heap.clear(); reference.clear();
        } else if (operation == 10) {
            heap.reserve(reference.size() + random() % 128);
        }
        verify_top(heap, reference);
        if (step % 4096 == 0) {
            verify_all(heap, reference);
            Heap copied = heap;
            copied.push(value);
            auto expected = reference; expected.insert(value);
            verify_all(copied, expected);
            verify_all(heap, reference);
            Heap moved = std::move(copied);
            verify_all(moved, expected);
            copied.clear(); copied.push(7); verify_all(copied, {7});
            Heap assigned;
            assigned = heap;
            verify_all(assigned, reference);
            assigned = std::move(moved);
            verify_all(assigned, expected);
            moved.clear(); moved.push(-7); verify_all(moved, {-7});
        }
    }
    verify_all(heap, reference);
}

void capacity_boundaries() {
    std::mt19937 random(87231);
    for (std::size_t count : {9u, 15u, 16u, 17u, 31u, 32u, 33u, 63u, 64u, 65u,
                              255u, 256u, 257u, 1023u, 1024u, 1025u,
                              4095u, 4096u, 4097u, 4098u,
                              65535u, 65536u, 65537u}) {
        std::vector<int> values(count);
        for (int& value : values) value = static_cast<int>(random() % 2'000'001) - 1'000'000;
        values[0] = std::numeric_limits<int>::max();
        values.back() = std::numeric_limits<int>::min();
        std::multiset<int> reference(values.begin(), values.end());
        Heap heap(std::move(values));
        verify_all(heap, reference);
        for (int step = 0; step < 128; ++step) {
            const int value = step % 2 == 0 ? std::numeric_limits<int>::max()
                                          : std::numeric_limits<int>::min();
            assert(heap.replace_top(value) == *reference.begin());
            reference.erase(reference.begin()); reference.insert(value);
            assert(heap.push_pop(-step) == reference_push_pop(reference, -step));
            heap.pop(); reference.erase(reference.begin());
            heap.push(step); reference.insert(step);
            verify_top(heap, reference);
        }
        verify_all(heap, reference);
    }
}

void root_replacement_boundaries() {
    for (std::size_t count : {4095u, 4096u, 4097u, 4098u}) {
        for (int shape = 0; shape < 3; ++shape) {
            std::vector<int> values(count, 100);
            if (shape != 0) {
                values[0] = 0;
                values[1] = shape == 1 ? 1 : 2;
                values[2] = shape == 1 ? 2 : 1;
            }
            const std::multiset<int> reference(values.begin(), values.end());
            Heap heap(std::move(values));
            for (int value : {-1, 0, 1, 2, 99, 100, 101}) {
                Heap edited = heap;
                auto expected = reference;
                assert(edited.replace_top(value) == *expected.begin());
                expected.erase(expected.begin()); expected.insert(value);
                verify_all(edited, expected);
                edited = heap;
                expected = reference;
                assert(edited.push_pop(value) == reference_push_pop(expected, value));
                verify_all(edited, expected);
            }
            verify_all(heap, reference);
        }
    }
}

int main() {
    boundaries();
    exhaustive();
    capacity_boundaries();
    root_replacement_boundaries();
    for (std::uint32_t seed : {1u, 20261022u, 0xffffffffu}) random_operations(seed);
    std::cout << "PASS heap API, exhaustive small states, 450000 updates, ownership and boundaries\n";
}
