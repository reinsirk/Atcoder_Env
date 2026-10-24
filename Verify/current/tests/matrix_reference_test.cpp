// SPDX-License-Identifier: CC0-1.0
#ifndef MATRIX_HEADER
#define MATRIX_HEADER "../../../Algorithm/rotate.cpp"
#endif
#include MATRIX_HEADER
#include MATRIX_HEADER
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>
struct Token{int v;explicit Token(int x):v(x){};bool operator==(const Token&)const=default;};
template<class Row>std::vector<Row> ref(const std::vector<Row>&a,int turns,bool transpose_only=false){int h=int(a.size()),w=h?int(a[0].size()):0;int r=((turns%4)+4)%4;if(transpose_only||r%2){std::vector<Row>b(w);for(int j=0;j<w;++j)for(int i=0;i<h;++i){if(transpose_only)b[j].push_back(a[i][j]);else if(r==1)b[j].push_back(a[h-1-i][j]);else b[w-1-j].push_back(a[i][j]);}return b;}auto b=a;if(r==2){std::reverse(b.begin(),b.end());for(auto&row:b)std::reverse(row.begin(),row.end());}return b;}
template<class Row>void check(const std::vector<Row>&a){auto original=a;auto right=ref(a,1);auto up=a;std::reverse(up.begin(),up.end());auto horizontal=a;for(auto&row:horizontal)std::reverse(row.begin(),row.end());auto legacy_right=right,legacy_up=up,legacy_horizontal=horizontal;if(a.empty()){legacy_right.resize(1);legacy_up.resize(1);legacy_horizontal.resize(1);}assert(rotate_90r(a)==legacy_right);assert(flip_i(a)==legacy_up);assert(flip_j(a)==legacy_horizontal);assert(rotate_90l(a)==ref(a,3));assert(rotate_180(a)==ref(a,2));assert(transpose(a)==ref(a,0,true));for(int t:{-9,-5,-4,-3,-2,-1,0,1,2,3,4,9,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()})assert(rotate_quarter(a,t)==ref(a,t));auto v=a;flip_i_inplace(v);assert(v==up);flip_i_inplace(v);assert(v==a);v=a;flip_j_inplace(v);assert(v==horizontal);flip_j_inplace(v);assert(v==a);if(!a.empty()&&!a[0].empty()){assert(rotate_quarter(rotate_quarter(a,1),3)==a);assert(transpose(transpose(a))==a);assert(rotate_180(rotate_180(a))==a);}assert(a==original);auto detached=rotate_quarter(a,0);if(!a.empty()&&!a[0].empty()){detached[0].clear();assert(a==original);}}
struct CopyOnly { int value; explicit CopyOnly(int v):value(v){} CopyOnly(const CopyOnly&)=default; CopyOnly& operator=(const CopyOnly&)=delete; bool operator==(const CopyOnly&)const=default; };
struct ThrowCopy {
    int value;
    inline static int remaining = -1;
    inline static int alive = 0;
    explicit ThrowCopy(int v):value(v){++alive;}
    ThrowCopy(const ThrowCopy& other):value(other.value){if(remaining==0)throw std::runtime_error("copy");if(remaining>0)--remaining;++alive;}
    ThrowCopy(ThrowCopy&& other) noexcept:value(other.value){++alive;}
    ThrowCopy& operator=(const ThrowCopy&)=delete;
    ~ThrowCopy(){--alive;}
};
int main(){
    {
        std::vector<CopyOnly> row; for(int i=0;i<33;++i)row.emplace_back(i);
        std::vector<std::vector<CopyOnly>> a(33,row);
        assert(rotate_quarter(rotate_90r(a),-1)==a);
    }
    {
        std::vector<std::vector<ThrowCopy>> a(8);
        for(auto& row:a)for(int i=0;i<12;++i)row.emplace_back(i);
        const int before=ThrowCopy::alive;
        for(int budget=0;budget<96;++budget){
            ThrowCopy::remaining=budget; bool thrown=false;
            try{(void)rotate_90r(a);}catch(const std::runtime_error&){thrown=true;}
            assert(thrown && ThrowCopy::alive==before);
            for(const auto& row:a)for(int i=0;i<12;++i)assert(row[i].value==i);
        }
        ThrowCopy::remaining=-1;
    }
    assert(ThrowCopy::alive==0);
std::mt19937 rng(91713);std::uint64_t cases=0;for(int h=0;h<40;++h)for(int w=0;w<40;++w){std::vector<std::vector<int>>a(h,std::vector<int>(w));std::vector<std::vector<bool>>bits(h,std::vector<bool>(w));std::vector<std::string>board(h,std::string(w,' '));for(int i=0;i<h;++i)for(int j=0;j<w;++j){a[i][j]=int(rng());bits[i][j]=(rng()&1);board[i][j]=static_cast<char>('a'+rng()%26);}check(a);check(bits);check(board);cases+=3;}for(auto[h,w]:std::vector<std::pair<int,int>>{{1,8192},{8192,1},{32,32768},{32768,32},{1024,1024}}){std::vector<std::vector<int>>a(h,std::vector<int>(w));for(auto&r:a)for(auto&x:r)x=int(rng());check(a);++cases;}check(std::vector<std::vector<Token>>{{Token(1),Token(2)},{Token(3),Token(4)}});check(std::vector<std::vector<std::string>>{{"ab","cd"},{"longer string value",""}});
#ifndef SKIP_RUNTIME_CHECKS
 auto bad=std::vector<std::vector<int>>{{1,2},{3},{4,5},{6,7}};const auto saved=bad;auto throws=[](auto f){bool ok=false;try{f();}catch(const std::invalid_argument&){ok=true;}assert(ok);};throws([&]{(void)rotate_90r(bad);});throws([&]{(void)rotate_90l(bad);});throws([&]{(void)rotate_180(bad);});throws([&]{(void)transpose(bad);});throws([&]{(void)flip_i(bad);});throws([&]{(void)flip_j(bad);});throws([&]{(void)rotate_quarter(bad,0);});assert(saved==bad);
 auto v=bad;throws([&]{flip_i_inplace(v);});v=bad;throws([&]{flip_j_inplace(v);});
#endif
 std::cout<<cases+2<<" independent matrix datasets passed\n";
}
