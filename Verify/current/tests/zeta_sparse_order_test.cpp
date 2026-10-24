// SPDX-License-Identifier: CC0-1.0
// 連続キー経路の更新順と、各算術例外点での部分状態を逐次処理と照合する。
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include <utility>
#include <vector>
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta/Sparse_Transform.cpp"
#endif
#include SOURCE
struct Fault{};
struct Event{std::uint32_t to,from;bool inverse;friend bool operator==(const Event&,const Event&)=default;};
static std::vector<Event> events;
static std::int64_t allowance=-1;
static std::size_t checks=0,operations=0,failures=0;
static void need(bool x){++checks;if(!x)std::abort();}
struct TraceVal{
 std::int32_t value;std::uint32_t id;
 TraceVal&operator+=(const TraceVal& x){events.push_back({id,x.id,false});if(allowance==0)throw Fault{};if(allowance>0)--allowance;value+=x.value;return *this;}
 TraceVal&operator-=(const TraceVal& x){events.push_back({id,x.id,true});if(allowance==0)throw Fault{};if(allowance>0)--allowance;value-=x.value;return *this;}
 friend bool operator==(const TraceVal&,const TraceVal&)=default;
};
static_assert(sizeof(TraceVal)==8&&std::is_trivially_copyable_v<TraceVal>);
using Data=std::vector<std::pair<int,TraceVal>>;
static void baseline(Data& f,const IntegerSieve& sieve,bool inverse){const auto n=f.size();for(int raw:sieve.prime_list){const auto p=std::size_t(raw);if(p>n)break;const auto count=n/p;if(inverse){for(std::size_t q=1;q<=count;++q)f[q-1].second-=f[q*p-1].second;}else{for(std::size_t q=count;q;--q)f[q-1].second+=f[q*p-1].second;}}}
static bool apply(Data& f,const IntegerSieve& sieve,bool inverse,bool old,std::int64_t fail){events.clear();allowance=fail;try{if(old)baseline(f,sieve,inverse);else if(inverse)sparse_multiple_mobius_transform(f,sieve);else sparse_multiple_zeta_transform(f,sieve);allowance=-1;return true;}catch(const Fault&){allowance=-1;return false;}}
int main(){IntegerSieve sieve(1000);std::vector<int> sizes;for(int n=0;n<=33;++n)sizes.push_back(n);for(int n:{63,64,65,127,128,129,255,256,257})sizes.push_back(n);for(int n:sizes)for(bool inverse:{false,true}){Data input;for(int k=1;k<=n;++k)input.emplace_back(k,TraceVal{k%7-3,std::uint32_t(k)});auto expected=input;need(apply(expected,sieve,inverse,true,-1));auto trace=events;operations+=trace.size();auto actual=input;need(apply(actual,sieve,inverse,false,-1));need(actual==expected);need(events==trace);for(std::size_t fail=0;fail<trace.size();++fail){expected=input;actual=input;need(!apply(expected,sieve,inverse,true,static_cast<std::int64_t>(fail)));auto expected_events=events;need(!apply(actual,sieve,inverse,false,static_cast<std::int64_t>(fail)));need(actual==expected);need(events==expected_events);++failures;}}
 std::printf("PASS checks=%zu sizes=%zu operations=%zu failed_arithmetic=%zu\n",checks,sizes.size(),operations,failures);
}
