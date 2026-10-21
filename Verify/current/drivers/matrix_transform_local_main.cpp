// SPDX-License-Identifier: CC0-1.0
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("matrix transform reference mismatch");};
    auto test=[&](const auto&a){using Matrix=std::decay_t<decltype(a)>;using Row=typename Matrix::value_type;
        std::size_t h=a.size(),w=h?a[0].size():0;
        Matrix right(w,Row(h,{})),left=right,tr=right,half=a,fi=a,fj=a;
        for(std::size_t i=0;i<h;++i)for(std::size_t j=0;j<w;++j){right[j][h-1-i]=a[i][j];left[w-1-j][i]=a[i][j];tr[j][i]=a[i][j];half[h-1-i][w-1-j]=a[i][j];fi[h-1-i][j]=a[i][j];fj[i][w-1-j]=a[i][j];}
        check(rotate_90r(a)==(h?right:Matrix{Row{}}));check(rotate_90l(a)==left);check(transpose(a)==tr);check(rotate_180(a)==half);
        check(flip_i(a)==(h?fi:Matrix{Row{}}));check(flip_j(a)==(h?fj:Matrix{Row{}}));auto ii=a,jj=a;flip_i_inplace(ii);flip_j_inplace(jj);check(ii==fi);check(jj==fj);
        for(int turns:{INT_MIN,-9,-4,-1,0,1,2,3,4,9,INT_MAX}){int r=(turns%4+4)%4;check(rotate_quarter(a,turns)==(r==0?a:r==1?right:r==2?half:left));}
    };
    for(int h:{0,1,2,7,31,32,33,65})for(int w:{0,1,2,7,31,32,33,65}){
        std::vector<std::vector<int>>a(h,std::vector<int>(w));std::vector<std::string>s(h,std::string(w,' '));std::vector<std::vector<bool>>b(h,std::vector<bool>(w));
        for(int i=0;i<h;++i)for(int j=0;j<w;++j){a[i][j]=i*100+j;s[i][j]=static_cast<char>((i*17+j)%256);b[i][j]=(i+j)%2;}
        test(a);test(s);test(b);
    }
    std::cout<<"OK\n";
}
