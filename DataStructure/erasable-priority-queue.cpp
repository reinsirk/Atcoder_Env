// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/erasable-priority-queue.hpp
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/erasable_priority_queue.md
//
// API一覧
// [ErasablePriorityQueue<T,Minimize>]
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
// swap(other): 有効な要素と遅延削除をまとめて交換する。O(1)。
// clear(): 全要素と遅延削除を破棄する。O(H)。Hは未回収の削除済み要素も含む保存数。
// Tの比較は厳密弱順序を満たし、==はその同値関係と一致すること。

#include <cstddef>
#include <functional>
#include <optional>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_ERASABLE_PRIORITY_QUEUE_DEFINED
#define SIRKIT_ERASABLE_PRIORITY_QUEUE_DEFINED

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
#if defined(__GNUC__) && !defined(__clang__)
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            if (!erased_.empty()) normalize();
        } else normalize();
#else
        normalize();
#endif
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
    void swap(ErasablePriorityQueue& other) noexcept {
        values_.swap(other.values_);
        erased_.swap(other.erased_);
    }
    void clear() noexcept {
        Queue{}.swap(values_);
        Queue{}.swap(erased_);
    }

private:
    struct Heap : Queue {
        using Queue::Queue;
        auto& storage() noexcept { return this->c; }
#if defined(__GNUC__) && !defined(__clang__)
        void pop() {
            auto& values = this->c;
            if (values.size() == 1) {
                values.pop_back();
                return;
            }
            const T value = values.back();
            values.pop_back();
            auto* const data = values.data();
            const auto count = values.size();
            std::size_t hole = 0;
            const auto cmp = typename Queue::value_compare{};
            while (hole < count / 2) {
                auto child = hole * 2 + 1;
                if (child + 1 < count && cmp(data[child], data[child + 1])) ++child;
                data[hole] = data[child];
                hole = child;
            }
            while (hole != 0) {
                const auto parent = (hole - 1) / 2;
                if (!cmp(data[parent], value)) break;
                data[hole] = data[parent];
                hole = parent;
            }
            data[hole] = value;
        }
#endif
    };
    using Storage = std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
        Heap, Queue>;
    Storage values_, erased_;

    void pop_pair() {
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            auto& a = values_.storage();
            auto& b = erased_.storage();
            const T last_a = a.back(), last_b = b.back();
            a.pop_back();
            b.pop_back();
            T* const pa = a.data();
            T* const pb = b.data();
            const std::size_t na = a.size(), nb = b.size();
            std::size_t ha = 0, hb = 0;
            const auto cmp = typename Queue::value_compare{};
            const auto descend = [&cmp](T* data, std::size_t count, std::size_t& hole) {
                std::size_t child = hole * 2 + 1;
                if (child + 1 < count && cmp(data[child], data[child + 1])) ++child;
                data[hole] = data[child];
                hole = child;
            };
            // 二つの独立した二分ヒープを交互に降りる。
            while (ha < na / 2 && hb < nb / 2) {
                descend(pa, na, ha);
                descend(pb, nb, hb);
            }
            while (ha < na / 2) descend(pa, na, ha);
            while (hb < nb / 2) descend(pb, nb, hb);
            const auto restore = [&cmp](T* data, std::size_t hole, const T value) {
                while (hole != 0) {
                    const std::size_t parent = (hole - 1) / 2;
                    if (!cmp(data[parent], value)) break;
                    data[hole] = data[parent];
                    hole = parent;
                }
                data[hole] = value;
            };
            if (na != 0) restore(pa, ha, last_a);
            if (nb != 0) restore(pb, hb, last_b);
        } else {
            values_.pop();
            erased_.pop();
        }
    }

    void normalize() {
        while (!erased_.empty() && values_.top() == erased_.top()) {
            if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
                pop_pair();
            } else {
                values_.pop();
                erased_.pop();
            }
        }
    }
};
#endif
