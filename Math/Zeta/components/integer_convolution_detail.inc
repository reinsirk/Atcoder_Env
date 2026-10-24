#ifndef SIRKIT_INTEGER_CONVOLUTION_DETAIL_DEFINED
#define SIRKIT_INTEGER_CONVOLUTION_DETAIL_DEFINED
namespace reinsirk::detail {
struct IntegerConvolution {
    template <class T> static decltype(auto) product(const T& left, const T& right) {
        if constexpr (std::is_unsigned_v<T> && sizeof(T) < sizeof(unsigned int))
            return static_cast<T>(static_cast<unsigned int>(left) * static_cast<unsigned int>(right));
        else return left * right;
    }
    template <class T> static void multiply_assign(T& left, T& right) {
        if constexpr (std::is_unsigned_v<T> && sizeof(T) < sizeof(unsigned int))
            left = static_cast<T>(static_cast<unsigned int>(left) * static_cast<unsigned int>(right));
        else left *= right;
    }

    template <bool Gcd, class T, class PrimeRange>
    static std::vector<T> compute(std::vector<T> a, std::vector<T> b, const PrimeRange& primes) {
        if (a.empty()) return a;
        const T left_zero = a.front(), right_zero = b.front();
        bool zero_terms_needed = true;
        if constexpr (std::equality_comparable<T>)
            zero_terms_needed = left_zero != T{} || right_zero != T{};
        if constexpr (Gcd) {
            std::vector<T> mixed;
            if (zero_terms_needed) {
                mixed.resize(a.size());
                for (std::size_t k = 1; k < a.size(); ++k)
                    mixed[k] = product(left_zero, b[k]) + product(a[k], right_zero);
            }
            reinsirk::detail::IntegerTransform::propagate<false, false>(a, primes);
            reinsirk::detail::IntegerTransform::propagate<false, false>(b, primes);
            for (std::size_t k = 0; k < a.size(); ++k) multiply_assign(a[k], b[k]);
            reinsirk::detail::IntegerTransform::propagate<false, true>(a, primes);
            if (zero_terms_needed)
                for (std::size_t k = 1; k < a.size(); ++k) a[k] += mixed[k];
        } else {
            T at_zero = product(left_zero, right_zero);
            if (zero_terms_needed) {
                T left_sum{}, right_sum{};
                for (std::size_t k = 1; k < a.size(); ++k) {
                    left_sum += a[k]; right_sum += b[k];
                }
                at_zero += product(left_zero, right_sum) + product(left_sum, right_zero);
            }
            reinsirk::detail::IntegerTransform::propagate<true, false>(a, primes);
            reinsirk::detail::IntegerTransform::propagate<true, false>(b, primes);
            for (std::size_t k = 1; k < a.size(); ++k) multiply_assign(a[k], b[k]);
            reinsirk::detail::IntegerTransform::propagate<true, true>(a, primes);
            a.front() = at_zero;
        }
        return a;
    }
    template <bool Gcd, class T, class PrimeRange>
    static std::vector<T> square(std::vector<T> a, const PrimeRange& primes) {
        if (a.empty()) return a;
        const T zero = a.front();
        bool zero_terms_needed = true;
        if constexpr (std::equality_comparable<T>) zero_terms_needed = zero != T{};
        if constexpr (Gcd) {
            std::vector<T> mixed;
            if (zero_terms_needed) {
                mixed.resize(a.size());
                for (std::size_t k = 1; k < a.size(); ++k)
                    mixed[k] = product(zero, a[k]) + product(a[k], zero);
            }
            IntegerTransform::propagate<false, false>(a, primes);
            for (std::size_t k = 0; k < a.size(); ++k) multiply_assign(a[k], a[k]);
            IntegerTransform::propagate<false, true>(a, primes);
            if (zero_terms_needed)
                for (std::size_t k = 1; k < a.size(); ++k) a[k] += mixed[k];
        } else {
            T at_zero = product(zero, zero);
            if (zero_terms_needed) {
                T sum{};
                for (std::size_t k = 1; k < a.size(); ++k) sum += a[k];
                at_zero += product(zero, sum) + product(sum, zero);
            }
            IntegerTransform::propagate<true, false>(a, primes);
            for (std::size_t k = 1; k < a.size(); ++k) multiply_assign(a[k], a[k]);
            IntegerTransform::propagate<true, true>(a, primes);
            a.front() = at_zero;
        }
        return a;
    }
};
} // namespace reinsirk::detail
#endif
