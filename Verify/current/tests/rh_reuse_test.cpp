// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RH_SOURCE
#define SIRKIT_RH_SOURCE "../../../String/RollingHash.cpp"
#endif
#include SIRKIT_RH_SOURCE
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>

namespace {
std::atomic<std::ptrdiff_t> budget{-1}, live{0};
std::atomic<std::size_t> allocations{0};
std::size_t checks = 0, failures = 0;
void require(bool ok) {
    ++checks;
    if (!ok) std::abort();
}
std::uint64_t multiply_reference(std::uint64_t a, std::uint64_t b) {
    std::uint64_t result = 0;
    while (b != 0) {
        if (b & 1) result = (result + a) % RollingHash::modulus;
        a = (a + a) % RollingHash::modulus;
        b >>= 1;
    }
    return result;
}
std::uint64_t hash_reference(std::string_view s, std::uint64_t base) {
    std::uint64_t h = 0;
    for (unsigned char byte : s)
        h = (multiply_reference(h, base) + byte) % RollingHash::modulus;
    return h;
}
void verify(const RollingHash& h, std::string_view s, std::uint64_t base) {
    require(h.size() == s.size() && h.empty() == s.empty() && h.base() == base);
    require(h.whole() == hash_reference(s, base));
    for (std::size_t l = 0; l <= s.size(); ++l) {
        const auto r = l + (s.size() - l) / 2;
        require(h.get(l, r) == hash_reference(s.substr(l, r - l), base));
    }
}
std::string text(std::size_t n, unsigned seed) {
    std::string s(n, '\0');
    for (std::size_t i = 0; i < n; ++i) s[i] = static_cast<char>((i * 73 + seed) % 256);
    return s;
}
void capacity_reuse() {
    const auto s = text(1024, 9), t = text(257, 91);
    const RollingHash source(s, RollingHash::modulus - 2), empty("", 2);
    RollingHash target(s, 17);
    const auto before = allocations.load();
    const auto owned = live.load();
    budget = 0;
    for (int round = 0; round < 12; ++round) {
        target.assign(t); verify(target, t, round == 0 ? 17 : 2);
        target.clear(); verify(target, "", round == 0 ? 17 : 2);
        target.assign(s); verify(target, s, round == 0 ? 17 : 2);
        target = source; verify(target, s, source.base());
        auto* alias = &target;
        target = *alias; target = std::move(*alias);
        verify(target, s, source.base());
        target = empty; verify(target, "", 2);
    }
    budget = -1;
    require(allocations == before && live == owned);
    RollingHash moved(std::move(target));
    target.assign(t); verify(target, t, 2);
    target = source; verify(target, s, source.base());
    swap(target, moved); verify(moved, s, source.base());
    verify(target, "", 2);
    target.assign(s); verify(target, s, 2);
}
void failure_atomicity() {
    const auto s = text(7, 3), large = text(513, 19);
    const RollingHash source(large, 911382323);
    for (int operation = 0; operation < 3; ++operation) {
        bool succeeded = false;
        for (std::ptrdiff_t at = 0; at < 8; ++at) {
            RollingHash target(s, 2);
            if (operation == 2) target.clear();
            const auto owned = live.load();
            budget = at;
            try {
                if (operation == 0) target.assign(large);
                else target = source;
                budget = -1;
                verify(target, large, operation == 0 ? 2 : source.base());
                succeeded = true;
            } catch (const std::bad_alloc&) {
                budget = -1;
                ++failures;
                verify(target, operation == 2 ? std::string_view{} : s, 2);
                require(live == owned);
                // 失敗後も非所有ポインタ・コピー・照会が有効。
                RollingHash copied(target);
                verify(copied, operation == 2 ? std::string_view{} : s, 2);
            }
            verify(source, large, 911382323);
            if (succeeded) break;
        }
        require(succeeded);
    }
}
void changing_lengths() {
    for (auto base : {std::uint64_t{2}, std::uint64_t{911382323}, RollingHash::modulus - 2}) {
        RollingHash target(text(1024, 1), base);
        for (std::size_t length : {1024, 17, 513, 0, 1024, 1000, 33, 32, 1, 65, 257, 512, 8}) {
            const auto s = text(length, static_cast<unsigned>(length + 7));
            const auto before = allocations.load();
            budget = 0;
            target.assign(s);
            verify(target, s, base);
            budget = -1;
            require(allocations == before);
            const auto copy = target;
            verify(copy, s, base);
        }
    }
}
}
void* operator new(std::size_t bytes) {
    auto remaining = budget.load();
    if (remaining == 0) throw std::bad_alloc();
    if (remaining > 0) --budget;
    if (void* p = std::malloc(bytes ? bytes : 1)) {
        ++live; ++allocations; return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* p) noexcept { if (p) --live; std::free(p); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
int main() {
    const auto before = live.load();
    capacity_reuse(); failure_atomicity(); changing_lengths();
    require(live == before);
    std::printf("PASS checks=%zu failures=%zu live_delta=%td\n", checks, failures,
                live.load() - before);
}
