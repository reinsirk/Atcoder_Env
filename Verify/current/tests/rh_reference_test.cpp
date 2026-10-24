// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RH_REQUIRE_EXTRA
#define SIRKIT_RH_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_RH_SOURCE
#define SIRKIT_RH_SOURCE "../../../String/RollingHash.cpp"
#endif
#include SIRKIT_RH_SOURCE
#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

namespace {
std::uint64_t checks = 0;
constexpr std::uint64_t p = (std::uint64_t{1} << 61) - 1;
void require(bool ok) { ++checks; if (!ok) throw std::runtime_error("rolling hash reference mismatch"); }
std::uint64_t mul(std::uint64_t a, std::uint64_t b) {
    std::uint64_t value = 0;
    while (b) { if (b & 1) value = (value + a) % p; a = (a + a) % p; b >>= 1; }
    return value;
}
std::uint64_t norm(std::uint64_t b) { return b < 2 ? 10 : b > p - 2 ? p - 10 : b; }
std::uint64_t hash(std::string_view s, std::uint64_t b) {
    std::uint64_t value = 0;
    for (unsigned char c : s) value = (mul(value, b) + c) % p;
    return value;
}
std::size_t direct_lcp(std::string_view a, std::string_view b, std::size_t limit) {
    const auto bound = std::min({a.size(), b.size(), limit}); std::size_t i = 0;
    while (i < bound && a[i] == b[i]) ++i;
    return i;
}
int direct_compare(std::string_view a, std::string_view b) {
    const auto common = direct_lcp(a, b, std::numeric_limits<std::size_t>::max());
    if (common == std::min(a.size(), b.size())) return (a.size() > b.size()) - (a.size() < b.size());
    const auto x = static_cast<unsigned char>(a[common]), y = static_cast<unsigned char>(b[common]);
    return (x > y) - (x < y);
}
template<class E, class F> void expect(F&& f) {
    bool caught = false; try { f(); } catch (const E&) { caught = true; } require(caught);
}
void check_one(const std::string& s, std::uint64_t base, std::mt19937_64& rng) {
    const auto b = norm(base); RollingHash a(s, base);
    require(a.size() == s.size()); require(a.empty() == s.empty()); require(a.base() == b);
    require(a.whole() == hash(s, b)); require(a.get(0, s.size()) == a.whole());
    for (std::size_t l = 0; l <= s.size(); ++l) {
        std::uint64_t h = 0;
        for (std::size_t r = l; r <= s.size(); ++r) {
            require(a.get(l, r) == h); require(a.get(l, r) < p);
            const auto mid = l + (r - l) / 2;
            require(a.concat(a.get(l, mid), a.get(mid, r), r - mid) == h);
            if (r != s.size()) h = (mul(h, b) + static_cast<unsigned char>(s[r])) % p;
        }
    }
    std::vector<std::uint64_t> powers(s.size() + 1, 1);
    for (std::size_t i = 1; i <= s.size(); ++i) powers[i] = mul(powers[i - 1], b);
    const std::array<std::uint64_t, 8> values{0, 1, 255, p/2, p-3, p-2, p-1, rng()%p};
    for (auto left : values) for (auto right : values) {
        for (auto len : {std::size_t{0}, s.size()/2, s.size()}) {
            require(a.concat(left, right, len) == (mul(left, powers[len]) + right) % p);
        }
    }
    // 衝突しやすい底でもequalは多項式ハッシュの一致として照合する。
    std::string other = s; if (!other.empty()) other[rng()%other.size()] ^= char(0x55);
    RollingHash other_hash(other, base);
    for (int t = 0; t < 80; ++t) {
        const auto l = rng()%(s.size()+1), r = l + rng()%(s.size()-l+1);
        const auto ol = rng()%(other.size()+1), orr = ol + rng()%(other.size()-ol+1);
        require(a.equal(l,r,other_hash,ol,orr) == (r-l==orr-ol && hash(std::string_view(s).substr(l,r-l),b)==hash(std::string_view(other).substr(ol,orr-ol),b)));
        const auto bound = std::min(s.size()-l, other.size()-ol);
        const auto got = a.lcp(l,other_hash,ol,bound);
        require(got <= bound); require(got >= direct_lcp(std::string_view(s).substr(l),std::string_view(other).substr(ol),bound));
    }
    auto copy = a; require(copy.whole() == a.whole()); require(copy.base() == b);
    auto moved = std::move(copy); require(copy.empty()); require(copy.whole() == 0); require(copy.get(0,0) == 0);
    require(copy.concat(0,0,0) == 0); require(copy.lcp(0,copy,0) == 0); require(copy.compare(0,0,copy,0,0)==0);
    require(moved.whole() == a.whole());
    auto* alias = &moved; moved = *alias; moved = std::move(*alias); require(moved.whole() == a.whole());
    RollingHash target("different", 123456789); target = a; require(target.whole() == a.whole());
    RollingHash destination("old destination", 123); destination = std::move(target);
    require(target.empty()); require(target.get(0,0) == 0); require(destination.whole() == a.whole());
    swap(destination, other_hash); require(destination.whole() == hash(other,b)); require(other_hash.whole()==a.whole());
    destination.swap(other_hash); require(destination.whole()==a.whole());
    for (std::size_t i = 0; i <= s.size(); ++i) {
        require(destination.get(i,s.size()) == a.get(i,s.size()));
        require(a.lcp(i,a,i)==s.size()-i); require(a.lcp(i,a,i,0)==0);
    }
#ifdef SIRKIT_RH_REQUIRE_EXTRA
    RollingHash fresh("old text",base); fresh.assign(s);
    require(fresh.base()==b && fresh.whole()==a.whole());
    for(std::size_t i=0;i<=s.size();++i)require(fresh.get(i,s.size())==a.get(i,s.size()));
    fresh.clear(); require(fresh.empty()&&fresh.whole()==0&&fresh.base()==b);
    require(fresh.get(0,0)==0&&fresh.concat(0,0,0)==0&&fresh.lcp(0,fresh,0)==0&&fresh.compare(0,0,fresh,0,0)==0);
    require(fresh.equal(0,0,fresh,0,0)); fresh.clear();
    auto cleared_copy=fresh; require(cleared_copy.empty()&&cleared_copy.base()==b&&cleared_copy.get(0,0)==0);
    auto cleared_move=std::move(fresh); require(cleared_move.empty()&&fresh.empty());
    fresh.assign(s); require(fresh.whole()==a.whole()&&fresh.base()==b);
    cleared_move.assign(std::string_view{}); require(cleared_move.empty()&&cleared_move.whole()==0&&cleared_move.base()==b);
    fresh.swap(cleared_move); require(fresh.empty()&&cleared_move.whole()==a.whole());
    moved.assign(s); require(moved.whole()==a.whole()&&moved.base()==b);
    static_assert(noexcept(std::declval<RollingHash&>().clear()));
#endif
#ifndef SKIP_RUNTIME_CHECKS
    const auto huge = std::numeric_limits<std::size_t>::max();
    expect<std::out_of_range>([&]{ (void)a.get(1,0); });
    expect<std::out_of_range>([&]{ (void)a.get(0,s.size()+1); });
    expect<std::out_of_range>([&]{ (void)a.get(huge,huge); });
    expect<std::out_of_range>([&]{ (void)a.concat(0,0,s.size()+1); });
    expect<std::invalid_argument>([&]{ (void)a.concat(p,0,0); });
    expect<std::invalid_argument>([&]{ (void)a.concat(0,p,0); });
    expect<std::out_of_range>([&]{ (void)a.lcp(s.size()+1,a,0); });
    expect<std::out_of_range>([&]{ (void)a.compare(0,s.size()+1,a,0,0); });
    expect<std::out_of_range>([&]{ (void)a.equal(0,0,a,0,s.size()+1); });
    expect<std::out_of_range>([&]{ (void)copy.get(0,1); });
    const RollingHash different_base(s,b==2?3:2);
    expect<std::invalid_argument>([&]{ (void)a.equal(0,0,different_base,0,0); });
    expect<std::invalid_argument>([&]{ (void)a.lcp(0,different_base,0,0); });
    expect<std::invalid_argument>([&]{ (void)a.compare(0,0,different_base,0,0); });
#endif
}
void check_comparisons(const std::string& s, const std::string& t, std::mt19937_64& rng) {
    for (const auto base : {std::uint64_t{911382323}, std::uint64_t{1000000007}}) {
        const RollingHash a(s,base), b(t,base);
        for (int trial=0;trial<120;++trial) {
            const auto l=rng()%(s.size()+1), r=l+rng()%(s.size()-l+1);
            const auto ol=rng()%(t.size()+1), orr=ol+rng()%(t.size()-ol+1);
            const auto limit=trial%3==0?std::numeric_limits<std::size_t>::max():rng()%512;
            const auto sv=std::string_view(s).substr(l), tv=std::string_view(t).substr(ol);
            require(a.lcp(l,b,ol,limit)==direct_lcp(sv,tv,limit));
            require(a.compare(l,r,b,ol,orr)==direct_compare(sv.substr(0,r-l),tv.substr(0,orr-ol)));
            require(a.equal(l,r,b,ol,orr)==(sv.substr(0,r-l)==tv.substr(0,orr-ol)));
        }
    }
}
void check_lcp_boundaries() {
    constexpr std::array<std::size_t,12> limits{0,1,2,62,63,64,65,66,127,128,129,std::numeric_limits<std::size_t>::max()};
    for (const auto n:{std::size_t{0},std::size_t{1},std::size_t{63},std::size_t{64},std::size_t{65},std::size_t{127},std::size_t{128},std::size_t{129},std::size_t{1024}}) {
        for (const auto change:{std::size_t{0},std::size_t{1},std::size_t{62},std::size_t{63},std::size_t{64},std::size_t{65},n?n-1:0,n}) {
            for (const auto l:{std::size_t{0},std::size_t{1},std::size_t{2}}) for (const auto ol:{std::size_t{0},std::size_t{1},std::size_t{3}}) {
                const std::string s=std::string(l,'x')+std::string(n,'a');std::string t=std::string(ol,'y')+std::string(n,'a');
                if(change<n)t[ol+change]='b';
                const RollingHash a(s,1000000007),b(t,1000000007);
                for(const auto limit:limits){require(a.lcp(l,b,ol,limit)==std::min({n,change,limit}));require(a.lcp(l,a,l,limit)==std::min(n,limit));require(b.lcp(ol,a,l,limit)==std::min({n,change,limit}));}
                require(a.compare(l,s.size(),b,ol,t.size())==(change<n?-1:0));
            }
        }
    }
}
}
int main() {
    std::mt19937_64 rng(61022); std::vector<std::string> cases{"", "a", "aa", "ab", "abracadabra", std::string(64,'\0'), std::string(79,'x')};
    std::string bytes; for(int c=0;c<256;++c) bytes.push_back(static_cast<char>(c)); cases.push_back(bytes);
    std::string reverse=bytes; std::reverse(reverse.begin(),reverse.end()); cases.push_back(reverse);
    for(int t=0;t<80;++t){std::string s(rng()%81,'\0');for(char& c:s)c=static_cast<char>(rng());cases.push_back(std::move(s));}
    constexpr std::array<std::uint64_t,10> bases{0,1,2,10,911382323,1000000007,p-2,p-1,p,std::numeric_limits<std::uint64_t>::max()};
    for(const auto& s:cases)for(auto base:bases)check_one(s,base,rng);
    for(std::size_t i=0;i<cases.size();++i)check_comparisons(cases[i],cases[(i+7)%cases.size()],rng);
    for(auto n:{std::size_t{0},std::size_t{1},std::size_t{2},std::size_t{31},std::size_t{32},std::size_t{63},std::size_t{64},std::size_t{65},std::size_t{255},std::size_t{256},std::size_t{257},std::size_t{1024},std::size_t{4096}}){
        std::string s(n,'a'), t=s; if(!t.empty())t.back()='b'; check_comparisons(s,t,rng);
        RollingHash a(s,1000000007), b(t,1000000007); require(a.lcp(0,a,0)==n);require(a.lcp(n,b,n)==0);
        require(a.lcp(0,b,0)==(n?n-1:0));require(a.compare(0,n,b,0,n)==(n?-1:0));
    }
    // 底2では異なる2バイト列が同じハッシュになる。equalの誤陽性は仕様上あり得る。
    const std::string left{char(1),char(2)}, right{char(0),char(4)};
    const RollingHash a(left,2), b(right,2);require(left!=right);require(a.whole()==4);require(a.equal(0,2,b,0,2));
    check_lcp_boundaries();
    std::cout << "PASS checks=" << checks << " strings=" << cases.size() << "\n";
}
