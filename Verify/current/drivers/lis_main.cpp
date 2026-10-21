// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>
#include <vector>
int main(){
    int n{};verify_in>>n;std::vector<std::int64_t>a(n);for(auto&x:a)verify_in>>x;
    const auto answer=LISIndices<true>(a);verify_out<<answer.size()<<'\n';
    for(std::size_t i=0;i<answer.size();++i)verify_out<<(i?" ":"")<<answer[i];
    verify_out<<'\n';
}
