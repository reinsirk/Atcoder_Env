// SPDX-License-Identifier: CC0-1.0
// 順次追加・密化前後・確保失敗後の再利用を独立配列と照合する。
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#endif
#include FENWICK_SOURCE
#include <numeric>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <new>
#include <random>
static int budget=-1;
[[gnu::noinline]] void* operator new(std::size_t n){if(budget==0)throw std::bad_alloc();if(budget>0)--budget;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
[[gnu::noinline]] void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]] void operator delete(void* p)noexcept{std::free(p);}[[gnu::noinline]] void operator delete[](void* p)noexcept{std::free(p);}
[[gnu::noinline]] void operator delete(void* p,std::size_t)noexcept{std::free(p);}[[gnu::noinline]] void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
using I=std::int64_t;using U=std::uint32_t;static std::uint64_t checks=0;
static void check(bool b,std::source_location loc=std::source_location::current()){++checks;if(!b){std::fprintf(stderr,"fail at line %u\n",loc.line());std::abort();}}
static std::mt19937 rng(485139);
struct Oracle {
 int n;DynamicFenwickTree<int,I> bit;std::vector<I> ref;std::vector<unsigned char> flags;std::size_t nodes=0;
 explicit Oracle(int n):n(n),bit(n),ref(n),flags(n+1){}
 void add(int k,I v){bit.add(k,v);ref[k]+=v;for(U c=U(k)+1;c<=U(n);c+=c&(U(0)-c))if(!flags[c]){flags[c]=1;++nodes;}check(bit.stored_nodes()==nodes);}
 void validate(int upto,bool nonnegative){
  I total=std::accumulate(ref.begin(),ref.end(),I{});check(bit.sum_all()==total);
  check(bit.sum(0,n)==total);check(bit.sum(n,n)==0);
  for(int k:{0,n/2,n-1,std::max(0,upto-1),std::min(n-1,upto)})check(bit.get(k)==ref[k]);
  for(int t=0;t<3;++t){int l=int(rng()%U(n+1)),r=int(rng()%U(n+1));if(l>r)std::swap(l,r);check(bit.sum(l,r)==std::accumulate(ref.begin()+l,ref.begin()+r,I{}));check(bit.sum(r)==std::accumulate(ref.begin(),ref.begin()+r,I{}));}
  if(nonnegative)for(I w:{I(-1),I(0),I(1),total/2,total,total+1}){
   int lower=0,upper=0;I s=0;
   if(w>0){lower=n;for(int i=0;i<n;++i){s+=ref[i];if(s>=w){lower=i;break;}}}
   s=0;if(w>=0){upper=n;for(int i=0;i<n;++i){s+=ref[i];if(s>w){upper=i;break;}}}
   const auto count=bit.stored_nodes();budget=0;auto a=bit.lower_bound(w),b=bit.upper_bound(w);budget=-1;check(a==lower&&b==upper&&bit.stored_nodes()==count);
  }
 }
};
static void sequence(int n,int additions,bool use_reserve){
 Oracle o(n);if(use_reserve)o.bit.reserve(1000);o.validate(0,true);
 for(int k=0;k<additions;++k){o.add(k,k%8);if(k<140||k%257==0||k+1==additions)o.validate(k+1,true);
  if(k==128||k==256||k==16000){auto copy=o.bit;I total=o.bit.sum_all();copy.add(k/2,-copy.get(k/2));check(copy.get(k/2)==0);copy.add(k,-99);check(copy.get(k)==o.ref[k]-99);check(o.bit.sum_all()==total);}
 }
 for(int rep=0;rep<150;++rep){int k=int(rng()%U(n));o.add(k,-I(rng()%20));if(rep%7==0)o.validate(additions,false);}
 auto copied=o.bit;check(copied.sum_all()==o.bit.sum_all());auto moved=std::move(copied);check(copied.empty()&&copied.sum_all()==0&&moved.sum_all()==o.bit.sum_all());
 o.bit.clear();check(o.bit.size()==n&&o.bit.stored_nodes()==0&&o.bit.sum_all()==0);o.bit.add(n-1,13);check(o.bit.get(n-1)==13&&o.bit.sum_all()==13);
 for(int next:{65,257,8192,500000,64,500001,n}){o.bit.reset(next);check(o.bit.size()==next&&o.bit.sum_all()==0&&o.bit.stored_nodes()==0);o.bit.add(0,-7);o.bit.add(next-1,9);check(o.bit.sum_all()==2&&o.bit.get(next-1)==9);}
}
static void export_failure(){
 for(int allowed:{0,1,2}){DynamicFenwickTree<int,I> b(65);b.add(0,1);b.add(2,2);check(b.stored_nodes()==8);budget=allowed;b.add(4,3);budget=-1;check(b.stored_nodes()==10&&b.get(4)==3&&b.sum_all()==6);b.add(4,-3);check(b.get(4)==0&&b.sum_all()==3);auto c=b;check(c.sum_all()==3);b.clear();b.add(64,11);check(b.get(64)==11&&b.sum_all()==11);b.reset(65);b.add(0,4);b.add(2,5);b.add(4,6);check(b.sum_all()==15);}
}
// clear後も残る容量と、空状態のコピーで容量を持たない場合の両方を検査する。
static void retry_after_failure(){
 for(bool discard_capacity:{false,true})for(int allowed:{0,1}){
  DynamicFenwickTree<int,I> bit(257);
  for(int k=0;k<80;++k)bit.add(k,k%7);
  bit.clear();
  if(discard_capacity){auto empty_copy=bit;bit=std::move(empty_copy);}
  check(bit.sum_all()==0&&bit.stored_nodes()==0);
  bool failed=false;budget=allowed;
  try{bit.add(256,9);}catch(const std::bad_alloc&){failed=true;}
  budget=-1;
  check(failed==discard_capacity);
  if(failed){
   check(bit.sum_all()==0&&bit.sum(0,257)==0&&bit.get(0)==0&&bit.get(256)==0&&bit.stored_nodes()==0);
   check(bit.lower_bound(1)==257&&bit.upper_bound(0)==257);
   bit.add(256,9);
  }
  check(bit.sum_all()==9&&bit.get(256)==9&&bit.stored_nodes()==1);
  bit.add(0,4);check(bit.sum_all()==13&&bit.get(0)==4);
  bit.clear();check(bit.sum_all()==0&&bit.stored_nodes()==0);
  bit.add(256,3);check(bit.get(256)==3&&bit.sum_all()==3);
 }
}

// 物理配列の番兵が論理範囲・最大値の探索へ混ざらないことを確認する。
template<class T,class S=int>static void sentinel_boundaries(int n){
 DynamicFenwickTree<S,T> bit(static_cast<S>(n));std::vector<T> values(n);
 const T maximum=std::numeric_limits<T>::max();
 const auto add=[&](int k,T value){bit.add(static_cast<S>(k),value);values[k]=static_cast<T>(values[k]+value);};
 const auto query=[&](T w){
  int lower=0,upper=0;std::uint64_t sum=0;
  if(w>T{}){lower=n;for(int k=0;k<n;++k){sum+=static_cast<std::uint64_t>(values[k]);if(sum>=static_cast<std::uint64_t>(w)){lower=k;break;}}}
  sum=0;if(w>=T{}){upper=n;for(int k=0;k<n;++k){sum+=static_cast<std::uint64_t>(values[k]);if(sum>static_cast<std::uint64_t>(w)){upper=k;break;}}}
  const auto nodes=bit.stored_nodes();budget=0;
  const S a=bit.lower_bound(w),b=bit.upper_bound(w);budget=-1;
  check(a==static_cast<S>(lower)&&b==static_cast<S>(upper)&&bit.stored_nodes()==nodes);
 };
 const auto validate=[&]{
  std::uint64_t total=0;for(T value:values)total+=static_cast<std::uint64_t>(value);
  check(bit.sum_all()==static_cast<T>(total));
  if(n)check(bit.get(static_cast<S>(n/2))==values[n/2]);
  for(T w:std::array<T,7>{T{},T{1},static_cast<T>(maximum/2),static_cast<T>(maximum/2+1),static_cast<T>(maximum-1),maximum,T{2}})query(w);
  if constexpr(std::is_signed_v<T>)query(T{-1});
 };
 validate();
 for(int k=0;k<n;++k)add(k,T{});
 check(bit.stored_nodes()==static_cast<std::size_t>(n));validate();
 if(n){
  if(n==1)add(0,maximum);
  else{const T first=static_cast<T>(maximum/2);add(0,first);add(n-1,static_cast<T>(maximum-first));}
  validate();
 }
 // fresh=falseの疎な再利用から全セル占有へ戻す。
 bit.clear();std::fill(values.begin(),values.end(),T{});validate();
 if(n){add(n/2,maximum);validate();}
 for(int k=0;k<n;++k)add(k,T{});
 check(bit.stored_nodes()==static_cast<std::size_t>(n));validate();
 if(n){
  if constexpr(std::is_signed_v<T>)add(n/2,static_cast<T>(-maximum));
  else add(n/2,static_cast<T>(T{}-maximum));
  validate();
 }
 bit.clear();std::fill(values.begin(),values.end(),T{});validate();
 for(int k=0;k<n;++k)add(k,T{});
 validate();
}
template<class T>static void sentinel_type(){
 for(int n:{0,1,2,3,63,64,65,127,128,255,256,257,8191,8192,8193,16384,16385})sentinel_boundaries<T>(n);
}

int main(){
 for(bool reserve:{false,true}){for(int n:{65,127,257,8191,8192})sequence(n,std::min(n,4096),reserve);sequence(500000,40000,reserve);}
 export_failure();retry_after_failure();
 sentinel_type<std::int8_t>();sentinel_type<std::int16_t>();sentinel_type<std::int32_t>();sentinel_type<std::int64_t>();
 sentinel_type<std::uint8_t>();sentinel_type<std::uint16_t>();sentinel_type<std::uint32_t>();sentinel_type<std::uint64_t>();
 sentinel_boundaries<std::uint8_t,std::uint8_t>(128);sentinel_boundaries<std::uint8_t,std::uint8_t>(255);sentinel_boundaries<std::uint64_t,std::uint16_t>(65535);
 std::printf("dense prefix: %zu checks passed\n",static_cast<std::size_t>(checks));
}
