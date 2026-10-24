// SPDX-License-Identifier: CC0-1.0
// 確保失敗時のコピー代入と、容量を再利用する代入を照合する。
#ifndef SIEVE_SOURCE
#define SIEVE_SOURCE "../../../Math/Eratosthenes.cpp"
#endif
#include SIEVE_SOURCE
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <utility>

namespace {
bool fail_next = false;
std::size_t allocations = 0, live_allocations = 0, checks = 0;
void require(bool condition) {
    ++checks;
    if (!condition) {
        std::fprintf(stderr, "prime sieve allocation check %zu failed\n", checks);
        std::abort();
    }
}
}

#if defined(__GNUC__) || defined(__clang__)
#define SIRKIT_TEST_NOINLINE [[gnu::noinline]]
#else
#define SIRKIT_TEST_NOINLINE
#endif
// 確保カウンタをClangのインライン最適化から分離する。
SIRKIT_TEST_NOINLINE void* operator new(std::size_t size) {
    if (std::exchange(fail_next, false)) throw std::bad_alloc();
    if (void* result = std::malloc(size == 0 ? 1 : size)) {
        ++allocations;
        ++live_allocations;
        return result;
    }
    throw std::bad_alloc();
}
SIRKIT_TEST_NOINLINE void* operator new[](std::size_t size) { return ::operator new(size); }
SIRKIT_TEST_NOINLINE void operator delete(void* pointer) noexcept {
    if (pointer != nullptr) {
        --live_allocations;
        std::free(pointer);
    }
}
SIRKIT_TEST_NOINLINE void operator delete[](void* pointer) noexcept { ::operator delete(pointer); }
SIRKIT_TEST_NOINLINE void operator delete(void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
SIRKIT_TEST_NOINLINE void operator delete[](void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
#undef SIRKIT_TEST_NOINLINE

int main() {
    {
        PrimeSieve source(1000000), destination(31);
        const auto expected = destination.primes();
        fail_next = true;
        bool thrown = false;
        try { destination = source; } catch (const std::bad_alloc&) { thrown = true; }
        fail_next = false;
        require(thrown);
        require(destination.limit() == 31);
        require(destination.primes() == expected);
        require(source.limit() == 1000000 && source.prime_count() == 78498);
        destination = source;
        require(destination.primes() == source.primes());
        PrimeSieve small(127);
        const auto before = allocations;
        destination = small;
        require(allocations == before);
        require(destination.limit() == 127 && destination.primes() == small.primes());
        auto* alias = &destination;
        fail_next = true;
        destination = *alias;
        require(fail_next);
        fail_next = false;
        require(destination.limit() == 127 && destination.prime_count() == 31);
        // 64bit環境ではUINT64_MAXでも数学上のword数はmax_size以下。
        // 巨大確保そのものは行わず、最初の確保を故意に失敗させる。
        fail_next = true;
        thrown = false;
        try { PrimeSieve oversized(std::numeric_limits<std::uint64_t>::max()); }
        catch (const std::bad_alloc&) { thrown = true; }
        catch (const std::length_error&) { thrown = true; }
        fail_next = false;
        require(thrown);
        require(destination.limit() == 127 && destination.prime_count() == 31);
    }
    require(live_allocations == 0);
    std::printf("prime_sieve_allocation_test: %zu checks passed, live allocations %zu\n",
                checks, live_allocations);
}
