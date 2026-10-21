// SPDX-License-Identifier: CC0-1.0
#include "../common/fast_io.hpp"
#include "../common/fast_io.hpp"
#include <cassert>
#include <cmath>
#include <memory>
#include <random>
#include <iostream>
struct CloseFile { void operator()(std::FILE* f) const noexcept { std::fclose(f); } };
using File = std::unique_ptr<std::FILE, CloseFile>;
std::uint64_t checks = 0;
void check(bool ok) { if(!ok) std::abort(); ++checks; }
File input(const std::string& s) {
    File f(std::tmpfile()); check(bool(f));
    check(std::fwrite(s.data(), 1, s.size(), f.get()) == s.size());
    std::rewind(f.get()); return f;
}
std::string read_all(std::FILE* f) {
    std::rewind(f); std::string result; char data[8192];
    while (auto n = std::fread(data,1,sizeof(data),f)) result.append(data,n);
    return result;
}
template<class T> void boundary(T value) {
    char buf[32]; auto end=std::to_chars(buf,buf+32,value).ptr;
    std::string number(buf,end);
    for (std::size_t offset : {0u, 1u, 131050u, 131060u, 131070u, 131071u, 131072u, 131073u, 262140u}) {
        auto f=input(std::string(offset,' ') + number);
        VerifyInput in(f.get()); T actual=0;
        in>>actual; check(bool(in)); check(actual==value);
        actual=42; in>>actual; check(!in); check(actual==42);
    }
}
template<class T> void invalid(const std::string& text) {
    auto f=input(text); VerifyInput in(f.get()); T value=42; in>>value;
    check(!in); check(value==42);
}
int main() {
    boundary<std::int8_t>(-128); boundary<std::int8_t>(127);
    boundary<std::uint8_t>(255);
    boundary<std::int16_t>(-32768); boundary<std::int16_t>(32767);
    boundary<std::uint16_t>(65535);
    boundary<std::int32_t>(std::numeric_limits<std::int32_t>::min());
    boundary<std::int32_t>(std::numeric_limits<std::int32_t>::max());
    boundary<std::uint32_t>(std::numeric_limits<std::uint32_t>::max());
    boundary<std::int64_t>(std::numeric_limits<std::int64_t>::min());
    boundary<std::int64_t>(std::numeric_limits<std::int64_t>::max());
    boundary<std::uint64_t>(std::numeric_limits<std::uint64_t>::max());
    boundary<std::int64_t>(0);
#ifndef SKIP_RUNTIME_CHECKS
    for (auto s : {"18446744073709551616", "18446744073709551620", "99999999999999999999", "100000000000000000000", "-1", "1x", "+", "-", "", " \t\r\n"}) invalid<std::uint64_t>(s);
    invalid<std::int64_t>("9223372036854775808"); invalid<std::int64_t>("-9223372036854775809");
    invalid<std::uint32_t>("4294967296"); invalid<std::int32_t>("2147483648");
    invalid<std::int32_t>("-2147483649"); invalid<std::int8_t>("-129");
#endif
    {
        auto f=input("+0000000000000000000000000000000000000000000000000123\t-0000\n1 0");
        VerifyInput in(f.get()); int a; unsigned b; bool c,d; in>>a>>b>>c>>d;
        check(bool(in)); check(a==123&&b==0&&c&&!d); check(bool(in));
    }
    {
        auto f=input(" \r\n\tX hello 1.25 -3.5e+2 0.03125"); VerifyInput in(f.get());
        char c=0; std::string s; float a=0; double b=0; long double d=0;
        in>>c>>s>>a>>b>>d; check(bool(in));
        check(c=='X'&&s=="hello"&&a==1.25f&&b==-350&&d==0.03125L);
        in>>c; check(!in);
    }
    {
        auto f=input(std::string(131070,' ') + std::string(400000,'x'));
        VerifyInput in(f.get()); std::string s; in>>s; check(bool(in)); check(s==std::string(400000,'x'));
    }
    {
        auto f=input("0"); VerifyInput in(f.get()); int v; in>>v; check(v==0&&bool(in)); in>>v; check(!in);
    }
    File f(std::tmpfile()); check(bool(f)); std::string expected;
    std::mt19937_64 random(98001212);
    {
        VerifyOutput out(f.get());
        for (int i=0;i<300000;++i) {
            auto u=random(); auto s=static_cast<std::int64_t>(random());
            out<<u<<' '<<s<<'\n';
            char buf[32]; auto e=std::to_chars(buf,buf+32,u).ptr; expected.append(buf,e);expected+=' ';
            e=std::to_chars(buf,buf+32,s).ptr; expected.append(buf,e);expected+='\n';
        }
        out<<std::numeric_limits<std::int64_t>::min()<<' '<<std::numeric_limits<std::uint64_t>::max()<<'\n';
        expected+="-9223372036854775808 18446744073709551615\n";
        out<<true<<false<<" hello "<<std::string("world")<<std::string_view("!\n");
        expected+="10 hello world!\n";
        out.fixed(12)<<1.25L<<' '<<-0.0<<'\n'; expected+="1.250000000000 -0.000000000000\n";
        out<<std::string(400000,'z');expected+=std::string(400000,'z');
        out.flush();check(bool(out));
        out<<" tail";expected+=" tail"; // 破棄時のflush
    }
    check(read_all(f.get())==expected);
    std::rewind(f.get()); random.seed(98001212);
    {
        VerifyInput in(f.get());
        for (int i=0;i<300000;++i) {
            auto u=random(); auto s=static_cast<std::int64_t>(random()); std::uint64_t a=0; std::int64_t b=0;
            in>>a>>b;check(bool(in));check(a==u&&b==s);
        }
    }
    {
        File large(std::tmpfile()); check(bool(large));
        { VerifyOutput out(large.get());out.fixed(12)<<std::numeric_limits<long double>::max();check(bool(out)); }
        char expected[10000]; const auto n=std::snprintf(expected,sizeof(expected),"%.12Lf",std::numeric_limits<long double>::max());
        check(n>0);check(read_all(large.get())==std::string(expected,static_cast<std::size_t>(n)));
    }
    std::cout<<"OK "<<checks<<" checks\n";
}
