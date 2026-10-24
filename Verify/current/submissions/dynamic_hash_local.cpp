// Verify: ローカル独立検査

// Library: DataStructure/dynamic-binary-indexed-tree.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/data-structure/dynamic-binary-indexed-tree.hpp
// Original Source (InternalHashMap): https://nyaannyaan.github.io/library/data-structure/hash-map-variable-length.hpp.html
// Original Source (mix): https://prng.di.unimi.it/splitmix64.c
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/dynamic_fenwick.md
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/internal_hash_map.md
//
// API一覧
// [DynamicFenwickTree<S,T>]
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

#ifndef SIRKIT_INTERNAL_HASH_MAP_DEFINED
#define SIRKIT_INTERNAL_HASH_MAP_DEFINED
namespace reinsirk::detail {
template <class Key, class Val, bool Packed = false, unsigned Denominator = 5>
class InternalHashMap {
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
        if (size_ != 0) {
            for (std::size_t i = 0; i < used_.size(); ++i) {
                if (!used_[i]) continue;
                std::size_t p = static_cast<std::size_t>((static_cast<std::uint64_t>(keys_[i]) * salt_) >> next_shift);
                while (used[p]) p = (p + 1) & (capacity - 1);
                keys[p] = keys_[i];
                values[p].value = values_[i].value;
                used[p] = 1;
            }
        }
        keys_.swap(keys);
        values_.swap(values);
        used_.swap(used);
        shift_ = next_shift;
    }

public:
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

    void reallocate() {
        if (keys_.empty()) {
            rehash(8);
        } else {
            if (keys_.size() > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            rehash(keys_.size() * 2);
        }
    }

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

    template <class F> Val& find_or_emplace(Key key, F&& factory) {
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
        values_[p].value = std::forward<F>(factory)();
        keys_[p] = key;
        used_[p] = 1;
        ++size_;
        return values_[p].value;
    }

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

    std::vector<std::pair<Key, Val>> enumerate() const {
        std::vector<std::pair<Key, Val>> result;
        if (empty()) return result;
        result.reserve(size_);
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) result.emplace_back(keys_[i], values_[i].value);
        return result;
    }

    template <class F> void for_each(F&& f) const {
        if (empty()) return;
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) f(keys_[i], values_[i].value);
    }

    [[nodiscard]] bool contains(Key key) const noexcept { return find(key) != nullptr; }
    [[nodiscard]] bool contain(Key key) const noexcept { return contains(key); }
    [[nodiscard]] int count(Key key) const noexcept { return contains(key) ? 1 : 0; }
    void clear() noexcept {
        if (empty()) return;
        std::fill(used_.begin(), used_.end(), Flag{0});
        size_ = 0;
    }
    void release() noexcept {
        decltype(keys_){}.swap(keys_);
        decltype(values_){}.swap(values_);
        decltype(used_){}.swap(used_);
        size_ = 0;
        shift_ = 61;
    }
    std::size_t capacity() const noexcept { return keys_.size(); }
    double load_factor() const noexcept {
        return capacity() == 0 ? 0.0 : static_cast<double>(size_) / capacity();
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    void set_default(const Val& value) { default_value_ = value; }
};

}  // namespace reinsirk::detail
#endif  // SIRKIT_INTERNAL_HASH_MAP_DEFINED




#include <array>
#include <memory>
#include <variant>
#ifndef SIRKIT_DYNAMIC_FENWICK_BPLUS_ROW_DEFINED
#define SIRKIT_DYNAMIC_FENWICK_BPLUS_ROW_DEFINED
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
  // dense中のroot_は、連続追加中だけ完了済みprefix長を保持する。
  V append_prefix(U endpoint) const noexcept {
    V result{};for(U x=std::min(endpoint,root_);x;x-=lowbit(x))result=plus(result,dense_values_[x]);
    return result;
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::noinline]]
#endif
  void finish_append() noexcept {
    const U prefix=root_;
    std::fill_n(dense_flags_.begin(),prefix/64,~std::uint64_t{0});
    if(prefix%64)dense_flags_[prefix/64]=(std::uint64_t{1}<<(prefix%64))-1;
    cells_=prefix;
    U cell=prefix;
    for(U next=cell+lowbit(cell);next<=dense_width_;next=cell+lowbit(cell)) {
      V value=dense_values_[cell];const U stop=next-lowbit(next);
      for(U x=cell-lowbit(cell);x>stop;x-=lowbit(x))value=plus(value,dense_values_[x]);
      dense_values_[next]=value;cell=next;
      dense_flags_[(cell-1)/64]|=std::uint64_t{1}<<((cell-1)%64);++cells_;
    }
    root_=absent;if constexpr(DirectReads)dense_clean_=true;
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::noinline]]
#endif
  bool append_add(U col,V delta) noexcept {
    if(col!=root_+1){finish_append();return false;}
    // 完了したセルの和を親1個だけへ伝える。
    const V value=plus(dense_values_[col],delta);
    dense_values_[col]=value;
    const U parent=col+lowbit(col);
    if(parent<=dense_width_)dense_values_[parent]=plus(dense_values_[parent],value);
    root_=col;
    if(root_==dense_width_)finish_append();
    return true;
  }
  template<bool Upper>U append_bound(V w,U m)const noexcept {
    if(Upper?append_prefix(root_)<=w:append_prefix(root_)<w)return m;
    U x=0;for(U step=std::bit_floor(root_);step;step>>=1) {
      const U next=x+step;if(next>root_)continue;
      const V cell=dense_values_[next];
      if(Upper?cell<=w:cell<w){w=minus(w,cell);x=next;}
    }
    return x;
  }
  void dense_add(U col,V delta) noexcept {
    if(cells_==dense_width_) {
      for(U c=col;c<=dense_width_;c+=lowbit(c))dense_values_[c]=plus(dense_values_[c],delta);
      return;
    }
    if constexpr(DirectReads) {
      if(root_!=absent && append_add(col,delta))return;
      // 占有済みセルの祖先もすべて占有済み。
      if(dense_used(col)) {
        for(U c=col;c<=dense_width_;c+=lowbit(c))dense_values_[c]=plus(dense_values_[c],delta);
        return;
      }
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
    if constexpr(DirectReads)if(root_!=absent)return static_cast<T>(minus(append_prefix(mr),append_prefix(ml)));
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
  void promote_prefix(U m,U prefix=0) noexcept {
    maybe_dense<true>(m);
    if constexpr(DirectReads)if(prefix && prefix<m && direct_dense_read() && !dense_values_.empty()) {
      // 重複する下位の部分和を除き、線形構築の途中状態へ揃える。
      V previous{};
      for(U x=prefix+lowbit(prefix);x<=m;x+=lowbit(x)) {
        const V partial=dense_values_[x];
        dense_values_[x]=minus(partial,previous);previous=partial;
      }
      root_=prefix;cells_=prefix;dense_clean_=false;
    }
  }
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
  std::size_t size() const noexcept {
    if constexpr(DirectReads)if(root_!=absent && !dense_values_.empty()) {
      std::size_t count=root_;for(U x=root_+lowbit(root_);x<=dense_width_;x+=lowbit(x))++count;
      return count;
    }
    return cells_;
  }
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
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline bool try_dense_add(U col,const T& value) noexcept {
    if(dense_values_.empty())return false;
    dense_add(col,static_cast<V>(value));return true;
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
      if constexpr(DirectReads)if(root_!=absent)return append_bound<Upper>(remaining,m);
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
    if constexpr(DirectReads)if(root_!=absent) {
      if(col>root_)return T{};
      V result=dense_values_[col];
      for(U x=col-1;x!=common;x-=lowbit(x))result=minus(result,dense_values_[x]);
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
    if constexpr(DirectReads)if(root_!=absent)return static_cast<T>(append_prefix(endpoint));
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

#ifndef SIRKIT_DYNAMIC_FENWICK_SMALL_ROW_DEFINED
#define SIRKIT_DYNAMIC_FENWICK_SMALL_ROW_DEFINED
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
  void promote_prefix(U m,U prefix=0) noexcept {if(auto* tree=std::get_if<2>(&storage_))tree->promote_prefix(m,prefix);}
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
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline void add_direct(U col,const T& value,U m) {
    if constexpr(UseTree && DirectReads)if(auto* tree=std::get_if<2>(&storage_))if(tree->try_dense_add(col,value))return;
    add_mode<UseTree>(col,value,m);
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
#endif  // SIRKIT_DYNAMIC_FENWICK_SMALL_ROW_DEFINED


#ifndef SIRKIT_DYNAMIC_FENWICK_TREE_DEFINED
#define SIRKIT_DYNAMIC_FENWICK_TREE_DEFINED
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
  FenwickFallback& operator=(const FenwickFallback& other) {
    if (this == &other) return *this;
    try {
      data_ = other.data_; sparse_ = other.sparse_;
      small_ = other.small_; dense_used_ = other.dense_used_;
      small_used_ = other.small_used_; n_ = other.n_;
    } catch (...) {
      // コピー失敗時は元の長さの零状態へ戻し、確保済み容量は再利用する。
      clear();
      throw;
    }
    return *this;
  }
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
      if(n_<=64) {
        // 登録済みセルの祖先も登録済みなので、ビットの書き戻しを省く。
        if(tiny->used&(UINT64_C(1)<<i))
          for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c))tiny->values[c]+=value;
        else for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c)){tiny->values[c]+=value;tiny->used|=UINT64_C(1)<<(c-1);}
      }
      else if(tiny->used==n_)for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c))tiny->values[c]+=value;
      else for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c)) {
        tiny->values[c]+=value;if(!tiny->flags[c]){tiny->flags[c]=1;++tiny->used;}
      }
      return;
    }
    if(auto* row=std::get_if<0>(&storage_)){
      if(n_<8192)row->template add_direct<false>(static_cast<std::uint32_t>(i)+1,value,static_cast<std::uint32_t>(n_));
      else row->template add_direct<true>(static_cast<std::uint32_t>(i)+1,value,static_cast<std::uint32_t>(n_));
      if(n_<8192 && !dense_failed_ && row->size()>=(static_cast<std::size_t>(n_)+7)/8) {
        try {Tiny next;next.used=row->size();row->export_cells(static_cast<std::uint32_t>(n_),next.values,next.flags);storage_.template emplace<1>(std::move(next));return;}
        catch(const std::bad_alloc&){dense_failed_=true;}
      }
      if(prefix_!=std::numeric_limits<U>::max()) {
        if(i!=prefix_)prefix_=std::numeric_limits<U>::max();
        else if(++prefix_>128 && row->size()>=(static_cast<std::size_t>(n_)+31)/32 && n_<=1'000'000){
          row->promote_prefix(static_cast<std::uint32_t>(n_),static_cast<std::uint32_t>(prefix_));prefix_=std::numeric_limits<U>::max();
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
        using Value=std::make_unsigned_t<T>;using Arithmetic=decltype(Value{}+0u);Value row_result{};
        auto c=end;
        while(c) {
          row_result=static_cast<Value>(static_cast<Arithmetic>(row_result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
          row_result=static_cast<Value>(static_cast<Arithmetic>(row_result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
        }
        return static_cast<T>(row_result);
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
      using Value=std::make_unsigned_t<T>;using Arithmetic=decltype(Value{}+0u);
      if(l==0) {
        Value result{};auto c=static_cast<std::uint32_t>(r);
        while(c) {
          result=static_cast<Value>(static_cast<Arithmetic>(result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
          result=static_cast<Value>(static_cast<Arithmetic>(result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
        }
        return static_cast<T>(result);
      }
      Value left_sum{},right_sum{};
      auto left=static_cast<std::uint32_t>(l),right=static_cast<std::uint32_t>(r);
      while(left|right){
        right_sum=static_cast<Value>(static_cast<Arithmetic>(right_sum)+static_cast<Arithmetic>(values[right]));
        left_sum=static_cast<Value>(static_cast<Arithmetic>(left_sum)+static_cast<Arithmetic>(values[left]));
        right-=right&(std::uint32_t{0}-right);left-=left&(std::uint32_t{0}-left);
        right_sum=static_cast<Value>(static_cast<Arithmetic>(right_sum)+static_cast<Arithmetic>(values[right]));
        left_sum=static_cast<Value>(static_cast<Arithmetic>(left_sum)+static_cast<Arithmetic>(values[left]));
        right-=right&(std::uint32_t{0}-right);left-=left&(std::uint32_t{0}-left);
      }
      return static_cast<T>(static_cast<Value>(static_cast<Arithmetic>(right_sum)-static_cast<Arithmetic>(left_sum)));
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
template<typename S,typename T>
class DynamicFenwickTree : public reinsirk::detail::FenwickBase<S,T> {
 using Base=reinsirk::detail::FenwickBase<S,T>;
 public: using Base::Base;
};
#endif


// Library: DataStructure/dynamic-binary-indexed-tree-2d.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/data-structure-2d/dynamic-binary-indexed-tree-2d.hpp
// Original Source (InternalHashMap): https://nyaannyaan.github.io/library/data-structure/hash-map-variable-length.hpp.html
// Original Source (mix): https://prng.di.unimi.it/splitmix64.c
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/dynamic_fenwick_2d.md
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/dynamic_fenwick.md
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/internal_hash_map.md
//
// API一覧
// [DynamicFenwickTree2D<T>]
// 0始まり・半開区間。Tの加減算の表現範囲は利用側の責任。
// 構築O(1)。通常の更新・矩形和は期待O(log n log m)、読み取りは領域を増やさない。
// 初回更新の索引準備はn<=100万でO(n)、新しいdense行の準備はO(m)。
// m<=64、またはm<=2048かつn*m<=2^22では必要行をdense化する。
// 整数型は32列まで点列（4列まで行内）。m<8192は以後セル表、m>=8192は128列まで点列、以後B+木。
// B+木はm<=100万かつ更新セルがm/5以上になるとO(m)で直接配列へ移す。
// DynamicFenwickTree2D(n=0,m=0): n行m列の零配列を作る。n,m>=0。
// add(i,j,x): 位置(i,j)へxを加算する。0<=i<rows(), 0<=j<cols()。
// sum(n,m): [0,n)×[0,m)の和。各端点は0以上、行数・列数以下。
// sum(nl,ml,nr,mr): [nl,nr)×[ml,mr)の和。各軸で左端<=右端。
// get(i,j): 位置(i,j)の値を返す。読み取りではノードを増やさない。
// set(i,j,x): 位置(i,j)の値をxへ変更する。
// sum_all(): 配列全体の和を返す。
// rows()/cols()/empty(): 行数／列数／いずれかの次元が0かを返す。
// clear(): 行数・列数と確保済み容量を維持して、全要素を0にする。
// reset(n,m): n行m列の零配列へ変更する。確保済み容量を再利用する。
// reserve(nodes): 内部セル数の目安から行pool・疎索引を予約するヒント。行内容量は必要時確保。
// stored_nodes(): 更新が到達したFenwickセル数。保持形式によらず、値が0になったセルも含む。
// swap(other): 寸法・値・確保済み領域を交換する。
//
// [同梱: DynamicFenwickTree<S,T>]
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

#ifndef SIRKIT_INTERNAL_HASH_MAP_DEFINED
#define SIRKIT_INTERNAL_HASH_MAP_DEFINED
namespace reinsirk::detail {
template <class Key, class Val, bool Packed = false, unsigned Denominator = 5>
class InternalHashMap {
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
        if (size_ != 0) {
            for (std::size_t i = 0; i < used_.size(); ++i) {
                if (!used_[i]) continue;
                std::size_t p = static_cast<std::size_t>((static_cast<std::uint64_t>(keys_[i]) * salt_) >> next_shift);
                while (used[p]) p = (p + 1) & (capacity - 1);
                keys[p] = keys_[i];
                values[p].value = values_[i].value;
                used[p] = 1;
            }
        }
        keys_.swap(keys);
        values_.swap(values);
        used_.swap(used);
        shift_ = next_shift;
    }

public:
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

    void reallocate() {
        if (keys_.empty()) {
            rehash(8);
        } else {
            if (keys_.size() > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            rehash(keys_.size() * 2);
        }
    }

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

    template <class F> Val& find_or_emplace(Key key, F&& factory) {
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
        values_[p].value = std::forward<F>(factory)();
        keys_[p] = key;
        used_[p] = 1;
        ++size_;
        return values_[p].value;
    }

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

    std::vector<std::pair<Key, Val>> enumerate() const {
        std::vector<std::pair<Key, Val>> result;
        if (empty()) return result;
        result.reserve(size_);
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) result.emplace_back(keys_[i], values_[i].value);
        return result;
    }

    template <class F> void for_each(F&& f) const {
        if (empty()) return;
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) f(keys_[i], values_[i].value);
    }

    [[nodiscard]] bool contains(Key key) const noexcept { return find(key) != nullptr; }
    [[nodiscard]] bool contain(Key key) const noexcept { return contains(key); }
    [[nodiscard]] int count(Key key) const noexcept { return contains(key) ? 1 : 0; }
    void clear() noexcept {
        if (empty()) return;
        std::fill(used_.begin(), used_.end(), Flag{0});
        size_ = 0;
    }
    void release() noexcept {
        decltype(keys_){}.swap(keys_);
        decltype(values_){}.swap(values_);
        decltype(used_){}.swap(used_);
        size_ = 0;
        shift_ = 61;
    }
    std::size_t capacity() const noexcept { return keys_.size(); }
    double load_factor() const noexcept {
        return capacity() == 0 ? 0.0 : static_cast<double>(size_) / capacity();
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    void set_default(const Val& value) { default_value_ = value; }
};

}  // namespace reinsirk::detail
#endif  // SIRKIT_INTERNAL_HASH_MAP_DEFINED




#include <array>
#include <memory>
#include <variant>
#ifndef SIRKIT_DYNAMIC_FENWICK_BPLUS_ROW_DEFINED
#define SIRKIT_DYNAMIC_FENWICK_BPLUS_ROW_DEFINED
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
  // dense中のroot_は、連続追加中だけ完了済みprefix長を保持する。
  V append_prefix(U endpoint) const noexcept {
    V result{};for(U x=std::min(endpoint,root_);x;x-=lowbit(x))result=plus(result,dense_values_[x]);
    return result;
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::noinline]]
#endif
  void finish_append() noexcept {
    const U prefix=root_;
    std::fill_n(dense_flags_.begin(),prefix/64,~std::uint64_t{0});
    if(prefix%64)dense_flags_[prefix/64]=(std::uint64_t{1}<<(prefix%64))-1;
    cells_=prefix;
    U cell=prefix;
    for(U next=cell+lowbit(cell);next<=dense_width_;next=cell+lowbit(cell)) {
      V value=dense_values_[cell];const U stop=next-lowbit(next);
      for(U x=cell-lowbit(cell);x>stop;x-=lowbit(x))value=plus(value,dense_values_[x]);
      dense_values_[next]=value;cell=next;
      dense_flags_[(cell-1)/64]|=std::uint64_t{1}<<((cell-1)%64);++cells_;
    }
    root_=absent;if constexpr(DirectReads)dense_clean_=true;
  }
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::noinline]]
#endif
  bool append_add(U col,V delta) noexcept {
    if(col!=root_+1){finish_append();return false;}
    // 完了したセルの和を親1個だけへ伝える。
    const V value=plus(dense_values_[col],delta);
    dense_values_[col]=value;
    const U parent=col+lowbit(col);
    if(parent<=dense_width_)dense_values_[parent]=plus(dense_values_[parent],value);
    root_=col;
    if(root_==dense_width_)finish_append();
    return true;
  }
  template<bool Upper>U append_bound(V w,U m)const noexcept {
    if(Upper?append_prefix(root_)<=w:append_prefix(root_)<w)return m;
    U x=0;for(U step=std::bit_floor(root_);step;step>>=1) {
      const U next=x+step;if(next>root_)continue;
      const V cell=dense_values_[next];
      if(Upper?cell<=w:cell<w){w=minus(w,cell);x=next;}
    }
    return x;
  }
  void dense_add(U col,V delta) noexcept {
    if(cells_==dense_width_) {
      for(U c=col;c<=dense_width_;c+=lowbit(c))dense_values_[c]=plus(dense_values_[c],delta);
      return;
    }
    if constexpr(DirectReads) {
      if(root_!=absent && append_add(col,delta))return;
      // 占有済みセルの祖先もすべて占有済み。
      if(dense_used(col)) {
        for(U c=col;c<=dense_width_;c+=lowbit(c))dense_values_[c]=plus(dense_values_[c],delta);
        return;
      }
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
    if constexpr(DirectReads)if(root_!=absent)return static_cast<T>(minus(append_prefix(mr),append_prefix(ml)));
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
  void promote_prefix(U m,U prefix=0) noexcept {
    maybe_dense<true>(m);
    if constexpr(DirectReads)if(prefix && prefix<m && direct_dense_read() && !dense_values_.empty()) {
      // 重複する下位の部分和を除き、線形構築の途中状態へ揃える。
      V previous{};
      for(U x=prefix+lowbit(prefix);x<=m;x+=lowbit(x)) {
        const V partial=dense_values_[x];
        dense_values_[x]=minus(partial,previous);previous=partial;
      }
      root_=prefix;cells_=prefix;dense_clean_=false;
    }
  }
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
  std::size_t size() const noexcept {
    if constexpr(DirectReads)if(root_!=absent && !dense_values_.empty()) {
      std::size_t count=root_;for(U x=root_+lowbit(root_);x<=dense_width_;x+=lowbit(x))++count;
      return count;
    }
    return cells_;
  }
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
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline bool try_dense_add(U col,const T& value) noexcept {
    if(dense_values_.empty())return false;
    dense_add(col,static_cast<V>(value));return true;
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
      if constexpr(DirectReads)if(root_!=absent)return append_bound<Upper>(remaining,m);
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
    if constexpr(DirectReads)if(root_!=absent) {
      if(col>root_)return T{};
      V result=dense_values_[col];
      for(U x=col-1;x!=common;x-=lowbit(x))result=minus(result,dense_values_[x]);
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
    if constexpr(DirectReads)if(root_!=absent)return static_cast<T>(append_prefix(endpoint));
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

#ifndef SIRKIT_DYNAMIC_FENWICK_SMALL_ROW_DEFINED
#define SIRKIT_DYNAMIC_FENWICK_SMALL_ROW_DEFINED
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
  void promote_prefix(U m,U prefix=0) noexcept {if(auto* tree=std::get_if<2>(&storage_))tree->promote_prefix(m,prefix);}
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
#if defined(__GNUC__) || defined(__clang__)
  [[gnu::always_inline]]
#endif
  inline void add_direct(U col,const T& value,U m) {
    if constexpr(UseTree && DirectReads)if(auto* tree=std::get_if<2>(&storage_))if(tree->try_dense_add(col,value))return;
    add_mode<UseTree>(col,value,m);
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
#endif  // SIRKIT_DYNAMIC_FENWICK_SMALL_ROW_DEFINED


#ifndef SIRKIT_DYNAMIC_FENWICK_TREE_DEFINED
#define SIRKIT_DYNAMIC_FENWICK_TREE_DEFINED
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
  FenwickFallback& operator=(const FenwickFallback& other) {
    if (this == &other) return *this;
    try {
      data_ = other.data_; sparse_ = other.sparse_;
      small_ = other.small_; dense_used_ = other.dense_used_;
      small_used_ = other.small_used_; n_ = other.n_;
    } catch (...) {
      // コピー失敗時は元の長さの零状態へ戻し、確保済み容量は再利用する。
      clear();
      throw;
    }
    return *this;
  }
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
      if(n_<=64) {
        // 登録済みセルの祖先も登録済みなので、ビットの書き戻しを省く。
        if(tiny->used&(UINT64_C(1)<<i))
          for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c))tiny->values[c]+=value;
        else for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c)){tiny->values[c]+=value;tiny->used|=UINT64_C(1)<<(c-1);}
      }
      else if(tiny->used==n_)for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c))tiny->values[c]+=value;
      else for(U c=i+1;static_cast<U>(c-U{1})<n_;c+=lowbit(c)) {
        tiny->values[c]+=value;if(!tiny->flags[c]){tiny->flags[c]=1;++tiny->used;}
      }
      return;
    }
    if(auto* row=std::get_if<0>(&storage_)){
      if(n_<8192)row->template add_direct<false>(static_cast<std::uint32_t>(i)+1,value,static_cast<std::uint32_t>(n_));
      else row->template add_direct<true>(static_cast<std::uint32_t>(i)+1,value,static_cast<std::uint32_t>(n_));
      if(n_<8192 && !dense_failed_ && row->size()>=(static_cast<std::size_t>(n_)+7)/8) {
        try {Tiny next;next.used=row->size();row->export_cells(static_cast<std::uint32_t>(n_),next.values,next.flags);storage_.template emplace<1>(std::move(next));return;}
        catch(const std::bad_alloc&){dense_failed_=true;}
      }
      if(prefix_!=std::numeric_limits<U>::max()) {
        if(i!=prefix_)prefix_=std::numeric_limits<U>::max();
        else if(++prefix_>128 && row->size()>=(static_cast<std::size_t>(n_)+31)/32 && n_<=1'000'000){
          row->promote_prefix(static_cast<std::uint32_t>(n_),static_cast<std::uint32_t>(prefix_));prefix_=std::numeric_limits<U>::max();
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
        using Value=std::make_unsigned_t<T>;using Arithmetic=decltype(Value{}+0u);Value row_result{};
        auto c=end;
        while(c) {
          row_result=static_cast<Value>(static_cast<Arithmetic>(row_result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
          row_result=static_cast<Value>(static_cast<Arithmetic>(row_result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
        }
        return static_cast<T>(row_result);
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
      using Value=std::make_unsigned_t<T>;using Arithmetic=decltype(Value{}+0u);
      if(l==0) {
        Value result{};auto c=static_cast<std::uint32_t>(r);
        while(c) {
          result=static_cast<Value>(static_cast<Arithmetic>(result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
          result=static_cast<Value>(static_cast<Arithmetic>(result)+static_cast<Arithmetic>(values[c]));c-=c&(std::uint32_t{0}-c);
        }
        return static_cast<T>(result);
      }
      Value left_sum{},right_sum{};
      auto left=static_cast<std::uint32_t>(l),right=static_cast<std::uint32_t>(r);
      while(left|right){
        right_sum=static_cast<Value>(static_cast<Arithmetic>(right_sum)+static_cast<Arithmetic>(values[right]));
        left_sum=static_cast<Value>(static_cast<Arithmetic>(left_sum)+static_cast<Arithmetic>(values[left]));
        right-=right&(std::uint32_t{0}-right);left-=left&(std::uint32_t{0}-left);
        right_sum=static_cast<Value>(static_cast<Arithmetic>(right_sum)+static_cast<Arithmetic>(values[right]));
        left_sum=static_cast<Value>(static_cast<Arithmetic>(left_sum)+static_cast<Arithmetic>(values[left]));
        right-=right&(std::uint32_t{0}-right);left-=left&(std::uint32_t{0}-left);
      }
      return static_cast<T>(static_cast<Value>(static_cast<Arithmetic>(right_sum)-static_cast<Arithmetic>(left_sum)));
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
template<typename S,typename T>
class DynamicFenwickTree : public reinsirk::detail::FenwickBase<S,T> {
 using Base=reinsirk::detail::FenwickBase<S,T>;
 public: using Base::Base;
};
#endif

#ifndef SIRKIT_DYNAMIC_FENWICK_TREE_2D_DEFINED
#define SIRKIT_DYNAMIC_FENWICK_TREE_2D_DEFINED
template <typename T>
class DynamicFenwickTree2D {
  static_assert(std::numeric_limits<int>::digits <= 31,
                "DynamicFenwickTree2D: int must have at most 31 value bits");
  using U = std::uint32_t;
  static constexpr bool small_rows = std::is_integral_v<T> && !std::is_same_v<T,bool>;
  using WideRow = std::conditional_t<small_rows, reinsirk::detail::SmallFenwickRow<T>,
                                   reinsirk::detail::InternalHashMap<U,T,false,4>>;
  struct DenseRow {
    std::vector<T> values;
    std::uint64_t used = 0;
    std::vector<std::uint64_t> flags;
    void reset(U m) {
      values.assign(m, T{});
      used = 0;
      if (m > 64) flags.assign((m + 63) / 64, 0);
      else flags.clear();
    }
  };
  U n_ = 0, m_ = 0;
  bool dense_ = true, direct_ = true, tree_ = false;
  std::vector<U> row_indices_;
  reinsirk::detail::InternalHashMap<U,U,false,4> row_map_;
  std::vector<WideRow> wide_rows_;
  std::vector<DenseRow> dense_rows_;
  std::size_t active_ = 0, nodes_ = 0;

  static constexpr U lowbit(U x) noexcept { return x & (U{0} - x); }
  void dimensions(int n, int m) noexcept {
    n_ = static_cast<U>(n); m_ = static_cast<U>(m);
    dense_ = m_ <= 64 || (m_ <= 2048 && std::uint64_t{n_} * m_ <= (std::uint64_t{1} << 22));
    direct_ = n_ <= 1'000'000;
    if constexpr (small_rows) tree_ = WideRow::uses_tree(m_);
  }
  static void check_size(int n, int m) {
#ifndef SKIP_RUNTIME_CHECKS
    if (n < 0 || m < 0) throw std::invalid_argument("DynamicFenwickTree2D: negative size");
#else
    (void)n; (void)m;
#endif
  }
  void check_position(int i, int j) const {
#ifndef SKIP_RUNTIME_CHECKS
    if (static_cast<U>(i) >= n_ || static_cast<U>(j) >= m_)
      throw std::out_of_range("DynamicFenwickTree2D: position out of range");
#else
    (void)i; (void)j;
#endif
  }
  void check_endpoint(int i, int j) const {
#ifndef SKIP_RUNTIME_CHECKS
    if (static_cast<U>(i) > n_ || static_cast<U>(j) > m_)
      throw std::out_of_range("DynamicFenwickTree2D: endpoint out of range");
#else
    (void)i; (void)j;
#endif
  }
  void add_nodes(std::size_t delta) {
    if (delta > std::numeric_limits<std::size_t>::max() - nodes_)
      throw std::length_error("DynamicFenwickTree2D: node count overflow");
    nodes_ += delta;
  }
  template <bool Dense, bool Direct, bool Tree = false>
  void update(U row, U col, const T& x) {
    if constexpr (Direct) {
      if (row_indices_.empty()) row_indices_.resize(std::size_t{n_} + 1);
      // 容量だけ確保し、各行の値やハッシュ表は最初の更新時に作る。
      if constexpr (Dense) {
        if (dense_rows_.capacity() == 0) dense_rows_.reserve(n_);
      } else {
        if (wide_rows_.capacity() == 0) wide_rows_.reserve(n_);
      }
    }
    // 有効な寸法はINT_MAX以下。k+lowbit(k)<=2^31なのでuint32_tの桁あふれはない。
    for (; row <= n_; row += lowbit(row)) {
      U& index = [&]() -> U& {
        if constexpr (Direct) return row_indices_[row];
        else return row_map_[row];
      }();
      if (index == 0) {
        if (active_ >= std::numeric_limits<U>::max())
          throw std::length_error("DynamicFenwickTree2D: row count overflow");
        if constexpr (Dense) {
          if (active_ == dense_rows_.size()) dense_rows_.emplace_back();
          dense_rows_[active_].reset(m_);
        } else {
          if (active_ == wide_rows_.size()) wide_rows_.emplace_back();
          else {
            if constexpr (small_rows) wide_rows_[active_].clear(m_);
            else wide_rows_[active_].clear();
          }
        }
        index = static_cast<U>(++active_);
      }
      if constexpr (Dense) {
        DenseRow& line = dense_rows_[index - 1];
        if (m_ <= 64) {
          std::uint64_t used = 0;
          for (U c = col; c <= m_; c += lowbit(c)) {
            line.values[c - 1] += x;
            used |= std::uint64_t{1} << (c - 1);
          }
          const std::uint64_t extra = used & ~line.used;
          if (extra) add_nodes(std::popcount(extra));
          line.used |= used;
        } else {
          std::size_t extra = 0;
          for (U c = col; c <= m_; c += lowbit(c)) {
            const U k = c - 1;
            std::uint64_t& flags = line.flags[k / 64];
            const std::uint64_t bit = std::uint64_t{1} << (k % 64);
            if ((flags & bit) == 0) { flags |= bit; ++extra; }
            line.values[k] += x;
          }
          add_nodes(extra);
        }
      } else {
        auto& line = wide_rows_[index - 1];
        const std::size_t before = [&] {
          if constexpr (small_rows) return line.template size_mode<Tree>();
          else return line.size();
        }();
        if constexpr (small_rows) {
          if constexpr (Tree) line.template add_mode<true>(col,x,m_);
          else {
            if (auto* map = line.fenwick_map())
              for (U c=col;c<=m_;c+=lowbit(c)) (*map)[c]+=x;
            else line.template add_mode<false>(col,x,m_);
          }
        } else for (U c=col;c<=m_;c+=lowbit(c)) line[c]+=x;
        const std::size_t after = [&] {
          if constexpr (small_rows) return line.template size_mode<Tree>();
          else return line.size();
        }();
        add_nodes(after-before);
      }
    }
  }
  template <bool Dense, bool Direct, bool Tree = false>
  T rectangle(U nl, U ml, U nr, U mr) const {
    T result{};
    if (nl == nr || ml == mr) return result;
    if constexpr (Direct) if (row_indices_.empty()) return result;
    const U col_common = (ml & mr) & (U{0} - std::bit_floor(ml ^ mr));
    const U row_common = (nl & nr) & (U{0} - std::bit_floor(nl ^ nr));
    auto row = [&](U i) {
      T value{};
      const U index = [&]() {
        if constexpr (Direct) return row_indices_[i];
        else {
          const U* p = row_map_.find(i);
          return p ? *p : U{0};
        }
      }();
      if (index == 0) return value;
      if constexpr (Dense) {
        const auto& values = dense_rows_[index - 1].values;
        for (U j = mr; j != col_common; j -= lowbit(j)) value += values[j - 1];
        for (U j = ml; j != col_common; j -= lowbit(j)) value -= values[j - 1];
      } else {
        const auto& line = wide_rows_[index - 1];
        if constexpr (small_rows) {
          if constexpr (Tree) value=line.template sum_mode<true>(ml,mr,col_common);
          else {
            if (const auto* map=line.fenwick_map()) {
              for (U j=mr;j!=col_common;j-=lowbit(j))
                if (const T* p=map->find_nonempty(j)) value+=*p;
              for (U j=ml;j!=col_common;j-=lowbit(j))
                if (const T* p=map->find_nonempty(j)) value-=*p;
            } else value=line.template sum_mode<false>(ml,mr,col_common);
          }
        } else {
          for (U j = mr; j != col_common; j -= lowbit(j))
            if (const T* p = line.find(j)) value += *p;
          for (U j = ml; j != col_common; j -= lowbit(j))
            if (const T* p = line.find(j)) value -= *p;
        }
      }
      return value;
    };
    for (; nr != row_common; nr -= lowbit(nr)) result += row(nr);
    for (; nl != row_common; nl -= lowbit(nl)) result -= row(nl);
    return result;
  }
  T rectangle(U nl, U ml, U nr, U mr) const {
    if (dense_) {
      if (direct_) return rectangle<true,true>(nl, ml, nr, mr);
      return rectangle<true,false>(nl, ml, nr, mr);
    }
    if constexpr (small_rows) if (tree_) {
      if (direct_) return rectangle<false,true,true>(nl,ml,nr,mr);
      return rectangle<false,false,true>(nl,ml,nr,mr);
    }
    if (direct_) return rectangle<false,true,false>(nl,ml,nr,mr);
    return rectangle<false,false,false>(nl,ml,nr,mr);
  }
 public:
  explicit DynamicFenwickTree2D(int n = 0, int m = 0) {
    check_size(n, m); dimensions(n, m);
  }
  DynamicFenwickTree2D(const DynamicFenwickTree2D&) = default;
  DynamicFenwickTree2D& operator=(const DynamicFenwickTree2D& other) {
    if (this != &other) { DynamicFenwickTree2D copy(other); swap(copy); }
    return *this;
  }
  DynamicFenwickTree2D(DynamicFenwickTree2D&& other) noexcept : DynamicFenwickTree2D() { swap(other); }
  DynamicFenwickTree2D& operator=(DynamicFenwickTree2D&& other) noexcept {
    if (this != &other) { DynamicFenwickTree2D moved(std::move(other)); swap(moved); }
    return *this;
  }
  void swap(DynamicFenwickTree2D& other) noexcept {
    using std::swap;
    swap(n_, other.n_); swap(m_, other.m_);
    swap(dense_, other.dense_); swap(direct_, other.direct_); swap(tree_,other.tree_);
    row_indices_.swap(other.row_indices_); row_map_.swap(other.row_map_);
    wide_rows_.swap(other.wide_rows_); dense_rows_.swap(other.dense_rows_);
    swap(active_, other.active_); swap(nodes_, other.nodes_);
  }
  int rows() const noexcept { return static_cast<int>(n_); }
  int cols() const noexcept { return static_cast<int>(m_); }
  bool empty() const noexcept { return n_ == 0 || m_ == 0; }
  std::size_t stored_nodes() const noexcept { return nodes_; }
  void reserve(std::size_t nodes) {
    const std::size_t rows = std::min(nodes, std::size_t{n_});
    if (!direct_) row_map_.reserve(rows);
    if (dense_) dense_rows_.reserve(rows);
    else wide_rows_.reserve(rows);
  }
  void clear() {
    std::fill(row_indices_.begin(), row_indices_.end(), U{0});
    row_map_.clear(); active_ = 0; nodes_ = 0;
  }
  void reset(int n, int m) {
    check_size(n, m); clear(); row_indices_.clear(); dimensions(n, m);
  }
  void add(int i, int j, const T& x) {
    check_position(i, j);
    U row = static_cast<U>(i), col = static_cast<U>(j);
    // 検査省略時も、1始まりへの変換で0へ巻き戻さない。
    if (row == std::numeric_limits<U>::max() || col == std::numeric_limits<U>::max()) return;
    ++row; ++col;
    if (dense_) {
      if (direct_) update<true,true>(row, col, x);
      else update<true,false>(row, col, x);
    } else {
      if constexpr (small_rows) if (tree_) {
        if (direct_) update<false,true,true>(row,col,x);
        else update<false,false,true>(row,col,x);
        return;
      }
      if (direct_) update<false,true,false>(row,col,x);
      else update<false,false,false>(row,col,x);
    }
  }
  T sum(int n, int m) const {
    check_endpoint(n, m);
    return rectangle(0, 0, static_cast<U>(n), static_cast<U>(m));
  }
  T sum(int nl, int ml, int nr, int mr) const {
    check_endpoint(nl, ml); check_endpoint(nr, mr);
#ifndef SKIP_RUNTIME_CHECKS
    if (nl > nr || ml > mr) throw std::invalid_argument("DynamicFenwickTree2D: reversed rectangle");
#endif
    return rectangle(static_cast<U>(nl), static_cast<U>(ml), static_cast<U>(nr), static_cast<U>(mr));
  }
  T get(int i, int j) const {
    check_position(i, j);
    const U row = static_cast<U>(i), col = static_cast<U>(j);
    if (row == std::numeric_limits<U>::max() || col == std::numeric_limits<U>::max()) return T{};
    return rectangle(row, col, row + U{1}, col + U{1});
  }
  void set(int i, int j, const T& x) { add(i, j, x - get(i, j)); }
  T sum_all() const { return rectangle(0, 0, n_, m_); }
};
#endif


// Library: DataStructure/dynamic-li-chao-tree.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/segment-tree/dynamic-li-chao-tree.hpp
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/dynamic_li_chao.md
//
// API一覧
// [DynamicLiChaoTree<T,INF>]
// 整数座標の閉区間で直線・線分の最小値を管理する。Tは符号付き64ビット以下の整数。
// 追加する線の対象区間では、数学的なa*x+bがTの範囲内かつINF未満であること。
// Wを座標数とすると直線追加・取得はO(log W)、線分追加はO(log² W)。
// queryは読み取り専用。SKIP_RUNTIME_CHECKSで座標・端点値の検査を除く。
// DynamicLiChaoTree(xmin, xmax): 整数の閉区間[xmin, xmax]で空の木を作る。
// コピーは独立した木を作る。コピー代入の確保失敗時は元の木を維持する。
// update(a, b) / add_line(a, b): 定義域全体にy=a*x+bを追加する。
// update_line_segment(a, b, left, right) / add_segment(...): 閉区間[left, right]に線分を追加。left>rightは無操作。
// query(x): xでの最小値を返す。被覆する線がなければINF。
// query_line(x): 最小値を与えるLineを返す。同値時は任意、非被覆ならnullopt。
// empty(): 追加した直線・空でない線分がなければtrue。O(1)。
// clear(): 全ての線を消し、確保した容量を再利用する。O(格納ノード数)。
// reserve(n): nノード分を事前確保する。直線本数とは異なる。
// stored_nodes(): 現在の格納ノード数を返す。取得操作では増えない。O(1)。
// domain_min() / domain_max(): 定義域の左端・右端を返す。O(1)。
//
// [DynamicLiChaoTree<T,INF>::Line]
// Line(a, b): y=a*x+b。既定値は空を表す定数INF。
// get(x): a*x+bを返す。値がTで表現可能であること。
// over(other, x): xでotherより小さい値を与えるときtrue。

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_DYNAMIC_LI_CHAO_TREE_DEFINED
#define SIRKIT_DYNAMIC_LI_CHAO_TREE_DEFINED
template <typename T, T INF>
class DynamicLiChaoTree {
 public:
  struct Line {
    T slope = 0;
    T intercept = INF;
    Line() = default;
    Line(T a, T b) : slope(a), intercept(b) {}
    T get(T x) const noexcept {
      using U = std::make_unsigned_t<T>;
      // 中間積がTを超えても、最終値が表現可能なら正確に復元できる。
      const U value = static_cast<U>(std::uint64_t(static_cast<U>(slope)) *
                                         std::uint64_t(static_cast<U>(x)) +
                                     std::uint64_t(static_cast<U>(intercept)));
      return std::bit_cast<T>(value);
    }
    bool over(const Line& other, T x) const noexcept { return get(x) < other.get(x); }
  };

 private:
  static_assert(std::is_integral_v<T> && std::is_signed_v<T> &&
                sizeof(T) <= sizeof(std::uint64_t) &&
                std::is_same_v<T, std::remove_cv_t<T>>);
  static_assert(INF > 0);
  using Index = std::uint32_t;
  static constexpr Index no_child = std::numeric_limits<Index>::max();
  struct Node {
    Line line;
    Index left = no_child;
    Index right = no_child;
  };
  T xmin_, xmax_;
#if defined(__GNUC__) && !defined(__clang__)
  class NodeStorage {
    static_assert(std::is_trivially_copyable_v<Node> &&
                  std::is_trivially_destructible_v<Node>);
    static_assert(alignof(Node) <= alignof(std::max_align_t));
    struct Release {
      void operator()(Node* pointer) const noexcept { std::free(pointer); }
    };
    std::unique_ptr<Node, Release> owner_;
    std::size_t size_ = 0, capacity_ = 0;

   public:
    NodeStorage() noexcept = default;
    NodeStorage(const NodeStorage& other) { *this = other; }
    NodeStorage& operator=(const NodeStorage& other) {
      if (this != &other) {
        if (other.size_ > capacity_) {
          void* next = std::realloc(nullptr, other.size_ * sizeof(Node));
          if (!next) throw std::bad_alloc();
          std::memcpy(next, other.owner_.get(), other.size_ * sizeof(Node));
          owner_.reset(static_cast<Node*>(next));
          capacity_ = other.size_;
        } else if (other.size_) {
          std::memcpy(owner_.get(), other.owner_.get(), other.size_ * sizeof(Node));
        }
        size_ = other.size_;
      }
      return *this;
    }
    NodeStorage(NodeStorage&& other) noexcept
        : owner_(std::move(other.owner_)), size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)) {}
    NodeStorage& operator=(NodeStorage&& other) noexcept {
      if (this != &other) {
        owner_ = std::move(other.owner_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
      }
      return *this;
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    std::size_t max_size() const noexcept { return std::vector<Node>{}.max_size(); }
    Node& operator[](std::size_t index) noexcept { return owner_.get()[index]; }
    const Node& operator[](std::size_t index) const noexcept { return owner_.get()[index]; }
    void clear() noexcept { size_ = 0; }
    void reserve(std::size_t count) {
      if (count <= capacity_) return;
      if (count > max_size()) throw std::length_error("Li Chao node limit");
      if (size_ != capacity_) {
        // 未使用容量をコピーせず、有効なノードだけを移す。
        void* next = std::realloc(nullptr, count * sizeof(Node));
        if (!next) throw std::bad_alloc();
        if (size_) std::memcpy(next, owner_.get(), size_ * sizeof(Node));
        owner_.reset(static_cast<Node*>(next));
      } else {
        // 満杯なら、その場で拡張できる場合に全体コピーを避ける。
        Node* previous = owner_.release();
        void* next = std::realloc(previous, count * sizeof(Node));
        if (!next) {
          owner_.reset(previous);
          throw std::bad_alloc();
        }
        owner_.reset(static_cast<Node*>(next));
      }
      capacity_ = count;
    }
    void push_back(Node value) {
      if (size_ == capacity_) {
        const std::size_t limit = max_size();
        if (size_ == limit) throw std::length_error("Li Chao node limit");
        const std::size_t next = capacity_ == 0 ? 1 :
            capacity_ > limit - capacity_ ? limit : capacity_ * 2;
        reserve(next);
      }
      std::construct_at(owner_.get() + size_, value);
      ++size_;
    }
    void emplace_back() { push_back(Node{}); }
  };
  NodeStorage nodes_;
#else
  std::vector<Node> nodes_;
#endif

#ifndef SKIP_RUNTIME_CHECKS
  void check_coordinate(T x) const {
    if (x < xmin_ || xmax_ < x) throw std::out_of_range("Li Chao coordinate");
  }
  static void check_value(const Line& line, T x) {
    using U = std::make_unsigned_t<T>;
    const U a = line.slope < 0 ? U{} - U(line.slope) : U(line.slope);
    const U v = x < 0 ? U{} - U(x) : U(x);
    // 積そのものを作らず、64ビット全域の値と相殺を含むa*x+bを検査する。
    const auto product_at_most = [a, v](U limit) {
      return a == 0 || v <= limit / a;
    };
    bool valid;
    if ((line.slope < 0) != (x < 0)) {
      valid = product_at_most(U(line.intercept) - U(std::numeric_limits<T>::min())) &&
              (line.intercept < INF || !product_at_most(U(line.intercept) - U(INF)));
    } else {
      valid = line.intercept < INF &&
              product_at_most(U(INF - 1) - U(line.intercept));
    }
    if (!valid) throw std::overflow_error("Li Chao value must fit T and be below INF");
  }
  static void check_line(const Line& line, T left, T right) {
    check_value(line, left);
    if (left != right) check_value(line, right);
  }
#endif

  static T middle_of(T left, T right) noexcept {
    using U = std::make_unsigned_t<T>;
    // left <= rightのため、符号なし差分で全整数幅の中点を求められる。
    const U distance = static_cast<U>(static_cast<U>(right) - static_cast<U>(left));
    return std::bit_cast<T>(static_cast<U>(static_cast<U>(left) + distance / 2));
  }
  Index make_node() {
    if (nodes_.size() >= no_child || nodes_.size() == nodes_.max_size())
      throw std::length_error("Li Chao node limit");
    const Index result = static_cast<Index>(nodes_.size());
    nodes_.emplace_back();
    return result;
  }
  Index child(Index index, bool right) {
    Index next = right ? nodes_[index].right : nodes_[index].left;
    if (next == no_child) {
      next = make_node();
      if (right) nodes_[index].right = next;
      else nodes_[index].left = next;
    }
    return next;
  }
  void insert_line(Index index, T left, T right, Line line) {
    for (;;) {
      Node& node = nodes_[index];
      if (node.line.intercept == INF && node.line.slope == 0) {
        node.line = line;
        return;
      }
      const bool better_left = line.over(node.line, left);
      const bool better_right = line.over(node.line, right);
      if (better_left == better_right) {
        if (better_left) node.line = line;
        return;
      }
      const T middle = middle_of(left, right);
      const bool better_middle = line.over(node.line, middle);
      if (better_middle) std::swap(node.line, line);
      if (better_left != better_middle) {
        index = child(index, false);
        right = middle;
      } else {
        index = child(index, true);
        left = static_cast<T>(middle + 1);
      }
    }
  }
  void insert_segment(Index index, T left, T right, T qleft, T qright,
                      const Line& line) {
    for (;;) {
      if (qleft <= left && right <= qright) {
        insert_line(index, left, right, line);
        return;
      }
      const Line& current = nodes_[index].line;
      // 切片INFは空ノードを含むので枝刈りを省き、通常の探索を続ける。
      if (current.intercept != INF) {
        // 差が最小になる重なり区間の端でも勝てなければ追加不要。
        const T endpoint = line.slope < current.slope
            ? (qright < right ? qright : right)
            : (qleft > left ? qleft : left);
        if (!line.over(current, endpoint)) return;
      }
      const T middle = middle_of(left, right);
      if (qright <= middle) {
        index = child(index, false);
        right = middle;
      } else if (middle < qleft) {
        index = child(index, true);
        left = static_cast<T>(middle + 1);
      } else {
        insert_segment(child(index, false), left, middle, qleft, qright, line);
        index = child(index, true);
        left = static_cast<T>(middle + 1);
      }
    }
  }
  template <bool ReturnLine>
  auto find_best(T x) const noexcept {
    Line best;
    T value = INF;
    T left = xmin_, right = xmax_;
    Index index = nodes_.empty() ? no_child : Index{0};
    while (index != no_child) {
      const Node& node = nodes_[index];
      const T candidate = node.line.get(x);
      if (candidate < value) {
        value = candidate;
        if constexpr (ReturnLine) best = node.line;
      }
      if (left == right) break;
      const T middle = middle_of(left, right);
      if (x <= middle) {
        index = node.left;
        right = middle;
      } else {
        index = node.right;
        left = static_cast<T>(middle + 1);
      }
    }
    if constexpr (ReturnLine) return best;
    else return value;
  }

 public:
  DynamicLiChaoTree(T xmin, T xmax) : xmin_(xmin), xmax_(xmax) {
#ifndef SKIP_RUNTIME_CHECKS
    if (xmax < xmin) throw std::invalid_argument("Li Chao domain");
#endif
  }
  DynamicLiChaoTree(const DynamicLiChaoTree&) = default;
  DynamicLiChaoTree(DynamicLiChaoTree&&) noexcept = default;
  DynamicLiChaoTree& operator=(const DynamicLiChaoTree& other) {
    nodes_ = other.nodes_;
    xmin_ = other.xmin_;
    xmax_ = other.xmax_;
    return *this;
  }
  DynamicLiChaoTree& operator=(DynamicLiChaoTree&&) noexcept = default;
  void update(T a, T b) {
    const Line line(a, b);
#ifndef SKIP_RUNTIME_CHECKS
    check_line(line, xmin_, xmax_);
#endif
    if (nodes_.empty()) make_node();
    insert_line(0, xmin_, xmax_, line);
  }
  void add_line(T a, T b) { update(a, b); }
  void update_line_segment(T a, T b, T left, T right) {
    if (right < left) return;
    const Line line(a, b);
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(left);
    check_coordinate(right);
    check_line(line, left, right);
#endif
    if (nodes_.empty()) make_node();
    insert_segment(0, xmin_, xmax_, left, right, line);
  }
  void add_segment(T a, T b, T left, T right) {
    update_line_segment(a, b, left, right);
  }
  T query(T x) const {
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(x);
#endif
    return find_best<false>(x);
  }
  std::optional<Line> query_line(T x) const {
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(x);
#endif
    Line result = find_best<true>(x);
    if (result.slope == 0 && result.intercept == INF) return std::nullopt;
    return result;
  }
  bool empty() const noexcept { return nodes_.empty(); }
  void clear() noexcept { nodes_.clear(); }
  void reserve(std::size_t count) {
    if (count > no_child || count > nodes_.max_size())
      throw std::length_error("Li Chao node limit");
    nodes_.reserve(count);
  }
  std::size_t stored_nodes() const noexcept { return nodes_.size(); }
  T domain_min() const noexcept { return xmin_; }
  T domain_max() const noexcept { return xmax_; }
};
#endif


// Library: DataStructure/dynamic-union-find.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/data-structure/dynamic-union-find.hpp
// Original Source (InternalHashMap): https://nyaannyaan.github.io/library/data-structure/hash-map-variable-length.hpp.html
// Original Source (mix): https://prng.di.unimi.it/splitmix64.c
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/dynamic_union_find.md
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/internal_hash_map.md
//
// API一覧
// [DynamicUnionFind]
// intの全域をキーにできる。要素は問い合わせ時に自動登録する。
// ハッシュ表の検索は期待O(1)、併合・探索は償却O(α(N))、空間O(N)。
// DynamicUnionFind(): 空で構築する。登録数はINT_MAXまで。
// DynamicUnionFind(n): n要素分の容量を予約して空で構築する。
// コピーは独立した複製。ムーブ元は空になり、再利用できる。
// data(k): 自動登録し、根なら負の要素数、非根なら親のキーを返す。
// find(k): 自動登録し、所属する連結成分の代表キーを返す。
// unite(x, y): 自動登録してサイズで併合し、併合したときtrueを返す。
// unite(x, y, f): 併合後にf(新しい代表キー, 吸収した代表キー)を1回呼ぶ。
// size(k): 自動登録し、kが所属する連結成分の要素数を返す。
// same(x, y): 自動登録し、同じ連結成分ならtrueを返す。
// add(k): 未登録のキーを単独の成分として追加し、追加したときtrueを返す。
// contains(k): キーが登録済みならtrueを返す。自動登録しない。
// node_count(): 登録済みのキーの個数をO(1)で返す。
// component_count(): 登録済みの連結成分数をO(1)で返す。
// empty(): 登録済みのキーがなければtrueを返す。
// reserve(n): n要素分を予約する。成分や登録状態は変えない。
// groups(): 全成分のキー一覧をO(N α(N))で返す。成分間の順序は未規定。
// clear(): 全登録を消去する。確保済みの容量は再利用する。
// dataの負の返値は、負の親キーとの区別を保証しない。判定にはfindを使う。

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

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

#ifndef SIRKIT_INTERNAL_HASH_MAP_DEFINED
#define SIRKIT_INTERNAL_HASH_MAP_DEFINED
namespace reinsirk::detail {
template <class Key, class Val, bool Packed = false, unsigned Denominator = 5>
class InternalHashMap {
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
        if (size_ != 0) {
            for (std::size_t i = 0; i < used_.size(); ++i) {
                if (!used_[i]) continue;
                std::size_t p = static_cast<std::size_t>((static_cast<std::uint64_t>(keys_[i]) * salt_) >> next_shift);
                while (used[p]) p = (p + 1) & (capacity - 1);
                keys[p] = keys_[i];
                values[p].value = values_[i].value;
                used[p] = 1;
            }
        }
        keys_.swap(keys);
        values_.swap(values);
        used_.swap(used);
        shift_ = next_shift;
    }

public:
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

    void reallocate() {
        if (keys_.empty()) {
            rehash(8);
        } else {
            if (keys_.size() > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            rehash(keys_.size() * 2);
        }
    }

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

    template <class F> Val& find_or_emplace(Key key, F&& factory) {
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
        values_[p].value = std::forward<F>(factory)();
        keys_[p] = key;
        used_[p] = 1;
        ++size_;
        return values_[p].value;
    }

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

    std::vector<std::pair<Key, Val>> enumerate() const {
        std::vector<std::pair<Key, Val>> result;
        if (empty()) return result;
        result.reserve(size_);
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) result.emplace_back(keys_[i], values_[i].value);
        return result;
    }

    template <class F> void for_each(F&& f) const {
        if (empty()) return;
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) f(keys_[i], values_[i].value);
    }

    [[nodiscard]] bool contains(Key key) const noexcept { return find(key) != nullptr; }
    [[nodiscard]] bool contain(Key key) const noexcept { return contains(key); }
    [[nodiscard]] int count(Key key) const noexcept { return contains(key) ? 1 : 0; }
    void clear() noexcept {
        if (empty()) return;
        std::fill(used_.begin(), used_.end(), Flag{0});
        size_ = 0;
    }
    void release() noexcept {
        decltype(keys_){}.swap(keys_);
        decltype(values_){}.swap(values_);
        decltype(used_){}.swap(used_);
        size_ = 0;
        shift_ = 61;
    }
    std::size_t capacity() const noexcept { return keys_.size(); }
    double load_factor() const noexcept {
        return capacity() == 0 ? 0.0 : static_cast<double>(size_) / capacity();
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    void set_default(const Val& value) { default_value_ = value; }
};

}  // namespace reinsirk::detail
#endif  // SIRKIT_INTERNAL_HASH_MAP_DEFINED


#ifndef SIRKIT_DYNAMIC_UNION_FIND_DEFINED
#define SIRKIT_DYNAMIC_UNION_FIND_DEFINED
class DynamicUnionFind {
    struct Node {
        int key;
        // 根なら負のサイズ、非根なら親の添字。キーの符号とは独立。
        int parent;
    };
    reinsirk::detail::InternalHashMap<int, int, false, 4> indices_;
    std::vector<Node> nodes_;
    std::size_t components_ = 0;

    static void check_capacity(std::size_t count) {
        if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("DynamicUnionFind: too many nodes");
    }

#if defined(__GNUC__) && !defined(__clang__)
    int index_for(int key) {
        return indices_.find_or_emplace(key, [&] {
            check_capacity(nodes_.size() + 1);
            const int index = static_cast<int>(nodes_.size());
            nodes_.push_back(Node{key, -1});
            ++components_;
            return index;
        });
    }
#else
    int index_for(int key) {
        if (const auto* index = indices_.find(key)) return *index;
        check_capacity(nodes_.size() + 1);
        const int index = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{key, -1});
        try {
            indices_[key] = index;
        } catch (...) {
            nodes_.pop_back();
            throw;
        }
        ++components_;
        return index;
    }
#endif

    int root_index(int index) noexcept {
        while (nodes_[index].parent >= 0) {
            const int parent = nodes_[index].parent;
            if (nodes_[parent].parent < 0) return parent;
            nodes_[index].parent = nodes_[parent].parent;
            index = nodes_[index].parent;
        }
        return index;
    }

    std::pair<int, int> merge_roots(int x, int y) {
#if defined(__GNUC__) && !defined(__clang__)
        const int ix = index_for(x);
        const int iy = index_for(y);
        int a = root_index(ix);
        int b = root_index(iy);
#else
        int a = root_index(index_for(x));
        int b = root_index(index_for(y));
#endif
        if (a == b) return {-1, -1};
        if (nodes_[a].parent > nodes_[b].parent) std::swap(a, b);
        // 各成分のサイズの合計は、上限検査済みの登録数以下。
        nodes_[a].parent += nodes_[b].parent;
        nodes_[b].parent = a;
        --components_;
        return {a, b};
    }

    std::vector<std::vector<int>> trivial_groups() {
        std::vector<std::vector<int>> result;
        result.reserve(components_);
        if (components_ == nodes_.size()) {
            for (const auto& node : nodes_) result.emplace_back(std::size_t{1}, node.key);
        } else {
            result.emplace_back(); result.back().reserve(nodes_.size());
            const int root = root_index(0);
            for (std::size_t i = 0; i < nodes_.size(); ++i) {
                result[0].push_back(nodes_[i].key);
                if (static_cast<int>(i) != root) nodes_[i].parent = root;
            }
        }
        return result;
    }

public:
    DynamicUnionFind() = default;
    explicit DynamicUnionFind(std::size_t expected_nodes) { reserve(expected_nodes); }
    DynamicUnionFind(const DynamicUnionFind&) = default;
    DynamicUnionFind& operator=(const DynamicUnionFind& other) {
        if (this == &other) return *this;
        DynamicUnionFind copy(other);
        indices_.swap(copy.indices_);
        nodes_.swap(copy.nodes_);
        std::swap(components_, copy.components_);
        return *this;
    }
    DynamicUnionFind(DynamicUnionFind&& other) noexcept
        : indices_(std::move(other.indices_)), nodes_(std::move(other.nodes_)),
          components_(std::exchange(other.components_, 0)) {
        other.nodes_.clear();
    }
    DynamicUnionFind& operator=(DynamicUnionFind&& other) noexcept {
        if (this == &other) return *this;
        indices_ = std::move(other.indices_);
        nodes_ = std::move(other.nodes_);
        components_ = std::exchange(other.components_, 0);
        other.nodes_.clear();
        return *this;
    }

    int data(int key) {
        const int index = index_for(key);
        return nodes_[index].parent < 0 ? nodes_[index].parent
                                             : nodes_[nodes_[index].parent].key;
    }

    int find(int key) { return nodes_[root_index(index_for(key))].key; }

    int unite(int x, int y) { return merge_roots(x, y).first != -1; }

    template <class F>
    int unite(int x, int y, const F& callback) {
        const auto [root, child] = merge_roots(x, y);
        if (root == -1) return false;
        callback(nodes_[root].key, nodes_[child].key);
        return true;
    }

    int size(int key) { return -nodes_[root_index(index_for(key))].parent; }
#if defined(__GNUC__) && !defined(__clang__)
    int same(int x, int y) {
        const int ix = index_for(x);
        const int iy = index_for(y);
        return root_index(ix) == root_index(iy);
    }
#else
    int same(int x, int y) { return find(x) == find(y); }

#endif

    bool add(int key) {
        const auto before = nodes_.size();
        index_for(key);
        return before != nodes_.size();
    }

    bool contains(int key) const {
        return indices_.contains(key);
    }
    std::size_t node_count() const noexcept { return nodes_.size(); }
    std::size_t component_count() const noexcept { return components_; }
    bool empty() const noexcept { return nodes_.empty(); }

    void reserve(std::size_t expected_nodes) {
        check_capacity(expected_nodes);
        nodes_.reserve(expected_nodes);
        indices_.reserve(expected_nodes);
    }

    std::vector<std::vector<int>> groups() {
#if defined(__GNUC__) && !defined(__clang__)
        if (nodes_.empty()) return {};
#endif
        if (components_ == nodes_.size() || components_ == 1) return trivial_groups();
        std::vector<std::vector<int>> result;
        result.reserve(components_);
#if defined(__GNUC__) && !defined(__clang__)
        using GroupIndex = std::make_unsigned_t<int>;
        // 根の添字だけを読み、全て直後の走査で代入する。
        std::array<GroupIndex, 32> local_indices;
        std::unique_ptr<GroupIndex[]> large_indices;
        GroupIndex* group_index = local_indices.data();
        if (nodes_.size() > local_indices.size()) {
            large_indices = std::make_unique_for_overwrite<GroupIndex[]>(nodes_.size());
            group_index = large_indices.get();
        }
#else
        std::vector<std::size_t> group_index(nodes_.size());
#endif
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (nodes_[i].parent >= 0) continue;
#if defined(__GNUC__) && !defined(__clang__)
            group_index[i] = static_cast<GroupIndex>(result.size());
#else
            group_index[i] = result.size();
#endif
            result.emplace_back();
            result.back().reserve(static_cast<std::size_t>(-nodes_[i].parent));
        }
        for (std::size_t i = 0; i < nodes_.size(); ++i)
            result[group_index[root_index(static_cast<int>(i))]].push_back(nodes_[i].key);
        return result;
    }

    void clear() {
        indices_.clear();
        nodes_.clear();
        components_ = 0;
    }
};
#endif


// Library: DataStructure/Hash_map.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/hashmap/hashmap.hpp
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/hash_map.md
//
// API一覧
// [HashMap<Key,Val,Hasher,KeyEqual>]
// Flat table。検索・追加と次要素を探さない削除は期待償却O(1)。列キーのハッシュ計算は列長に比例する。
// 次iteratorを返す削除は空きバケットを走査し、最悪O(capacity)。返却iteratorを順に使う全削除はO(capacity)。
// 再配置・clear・swap・ムーブでiterator/参照は無効。eraseは対象だけを無効にする。
// Key/Valはコピー可能、またはvalue_typeのムーブ構築がnoexceptであること。
// Hasher/KeyEqualは同値なキーに同じハッシュ値を返し、格納中に規則を変えないこと。
// 再配置を伴う新規追加の引数には、本表の要素への参照を渡さず先にコピーする。
// HashMap(): 空の表を構築する。必要になるまで配列は確保しない。
// HashMap(n, hash, equal): n要素分を予約し、指定したハッシュ・同値判定で構築する。
// operator[](key): 未登録ならVal{}で追加し、値の参照を返す。
// find(key): 対応するiteratorを返す。未登録はend()。const版は変更できない。
// contain(key)/contains(key): 登録済みならtrueを返す。追加はしない。
// count(key): 登録済みなら1、未登録なら0を返す。
// at(key): 値の参照を返す。未登録はout_of_range。const版は変更できない。
// insert({key, value})/emplace(key, value): 未登録なら追加し、対応するiteratorを返す。
// try_emplace(key, args...): 未登録時だけ値を構築し、{iterator, 追加の有無}を返す。
// insert_or_assign(key, value): 追加または上書きし、{iterator, 追加の有無}を返す。
// get_or_default(key, fallback): 登録値またはfallbackのコピーを返す。追加はしない。
// erase(it, next=true): 対象を削除し次のiteratorを返す。false指定ならend()。
// erase(key): キーを削除し、次のiteratorを返す。未登録・erase(end())はend()。
// erase(key, next): falseなら次要素を探さず削除してend()、trueならerase(key)と同じ。
// begin()/end()/cbegin()/cend(): 格納要素を順序不定で列挙する。キーは変更できない。
// 空表のbeginはO(1)、それ以外は先頭要素までのバケットを走査する。
// size()/empty(): 登録数・空かどうかをO(1)で返す。
// clear(): 全要素を破棄し、確保済みの容量を再利用する。
// reserve(n): n要素分の容量を予約する。縮小しない。
// rehash(n): nバケット以上・現登録数に必要な容量へ再配置し、墓石を除く。
// capacity()/load_factor(): バケット数・使用率をO(1)で返す。
// swap(other): 内容とハッシュ・同値判定を交換する。コピーは独立した複製。
// swapはHasher/KeyEqualのswapが例外を送出しないことを要求する。
//
// [GenericHash]
// GenericHash(): 実行時に選んだ種で構築する。同じオブジェクトでは結果は一定。
// GenericHash(seed): 指定した種で構築する。std::hash対応型の結果はその実装にも依存する。
// operator()(x): 整数・列挙・浮動小数・tuple・range・std::hash対応型を再帰的にハッシュ化する。
// unorderedコンテナの要素順は無視する。独自の同値関係には対応するHasherも用意する。

#include <algorithm>
#include <bit>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#if !defined(SIRKIT_DISABLE_SIMD) && defined(__SSE2__) && (defined(__x86_64__) || defined(__i386__)) && defined(__GNUC__) && !defined(__clang__)
#include <emmintrin.h>
#endif

#ifndef SIRKIT_GENERIC_HASH_DEFINED
#define SIRKIT_GENERIC_HASH_DEFINED
struct GenericHash {
private:
    std::uint64_t seed_;
    static constexpr std::uint64_t mix(std::uint64_t x) noexcept {
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        return x ^ (x >> 31);
    }
    template <class> static constexpr bool dependent_false = false;
    template <class T> static constexpr bool integral_pair = requires (const T& value) {
        value.first; value.second;
        requires std::is_integral_v<std::remove_cvref_t<decltype(value.first)>>;
        requires std::is_integral_v<std::remove_cvref_t<decltype(value.second)>>;
    };
    template <class T>
    std::uint64_t raw(const T& value) const {
        using U = std::remove_cvref_t<T>;
        if constexpr (std::is_integral_v<U>) {
            static_assert(sizeof(U) <= sizeof(std::uint64_t), "GenericHash: integer exceeds 64 bits");
            return static_cast<std::uint64_t>(value);
        } else if constexpr (std::is_enum_v<U>) {
            return static_cast<std::uint64_t>(static_cast<std::underlying_type_t<U>>(value));
        } else if constexpr (requires { typename std::tuple_size<U>::type; }) {
            std::uint64_t result = seed_;
            std::apply([&](const auto&... elements) {
                ((result = result * UINT64_C(11995408973635179863) + raw(elements)), ...);
            }, value);
            return mix(result ^ std::tuple_size_v<U>);
        } else if constexpr (requires { typename U::hasher; typename U::key_equal; std::begin(value); std::end(value); }) {
            // 同じ要素集合は列挙順にかかわらず同じ値になる。
            std::uint64_t sum = 0, second = 0, count = 0;
            for (const auto& element : value) {
                const auto h = mix(raw(element) + seed_);
                sum += h;
                second += h * h;
                ++count;
            }
            return mix(sum ^ std::rotl(second, 23) ^ count);
        } else if constexpr (requires { { std::hash<U>{}(value) } -> std::convertible_to<std::size_t>; }) {
            // 浮動小数の+0/-0もstd::hashに従い等しい値にする。
            return static_cast<std::uint64_t>(std::hash<U>{}(value));
        } else if constexpr (requires { std::begin(value); std::end(value); }) {
            std::uint64_t result = seed_, count = 0;
            for (const auto& element : value) {
                result = result * UINT64_C(11995408973635179863) + raw(element);
                ++count;
            }
            return mix(result ^ count);
        } else {
            static_assert(dependent_false<U>, "GenericHash: provide std::hash or a custom Hasher for this key");
        }
    }
public:
    GenericHash() : GenericHash(static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count())) {}
    explicit constexpr GenericHash(std::uint64_t seed) noexcept : seed_(mix(seed) | 1) {}
    template <class T>
    std::uint64_t operator()(const T& value) const noexcept(
        std::is_integral_v<T> || std::is_enum_v<T> || integral_pair<T>) {
        using U = std::remove_cvref_t<T>;
        if constexpr (std::is_integral_v<U> || std::is_enum_v<U>) {
            return (raw(value) ^ seed_) * UINT64_C(11995408973635179863);
        } else if constexpr (integral_pair<U>) {
            return (raw(value.first) ^ seed_) * UINT64_C(11995408973635179863) +
                   (raw(value.second) ^ seed_) * UINT64_C(10150724397891781847);
        } else return raw(value) * seed_;
    }
};
#endif

#ifndef SIRKIT_HASH_MAP_DEFINED
#define SIRKIT_HASH_MAP_DEFINED
template <class Key, class Val, class Hasher = GenericHash, class KeyEqual = std::equal_to<Key>>
class HashMap {
public:
    using key_type = Key;
    using mapped_type = Val;
    using value_type = std::pair<const Key, Val>;
    using Data = value_type;
    using size_type = std::size_t;
private:
    // 自明にコピー・破棄できる型は、GCCのx86では登録時に構築する。
    // 構築に失敗しうる型はoptionalで未構築状態を管理する。
#if defined(__GNUC__) && !defined(__clang__) && (defined(__x86_64__) || defined(__i386__))
    union CompactSlot {
        value_type value;
        CompactSlot() noexcept {}
        value_type& operator*() noexcept { return value; }
        const value_type& operator*() const noexcept { return value; }
        value_type* operator->() noexcept { return std::addressof(value); }
        const value_type* operator->() const noexcept { return std::addressof(value); }
        template <class... Args> void emplace(Args&&... args) {
            value_type next(std::forward<Args>(args)...);
            std::construct_at(std::addressof(value), std::move(next));
        }
        void reset() noexcept {} // 自明な破棄なので、削除状態だけをstate_で変更する。
    };
#else
    struct CompactSlot {
        value_type value{};
        value_type& operator*() noexcept { return value; }
        const value_type& operator*() const noexcept { return value; }
        value_type* operator->() noexcept { return std::addressof(value); }
        const value_type* operator->() const noexcept { return std::addressof(value); }
        template <class... Args> void emplace(Args&&... args) {
            value_type next(std::forward<Args>(args)...);
            std::destroy_at(std::addressof(value));
            std::construct_at(std::addressof(value), std::move(next));
        }
        void reset() noexcept {} // 自明な破棄なので、削除状態だけをstate_で変更する。
    };
#endif
    using Slot = std::conditional_t<
        std::is_nothrow_default_constructible_v<value_type> &&
        std::is_nothrow_move_constructible_v<value_type> &&
        std::is_trivially_copy_constructible_v<value_type> &&
        std::is_trivially_destructible_v<value_type>, CompactSlot, std::optional<value_type>>;
    static_assert(std::is_copy_constructible_v<value_type> ||
                  std::is_nothrow_move_constructible_v<value_type>,
                  "HashMap: entries must be copyable or nothrow movable");
    static_assert(std::is_copy_constructible_v<Hasher> && std::is_copy_constructible_v<KeyEqual> &&
                  std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>,
                  "HashMap: Hasher and KeyEqual must be copyable and nothrow swappable");
    std::vector<Slot> slots_;
    std::vector<std::uint8_t> state_; // 0=空、1=登録済み、2=削除済み
    size_type size_ = 0, occupied_ = 0;
    unsigned shift_ = 64;
    [[no_unique_address]] Hasher hash_;
    [[no_unique_address]] KeyEqual equal_;
#ifndef SKIP_RUNTIME_CHECKS
    size_type generation_ = 0;
#endif
    void invalidate() noexcept {
#ifndef SKIP_RUNTIME_CHECKS
        ++generation_;
#endif
    }
    size_type max_capacity() const noexcept {
        return std::bit_floor(std::min(slots_.max_size(), state_.max_size()));
    }
    static size_type threshold(size_type capacity) noexcept { return capacity == 0 ? 0 : (capacity - 1) / 2; }
    size_type bucket(const Key& key, unsigned shift) const noexcept(
        std::is_nothrow_invocable_v<const Hasher&, const Key&>) {
        const auto h = static_cast<std::uint64_t>(hash_(key));
        if constexpr (std::is_same_v<Hasher, GenericHash>) return static_cast<size_type>(h >> shift);
        else return static_cast<size_type>((h * UINT64_C(11995408973635179863)) >> shift);
    }
    size_type locate(const Key& key) const {
        if (slots_.empty()) return 0;
        size_type p = bucket(key, shift_);
        while (state_[p] == 1) {
            if (equal_(slots_[p]->first, key)) return p;
            p = (p + 1) & (slots_.size() - 1);
        }
        if (state_[p] == 0) return slots_.size();
        while (state_[p]) {
            if constexpr (std::is_same_v<Slot, CompactSlot>) {
                if (equal_(slots_[p]->first, key)) return state_[p] == 1 ? p : slots_.size();
            } else if (state_[p] == 1 && equal_(slots_[p]->first, key)) return p;
            p = (p + 1) & (slots_.size() - 1);
        }
        return slots_.size();
    }
#if !defined(SIRKIT_DISABLE_SIMD) && defined(__SSE2__) && (defined(__x86_64__) || defined(__i386__)) && defined(__GNUC__) && !defined(__clang__)
    // 単純な型の再配置で、登録状態を16バケットずつ判定する。
    void rebuild_simd(size_type capacity) {
        std::vector<Slot> next(capacity);
#if defined(__GNUC__) && !defined(__clang__) && (defined(__x86_64__) || defined(__i386__))
        // 値の構築前に領域を一定間隔で書き、ランダムな初回書き込みの負担を抑える。
        if constexpr (std::is_same_v<Slot, CompactSlot>) {
            if (capacity >= 256) {
                constexpr size_type stride = std::max(size_type{1}, size_type{4096} / sizeof(Slot));
                for (size_type i = 0; i < capacity; i += stride)
                    *reinterpret_cast<volatile unsigned char*>(std::addressof(next[i])) = 0;
            }
        }
#endif
        std::vector<std::uint8_t> state(capacity, 0);
        const auto shift = static_cast<unsigned>(64 - std::countr_zero(capacity));
        const auto live = _mm_set1_epi8(1);
        for (size_type base = 0; base < slots_.size(); base += 16) {
            const auto states = _mm_loadu_si128(reinterpret_cast<const __m128i*>(state_.data() + base));
            unsigned mask = static_cast<unsigned>(_mm_movemask_epi8(_mm_cmpeq_epi8(states, live)));
            while (mask) {
                const size_type i = base + std::countr_zero(mask);
                size_type p = bucket(slots_[i]->first, shift);
                while (state[p]) p = (p + 1) & (capacity - 1);
                next[p].emplace(*slots_[i]); // 自明なコピーなので、hashが投げても元の値を保つ。
                state[p] = 1;
                mask &= mask - 1;
            }
        }
        slots_.swap(next);
        state_.swap(state);
        occupied_ = size_;
        shift_ = shift;
        invalidate();
    }
#endif
    void rebuild(size_type capacity) {
        if (capacity > max_capacity()) throw std::length_error("HashMap: capacity overflow");
#if !defined(SIRKIT_DISABLE_SIMD) && defined(__SSE2__) && (defined(__x86_64__) || defined(__i386__)) && defined(__GNUC__) && !defined(__clang__)
        if constexpr (std::is_same_v<Slot, CompactSlot>) {
            if (slots_.size() >= 64) { rebuild_simd(capacity); return; }
        }
#endif
        std::vector<Slot> next(capacity);
#if defined(__GNUC__) && !defined(__clang__) && (defined(__x86_64__) || defined(__i386__))
        // 値の構築前に領域を一定間隔で書き、ランダムな初回書き込みの負担を抑える。
        if constexpr (std::is_same_v<Slot, CompactSlot>) {
            if (capacity >= 256) {
                constexpr size_type stride = std::max(size_type{1}, size_type{4096} / sizeof(Slot));
                for (size_type i = 0; i < capacity; i += stride)
                    *reinterpret_cast<volatile unsigned char*>(std::addressof(next[i])) = 0;
            }
        }
#endif
        std::vector<std::uint8_t> state(capacity, 0);
        const auto shift = static_cast<unsigned>(64 - std::countr_zero(capacity));
        if constexpr (std::is_nothrow_move_constructible_v<value_type> &&
                      std::is_nothrow_invocable_v<const Hasher&, const Key&>) {
            for (size_type i = 0; i < slots_.size(); ++i) {
                if (state_[i] != 1) continue;
                size_type p = bucket(slots_[i]->first, shift);
                while (state[p]) p = (p + 1) & (capacity - 1);
                next[p].emplace(std::move(*slots_[i]));
                state[p] = 1;
            }
        } else {
            // 例外を送出しうるハッシュは、移動を始める前に全移動先を計算する。
            std::vector<std::pair<size_type, size_type>> positions;
            positions.reserve(size_);
            for (size_type i = 0; i < slots_.size(); ++i) {
                if (state_[i] != 1) continue;
                size_type p = bucket(slots_[i]->first, shift);
                while (state[p]) p = (p + 1) & (capacity - 1);
                state[p] = 1;
                positions.emplace_back(i, p);
            }
            for (const auto& [old, dest] : positions) {
                if constexpr (std::is_nothrow_move_constructible_v<value_type>)
                    next[dest].emplace(std::move(*slots_[old]));
                else next[dest].emplace(*slots_[old]);
            }
        }
        slots_.swap(next);
        state_.swap(state);
        occupied_ = size_;
        shift_ = shift;
        invalidate();
    }
    size_type insertion_slot(const Key& key) {
        if (slots_.empty()) rebuild(8);
        size_type p = bucket(key, shift_), first_deleted = slots_.size();
        while (state_[p]) {
            if constexpr (std::is_same_v<Slot, CompactSlot>) {
                // 同じキーの墓石より後ろに同キーの有効要素はない。
                if (equal_(slots_[p]->first, key)) return p;
            } else if (state_[p] == 1 && equal_(slots_[p]->first, key)) return p;
            if (state_[p] == 2 && first_deleted == slots_.size()) first_deleted = p;
            p = (p + 1) & (slots_.size() - 1);
        }
        if (size_ == threshold(slots_.size())) {
            if (slots_.size() > max_capacity() / 2) throw std::length_error("HashMap: capacity overflow");
            rebuild(slots_.size() * 2);
            return insertion_slot(key);
        }
        if (first_deleted != slots_.size()) return first_deleted;
        if (occupied_ == threshold(slots_.size())) {
            rebuild(slots_.size());
            return insertion_slot(key);
        }
        return p;
    }
    void mark_inserted(size_type p) noexcept {
        if (state_[p] == 0) ++occupied_;
        state_[p] = 1;
        ++size_;
    }
public:
    template <bool Const>
    class BasicIterator {
        friend class HashMap;
        template <bool> friend class BasicIterator;
        using Owner = std::conditional_t<Const, const HashMap, HashMap>;
        Owner* owner_ = nullptr;
        size_type index_ = 0;
#ifndef SKIP_RUNTIME_CHECKS
        size_type generation_ = 0;
#endif
        BasicIterator(Owner* owner, size_type index) noexcept : owner_(owner), index_(index) {
            // 内部で新しく作るiteratorは有効な要素またはendだけを指す。
            if (index > owner->slots_.size() ||
                (index < owner->slots_.size() && owner->state_[index] != 1)) std::unreachable();
#ifndef SKIP_RUNTIME_CHECKS
            generation_ = owner->generation_;
#endif
        }
        void check(bool element) const {
#ifndef SKIP_RUNTIME_CHECKS
            if (!owner_ || generation_ != owner_->generation_ || index_ > owner_->slots_.size() ||
                (element && index_ == owner_->slots_.size()) ||
                (index_ < owner_->slots_.size() && owner_->state_[index_] != 1))
                throw std::out_of_range("HashMap: invalid iterator");
#else
            (void)element;
#endif
        }
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using iterator_concept = std::bidirectional_iterator_tag;
        using value_type = typename HashMap::value_type;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<Const, const value_type&, value_type&>;
        using pointer = std::conditional_t<Const, const value_type*, value_type*>;
        BasicIterator() noexcept = default;
        template <bool Other> requires (Const && !Other)
        BasicIterator(const BasicIterator<Other>& other) noexcept : owner_(other.owner_), index_(other.index_) {
#ifndef SKIP_RUNTIME_CHECKS
            generation_ = other.generation_;
#endif
        }
        reference operator*() const { check(true); return *owner_->slots_[index_]; }
        pointer operator->() const { return std::addressof(operator*()); }
        BasicIterator& operator++() {
            check(true);
            do { ++index_; } while (index_ < owner_->slots_.size() && owner_->state_[index_] != 1);
            return *this;
        }
        BasicIterator operator++(int) { auto copy = *this; ++*this; return copy; }
        BasicIterator& operator--() {
            check(false);
            auto p = index_;
            while (p != 0) { --p; if (owner_->state_[p] == 1) { index_ = p; return *this; } }
#ifndef SKIP_RUNTIME_CHECKS
            throw std::out_of_range("HashMap: iterator before begin");
#else
            index_ = 0;
            return *this;
#endif
        }
        BasicIterator operator--(int) { auto copy = *this; --*this; return copy; }
        template <bool Other> bool operator==(const BasicIterator<Other>& other) const noexcept {
            return owner_ == other.owner_ && index_ == other.index_;
        }
    };
    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;
    using itr = iterator;

    HashMap() = default;
    explicit HashMap(size_type expected, Hasher hash = {}, KeyEqual equal = {})
        : hash_(std::move(hash)), equal_(std::move(equal)) { reserve(expected); }
    HashMap(const HashMap&) = default;
    HashMap(HashMap&& other) noexcept(std::is_nothrow_copy_constructible_v<Hasher> &&
                                    std::is_nothrow_copy_constructible_v<KeyEqual> &&
                                    std::is_nothrow_swappable_v<Hasher> &&
                                    std::is_nothrow_swappable_v<KeyEqual>)
        : hash_(other.hash_), equal_(other.equal_) { swap(other); }
    HashMap& operator=(HashMap other) { swap(other); return *this; }
    void swap(HashMap& other) noexcept(std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>) {
        static_assert(std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>,
                      "HashMap: Hasher and KeyEqual must be nothrow swappable");
        using std::swap;
        swap(hash_, other.hash_); swap(equal_, other.equal_);
        slots_.swap(other.slots_); state_.swap(other.state_);
        swap(size_, other.size_); swap(occupied_, other.occupied_); swap(shift_, other.shift_);
        invalidate(); other.invalidate();
    }
    iterator begin() noexcept {
        if (size_ == 0) return end();
        size_type p = 0; while (p < slots_.size() && state_[p] != 1) ++p;
        return iterator(this, p);
    }
    const_iterator begin() const noexcept {
        if (size_ == 0) return end();
        size_type p = 0; while (p < slots_.size() && state_[p] != 1) ++p;
        return const_iterator(this, p);
    }
    iterator end() noexcept { return iterator(this, slots_.size()); }
    const_iterator end() const noexcept { return const_iterator(this, slots_.size()); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }
    iterator find(const Key& key) { return iterator(this, locate(key)); }
    const_iterator find(const Key& key) const { return const_iterator(this, locate(key)); }
    bool contains(const Key& key) const { return locate(key) != slots_.size(); }
    bool contain(const Key& key) const { return contains(key); }
    int count(const Key& key) const { return contains(key) ? 1 : 0; }
    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    size_type capacity() const noexcept { return slots_.size(); }
    double load_factor() const noexcept { return slots_.empty() ? 0.0 : static_cast<double>(size_) / slots_.size(); }
    Val& at(const Key& key) {
        const auto p = locate(key);
        if (p == slots_.size()) throw std::out_of_range("HashMap::at: missing key");
        return slots_[p]->second;
    }
    const Val& at(const Key& key) const {
        const auto p = locate(key);
        if (p == slots_.size()) throw std::out_of_range("HashMap::at: missing key");
        return slots_[p]->second;
    }
    Val get_or_default(const Key& key, Val fallback = Val{}) const {
        const auto p = locate(key);
        return p == slots_.size() ? std::move(fallback) : slots_[p]->second;
    }
    template <class... Args>
    std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args) {
        const auto p = insertion_slot(key);
        if (state_[p] == 1) return {iterator(this, p), false};
        slots_[p].emplace(std::piecewise_construct, std::forward_as_tuple(key),
                          std::forward_as_tuple(std::forward<Args>(args)...));
        mark_inserted(p);
        return {iterator(this, p), true};
    }
    Val& operator[](const Key& key) { return try_emplace(key).first->second; }
    iterator insert(const value_type& entry) { return try_emplace(entry.first, entry.second).first; }
    iterator insert(value_type&& entry) { return try_emplace(entry.first, std::move(entry.second)).first; }
    iterator emplace(const Key& key, const Val& value) { return try_emplace(key, value).first; }
    template <class V>
    std::pair<iterator, bool> insert_or_assign(const Key& key, V&& value) {
        auto result = try_emplace(key, std::forward<V>(value));
        if (!result.second) result.first->second = std::forward<V>(value);
        return result;
    }
    iterator erase(const_iterator it, bool get_next = true) {
#ifndef SKIP_RUNTIME_CHECKS
        if (it.owner_ != this || it.generation_ != generation_ || it.index_ > slots_.size())
            throw std::out_of_range("HashMap::erase: foreign or stale iterator");
#endif
        if (it.index_ == slots_.size()) return end();
#ifndef SKIP_RUNTIME_CHECKS
        if (state_[it.index_] != 1)
            throw std::out_of_range("HashMap::erase: invalid iterator");
#endif
        const auto p = it.index_;
        slots_[p].reset(); state_[p] = 2; --size_;
        if (!get_next) return end();
        auto next = p;
        do { ++next; } while (next < slots_.size() && state_[next] != 1);
        return iterator(this, next);
    }
    iterator erase(iterator it, bool get_next = true) { return erase(const_iterator(it), get_next); }
    iterator erase(const Key& key) { return erase(find(key)); }
    iterator erase(const Key& key, bool get_next) { return erase(find(key), get_next); }
    void clear() noexcept {
        if (occupied_ == 0) { invalidate(); return; }
        for (size_type i = 0; i < slots_.size(); ++i) if (state_[i] == 1) slots_[i].reset();
        std::fill(state_.begin(), state_.end(), std::uint8_t{0});
        size_ = occupied_ = 0; invalidate();
    }
    void reserve(size_type count) {
        if (count <= threshold(slots_.size())) return;
        if (count > threshold(max_capacity())) throw std::length_error("HashMap::reserve: capacity overflow");
        size_type capacity = 8;
        while (count > threshold(capacity)) capacity *= 2;
        rebuild(capacity);
    }
    void rehash(size_type requested) {
        if (requested > max_capacity()) throw std::length_error("HashMap::rehash: capacity overflow");
        size_type capacity = 8;
        while (requested > capacity || size_ > threshold(capacity)) {
            if (capacity > max_capacity() / 2) throw std::length_error("HashMap::rehash: capacity overflow");
            capacity *= 2;
        }
        rebuild(capacity);
    }
};
#endif


// Library: Internal/Nyaan_hash_map_min.cpp
// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/hash-map-variable-length.hpp.html
// Original Source (mix): https://prng.di.unimi.it/splitmix64.c
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/internal_hash_map.md
//
// API一覧
// [reinsirk::detail::InternalHashMap<Key,Val,Packed=false,Denominator=5>]
// Keyはbool・const・volatileを除く64bit以下の整数型。Denominatorは2〜8。
// dynamic系で共有する内部用の整数ハッシュ表。再確保・release・代入で参照は無効。
// 検索は期待O(1)、追加は期待償却O(1)。値は既定構築・コピー代入可能であること。
// Packedは占有フラグのbit化、Denominatorは空き容量の比率1/Denominatorを指定する。
// InternalHashMap(): 空の表を構築する。初期配列は確保しない。
// コピー構築は値と配列をコピーし、ムーブ構築は所有権を移す。
// operator=(other): 値渡しの表と交換し、自身への参照を返す。
// operator[](key): 未登録なら設定済みの初期値で追加し、値の参照を返す。
// find_or_emplace(key, f): 未登録ならf()で追加し、値の参照を返す。fは表を変更せず、表内の参照を使わない。
// find(key): 可変表はVal*、const表はconst Val*を返す。未登録はnullptr、追加しない。
// find_nonempty(key): 容量確保済みの表でfindと同じ結果を返す。内部ループ用。
// contains(key)/contain(key): キーが登録済みならtrueを返す。
// count(key): 登録済みなら1、未登録なら0を返す。
// for_each(f): 各キーと値のconst参照にfを適用する。fは表を変更しない。非空O(容量)、空O(1)。
// size()/empty()/capacity()/load_factor(): 件数・空判定・スロット数・使用率をO(1)で返す。
// reserve(n): n件まで再確保せず追加できる容量を準備する。
// reallocate(): 容量を2倍にする。空なら初期容量を確保する。
// clear(): 登録を全て削除する。領域と値の資源を保持。非空はO(容量)、空はO(1)。
// release(): 領域と登録値をO(容量)で破棄する。設定済み初期値を保持。
// set_default(value): 今後追加する要素の初期値を設定する。
// enumerate(): 登録済みの{キー, 値}を順序不定でコピーする。非空O(容量)、空O(1)。
// swap(other): 表の内容・初期値を交換する。

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

#ifndef SIRKIT_INTERNAL_HASH_MAP_DEFINED
#define SIRKIT_INTERNAL_HASH_MAP_DEFINED
namespace reinsirk::detail {
template <class Key, class Val, bool Packed = false, unsigned Denominator = 5>
class InternalHashMap {
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
        if (size_ != 0) {
            for (std::size_t i = 0; i < used_.size(); ++i) {
                if (!used_[i]) continue;
                std::size_t p = static_cast<std::size_t>((static_cast<std::uint64_t>(keys_[i]) * salt_) >> next_shift);
                while (used[p]) p = (p + 1) & (capacity - 1);
                keys[p] = keys_[i];
                values[p].value = values_[i].value;
                used[p] = 1;
            }
        }
        keys_.swap(keys);
        values_.swap(values);
        used_.swap(used);
        shift_ = next_shift;
    }

public:
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

    void reallocate() {
        if (keys_.empty()) {
            rehash(8);
        } else {
            if (keys_.size() > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            rehash(keys_.size() * 2);
        }
    }

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

    template <class F> Val& find_or_emplace(Key key, F&& factory) {
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
        values_[p].value = std::forward<F>(factory)();
        keys_[p] = key;
        used_[p] = 1;
        ++size_;
        return values_[p].value;
    }

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

    std::vector<std::pair<Key, Val>> enumerate() const {
        std::vector<std::pair<Key, Val>> result;
        if (empty()) return result;
        result.reserve(size_);
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) result.emplace_back(keys_[i], values_[i].value);
        return result;
    }

    template <class F> void for_each(F&& f) const {
        if (empty()) return;
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) f(keys_[i], values_[i].value);
    }

    [[nodiscard]] bool contains(Key key) const noexcept { return find(key) != nullptr; }
    [[nodiscard]] bool contain(Key key) const noexcept { return contains(key); }
    [[nodiscard]] int count(Key key) const noexcept { return contains(key) ? 1 : 0; }
    void clear() noexcept {
        if (empty()) return;
        std::fill(used_.begin(), used_.end(), Flag{0});
        size_ = 0;
    }
    void release() noexcept {
        decltype(keys_){}.swap(keys_);
        decltype(values_){}.swap(values_);
        decltype(used_){}.swap(used_);
        size_ = 0;
        shift_ = 61;
    }
    std::size_t capacity() const noexcept { return keys_.size(); }
    double load_factor() const noexcept {
        return capacity() == 0 ? 0.0 : static_cast<double>(size_) / capacity();
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    void set_default(const Val& value) { default_value_ = value; }
};

}  // namespace reinsirk::detail
#endif  // SIRKIT_INTERNAL_HASH_MAP_DEFINED


// Library: DataStructure/line-container.cpp
// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/line-container.hpp.html
// Original Source: https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/LineContainer.h
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/line_container.md
//
// API一覧
// [LineContainer<T,objective>・MinLineContainer<T>・MaxLineContainer<T>]
// add(k,m): y=k*x+mを追加、償却O(log h)。整数は符号付き64bit以下、浮動係数は有限値。
// query(x): 最適値をO(log h)で返す。空は検査時例外、省略時は非空が前提。整数範囲外は常に例外。
// query_optional(x): 空ならnullopt、それ以外はqueryと同じ。xは浮動小数点なら有限値。
// query_line(x): 選択した元の(k,m)をO(log h)で返す。同点の係数は未規定。
// query_line_optional(x): 空ならnullopt、それ以外はquery_lineと同じ。
// query_with_line(x): 1回の探索でResult{最適値,k,m}を返す。queryと同じ前提・O(log h)。
// query_with_line_optional(x): 空ならnullopt、それ以外はquery_with_lineと同じ。
// size()/empty()/clear(): 有効な包絡直線の本数・空判定・全削除、O(1)/O(1)/O(h)。
// begin()/end()/lower_bound(x): 元係数と交点を参照。最小化の傾き順は降順。直接変更は不変条件を壊す。
// insect(x,y): 旧互換の内部交点更新。正しい包絡iterator以外では呼び出さない。
// コピーは独立した包絡。hは有効直線数。浮動小数点版の交点・最適値には丸め誤差がある。
// objectiveはMINIMIZE/MAXIMIZE。MinLineContainer<T>/MaxLineContainer<T>は最小化/最大化の別名。
//
// [Line<T>・補助関数]
// Line<T>のk,m: 元の係数。p: この直線が選ばれる右端(整数は型の範囲に制限)。
// lc_inf<T>(): 整数では最大値、浮動小数点では正の無限大を返す。
// lc_div(a,b): 整数は床除算、浮動小数点は通常除算。0除算・整数結果の範囲外は例外。

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

#ifndef SIRKIT_LINE_CONTAINER_DEFINED
#define SIRKIT_LINE_CONTAINER_DEFINED

enum Objective { MAXIMIZE = +1, MINIMIZE = -1 };

template <typename T>
struct Line {
  mutable T k, m, p;
  bool operator<(const Line& other) const { return k < other.k; }
  bool operator<(T x) const { return p < x; }
};

template <typename T>
constexpr T lc_inf() {
  if constexpr (std::is_floating_point_v<T>) return std::numeric_limits<T>::infinity();
  else return std::numeric_limits<T>::max();
}

template <typename T>
T lc_div(T a, T b) {
  if (b == 0) throw std::domain_error("lc_div: division by zero");
  if constexpr (std::is_integral_v<T>) {
    static_assert(std::is_signed_v<T>);
    if (a == std::numeric_limits<T>::min() && b == -1)
      throw std::overflow_error("lc_div: quotient overflow");
    return static_cast<T>(a / b - ((a < 0) != (b < 0) && a % b != 0));
  } else return a / b;
}

namespace reinsirk::detail {
template <typename T, Objective objective>
struct LineComparator {
  using is_transparent = void;
  bool operator()(const Line<T>& a, const Line<T>& b) const {
    if constexpr (objective == MAXIMIZE) return a.k < b.k;
    else return a.k > b.k;
  }
  bool operator()(const Line<T>& a, T x) const { return a < x; }
  bool operator()(T x, const Line<T>& a) const { return x < a.p; }
};
}  // namespace reinsirk::detail

template <typename T, Objective objective>
struct LineContainer : std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>> {
  static_assert((std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= 8) ||
                std::is_floating_point_v<T>);
  static_assert(objective == MAXIMIZE || objective == MINIMIZE);
  using super = std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>>;
  using super::begin, super::end, super::insert, super::erase;
  using super::empty, super::lower_bound;
  static constexpr T inf = lc_inf<T>();
  struct Result { T value, k, m; };

 private:
  static bool boundary_less(const Line<T>& a, const Line<T>& b) {
    return a.p < b.p;
  }
  static bool intersection(Line<T> const& a, Line<T> const& b) {
    if constexpr (std::is_integral_v<T>) {
      using U = std::make_unsigned_t<T>;
      const bool negative_n = b.m < a.m, negative_d = a.k < b.k;
      const U n = negative_n ? U(a.m) - U(b.m) : U(b.m) - U(a.m);
      const U d = negative_d ? U(b.k) - U(a.k) : U(a.k) - U(b.k);
      U q = n / d;
      constexpr U top = U(std::numeric_limits<T>::max());
      if (negative_n != negative_d) {
        q = static_cast<U>(q + U(n % d != 0));
        if (q >= top + U{1}) {
          a.p = std::numeric_limits<T>::min();
          return q > top + U{1};
        }
        else a.p = static_cast<T>(-static_cast<T>(q));
      } else a.p = q > top ? inf : static_cast<T>(q);
      return false;
    } else {
      const T n = b.m - a.m, d = a.k - b.k;
      if (std::isfinite(n + d) || (std::isfinite(n) && std::isfinite(d))) a.p = n / d;
      else if (!std::isfinite(n) && !std::isfinite(d))
        a.p = (b.m / 2 - a.m / 2) / (a.k / 2 - b.k / 2);
      else if (!std::isfinite(n)) a.p = ((b.m / 2 - a.m / 2) / d) * 2;
      else a.p = (n / (a.k / 2 - b.k / 2)) / 2;
      return false;
    }
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  static T evaluate(T k, T m, T x) {
    if constexpr (std::is_floating_point_v<T>) return k * x + m;
    else if constexpr (sizeof(T) < 8) {
      const std::int64_t result = std::int64_t(k) * x + m;
      if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
        throw std::overflow_error("LineContainer: value overflow");
      return static_cast<T>(result);
    } else {
#if defined(__SIZEOF_INT128__) && !defined(SIRKIT_LINE_CONTAINER_FORCE_PORTABLE)
      T fast;
      if (!__builtin_mul_overflow(k, x, &fast) && !__builtin_add_overflow(fast, m, &fast))
#if defined(__clang__)
        [[likely]]
#endif
        return fast;
      __extension__ using Wide = __int128;
      const Wide result = Wide(k) * x + m;
      if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
        throw std::overflow_error("LineContainer: value overflow");
      return static_cast<T>(result);
#else
      using U = std::uint64_t;
      // 32bit範囲なら積と和が64bitに収まる。
      constexpr U narrow_offset = U{1} << 31;
      if (((U(k) + narrow_offset) | (U(x) + narrow_offset) |
           (U(m) + narrow_offset)) < (U{1} << 32))
        return k * x + m;
      const U a = k < 0 ? U{0} - U(k) : U(k);
      const U b = x < 0 ? U{0} - U(x) : U(x);
      const U c = m < 0 ? U{0} - U(m) : U(m);
      constexpr U mask = 0xffffffffU;
      const U t0 = (a & mask) * (b & mask);
      const U t1 = (a >> 32) * (b & mask) + (t0 >> 32);
      const U t2 = (a & mask) * (b >> 32) + (t1 & mask);
      U hi = (a >> 32) * (b >> 32) + (t1 >> 32) + (t2 >> 32);
      U lo = (t2 << 32) | (t0 & mask);
      bool negative = (k < 0) != (x < 0);
      if (negative == (m < 0)) {
        const U previous = lo;
        lo += c;
        hi += lo < previous;
      } else if (hi != 0 || lo >= c) {
        const U previous = lo;
        lo -= c;
        hi -= previous < c;
      } else { lo = c - lo; negative = m < 0; }
      const U limit = U(std::numeric_limits<T>::max()) + U(negative);
      if (hi != 0 || lo > limit) throw std::overflow_error("LineContainer: value overflow");
      if (negative && lo == U(std::numeric_limits<T>::max()) + U{1})
        return std::numeric_limits<T>::min();
      return negative ? -static_cast<T>(lo) : static_cast<T>(lo);
#endif
    }
  }
  static void check_x(T x) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (std::is_floating_point_v<T>)
      if (!std::isfinite(x)) throw std::invalid_argument("LineContainer: nonfinite x");
#else
    (void)x;
#endif
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  typename super::const_iterator selected(T x) const {
    check_x(x);
#ifndef SKIP_RUNTIME_CHECKS
    if (empty()) throw std::out_of_range("LineContainer: empty query");
#endif
    return lower_bound(x);
  }

  bool insect_impl(typename super::iterator x, typename super::iterator y, bool& below) {
    below = false;
    if (y == end()) { x->p = inf; return false; }
    if (x->k == y->k) {
      const bool better = objective == MAXIMIZE ? x->m > y->m : x->m < y->m;
      if constexpr (std::is_integral_v<T>) {
        x->p = better ? inf : std::numeric_limits<T>::min();
        below = !better;
      }
      else x->p = better ? inf : -inf;
    } else below = intersection(*x, *y);
    return !below && !boundary_less(*x, *y);
  }

 public:
  bool insect(typename super::iterator x, typename super::iterator y) {
    bool below;
    return insect_impl(x,y,below);
  }
  void add(T k, T m) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (std::is_floating_point_v<T>)
      if (!std::isfinite(k) || !std::isfinite(m))
        throw std::invalid_argument("LineContainer: nonfinite coefficient");
#endif
    add_unchecked(k, m);
  }

 private:
  friend struct LineContainer2D;
  void add_unchecked(T k, T m) {
    if constexpr (std::is_integral_v<T>) {
      // 小さい包絡では、確保前に不要な直線を除く。
      if (super::size() <= 256) {
        Line<T> candidate{k, m, inf};
        auto z = lower_bound(candidate);
        if (z != end() && z->k == k) {
          const bool better = objective == MAXIMIZE ? m > z->m : m < z->m;
          if (!better) return;
        } else if (z != end() && intersection(candidate, *z)) return;
        auto x = z;
        const bool has_previous = z != begin();
        Line<T> previous{};
        // 下端だけで選ばれる直線と、選択域が型の範囲外の直線を区別する。
        bool previous_below = false;
        if (has_previous) {
          --x;
          previous = *x;
          previous_below = intersection(previous, candidate);
          if (!previous_below && previous.p >= candidate.p &&
              (z == end() || candidate.p < z->p)) return;
        }
        auto y = insert(z, candidate);
        while (z != end() && !boundary_less(*y, *z)) {
          z = erase(z);
          insect(y, z);
        }
        if (has_previous) {
          x->p = previous.p;
          if (!previous_below && !boundary_less(*x, *y)) insect(x, y = erase(y));
        } else x = y;
        while ((y = x) != begin() && !boundary_less(*--x, *y)) insect(x, erase(y));
        // floorが最小値の直線はその点で最適になり得る。真の域外だけ除く。
        if constexpr (std::is_integral_v<T>) {
          const auto first = begin();
          if (first->p == std::numeric_limits<T>::min()) [[unlikely]] {
            const auto next = std::next(first);
            if (next != end()) {
              bool first_below;
              insect_impl(first, next, first_below);
              if (first_below) erase(first);
            }
          }
        }
        return;
      }
    }
    auto z = insert({k, m, 0}), y = z++, x = y;
    bool below;
    while (insect_impl(y, z, below)) z = erase(z);
    if constexpr (std::is_integral_v<T>)
      if (below) { erase(y); return; }
    if (x != begin() && insect(--x, y)) insect(x, y = erase(y));
    while ((y = x) != begin() && !boundary_less(*--x, *y)) insect(x, erase(y));
    // floorが最小値の直線はその点で最適になり得る。真の域外だけ除く。
    if constexpr (std::is_integral_v<T>) {
      const auto first = begin();
      if (first->p == std::numeric_limits<T>::min()) [[unlikely]] {
        const auto next = std::next(first);
        if (next != end()) {
          bool first_below;
          insect_impl(first, next, first_below);
          if (first_below) erase(first);
        }
      }
    }
  }
 public:
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  T query(T x) const {
    if constexpr (std::is_integral_v<T>) {
#ifndef SKIP_RUNTIME_CHECKS
      if (empty()) throw std::out_of_range("LineContainer: empty query");
#endif
      const auto first = begin();
      if (x <= first->p) [[unlikely]] return evaluate(first->k, first->m, x);
      const auto line = lower_bound(x);
      return evaluate(line->k, line->m, x);
    } else {
      const auto line = selected(x);
      return evaluate(line->k, line->m, x);
    }
  }
  std::optional<T> query_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return evaluate(line->k, line->m, x);
  }
  std::pair<T,T> query_line(T x) const {
    const auto line = selected(x);
    return {line->k, line->m};
  }
  std::optional<std::pair<T,T>> query_line_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return std::pair<T,T>{line->k, line->m};
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  Result query_with_line(T x) const {
    const auto line = selected(x);
    return {evaluate(line->k, line->m, x), line->k, line->m};
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  std::optional<Result> query_with_line_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return Result{evaluate(line->k, line->m, x), line->k, line->m};
  }
};
template <typename T> using MinLineContainer = LineContainer<T, MINIMIZE>;
template <typename T> using MaxLineContainer = LineContainer<T, MAXIMIZE>;
#endif


// Library: DataStructure/line-container-2d.cpp
// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/line-container-2d.hpp.html
// Original Source: https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/LineContainer.h
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/line_container_2d.md
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/line_container.md
//
// API一覧
// [LineContainer2D]
// add(x,y): 有限の点を追加、償却O(log h)。確保失敗時は空に戻して例外を伝える。
// max_ld(a,b)/min_ld(a,b): ax+byの最大/最小をO(log h)で返す。有限係数・非空が前提。
// max_ld/min_ld(整数,整数): 値域がint64_t内の整数型は、型から保証できる有限性・比の検査を省く。
// b/aが浮動型の範囲を超える問い合わせは常にoverflow_error。
// max_ll(a,b)/min_ll(a,b): 浮動計算を±4.1e18に制限し、0方向へ切り捨てる(旧挙動)。
// max_rounded_ll(a,b)/min_rounded_ll(a,b): 同じ範囲制限後、半整数は0から遠い方へ丸める。
// max_optional(a,b)/min_optional(a,b): 空ならnullopt、それ以外はld版と同じ。
// argmax(a,b)/argmin(a,b): 最適な元の点(x,y)をO(log h)で返す。同点の点は未規定。
// max_with_point(a,b)/min_with_point(a,b): 1回の探索でResult{内積値,x,y}を返す。ld係数・非空、O(log h)。
// argmax_optional(a,b)/argmin_optional(a,b): 空ならnullopt、それ以外はarg版と同じ。
// size()/empty()/clear(): 登録点数(重複込み)・空判定・全削除、O(1)/O(1)/O(h)。
// swap(other): 2コンテナの全状態をO(1)で交換する。コピーは独立、move後の元は空。
// 公開のminlc/maxlc・座標範囲は参照用。外からの変更は不変条件を壊す。
// long doubleの丸め誤差があり、ll版も厳密な整数内積ではない。hは包絡に残る点数。
// 既定の丸めモード・型本来の精度が前提。ゼロの符号は未規定。
//
// [同梱: LineContainer<T,objective>]
// add(k,m): y=k*x+mを追加、償却O(log h)。整数は符号付き64bit以下、浮動係数は有限値。
// query(x): 最適値をO(log h)で返す。空は検査時例外、省略時は非空が前提。整数範囲外は常に例外。
// query_optional(x): 空ならnullopt、それ以外はqueryと同じ。xは浮動小数点なら有限値。
// query_line(x): 選択した元の(k,m)をO(log h)で返す。同点の係数は未規定。
// query_line_optional(x): 空ならnullopt、それ以外はquery_lineと同じ。
// query_with_line(x): 1回の探索でResult{最適値,k,m}を返す。queryと同じ前提・O(log h)。
// query_with_line_optional(x): 空ならnullopt、それ以外はquery_with_lineと同じ。
// size()/empty()/clear(): 有効な包絡直線の本数・空判定・全削除、O(1)/O(1)/O(h)。
// begin()/end()/lower_bound(x): 元係数と交点を参照。最小化の傾き順は降順。直接変更は不変条件を壊す。
// insect(x,y): 旧互換の内部交点更新。正しい包絡iterator以外では呼び出さない。
// コピーは独立した包絡。hは有効直線数。浮動小数点版の交点・最適値には丸め誤差がある。
// objectiveはMINIMIZE/MAXIMIZE。MinLineContainer<T>/MaxLineContainer<T>は最小化/最大化の別名。
//
// [Line<T>・補助関数]
// Line<T>のk,m: 元の係数。p: この直線が選ばれる右端(整数は型の範囲に制限)。
// lc_inf<T>(): 整数では最大値、浮動小数点では正の無限大を返す。
// lc_div(a,b): 整数は床除算、浮動小数点は通常除算。0除算・整数結果の範囲外は例外。

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

#ifndef SIRKIT_LINE_CONTAINER_DEFINED
#define SIRKIT_LINE_CONTAINER_DEFINED

enum Objective { MAXIMIZE = +1, MINIMIZE = -1 };

template <typename T>
struct Line {
  mutable T k, m, p;
  bool operator<(const Line& other) const { return k < other.k; }
  bool operator<(T x) const { return p < x; }
};

template <typename T>
constexpr T lc_inf() {
  if constexpr (std::is_floating_point_v<T>) return std::numeric_limits<T>::infinity();
  else return std::numeric_limits<T>::max();
}

template <typename T>
T lc_div(T a, T b) {
  if (b == 0) throw std::domain_error("lc_div: division by zero");
  if constexpr (std::is_integral_v<T>) {
    static_assert(std::is_signed_v<T>);
    if (a == std::numeric_limits<T>::min() && b == -1)
      throw std::overflow_error("lc_div: quotient overflow");
    return static_cast<T>(a / b - ((a < 0) != (b < 0) && a % b != 0));
  } else return a / b;
}

namespace reinsirk::detail {
template <typename T, Objective objective>
struct LineComparator {
  using is_transparent = void;
  bool operator()(const Line<T>& a, const Line<T>& b) const {
    if constexpr (objective == MAXIMIZE) return a.k < b.k;
    else return a.k > b.k;
  }
  bool operator()(const Line<T>& a, T x) const { return a < x; }
  bool operator()(T x, const Line<T>& a) const { return x < a.p; }
};
}  // namespace reinsirk::detail

template <typename T, Objective objective>
struct LineContainer : std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>> {
  static_assert((std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= 8) ||
                std::is_floating_point_v<T>);
  static_assert(objective == MAXIMIZE || objective == MINIMIZE);
  using super = std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>>;
  using super::begin, super::end, super::insert, super::erase;
  using super::empty, super::lower_bound;
  static constexpr T inf = lc_inf<T>();
  struct Result { T value, k, m; };

 private:
  static bool boundary_less(const Line<T>& a, const Line<T>& b) {
    return a.p < b.p;
  }
  static bool intersection(Line<T> const& a, Line<T> const& b) {
    if constexpr (std::is_integral_v<T>) {
      using U = std::make_unsigned_t<T>;
      const bool negative_n = b.m < a.m, negative_d = a.k < b.k;
      const U n = negative_n ? U(a.m) - U(b.m) : U(b.m) - U(a.m);
      const U d = negative_d ? U(b.k) - U(a.k) : U(a.k) - U(b.k);
      U q = n / d;
      constexpr U top = U(std::numeric_limits<T>::max());
      if (negative_n != negative_d) {
        q = static_cast<U>(q + U(n % d != 0));
        if (q >= top + U{1}) {
          a.p = std::numeric_limits<T>::min();
          return q > top + U{1};
        }
        else a.p = static_cast<T>(-static_cast<T>(q));
      } else a.p = q > top ? inf : static_cast<T>(q);
      return false;
    } else {
      const T n = b.m - a.m, d = a.k - b.k;
      if (std::isfinite(n + d) || (std::isfinite(n) && std::isfinite(d))) a.p = n / d;
      else if (!std::isfinite(n) && !std::isfinite(d))
        a.p = (b.m / 2 - a.m / 2) / (a.k / 2 - b.k / 2);
      else if (!std::isfinite(n)) a.p = ((b.m / 2 - a.m / 2) / d) * 2;
      else a.p = (n / (a.k / 2 - b.k / 2)) / 2;
      return false;
    }
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  static T evaluate(T k, T m, T x) {
    if constexpr (std::is_floating_point_v<T>) return k * x + m;
    else if constexpr (sizeof(T) < 8) {
      const std::int64_t result = std::int64_t(k) * x + m;
      if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
        throw std::overflow_error("LineContainer: value overflow");
      return static_cast<T>(result);
    } else {
#if defined(__SIZEOF_INT128__) && !defined(SIRKIT_LINE_CONTAINER_FORCE_PORTABLE)
      T fast;
      if (!__builtin_mul_overflow(k, x, &fast) && !__builtin_add_overflow(fast, m, &fast))
#if defined(__clang__)
        [[likely]]
#endif
        return fast;
      __extension__ using Wide = __int128;
      const Wide result = Wide(k) * x + m;
      if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
        throw std::overflow_error("LineContainer: value overflow");
      return static_cast<T>(result);
#else
      using U = std::uint64_t;
      // 32bit範囲なら積と和が64bitに収まる。
      constexpr U narrow_offset = U{1} << 31;
      if (((U(k) + narrow_offset) | (U(x) + narrow_offset) |
           (U(m) + narrow_offset)) < (U{1} << 32))
        return k * x + m;
      const U a = k < 0 ? U{0} - U(k) : U(k);
      const U b = x < 0 ? U{0} - U(x) : U(x);
      const U c = m < 0 ? U{0} - U(m) : U(m);
      constexpr U mask = 0xffffffffU;
      const U t0 = (a & mask) * (b & mask);
      const U t1 = (a >> 32) * (b & mask) + (t0 >> 32);
      const U t2 = (a & mask) * (b >> 32) + (t1 & mask);
      U hi = (a >> 32) * (b >> 32) + (t1 >> 32) + (t2 >> 32);
      U lo = (t2 << 32) | (t0 & mask);
      bool negative = (k < 0) != (x < 0);
      if (negative == (m < 0)) {
        const U previous = lo;
        lo += c;
        hi += lo < previous;
      } else if (hi != 0 || lo >= c) {
        const U previous = lo;
        lo -= c;
        hi -= previous < c;
      } else { lo = c - lo; negative = m < 0; }
      const U limit = U(std::numeric_limits<T>::max()) + U(negative);
      if (hi != 0 || lo > limit) throw std::overflow_error("LineContainer: value overflow");
      if (negative && lo == U(std::numeric_limits<T>::max()) + U{1})
        return std::numeric_limits<T>::min();
      return negative ? -static_cast<T>(lo) : static_cast<T>(lo);
#endif
    }
  }
  static void check_x(T x) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (std::is_floating_point_v<T>)
      if (!std::isfinite(x)) throw std::invalid_argument("LineContainer: nonfinite x");
#else
    (void)x;
#endif
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  typename super::const_iterator selected(T x) const {
    check_x(x);
#ifndef SKIP_RUNTIME_CHECKS
    if (empty()) throw std::out_of_range("LineContainer: empty query");
#endif
    return lower_bound(x);
  }

  bool insect_impl(typename super::iterator x, typename super::iterator y, bool& below) {
    below = false;
    if (y == end()) { x->p = inf; return false; }
    if (x->k == y->k) {
      const bool better = objective == MAXIMIZE ? x->m > y->m : x->m < y->m;
      if constexpr (std::is_integral_v<T>) {
        x->p = better ? inf : std::numeric_limits<T>::min();
        below = !better;
      }
      else x->p = better ? inf : -inf;
    } else below = intersection(*x, *y);
    return !below && !boundary_less(*x, *y);
  }

 public:
  bool insect(typename super::iterator x, typename super::iterator y) {
    bool below;
    return insect_impl(x,y,below);
  }
  void add(T k, T m) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (std::is_floating_point_v<T>)
      if (!std::isfinite(k) || !std::isfinite(m))
        throw std::invalid_argument("LineContainer: nonfinite coefficient");
#endif
    add_unchecked(k, m);
  }

 private:
  friend struct LineContainer2D;
  void add_unchecked(T k, T m) {
    if constexpr (std::is_integral_v<T>) {
      // 小さい包絡では、確保前に不要な直線を除く。
      if (super::size() <= 256) {
        Line<T> candidate{k, m, inf};
        auto z = lower_bound(candidate);
        if (z != end() && z->k == k) {
          const bool better = objective == MAXIMIZE ? m > z->m : m < z->m;
          if (!better) return;
        } else if (z != end() && intersection(candidate, *z)) return;
        auto x = z;
        const bool has_previous = z != begin();
        Line<T> previous{};
        // 下端だけで選ばれる直線と、選択域が型の範囲外の直線を区別する。
        bool previous_below = false;
        if (has_previous) {
          --x;
          previous = *x;
          previous_below = intersection(previous, candidate);
          if (!previous_below && previous.p >= candidate.p &&
              (z == end() || candidate.p < z->p)) return;
        }
        auto y = insert(z, candidate);
        while (z != end() && !boundary_less(*y, *z)) {
          z = erase(z);
          insect(y, z);
        }
        if (has_previous) {
          x->p = previous.p;
          if (!previous_below && !boundary_less(*x, *y)) insect(x, y = erase(y));
        } else x = y;
        while ((y = x) != begin() && !boundary_less(*--x, *y)) insect(x, erase(y));
        // floorが最小値の直線はその点で最適になり得る。真の域外だけ除く。
        if constexpr (std::is_integral_v<T>) {
          const auto first = begin();
          if (first->p == std::numeric_limits<T>::min()) [[unlikely]] {
            const auto next = std::next(first);
            if (next != end()) {
              bool first_below;
              insect_impl(first, next, first_below);
              if (first_below) erase(first);
            }
          }
        }
        return;
      }
    }
    auto z = insert({k, m, 0}), y = z++, x = y;
    bool below;
    while (insect_impl(y, z, below)) z = erase(z);
    if constexpr (std::is_integral_v<T>)
      if (below) { erase(y); return; }
    if (x != begin() && insect(--x, y)) insect(x, y = erase(y));
    while ((y = x) != begin() && !boundary_less(*--x, *y)) insect(x, erase(y));
    // floorが最小値の直線はその点で最適になり得る。真の域外だけ除く。
    if constexpr (std::is_integral_v<T>) {
      const auto first = begin();
      if (first->p == std::numeric_limits<T>::min()) [[unlikely]] {
        const auto next = std::next(first);
        if (next != end()) {
          bool first_below;
          insect_impl(first, next, first_below);
          if (first_below) erase(first);
        }
      }
    }
  }
 public:
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  T query(T x) const {
    if constexpr (std::is_integral_v<T>) {
#ifndef SKIP_RUNTIME_CHECKS
      if (empty()) throw std::out_of_range("LineContainer: empty query");
#endif
      const auto first = begin();
      if (x <= first->p) [[unlikely]] return evaluate(first->k, first->m, x);
      const auto line = lower_bound(x);
      return evaluate(line->k, line->m, x);
    } else {
      const auto line = selected(x);
      return evaluate(line->k, line->m, x);
    }
  }
  std::optional<T> query_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return evaluate(line->k, line->m, x);
  }
  std::pair<T,T> query_line(T x) const {
    const auto line = selected(x);
    return {line->k, line->m};
  }
  std::optional<std::pair<T,T>> query_line_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return std::pair<T,T>{line->k, line->m};
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  Result query_with_line(T x) const {
    const auto line = selected(x);
    return {evaluate(line->k, line->m, x), line->k, line->m};
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  std::optional<Result> query_with_line_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return Result{evaluate(line->k, line->m, x), line->k, line->m};
  }
};
template <typename T> using MinLineContainer = LineContainer<T, MINIMIZE>;
template <typename T> using MaxLineContainer = LineContainer<T, MAXIMIZE>;
#endif

#ifndef SIRKIT_LINE_CONTAINER_2D_DEFINED
#define SIRKIT_LINE_CONTAINER_2D_DEFINED

struct LineContainer2D {
  using ld = long double;
  using ll = std::int64_t;
  struct Result { ld value, x, y; };
  static constexpr ld INF = 4.1e18L;
  static constexpr ll INTEGER_LIMIT = 4'100'000'000'000'000'000;
  ld Xmax = -std::numeric_limits<ld>::infinity();
  ld Xmin = std::numeric_limits<ld>::infinity();
  ld Ymax = -std::numeric_limits<ld>::infinity();
  ld Ymin = std::numeric_limits<ld>::infinity();
  MinLineContainer<ld> minlc;
  MaxLineContainer<ld> maxlc;

 private:
  std::size_t count_ = 0;
  // 照会では状態を変えず、包絡が変わった追加後にだけ更新する。
  // 算術の代替指定と、処理系に合わせたキャッシュ選択を分ける。
#if (defined(__clang__) || defined(__GNUC__))
  static constexpr bool cache_points = true;
#else
  static constexpr bool cache_points = false;
#endif
  struct NoCache {};
  struct HintEntry {
    const Line<ld>* line = nullptr;
    const Line<ld>* previous = nullptr;
  };
  struct CachedHintEntry : HintEntry { Line<ld> cached{}; };
  struct AdjacentLines {
    const Line<ld>* before = nullptr;
    const Line<ld>* after = nullptr;
  };
  struct SearchHint {
    using Entry = std::conditional_t<cache_points, CachedHintEntry, HintEntry>;
    Entry positive, negative;
    // 0: 別区間、1/2: 全包絡、3: 共通区間、4/5: 境界が+1/-1の2本。
    unsigned small = 0;
    [[no_unique_address]] std::conditional_t<cache_points, AdjacentLines, NoCache> adjacent{};
    [[no_unique_address]] std::conditional_t<cache_points, Line<ld>, NoCache> after_cached{};
  } min_hint_, max_hint_;
  template <class Hull, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static void refresh_hint(const Hull& hull, Hint& hint) {
    if constexpr (cache_points) {
      if (hull.empty()) { hint = {}; return; }
      hint.small = 0;
      hint.positive.previous = nullptr;
      hint.negative.line = nullptr;
      hint.negative.previous = nullptr;
      hint.adjacent = {};
    } else {
      hint = {};
      if (hull.empty()) return;
    }
    if (hull.size() <= 2) {
      // 2本なら先頭の右端は有限なので、+inf検索が必ず末尾を選ぶ。
      const auto first = hull.begin();
      hint.positive.line = &*first;
      hint.negative.line = hull.size() == 2 ? &*hull.lower_bound(lc_inf<ld>()) : &*first;
      if constexpr (cache_points) {
        // 1/2本では境界も更新する。3の境界には木の値を使う。
        { hint.positive.cached.k = (*hint.positive.line).k; hint.positive.cached.m = (*hint.positive.line).m; hint.positive.cached.p = (*hint.positive.line).p; }
        { hint.negative.cached.k = (*hint.negative.line).k; hint.negative.cached.m = (*hint.negative.line).m; }
      }
      hint.small = static_cast<unsigned>(hull.size());
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
      if constexpr (cache_points && std::numeric_limits<ld>::radix == 2 &&
                    std::numeric_limits<ld>::is_iec559 &&
                    std::is_same_v<Hull, MinLineContainer<ld>>) {
        if (hint.small == 2) {
          if (std::abs(hint.positive.cached.p) == ld{1})
            hint.small = std::signbit(hint.positive.cached.p) ? 5 : 4;
        }
      }
#endif
      return;
    }
    const auto positive = hull.lower_bound(ld{0.5});
    hint.positive.line = &*positive;
    if constexpr (cache_points) { hint.positive.cached.k = (*positive).k; hint.positive.cached.m = (*positive).m; }
    hint.positive.previous = positive == hull.begin() ? nullptr : &*std::prev(positive);
    if (!hint.positive.previous || hint.positive.previous->p < ld{-0.5}) {
      hint.small = 3;
      if constexpr (cache_points) {
        if (positive != hull.begin()) {
          const auto prev = std::prev(positive);
          hint.adjacent.before = prev == hull.begin() ? nullptr : &*std::prev(prev);
          { hint.negative.cached.k = (*prev).k; hint.negative.cached.m = (*prev).m; }
        }
        const auto next = std::next(positive);
        hint.adjacent.after = next == hull.end() ? nullptr : &*next;
        if (hint.adjacent.after) { hint.after_cached.k = (*hint.adjacent.after).k; hint.after_cached.m = (*hint.adjacent.after).m; }
      }
      return;
    }
    const auto negative = hull.lower_bound(ld{-0.5});
    hint.negative.line = &*negative;
    if constexpr (cache_points) { hint.negative.cached.k = (*negative).k; hint.negative.cached.m = (*negative).m; }
    hint.negative.previous = negative == hull.begin() ? nullptr : &*std::prev(negative);
    if (positive == negative) hint.small = 3;
  }
  void refresh_hints() {
    refresh_hint(minlc, min_hint_);
    refresh_hint(maxlc, max_hint_);
  }
  template <bool use_shared = false, bool copy = false, class Hull, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static std::conditional_t<copy, Line<ld>, const Line<ld>&> hinted_line(const Hull& hull, const Hint& hint, ld c) {
    if (hint.small == 3) {
      const auto& entry = hint.positive;
      if constexpr (cache_points) {
        // 中央の前後も、同じ境界を再利用して判定する。
        if (c <= entry.line->p) {
          if (entry.previous && c <= entry.previous->p) {
            if (!hint.adjacent.before || hint.adjacent.before->p < c) return hint.negative.cached;
          } else return entry.cached;
        } else if (hint.adjacent.after && c <= hint.adjacent.after->p) return hint.after_cached;
      } else if constexpr (use_shared) {
        if ((!entry.previous || entry.previous->p < c) && c <= entry.line->p) return *entry.line;
      }
      return *hull.lower_bound(c);
    }
    if (hint.small) {
      if constexpr (cache_points)
        return hint.positive.cached.p < c ? hint.negative.cached : hint.positive.cached;
      else return hint.positive.line->p < c ? *hint.negative.line : *hint.positive.line;
    }
    const auto& entry = c >= 0 ? hint.positive : hint.negative;
    if ((!entry.previous || entry.previous->p < c) && c <= entry.line->p) {
      if constexpr (cache_points) return entry.cached;
      else return *entry.line;
    }
    return *hull.lower_bound(c);
  }
  // 不要な追加を確保前に棄却し、照会キャッシュの更新要否を返す。
  template <Objective objective, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static bool add_hull(LineContainer<ld, objective>& hull, ld k, ld m, const Hint& hint) {
    using Hull = LineContainer<ld, objective>;
    Line<ld> candidate{k, m, lc_inf<ld>()};
    auto z = hull.lower_bound(candidate);
    if (z != hull.end() && z->k == k) {
      const bool better = objective == MAXIMIZE ? m > z->m : m < z->m;
      if (!better) return false;
    }
    auto x = z;
    bool has_previous = z != hull.begin();
    Line<ld> previous{};
    if (z != hull.end()) {
      if (z->k == k) candidate.p = lc_inf<ld>();
      else Hull::intersection(candidate, *z);
    }
    if (has_previous) {
      --x;
      previous = *x;
      Hull::intersection(previous, candidate);
      if (z == hull.end() || candidate.p < z->p) {
        if (previous.p >= candidate.p) return false;
      }
    }
    // 共通区間の外だけが変わる場合は、既存の点・境界参照を保つ。
    bool refresh = true;
    ld lower = 0, upper = 0;
    if constexpr (cache_points) {
      if (hint.small == 3) {
        const auto* first = hint.adjacent.before;
        const auto* last = hint.adjacent.after ? hint.adjacent.after : hint.positive.line;
        lower = first ? first->k : (objective == MAXIMIZE ? -lc_inf<ld>() : lc_inf<ld>());
        upper = last->k;
        refresh = objective == MAXIMIZE ? lower <= k && k <= upper : upper <= k && k <= lower;
      }
    } else (void)hint;
    const auto mark_removed = [&](const Line<ld>& line) {
      if constexpr (cache_points) {
        if (!refresh) refresh = objective == MAXIMIZE ? lower <= line.k && line.k <= upper : upper <= line.k && line.k <= lower;
      }
    };
    auto y = hull.insert(z, candidate);
    bool removed_successor = false, changed = true;
    while (z != hull.end() && !Hull::boundary_less(*y, *z)) {
      mark_removed(*z);
      z = hull.erase(z);
      removed_successor = true;
      hull.insect(y, z);
    }
    if (has_previous) {
      x->p = previous.p;
      if (!Hull::boundary_less(*x, *y)) {
        mark_removed(*y);
        hull.insect(x, y = hull.erase(y));
        changed = removed_successor;
      }
    } else x = y;
    while ((y = x) != hull.begin() && !Hull::boundary_less(*--x, *y)) {
      mark_removed(*y);
      hull.insect(x, hull.erase(y));
      changed = true;
    }
    return changed && refresh;
  }
  static void check_coefficients(ld a, ld b) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!std::isfinite(a) || !std::isfinite(b))
      throw std::invalid_argument("LineContainer2D: nonfinite coefficient");
#else
    (void)a; (void)b;
#endif
  }
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::noinline, gnu::cold]]
#endif
  static ld recover_line_dot(ld a, ld b, const Line<ld>* line) {
    // 巨大な有限積同士の相殺でもinf-infを返さない。
    const ld x = line->m, y = line->k;
    int ea = 0, eb = 0, ex = 0, ey = 0;
    std::frexp(a, &ea); std::frexp(b, &eb);
    std::frexp(x, &ex); std::frexp(y, &ey);
    const int e1 = std::max(ea, eb), e2 = std::max(ex, ey);
    const ld sa = std::ldexp(a, -e1), sb = std::ldexp(b, -e1);
    const ld sx = std::ldexp(x, -e2), sy = std::ldexp(y, -e2);
    return std::ldexp(sa * sx + sb * sy, e1 + e2);
  }
  static ll to_rounded_integer(ld value) {
    const ld limited = std::clamp(value, -INF, INF);
    const ll integral = static_cast<ll>(limited);
    const ld fraction = limited - ld(integral);
    return integral + (fraction >= 0.5L) - (fraction <= -0.5L);
  }
  template <bool ratio_is_finite = false>
  std::pair<ld, ld> select(ld a, ld b) const {
    if (a == 0) {
      if constexpr (!ratio_is_finite) check_coefficients(a, b);
      const auto it = b > 0 ? std::prev(maxlc.end()) : maxlc.begin();
      return {it->m, it->k};
    }
    if (b == 0) {
      if constexpr (!ratio_is_finite) check_coefficients(a, b);
      const auto& line = a > 0 ? *maxlc.lower_bound(ld{0}) : *minlc.lower_bound(ld{0});
      return {line.m, line.k};
    }
    return select_nonzero<ratio_is_finite>(a, b);
  }
  [[noreturn]] static void slope_error(ld a, ld b) {
    check_coefficients(a, b);
    throw std::overflow_error("LineContainer2D: query slope overflow");
  }
  template <bool ratio_is_finite>
  std::pair<ld, ld> select_nonzero(ld a, ld b) const {
    const ld c = b / a;
    if constexpr (!ratio_is_finite) {
#ifndef SKIP_RUNTIME_CHECKS
      // 有限aと有限b / aからbの有限性も分かる。bを重ねて検査しない。
      if (!std::isfinite(a) || !std::isfinite(c)) [[unlikely]] slope_error(a, b);
#else
      if (!std::isfinite(c)) [[unlikely]] slope_error(a, b);
#endif
    }
    const auto& line = [&]() -> const Line<ld>& {
      if constexpr (ratio_is_finite) return a > 0 ? *maxlc.lower_bound(c) : *minlc.lower_bound(c);
      else return a > 0 ? hinted_line<ratio_is_finite>(maxlc, max_hint_, c) : hinted_line<ratio_is_finite>(minlc, min_hint_, c);
    }();
    return {line.m, line.k};
  }
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::always_inline]]
#endif
  void check_empty() const {
#ifndef SKIP_RUNTIME_CHECKS
    if (empty()) throw std::out_of_range("LineContainer2D: empty query");
#endif
  }

#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
  [[gnu::noinline, gnu::cold]]
  static void check_small_ratio(ld a, ld b) {
    if (!std::isfinite(b / a)) slope_error(a, b);
  }
#endif
  template <bool proof, bool negative_a = false, bool optimized_float = false, class Hull, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static ld nonzero_value(const Hull& hull, const Hint& hint, ld a, ld b) {
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    // 単位境界の比較では除算を省き、小さいaの商溢れだけ別途確認する。
    if constexpr (optimized_float && !proof && cache_points &&
                  std::numeric_limits<ld>::radix == 2 && std::numeric_limits<ld>::is_iec559 &&
                  std::is_same_v<Hull, MinLineContainer<ld>>) {
      if (hint.small >= 4) {
        if (std::abs(a) < ld{1}) [[unlikely]] check_small_ratio(a, b);
        const ld boundary = hint.small == 4 ? a : -a;
        const bool right = negative_a ? b < boundary : b > boundary;
        const auto l = right ? hint.negative.cached : hint.positive.cached;
        const ld result = a * l.m + b * l.k;
        if (std::isnan(result)) [[unlikely]] return recover_line_dot(a, b, &l);
        return result;
      }
    }
#endif
    const ld c = b / a;
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    const auto& l = hinted_line<(cache_points || proof), optimized_float && !proof>(hull,hint,c);
#else
    const auto& l = hinted_line<(cache_points || proof)>(hull, hint, c);
#endif
    const ld result = a * l.m + b * l.k;
    // 有限cと有限resultならa, bも有限。例外経路だけ個別検査する。
    if constexpr (proof) {
      if (std::isnan(result)) [[unlikely]] return recover_line_dot(a, b, &l);
    } else if constexpr (cache_points) {
      if (!(std::abs(c + result) <= std::numeric_limits<ld>::max())) [[unlikely]]
        return checked_result(a, b, c, result, &l);
    } else if (!std::isfinite(c + result)) [[unlikely]]
      return checked_result(a, b, c, result, &l);
    return result;
  }
  template <bool ratio_is_finite = false, bool optimized_float = false>
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::always_inline]]
#endif
  ld value(ld a, ld b) const {
    if (a > 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmax;
      }
      return nonzero_value<ratio_is_finite, false, optimized_float>(maxlc, max_hint_, a, b);
    }
    if (a < 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmin;
      }
      return nonzero_value<ratio_is_finite, true, optimized_float>(minlc, min_hint_, a, b);
    }
    if constexpr (!ratio_is_finite) check_coefficients(a, b);
    return b == 0 ? 0 : b * (b > 0 ? Ymax : Ymin);
  }
#if defined(__clang__)
  template <bool ratio_is_finite = false, bool optimized_float = false>
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::always_inline]]
#endif
  ld minimum_value(ld a, ld b) const {
    if (a > 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmin;
      }
      return nonzero_value<ratio_is_finite, false, optimized_float>(minlc, min_hint_, a, b);
    }
    if (a < 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmax;
      }
      return nonzero_value<ratio_is_finite, true, optimized_float>(maxlc, max_hint_, a, b);
    }
    if constexpr (!ratio_is_finite) check_coefficients(a, b);
    return b == 0 ? -ld{0} : b * (b > 0 ? Ymin : Ymax);
  }
#endif
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::noinline, gnu::cold]]
#endif
  static ld checked_result(ld a, ld b, ld c, ld result, const Line<ld>* line) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!std::isfinite(a) || !std::isfinite(c)) slope_error(a, b);
#else
    if (!std::isfinite(c)) slope_error(a, b);
#endif
    return std::isnan(result) ? recover_line_dot(a, b, line) : result;
  }
  template <bool minimum>
  ld integer_value(ll a, ll b) const {
#ifndef SKIP_RUNTIME_CHECKS
    if (empty()) throw std::out_of_range("LineContainer2D: empty query");
#endif
    // int64係数の変換と非零分母との比は必ず有限。重複する浮動検査を省く。
    if constexpr (minimum) return -value<true>(-ld(a), -ld(b));
    else return value<true>(ld(a), ld(b));
  }

#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
  [[noreturn, gnu::noinline, gnu::cold]]
  static void count_error() { throw std::length_error("LineContainer2D: point count overflow"); }
#endif
 public:
  LineContainer2D() = default;
  LineContainer2D(const LineContainer2D& other)
      : Xmax(other.Xmax), Xmin(other.Xmin), Ymax(other.Ymax), Ymin(other.Ymin),
        minlc(other.minlc), maxlc(other.maxlc), count_(other.count_) { refresh_hints(); }
  LineContainer2D& operator = (const LineContainer2D& other) {
    if (this != &other) { LineContainer2D copy(other); swap(copy); }
    return *this;
  }
  LineContainer2D(LineContainer2D&& other) noexcept
      : Xmax(other.Xmax), Xmin(other.Xmin), Ymax(other.Ymax), Ymin(other.Ymin),
        minlc(std::move(other.minlc)), maxlc(std::move(other.maxlc)), count_(other.count_),
        min_hint_(other.min_hint_), max_hint_(other.max_hint_) {
    other.clear();
  }
  LineContainer2D& operator = (LineContainer2D&& other) noexcept {
    if (this != &other) {
      minlc = std::move(other.minlc); maxlc = std::move(other.maxlc);
      Xmax = other.Xmax; Xmin = other.Xmin; Ymax = other.Ymax; Ymin = other.Ymin;
      count_ = other.count_;
      min_hint_ = other.min_hint_; max_hint_ = other.max_hint_;
      other.clear();
    }
    return *this;
  }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  void add(ld x, ld y) {
    check_coefficients(x, y);
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    if (count_ == std::numeric_limits<std::size_t>::max()) [[unlikely]] count_error();
#else
    if (count_ == std::numeric_limits<std::size_t>::max())
      throw std::length_error("LineContainer2D: point count overflow");
#endif
    bool refresh_min, refresh_max;
    try {
      refresh_min = add_hull(minlc, y, x, min_hint_);
      refresh_max = add_hull(maxlc, y, x, max_hint_);
    }
    catch (...) { clear(); throw; }
    if (refresh_min) refresh_hint(minlc, min_hint_);
    if (refresh_max) refresh_hint(maxlc, max_hint_);
    Xmax = std::max(Xmax, x); Xmin = std::min(Xmin, x);
    Ymax = std::max(Ymax, y); Ymin = std::min(Ymin, y);
    ++count_;
  }
  template <class A, class B>
    requires (std::is_integral_v<A> && std::is_integral_v<B> && std::numeric_limits<A>::digits <= 63 &&
              std::numeric_limits<B>::digits <= 63)
  ld max_ld(A a, B b) const { return integer_value<false>(a, b); }
  template <class A, class B>
    requires (std::is_integral_v<A> && std::is_integral_v<B> && std::numeric_limits<A>::digits <= 63 &&
              std::numeric_limits<B>::digits <= 63)
  ld min_ld(A a, B b) const { return integer_value<true>(a, b); }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  ld max_ld(ld a, ld b) const {
    check_empty();
    return value<false, true>(a, b);
  }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  ld min_ld(ld a, ld b) const {
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    check_empty(); return minimum_value<false, true>(a,b);
#else
    return -max_ld(-a,-b);
#endif
  }
  ll max_ll(ll a, ll b) const { return static_cast<ll>(std::clamp(integer_value<false>(a, b), -INF, INF)); }
  ll min_ll(ll a, ll b) const { return static_cast<ll>(std::clamp(integer_value<true>(a, b), -INF, INF)); }
  ll max_rounded_ll(ll a, ll b) const { return to_rounded_integer(integer_value<false>(a, b)); }
  ll min_rounded_ll(ll a, ll b) const { return to_rounded_integer(integer_value<true>(a, b)); }
  Result max_with_point(ld a, ld b) const {
    check_empty();
    const auto [x, y] = select(a, b);
    ld result = a * x + b * y;
    if (std::isnan(result)) [[unlikely]] {
      const Line<ld> line{y, x, 0};
      result = recover_line_dot(a, b, &line);
    }
    return {result, x, y};
  }
  Result min_with_point(ld a, ld b) const {
    const auto result = max_with_point(-a, -b);
    return {-result.value, result.x, result.y};
  }
  std::pair<ld, ld> argmax(ld a, ld b) const { check_empty(); return select(a, b); }
  std::pair<ld, ld> argmin(ld a, ld b) const { return argmax(-a, -b); }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  std::optional<ld> max_optional(ld a, ld b) const {
    if (empty()) { check_coefficients(a, b); return std::nullopt; }
    return value(a, b);
  }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  std::optional<ld> min_optional(ld a, ld b) const {
    return max_optional(-a,-b).transform([](ld v){return -v;});
  }
  std::optional<std::pair<ld, ld>> argmax_optional(ld a, ld b) const {
    if (empty()) { check_coefficients(a, b); return std::nullopt; }
    return select(a, b);
  }
  std::optional<std::pair<ld, ld>> argmin_optional(ld a, ld b) const { return argmax_optional(-a, -b); }
  void swap(LineContainer2D& other) noexcept {
    minlc.swap(other.minlc); maxlc.swap(other.maxlc);
    std::swap(Xmax, other.Xmax); std::swap(Xmin, other.Xmin);
    std::swap(Ymax, other.Ymax); std::swap(Ymin, other.Ymin);
    std::swap(count_, other.count_);
    std::swap(min_hint_, other.min_hint_); std::swap(max_hint_, other.max_hint_);
  }
  std::size_t size() const noexcept { return count_; }
  bool empty() const noexcept { return count_ == 0; }
  void clear() noexcept {
    minlc.clear(); maxlc.clear(); count_ = 0;
    min_hint_ = {}; max_hint_ = {};
    Xmax = Ymax = -std::numeric_limits<ld>::infinity();
    Xmin = Ymin = std::numeric_limits<ld>::infinity();
  }
};
#endif


// Library: DataStructure/heap.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/heap.md
//
// API一覧
// [Heap]
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


// Library: DataStructure/implicit_treap.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/implicit_treap.md
//
// API一覧
// [lazy_implicit_treap]
// 可変長列。通常操作は期待 O(log N)、erase は O(削除数 + log N)、領域 O(最大要素数)。
// op は結合的、e は単位元。mapping は集約に分配し、composition(f,g) は g の後に f。
// op・mapping等の計算時間をO(1)として記す。
// 非可換 op では順方向・逆方向を保持する。添字と区間は 0 始まり・半開区間。
// 第8引数 Commutative=true は op の可換性を指定する。既知の既定 op は自動認識する。
// lazy_implicit_treap() は空列、(vector<S>) は O(N) で列を構築する。
// コピーは O(スロット数) で独立した所有。コピー代入失敗時は元の列を保持する。
// ムーブ元は空列、swap は O(1)。
// size() / empty() は要素数・空判定を O(1) で返す。
// clear() は O(スロット数) で要素を破棄し、確保済み容量を保持する。
// reserve(n) は容量を確保する。int の範囲を超える要求は例外。
// get(pos) / set(pos,value) は要素の取得・置換。0 <= pos < size()。
// insert_at(pos,value) は位置 pos の直前へ挿入。0 <= pos <= size()。
// push_back(value) / push_front(value) は末尾・先頭へ追加する。
// erase(l,r) は区間を削除。不正な区間は検査有効時に何もしない。
// erase_at(pos) は1要素を削除。0 <= pos < size()。
// pop_back() / pop_front() は末尾・先頭を削除。空列なら何もしない。
// apply(l,r,f) は区間に作用 f を適用する。空区間は何もしない。
// prod(l,r) は順序を保って集約し、空区間は e()。all_prod() は O(1)。
// reverse(l,r) は区間を反転。rotate(l,m,r) は [l,m),[m,r) を交換。l <= m <= r。
// to_vector() は現在の列を O(N) で返す。
// 以下の比較 API は列全体が operator< による昇順であることが前提。
// lower_bound(value) / upper_bound(value) は値以上・値より大きい最初の添字、なければ size()。
// get_rank(value) は厳密に小さい要素数、count(value) は < による等価要素数。
// contains(value) は lower_bound 位置を == で判定。== がない型は < による等価判定。
// insert_sorted(value) は昇順を維持して追加。erase_val(value) は contains と同じ一致判定で1個削除。
// erase_max() は最大要素を1個削除。空列なら何もしない。
// max_right(l,g) / max_right<g>(l) は g(prod(l,r)) が真となる最大 r。
// min_left(r,g) / min_left<g>(r) は g(prod(l,r)) が真となる最小 l。
// 境界探索は g(e()) が真で区間伸長に単調であること。検査省略時の不正入力は未定義。
//
// [既定の作用: RangeAddMin]
// 区間加算・最小値。値と加算後の値は INF 以下、各演算は int64_t に収まること。
// op(a,b)/e(): 最小値/単位元。mapping(f,x): 加算。composition(f,g)/id(): 加算の合成/0。
//
// [既定の作用: RangeAddSum]
// 区間加算・和。各要素は S{値, 1} とし、演算結果は int64_t に収まること。
// op(a,b)/e(): 和と個数の合計/空。mapping(f,x): 各要素へ加算。composition(f,g)/id(): 合成/0。

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_RANGE_ADD_MIN_DEFINED
#define SIRKIT_RANGE_ADD_MIN_DEFINED
struct RangeAddMin {
    using S = std::int64_t;
    using F = std::int64_t;
    static constexpr S INF = INT64_C(8000000000000000000);
    static S op(S a, S b) { return std::min(a, b); }
    static S e() { return INF; }
    static S mapping(F f, S x) { return x + f; }
    static F composition(F f, F g) { return f + g; }
    static F id() { return 0; }
};
#endif

#ifndef SIRKIT_RANGE_ADD_SUM_DEFINED
#define SIRKIT_RANGE_ADD_SUM_DEFINED
struct RangeAddSum {
    struct S {
        std::int64_t sum, size;
        friend bool operator<(S a, S b) { return a.sum < b.sum; }
        friend bool operator==(S a, S b) { return a.sum == b.sum; }
    };
    using F = std::int64_t;
    static S op(S a, S b) { return {a.sum + b.sum, a.size + b.size}; }
    static S e() { return {0, 0}; }
    static S mapping(F f, S x) { return {x.sum + f * x.size, x.size}; }
    static F composition(F f, F g) { return f + g; }
    static F id() { return 0; }
};
#endif

#ifndef SIRKIT_IMPLICIT_TREAP_DEFINED
#define SIRKIT_IMPLICIT_TREAP_DEFINED
template<class S = RangeAddMin::S,
         S (*op)(S, S) = RangeAddMin::op,
         S (*e)() = RangeAddMin::e,
         class F = RangeAddMin::F,
         S (*mapping)(F, S) = RangeAddMin::mapping,
         F (*composition)(F, F) = RangeAddMin::composition,
         F (*id)() = RangeAddMin::id,
         bool Commutative = false>
class lazy_implicit_treap {
    static constexpr bool commutative_ = Commutative || [] {
        if constexpr (std::is_same_v<S, RangeAddMin::S>) return op == RangeAddMin::op;
        else if constexpr (std::is_same_v<S, RangeAddSum::S>) return op == RangeAddSum::op;
        else return false;
    }();
    struct ReverseAggregate {
        S value;
        explicit ReverseAggregate(S&& x) : value(std::move(x)) {}
    };
    struct EmptyAggregate { explicit EmptyAggregate(S&&) {} };
    struct Node {
        S value, forward;
        [[no_unique_address]] std::conditional_t<commutative_, EmptyAggregate, ReverseAggregate> backward;
        F action;
        std::uint64_t priority;
        int left = -1, right = -1, length = 1;
        bool pending = false, reversed = false;
        Node(S x, std::uint64_t p)
            : value(x), forward(x), backward(std::move(x)), action(id()), priority(p) {}
    };
    std::vector<Node> nodes_;
    int root_ = -1, free_ = -1;
    std::uint64_t random_ = seed();

    static std::uint64_t seed() {
        return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    std::uint64_t priority() {
        // 平方根に由来する定数を選んだ局所的な線形合同法。下位ビットを添字に使わない。
        random_ = random_ * UINT64_C(0x6a09e667f3bcc909) + UINT64_C(0xbb67ae8584caa73b);
        return random_;
    }
    int length(int p) const { return p < 0 ? 0 : nodes_[p].length; }
    S forward(int p) const { return p < 0 ? e() : nodes_[p].forward; }
    S backward(int p) const {
        if constexpr (commutative_) return forward(p);
        else return p < 0 ? e() : nodes_[p].backward.value;
    }
    void pull(int p) {
        Node& n = nodes_[p];
        n.length = 1 + length(n.left) + length(n.right);
        n.forward = op(op(forward(n.left), n.value), forward(n.right));
        if constexpr (!commutative_)
            n.backward.value = op(op(backward(n.right), n.value), backward(n.left));
    }
    void act(int p, F f) {
        if (p < 0) return;
        Node& n = nodes_[p];
        n.value = mapping(f, n.value);
        n.forward = mapping(f, n.forward);
        if constexpr (!commutative_) n.backward.value = mapping(f, n.backward.value);
        n.action = n.pending ? composition(f, n.action) : f;
        n.pending = true;
    }
    void flip(int p) {
        if (p < 0) return;
        Node& n = nodes_[p];
        std::swap(n.left, n.right);
        if constexpr (!commutative_) std::swap(n.forward, n.backward.value);
        n.reversed = !n.reversed;
    }
    void push(int p) {
        Node& n = nodes_[p];
        if (n.pending) {
            act(n.left, n.action);
            act(n.right, n.action);
            n.pending = false;
        }
        if (n.reversed) {
            flip(n.left);
            flip(n.right);
            n.reversed = false;
        }
    }
    std::pair<int, int> split(int p, int k) {
        if (p < 0) return {-1, -1};
        push(p);
        const int left_size = length(nodes_[p].left);
        if (k <= left_size) {
            auto [a, b] = split(nodes_[p].left, k);
            nodes_[p].left = b;
            pull(p);
            return {a, p};
        }
        auto [a, b] = split(nodes_[p].right, k - left_size - 1);
        nodes_[p].right = a;
        pull(p);
        return {p, b};
    }
    int merge(int a, int b) {
        if (a < 0) return b;
        if (b < 0) return a;
        if (nodes_[a].priority >= nodes_[b].priority) {
            push(a);
            nodes_[a].right = merge(nodes_[a].right, b);
            pull(a);
            return a;
        }
        push(b);
        nodes_[b].left = merge(a, nodes_[b].left);
        pull(b);
        return b;
    }
    std::pair<int, int> split_sorted(int p, const S& value) {
        if (p < 0) return {-1, -1};
        push(p);
        if (nodes_[p].value < value) {
            auto [a, b] = split_sorted(nodes_[p].right, value);
            nodes_[p].right = a;
            pull(p);
            return {p, b};
        }
        auto [a, b] = split_sorted(nodes_[p].left, value);
        nodes_[p].left = b;
        pull(p);
        return {a, p};
    }
    int insert_sorted_node(int p, int fresh) {
        if (p < 0) return fresh;
        if (nodes_[fresh].priority > nodes_[p].priority) {
            auto [a, b] = split_sorted(p, nodes_[fresh].value);
            nodes_[fresh].left = a;
            nodes_[fresh].right = b;
            pull(fresh);
            return fresh;
        }
        push(p);
        if (nodes_[p].value < nodes_[fresh].value)
            nodes_[p].right = insert_sorted_node(nodes_[p].right, fresh);
        else nodes_[p].left = insert_sorted_node(nodes_[p].left, fresh);
        pull(p);
        return p;
    }
    int insert_node(int p, int fresh, int position) {
        if (p < 0) return fresh;
        if (nodes_[fresh].priority > nodes_[p].priority) {
            auto [left, right] = split(p, position);
            nodes_[fresh].left = left;
            nodes_[fresh].right = right;
            pull(fresh);
            return fresh;
        }
        push(p);
        const int left_size = length(nodes_[p].left);
        if (position <= left_size) nodes_[p].left = insert_node(nodes_[p].left, fresh, position);
        else nodes_[p].right = insert_node(nodes_[p].right, fresh, position - left_size - 1);
        pull(p);
        return p;
    }
    int allocate(S value) {
        if (free_ >= 0) {
            int p = free_;
            free_ = nodes_[p].left;
            nodes_[p] = Node(std::move(value), priority());
            return p;
        }
        if (nodes_.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("lazy_implicit_treap capacity");
        nodes_.emplace_back(std::move(value), priority());
        return static_cast<int>(nodes_.size()) - 1;
    }
    void recycle(int p) {
        nodes_[p] = Node(e(), 0);
        nodes_[p].left = free_;
        free_ = p;
    }
    int erase_node(int p, int position, int& removed) {
        push(p);
        const int left_size = length(nodes_[p].left);
        if (position < left_size) nodes_[p].left = erase_node(nodes_[p].left, position, removed);
        else if (position > left_size) nodes_[p].right = erase_node(nodes_[p].right, position - left_size - 1, removed);
        else {
            const int result = merge(nodes_[p].left, nodes_[p].right);
            removed = p;
            return result;
        }
        pull(p);
        return p;
    }
    void release(int p) {
        if (p < 0) return;
        release(nodes_[p].left);
        release(nodes_[p].right);
        // 値と遅延作用を単位元へ戻して、スロットを再利用する。
        recycle(p);
    }
    void check_position(int p, bool end_allowed = false) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (p < 0 || p > size() || (!end_allowed && p == size()))
            throw std::out_of_range("lazy_implicit_treap position");
#else
        (void)p;
        (void)end_allowed;
#endif
    }
    void check_range(int l, int r) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (l < 0 || l > r || r > size())
            throw std::out_of_range("lazy_implicit_treap range");
#else
        (void)l;
        (void)r;
#endif
    }
    void apply_range(int p, int l, int r, const F& f) {
        if (l == 0 && r == length(p)) { act(p, f); return; }
        push(p);
        const int k = length(nodes_[p].left);
        if (l < k) apply_range(nodes_[p].left, l, std::min(r, k), f);
        if (l <= k && k < r) nodes_[p].value = mapping(f, nodes_[p].value);
        if (r > k + 1) apply_range(nodes_[p].right, std::max(0, l - k - 1), r - k - 1, f);
        pull(p);
    }
    S query(int p, int l, int r) {
        if (l == 0 && r == length(p)) return nodes_[p].forward;
        push(p);
        const int k = length(nodes_[p].left);
        if (r <= k) return query(nodes_[p].left, l, r);
        if (l > k) return query(nodes_[p].right, l - k - 1, r - k - 1);
        S result = l < k ? query(nodes_[p].left, l, k) : e();
        result = op(result, nodes_[p].value);
        if (r > k + 1) result = op(result, query(nodes_[p].right, 0, r - k - 1));
        return result;
    }
    template<class G>
    bool right_walk(int p, int start, int offset, S& acc, G& g, int& result) {
        if (p < 0 || offset + length(p) <= start) return true;
        if (start <= offset) {
            S next = op(acc, nodes_[p].forward);
            if (g(next)) { acc = std::move(next); return true; }
        }
        push(p);
        const int middle = offset + length(nodes_[p].left);
        if (!right_walk(nodes_[p].left, start, offset, acc, g, result)) return false;
        if (start <= middle) {
            S next = op(acc, nodes_[p].value);
            if (!g(next)) { result = middle; return false; }
            acc = std::move(next);
        }
        return right_walk(nodes_[p].right, start, middle + 1, acc, g, result);
    }
    template<class G>
    bool left_walk(int p, int finish, int offset, S& acc, G& g, int& result) {
        if (p < 0 || finish <= offset) return true;
        if (offset + length(p) <= finish) {
            S next = op(nodes_[p].forward, acc);
            if (g(next)) { acc = std::move(next); return true; }
        }
        push(p);
        const int middle = offset + length(nodes_[p].left);
        if (!left_walk(nodes_[p].right, finish, middle + 1, acc, g, result)) return false;
        if (middle < finish) {
            S next = op(nodes_[p].value, acc);
            if (!g(next)) { result = middle + 1; return false; }
            acc = std::move(next);
        }
        return left_walk(nodes_[p].left, finish, offset, acc, g, result);
    }
    void flatten(int p, std::vector<S>& out) {
        if (p < 0) return;
        push(p);
        flatten(nodes_[p].left, out);
        out.push_back(nodes_[p].value);
        flatten(nodes_[p].right, out);
    }
    static bool equal(const S& a, const S& b) {
        if constexpr (requires { static_cast<bool>(a == b); }) return static_cast<bool>(a == b);
        else return !(a < b) && !(b < a);
    }
    int bound(S value, bool upper) {
        int p = root_, offset = 0, result = size();
        while (p >= 0) {
            push(p);
            const int k = offset + length(nodes_[p].left);
            const bool before = upper ? !(value < nodes_[p].value) : nodes_[p].value < value;
            if (before) { offset = k + 1; p = nodes_[p].right; }
            else { result = k; p = nodes_[p].left; }
        }
        return result;
    }
public:
    lazy_implicit_treap() = default;
    explicit lazy_implicit_treap(const std::vector<S>& values) {
        reserve(values.size());
        std::vector<int> stack;
        stack.reserve(values.size());
        for (const S& value : values) {
            int p = allocate(value), left = -1;
            while (!stack.empty() && nodes_[stack.back()].priority < nodes_[p].priority) {
                left = stack.back();
                stack.pop_back();
                pull(left);
            }
            nodes_[p].left = left;
            if (!stack.empty()) nodes_[stack.back()].right = p;
            stack.push_back(p);
        }
        if (!stack.empty()) root_ = stack.front();
        while (!stack.empty()) { pull(stack.back()); stack.pop_back(); }
    }
    lazy_implicit_treap(const lazy_implicit_treap&) = default;
    lazy_implicit_treap& operator=(const lazy_implicit_treap& other) {
        if (this == &other) return *this;
        if constexpr (std::is_nothrow_copy_constructible_v<Node> && std::is_nothrow_copy_assignable_v<Node>) {
            if (other.nodes_.size() <= nodes_.capacity()) {
                nodes_ = other.nodes_;
                root_ = other.root_;
                free_ = other.free_;
                random_ = other.random_;
                return *this;
            }
        }
        lazy_implicit_treap temporary(other);
        swap(temporary);
        return *this;
    }
    lazy_implicit_treap(lazy_implicit_treap&& other) noexcept
        : nodes_(std::move(other.nodes_)), root_(std::exchange(other.root_, -1)),
          free_(std::exchange(other.free_, -1)), random_(other.random_) { other.nodes_.clear(); }
    lazy_implicit_treap& operator=(lazy_implicit_treap&& other) noexcept {
        if (this != &other) { lazy_implicit_treap temporary(std::move(other)); swap(temporary); }
        return *this;
    }
    void swap(lazy_implicit_treap& other) noexcept {
        nodes_.swap(other.nodes_);
        std::swap(root_, other.root_);
        std::swap(free_, other.free_);
        std::swap(random_, other.random_);
    }
    friend void swap(lazy_implicit_treap& a, lazy_implicit_treap& b) noexcept { a.swap(b); }
    int size() const { return length(root_); }
    bool empty() const { return root_ < 0; }
    void clear() { nodes_.clear(); root_ = free_ = -1; }
    void reserve(std::size_t n) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("lazy_implicit_treap capacity");
        nodes_.reserve(n);
    }
    S get(int pos) {
        check_position(pos);
        int p = root_;
        for (;;) {
            push(p);
            int k = length(nodes_[p].left);
            if (pos == k) return nodes_[p].value;
            if (pos < k) p = nodes_[p].left;
            else { pos -= k + 1; p = nodes_[p].right; }
        }
    }
    void set(int pos, S value) {
        check_position(pos);
        set_value(root_, pos, std::move(value));
    }
private:
    void set_value(int p, int pos, S value) {
        push(p);
        int k = length(nodes_[p].left);
        if (pos == k) nodes_[p].value = std::move(value);
        else if (pos < k) set_value(nodes_[p].left, pos, std::move(value));
        else set_value(nodes_[p].right, pos - k - 1, std::move(value));
        pull(p);
    }
public:
    void insert_at(int pos, S value) {
        check_position(pos, true);
        int p = allocate(std::move(value));
        if (pos == size()) root_ = merge(root_, p);
        else if (pos == 0) root_ = merge(p, root_);
        else root_ = insert_node(root_, p, pos);
    }
    void push_back(S value) { insert_at(size(), std::move(value)); }
    void push_front(S value) { insert_at(0, std::move(value)); }
    void erase(int l, int r) {
#ifndef SKIP_RUNTIME_CHECKS
        if (l < 0 || l > r || r > size()) return;
#endif
        if (l == r) return;
        auto [a, bc] = split(root_, l);
        auto [b, c] = split(bc, r - l);
        root_ = merge(a, c);
        release(b);
    }
    void erase_at(int pos) {
        check_position(pos);
        int removed = -1;
        root_ = erase_node(root_, pos, removed);
        recycle(removed);
    }
    void pop_back() { if (!empty()) erase_at(size() - 1); }
    void pop_front() { if (!empty()) erase_at(0); }
    void apply(int l, int r, F f) {
        check_range(l, r);
        if (l != r) apply_range(root_, l, r, f);
    }
    S prod(int l, int r) { check_range(l, r); return l == r ? e() : query(root_, l, r); }
    S all_prod() const { return forward(root_); }
    void reverse(int l, int r) {
        check_range(l, r);
        if (r - l <= 1) return;
        auto [a, bc] = split(root_, l);
        auto [b, c] = split(bc, r - l);
        flip(b);
        root_ = merge(merge(a, b), c);
    }
    void rotate(int l, int m, int r) {
        check_range(l, r);
#ifndef SKIP_RUNTIME_CHECKS
        if (m < l || m > r) throw std::out_of_range("lazy_implicit_treap middle");
#endif
        if (l == m || m == r) return;
        auto [a, bcd] = split(root_, l);
        auto [b, cd] = split(bcd, m - l);
        auto [c, d] = split(cd, r - m);
        root_ = merge(merge(merge(a, c), b), d);
    }
    std::vector<S> to_vector() { std::vector<S> out; out.reserve(size()); flatten(root_, out); return out; }
    int lower_bound(S value) { return bound(std::move(value), false); }
    int upper_bound(S value) { return bound(std::move(value), true); }
    int get_rank(S value) { return lower_bound(std::move(value)); }
    int count(S value) { return upper_bound(value) - lower_bound(value); }
    bool contains(S value) {
        int p = lower_bound(value);
        return p < size() && equal(get(p), value);
    }
    void insert_sorted(S value) {
        int p = allocate(std::move(value));
        root_ = insert_sorted_node(root_, p);
    }
    void erase_val(S value) {
        int p = lower_bound(value);
        if (p < size() && equal(get(p), value)) erase_at(p);
    }
    void erase_max() { pop_back(); }
    template<class G> int max_right(int l, G g) {
        check_position(l, true);
        S acc = e();
#ifndef SKIP_RUNTIME_CHECKS
        if (!g(acc)) throw std::invalid_argument("lazy_implicit_treap predicate identity");
#endif
        int result = size();
        right_walk(root_, l, 0, acc, g, result);
        return result;
    }
    template<bool (*g)(S)> int max_right(int l) { return max_right(l, g); }
    template<class G> int min_left(int r, G g) {
        check_position(r, true);
        S acc = e();
#ifndef SKIP_RUNTIME_CHECKS
        if (!g(acc)) throw std::invalid_argument("lazy_implicit_treap predicate identity");
#endif
        int result = 0;
        left_walk(root_, r, 0, acc, g, result);
        return result;
    }
    template<bool (*g)(S)> int min_left(int r) { return min_left(r, g); }
};
#endif


// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

// 逆順集約を確認する非可換モノイド。作用は恒等写像。
struct VerifyStringMonoid {
    using S = std::string;
    using F = int;
    static S op(S a, S b) { return a + b; }
    static S e() { return {}; }
    static S mapping(F, S x) { return x; }
    static F composition(F, F) { return 0; }
    static F id() { return 0; }
};

int main() {
    auto require = [](bool b) { if (!b) throw std::runtime_error("dynamic/hash local verification failed"); };
    std::mt19937_64 random(20261019);
    // 1次元BIT: 検索の前提を満たす非負配列と、疎な64ビット領域。
    for (int n : {0, 1, 8, 64, 65, 129}) {
        DynamicFenwickTree<int, std::int64_t> bit(n);
        std::vector<std::int64_t> a(n);
        for (int step = 0; step < 500; ++step) {
            if (n) { int p = random() % n; a[p] = random() % 8; bit.set(p, a[p]); }
            int l = random() % (n + 1), r = random() % (n + 1);
            if (l > r) std::swap(l, r);
            require(bit.sum(l, r) == std::accumulate(a.begin() + l, a.begin() + r, std::int64_t{}));
            auto total = std::accumulate(a.begin(), a.end(), std::int64_t{});
            require(bit.sum_all() == total);
            std::int64_t target = static_cast<std::int64_t>(random() % (total + 3)) - 1;
            int lower = 0, upper = 0; std::int64_t sum = 0;
            if (target > 0) { while (lower < n && sum + a[lower] < target) sum += a[lower++]; }
            sum = 0;
            if (target >= 0) { while (upper < n && sum + a[upper] <= target) sum += a[upper++]; }
            require(bit.lower_bound(target) == lower && bit.upper_bound(target) == upper);
            const auto nodes = bit.stored_nodes();
            for (int p = 0; p < n; ++p) require(bit[p] == a[p]);
            require(bit.stored_nodes() == nodes);
        }
        auto copy = bit; bit.clear(); require(bit.sum_all() == 0);
        require(copy.sum_all() == std::accumulate(a.begin(), a.end(), std::int64_t{}));
        bit.reset(3); bit.add(2, 9); require(bit.get(2) == 9 && bit.size() == 3);
    }
    {
        constexpr std::uint64_t n = std::uint64_t{1} << 40;
        DynamicFenwickTree<std::uint64_t, std::int64_t> bit(n);
        bit.add(n - 1, 7); bit.add(123, 3);
        require(bit.sum(n) == 10 && bit.lower_bound(4) == n - 1 && bit.upper_bound(10) == n);
    }
    // 2次元BIT: 小規模の矩形照合と、直接の大きな疎座標。
    {
        DynamicFenwickTree2D<std::int64_t> bit(7, 9);
        std::int64_t a[7][9]{};
        for (int step = 0; step < 500; ++step) {
            int x = random() % 7, y = random() % 9;
            std::int64_t value = static_cast<std::int64_t>(random() % 41) - 20;
            if (step & 1) { a[x][y] += value; bit.add(x, y, value); }
            else { a[x][y] = value; bit.set(x, y, value); }
            int l = random() % 8, r = random() % 8, d = random() % 10, u = random() % 10;
            if (l > r) std::swap(l, r);
            if (d > u) std::swap(d, u);
            std::int64_t expected = 0;
            for (int i = l; i < r; ++i) for (int j = d; j < u; ++j) expected += a[i][j];
            const auto nodes = bit.stored_nodes();
            require(bit.sum(l, d, r, u) == expected && bit.get(x, y) == a[x][y]);
            require(bit.stored_nodes() == nodes);
        }
        auto copy = bit; bit.clear(); require(bit.sum_all() == 0);
        require(copy.get(1, 1) == a[1][1]);
        bit.reset(1'000'000'000, 1'000'000'000);
        bit.add(999'999'999, 900'000'000, 11); bit.add(0, 0, 3);
        require(bit.sum_all() == 14 && bit.sum(1, 1, 1'000'000'000, 1'000'000'000) == 11);
    }
    // 自動登録と併合コールバック。
    {
        DynamicUnionFind uf;
        require(uf.empty() && !uf.contains(-4));
        require(uf.add(-4) && !uf.add(-4));
        int callbacks = 0;
        uf.unite(-4, std::numeric_limits<int>::max(), [&](int root, int child) {
            require(root != child); ++callbacks;
        });
        require(uf.same(-4, std::numeric_limits<int>::max()) && uf.size(-4) == 2 && callbacks == 1);
        uf.unite(-4, -4, [&](int, int) { ++callbacks; }); require(callbacks == 1);
        uf.add(std::numeric_limits<int>::min()); require(uf.component_count() == 2 && uf.node_count() == 3);
        auto groups = uf.groups(); require(groups.size() == 2);
        auto copy = uf; uf.clear(); require(uf.empty() && copy.node_count() == 3);
    }
    // 公開HashMap: 整数操作をstd::mapと照合し、複合キーも確認。
    {
        HashMap<int, std::int64_t> table; std::map<int, std::int64_t> reference;
        for (int step = 0; step < 3000; ++step) {
            int k = static_cast<int>(random() % 129) - 64;
            if (step % 3 == 0) { table.erase(table.find(k), false); reference.erase(k); }
            else table[k] = reference[k] = static_cast<std::int64_t>(random());
            require(table.size() == reference.size());
            require(table.contains(k) == reference.contains(k));
            for (const auto& [key, value] : table) require(reference.at(key) == value);
        }
        auto copy = table; table.clear(); require(table.empty() && copy.size() == reference.size());
        auto moved = std::move(copy); require(copy.empty() && moved.size() == reference.size());
        HashMap<std::tuple<int, std::string>, int> tuples; tuples[{4, "x"}] = 8; require(tuples.at({4, "x"}) == 8);
        HashMap<std::vector<int>, int> vectors; vectors[{1, 2}] = 3; require(vectors.at({1, 2}) == 3);
        GenericHash hash(123);
        require(hash(0.0) == hash(-0.0));
        std::unordered_set<int> first{1, 2, 3}, second{3, 2, 1}; require(hash(first) == hash(second));
        reinsirk::detail::InternalHashMap<std::int64_t, int> internal;
        internal.set_default(7); internal.reserve(128); require(!internal.find(-1));
        require(internal[-1] == 7); internal[-1] += 2; require(*internal.find(-1) == 9);
        require(internal.enumerate().size() == 1 && internal.count(-1) == 1);
        auto internal_copy = internal; internal.clear(); require(internal.empty() && *internal_copy.find(-1) == 9);
    }
    // Li Chao: 区間ごとの直接評価。取得操作がノードを増やさないことも確認。
    {
        constexpr auto infinity = std::numeric_limits<std::int64_t>::max();
        DynamicLiChaoTree<std::int64_t, infinity> tree(-20, 20);
        struct Segment { std::int64_t a, b; int l, r; };
        std::vector<Segment> lines;
        for (int step = 0; step < 100; ++step) {
            std::int64_t a = static_cast<std::int64_t>(random() % 21) - 10, b = static_cast<std::int64_t>(random() % 51) - 25;
            int l = static_cast<int>(random() % 41) - 20, r = static_cast<int>(random() % 41) - 20;
            if (l > r) std::swap(l, r);
            tree.add_segment(a, b, l, r); lines.push_back({a, b, l, r});
            auto nodes = tree.stored_nodes();
            for (int x = -20; x <= 20; ++x) {
                auto expected = infinity;
                for (auto line : lines) if (line.l <= x && x <= line.r) expected = std::min(expected, line.a * x + line.b);
                require(tree.query(x) == expected);
                auto line = tree.query_line(x);
                require((!line && expected == infinity) || (line && line->get(x) == expected));
            }
            require(nodes == tree.stored_nodes());
        }
        tree.clear(); require(tree.empty() && tree.query(0) == infinity);
        tree.add_line(1'000'000'000, 2'000'000'000'000'000'000);
        require(tree.query(20) == 2'000'000'020'000'000'000);
    }
    // 1次元・2次元の包絡を全直線・全点の走査と照合。
    {
        MinLineContainer<std::int64_t> low; MaxLineContainer<std::int64_t> high;
        LineContainer2D points;
        std::vector<std::pair<std::int64_t, std::int64_t>> lines;
        for (int step = 0; step < 150; ++step) {
            std::int64_t a = static_cast<std::int64_t>(random() % 101) - 50, b = static_cast<std::int64_t>(random() % 101) - 50;
            low.add(a, b); high.add(a, b); points.add(a, b); lines.emplace_back(a, b);
            for (std::int64_t x = -20; x <= 20; ++x) {
                auto mn = std::numeric_limits<std::int64_t>::max(), mx = std::numeric_limits<std::int64_t>::min();
                for (auto [slope, intercept] : lines) { mn = std::min(mn, slope * x + intercept); mx = std::max(mx, slope * x + intercept); }
                require(low.query(x) == mn && high.query(x) == mx);
                auto [slope, intercept] = low.query_line(x); require(slope * x + intercept == mn);
                for (std::int64_t y : {-3, 0, 4}) {
                    std::int64_t lo = std::numeric_limits<std::int64_t>::max(), hi = std::numeric_limits<std::int64_t>::min();
                    for (auto [px, py] : lines) { lo = std::min(lo, x * px + y * py); hi = std::max(hi, x * px + y * py); }
                    require(points.min_rounded_ll(x, y) == lo && points.max_rounded_ll(x, y) == hi);
                    auto p = points.argmax(x, y); require(p.first * x + p.second * y == hi);
                }
            }
        }
        auto copy = points; points.clear(); require(points.empty() && !points.max_optional(1, 2) && !copy.empty());
        LineContainer2D fractional; fractional.add(0.25L, -1.5L); fractional.add(-2.75L, 3.125L);
        require(std::fabs(fractional.max_ld(0.125L, -2.5L) - 3.78125L) < 1e-15L);
        require(lc_div<std::int64_t>(-7, 3) == -3 && lc_div<std::int64_t>(7, -3) == -3);
    }
    // Heapの置換・同時挿入取り出し。
    {
        Heap heap; std::multiset<int> reference;
        for (int step = 0; step < 5000; ++step) {
            int x = static_cast<int>(random() % 101) - 50;
            unsigned type = random() % 4;
            if (reference.empty() || type == 0) { heap.push(x); reference.insert(x); }
            else if (type == 1) { require(heap.pop_value() == *reference.begin()); reference.erase(reference.begin()); }
            else if (type == 2) { require(heap.replace_top(x) == *reference.begin()); reference.erase(reference.begin()); reference.insert(x); }
            else { reference.insert(x); require(heap.push_pop(x) == *reference.begin()); reference.erase(reference.begin()); }
            require(heap.size() == reference.size() && heap.empty() == reference.empty());
            require(heap.top_or(999) == (reference.empty() ? 999 : *reference.begin()));
        }
        auto copy = heap; heap.clear(); require(heap.top() == -1 && copy.size() == reference.size());
    }
    // Treap: 値列をvectorで追跡し、区間作用・回転・照会を照合。
    {
        using A = RangeAddSum;
        lazy_implicit_treap<A::S, A::op, A::e, A::F, A::mapping, A::composition, A::id> tree;
        std::vector<std::int64_t> a;
        for (int step = 0; step < 1500; ++step) {
            int n = static_cast<int>(a.size()), l = random() % (n + 1), r = random() % (n + 1);
            if (l > r) std::swap(l, r);
            unsigned type = random() % 7;
            if (n == 0 || type == 0) { std::int64_t x = random() % 30; tree.insert_at(l, {x, 1}); a.insert(a.begin() + l, x); }
            else if (type == 1) { tree.erase(l, r); a.erase(a.begin() + l, a.begin() + r); }
            else if (type == 2) { tree.reverse(l, r); std::reverse(a.begin() + l, a.begin() + r); }
            else if (type == 3) { tree.apply(l, r, 3); for (int i = l; i < r; ++i) a[i] += 3; }
            else if (type == 4) { int m = l + random() % (r - l + 1); tree.rotate(l, m, r); std::rotate(a.begin() + l, a.begin() + m, a.begin() + r); }
            else if (type == 5) { int p = random() % n; tree.set(p, {7, 1}); a[p] = 7; }
            else require(tree.prod(l, r).sum == std::accumulate(a.begin() + l, a.begin() + r, std::int64_t{}));
            require(tree.size() == static_cast<int>(a.size()));
            require(tree.all_prod().sum == std::accumulate(a.begin(), a.end(), std::int64_t{}));
            auto values = tree.to_vector(); for (std::size_t i = 0; i < a.size(); ++i) require(values[i].sum == a[i]);
        }
        tree.clear(); tree.push_back({2, 1}); tree.push_back({3, 1}); tree.push_back({4, 1});
        require(tree.max_right(0, [](A::S x) { return x.sum <= 5; }) == 2);
        require(tree.min_left(3, [](A::S x) { return x.sum <= 7; }) == 1);
        auto copy = tree; auto moved = std::move(copy); require(copy.empty() && moved.all_prod().sum == 9);
        lazy_implicit_treap<> ordered;
        ordered.insert_sorted(3); ordered.insert_sorted(1); ordered.insert_sorted(3);
        require(ordered.lower_bound(3) == 1 && ordered.upper_bound(3) == 3 && ordered.count(3) == 2);
        ordered.erase_val(3); require(ordered.count(3) == 1); ordered.erase_max(); require(ordered.to_vector() == std::vector<std::int64_t>{1});
        using M = VerifyStringMonoid;
        lazy_implicit_treap<M::S, M::op, M::e, M::F, M::mapping, M::composition, M::id> strings(std::vector<std::string>{"a", "b", "c", "d"});
        strings.reverse(0, 4); require(strings.prod(1, 4) == "cba");
        strings.rotate(0, 2, 4); require(strings.all_prod() == "badc");
    }
    std::cout << "OK\n";
}
