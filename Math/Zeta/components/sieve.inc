#ifndef SIRKIT_INTEGER_SIEVE_DEFINED
#define SIRKIT_INTEGER_SIEVE_DEFINED
// IntegerSieve(maxn): maxn>=0の最小素因数表・昇順素数列をO(maxn)時間・領域で作る。0,1の最小素因数は0。
// least_prime_factor[x]は最小素因数、prime_listは昇順素数列。表は書き換えず利用する。
// コピーは独立、コピー代入は強保証。ムーブ元は空となりmax_value()=-1。
// swap(other): 表と素数列をO(1)で交換する。
// max_value(): 表の上限を返す。is_prime(x): 0<=x<=max_value()の素数判定をO(1)で返す。
// prime_factorization(x): 1<=x<=max_value()^2の素因数と指数のmapを返す。x=1は空の表でも許す。
// enumerate_divisors(x), sorted_divisors(x): prime_factorizationと同じ前提で全約数を返す。後者だけ昇順。
// divisor_totients(x): prime_factorizationと同じ前提で各約数dからphi(d)へのmapを返す。
// mobius_table(): O(maxn)でmu表を返す。mu[0]=0,mu[1]=1。
// totient_table(): O(maxn)でphi表を返す。phi[0]=0,phi[1]=1。
// power_table<Mint>(k,nmax): 0<=nmax<=max_value(),k>=0でn^kの表。0^0=1。
// べき乗表はO(nmax+pi(nmax)log(k+1))時間・O(nmax)領域。公開配列を変更せず使う。
struct IntegerSieve {
    std::vector<int> least_prime_factor;
    std::vector<int> prime_list;

    explicit IntegerSieve(int maxn) : least_prime_factor(table_size(maxn)) {
        const std::size_t limit = least_prime_factor.size();
        for (std::size_t i = 2; i < limit; ++i) {
            if (least_prime_factor[i] == 0) {
                least_prime_factor[i] = static_cast<int>(i);
                prime_list.push_back(static_cast<int>(i));
            }
            const int first = least_prime_factor[i];
            for (const int p : prime_list) {
                const std::uint64_t next = static_cast<std::uint64_t>(i) *
                                           static_cast<std::uint64_t>(p);
                if (next >= limit) break;
                least_prime_factor[static_cast<std::size_t>(next)] = p;
                if (p == first) break;
            }
        }
    }

    IntegerSieve(const IntegerSieve&) = default;
    IntegerSieve& operator=(const IntegerSieve& other) {
        if (this != &other) {
            IntegerSieve copy(other);
            swap(copy);
        }
        return *this;
    }
    IntegerSieve(IntegerSieve&& other) noexcept
        : least_prime_factor(std::move(other.least_prime_factor)), prime_list(std::move(other.prime_list)) {
        other.least_prime_factor.clear();
        other.prime_list.clear();
    }
    IntegerSieve& operator=(IntegerSieve&& other) noexcept {
        if (this != &other) {
            least_prime_factor = std::move(other.least_prime_factor);
            prime_list = std::move(other.prime_list);
            other.least_prime_factor.clear();
            other.prime_list.clear();
        }
        return *this;
    }

    void swap(IntegerSieve& other) noexcept {
        least_prime_factor.swap(other.least_prime_factor);
        prime_list.swap(other.prime_list);
    }

    int max_value() const noexcept {
        return least_prime_factor.empty() ? -1 : static_cast<int>(least_prime_factor.size() - 1);
    }

    template <std::integral Int>
    bool is_prime(Int x) const {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (std::signed_integral<Int>) {
            if (x < 0) throw std::out_of_range("IntegerSieve::is_prime: negative input");
        }
        if (static_cast<std::uint64_t>(x) >= least_prime_factor.size()) {
            throw std::out_of_range("IntegerSieve::is_prime: input outside table");
        }
#endif
        const std::uint64_t value = static_cast<std::uint64_t>(x);
        return value >= 2 && static_cast<std::uint64_t>(least_prime_factor[static_cast<std::size_t>(value)]) == value;
    }

    template <std::integral Int>
    std::map<Int, int> prime_factorization(Int x) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (x < 1) throw std::invalid_argument("IntegerSieve::prime_factorization: input must be positive");
#endif
        std::uint64_t rest = static_cast<std::uint64_t>(x);
        std::map<Int, int> result;
        if (rest == 1) return result;
        const int maximum = max_value();
#ifndef SKIP_RUNTIME_CHECKS
        if (maximum < 1 || rest > static_cast<std::uint64_t>(maximum) *
                                     static_cast<std::uint64_t>(maximum)) {
            throw std::out_of_range("IntegerSieve::prime_factorization: input exceeds table limit squared");
        }
#endif
        const std::uint64_t bound = static_cast<std::uint64_t>(maximum);
        if (rest > bound) {
            for (const int prime : prime_list) {
                const std::uint64_t p = static_cast<std::uint64_t>(prime);
                if (p * p > rest) break;
                if (rest % p == 0) {
                    int exponent = 0;
                    do {
                        rest /= p;
                        ++exponent;
                    } while (rest % p == 0);
                    result.emplace(static_cast<Int>(prime), exponent);
                    if (rest <= bound) break;
                }
            }
            if (rest > bound) {
                result.emplace(static_cast<Int>(rest), 1);
                return result;
            }
        }
        while (rest > 1) {
            const int prime = least_prime_factor[static_cast<std::size_t>(rest)];
            int exponent = 0;
            do {
                rest /= static_cast<std::uint64_t>(prime);
                ++exponent;
            } while (rest > 1 && least_prime_factor[static_cast<std::size_t>(rest)] == prime);
            result.emplace(static_cast<Int>(prime), exponent);
        }
        return result;
    }

    template <std::integral Int>
    std::vector<Int> enumerate_divisors(Int x) const {
        const auto factors = prime_factorization(x);
        std::vector<Int> result{static_cast<Int>(1)};
        for (const auto& [prime, exponent] : factors) {
            const std::size_t previous = result.size();
            reserve_multiple(result, static_cast<std::size_t>(exponent) + 1);
            std::uint64_t power = 1;
            for (int e = 1; e <= exponent; ++e) {
                power *= static_cast<std::uint64_t>(prime);
                for (std::size_t i = 0; i < previous; ++i) {
                    result.push_back(static_cast<Int>(static_cast<std::uint64_t>(result[i]) * power));
                }
            }
        }
        return result;
    }

    template <std::integral Int>
    std::vector<Int> sorted_divisors(Int x) const {
        auto result = enumerate_divisors(x);
        std::sort(result.begin(), result.end());
        return result;
    }

    template <std::integral Int>
    std::map<Int, Int> divisor_totients(Int x) const {
        const auto factors = prime_factorization(x);
        std::vector<std::pair<Int, Int>> values{{static_cast<Int>(1), static_cast<Int>(1)}};
        for (const auto& [prime, exponent] : factors) {
            const std::size_t previous = values.size();
            reserve_multiple(values, static_cast<std::size_t>(exponent) + 1);
            const std::uint64_t p = static_cast<std::uint64_t>(prime);
            std::uint64_t power = 1;
            std::uint64_t phi_power = p - 1;
            for (int e = 1; e <= exponent; ++e) {
                power *= p;
                for (std::size_t i = 0; i < previous; ++i) {
                    values.emplace_back(
                        static_cast<Int>(static_cast<std::uint64_t>(values[i].first) * power),
                        static_cast<Int>(static_cast<std::uint64_t>(values[i].second) * phi_power));
                }
                if (e < exponent) phi_power *= p;
            }
        }
        return std::map<Int, Int>(values.begin(), values.end());
    }

    std::vector<int> mobius_table() const {
        std::vector<int> result(least_prime_factor.size());
        if (result.size() > 1) result[1] = 1;
        for (std::size_t i = 2; i < result.size(); ++i) {
            const int p = least_prime_factor[i];
            const std::size_t quotient = i / static_cast<std::size_t>(p);
            result[i] = least_prime_factor[quotient] == p ? 0 : -result[quotient];
        }
        return result;
    }

    std::vector<int> totient_table() const {
        std::vector<int> result(least_prime_factor.size());
        if (result.size() > 1) result[1] = 1;
        for (std::size_t i = 2; i < result.size(); ++i) {
            const int p = least_prime_factor[i];
            const std::size_t quotient = i / static_cast<std::size_t>(p);
            result[i] = result[quotient] * (least_prime_factor[quotient] == p ? p : p - 1);
        }
        return result;
    }

    template <class MODINT>
    std::vector<MODINT> power_table(std::int64_t k, int nmax) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (k < 0) throw std::invalid_argument("IntegerSieve::power_table: negative exponent");
        if (nmax < 0 || nmax > max_value()) {
            throw std::out_of_range("IntegerSieve::power_table: input outside table");
        }
#endif
        const std::size_t count = table_size(nmax);
        if (count > std::vector<MODINT>().max_size()) {
            throw std::length_error("IntegerSieve::power_table: table too large");
        }
        if (k == 0) return std::vector<MODINT>(count, MODINT(1));
        std::vector<MODINT> result(count);
        if (count > 0) result[0] = MODINT(0);
        if (count > 1) result[1] = MODINT(1);
        for (std::size_t i = 2; i < count; ++i) {
            const int p = least_prime_factor[i];
            if (static_cast<std::size_t>(p) == i) {
                result[i] = MODINT(p).pow(k);
            } else {
                result[i] = result[static_cast<std::size_t>(p)] *
                            result[i / static_cast<std::size_t>(p)];
            }
        }
        return result;
    }

private:
    static std::size_t table_size(int maximum) {
#ifndef SKIP_RUNTIME_CHECKS
        if (maximum < 0) throw std::invalid_argument("IntegerSieve: negative table limit");
#endif
        const std::uint64_t count = static_cast<std::uint64_t>(maximum) + 1;
        if (count > std::vector<int>().max_size()) {
            throw std::length_error("IntegerSieve: table too large");
        }
        return static_cast<std::size_t>(count);
    }

    template <class T>
    static void reserve_multiple(std::vector<T>& values, std::size_t multiplier) {
        if (multiplier > values.max_size() / values.size()) {
            throw std::length_error("IntegerSieve: divisor table too large");
        }
        values.reserve(values.size() * multiplier);
    }
};
#endif

