// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RH_REQUIRE_EXTRA
#define SIRKIT_RH_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_RH_SOURCE
#define SIRKIT_RH_SOURCE "../../../String/RollingHash.cpp"
#endif
#include SIRKIT_RH_SOURCE
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <type_traits>

static volatile std::ptrdiff_t countdown=-1;
static volatile std::size_t live=0,calls=0;
static std::size_t checks=0,failures=0;
[[gnu::noinline]] void* operator new(std::size_t size){if(countdown==0)throw std::bad_alloc();if(countdown>0)countdown=countdown-1;if(void*p=std::malloc(size?size:1)){live=live+1;calls=calls+1;return p;}throw std::bad_alloc();}
[[gnu::noinline]] void* operator new[](std::size_t size){return ::operator new(size);}
[[gnu::noinline]] static void release(void*p)noexcept{if(p)live=live-1;std::free(p);}
void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}void operator delete(void*p,std::size_t)noexcept{release(p);}void operator delete[](void*p,std::size_t)noexcept{release(p);}
static void require_impl(bool ok,const char*what,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s\n",line,what);std::abort();}}
#define require(x) require_impl(bool(x),#x,__LINE__)
static std::string text(std::size_t n){std::string s(n,'\0');for(std::size_t i=0;i<n;++i)s[i]=static_cast<char>((i*73)%256);return s;}
struct Snapshot{std::size_t size;std::uint64_t base,whole;std::vector<std::uint64_t>prefix,suffix;};
static Snapshot capture(const RollingHash&a){Snapshot x{a.size(),a.base(),a.whole(),{}, {}};for(std::size_t i=0;i<=a.size();++i){x.prefix.push_back(a.get(0,i));x.suffix.push_back(a.get(i,a.size()));}return x;}
static void unchanged(const RollingHash&a,const Snapshot&x){require(a.size()==x.size);require(a.base()==x.base);require(a.whole()==x.whole);for(std::size_t i=0;i<=a.size();++i){require(a.get(0,i)==x.prefix[i]);require(a.get(i,a.size())==x.suffix[i]);}}
static void constructor_fail(){const auto s=text(257);bool success=false;for(int k=0;k<16;++k){const auto before=live;bool failed=false;countdown=k;try{RollingHash a(s,1000000007);countdown=-1;require(a.size()==s.size());}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==before);if(!failed){success=true;break;}}require(success);}
static void copy_constructor_fail(){RollingHash source(text(257),1000000007);const auto src=capture(source);bool success=false;for(int k=0;k<16;++k){const auto before=live;bool failed=false;countdown=k;try{RollingHash copy(source);countdown=-1;unchanged(copy,src);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}unchanged(source,src);require(live==before);if(!failed){success=true;break;}}require(success);}
static void assignment_fail(){RollingHash source(text(257),1000000007);const auto src=capture(source);bool success=false;for(int k=0;k<16;++k){RollingHash target("different\0bytes",2);const auto previous=capture(target);const auto before=live;bool failed=false;countdown=k;try{target=source;countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}unchanged(source,src);if(failed){unchanged(target,previous);require(live==before);}else{unchanged(target,src);success=true;break;}}require(success);}
static void source_lifetime(){auto s=text(513);RollingHash a(s,911382323);const auto expected=capture(a);s.assign(9000,'z');std::string{}.swap(s);unchanged(a,expected);RollingHash copy=a;unchanged(copy,expected);}
static void nonallocating(){RollingHash a(text(1024),1000000007),b(text(1024),1000000007);const auto before=calls;countdown=0;for(std::size_t i=0;i<=a.size();++i){require(a.get(i,i)==0);require(a.equal(i,a.size(),b,i,b.size()));require(a.lcp(i,b,i)==a.size()-i);require(a.compare(i,a.size(),b,i,b.size())==0);const auto mid=i+(a.size()-i)/2;require(a.concat(a.get(i,mid),a.get(mid,a.size()),a.size()-mid)==a.get(i,a.size()));}auto*alias=&a;a=*alias;a=std::move(*alias);a.swap(b);RollingHash moved(std::move(a));a=std::move(moved);swap(a,b);countdown=-1;require(calls==before);require(moved.empty());require(moved.get(0,0)==0);require(moved.base()==1000000007);
#ifdef SIRKIT_RH_REQUIRE_EXTRA
 const auto old_calls=calls,old_live=live;const auto base=a.base();countdown=0;a.clear();require(a.empty()&&a.size()==0&&a.whole()==0&&a.base()==base);require(a.get(0,0)==0&&a.concat(0,0,0)==0&&a.lcp(0,a,0)==0&&a.compare(0,0,a,0,0)==0);a.clear();countdown=-1;require(calls==old_calls&&live==old_live);static_assert(noexcept(std::declval<RollingHash&>().clear()));
#endif
}
#ifdef SIRKIT_RH_REQUIRE_EXTRA
static void assign_fail(){const auto s=text(513);bool success=false;for(int k=0;k<16;++k){RollingHash target("old state",2);const auto previous=capture(target);const auto before=live;bool failed=false;countdown=k;try{target.assign(s);countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}if(failed){unchanged(target,previous);require(live==before);}else{const RollingHash expected(s,2);require(target.base()==2);unchanged(target,capture(expected));success=true;break;}}require(success);RollingHash cleared("old",911382323);cleared.clear();cleared.assign(std::string_view{});require(cleared.empty()&&cleared.base()==911382323&&cleared.get(0,0)==0);cleared.assign(s);require(cleared.size()==s.size()&&cleared.base()==911382323);}
#endif
int main(){const auto old=live;constructor_fail();copy_constructor_fail();assignment_fail();source_lifetime();nonallocating();
#ifdef SIRKIT_RH_REQUIRE_EXTRA
assign_fail();
#endif
require(live==old);std::printf("PASS checks=%zu allocation_failures=%zu live_delta=%td\n",checks,failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(old));}
