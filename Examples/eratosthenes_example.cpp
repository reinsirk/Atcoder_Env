#include "../Math/Eratosthenes.cpp"
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    for (auto p : eratosthenes(20)) std::cout << p << ' ';
    std::cout << '\n'; // 2 3 5 7 11 13 17 19
    std::vector<std::uint64_t> output{101};
    eratosthenes(5, output); // 従来のAPIは末尾への追記。
    for (auto p : output) std::cout << p << ' ';
    std::cout << '\n'; // 101 2 3 5
    PrimeSieve sieve(100);
    std::cout << sieve.is_prime(97) << ' ' << sieve.is_prime(1) << '\n'; // 1 0
    std::cout << sieve.limit() << ' ' << sieve.prime_count() << '\n'; // 100 25
    const auto primes = sieve.primes();
    std::cout << primes.back() << '\n'; // 97
    std::vector<std::uint64_t> copy;
    sieve.append_to(copy);
    std::cout << (primes == copy) << ' ' << eratosthenes(0).empty() << '\n'; // 1 1
}
