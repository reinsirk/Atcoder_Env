// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RH_SOURCE
#define SIRKIT_RH_SOURCE "../../../String/RollingHash.cpp"
#endif
#include SIRKIT_RH_SOURCE
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using Big = boost::multiprecision::cpp_int;
constexpr std::uint64_t p = (std::uint64_t{1} << 61) - 1;
std::size_t checks = 0, collision_queries = 0;
void require(bool value) { ++checks; if (!value) std::abort(); }
std::uint64_t residue(Big value) {
    value %= p;
    if (value < 0) value += p;
    return value.convert_to<std::uint64_t>();
}
struct Reference {
    std::vector<std::uint64_t> hashes, powers;
    Reference(std::string_view text, std::uint64_t base)
        : hashes(text.size() + 1), powers(text.size() + 1, 1) {
        for (std::size_t i = 0; i < text.size(); ++i) {
            hashes[i + 1] = residue(Big(hashes[i]) * base +
                                   static_cast<unsigned char>(text[i]));
            powers[i + 1] = residue(Big(powers[i]) * base);
        }
    }
    std::uint64_t get(std::size_t l, std::size_t length) const {
        return residue(Big(hashes[l + length]) - Big(hashes[l]) * powers[length]);
    }
};
std::size_t byte_lcp(std::string_view a, std::string_view b, std::size_t limit) {
    const auto bound = std::min({a.size(), b.size(), limit});
    std::size_t i = 0;
    while (i < bound && a[i] == b[i]) ++i;
    return i;
}
int byte_compare(std::string_view a, std::string_view b) {
    const auto common = byte_lcp(a, b, std::numeric_limits<std::size_t>::max());
    if (common == std::min(a.size(), b.size()))
        return (a.size() > b.size()) - (a.size() < b.size());
    return (static_cast<unsigned char>(a[common]) > static_cast<unsigned char>(b[common])) -
           (static_cast<unsigned char>(a[common]) < static_cast<unsigned char>(b[common]));
}
template<class E, class F> void expect(F&& f) {
    bool caught = false;
    try { f(); } catch (const E&) { caught = true; }
    require(caught);
}

void query(const RollingHash& a, const Reference& ra, std::string_view sa,
           const RollingHash& b, const Reference& rb, std::string_view sb,
           std::size_t l, std::size_t ol, std::size_t requested) {
    const auto bound = std::min({sa.size() - l, sb.size() - ol, requested});
    const auto exact = byte_lcp(sa.substr(l), sb.substr(ol), bound);
    bool collision = false;
    for (std::size_t length = 0; length <= bound; ++length) {
        const auto ah = ra.get(l, length), bh = rb.get(ol, length);
        require(a.get(l, l + length) == ah);
        require(b.get(ol, ol + length) == bh);
        require(a.equal(l, l + length, b, ol, ol + length) == (ah == bh));
        if (length > exact && ah == bh) collision = true;
    }
    const auto got = a.lcp(l, b, ol, requested);
    require(exact <= got && got <= bound);
    require(ra.get(l, got) == rb.get(ol, got));
    if (!collision) {
        require(got == exact);
        require(a.compare(l, l + bound, b, ol, ol + bound) ==
                byte_compare(sa.substr(l, bound), sb.substr(ol, bound)));
    } else {
        ++collision_queries;
    }
}

void inspect(std::string left, std::string right, std::uint64_t raw_base) {
    const auto base = raw_base < 2 ? 10 : raw_base > p - 2 ? p - 10 : raw_base;
    const RollingHash a(left, raw_base), b(right, raw_base);
    const Reference ra(left, base), rb(right, base);
    const auto max = std::numeric_limits<std::size_t>::max();
    const std::array<std::size_t, 13> limits{0,1,2,3,4,7,8,60,61,62,127,128,max};
    const std::array<std::size_t, 6> offsets{0,1,2,3,7,9};
    for (auto l : offsets) if (l <= left.size()) {
        for (auto ol : offsets) if (ol <= right.size()) {
            for (auto limit : limits) query(a,ra,left,b,rb,right,l,ol,limit);
        }
    }
    // 同一位置・異なるオブジェクトの同値列・末尾の空区間を区別する。
    for (std::size_t l : {std::size_t{0}, left.size()/2, left.size()})
        for (auto limit : limits) {
            query(a,ra,left,a,ra,left,l,l,limit);
            require(a.lcp(l,a,l,limit) == std::min(left.size()-l,limit));
        }
    query(a,ra,left,b,rb,right,left.size(),right.size(),max);
    RollingHash moved(left,raw_base);
    RollingHash destination(std::move(moved));
    RollingHash cleared(right,raw_base);
    cleared.clear();
    const RollingHash empty("",raw_base);
    for (auto limit : limits) {
        require(moved.lcp(0,cleared,0,limit)==0);
        require(cleared.lcp(0,empty,0,limit)==0);
        require(a.lcp(left.size(),moved,0,limit)==0);
        require(moved.compare(0,0,cleared,0,0)==0);
    }
#ifndef SKIP_RUNTIME_CHECKS
    const RollingHash different("",base==2?3:2);
    expect<std::invalid_argument>([&]{ (void)moved.lcp(0,different,0,0); });
    expect<std::invalid_argument>([&]{ (void)moved.compare(0,0,different,0,0); });
    expect<std::out_of_range>([&]{ (void)a.lcp(left.size()+1,b,0,0); });
    expect<std::out_of_range>([&]{ (void)a.lcp(0,b,right.size()+1,0); });
    expect<std::out_of_range>([&]{ (void)moved.lcp(max,cleared,0,0); });
#endif
}
}

int main() {
    std::vector<std::string> texts{
        "", std::string(1,'\0'), std::string(1,char(255)),
        std::string(61,'\0'), std::string(61,char(1)),
        std::string(61,char(3)), std::string(61,char(255)),
        std::string{char(1),char(2)}, std::string{char(0),char(4)}};
    for (auto length : {std::size_t{60},std::size_t{62},std::size_t{127},std::size_t{129}}) {
        std::string text(length,'\0');
        for (std::size_t i=0;i<length;++i) text[i]=static_cast<char>(i%2?2:1);
        texts.push_back(text);
        text.back()^=char(255);
        texts.push_back(text);
    }
    // 等しい内容の前に異なる値を置き、両側の非0開始位置で引き算を通す。
    const std::string common(129,char(255));
    const std::string prefixed_left=std::string(7,char(127))+common;
    const std::string prefixed_right=std::string(9,char(254))+common;
    for (const auto base : {std::uint64_t{0},std::uint64_t{2},std::uint64_t{10},
                           std::uint64_t{256},std::uint64_t{1000000007},
                           p/2,p-2,p-1,p,std::numeric_limits<std::uint64_t>::max()}) {
        for (std::size_t i=0;i<texts.size();++i)
            inspect(texts[i],texts[(i+4)%texts.size()],base);
        inspect(prefixed_left,prefixed_right,base);
        inspect(std::string(61,char(1)),std::string(61,'\0'),base);
        inspect(std::string{char(1),char(2)},std::string{char(0),char(4)},base);
    }
    // 底2で2^61-1の倍数を作り、抽出ハッシュ0の剰余表現を狙う。
    const RollingHash zeros(std::string(61,'\0'),2);
    for (unsigned value : {1U,3U,255U}) {
        const RollingHash collision(std::string(61,static_cast<char>(value)),2);
        require(collision.whole()==0);
        require(collision.equal(0,61,zeros,0,61));
    }
    // 128bit経路の一致判定で積和がちょうどp²になる端点を作る。
    std::string suffix(59, '\0');
    suffix += std::string(2, char(1));
    const RollingHash tail(suffix, p - 2);
    const RollingHash prefixed(std::string(2, char(1)) + suffix, p - 2);
    require(tail.whole() == p - 1 && prefixed.whole() == 0);
    require(tail.lcp(0, prefixed, 2, 61) == 61);
    require(prefixed.lcp(2, tail, 0, 61) == 61);
    std::printf("PASS lcp edge checks=%zu collision_queries=%zu\n",checks,collision_queries);
}
