#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/point_add_rectangle_sum
// Submission: https://judge.yosupo.jp/submission/412247
// Submission: https://judge.yosupo.jp/submission/412279

// Library: DataStructure/dynamic-binary-indexed-tree.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/data-structure/dynamic-binary-indexed-tree.hpp
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_INTERNAL_HASH_MAP_DEFINED
#define REINSIRK_INTERNAL_HASH_MAP_DEFINED
namespace reinsirk::detail {
// dynamic系で共有する内部用の整数ハッシュ表。参照・ポインタは再確保で無効。
// Packedは占有フラグのbit化、Denominatorは空き容量の比率1/Denominatorを指定する。
template <class Key, class Val, bool Packed = false, unsigned Denominator = 5>
class InternalHashMap {
    // InternalHashMap(): 空の表を構築する。初期配列は確保しない。
    // operator[](key): 未登録なら設定済みの初期値で追加し、値の参照を返す。
    // find(key): 値へのポインタを返す。未登録はnullptrで、追加はしない。
    // find_nonempty(key): 容量確保済みの表でfindと同じ結果を返す。内部ループ用。
    // contains(key)/contain(key): キーが登録済みならtrueを返す。
    // count(key): 登録済みなら1、未登録なら0を返す。
    // for_each(f): 各登録キーと値のconst参照にfを適用する。fは表を変更しないこと。
    // size()/empty(): 登録件数・空かどうかを返す。
    // reserve(n): n件まで再確保せず追加できる容量を準備する。
    // reallocate(): 容量を2倍にする。空なら初期容量を確保する。
    // clear(): 登録を全て削除する。確保済みの領域は再利用する。
    // set_default(value): 今後追加する要素の初期値を設定する。
    // enumerate(): 登録済みの{キー, 値}を順序不定で列挙する。
    // swap(other): 表の内容・初期値を交換する。
    static_assert(std::is_integral_v<Key> &&
                  std::is_same_v<Key, std::remove_cv_t<Key>> &&
                  !std::is_same_v<Key, bool> &&
                  sizeof(Key) <= sizeof(std::uint64_t),
                  "InternalHashMap: Key must be an integer of at most 64 bits");
    static_assert(Denominator >= 2 && Denominator <= 8);
    struct Value { Val value{}; };
    std::vector<Key> keys_;
    std::vector<Value> values_;
    using Flag = std::conditional_t<Packed, bool, std::uint8_t>;
    std::vector<Flag> used_;
    std::size_t size_ = 0;
    Val default_value_{};
    std::uint64_t salt_ = 0;
    unsigned shift_ = 61;

    static std::uint64_t mix(std::uint64_t x) noexcept {
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        return x ^ (x >> 31);
    }

    static std::uint64_t initial_salt() noexcept {
        static const std::uint64_t value = mix(static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count())) | std::uint64_t{1};
        return value;
    }

    std::size_t hash(Key key) const noexcept {
        return static_cast<std::size_t>((static_cast<std::uint64_t>(key) * salt_) >> shift_);
    }

    std::size_t max_capacity() const noexcept {
        return std::bit_floor(std::min({keys_.max_size(), values_.max_size(), used_.max_size()}));
    }

    void rehash(std::size_t capacity) {
        if (salt_ == 0) salt_ = initial_salt();
        if (capacity > max_capacity())
            throw std::length_error("InternalHashMap: capacity overflow");
        const unsigned next_shift = 64 - std::countr_zero(capacity);
        std::vector<Key> keys(capacity);
        std::vector<Value> values(capacity);
        std::vector<Flag> used(capacity, 0);
        for (std::size_t i = 0; i < used_.size(); ++i) {
            if (!used_[i]) continue;
            std::size_t p = static_cast<std::size_t>((static_cast<std::uint64_t>(keys_[i]) * salt_) >> next_shift);
            while (used[p]) p = (p + 1) & (capacity - 1);
            keys[p] = keys_[i];
            values[p].value = values_[i].value;
            used[p] = 1;
        }
        keys_.swap(keys);
        values_.swap(values);
        used_.swap(used);
        shift_ = next_shift;
    }

public:
    // 空の状態では配列を確保しない。
    InternalHashMap() noexcept(std::is_nothrow_default_constructible_v<Val>) = default;
    InternalHashMap(const InternalHashMap&) = default;
    InternalHashMap(InternalHashMap&& other) noexcept(std::is_nothrow_default_constructible_v<Val> &&
                                    std::is_nothrow_swappable_v<Val>) : InternalHashMap() { swap(other); }
    InternalHashMap& operator=(InternalHashMap other) {
        swap(other);
        return *this;
    }

    void swap(InternalHashMap& other) noexcept(std::is_nothrow_swappable_v<Val>) {
        using std::swap;
        swap(default_value_, other.default_value_);
        keys_.swap(other.keys_);
        values_.swap(other.values_);
        used_.swap(other.used_);
        swap(size_, other.size_);
        swap(salt_, other.salt_);
        swap(shift_, other.shift_);
    }

    // n要素まで再確保せずに格納できる容量を確保する。
    void reserve(std::size_t n) {
        if (n <= keys_.size() - keys_.size() / Denominator) return;
        std::size_t capacity = std::max(std::size_t{8}, keys_.size());
        while (n > capacity - capacity / Denominator) {
            if (capacity > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            capacity *= 2;
        }
        rehash(capacity);
    }

    // 容量を2倍にする。空なら初期容量を確保する。
    void reallocate() {
        if (keys_.empty()) {
            rehash(8);
        } else {
            if (keys_.size() > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            rehash(keys_.size() * 2);
        }
    }

    // 未登録なら設定済みの初期値で追加する。
    Val& operator[](Key key) {
        if (keys_.empty()) reallocate();
        std::size_t p = hash(key);
        while (used_[p]) {
            if (keys_[p] == key) return values_[p].value;
            p = (p + 1) & (keys_.size() - 1);
        }
        if (size_ == keys_.size() - keys_.size() / Denominator) {
            reallocate();
            p = hash(key);
            while (used_[p]) p = (p + 1) & (keys_.size() - 1);
        }
        keys_[p] = key;
        values_[p].value = default_value_;
        used_[p] = 1;
        ++size_;
        return values_[p].value;
    }

    // 未登録ならnullptrを返す。読取では要素を追加しない。
    const Val* find(const Key& key) const noexcept {
        if (keys_.empty()) return nullptr;
        return find_nonempty(key);
    }
    const Val* find_nonempty(const Key& key) const noexcept {
        std::size_t p = hash(key);
        while (used_[p]) {
            if (keys_[p] == key) return &values_[p].value;
            p = (p + 1) & (keys_.size() - 1);
        }
        return nullptr;
    }
    Val* find(const Key& key) noexcept {
        return const_cast<Val*>(static_cast<const InternalHashMap&>(*this).find(key));
    }

    // 登録済みのキーと値を列挙する。順序は不定。
    std::vector<std::pair<Key, Val>> enumerate() const {
        std::vector<std::pair<Key, Val>> result;
        result.reserve(size_);
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) result.emplace_back(keys_[i], values_[i].value);
        return result;
    }

    template <class F> void for_each(F&& f) const {
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) f(keys_[i], values_[i].value);
    }

    [[nodiscard]] bool contains(Key key) const noexcept { return find(key) != nullptr; }
    [[nodiscard]] bool contain(Key key) const noexcept { return contains(key); }
    [[nodiscard]] int count(Key key) const noexcept { return contains(key) ? 1 : 0; }
    void clear() noexcept {
        std::fill(used_.begin(), used_.end(), Flag{0});
        size_ = 0;
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    // 今後追加する要素の初期値を設定する。
    void set_default(const Val& value) { default_value_ = value; }
};

}  // namespace reinsirk::detail
#endif  // REINSIRK_INTERNAL_HASH_MAP_DEFINED




#include <array>
#include <memory>
#include <variant>
#ifndef REINSIRK_DYNAMIC_FENWICK_BPLUS_ROW_DEFINED
#define REINSIRK_DYNAMIC_FENWICK_BPLUS_ROW_DEFINED
namespace reinsirk::detail {
// 元列の値を葉32点・内部16子で保持する。累積値はT幅の剰余で扱う。
template <class T, bool DirectReads = false>
class BPlusFenwickRow {
  using U = std::uint32_t;
  using V = std::make_unsigned_t<T>;
  using A = decltype(V{} + 0u);
  static constexpr U absent = std::numeric_limits<U>::max();
  struct Node {
    std::array<U,33> keys = [] {std::array<U,33> keys{};keys.fill(absent);return keys;}();
    std::array<V,33> prefix{};
    std::array<U,17> children{};
    U previous = absent, next = absent;
    std::uint8_t count = 0;
    bool leaf = true;
  };
  struct Frame { U node; unsigned position; };
  std::vector<Node> nodes_;
  U root_ = absent, cells_ = 0;
  std::vector<V> dense_values_;
  std::vector<std::uint64_t> dense_flags_;
  U dense_width_=0;
  bool dense_failed_=false;
  struct NoDirectReadState {};
  [[no_unique_address]] std::conditional_t<DirectReads,bool,NoDirectReadState> dense_clean_ = [] {
    if constexpr(DirectReads)return true;
    else return NoDirectReadState{};
  }();
  bool direct_dense_read() const noexcept {
    if constexpr(DirectReads)return dense_clean_;
    else return false;
  }
  static V plus(V a, V b) noexcept { return static_cast<V>(static_cast<A>(a)+static_cast<A>(b)); }
  static V minus(V a, V b) noexcept { return static_cast<V>(static_cast<A>(a)-static_cast<A>(b)); }
  static constexpr U lowbit(U x) noexcept { return x&(U{0}-x); }
  bool dense_used(U c) const noexcept {
    return (dense_flags_[(c-1)/64]&(std::uint64_t{1}<<((c-1)%64)))!=0;
  }
  void dense_add(U col,V delta) noexcept {
    if(cells_==dense_width_) {
      for(U c=col;c<=dense_width_;c+=lowbit(c))dense_values_[c]=plus(dense_values_[c],delta);
      return;
    }
    for(U c=col;c<=dense_width_;c+=lowbit(c)) {
      std::uint64_t& flags=dense_flags_[(c-1)/64];
      const std::uint64_t bit=std::uint64_t{1}<<((c-1)%64);
      if((flags&bit)==0) { flags|=bit;dense_values_[c]=delta;++cells_; }
      else dense_values_[c]=plus(dense_values_[c],delta);
    }
  }
  T dense_sum(U ml,U mr) const noexcept {
    if(ml==mr)return T{};
    const U common=(ml&mr)&(U{0}-std::bit_floor(ml^mr));
    V result{};
    if(cells_==dense_width_ || direct_dense_read()) {
      for(U c=mr;c!=common;c-=lowbit(c))result=plus(result,dense_values_[c]);
      for(U c=ml;c!=common;c-=lowbit(c))result=minus(result,dense_values_[c]);
      return static_cast<T>(result);
    }
    for(U c=mr;c!=common;c-=lowbit(c)) {
      const V mask=static_cast<V>(A{0}-static_cast<A>(dense_used(c)));
      result=plus(result,static_cast<V>(dense_values_[c]&mask));
    }
    for(U c=ml;c!=common;c-=lowbit(c)) {
      const V mask=static_cast<V>(A{0}-static_cast<A>(dense_used(c)));
      result=minus(result,static_cast<V>(dense_values_[c]&mask));
    }
    return static_cast<T>(result);
  }
  template<bool Force=false>
  void maybe_dense(U m) noexcept {
    if(m>1'000'000 || dense_failed_ || (!Force && cells_<(m+4)/5) || !dense_values_.empty())return;
    try {
      const U capacity=DirectReads ? std::bit_ceil(m):m;
      std::vector<V> values(std::size_t{capacity}+1,V{});
      if constexpr(DirectReads)std::fill(values.begin()+std::size_t{m}+1,values.end(),static_cast<V>(std::numeric_limits<T>::max()));
      std::vector<std::uint64_t> flags((std::size_t{m}+63)/64,0);
      for(const Node& node:nodes_)if(node.leaf)for(unsigned i=0;i<node.count;++i) {
        const U c=node.keys[i];
        values[c]=minus(node.prefix[i],i ? node.prefix[i-1] : V{});
        flags[(c-1)/64]|=std::uint64_t{1}<<((c-1)%64);
      }
      for(U c=1;c<=m;++c) {
        const U parent=c+lowbit(c);
        if(parent<=m)values[parent]=plus(values[parent],values[c]);
      }
      // 非64倍数のセルはword内、64倍数だけをblock間で伝播する。
      for(std::uint64_t& word:flags) {
        word|=(word<<1)&UINT64_C(0xaaaaaaaaaaaaaaaa);
        word|=(word<<2)&UINT64_C(0x8888888888888888);
        word|=(word<<4)&UINT64_C(0x8080808080808080);
        word|=(word<<8)&UINT64_C(0x8000800080008000);
        word|=(word<<16)&UINT64_C(0x8000000080000000);
        word|=(word<<32)&UINT64_C(0x8000000000000000);
      }
      const U blocks=m/64;
      for(U block=1;block<=blocks;++block) {
        const U parent=block+lowbit(block);
        if(parent<=blocks)flags[parent-1]|=flags[block-1]&UINT64_C(0x8000000000000000);
      }
      if(m%64)flags.back()&=(std::uint64_t{1}<<(m%64))-1;
      dense_values_.swap(values);dense_flags_.swap(flags);dense_width_=m;
      if constexpr(DirectReads)dense_clean_=true;
      root_=absent;std::vector<Node> empty;nodes_.swap(empty);
    } catch(const std::bad_alloc&) { dense_failed_=true; }
      catch(const std::length_error&) { dense_failed_=true; }
  }
  static unsigned position(const Node& n, U key) noexcept {
    unsigned p=0;
    if(n.leaf)for(unsigned i=0;i<32;++i)p+=static_cast<unsigned>(n.keys[i]<key);
    else for(unsigned i=0;i<16;++i)p+=static_cast<unsigned>(n.keys[i]<key);
    return p;
  }
  static V total(const Node& n) noexcept { return n.count ? n.prefix[n.count-1] : V{}; }
  static U last(const Node& n) noexcept { return n.keys[n.count-1]; }
  void ensure_nodes(std::size_t extra) {
    const std::size_t maximum = std::min(nodes_.max_size(),std::size_t{absent});
    if (extra > maximum-nodes_.size()) throw std::length_error("BPlusFenwickRow: node capacity overflow");
    const std::size_t needed=nodes_.size()+extra;
    if (needed <= nodes_.capacity()) return;
    std::size_t capacity=std::max(std::size_t{4},nodes_.capacity());
    while (capacity < needed) {
      if (capacity > maximum/2) { capacity=maximum; break; }
      capacity*=2;
    }
    if (capacity > maximum) capacity=maximum;
    if (capacity < needed) throw std::length_error("BPlusFenwickRow: node capacity overflow");
    nodes_.reserve(capacity);
  }
  U create(bool leaf) {
    const U id=static_cast<U>(nodes_.size());
    nodes_.emplace_back(); nodes_.back().leaf=leaf; return id;
  }
  // 先に容量を確保し、この後は整数・固定長配列の無例外操作だけで変更する。
  U split(U id) {
    const bool leaf=nodes_[id].leaf;
    const U right_id=create(leaf);
    Node& left=nodes_[id]; Node& right=nodes_[right_id];
    const unsigned lower=left.count/2, upper=left.count-lower;
    const V base=left.prefix[lower-1];
    for (unsigned i=0;i<upper;++i) {
      right.keys[i]=left.keys[lower+i];
      right.prefix[i]=minus(left.prefix[lower+i],base);
      if (!leaf) right.children[i]=left.children[lower+i];
    }
    right.count=static_cast<std::uint8_t>(upper); left.count=static_cast<std::uint8_t>(lower);
    for(unsigned i=lower;i<left.keys.size();++i)left.keys[i]=absent;
    if (leaf) {
      right.previous=id; right.next=left.next;
      if (left.next!=absent) nodes_[left.next].previous=right_id;
      left.next=right_id;
    }
    return right_id;
  }
  static void insert(Node& node, unsigned p, U key, V value, U child=absent) noexcept {
    const unsigned count=node.count;
    for (unsigned i=count;i>p;--i) {
      node.keys[i]=node.keys[i-1]; node.prefix[i]=plus(node.prefix[i-1],value);
      if (!node.leaf) node.children[i]=node.children[i-1];
    }
    node.keys[p]=key; node.prefix[p]=plus(p ? node.prefix[p-1] : V{},value);
    if (!node.leaf) node.children[p]=child;
    ++node.count;
  }
  std::size_t extra_cells(U col, U m, U previous, U next) const noexcept {
    const U a=col-1;
    U nearest=std::numeric_limits<U>::max();
    if (previous!=0) nearest=std::min(nearest,a^(previous-1));
    if (next!=0) nearest=std::min(nearest,a^(next-1));
    const unsigned width=std::bit_width(nearest);
    const std::uint64_t common=(std::uint64_t{a}|((std::uint64_t{1}<<width)-1))+1;
    if (common<=m) return std::popcount(static_cast<U>(common)-col);
    const U root=m&(std::numeric_limits<U>::max()<<(std::bit_width(a^m)-1));
    return std::size_t{1}+std::popcount(root-col);
  }
  V point(U key) const noexcept {
    if(root_==absent)return V{};
    U id=root_;
    for(;;) {
      const Node& node=nodes_[id];
      const unsigned p=position(node,key);
      if(p==node.count)return V{};
      if(node.leaf)return node.keys[p]==key ? minus(node.prefix[p],p ? node.prefix[p-1] : V{}) : V{};
      id=node.children[p];
    }
  }
  V prefix_from(U id,U endpoint) const noexcept {
    if (id==absent) return V{};
    V result{};
    for (;;) {
      const Node& node=nodes_[id];
      unsigned p=0;
      if(node.leaf)for(unsigned i=0;i<32;++i)p+=static_cast<unsigned>(node.keys[i]<=endpoint);
      else for(unsigned i=0;i<16;++i)p+=static_cast<unsigned>(node.keys[i]<=endpoint);
      if (p) result=plus(result,node.prefix[p-1]);
      if (node.leaf || p==node.count) return result;
      id=node.children[p];
    }
  }
  V interval(U ml,U mr) const noexcept {
    if(root_==absent || ml==mr)return V{};
    U id=root_;
    for(;;) {
      const Node& node=nodes_[id];
      unsigned left=0,right=0;
      if(node.leaf)for(unsigned i=0;i<32;++i) {
        const U key=node.keys[i];left+=static_cast<unsigned>(key<=ml);right+=static_cast<unsigned>(key<=mr);
      } else for(unsigned i=0;i<16;++i) {
        const U key=node.keys[i];left+=static_cast<unsigned>(key<=ml);right+=static_cast<unsigned>(key<=mr);
      }
      if(node.leaf)return minus(right ? node.prefix[right-1] : V{},left ? node.prefix[left-1] : V{});
      if(left==right) {
        if(left==node.count)return V{};
        id=node.children[left];continue;
      }
      V result=minus(node.prefix[right-1],left ? node.prefix[left-1] : V{});
      result=minus(result,prefix_from(node.children[left],ml));
      if(right<node.count)result=plus(result,prefix_from(node.children[right],mr));
      return result;
    }
  }
 public:
  void promote_prefix(U m) noexcept {maybe_dense<true>(m);}
  BPlusFenwickRow() noexcept = default;
  BPlusFenwickRow(const BPlusFenwickRow&)=default;
  BPlusFenwickRow(BPlusFenwickRow&& other) noexcept { swap(other); }
  BPlusFenwickRow& operator=(BPlusFenwickRow other) { swap(other); return *this; }
  void swap(BPlusFenwickRow& other) noexcept {
    nodes_.swap(other.nodes_); std::swap(root_,other.root_);std::swap(cells_,other.cells_);
    dense_values_.swap(other.dense_values_);dense_flags_.swap(other.dense_flags_);
    std::swap(dense_width_,other.dense_width_);std::swap(dense_failed_,other.dense_failed_);
    if constexpr(DirectReads)std::swap(dense_clean_,other.dense_clean_);
  }
  std::size_t size() const noexcept { return cells_; }
  bool empty() const noexcept { return cells_==0; }
  void clear() noexcept {
    nodes_.clear();root_=absent;cells_=0;dense_failed_=false;
    if constexpr(DirectReads)dense_clean_=dense_values_.empty();
    std::fill(dense_flags_.begin(),dense_flags_.end(),std::uint64_t{0});
  }
  void clear(U m) noexcept {
    if(!dense_values_.empty() && dense_width_!=m) {
      dense_values_.clear();dense_flags_.clear();dense_width_=0;
    }
    clear();
  }
  void reserve_points(std::size_t points) {
    if (points > std::numeric_limits<std::size_t>::max()-15)
      throw std::length_error("BPlusFenwickRow: point capacity overflow");
    ensure_nodes((points+15)/16+1);
  }
  void add(U col, const T& value, U m) {
    const V delta=static_cast<V>(value);
    if(!dense_values_.empty()) { dense_add(col,delta);return; }
    if (root_==absent) {
      const std::size_t extra=extra_cells(col,m,0,0);
      ensure_nodes(1);root_=create(true);insert(nodes_[root_],0,col,delta);cells_=static_cast<U>(extra);maybe_dense(m);return;
    }
    std::array<Frame,32> path;
    unsigned depth=0;U id=root_;
    while (!nodes_[id].leaf) {
      const Node& node=nodes_[id];
      unsigned p=position(node,col);if(p==node.count)--p;
      if(depth==path.size())throw std::length_error("BPlusFenwickRow: tree depth overflow");
      path[depth++]={id,p};id=node.children[p];
    }
    const Node& found=nodes_[id];
    const unsigned p=position(found,col);
    const bool existing=p<found.count&&found.keys[p]==col;
    if(existing) {
      Node& leaf=nodes_[id];
      for(unsigned i=0;i<32;++i)leaf.prefix[i]=plus(leaf.prefix[i],i>=p ? delta : V{});
      for(unsigned d=0;d<depth;++d) {
        const Frame frame=path[d];Node& parent=nodes_[frame.node];
        for(unsigned i=0;i<16;++i)parent.prefix[i]=plus(parent.prefix[i],i>=frame.position ? delta : V{});
      }
      return;
    }
    std::size_t extra=0;
    if (!existing) {
      const U previous=p ? found.keys[p-1] : (found.previous!=absent ? last(nodes_[found.previous]) : U{0});
      const U next=p<found.count ? found.keys[p] : (found.next!=absent ? nodes_[found.next].keys[0] : U{0});
      extra=extra_cells(col,m,previous,next);
      if(found.count<32) {
        Node& leaf=nodes_[id];insert(leaf,p,col,delta);
        for(unsigned d=0;d<depth;++d) {
          const Frame frame=path[d];Node& parent=nodes_[frame.node];
          parent.keys[frame.position]=std::max(parent.keys[frame.position],col);
          for(unsigned i=0;i<16;++i)parent.prefix[i]=plus(parent.prefix[i],i>=frame.position ? delta : V{});
        }
        cells_+=static_cast<U>(extra);maybe_dense(m);return;
      }
      std::size_t allocation=0;
      if(found.count==32) {
        allocation=1;unsigned d=depth;
        while(d && nodes_[path[d-1].node].count==16){++allocation;--d;}
        if(!d)++allocation;
      }
      ensure_nodes(allocation);
    }
    Node& leaf=nodes_[id];
    insert(leaf,p,col,delta);
    U right=leaf.count>32 ? split(id) : absent;
    for(unsigned d=depth;d>0;--d) {
      const Frame frame=path[d-1];Node& parent=nodes_[frame.node];
      const V old=minus(parent.prefix[frame.position],frame.position ? parent.prefix[frame.position-1] : V{});
      const V change=minus(total(nodes_[id]),old);
      for(unsigned i=frame.position;i<parent.count;++i)parent.prefix[i]=plus(parent.prefix[i],change);
      parent.keys[frame.position]=last(nodes_[id]);
      if(right!=absent)insert(parent,frame.position+1,last(nodes_[right]),total(nodes_[right]),right);
      id=frame.node;right=parent.count>16 ? split(id) : absent;
    }
    if(right!=absent) {
      const U next=create(false);Node& node=nodes_[next];
      insert(node,0,last(nodes_[id]),total(nodes_[id]),id);
      insert(node,1,last(nodes_[right]),total(nodes_[right]),right);root_=next;
    }
    cells_+=static_cast<U>(extra);maybe_dense(m);
  }

  // bound<Upper>(w,m): 非負点列で累積和>=w／>wとなる最初の0始まり位置。なければm。
  template<bool Upper> U bound(T w,U m) const noexcept {
    if constexpr(Upper) { if(w<T{})return 0; }
    else if(w<=T{})return 0;
    if(cells_==0)return m;
    V remaining=static_cast<V>(w);
    if(!dense_values_.empty()) {
      U x=0;
      const bool full=cells_==dense_width_ || direct_dense_read();
      for(U step=std::bit_floor(m);step;step>>=1) {
        const U next=x+step;
        if(next>m)continue;
        const V cell=full || dense_used(next) ? dense_values_[next] : V{};
        if(Upper ? cell<=remaining : cell<remaining) { remaining=minus(remaining,cell);x=next; }
      }
      return x;
    }
    if(root_==absent)return m;
    U id=root_;
    for(;;) {
      const Node& node=nodes_[id];
      unsigned p=0;
      for(unsigned step=std::bit_floor(static_cast<unsigned>(node.count));step;step>>=1) {
        const unsigned next=p+step;
        if(next<=node.count && (Upper ? node.prefix[next-1]<=remaining : node.prefix[next-1]<remaining))p=next;
      }
      if(p==node.count)return m;
      if(node.leaf)return node.keys[p]-1;
      if(p)remaining=minus(remaining,node.prefix[p-1]);
      id=node.children[p];
    }
  }
  // reserve(nodes,m): Fenwickセル数を点数の上限として木の容量を準備するヒント。
  void reserve(std::size_t nodes,U m) {
    if(!nodes || !dense_values_.empty())return;
    reserve_points(std::min(nodes,static_cast<std::size_t>(m)));
  }

  // direct_values(): 新規denseまたは全セル占有時だけ、1始まりの値配列を非所有で返す。
  const V* direct_values() const noexcept {
    if constexpr(DirectReads) {
      if(!dense_values_.empty() && (direct_dense_read() || cells_==dense_width_))return dense_values_.data();
    }
    return nullptr;
  }
  // get(col): 1始まりの元列値。prefix(endpoint): 元列[1,endpoint]の和。
  T get(U col) const noexcept {
    if(dense_values_.empty())return static_cast<T>(point(col));
    const U common=col-lowbit(col);
    if(cells_==dense_width_ || direct_dense_read()) {
      V result=dense_values_[col];
      for(U c=col-1;c!=common;c-=lowbit(c))result=minus(result,dense_values_[c]);
      return static_cast<T>(result);
    }
    V result=dense_used(col) ? dense_values_[col] : V{};
    for(U c=col-1;c!=common;c-=lowbit(c)) {
      const V mask=static_cast<V>(A{0}-static_cast<A>(dense_used(c)));
      result=minus(result,static_cast<V>(dense_values_[c]&mask));
    }
    return static_cast<T>(result);
  }
  T prefix(U endpoint) const noexcept {
    if(dense_values_.empty())return static_cast<T>(prefix_from(root_,endpoint));
    V result{};
    if(cells_==dense_width_ || direct_dense_read()) {
      for(U c=endpoint;c;c-=lowbit(c))result=plus(result,dense_values_[c]);
    } else for(U c=endpoint;c;c-=lowbit(c)) {
      const V mask=static_cast<V>(A{0}-static_cast<A>(dense_used(c)));
      result=plus(result,static_cast<V>(dense_values_[c]&mask));
    }
    return static_cast<T>(result);
  }
  T sum(U ml,U mr) const noexcept {
    if(!dense_values_.empty())return dense_sum(ml,mr);
    return static_cast<T>(mr-ml==1 ? point(mr) : interval(ml,mr));
  }
};
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_DYNAMIC_FENWICK_SMALL_ROW_DEFINED
#define REINSIRK_DYNAMIC_FENWICK_SMALL_ROW_DEFINED
namespace reinsirk::detail {
// 少数点を直接保持し、列幅に応じてFenwickセル表または集約木へ移す。
template <class T, bool DirectReads = false>
class SmallFenwickRow {
  using U = std::uint32_t;
  // 集計順を変えても旧BITと一致するよう、少点の値はT幅の剰余で保持する。
  using Unsigned = std::make_unsigned_t<T>;
  using Arithmetic = decltype(Unsigned{} + 0u);
  using Hash = InternalHashMap<U,T,false,4>;
  using Tree = BPlusFenwickRow<T, DirectReads>;
  static constexpr U tree_minimum_width = 8192;
  static_assert(std::is_nothrow_move_constructible_v<Hash> &&
                noexcept(std::declval<Hash&>().swap(std::declval<Hash&>())));
  struct Point { U col = 0; Unsigned value{}; };
  static constexpr std::size_t point_limit = 32;
  static constexpr std::size_t inline_limit = 4;
  static_assert(point_limit >= inline_limit && point_limit <= std::numeric_limits<U>::max() / 32);
  struct IndexedPoint { U col = 0, position = 0; Unsigned value{}; };
  // 少点の状態と移行後の表は同時には必要ない。
  struct SmallStorage {
    U point_count_ = 0, small_nodes_ = 0;
    std::vector<IndexedPoint> point_index_;
    U index_salt_ = 0;
    unsigned index_shift_ = 29;
    std::array<U, inline_limit> inline_columns_{};
    std::array<Unsigned, inline_limit> inline_values_{};
    std::vector<Point> overflow_;
  };
  struct FlatRow {
    struct Payload {
      std::array<U,128> cols{};
      std::array<Unsigned,128> values{};
      std::array<std::uint8_t,256> slots{};
    };
    std::unique_ptr<Payload> payload;
    U count=0, cells=0, salt=0;
    FlatRow() : payload(std::make_unique<Payload>()), salt(initial_index_salt()) {}
    FlatRow(const FlatRow& other)
      : payload(other.payload ? std::make_unique<Payload>(*other.payload) : nullptr),
        count(other.count),cells(other.cells),salt(other.salt) {}
    FlatRow(FlatRow&& other) noexcept
      : payload(std::move(other.payload)),count(std::exchange(other.count,0)),
        cells(std::exchange(other.cells,0)),salt(std::exchange(other.salt,0)) {}
    FlatRow& operator=(FlatRow other) { swap(other); return *this; }
    void swap(FlatRow& other) noexcept {
      payload.swap(other.payload);
      using std::swap;swap(count,other.count);swap(cells,other.cells);swap(salt,other.salt);
    }
    std::size_t position(U col) const noexcept {
      std::size_t p=static_cast<U>(col*salt)>>24;
      while(payload->slots[p] && payload->cols[payload->slots[p]-1]!=col)p=(p+1)&255;
      return p;
    }
    std::size_t extra_nodes(U col,U m) const noexcept {
      const U a=col-1;U nearest=std::numeric_limits<U>::max();
      for(U i=0;i<128;++i) nearest=std::min(nearest,a^(payload->cols[i]-1));
      const unsigned width=std::bit_width(nearest);
      const std::uint64_t common=(std::uint64_t{a}|((std::uint64_t{1}<<width)-1))+1;
      if(common<=m)return std::popcount(static_cast<U>(common)-col);
      const U root=m&(std::numeric_limits<U>::max()<<(std::bit_width(a^m)-1));
      return std::size_t{1}+std::popcount(root-col);
    }
    // 既知の異なる列を移す。セル数は移行元から別に引き継ぐ。
    void append_known(U col,Unsigned value) noexcept {
      const auto p=position(col);
      payload->cols[count]=col;payload->values[count]=value;
      payload->slots[p]=static_cast<std::uint8_t>(++count);
    }
    bool add(U col,const T& value,U m) noexcept {
      const auto p=position(col);const U code=payload->slots[p];
      if(code) {
        Unsigned& v=payload->values[code-1];
        v=static_cast<Unsigned>(static_cast<Arithmetic>(v)+static_cast<Arithmetic>(static_cast<Unsigned>(value)));
        return true;
      }
      if(count==128)return false;
      cells+=static_cast<U>(extra_nodes(col,m));
      payload->cols[count]=col;payload->values[count]=static_cast<Unsigned>(value);
      payload->slots[p]=static_cast<std::uint8_t>(++count);
      return true;
    }
    T sum(U ml,U mr) const noexcept {
      if(!count)return T{};
      if(mr-ml==1) {
        const U code=payload->slots[position(mr)];
        return code ? static_cast<T>(payload->values[code-1]) : T{};
      }
      Arithmetic result{};
      for(U i=0;i<128;++i) {
        const bool inside=payload->cols[i]-ml-U{1}<mr-ml;
        result+=static_cast<Arithmetic>(payload->values[i])*static_cast<Arithmetic>(inside);
      }
      return static_cast<T>(result);
    }
    void clear() noexcept {
      if(payload) { payload->cols.fill(0);payload->slots.fill(0); }
      count=0;cells=0;
    }
  };
  using Storage = std::variant<SmallStorage, Hash, Tree, FlatRow>;
  static_assert(std::is_nothrow_move_constructible_v<SmallStorage> &&
                std::is_nothrow_move_constructible_v<Storage>);
  static_assert(std::is_nothrow_move_constructible_v<FlatRow>);
  static_assert(std::is_nothrow_move_constructible_v<Tree> &&
                noexcept(std::declval<Tree&>().swap(std::declval<Tree&>())));
  Storage storage_{std::in_place_index<0>};
  static void move_state(Storage& source, Storage& destination) noexcept {
    switch (source.index()) {
      case 0: destination.template emplace<0>(std::move(*std::get_if<0>(&source))); break;
      case 1: destination.template emplace<1>(std::move(*std::get_if<1>(&source))); break;
      case 2: destination.template emplace<2>(std::move(*std::get_if<2>(&source))); break;
      default: destination.template emplace<3>(std::move(*std::get_if<3>(&source))); break;
    }
  }
  SmallStorage& small() noexcept { return *std::get_if<0>(&storage_); }
  const SmallStorage& small() const noexcept { return *std::get_if<0>(&storage_); }


  static U initial_index_salt() noexcept {
    static const U salt = [] {
      std::uint64_t x = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
      x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
      x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
      return static_cast<U>(x ^ (x >> 31)) | U{1};
    }();
    return salt;
  }
  std::size_t index_position(U col) const noexcept {
    std::size_t p = static_cast<U>(col * small().index_salt_) >> small().index_shift_;
    while (small().point_index_[p].col != 0 && small().point_index_[p].col != col)
      p = (p + 1) & (small().point_index_.size() - 1);
    return p;
  }
  void prepare_index() {
    std::size_t capacity = std::max(std::size_t{8}, small().point_index_.size());
    while (std::size_t{small().point_count_} + 1 > capacity - capacity / 4) {
      if (capacity > std::bit_floor(small().point_index_.max_size()) / 2)
        throw std::length_error("SmallFenwickRow: point index overflow");
      capacity *= 2;
    }
    if (capacity != small().point_index_.size()) {
      const U salt = small().index_salt_ ? small().index_salt_ : initial_index_salt();
      const unsigned shift = 32 - std::countr_zero(capacity);
      std::vector<IndexedPoint> next(capacity);
      for (const IndexedPoint& entry : small().point_index_) {
        if (entry.col == 0) continue;
        std::size_t p = static_cast<U>(entry.col * salt) >> shift;
        while (next[p].col != 0) p = (p + 1) & (capacity - 1);
        next[p] = entry;
      }
      small().point_index_.swap(next);
      small().index_salt_ = salt;
      small().index_shift_ = shift;
    }
    // 5点目の追加失敗後にinline点だけ変わった場合も、キャッシュを作り直す。
    if (small().point_count_ == inline_limit)
      for (U i = 0; i < inline_limit; ++i) {
        const Point point{small().inline_columns_[i], small().inline_values_[i]};
        small().point_index_[index_position(point.col)] = IndexedPoint{point.col, i, point.value};
      }
  }

  static constexpr U lowbit(U x) noexcept { return x & (U{0} - x); }
  U inline_count() const noexcept { return std::min(small().point_count_, static_cast<U>(inline_limit)); }
  std::size_t extra_nodes(U col, U m) const noexcept {
    const U a = col - 1;
    U nearest = std::numeric_limits<U>::max();
    for (U i = 0, end = inline_count(); i < end; ++i)
      nearest = std::min(nearest, a ^ (small().inline_columns_[i] - 1));
    for (const Point& point : small().overflow_)
      nearest = std::min(nearest, a ^ (point.col - 1));
    const unsigned width = std::bit_width(nearest);
    const std::uint64_t mask = (std::uint64_t{1} << width) - 1;
    const std::uint64_t common = (std::uint64_t{a} | mask) + 1;
    if (common <= m) return std::popcount(static_cast<U>(common) - col);
    const U root = m & (std::numeric_limits<U>::max() << (std::bit_width(a ^ m) - 1));
    return std::size_t{1} + std::popcount(root - col);
  }
 public:
  void export_cells(U m,std::vector<T>& values,std::vector<std::uint8_t>& flags)const {
    values.assign(std::size_t{std::bit_ceil(m)}+1,T{});flags.assign(std::size_t{m}+1,0);
    std::fill(values.begin()+std::size_t{m}+1,values.end(),std::numeric_limits<T>::max());
    if(const auto* map=fenwick_map()) {
      map->for_each([&](const U& col,const T& value){values[col]=value;flags[col]=1;});return;
    }
    const auto put=[&](U col,Unsigned value){
      for(U c=col;c<=m;c+=lowbit(c)) {
        values[c]=static_cast<T>(static_cast<Arithmetic>(static_cast<Unsigned>(values[c]))+static_cast<Arithmetic>(value));flags[c]=1;
      }
    };
    for(U i=0;i<inline_count();++i)put(small().inline_columns_[i],small().inline_values_[i]);
    for(const Point& point:small().overflow_)put(point.col,point.value);
  }
  void promote_prefix(U m) noexcept {if(auto* tree=std::get_if<2>(&storage_))tree->promote_prefix(m);}
  SmallFenwickRow() noexcept = default;
  SmallFenwickRow(const SmallFenwickRow&) = default;
  SmallFenwickRow(SmallFenwickRow&& other) noexcept { swap(other); }
  SmallFenwickRow& operator=(SmallFenwickRow other) {
    swap(other);
    return *this;
  }
  void swap(SmallFenwickRow& other) noexcept {
    if (storage_.index() != other.storage_.index()) {
      Storage next(std::move(storage_));
      move_state(other.storage_, storage_);
      move_state(next, other.storage_);
    } else if (auto* map = fenwick_map()) map->swap(*other.fenwick_map());
    else if (auto* tree = std::get_if<2>(&storage_)) tree->swap(*std::get_if<2>(&other.storage_));
    else if (auto* flat = std::get_if<3>(&storage_)) flat->swap(*std::get_if<3>(&other.storage_));
    else {
      using std::swap;
      swap(small().point_count_, other.small().point_count_);
      swap(small().small_nodes_, other.small().small_nodes_);
      small().point_index_.swap(other.small().point_index_);
      swap(small().index_salt_, other.small().index_salt_);
      swap(small().index_shift_, other.small().index_shift_);
      small().inline_columns_.swap(other.small().inline_columns_);
      small().inline_values_.swap(other.small().inline_values_);
      small().overflow_.swap(other.small().overflow_);
    }
  }
  Hash* fenwick_map() noexcept { return std::get_if<1>(&storage_); }
  const Hash* fenwick_map() const noexcept { return std::get_if<1>(&storage_); }
  std::size_t size() const noexcept {
    const auto* map = fenwick_map();
    if (map) return map->size();
    if (const auto* tree = std::get_if<2>(&storage_)) return tree->size();
    if (const auto* flat = std::get_if<3>(&storage_)) return flat->cells;
    return small().small_nodes_;
  }
  template <bool UseTree>
  std::size_t size_mode() const noexcept {
    if constexpr (UseTree) {
      const auto* tree = std::get_if<2>(&storage_);
      if(tree)return tree->size();
      if(const auto* flat=std::get_if<3>(&storage_))return flat->cells;
      return small().small_nodes_;
    } else {
      const auto* map = fenwick_map();
      return map ? map->size() : small().small_nodes_;
    }
  }
  static constexpr bool uses_tree(U m) noexcept { return m >= tree_minimum_width; }
  bool empty() const noexcept { return size() == 0; }
  static void clear_small(SmallStorage& state) noexcept {
    state.point_count_=0;state.overflow_.clear();
    for(IndexedPoint& entry:state.point_index_)entry.col=0;
    state.small_nodes_=0;
  }
  void clear() noexcept {
    if(auto* state=std::get_if<0>(&storage_)) { clear_small(*state);return; }
    if (auto* flat=std::get_if<3>(&storage_)) { flat->clear();return; }
    if (auto* tree = std::get_if<2>(&storage_)) { tree->clear(); return; }
    if (auto* map = fenwick_map()) { map->clear(); return; }
  }
  void clear(U m) noexcept {
    if(auto* state=std::get_if<0>(&storage_)) { clear_small(*state);return; }
    if(auto* flat=std::get_if<3>(&storage_)) {
      if(uses_tree(m))flat->clear();else storage_.template emplace<0>();
      return;
    }
    if (auto* tree = std::get_if<2>(&storage_)) {
      if (uses_tree(m)) tree->clear(m);
      else storage_.template emplace<0>();
      return;
    }
    if (fenwick_map() && uses_tree(m)) {
      storage_.template emplace<0>();
      return;
    }
    clear();
  }
  void add(U col, const T& value, U m) {
    if (fenwick_map()) add_mode<false>(col,value,m);
    else if (std::get_if<2>(&storage_) || std::get_if<3>(&storage_) || uses_tree(m)) add_mode<true>(col,value,m);
    else add_mode<false>(col,value,m);
  }
  template <bool UseTree>
  void add_mode(U col, const T& value, U m) {
    if constexpr (UseTree) {
      if (storage_.index() != 0) {
        if (auto* tree = std::get_if<2>(&storage_)) { tree->add(col,value,m); return; }
        if(auto* flat=std::get_if<3>(&storage_)) {
          if(flat->add(col,value,m))return;
          Tree next;
          next.reserve_points(std::size_t{flat->count}+1);
          for(U i=0;i<flat->count;++i)
            next.add(flat->payload->cols[i],static_cast<T>(flat->payload->values[i]),m);
          next.add(col,value,m);
          storage_.template emplace<2>(std::move(next));
          return;
        }
      }
    } else {
      if (auto* map = fenwick_map()) {
        for (U c=col; c<=m; c+=lowbit(c)) (*map)[c]+=value;
        return;
      }
    }
    if (small().point_count_ > inline_limit) {
      IndexedPoint& entry = small().point_index_[index_position(col)];
      if (entry.col == col) {
        entry.value = static_cast<Unsigned>(static_cast<Arithmetic>(entry.value) +
                                           static_cast<Arithmetic>(static_cast<Unsigned>(value)));
        if (entry.position < inline_limit) small().inline_values_[entry.position] = entry.value;
        else small().overflow_[entry.position - inline_limit].value = entry.value;
        return;
      }
    } else {
      for (U i = 0; i < small().point_count_; ++i) {
        if (small().inline_columns_[i] == col) {
          auto& value_at = small().inline_values_[i];
          value_at = static_cast<Unsigned>(static_cast<Arithmetic>(value_at) +
                                             static_cast<Arithmetic>(static_cast<Unsigned>(value)));
          return;
        }
      }
    }
    if(small().point_count_<inline_limit) {
      const std::size_t extra=extra_nodes(col,m);
      const U position=small().point_count_;
      small().inline_columns_[position]=col;
      small().inline_values_[position]=static_cast<Unsigned>(value);
      ++small().point_count_;small().small_nodes_+=static_cast<U>(extra);
      return;
    }
    insert_new<UseTree>(col,value,m);
  }
  template <bool UseTree>
  void insert_new(U col,const T& value,U m) {
    if (small().point_count_ < point_limit) {
      const std::size_t extra = extra_nodes(col, m);
      prepare_index();
      const std::size_t slot = index_position(col);
      small().overflow_.push_back(Point{col, static_cast<Unsigned>(value)});
      small().point_index_[slot] = IndexedPoint{col, small().point_count_, static_cast<Unsigned>(value)};
      ++small().point_count_;
      small().small_nodes_ += static_cast<U>(extra);
      return;
    }
    if constexpr (!UseTree) {
      Hash next;
      next.reserve(std::size_t{small().small_nodes_} + extra_nodes(col, m));
      const auto replay = [&](const Point& point) {
        for (U c = point.col; c <= m; c += lowbit(c)) {
          T& cell = next[c];
          cell = static_cast<T>(static_cast<Arithmetic>(static_cast<Unsigned>(cell)) +
                                static_cast<Arithmetic>(point.value));
        }
      };
      for (U i = 0, end = inline_count(); i < end; ++i)
        replay(Point{small().inline_columns_[i], small().inline_values_[i]});
      for (const Point& point : small().overflow_) replay(point);
      replay(Point{col,static_cast<Unsigned>(value)});
      storage_.template emplace<1>(std::move(next));
    } else {
      FlatRow next;
      for(U i=0,end=inline_count();i<end;++i)
        next.append_known(small().inline_columns_[i],small().inline_values_[i]);
      for(const Point& point:small().overflow_) next.append_known(point.col,point.value);
      next.cells=small().small_nodes_;
      next.add(col,value,m);
      storage_.template emplace<3>(std::move(next));
    }
  }

  // bound<Upper>(w,m): 非負点列の累積和を探索。少点はスタック上で重み付き選択する。
  template<bool Upper> U bound(T w,U m) const noexcept {
    if constexpr(Upper) { if(w<T{})return 0; }
    else if(w<=T{})return 0;
    if(const auto* map=fenwick_map()) {
      if(map->empty())return m;
      U x=0;
      for(U step=std::bit_floor(m);step;step>>=1) {
        const U next=x+step;
        if(next>m)continue;
        const T* found=map->find_nonempty(next);
        const T cell=found ? *found : T{};
        if(Upper ? cell<=w : cell<w) { w=static_cast<T>(w-cell);x=next; }
      }
      return x;
    }
    if(const auto* tree=std::get_if<2>(&storage_))return tree->template bound<Upper>(w,m);
    struct WeightedPoint { U col;Unsigned value; };
    std::array<WeightedPoint,128> points;
    U count=0;
    Unsigned total{};
    const auto append=[&](U col,Unsigned value) {
      points[count++]={col,value};
      total=static_cast<Unsigned>(static_cast<Arithmetic>(total)+static_cast<Arithmetic>(value));
    };
    if(const auto* flat=std::get_if<3>(&storage_)) {
      for(U i=0;i<flat->count;++i)append(flat->payload->cols[i],flat->payload->values[i]);
    } else {
      for(U i=0,end=inline_count();i<end;++i)append(small().inline_columns_[i],small().inline_values_[i]);
      for(const Point& point:small().overflow_)append(point.col,point.value);
    }
    Unsigned remaining=static_cast<Unsigned>(w);
    if(Upper ? total<=remaining : total<remaining)return m;
    U begin=0,end=count;
    U random=initial_index_salt();
    while(end-begin>1) {
      random^=random<<13;random^=random>>17;random^=random<<5;
      const U pivot=points[begin+random%(end-begin)].col;
      U left=begin,scan=begin,right=end;
      Unsigned before{},equal{};
      while(scan<right) {
        if(points[scan].col<pivot) {
          before=static_cast<Unsigned>(static_cast<Arithmetic>(before)+static_cast<Arithmetic>(points[scan].value));
          std::swap(points[left++],points[scan++]);
        } else if(points[scan].col>pivot)std::swap(points[scan],points[--right]);
        else { equal=points[scan].value;++scan; }
      }
      if(Upper ? remaining<before : remaining<=before) {end=left;continue;}
      const Unsigned through=static_cast<Unsigned>(static_cast<Arithmetic>(before)+static_cast<Arithmetic>(equal));
      if(Upper ? remaining<through : remaining<=through)return pivot-1;
      remaining=static_cast<Unsigned>(static_cast<Arithmetic>(remaining)-static_cast<Arithmetic>(through));begin=right;
    }
    return points[begin].col-1;
  }
  // reserve(nodes,m): 既存セル数を変えず、予定セル数に応じた保持形式と容量を準備する。
  void reserve(std::size_t nodes,U m) {
    if(!nodes)return;
    nodes=std::min(nodes,static_cast<std::size_t>(m));
    if(!nodes)return;
    if(auto* map=fenwick_map()) {map->reserve(nodes);return;}
    if(auto* tree=std::get_if<2>(&storage_)) {tree->reserve(nodes,m);return;}
    if(auto* flat=std::get_if<3>(&storage_)) {
      if(nodes<=128)return; // 固定payloadには128点分が既に確保されている。
      Tree next;next.reserve(nodes,m);
      for(U i=0;i<flat->count;++i)next.add(flat->payload->cols[i],static_cast<T>(flat->payload->values[i]),m);
      storage_.template emplace<2>(std::move(next));return;
    }
    if(nodes>point_limit) {
      if(uses_tree(m)) {
        if(nodes<=128) {
          FlatRow next;
          for(U i=0,end=inline_count();i<end;++i)next.append_known(small().inline_columns_[i],small().inline_values_[i]);
          for(const Point& point:small().overflow_)next.append_known(point.col,point.value);
          next.cells=small().small_nodes_;storage_.template emplace<3>(std::move(next));
        } else {
          Tree next;next.reserve(nodes,m);
          for(U i=0,end=inline_count();i<end;++i)next.add(small().inline_columns_[i],static_cast<T>(small().inline_values_[i]),m);
          for(const Point& point:small().overflow_)next.add(point.col,static_cast<T>(point.value),m);
          storage_.template emplace<2>(std::move(next));
        }
      } else {
        Hash next;next.reserve(nodes);
        const auto replay=[&](U col,Unsigned value) {
          for(U c=col;c<=m;c+=lowbit(c)) {
            T& cell=next[c];
            cell=static_cast<T>(static_cast<Arithmetic>(static_cast<Unsigned>(cell))+static_cast<Arithmetic>(value));
          }
        };
        for(U i=0,end=inline_count();i<end;++i)replay(small().inline_columns_[i],small().inline_values_[i]);
        for(const Point& point:small().overflow_)replay(point.col,point.value);
        storage_.template emplace<1>(std::move(next));
      }
      return;
    }
    // inline分は行内に確保済み。以後の点列と索引をまとめて準備する。
    if(nodes<=inline_limit)return;
    small().overflow_.reserve(nodes-inline_limit);
    std::size_t capacity=8;
    while(nodes>capacity-capacity/4)capacity*=2;
    if(capacity<=small().point_index_.size())return;
    const U salt=small().index_salt_ ? small().index_salt_ : initial_index_salt();
    const unsigned shift=32-std::countr_zero(capacity);
    std::vector<IndexedPoint> next(capacity);
    for(const IndexedPoint& entry:small().point_index_) {
      if(!entry.col)continue;
      std::size_t p=static_cast<U>(entry.col*salt)>>shift;
      while(next[p].col)p=(p+1)&(capacity-1);
      next[p]=entry;
    }
    small().point_index_.swap(next);small().index_salt_=salt;small().index_shift_=shift;
  }

  // direct_values(): 直接読めるdense行の1始まり値配列。その他はnullptr。
  const Unsigned* direct_values() const noexcept {
    if constexpr(DirectReads) {
      if(const auto* tree=std::get_if<2>(&storage_))return tree->direct_values();
    }
    return nullptr;
  }
  // get(col): 1始まりの元列値。prefix(endpoint): 元列[1,endpoint]の和。
  T get(U col) const {
    if(const auto* tree=std::get_if<2>(&storage_))return tree->get(col);
    if(const auto* map=fenwick_map()) {
      if(map->empty())return T{};
      const T* value=map->find_nonempty(col);
      Unsigned result=value ? static_cast<Unsigned>(*value) : Unsigned{};
      const U common=col-lowbit(col);
      for(U c=col-1;c!=common;c-=lowbit(c))if(const T* cell=map->find_nonempty(c))
        result=static_cast<Unsigned>(static_cast<Arithmetic>(result)-static_cast<Arithmetic>(static_cast<Unsigned>(*cell)));
      return static_cast<T>(result);
    }
    if(const auto* flat=std::get_if<3>(&storage_))return flat->sum(col-1,col);
    if(small().point_count_>inline_limit) {
      const IndexedPoint& entry=small().point_index_[index_position(col)];
      return entry.col==col ? static_cast<T>(entry.value) : T{};
    }
    for(U i=0;i<small().point_count_;++i)
      if(small().inline_columns_[i]==col)return static_cast<T>(small().inline_values_[i]);
    return T{};
  }
  T prefix(U endpoint) const {
    if(!endpoint)return T{};
    if(const auto* tree=std::get_if<2>(&storage_))return tree->prefix(endpoint);
    if(const auto* map=fenwick_map()) {
      if(map->empty())return T{};
      Arithmetic result{};
      for(U c=endpoint;c;c-=lowbit(c))if(const T* value=map->find_nonempty(c))
        result+=static_cast<Arithmetic>(static_cast<Unsigned>(*value));
      return static_cast<T>(result);
    }
    if(const auto* flat=std::get_if<3>(&storage_))return flat->sum(0,endpoint);
    Arithmetic result{};
    for(U i=0,end=inline_count();i<end;++i)
      result+=static_cast<Arithmetic>(small().inline_values_[i])*static_cast<Arithmetic>(small().inline_columns_[i]<=endpoint);
    for(const Point& point:small().overflow_)
      result+=static_cast<Arithmetic>(point.value)*static_cast<Arithmetic>(point.col<=endpoint);
    return static_cast<T>(result);
  }
  T sum(U ml, U mr, U common) const {
    if (std::get_if<2>(&storage_) || std::get_if<3>(&storage_)) return sum_mode<true>(ml,mr,common);
    return sum_mode<false>(ml,mr,common);
  }
  template <bool UseTree>
  T sum_mode(U ml,U mr,U common) const {
    if constexpr (!UseTree) {
    if (const auto* map = fenwick_map()) {
      T result{};
      for (U c = mr; c != common; c -= lowbit(c))
        if (const T* value = map->find_nonempty(c)) result += *value;
      for (U c = ml; c != common; c -= lowbit(c))
        if (const T* value = map->find_nonempty(c)) result -= *value;
      return result;
    }
    }
    if constexpr (UseTree) {
      if (storage_.index() != 0) {
        if (const auto* tree = std::get_if<2>(&storage_)) return tree->sum(ml,mr);
        if(const auto* flat=std::get_if<3>(&storage_))return flat->sum(ml,mr);
      }
    }
    {
      if (mr - ml == 1) {
        if (small().point_count_ > inline_limit) {
          const IndexedPoint& entry = small().point_index_[index_position(mr)];
          return entry.col == mr ? static_cast<T>(entry.value) : T{};
        }
        for (U i = 0; i < small().point_count_; ++i)
          if (small().inline_columns_[i] == mr) return static_cast<T>(small().inline_values_[i]);
        return T{};
      }
      Arithmetic result{};
      for (U i = 0, end = inline_count(); i < end; ++i) {
        const Point point{small().inline_columns_[i], small().inline_values_[i]};
        const bool inside = point.col - ml - U{1} < mr - ml;
        result += static_cast<Arithmetic>(point.value) * static_cast<Arithmetic>(inside);
      }
      for (const Point& point : small().overflow_) {
        const bool inside = point.col - ml - U{1} < mr - ml;
        result += static_cast<Arithmetic>(point.value) * static_cast<Arithmetic>(inside);
      }
      return static_cast<T>(result);
    }
  }
};
}  // namespace reinsirk::detail
#endif  // REINSIRK_DYNAMIC_FENWICK_SMALL_ROW_DEFINED


#ifndef REINSIRK_DYNAMIC_FENWICK_TREE_DEFINED
#define REINSIRK_DYNAMIC_FENWICK_TREE_DEFINED
namespace reinsirk::detail {
template <typename S, typename T>
class FenwickFallback {
  // FenwickFallback(n=0): 長さnの零配列を作る。n>=0。
  // add(k,x): 0<=k<size()の要素にxを加算する。
  // sum(k): 半開区間[0,k)の和。0<=k<=size()。
  // sum(l,r): 半開区間[l,r)の和。0<=l<=r<=size()。
  // get(k)/operator[](k): 位置kの値を返す。読み取りではノードを増やさない。
  // set(k,x): 位置kの値をxへ変更する。
  // sum_all(): 配列全体の和を返す。
  // lower_bound(w): 前方累積和がw以上になる最初の要素位置。なければsize()、w<=0なら0。
  // upper_bound(w): 前方累積和がwより大きくなる最初の要素位置。なければsize()、w<0なら0。
  // size()/empty(): 配列の長さ／長さが0かを返す（全要素0とは区別する）。
  // clear(): 長さと確保済み容量を維持して、全要素を0にする。
  // reset(n): 長さn>=0の零配列へ変更する。確保済み容量を再利用する。
  // reserve(nodes): 格納する内部ノード数を事前確保する（要素数ではない）。
  // stored_nodes(): 格納済み内部ノード数。値が0になったノードも含む。
  static_assert(std::is_integral_v<S> && !std::is_same_v<S, bool> &&
                std::is_same_v<S, std::remove_cv_t<S>> && sizeof(S) <= 8);
  using U = std::make_unsigned_t<S>;
  U n_ = 0;
  // 広い疎領域は占有フラグをbitで持ち、密な範囲は直接配列へ移す。
  reinsirk::detail::InternalHashMap<U, T, false, 4> data_;
  reinsirk::detail::InternalHashMap<U, T, true, 4> sparse_;
  bool sparse_domain() const noexcept { return n_ > 1'000'000; }
  template <class F> decltype(auto) with_data(F&& f) {
    if (sparse_domain()) return f(sparse_);
    return f(data_);
  }
  template <class F> decltype(auto) with_data(F&& f) const {
    if (sparse_domain()) return f(sparse_);
    return f(data_);
  }
  struct SmallCell { T value{}; };
  std::vector<SmallCell> small_;
  std::uint64_t small_used_ = 0;
  std::vector<std::uint8_t> dense_used_;
  bool direct_storage() const noexcept { return small_domain() || !dense_used_.empty(); }
  void promote_dense() {
    // 値型による例外を増やさず、直接配列は合計16MiB以内に制限する。
    if constexpr (std::is_nothrow_default_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) {
      constexpr std::size_t budget = std::size_t{16} << 20;
      if (n_ > 1'000'000 || small_used_ == UINT64_MAX ||
          sizeof(SmallCell) >= budget ||
          static_cast<std::size_t>(n_) >= budget / (sizeof(SmallCell) + 1) ||
          data_.size() < (static_cast<std::size_t>(n_) + 7) / 8) return;
      try {
        std::vector<SmallCell> values(static_cast<std::size_t>(n_) + 1);
        std::vector<std::uint8_t> used(values.size(), 0);
        data_.for_each([&](const U& key, const T& value) {
          values[key].value = value;
          used[key] = 1;
        });
        small_used_ = data_.size();
        small_.swap(values); dense_used_.swap(used); data_.clear();
      } catch (const std::bad_alloc&) {
        // 元表を保ち、次のclear/resetまで再試行しない。
        small_used_ = UINT64_MAX;
      }
    }
  }
  bool small_domain() const noexcept { return n_ <= 64; }

  static constexpr U lowbit(U x) noexcept { return x & (U{0} - x); }
  static U checked_size(S n) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (std::is_signed_v<S>)
      if (n < 0) throw std::invalid_argument("FenwickFallback: negative size");
#endif
    return static_cast<U>(n);
  }
  void check_position(S k) const {
#ifndef SKIP_RUNTIME_CHECKS
    if (static_cast<U>(k) >= n_)
      throw std::out_of_range("FenwickFallback: position out of range");
#else
    (void)k;
#endif
  }
  void check_endpoint(S k) const {
#ifndef SKIP_RUNTIME_CHECKS
    if (static_cast<U>(k) > n_)
      throw std::out_of_range("FenwickFallback: endpoint out of range");
#else
    (void)k;
#endif
  }
  T interval(U l, U r) const {
    T result{};
    if (l == r) return result;
    if (direct_storage()) {
      if (small_.empty()) return result;
      for (; r; r -= lowbit(r)) result += small_[r].value;
      for (; l; l -= lowbit(l)) result -= small_[l].value;
      return result;
    }
    return with_data([&](const auto& data) {
      T total{};
      if (data.empty()) return total;
      U common=0;
      if constexpr(std::is_integral_v<T> && !std::is_same_v<T,bool>)
        common=static_cast<U>((l&r)&static_cast<U>(U{0}-std::bit_floor(static_cast<U>(l^r))));
      for (; r != common; r -= lowbit(r)) if (const T* p = data.find_nonempty(r)) total += *p;
      for (; l != common; l -= lowbit(l)) if (const T* p = data.find_nonempty(l)) total -= *p;
      return total;
    });
  }
  template <bool Upper>
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline S bound(T w) const {
    if constexpr (Upper) { if (w < T{}) return S{0}; }
    else { if (w <= T{}) return S{0}; }
    if (direct_storage()) {
      if (small_.empty()) return static_cast<S>(n_);
      // 正の累積和だけを探す場合、先頭1要素で決まれば二分探索を省く。
      if constexpr (Upper) {
        if (w < small_[1].value) return S{0};
      } else {
        if (w <= small_[1].value) return S{0};
      }
      U x = 0;
      for (U step = std::bit_floor(n_); step; step >>= 1) {
        const U next = x + step;
        if (next <= n_) {
          const bool advance = Upper ? small_[next].value <= w : small_[next].value < w;
          if (advance) { w -= small_[next].value; x = next; }
        }
      }
      return static_cast<S>(x);
    }
    return with_data([&](const auto& data) {
      if (data.empty()) return static_cast<S>(n_);
      U x = 0;
      for (U step = std::bit_floor(n_); step; step >>= 1) {
        const U next = x + step;
        if (next > n_) continue;
        const T* p = data.find_nonempty(next);
        const T cell = p ? *p : T{};
        const bool advance = Upper ? cell <= w : cell < w;
        if (advance) { w -= cell; x = next; }
      }
      return static_cast<S>(x);
    });
  }

 public:
  explicit FenwickFallback(S n = 0) : n_(checked_size(n)) {}
  FenwickFallback(const FenwickFallback&) = default;
  FenwickFallback& operator=(const FenwickFallback&) = default;
  FenwickFallback(FenwickFallback&& other) noexcept(
      std::is_nothrow_move_constructible_v<reinsirk::detail::InternalHashMap<U,T,false,4>>)
      : n_(std::exchange(other.n_, 0)), data_(std::move(other.data_)), sparse_(std::move(other.sparse_)),
        small_(std::move(other.small_)), small_used_(std::exchange(other.small_used_, 0)),
        dense_used_(std::move(other.dense_used_)) {}
  FenwickFallback& operator=(FenwickFallback&& other) noexcept(
      std::is_nothrow_move_assignable_v<reinsirk::detail::InternalHashMap<U,T,false,4>>) {
    if (this != &other) {
      data_ = std::move(other.data_); sparse_ = std::move(other.sparse_);
      small_ = std::move(other.small_); small_used_ = std::exchange(other.small_used_, 0);
      dense_used_ = std::move(other.dense_used_);
      n_ = std::exchange(other.n_, 0);
    }
    return *this;
  }
  S size() const noexcept { return static_cast<S>(n_); }
  bool empty() const noexcept { return n_ == 0; }
  std::size_t stored_nodes() const noexcept {
    const auto direct = small_domain() ? static_cast<std::size_t>(std::popcount(small_used_))
                                      : dense_used_.empty() ? 0 : small_used_;
    return data_.size() + sparse_.size() + direct;
  }
  void reserve(std::size_t nodes) {
    if (nodes == 0) return;
    if (direct_storage()) small_.reserve(static_cast<std::size_t>(n_) + 1);
    else with_data([&](auto& data) { data.reserve(nodes); });
  }
  void clear() { data_.clear(); sparse_.clear(); small_.clear(); dense_used_.clear(); small_used_ = 0; }
  void reset(S n) { const U next = checked_size(n); clear(); n_ = next; }
  void add(S k, const T& x) {
    check_position(k);
    U i = static_cast<U>(k);
    if (i == std::numeric_limits<U>::max()) return;
    if (direct_storage()) {
      if (small_.empty()) small_.resize(static_cast<std::size_t>(n_) + 1);
      if (small_domain()) {
        for (++i; i <= n_; i += lowbit(i)) {
          small_[i].value += x;
          small_used_ |= UINT64_C(1) << (i - 1);
        }
      } else if (small_used_ == n_) {
        for (++i; static_cast<U>(i - U{1}) < n_; i += lowbit(i)) small_[i].value += x;
      } else {
        for (++i; static_cast<U>(i - U{1}) < n_; i += lowbit(i)) {
          small_[i].value += x;
          if (!dense_used_[i]) { dense_used_[i] = 1; ++small_used_; }
        }
      }
      return;
    }
    // 1始まりへの変換と更新先の加算は、検査省略時も桁あふれさせない。
    with_data([&](auto& data) {
      for (++i; i <= n_;) {
        data[i] += x;
        const U step = lowbit(i);
        if (step > n_ - i) break;
        i += step;
      }
    });
    if (!sparse_domain()) promote_dense();
  }
  T sum(S k) const { check_endpoint(k); return interval(0, static_cast<U>(k)); }
  T sum(S l, S r) const {
    check_endpoint(l); check_endpoint(r);
#ifndef SKIP_RUNTIME_CHECKS
    if (l > r) throw std::invalid_argument("FenwickFallback: reversed interval");
#endif
    return interval(static_cast<U>(l), static_cast<U>(r));
  }
  T get(S k) const {
    check_position(k);
    const U i = static_cast<U>(k);
    if (i == std::numeric_limits<U>::max()) return T{};
    if (direct_storage()) {
      if (small_.empty()) return T{};
      U j = i + U{1}; T result = small_[j].value;
      const U common = j - lowbit(j);
      for (j = i; j != common; j -= lowbit(j)) result -= small_[j].value;
      return result;
    }
    return with_data([&](const auto& data) {
      if (data.empty()) return T{};
      U j = i + U{1};
      T result{};
      if (const T* p = data.find_nonempty(j)) result = *p;
      const U common = j - lowbit(j);
      for (j = i; j != common; j -= lowbit(j))
        if (const T* p = data.find_nonempty(j)) result -= *p;
      return result;
    });
  }
  T operator[](S k) const { return get(k); }
  void set(S k, const T& x) { add(k, x - get(k)); }
  T sum_all() const { return interval(0, n_); }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline S lower_bound(T w) const { return bound<false>(std::move(w)); }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline S upper_bound(T w) const { return bound<true>(std::move(w)); }
};

template<class S,class T>
class CompactFenwick {
  using U=std::make_unsigned_t<S>;
  using Row=SmallFenwickRow<T, true>;
  using Legacy=FenwickFallback<S,T>;
  struct Tiny {
    std::vector<T> values;
    std::vector<std::uint8_t> flags;
    std::uint64_t used=0;
  };
  struct Wide {
    std::unique_ptr<Legacy> ptr;
    Wide()=default;
    Wide(const Wide& other):ptr(other.ptr ? std::make_unique<Legacy>(*other.ptr):nullptr){}
    Wide(Wide&&)=default;
    Wide& operator=(Wide other) noexcept {ptr.swap(other.ptr);return *this;}
  };
  using Storage=std::variant<Row,Tiny,Wide>;
  U n_=0, prefix_=0;
  bool dense_failed_=false;
  Storage storage_{std::in_place_index<1>};
  static U checked_size(S n) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr(std::is_signed_v<S>)if(n<0)throw std::invalid_argument("DynamicFenwickTree: negative size");
#endif
    return static_cast<U>(n);
  }
  static constexpr U lowbit(U x)noexcept{return x&(U{0}-x);}
  void check_position(S k)const {
#ifndef SKIP_RUNTIME_CHECKS
    if(static_cast<U>(k)>=n_)throw std::out_of_range("DynamicFenwickTree: position out of range");
#else
    (void)k;
#endif
  }
  void check_endpoint(S k)const {
#ifndef SKIP_RUNTIME_CHECKS
    if(static_cast<U>(k)>n_)throw std::out_of_range("DynamicFenwickTree: endpoint out of range");
#else
    (void)k;
#endif
  }
  bool compact()const noexcept{return n_>64 && std::uint64_t(n_)<=INT32_MAX;}
  void select_storage() {
    if(n_<=64) storage_.template emplace<1>();
    else if(compact())storage_.template emplace<0>();
    else storage_.template emplace<2>();
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline T interval(U l,U r)const {
    if(l==r)return T{};
    if(const auto* tiny=std::get_if<1>(&storage_)) {
      T result{};if(tiny->values.empty())return result;
      const U common=(l&r)&(U{0}-std::bit_floor(static_cast<U>(l^r)));
      for(;r!=common;r-=lowbit(r))result+=tiny->values[r];
      for(;l!=common;l-=lowbit(l))result-=tiny->values[l];
      return result;
    }
    if(const auto* row=std::get_if<0>(&storage_)) {
      const auto a=static_cast<std::uint32_t>(l),b=static_cast<std::uint32_t>(r);
      const auto common=(a&b)&(std::uint32_t{0}-std::bit_floor(a^b));
      return n_<8192 ? row->template sum_mode<false>(a,b,common):row->template sum_mode<true>(a,b,common);
    }
    const auto& wide=std::get<2>(storage_);
    return wide.ptr ? wide.ptr->sum(static_cast<S>(l),static_cast<S>(r)):T{};
  }
  template<bool Upper>
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline S bound(T w)const {
    if constexpr(Upper){if(w<T{})return S{0};}
    else if(w<=T{})return S{0};
    if(const auto* tiny=std::get_if<1>(&storage_)) {
      if(tiny->values.empty())return static_cast<S>(n_);
      if constexpr(Upper){if(w<tiny->values[1])return S{0};}
      else if(w<=tiny->values[1])return S{0};
      using Index=std::conditional_t<(sizeof(U)>4),U,std::uint32_t>;const Index width=static_cast<Index>(n_);Index x=0;
      Index first=std::bit_floor(width);
      if(first==width) {
        const T total=tiny->values[width];
        if(Upper ? total<=w:total<w)return static_cast<S>(n_);
        first>>=1;
      }
      for(Index step=first;step;step>>=1){
        const Index next=x+step;
        {
          const T cell=tiny->values[next];
          if(Upper ? cell<=w:cell<w){w-=cell;x=next;}
        }
      }
      return static_cast<S>(std::min(x,width));
    }
    if(const auto* row=std::get_if<0>(&storage_))if(const auto* values=row->direct_values()) {
      if constexpr(Upper){if(w<static_cast<T>(values[1]))return S{0};}
      else if(w<=static_cast<T>(values[1]))return S{0};
      using Index=std::uint32_t;const Index width=static_cast<Index>(n_);Index x=0;
      Index first=std::bit_floor(width);
      if(first==width) {
        const T total=static_cast<T>(values[width]);
        if(Upper ? total<=w:total<w)return static_cast<S>(width);
        first>>=1;
      }
      for(Index step=first;step;step>>=1) {
        const Index next=x+step;
        {
          const T cell=static_cast<T>(values[next]);
          if(Upper ? cell<=w : cell<w){w-=cell;x=next;}
        }
      }
      return static_cast<S>(std::min(x,width));
    }
    return bound_sparse<Upper>(w);
  }
  template<bool Upper>
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline S bound_sparse(T w)const {
    if(const auto* row=std::get_if<0>(&storage_))return static_cast<S>(row->template bound<Upper>(w,static_cast<std::uint32_t>(n_)));
    const auto& wide=std::get<2>(storage_);
    if(!wide.ptr)return static_cast<S>(n_);
    if constexpr(Upper)return wide.ptr->upper_bound(w);
    else return wide.ptr->lower_bound(w);
  }
 public:
  explicit CompactFenwick(S n=0):n_(checked_size(n)){select_storage();}
  CompactFenwick(const CompactFenwick&)=default;
  CompactFenwick& operator=(const CompactFenwick& other){if(this!=&other){CompactFenwick copy(other);*this=std::move(copy);}return *this;}
  CompactFenwick(CompactFenwick&& other)noexcept:n_(std::exchange(other.n_,0)),prefix_(std::exchange(other.prefix_,0)),dense_failed_(std::exchange(other.dense_failed_,false)),storage_(std::move(other.storage_)){other.storage_.template emplace<1>();}
  CompactFenwick& operator=(CompactFenwick&& other)noexcept {
    if(this!=&other){storage_=std::move(other.storage_);n_=std::exchange(other.n_,0);prefix_=std::exchange(other.prefix_,0);dense_failed_=std::exchange(other.dense_failed_,false);other.storage_.template emplace<1>();}return *this;
  }
  S size()const noexcept{return static_cast<S>(n_);}
  bool empty()const noexcept{return n_==0;}
  std::size_t stored_nodes()const noexcept {
    if(const auto* row=std::get_if<0>(&storage_))return row->size();
    if(const auto* tiny=std::get_if<1>(&storage_))return n_<=64 ? std::popcount(tiny->used):tiny->used;
    const auto& wide=std::get<2>(storage_);return wide.ptr ? wide.ptr->stored_nodes():0;
  }
  void reserve(std::size_t nodes) {
    if(!nodes)return;
    if(auto* tiny=std::get_if<1>(&storage_)){tiny->values.reserve(static_cast<std::size_t>(std::bit_ceil(static_cast<std::uint32_t>(n_)))+1);if(n_>64)tiny->flags.reserve(static_cast<std::size_t>(n_)+1);return;}
    if(auto* row=std::get_if<0>(&storage_)){row->reserve(nodes,static_cast<std::uint32_t>(n_));return;}
    auto& wide=std::get<2>(storage_);if(!wide.ptr)wide.ptr=std::make_unique<Legacy>(static_cast<S>(n_));wide.ptr->reserve(nodes);
  }
  void clear() {
    prefix_=0;dense_failed_=false;
    if(auto* row=std::get_if<0>(&storage_)){row->clear(static_cast<std::uint32_t>(n_));return;}
    if(auto* tiny=std::get_if<1>(&storage_)){tiny->values.clear();tiny->flags.clear();tiny->used=0;return;}
    auto& wide=std::get<2>(storage_);if(wide.ptr)wide.ptr->clear();
  }
  void reset(S n){const U next=checked_size(n);clear();n_=next;
    const unsigned kind=n_<=64?1:compact()?0:2;
    if(storage_.index()!=kind)select_storage();
    else if(auto* row=std::get_if<0>(&storage_))row->clear(static_cast<std::uint32_t>(n_));
    else if(auto* wide=std::get_if<2>(&storage_))if(wide->ptr)wide->ptr->reset(static_cast<S>(n_));
  }
  void add(S k,const T& value) {
    check_position(k);const U i=static_cast<U>(k);
    if(auto* tiny=std::get_if<1>(&storage_)) {
      if(tiny->values.empty()) {
        if(n_>64)tiny->flags.assign(static_cast<std::size_t>(n_)+1,0);
        const auto capacity=static_cast<std::size_t>(std::bit_ceil(static_cast<std::uint32_t>(n_)));
        tiny->values.resize(capacity+1);
        std::fill(tiny->values.begin()+static_cast<std::size_t>(n_)+1,tiny->values.end(),std::numeric_limits<T>::max());
      }
      if(n_<=64)for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c)){tiny->values[c]+=value;tiny->used|=UINT64_C(1)<<(c-1);}
      else if(tiny->used==n_)for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c))tiny->values[c]+=value;
      else for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c)) {
        tiny->values[c]+=value;if(!tiny->flags[c]){tiny->flags[c]=1;++tiny->used;}
      }
      return;
    }
    if(auto* row=std::get_if<0>(&storage_)){
      if(n_<8192)row->template add_mode<false>(static_cast<std::uint32_t>(i)+1,value,static_cast<std::uint32_t>(n_));
      else row->template add_mode<true>(static_cast<std::uint32_t>(i)+1,value,static_cast<std::uint32_t>(n_));
      if(n_<8192 && !dense_failed_ && row->size()>=(static_cast<std::size_t>(n_)+7)/8) {
        try {Tiny next;next.used=row->size();row->export_cells(static_cast<std::uint32_t>(n_),next.values,next.flags);storage_.template emplace<1>(std::move(next));return;}
        catch(const std::bad_alloc&){dense_failed_=true;}
      }
      if(prefix_!=std::numeric_limits<U>::max()) {
        if(i!=prefix_)prefix_=std::numeric_limits<U>::max();
        else if(++prefix_>128 && row->size()>=(static_cast<std::size_t>(n_)+31)/32 && n_<=1'000'000){
          row->promote_prefix(static_cast<std::uint32_t>(n_));prefix_=std::numeric_limits<U>::max();
        }
      }
      return;
    }
    auto& wide=std::get<2>(storage_);if(!wide.ptr)wide.ptr=std::make_unique<Legacy>(static_cast<S>(n_));wide.ptr->add(k,value);
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline T sum(S k)const {
    check_endpoint(k);U r=static_cast<U>(k);T result{};
    if(const auto* tiny=std::get_if<1>(&storage_)) {
      if(tiny->values.empty())return result;
      for(;r;r-=lowbit(r))result+=tiny->values[r];
      return result;
    }
    if(const auto* row=std::get_if<0>(&storage_)) {
      const auto end=static_cast<std::uint32_t>(r);
      if(const auto* values=row->direct_values()) {
        for(std::uint32_t c=end;c;c-=c&(std::uint32_t{0}-c))result+=static_cast<T>(values[c]);
        return result;
      }
      return row->prefix(end);
    }
    const auto& wide=std::get<2>(storage_);return wide.ptr ? wide.ptr->sum(k):T{};
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline T sum(S l,S r)const {
    check_endpoint(l);check_endpoint(r);
#ifndef SKIP_RUNTIME_CHECKS
    if(l>r)throw std::invalid_argument("DynamicFenwickTree: reversed interval");
#endif
    if(const auto* tiny=std::get_if<1>(&storage_)) {
      T result{};if(tiny->values.empty())return result;
      U left=static_cast<U>(l),right=static_cast<U>(r);
      while(left!=right){if(left<right){result+=tiny->values[right];right-=lowbit(right);}else{result-=tiny->values[left];left-=lowbit(left);}}
      return result;
    }
    if(const auto* row=std::get_if<0>(&storage_)) {
      if(l==r)return T{};
      if(const auto* values=row->direct_values()) {
      T result{};
      for(std::uint32_t c=static_cast<std::uint32_t>(r);c;c-=c&(std::uint32_t{0}-c))result+=static_cast<T>(values[c]);
      for(std::uint32_t c=static_cast<std::uint32_t>(l);c;c-=c&(std::uint32_t{0}-c))result-=static_cast<T>(values[c]);
      return result;
      }
    }
    return interval(static_cast<U>(l),static_cast<U>(r));
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline T get(S k)const {
    check_position(k);const U i=static_cast<U>(k);
    if(i==std::numeric_limits<U>::max())return T{};
    if(const auto* tiny=std::get_if<1>(&storage_)) {
      if(tiny->values.empty())return T{};
      U j=i+U{1};T result=tiny->values[j];const U common=j-lowbit(j);
      for(j=i;j!=common;j-=lowbit(j))result-=tiny->values[j];
      return result;
    }
    if(const auto* wide=std::get_if<2>(&storage_))return wide->ptr ? wide->ptr->get(k):T{};
    const auto& row=std::get<0>(storage_);
    if(const auto* values=row.direct_values()) {
      std::uint32_t c=static_cast<std::uint32_t>(i)+1;T result=static_cast<T>(values[c]);
      const auto common=c-(c&(std::uint32_t{0}-c));
      for(--c;c!=common;c-=c&(std::uint32_t{0}-c))result-=static_cast<T>(values[c]);
      return result;
    }
    return row.get(static_cast<std::uint32_t>(i)+1);
  }
  T operator[](S k)const{return get(k);}
  void set(S k,const T& value){
    if constexpr(sizeof(U)>4){
      if(auto* wide=std::get_if<2>(&storage_)){
        check_position(k);
        if(!wide->ptr)wide->ptr=std::make_unique<Legacy>(static_cast<S>(n_));
        wide->ptr->set(k,value);return;
      }
    }
    add(k,value-get(k));
  }
  T sum_all()const{return sum(static_cast<S>(n_));}
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline S lower_bound(T w)const{return bound<false>(w);}
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline S upper_bound(T w)const{return bound<true>(w);}
};
template<class S,class T,bool Fast=std::is_integral_v<T> && !std::is_same_v<T,bool>>
class FenwickBase: public FenwickFallback<S,T>{
 using Base=FenwickFallback<S,T>;
 public: using Base::Base;
};
template<class S,class T>class FenwickBase<S,T,true>:public CompactFenwick<S,T>{
 using Base=CompactFenwick<S,T>;
 public: using Base::Base;
};

} // namespace reinsirk::detail
// Sはbool以外の64bit以下の整数型。Tは加減算・零初期化に対応し、演算結果が表現範囲内であること。
// 構築O(1)。通常操作は期待償却O(log n)。整数の少数点は点列、多数点はB+木、密な領域は配列を使う。
// 配列への変換はO(n)、n<=100万・16MiB以内。浮動型・独自型は従来のFenwickセル順で集計する。
// 読み取りは領域を増やさない。stored_nodesは保持形式によらず更新経路のFenwickセル数を返す。
// DynamicFenwickTree(n=0): 長さn>=0の零配列を作る。
// add(k,x): 0<=k<size()の位置へxを加算する。
// sum(k)/sum(l,r): [0,k)／[l,r)の和。0<=l<=r<=size()。
// get(k)/operator[](k): 位置kの値を返す。0<=k<size()。
// set(k,x): 位置kの値をxへ変更する。
// sum_all(): 全要素の和を返す。
// lower_bound(w)/upper_bound(w): 前方累積和がw以上／より大きくなる最初の位置。なければsize()。
// 検索は全要素非負が前提。lower_bound(w<=0)とupper_bound(w<0)は0。
// size()/empty(): 配列の長さ／長さが0かを返す（全要素0とは区別）。
// clear(): 長さと再利用できる容量を保ち、全要素を0へ戻す。
// reset(n): 長さn>=0の零配列へ変更する。保持形式が対応するとき容量を再利用する。
// reserve(nodes): 内部Fenwickセル数を目安に容量を準備するヒント。
// stored_nodes(): 値0や相殺済みも含む登録セル数。読み取りやreserveでは変わらない。
template<typename S,typename T>
class DynamicFenwickTree : public reinsirk::detail::FenwickBase<S,T> {
 using Base=reinsirk::detail::FenwickBase<S,T>;
 public: using Base::Base;
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

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

int main() {
    int n, q;
    if (!(verify_in >> n >> q)) return 0;
    using Point = std::array<std::int64_t, 3>;
    using Query = std::array<std::int64_t, 5>;
    std::vector<Point> initial(n);
    std::vector<Query> queries(q);
    std::vector<int> xs, ys;
    for (auto& p : initial) {
        verify_in >> p[0] >> p[1] >> p[2];
        xs.push_back(static_cast<int>(p[0])); ys.push_back(static_cast<int>(p[1]));
    }
    for (auto& c : queries) {
        verify_in >> c[0] >> c[1] >> c[2] >> c[3];
        if (c[0]) verify_in >> c[4];
        else { xs.push_back(static_cast<int>(c[1])); ys.push_back(static_cast<int>(c[2])); }
    }
    std::sort(xs.begin(), xs.end()); xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end()); ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    auto xi = [&](std::int64_t x) { return static_cast<int>(std::lower_bound(xs.begin(), xs.end(), x) - xs.begin()); };
    auto yi = [&](std::int64_t y) { return static_cast<int>(std::lower_bound(ys.begin(), ys.end(), y) - ys.begin()); };
    // 圧縮するのは初期点・追加点の座標だけ。照会の端点はlower_boundで変換する。
    // 原典と同じ外側Fenwickを組み、各行に今回の1次元BITを利用する。
    const int width = static_cast<int>(xs.size());
    std::vector<DynamicFenwickTree<int, std::int64_t>> rows;
    for (int i = 0; i <= width; ++i) rows.emplace_back(static_cast<int>(ys.size()));
    auto add = [&](int x, int y, std::int64_t w) {
        for (++x; x <= width; x += x & -x) rows[x].add(y, w);
    };
    auto rectangle = [&](int l, int d, int r, int u) {
        std::int64_t answer = 0;
        while (l != r) {
            if (l < r) { answer += rows[r].sum(d, u); r -= r & -r; }
            else { answer -= rows[l].sum(d, u); l -= l & -l; }
        }
        return answer;
    };
    for (const auto& p : initial) add(xi(p[0]), yi(p[1]), p[2]);
    for (const auto& c : queries) {
        if (c[0] == 0) add(xi(c[1]), yi(c[2]), c[3]);
        else verify_out << rectangle(xi(c[1]), yi(c[2]), xi(c[3]), yi(c[4])) << '\n';
    }
}
