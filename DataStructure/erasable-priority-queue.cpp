// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/erasable-priority-queue.hpp

#include <cstddef>
#include <functional>
#include <optional>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_ERASABLE_PRIORITY_QUEUE_DEFINED
#define REINSIRK_ERASABLE_PRIORITY_QUEUE_DEFINED

// ErasablePriorityQueue<T, Minimize>: trueなら最小値、falseなら最大値を先頭にする。
// ErasablePriorityQueue(values): vectorを受け取りO(n)で一括構築する。
// push(value), push(T&&): 1要素を追加する。O(log H)。
// emplace(args...): 引数から要素を構築して追加する。O(log H)。
// top(): 先頭の値を返す。空でないこと。遅延削除を含め償却O(log H)。
// pop(): 先頭を1個削除する。空でないこと。償却O(log H)。
// erase(value): 存在する値を1個遅延削除する。存在・個数は検査しない。O(log H)。
// empty(), size(): 有効な要素の有無・個数を返す。const、O(1)。
// try_top(): 空ならnullopt、そうでなければ先頭の値を返す。償却O(log H)。
// extract_top(): 空ならnullopt、そうでなければ先頭を1個削除して返す。償却O(log H)。
// clear(): 全要素と遅延削除を破棄する。O(H)。Hは未回収の削除済み要素も含む保存数。
// Tの比較は厳密弱順序を満たし、==はその同値関係と一致すること。
template <typename T, bool Minimize = true>
class ErasablePriorityQueue {
public:
    using Queue = std::conditional_t<Minimize,
        std::priority_queue<T, std::vector<T>, std::greater<T>>,
        std::priority_queue<T>>;

    ErasablePriorityQueue() = default;
    explicit ErasablePriorityQueue(std::vector<T> values)
        : values_(typename Queue::value_compare{}, std::move(values)) {}

    ErasablePriorityQueue(const ErasablePriorityQueue&) = default;
    ErasablePriorityQueue& operator=(const ErasablePriorityQueue&) = default;
    ErasablePriorityQueue(ErasablePriorityQueue&& other) noexcept {
        values_.swap(other.values_);
        erased_.swap(other.erased_);
    }
    ErasablePriorityQueue& operator=(ErasablePriorityQueue&& other) noexcept {
        if (this != &other) {
            ErasablePriorityQueue next(std::move(other));
            values_.swap(next.values_);
            erased_.swap(next.erased_);
        }
        return *this;
    }

    void push(const T& value) { values_.push(value); }
    void push(T&& value) { values_.push(std::move(value)); }
    template <class... Args>
    void emplace(Args&&... args) { values_.emplace(std::forward<Args>(args)...); }

    T top() {
        normalize();
#ifndef SKIP_RUNTIME_CHECKS
        if (values_.empty()) [[unlikely]]
            throw std::out_of_range("ErasablePriorityQueue::top: empty queue");
#endif
        return values_.top();
    }
    void pop() {
        normalize();
#ifndef SKIP_RUNTIME_CHECKS
        if (values_.empty()) [[unlikely]]
            throw std::out_of_range("ErasablePriorityQueue::pop: empty queue");
#endif
        values_.pop();
    }
    void erase(const T& value) {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty()) [[unlikely]]
            throw std::out_of_range("ErasablePriorityQueue::erase: empty queue");
#endif
        erased_.push(value);
    }
    bool empty() const noexcept { return values_.size() == erased_.size(); }
    std::size_t size() const noexcept { return values_.size() - erased_.size(); }

    std::optional<T> try_top() {
        if (empty()) return std::nullopt;
        normalize();
        return values_.top();
    }
    std::optional<T> extract_top() {
        if (empty()) return std::nullopt;
        normalize();
        std::optional<T> result(values_.top());
        values_.pop();
        return result;
    }
    void clear() noexcept {
        Queue{}.swap(values_);
        Queue{}.swap(erased_);
    }

private:
    Queue values_, erased_;

    void normalize() {
        while (!erased_.empty() && !values_.empty() && values_.top() == erased_.top()) {
            values_.pop();
            erased_.pop();
        }
    }
};
#endif
