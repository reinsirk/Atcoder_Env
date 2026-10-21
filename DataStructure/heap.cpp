// SPDX-License-Identifier: CC0-1.0

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef SIRKIT_HEAP_DEFINED
#define SIRKIT_HEAP_DEFINED

// int の最小ヒープ。値を所有し、コピー・ムーブ可能。
// Heap(): 空を構築。Heap(vector<int>): 値を受け取り O(N) で構築。
// Heap({値...}), Heap(first,last): 値をコピーし O(N) で構築。
// empty(), size(): 空判定・要素数を O(1) で返す。
// top(): 最小値、空なら -1 を O(1) で返す。top_or(fallback): 空なら fallback。
// push(value): 値を追加、償却 O(log N)。pop(): 最小値を削除、空なら何もしない、O(log N)。
// pop_value(): 最小値を削除して返す、O(log N)。空なら out_of_range、検査省略時は非空が前提。
// replace_top(value): 最小値を置換して旧値を返す、O(log N)。空の扱いは pop_value() と同じ。
// push_pop(value): 値を追加した後の最小値を返して削除、O(log N)、空なら value を返す。
// clear(): 全要素を削除、容量を保持。reserve(n): n 要素分の容量を確保、再確保時 O(N)。
class Heap {
    std::vector<int> values_;

    void make_heap() {
        std::make_heap(values_.begin(), values_.end(), std::greater<int>{});
    }

    void repair_root(int value) noexcept {
        const std::size_t count = values_.size();
        if (count == 1) {
            values_[0] = value;
            return;
        }
        std::size_t position;
        if (count > 2 && values_[2] < values_[1]) {
            if (value <= values_[2]) {
                values_[0] = value;
                return;
            }
            values_[0] = values_[2];
            position = 2;
        } else {
            if (value <= values_[1]) {
                values_[0] = value;
                return;
            }
            values_[0] = values_[1];
            position = 1;
        }
        while (position < count / 2) {
            const std::size_t left = position * 2 + 1;
            const std::size_t right = left + 1;
            if (right < count && values_[right] < values_[left]) {
                if (value <= values_[right]) break;
                values_[position] = values_[right];
                position = right;
            } else {
                if (value <= values_[left]) break;
                values_[position] = values_[left];
                position = left;
            }
        }
        values_[position] = value;
    }

public:
    Heap() = default;

    explicit Heap(std::vector<int> values) : values_(std::move(values)) {
        make_heap();
    }

    Heap(std::initializer_list<int> values) : values_(values) {
        make_heap();
    }

    template <std::input_iterator Iterator>
    Heap(Iterator first, Iterator last) : values_(first, last) {
        make_heap();
    }

    [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

    [[nodiscard]] int top() const noexcept { return top_or(-1); }
    [[nodiscard]] int top_or(int fallback) const noexcept {
        return empty() ? fallback : values_.front();
    }

    void push(int value) {
        values_.push_back(value);
        std::size_t position = values_.size() - 1;
        while (position != 0) {
            const std::size_t parent = (position - 1) / 2;
            if (values_[parent] <= value) break;
            values_[position] = values_[parent];
            position = parent;
        }
        values_[position] = value;
    }

    void pop() noexcept {
        if (empty()) return;
        const int value = values_.back();
        values_.pop_back();
        if (!values_.empty()) repair_root(value);
    }

    int pop_value() {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty()) throw std::out_of_range("Heap::pop_value: empty heap");
#endif
        const int result = values_.front();
        const int value = values_.back();
        values_.pop_back();
        if (!values_.empty()) repair_root(value);
        return result;
    }

    int replace_top(int value) {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty()) throw std::out_of_range("Heap::replace_top: empty heap");
#endif
        const int result = values_.front();
        repair_root(value);
        return result;
    }

    int push_pop(int value) noexcept {
        if (empty() || value <= values_.front()) return value;
        const int result = values_.front();
        repair_root(value);
        return result;
    }

    void clear() noexcept { values_.clear(); }
    void reserve(std::size_t count) { values_.reserve(count); }
};

#endif
