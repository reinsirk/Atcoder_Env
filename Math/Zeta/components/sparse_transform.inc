#ifndef SIRKIT_INTERNAL_SPARSE_INTEGER_TRANSFORM_DEFINED
#define SIRKIT_INTERNAL_SPARSE_INTEGER_TRANSFORM_DEFINED
namespace reinsirk::detail {
// 内部用。密な範囲は位置配列、疎な範囲は素数別の伝播列を使う。
template <bool Inverse, std::integral Int, class Val>
void sparse_integer_transform(std::vector<std::pair<Int, Val>>& f,
                                      const IntegerSieve& sieve) {
    if (f.empty()) return;
    std::sort(f.begin(), f.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
#ifndef SKIP_RUNTIME_CHECKS
    if (sieve.max_value() < 0 || f.front().first <= 0
        || static_cast<std::uintmax_t>(f.back().first)
            > static_cast<std::uintmax_t>(sieve.max_value())) {
        throw std::out_of_range("sparse integer transform: key out of range");
    }
#endif
    const std::size_t m = f.size();
    const auto largest = static_cast<std::uintmax_t>(f.back().first);
    constexpr auto absent = std::numeric_limits<std::size_t>::max();
    auto propagate = [&](std::size_t src, std::size_t dst) {
        if constexpr (Inverse) f[dst].second -= f[src].second;
        else f[dst].second += f[src].second;
    };

    // 範囲をmの4倍以下に制限し、値域に比例する記憶量を避ける。
    if (largest <= std::numeric_limits<std::size_t>::max() - 1
        && (largest / m < 4 || (largest / m == 4 && largest % m == 0))) {
        const auto n = static_cast<std::size_t>(largest);
        std::vector<std::size_t> position;
        if (n >= position.max_size()) {
            throw std::length_error("sparse integer transform: position capacity");
        }
        position.assign(n + 1, absent);
        for (std::size_t i = 0; i < m; ++i) {
            const auto key = static_cast<std::size_t>(f[i].first);
#ifndef SKIP_RUNTIME_CHECKS
            if (position[key] != absent) {
                throw std::invalid_argument("sparse integer transform: duplicate key");
            }
#endif
            position[key] = i;
        }
        for (const auto raw_prime : sieve.prime_list) {
            const auto p = static_cast<std::size_t>(raw_prime);
            if (p > n) break;
            const auto limit = n / p;
            auto visit = [&](std::size_t q) {
                const auto src = position[q * p];
                if (src == absent) return;
                const auto dst = position[q];
#ifndef SKIP_RUNTIME_CHECKS
                if (dst == absent) {
                    throw std::invalid_argument("sparse integer transform: missing divisor");
                }
#endif
                propagate(src, dst);
            };
            if constexpr (Inverse) {
                for (std::size_t q = 1; q <= limit; ++q) visit(q);
            } else {
                for (std::size_t q = limit; q != 0; --q) visit(q);
            }
        }
        return;
    }

    struct Transfer {
        std::size_t src, dst, next;
    };
    std::vector<std::size_t> head(m, absent);
    std::vector<Transfer> transfers;
    if (m <= transfers.max_size() / 4) transfers.reserve(m * 4);
    else transfers.reserve(m);
    // 最大8回だけ探索し、衝突が続けば二分探索へ戻す。
    std::vector<std::size_t> slots;
    if (m > slots.max_size() / 4) {
        throw std::length_error("sparse integer transform: lookup capacity");
    }
    const auto slot_count = std::bit_ceil(m * 2);
    slots.assign(slot_count, absent);
    const auto mask = slot_count - 1;
    const auto hash_shift = std::numeric_limits<std::uint64_t>::digits
        - std::bit_width(mask);
    auto hash_key = [&](std::uintmax_t key) {
        const auto h = static_cast<std::uint64_t>(key) * UINT64_C(0x9e3779b97f4a7c15);
        return static_cast<std::size_t>(h >> hash_shift);
    };
    std::array<std::size_t, 64> small_position;
    small_position.fill(absent);
    for (std::size_t i = 0; i < m; ++i) {
#ifndef SKIP_RUNTIME_CHECKS
        if (i != 0 && f[i - 1].first == f[i].first) {
            throw std::invalid_argument("sparse integer transform: duplicate key");
        }
#endif
        const auto key = static_cast<std::uintmax_t>(f[i].first);
        if (key < small_position.size()) small_position[static_cast<std::size_t>(key)] = i;
        auto at = hash_key(key);
        for (unsigned probe = 0; probe != 8; ++probe) {
            if (slots[at] == absent) {
                slots[at] = i;
                break;
            }
            at = (at + 1) & mask;
        }
    }
    auto locate = [&](std::uintmax_t key) {
        if (key < small_position.size()) {
            const auto index = small_position[static_cast<std::size_t>(key)];
#ifndef SKIP_RUNTIME_CHECKS
            if (index == absent) {
                throw std::invalid_argument("sparse integer transform: missing divisor");
            }
#endif
            return index;
        }
        auto at = hash_key(key);
        for (unsigned probe = 0; probe != 8; ++probe) {
            const auto index = slots[at];
            if (index == absent) break;
            if (static_cast<std::uintmax_t>(f[index].first) == key) return index;
            at = (at + 1) & mask;
        }
        const auto it = std::lower_bound(f.begin(), f.end(), key,
            [](const auto& item, std::uintmax_t value) {
                return static_cast<std::uintmax_t>(item.first) < value;
            });
#ifndef SKIP_RUNTIME_CHECKS
        if (it == f.end() || static_cast<std::uintmax_t>(it->first) != key) {
            throw std::invalid_argument("sparse integer transform: missing divisor");
        }
#endif
        return static_cast<std::size_t>(it - f.begin());
    };
    for (std::size_t step = 0; step < m; ++step) {
        // 先頭への連結でゼータは降順、メビウスは昇順に伝播する。
        const auto i = Inverse ? m - 1 - step : step;
        using Factor = std::make_unsigned_t<decltype(sieve.max_value())>;
        const auto original = static_cast<Factor>(f[i].first);
        auto remaining = original;
        while (remaining > 1) {
            const auto p = static_cast<Factor>(
                sieve.least_prime_factor[static_cast<std::size_t>(remaining)]);
            const auto prime_index = locate(p);
            const auto dst = locate(original / p);
            if (transfers.size() == transfers.max_size()) {
                throw std::length_error("sparse integer transform: transfer capacity");
            }
            transfers.push_back({i, dst, head[prime_index]});
            head[prime_index] = transfers.size() - 1;
            do remaining /= p;
            while (remaining > 1
                && static_cast<Factor>(sieve.least_prime_factor[static_cast<std::size_t>(remaining)]) == p);
        }
    }
    for (const auto first : head) {
        for (auto at = first; at != absent; at = transfers[at].next) {
            const auto& transfer = transfers[at];
            propagate(transfer.src, transfer.dst);
        }
    }
}
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_SPARSE_FAST_INTEGER_ZETA_DEFINED
#define SIRKIT_SPARSE_FAST_INTEGER_ZETA_DEFINED
// m=f.size()、rは各keyの素因数を重複込みで数えた総数（以下の両API共通）。
// 正で一意なkeyの全約数を含むfをkey昇順にし、各値を倍数和へ置換。key<=sieve.max_value()、Valは+=対応。O((m+r)log(m+1))時間、O(m+r)補助領域。
template <std::integral Int, class Val>
void sparse_multiple_zeta_transform(std::vector<std::pair<Int, Val>>& f,
                              const IntegerSieve& sieve) {
    reinsirk::detail::sparse_integer_transform<false>(f, sieve);
}
#endif

#ifndef SIRKIT_SPARSE_FAST_INTEGER_MOEBIUS_DEFINED
#define SIRKIT_SPARSE_FAST_INTEGER_MOEBIUS_DEFINED
// 正で一意なkeyの全約数を含むfをkey昇順にし、倍数和の逆変換を実行。key<=sieve.max_value()、Valは-=対応。O((m+r)log(m+1))時間、O(m+r)補助領域。
template <std::integral Int, class Val>
void sparse_multiple_mobius_transform(std::vector<std::pair<Int, Val>>& f,
                                 const IntegerSieve& sieve) {
    reinsirk::detail::sparse_integer_transform<true>(f, sieve);
}
#endif
