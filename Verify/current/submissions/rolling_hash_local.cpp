// Verify: ローカル独立検査

// Library: String/RollingHash.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/rolling_hash.md
//
// API一覧
// [RollingHash]
// RollingHash(s, base): 符号なしバイト列をO(|s|+1)時間・空間で構築。base<2は10、base>modulus-2はmodulus-10に補正。
// modulus: ハッシュの法2^61-1。ハッシュ値は常に[0,modulus)。
// get(l,r): 半開区間[l,r)のハッシュ、whole(): 全体のハッシュをO(1)で返す。空区間は0。
// size(), empty(), base(): 長さ・空判定・補正後の底をO(1)で返す。
// concat(left,right,right_length): 同じ底の正規化済みハッシュを連結してO(1)で返す。right_length<=size()。
// equal(l,r,other,ol,orr): 同じ底の2区間の長さとハッシュをO(1)で比較する。
// lcp(l,other,ol,limit): 同じ底の接尾辞の最長共通接頭辞長をlimit以下でO(log(min(残長,limit)+1))で返す。
// compare(l,r,other,ol,orr): 同じ底の2区間を符号なしバイト辞書順で比較し-1/0/1をO(log(短い方の長さ+1))で返す。
// 比較にはハッシュ衝突による誤判定の可能性がある。検査有効時は不正な範囲にout_of_range、底・ハッシュにinvalid_argument。
// assign(s): 現在の底で再構築し容量・利用可能な累乗表を再利用、O(旧長+|s|+1)上界、強い例外保証。clear(): 空へ戻して底・容量を保持、O(旧長+1)上界、noexcept。
// 元文字列は保持しない。コピーはO(n+1)で独立、代入は容量を再利用し強い例外保証。ムーブ元は空（自己ムーブ代入は保持）。swapはO(1)。

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#ifndef SIRKIT_ROLLING_HASH_DEFINED
#define SIRKIT_ROLLING_HASH_DEFINED

class RollingHash {
public:
    static constexpr std::uint64_t modulus = (std::uint64_t{1} << 61) - 1;

    RollingHash(std::string_view s, std::uint64_t base)
        : base_(normalize_base(base)) {
        if (s.size() >= data_.max_size() / 2) {
            throw std::length_error("RollingHash: capacity overflow");
        }
        data_.resize(s.size() + 1);
        power_data_.resize(s.size() + 1);
        const std::size_t offset = s.size() + 1;
        offset_ = offset;
        Entry* const prefixes = data_.data();
        Entry* const powers = power_data_.data();
        powers_ = powers;
        const std::uint64_t factor = base_;
        std::uint64_t hash = 0;
        std::uint64_t power = 1;
        prefixes[0].value = hash;
        powers[0].value = power;
#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__) && \
    (!defined(__SIZEOF_INT128__) || defined(SIRKIT_FORCE_STANDARD_ARITHMETIC))
        std::size_t i = 0;
        for (; s.size() - i >= 4; i += 4) {
            const auto byte0 = static_cast<unsigned char>(s[i + 0]);
            hash = multiply_add_byte(hash, factor, byte0);
            power = multiply(power, factor);
            prefixes[i + 1].value = hash;
            powers[i + 1].value = power;

            const auto byte1 = static_cast<unsigned char>(s[i + 1]);
            hash = multiply_add_byte(hash, factor, byte1);
            power = multiply(power, factor);
            prefixes[i + 2].value = hash;
            powers[i + 2].value = power;

            const auto byte2 = static_cast<unsigned char>(s[i + 2]);
            hash = multiply_add_byte(hash, factor, byte2);
            power = multiply(power, factor);
            prefixes[i + 3].value = hash;
            powers[i + 3].value = power;

            const auto byte3 = static_cast<unsigned char>(s[i + 3]);
            hash = multiply_add_byte(hash, factor, byte3);
            power = multiply(power, factor);
            prefixes[i + 4].value = hash;
            powers[i + 4].value = power;
        }
        for (; i < s.size(); ++i) {
            const auto byte = static_cast<unsigned char>(s[i]);
            hash = multiply_add_byte(hash, factor, byte);
            power = multiply(power, factor);
            prefixes[i + 1].value = hash;
            powers[i + 1].value = power;
        }
#else
        for (std::size_t i = 0; i < s.size(); ++i) {
            const auto byte = static_cast<unsigned char>(s[i]);
            hash = multiply_add_byte(hash, factor, byte);
            power = multiply(power, factor);
            prefixes[i + 1].value = hash;
            powers[i + 1].value = power;
        }
#endif
    }

    void assign(std::string_view s) {
        if (s.size() >= data_.max_size() / 2) {
            throw std::length_error("RollingHash: capacity overflow");
        }
        const std::size_t count = s.size() + 1;
        const bool reuse_powers = power_data_.size() >= count;
        // 両方の確保を先に終え、失敗時も内容と非所有ポインタを保つ。
        data_.reserve(count);
        power_data_.reserve(count);
        data_.resize(count);
        power_data_.resize(count);
        offset_ = count;
        powers_ = power_data_.data();
        Entry* const prefixes = data_.data();
        Entry* const powers = power_data_.data();
        const std::uint64_t factor = base_;
        std::uint64_t hash = 0;
        prefixes[0].value = hash;
        powers[0].value = 1;
        if (reuse_powers) {
#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__) && \
    (!defined(__SIZEOF_INT128__) || defined(SIRKIT_FORCE_STANDARD_ARITHMETIC))
            std::size_t i = 0;
            for (; s.size() - i >= 4; i += 4) {
                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 0]));
                prefixes[i + 1].value = hash;

                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 1]));
                prefixes[i + 2].value = hash;

                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 2]));
                prefixes[i + 3].value = hash;

                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 3]));
                prefixes[i + 4].value = hash;
            }
            for (; i < s.size(); ++i) {
                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i]));
                prefixes[i + 1].value = hash;
            }
#else
            for (std::size_t i = 0; i < s.size(); ++i) {
                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i]));
                prefixes[i + 1].value = hash;
            }
#endif
        } else {
            std::uint64_t power = 1;
#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__) && \
    (!defined(__SIZEOF_INT128__) || defined(SIRKIT_FORCE_STANDARD_ARITHMETIC))
            std::size_t i = 0;
            for (; s.size() - i >= 4; i += 4) {
                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 0]));
                power = multiply(power, factor);
                prefixes[i + 1].value = hash;
                powers[i + 1].value = power;

                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 1]));
                power = multiply(power, factor);
                prefixes[i + 2].value = hash;
                powers[i + 2].value = power;

                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 2]));
                power = multiply(power, factor);
                prefixes[i + 3].value = hash;
                powers[i + 3].value = power;

                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i + 3]));
                power = multiply(power, factor);
                prefixes[i + 4].value = hash;
                powers[i + 4].value = power;
            }
            for (; i < s.size(); ++i) {
                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i]));
                power = multiply(power, factor);
                prefixes[i + 1].value = hash;
                powers[i + 1].value = power;
            }
#else
            for (std::size_t i = 0; i < s.size(); ++i) {
                hash = multiply_add_byte(hash, factor, static_cast<unsigned char>(s[i]));
                power = multiply(power, factor);
                prefixes[i + 1].value = hash;
                powers[i + 1].value = power;
            }
#endif
        }
    }

    void clear() noexcept {
        data_.clear(); power_data_.clear();
        offset_ = 0; powers_ = nullptr;
    }

    RollingHash(const RollingHash& other)
        : data_(other.data_), power_data_(other.power_data_),
          base_(other.base_), offset_(other.offset_),
          powers_(data_.empty() ? nullptr : power_data_.data()) {}

    RollingHash& operator=(const RollingHash& other) {
        if (this != &other) {
            data_.reserve(other.data_.size());
            power_data_.reserve(other.power_data_.size());
            data_.resize(other.data_.size());
            power_data_.resize(other.power_data_.size());
            // Entryはtrivially copyable。独立した配列間で、構築済み要素の表現を写す。
            if (!other.data_.empty()) {
                std::memcpy(data_.data(), other.data_.data(), other.data_.size() * sizeof(Entry));
                std::memcpy(power_data_.data(), other.power_data_.data(), other.power_data_.size() * sizeof(Entry));
            }
            base_ = other.base_;
            offset_ = other.offset_;
            powers_ = data_.empty() ? nullptr : power_data_.data();
        }
        return *this;
    }

    RollingHash(RollingHash&& other) noexcept
        : data_(std::move(other.data_)), power_data_(std::move(other.power_data_)),
          base_(other.base_), offset_(other.offset_),
          powers_(other.powers_) {
        other.data_.clear();
        other.power_data_.clear();
        other.offset_ = 0;
        other.powers_ = nullptr;
    }

    RollingHash& operator=(RollingHash&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            power_data_ = std::move(other.power_data_);
            base_ = other.base_;
            offset_ = other.offset_;
            powers_ = other.powers_;
            other.data_.clear();
            other.power_data_.clear();
            other.offset_ = 0;
            other.powers_ = nullptr;
        }
        return *this;
    }

    void swap(RollingHash& other) noexcept {
        data_.swap(other.data_);
        power_data_.swap(other.power_data_);
        std::swap(base_, other.base_);
        std::swap(offset_, other.offset_);
        std::swap(powers_, other.powers_);
    }

    friend void swap(RollingHash& left, RollingHash& right) noexcept {
        left.swap(right);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return offset_ == 0 ? 0 : offset_ - 1;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] std::uint64_t base() const noexcept {
        return base_;
    }

    [[nodiscard]] std::uint64_t whole() const noexcept {
        return offset_ == 0 ? 0 : data_[offset_ - 1].value;
    }

    [[nodiscard]] std::uint64_t get(std::size_t l, std::size_t r) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, r);
#endif
        return get_unchecked(l, r);
    }

    [[nodiscard]] std::uint64_t concat(std::uint64_t left,
                                     std::uint64_t right,
                                     std::size_t right_length) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (right_length > size()) {
            throw std::out_of_range("RollingHash: concatenation length");
        }
        if (left >= modulus || right >= modulus) {
            throw std::invalid_argument("RollingHash: noncanonical hash");
        }
#endif
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        if (right_length == 0) return add(left, right);
        const __uint128_t product =
            static_cast<__uint128_t>(left) * powers_[right_length].value + right;
        const std::uint64_t folded =
            (static_cast<std::uint64_t>(product) & modulus) +
            static_cast<std::uint64_t>(product >> 61);
        return folded >= modulus ? folded - modulus : folded;
#else
        return add(right_length == 0 ? left
                                    : multiply(left, powers_[right_length].value),
                   right);
#endif
    }

    [[nodiscard]] bool equal(std::size_t l, std::size_t r,
                             const RollingHash& other,
                             std::size_t ol, std::size_t orr) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, r);
        other.check_range(ol, orr);
        check_base(other);
#endif
        return r - l == orr - ol &&
               get_unchecked(l, r) == other.get_unchecked(ol, orr);
    }

    [[nodiscard]] std::size_t lcp(
        std::size_t l, const RollingHash& other, std::size_t ol,
        std::size_t limit = std::numeric_limits<std::size_t>::max()) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, l);
        other.check_range(ol, ol);
        check_base(other);
#endif
        return lcp_unchecked(l, other, ol,
                             std::min({size() - l, other.size() - ol, limit}));
    }

    [[nodiscard]] int compare(std::size_t l, std::size_t r,
                              const RollingHash& other,
                              std::size_t ol, std::size_t orr) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, r);
        other.check_range(ol, orr);
        check_base(other);
#endif
        const std::size_t length = r - l;
        const std::size_t other_length = orr - ol;
        const std::size_t common = lcp_unchecked(
            l, other, ol, std::min(length, other_length));
        if (common == std::min(length, other_length)) {
            return (length > other_length) - (length < other_length);
        }
        const std::uint64_t byte = get_unchecked(l + common, l + common + 1);
        const std::uint64_t other_byte =
            other.get_unchecked(ol + common, ol + common + 1);
        return (byte > other_byte) - (byte < other_byte);
    }

private:
    struct Entry {
        std::uint64_t value;
        // 構築ループで全要素を設定し、不要なゼロ初期化を避ける。
        Entry() noexcept {}
    };

    std::vector<Entry> data_, power_data_;
    std::uint64_t base_;
    std::size_t offset_ = 0;
    const Entry* powers_ = nullptr; // power_data_の累乗領域への非所有ポインタ。

    static constexpr std::uint64_t normalize_base(std::uint64_t base) noexcept {
        return base < 2 ? 10 : (base > modulus - 2 ? modulus - 10 : base);
    }

    static constexpr std::uint64_t add(std::uint64_t a,
                                       std::uint64_t b) noexcept {
        const std::uint64_t sum = a + b;
        return sum >= modulus ? sum - modulus : sum;
    }

    static constexpr std::uint64_t multiply_raw(std::uint64_t a,
                                            std::uint64_t b) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && \
    !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        const __uint128_t product = static_cast<__uint128_t>(a) * b;
        const std::uint64_t folded =
            (static_cast<std::uint64_t>(product) & modulus) +
            static_cast<std::uint64_t>(product >> 61);
#else
        // 32bitの4積を2^61=1、2^64=8として直接折り返す。
        const std::uint64_t a0 = static_cast<std::uint32_t>(a);
        const std::uint64_t b0 = static_cast<std::uint32_t>(b);
        const std::uint64_t a1 = a >> 32;
        const std::uint64_t b1 = b >> 32;
        const std::uint64_t low = a0 * b0;
        const std::uint64_t cross = a0 * b1 + a1 * b0;
        constexpr std::uint64_t mask29 = (std::uint64_t{1} << 29) - 1;
        const std::uint64_t sum = (low & modulus) + (low >> 61) +
                                  ((cross & mask29) << 32) + (cross >> 29) +
                                  ((a1 * b1) << 3);
        const std::uint64_t folded = sum;
#endif
        return folded;
    }

    static constexpr std::uint64_t multiply(std::uint64_t a, std::uint64_t b) noexcept {
        std::uint64_t folded = multiply_raw(a, b);
#if !((defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)) || defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        folded = (folded & modulus) + (folded >> 61);
#endif
        return folded >= modulus ? folded - modulus : folded;
    }

    // byteは0..255。乗算結果への加算を正規化前にまとめる。
    static constexpr std::uint64_t multiply_add_byte(
        std::uint64_t a, std::uint64_t b, unsigned char byte) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        const __uint128_t product = static_cast<__uint128_t>(a) * b + byte;
        const std::uint64_t folded =
            (static_cast<std::uint64_t>(product) & modulus) +
            static_cast<std::uint64_t>(product >> 61);
#else
        const std::uint64_t sum = multiply_raw(a, b) + byte;
        const std::uint64_t folded = (sum & modulus) + (sum >> 61);
#endif
        return folded >= modulus ? folded - modulus : folded;
    }

    [[nodiscard]] std::uint64_t get_unchecked(std::size_t l,
                                             std::size_t r) const noexcept {
        if (l == r) {
            return 0;
        }
        const std::uint64_t right = data_[r].value;
        const std::uint64_t left = multiply_raw(data_[l].value, powers_[r - l].value);
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        const std::uint64_t difference = right + 2 * modulus - left;
#else
        // raw積は4p未満、右辺全体は5p未満で64bitに収まる。
        const std::uint64_t difference = right + 4 * modulus - left;
#endif
        const std::uint64_t folded = (difference & modulus) + (difference >> 61);
        return folded >= modulus ? folded - modulus : folded;
    }

    [[nodiscard]] std::size_t lcp_unchecked(std::size_t l,
                                           const RollingHash& other,
                                           std::size_t ol,
                                           std::size_t limit) const noexcept {
        if (limit == 0) return 0;
        const Entry* const left = data_.data() + l;
        const Entry* const right = other.data_.data() + ol;
        const Entry* const powers = powers_;
        const std::uint64_t start = left[0].value + modulus - right[0].value;
        const std::uint64_t delta = start >= modulus ? start - modulus : start;
        // 開始接頭辞が同じなら、端点の比較だけで区間ハッシュを比較できる。
        if (delta == 0) {
            if (left[limit].value == right[limit].value) return limit;
            std::size_t low = 0, high = limit - 1;
            while (low < high) {
                const std::size_t middle = high - (high - low) / 2;
                if (left[middle].value == right[middle].value) low = middle;
                else high = middle - 1;
            }
            return low;
        }
        const auto equal_prefix = [&](std::size_t length) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
            // 0<product<=p²。折り返し結果がpなら、区間ハッシュの差は0。
            const __uint128_t product =
                static_cast<__uint128_t>(delta) * powers[length].value +
                (right[length].value + modulus - left[length].value);
            return ((static_cast<std::uint64_t>(product) & modulus) +
                    static_cast<std::uint64_t>(product >> 61)) == modulus;
#else
            // raw積は4p未満。0<difference<6pで、全中間値は64bitに収まる。
            const std::uint64_t difference = left[length].value + 5 * modulus -
                multiply_raw(delta, powers[length].value) - right[length].value;
            return ((difference & modulus) + (difference >> 61)) == modulus;
#endif
        };
        if (!equal_prefix(1)) return 0;
        if (equal_prefix(limit)) return limit;
        std::size_t low = 1, high = limit - 1;
        while (low < high) {
            const std::size_t middle = high - (high - low) / 2;
            if (equal_prefix(middle)) low = middle;
            else high = middle - 1;
        }
        return low;
    }

#ifndef SKIP_RUNTIME_CHECKS
    void check_range(std::size_t l, std::size_t r) const {
        if (l > r || r > size()) {
            throw std::out_of_range("RollingHash: substring range");
        }
    }

    void check_base(const RollingHash& other) const {
        if (base_ != other.base_) {
            throw std::invalid_argument("RollingHash: different bases");
        }
    }
#endif
};

#endif


// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("rolling hash reference mismatch");};std::mt19937_64 rng(198273);
    constexpr std::uint64_t mod=(std::uint64_t{1}<<61)-1;
    auto multiply=[](std::uint64_t a,std::uint64_t b){std::uint64_t result=0;while(b){if(b&1)result=(result+a)%mod;a=(a+a)%mod;b>>=1;}return result;};
    for(int test=0;test<160;++test){std::string s,t;for(unsigned i=0,n=rng()%45;i<n;++i)s+=static_cast<char>(rng()%256);for(unsigned i=0,n=rng()%45;i<n;++i)t+=static_cast<char>(rng()%256);
        std::uint64_t base=test%4==0?0:test%4==1?mod-1:test%4==2?1000000007:mod-2;RollingHash a(s,base),b(t,base);RollingHash compare_s(s,1000000007),compare_t(t,1000000007);
        check(a.size()==s.size());check(a.empty()==s.empty());std::uint64_t normalized=base<2?10:base>mod-2?mod-10:base;check(a.base()==normalized);
        for(std::size_t l=0;l<=s.size();++l){std::uint64_t hash=0;for(std::size_t r=l;r<=s.size();++r){check(a.get(l,r)==hash);std::size_t mid=l+(r-l)/2;check(a.concat(a.get(l,mid),a.get(mid,r),r-mid)==hash);
                if(r<s.size())hash=(multiply(hash,normalized)+static_cast<unsigned char>(s[r]))%mod;}}
        check(a.whole()==a.get(0,s.size()));
        for(int trial=0;trial<60;++trial){std::size_t l=rng()%(s.size()+1),r=l+rng()%(s.size()-l+1),ol=rng()%(t.size()+1),orr=ol+rng()%(t.size()-ol+1),limit=rng()%50;
            std::size_t common=0;while(l+common<s.size()&&ol+common<t.size()&&s[l+common]==t[ol+common]&&common<limit)++common;check(compare_s.lcp(l,compare_t,ol,limit)==common);
            check(compare_s.equal(l,r,compare_t,ol,orr)==(s.substr(l,r-l)==t.substr(ol,orr-ol)));
            auto less=[](char x,char y){return static_cast<unsigned char>(x)<static_cast<unsigned char>(y);};
            int compare=std::lexicographical_compare(s.begin()+l,s.begin()+r,t.begin()+ol,t.begin()+orr,less)?-1:std::lexicographical_compare(t.begin()+ol,t.begin()+orr,s.begin()+l,s.begin()+r,less)?1:0;
            check(compare_s.compare(l,r,compare_t,ol,orr)==compare);
        }
        auto copy=a;check(copy.whole()==a.whole());auto moved=std::move(copy);check(copy.empty());check(moved.whole()==a.whole());swap(a,b);check(a.size()==t.size());
    }std::cout<<"OK\n";
}
