#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/abc328/tasks/abc328_e

// Library: Algorithm/next_combination.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://github.com/drken1215/algorithm/blob/master/Search/next_combination.cpp
#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

#ifndef SIRKIT_NEXT_COMBINATION_DEFINED
#define SIRKIT_NEXT_COMBINATION_DEFINED
// next_combination(sub): 同じ1の個数で次に大きいマスク。0・型内に次がなければ0。O(1)。
// unsigned整数型に対応。既存int版は非負が前提で、正のintに収まらない次も0。
template <std::unsigned_integral UInt> requires (!std::same_as<UInt, bool>)
constexpr UInt next_combination(UInt sub) noexcept {
    if (sub == 0) return 0;
    const UInt low = static_cast<UInt>(sub & static_cast<UInt>(UInt{0} - sub));
    const UInt high = static_cast<UInt>(sub + low);
    if (high == 0) return 0;
    const UInt tail = static_cast<UInt>(static_cast<UInt>((sub ^ high) >> 2) >> std::countr_zero(sub));
    return static_cast<UInt>(high | tail);
}
inline constexpr int next_combination(int sub) {
#ifndef SKIP_RUNTIME_CHECKS
    if (sub < 0) throw std::out_of_range("next_combination: negative mask");
#endif
    const auto next = next_combination(static_cast<unsigned int>(sub));
    return next > static_cast<unsigned int>(std::numeric_limits<int>::max()) ? 0 : static_cast<int>(next);
}
#endif

#ifndef SIRKIT_COMBINATION_MASK_UTILITIES_DEFINED
#define SIRKIT_COMBINATION_MASK_UTILITIES_DEFINED
// first_combination<UInt=uint64_t>(k): 下位kビットが1の最小マスク。0<=k<=型幅。O(1)。
// next_combination(sub, n): 下位nビット内の次へsubを進める。次がなければfalseでsub不変。O(1)。
// for_each_combination<UInt=uint64_t>(n,k,visit): nビット内のk個の1を持つ全マスクを昇順にvisit(mask)へ渡す。k=0も1回。
// for_eachはC(n,k)回の呼出し・O(1)追加領域（visitの処理を除く）。入力検査は最初だけ行う。
// n,k,maskの検査はSKIPで省く。全幅のシフト・0除算を避ける処理は常に維持する。
namespace reinsirk::detail {
template <std::unsigned_integral UInt> requires (!std::same_as<UInt, bool>)
constexpr bool NextCombinationBounded(UInt& sub, int n) noexcept {
    constexpr int width = std::numeric_limits<UInt>::digits;
    const auto next = next_combination(sub);
    if (next == 0 || n <= 0 || (n < width && (next >> n) != 0)) return false;
    sub = next;
    return true;
}
} // namespace reinsirk::detail
template <std::unsigned_integral UInt = std::uint64_t> requires (!std::same_as<UInt, bool>)
constexpr UInt first_combination(int k) {
    constexpr int width = std::numeric_limits<UInt>::digits;
#ifndef SKIP_RUNTIME_CHECKS
    if (k < 0 || k > width) throw std::out_of_range("first_combination: invalid count");
#endif
    if (k <= 0) return 0;
    if (k >= width) return std::numeric_limits<UInt>::max();
    return static_cast<UInt>((UInt{1} << k) - UInt{1});
}
template <std::unsigned_integral UInt> requires (!std::same_as<UInt, bool>)
constexpr bool next_combination(UInt& sub, int n) {
#ifndef SKIP_RUNTIME_CHECKS
    constexpr int width = std::numeric_limits<UInt>::digits;
    if (n < 0 || n > width || (n < width && (sub >> n) != 0))
        throw std::out_of_range("next_combination: invalid width or mask");
#endif
    return reinsirk::detail::NextCombinationBounded(sub, n);
}
template <std::unsigned_integral UInt = std::uint64_t, class Visitor> requires (!std::same_as<UInt, bool>)
constexpr void for_each_combination(int n, int k, Visitor&& visit) {
#ifndef SKIP_RUNTIME_CHECKS
    constexpr int width = std::numeric_limits<UInt>::digits;
    if (n < 0 || n > width || k < 0 || k > n)
        throw std::out_of_range("for_each_combination: invalid width or count");
#endif
    if (n < 0 || k < 0 || k > n) return;
    UInt mask = first_combination<UInt>(k);
    do { visit(UInt{mask}); } while (reinsirk::detail::NextCombinationBounded(mask, n));
}
#endif


// SPDX-License-Identifier: CC0-1.0
// Verify I/O: Verify/current/common/fast_io.hpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef SIRKIT_VERIFY_FAST_IO_DEFINED
#define SIRKIT_VERIFY_FAST_IO_DEFINED

// VerifyInput(file=stdin): 非所有のFILEから読む。別の入力方式と混用しない。
// >> 整数: 64bit以下の十進整数を読む。検査省略時は値域内の正しい入力を前提とする。
// >> char/string: 空白を飛ばし、1文字／空白までの文字列を読む。
// >> 浮動小数点: 十進文字列を変換する。通常のCロケールで使う。
// bool / !: 直前までの読み取りが成功しているか／失敗したかを返す。
class VerifyInput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<unsigned char, buffer_size> buffer_;
    std::size_t position_ = 0, size_ = 0;
    bool good_ = true;

    int peek() {
        if (position_ == size_) {
            size_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            position_ = 0;
            if (size_ == 0) return EOF;
        }
        return buffer_[position_];
    }

    int skip_space() {
        int c;
        while ((c = peek()) != EOF && c <= ' ') ++position_;
        if (c == EOF) good_ = false;
        return c;
    }

public:
    explicit VerifyInput(std::FILE* file = stdin) : file_(file) {}
    VerifyInput(const VerifyInput&) = delete;
    VerifyInput& operator=(const VerifyInput&) = delete;

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyInput& operator>>(T& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        bool negative = false;
        if (c == '-' || c == '+') {
            negative = c == '-';
            ++position_;
            c = peek();
        }
        if (c == EOF) {
            good_ = false;
            return *this;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (c < '0' || c > '9') {
            good_ = false;
            return *this;
        }
        // 先頭の0を除けば、20桁以下の末尾直前の値はuint64_tに収まる。
        while (c == '0') {
            ++position_;
            c = peek();
        }
        std::uint64_t previous = 0;
        unsigned last_digit = 0;
        std::size_t digits = 0;
#endif
        std::uint64_t value = 0;
        while (c >= '0' && c <= '9') {
#ifndef SKIP_RUNTIME_CHECKS
            previous = value;
            last_digit = static_cast<unsigned>(c - '0');
            if (digits <= 20) ++digits;
#endif
            value = value * 10 + static_cast<unsigned>(c - '0');
            ++position_;
            c = peek();
        }
#ifndef SKIP_RUNTIME_CHECKS
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if ((c != EOF && c > ' ') || digits > 20
            || (digits == 20 && (previous > maximum / 10
                || (previous == maximum / 10 && last_digit > maximum % 10)))) {
            good_ = false;
            return *this;
        }
        std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max());
        if constexpr (std::is_signed_v<T>) limit += negative;
        else if (negative && value != 0) good_ = false;
        if (value > limit) good_ = false;
        if (!good_) return *this;
#endif
        if constexpr (std::is_signed_v<T>) {
            constexpr std::uint64_t minimum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<T>::max()) + 1;
            // -(最小値)は評価せず、絶対値が最大値+1のときだけ最小値を使う。
            result = negative
                ? (value == minimum_magnitude ? std::numeric_limits<T>::min()
                                  : static_cast<T>(-static_cast<T>(value)))
                : static_cast<T>(value);
        } else result = static_cast<T>(value);
        return *this;
    }

    VerifyInput& operator>>(char& result) {
        if (!good_) return *this;
        const int c = skip_space();
        if (c != EOF) {
            result = static_cast<char>(c);
            ++position_;
        }
        return *this;
    }

    VerifyInput& operator>>(std::string& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        result.clear();
        do {
            result.push_back(static_cast<char>(c));
            ++position_;
            c = peek();
        } while (c != EOF && c > ' ');
        return *this;
    }

    template<class T> requires std::is_floating_point_v<T>
    VerifyInput& operator>>(T& result) {
        std::string token;
        *this >> token;
        if (!good_) return *this;
#ifndef SKIP_RUNTIME_CHECKS
        char* end = nullptr;
        const long double value = std::strtold(token.c_str(), &end);
        if (end != token.c_str() + token.size()) good_ = false;
        else result = static_cast<T>(value);
#else
        result = static_cast<T>(std::strtold(token.c_str(), nullptr));
#endif
        return *this;
    }
};

// VerifyOutput(file=stdout): 非所有のFILEへ書く。破棄時にflushし、FILEは閉じない。
// << 整数/char/文字列/浮動小数点: 値を出力する。整数は64bit以下に対応する。
// fixed(digits): 浮動小数点を小数点以下digits桁で出力する（digits>=0）。
// flush(): バッファの内容をFILEへ渡す。bool / !で書き込み失敗を確認できる。
class VerifyOutput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<char, buffer_size> buffer_;
    std::size_t size_ = 0;
    int precision_ = 6;
    bool fixed_ = false, good_ = true;

    void write(std::string_view text) {
        while (!text.empty() && good_) {
            if (size_ == buffer_.size()) flush();
            const auto count = std::min(text.size(), buffer_.size() - size_);
            std::memcpy(buffer_.data() + size_, text.data(), count);
            size_ += count;
            text.remove_prefix(count);
        }
    }

public:
    explicit VerifyOutput(std::FILE* file = stdout) : file_(file) {}
    VerifyOutput(const VerifyOutput&) = delete;
    VerifyOutput& operator=(const VerifyOutput&) = delete;
    ~VerifyOutput() { flush(); }

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    VerifyOutput& fixed(int digits) {
        if (digits < 0) throw std::invalid_argument("negative output precision");
        fixed_ = true;
        precision_ = digits;
        return *this;
    }

    void flush() noexcept {
        std::size_t written = 0;
        while (written < size_ && good_) {
            const auto count = std::fwrite(buffer_.data() + written, 1, size_ - written, file_);
            if (count == 0) good_ = false;
            written += count;
        }
        size_ = 0;
    }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyOutput& operator<<(T value) {
        if constexpr (std::is_same_v<T, bool>) return *this << static_cast<char>('0' + value);
        else {
            char digits[std::numeric_limits<std::uint64_t>::digits10 + 3];
            const auto end = std::to_chars(digits, digits + sizeof(digits), value).ptr;
            write(std::string_view(digits, static_cast<std::size_t>(end - digits)));
            return *this;
        }
    }

    VerifyOutput& operator<<(char value) {
        if (size_ == buffer_.size()) flush();
        if (good_) buffer_[size_++] = value;
        return *this;
    }

    VerifyOutput& operator<<(std::string_view value) { write(value); return *this; }
    VerifyOutput& operator<<(const std::string& value) { write(value); return *this; }
    VerifyOutput& operator<<(const char* value) { write(value); return *this; }

    template<class T> requires std::is_floating_point_v<T>
    VerifyOutput& operator<<(T value) {
        char local[128];
        const char* format = fixed_ ? "%.*Lf" : "%.*Lg";
        const int count = std::snprintf(local, sizeof(local), format, precision_,
                                        static_cast<long double>(value));
        if (count < 0) good_ = false;
        else if (static_cast<std::size_t>(count) < sizeof(local))
            write(std::string_view(local, static_cast<std::size_t>(count)));
        else {
            std::vector<char> text(static_cast<std::size_t>(count) + 1);
            std::snprintf(text.data(), text.size(), format, precision_, static_cast<long double>(value));
            write(std::string_view(text.data(), static_cast<std::size_t>(count)));
        }
        return *this;
    }
};

inline VerifyInput verify_in;
inline VerifyOutput verify_out;
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <vector>
int main(){
    int n{},m{};std::int64_t k{};verify_in>>n>>m>>k;
    struct E{int u{},v{};std::int64_t w{};};std::vector<E> edges(m);
    for(auto&e:edges){verify_in>>e.u>>e.v>>e.w;--e.u;--e.v;}
    std::int64_t answer=k;
    for_each_combination<std::uint32_t>(m,n-1,[&](std::uint32_t mask){
        std::array<int,8> parent;parent.fill(-1);std::int64_t sum=0;bool tree=true;
        auto root=[&](int u){while(parent[u]>=0)u=parent[u];return u;};
        while(mask){int i=std::countr_zero(mask);mask&=mask-1;const auto&e=edges[i];int u=root(e.u),v=root(e.v);
            if(u==v){tree=false;break;}parent[u]+=parent[v];parent[v]=u;sum+=e.w;
        }
        if(tree)answer=std::min(answer,sum%k);
    });verify_out<<answer<<'\n';
}
