// SPDX-License-Identifier: CC0-1.0
#include <cstddef>
#include <atomic>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <new>
#include <vector>
namespace {
std::atomic<std::ptrdiff_t> remaining{-1};
std::atomic<std::size_t> live{0}, requests{0};
std::uint64_t checks = 0;
int current_h=0,current_w=0,current_mode=0,current_budget=0;
void check(bool x) { ++checks; if (!x) { std::fprintf(stderr,"check=%" PRIu64 " h=%d w=%d mode=%d budget=%d live=%zu remain=%td requests=%zu\n",checks,current_h,current_w,current_mode,current_budget,live.load(),remaining.load(),requests.load()); std::abort(); } }
}
[[gnu::noinline]] void* operator new(std::size_t n) {
    ++requests;
    if (remaining == 0) throw std::bad_alloc();
    if (remaining > 0) --remaining;
    if (void* p = std::malloc(n == 0 ? 1 : n)) { ++live; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void operator delete(void* p) noexcept { if (p) { --live; std::free(p); } }
[[gnu::noinline]] void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
[[gnu::noinline]] void* operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] void operator delete[](void* p) noexcept { ::operator delete(p); }
[[gnu::noinline]] void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
#ifndef MATRIX_HEADER
#define MATRIX_HEADER "../../../Algorithm/rotate.cpp"
#endif
#include MATRIX_HEADER
int main() {
    std::size_t failures = 0;
    const auto global_before = live.load();
    for (auto [h,w] : {std::pair{32,32}, {33,65}, {129,33}}) {
        std::vector<std::vector<std::uint32_t>> input(h, std::vector<std::uint32_t>(w));
        for (int i=0;i<h;++i)for(int j=0;j<w;++j)input[i][j]=std::uint32_t(i*257+j);
        const auto before = input;
        for (int mode : {0,1,3}) for (int budget=0;budget<w+2;++budget) {
            current_h=h;current_w=w;current_mode=mode;current_budget=budget;
            const auto owned = live.load();
            remaining = budget;
            try { auto output = mode==0 ? transpose(input) : mode==1 ? rotate_90r(input) : rotate_90l(input); remaining=-1; check(output.size()==std::size_t(w)); }
            catch (const std::bad_alloc&) { remaining=-1; ++failures; }
            check(input==before); check(live.load()==owned);
        }
    }
    check(live.load()==global_before);
    std::cout << "PASS checks=" << checks << " failures=" << failures << " live_delta=" << (live.load()-global_before) << '\n';
}
