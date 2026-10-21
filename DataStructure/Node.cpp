// SPDX-License-Identifier: CC0-1.0
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#ifndef SIRKIT_NODE_DEFINED
#define SIRKIT_NODE_DEFINED
// Node(next={}): 値を持たない単方向ノード。nはNode自体を所有するshared_ptrを接続する。
// コピーはnを共有し、ムーブはnを移す。循環の自動回収は行わない。
// 非共有の後続k個をO(k)時間・O(1)追加領域で解放。長い連結は反復、2ノード以下は通常破棄。
struct Node {
    std::shared_ptr<Node> n;

    explicit Node(std::shared_ptr<Node> next = {}) noexcept : n(std::move(next)) {}
    Node(const Node&) noexcept = default;
    Node(Node&&) noexcept = default;
    Node& operator=(const Node&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;

    ~Node() {
        if (!n || !n->n) return;
        if (n.use_count() != 1) return;
        auto current = std::move(n);
        do {
            auto next = std::move(current->n);
            current.swap(next); // nextが解放するノードのnは空なので、連結に沿って再帰しない。
        } while (current.use_count() == 1 && current);
    }
};
#endif

#ifndef SIRKIT_MAKE_NODE_DEFINED
#define SIRKIT_MAKE_NODE_DEFINED
// make_node(next={}): nextを後続に持つ新しいノードを返す。O(1)時間・1回確保。
inline std::shared_ptr<Node> make_node(std::shared_ptr<Node> next = {}) {
    return std::make_shared<Node>(std::move(next));
}
#endif

#ifndef SIRKIT_MAKE_CHAIN_DEFINED
#define SIRKIT_MAKE_CHAIN_DEFINED
// make_chain(count, tail={}): tailの前にcount個追加して先頭を返す。0個ならtail。O(count)時間・count回確保。
inline std::shared_ptr<Node> make_chain(std::size_t count, std::shared_ptr<Node> tail = {}) {
    while (count != 0) {
        tail = make_node(std::move(tail));
        --count;
    }
    return tail;
}
#endif

#ifndef SKIP_RUNTIME_CHECKS
#ifndef SIRKIT_INTERNAL_CHAIN_CYCLE_CHECK_DEFINED
#define SIRKIT_INTERNAL_CHAIN_CYCLE_CHECK_DEFINED
namespace reinsirk::detail {
// 通常走査で進んだ先を受け取り、循環を検出する。
struct ChainCycleCheck {
    const Node* checkpoint;
    std::size_t power = 1;
    std::size_t distance = 0;

    explicit ChainCycleCheck(const Node* head) noexcept : checkpoint(head) {}

    void advance(const Node* current) {
        if (current == nullptr) return;
        ++distance;
        if (current == checkpoint) throw std::invalid_argument("chain contains a cycle");
        if (distance == power) {
            checkpoint = current;
            distance = 0;
            constexpr auto max = std::numeric_limits<std::size_t>::max();
            power = power > max / 2 ? max : power * 2;
        }
    }
};
}  // namespace reinsirk::detail
#endif
#endif

#ifndef SIRKIT_CHAIN_SIZE_DEFINED
#define SIRKIT_CHAIN_SIZE_DEFINED
// chain_size(head): ノード数を返す。循環は既定でinvalid_argument、検査省略時は循環なしが前提。O(k)時間・O(1)領域。
inline std::size_t chain_size(const std::shared_ptr<Node>& head) {
    std::size_t size = 0;
    const Node* current = head.get();
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::ChainCycleCheck cycle_check(current);
#endif
    while (current != nullptr) {
        if (size == std::numeric_limits<std::size_t>::max()) {
            throw std::length_error("chain size is not representable");
        }
        ++size;
        current = current->n.get();
#ifndef SKIP_RUNTIME_CHECKS
        cycle_check.advance(current);
#endif
    }
    return size;
}
#endif

#ifndef SIRKIT_CHAIN_TAIL_DEFINED
#define SIRKIT_CHAIN_TAIL_DEFINED
// chain_tail(head): 末尾（空ならnullptr）を返す。循環は既定でinvalid_argument、検査省略時は循環なしが前提。O(k)時間・O(1)領域。
inline std::shared_ptr<Node> chain_tail(const std::shared_ptr<Node>& head) {
    const std::shared_ptr<Node>* link = &head;
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::ChainCycleCheck cycle_check(head.get());
#endif
    while (*link && (*link)->n) {
        link = &(*link)->n;
#ifndef SKIP_RUNTIME_CHECKS
        cycle_check.advance(link->get());
#endif
    }
    return *link;
}
#endif

#ifndef SIRKIT_CHAIN_NTH_DEFINED
#define SIRKIT_CHAIN_NTH_DEFINED
// chain_nth(head, index): 0始まりのindex番目（不足ならnullptr）を返す。循環も可。非循環O(min(index+1,k))、循環O(index+1)時間・O(1)領域。
inline std::shared_ptr<Node> chain_nth(const std::shared_ptr<Node>& head, std::size_t index) {
    const std::shared_ptr<Node>* link = &head;
    while (*link && index != 0) {
        link = &(*link)->n;
        --index;
    }
    return *link;
}
#endif

#ifndef SIRKIT_PUSH_FRONT_NODE_DEFINED
#define SIRKIT_PUSH_FRONT_NODE_DEFINED
// push_front_node(head): 新しい先頭を追加してheadを更新する。O(1)時間・1回確保。
inline void push_front_node(std::shared_ptr<Node>& head) {
    head = make_node(head);
}
#endif

#ifndef SIRKIT_POP_FRONT_NODE_DEFINED
#define SIRKIT_POP_FRONT_NODE_DEFINED
// pop_front_node(head): headを次へ移し、空ならfalseを返す。共有された元先頭のnは保持する。O(1)時間・領域。
inline bool pop_front_node(std::shared_ptr<Node>& head) noexcept {
    if (!head) return false;
    auto next = head->n;
    head = std::move(next);
    return true;
}
#endif

#ifndef SIRKIT_REVERSE_CHAIN_DEFINED
#define SIRKIT_REVERSE_CHAIN_DEFINED
// reverse_chain(head): 循環なしのnを逆転し新しい先頭を返す。共有者にも変更が見える。O(k)時間・O(1)領域。
inline std::shared_ptr<Node> reverse_chain(std::shared_ptr<Node> head) noexcept {
    std::shared_ptr<Node> previous;
    while (head) {
        auto next = std::move(head->n);
        head->n = std::move(previous);
        previous = std::move(head);
        head = std::move(next);
    }
    return previous;
}
#endif

#ifndef SIRKIT_CLONE_CHAIN_DEFINED
#define SIRKIT_CLONE_CHAIN_DEFINED
// clone_chain(head): 独立した同長の列を返す。循環は既定でinvalid_argument、検査省略時は循環なしが前提。O(k)時間・k回確保。
inline std::shared_ptr<Node> clone_chain(const std::shared_ptr<Node>& head) {
    std::shared_ptr<Node> result;
    auto* link = &result;
    const Node* current = head.get();
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::ChainCycleCheck cycle_check(current);
#endif
    while (current != nullptr) {
        *link = make_node();
        link = &(*link)->n;
        current = current->n.get();
#ifndef SKIP_RUNTIME_CHECKS
        cycle_check.advance(current);
#endif
    }
    return result;
}
#endif

#ifndef SIRKIT_CYCLE_ENTRY_DEFINED
#define SIRKIT_CYCLE_ENTRY_DEFINED
// cycle_entry(head): 循環入口の非所有ポインタ（循環なしならnullptr）を返す。O(k)時間・O(1)領域。
inline Node* cycle_entry(const std::shared_ptr<Node>& head) noexcept {
    Node* slow = head.get();
    Node* fast = head.get();
    do {
        if (fast == nullptr || !fast->n) return nullptr;
        slow = slow->n.get();
        fast = fast->n->n.get();
    } while (slow != fast);
    slow = head.get();
    while (slow != fast) {
        slow = slow->n.get();
        fast = fast->n.get();
    }
    return slow;
}
#endif

#ifndef SIRKIT_HAS_CYCLE_DEFINED
#define SIRKIT_HAS_CYCLE_DEFINED
// has_cycle(head): 循環の有無を返す。O(k)時間・O(1)領域。
inline bool has_cycle(const std::shared_ptr<Node>& head) noexcept {
    return cycle_entry(head) != nullptr;
}
#endif

#ifndef SIRKIT_BREAK_CYCLE_DEFINED
#define SIRKIT_BREAK_CYCLE_DEFINED
// break_cycle(head): 循環の末尾から入口への辺を切り、切ったらtrueを返す。共有者にも変更が見える。O(k)時間・O(1)領域。
inline bool break_cycle(const std::shared_ptr<Node>& head) noexcept {
    auto keep_alive = head;
    Node* entry = cycle_entry(keep_alive);
    if (entry == nullptr) return false;
    Node* last = entry;
    while (last->n.get() != entry) last = last->n.get();
    last->n.reset();
    return true;
}
#endif

#ifndef SIRKIT_CLEAR_CHAIN_DEFINED
#define SIRKIT_CLEAR_CHAIN_DEFINED
// clear_chain(head): headを空にし、共有されていない後続k個を反復解放する。循環回収には先にbreak_cycle。O(k)時間・O(1)領域。
inline void clear_chain(std::shared_ptr<Node>& head) noexcept {
    head.reset();
}
#endif
