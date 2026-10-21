# ゼータ／メビウス変換と整数篩

[全機能版](../Math/Zeta_Mebius.cpp)・[用途別版](../Math/Zeta/README.md)・[全体の使用例](../Examples/zeta_mobius_example.cpp)

## 集合の変換と畳み込み

添字を集合のビット表現として扱い、ビットごとに値を伝播します。入力は空配列または長さ`n=2^k`の配列です。

| API | 処理 |
| --- | --- |
| `superset_zeta_transform(f)` | 各集合に、その上位集合の値の総和を入れる |
| `superset_mobius_transform(f)` | 上位集合ゼータ変換を元に戻す |
| `subset_zeta_transform(f)` | 各集合に、その部分集合の値の総和を入れる |
| `subset_mobius_transform(f)` | 部分集合ゼータ変換を元に戻す |
| `bitwise_and_convolution(f,g)` | `h[k]=sum_{i&j=k} f[i]g[j]`を返す |
| `bitwise_or_convolution(f,g)` | `h[k]=sum_{i\|j=k} f[i]g[j]`を返す |

各変換は配列を直接更新し、O(n log n)時間・O(1)追加領域です。畳み込みは同じ長さの配列を受け取り、変換・各点の積・逆変換によりO(n log n)時間・O(n)追加領域で求めます。

[集合変換の使用例](../Examples/zeta_set_example.cpp)・[ビット畳み込みの使用例](../Examples/zeta_bitwise_example.cpp)

## 整数篩

`IntegerSieve(M)`は`0<=M`に対し、線形篩で最小素因数表と昇順の素数列をO(M)時間・領域で構築します。公開配列`least_prime_factor`と`prime_list`は変更せず使います。0と1の最小素因数は0です。

| API | 戻り値・前提 |
| --- | --- |
| `max_value()` | 表の上限。ムーブ元などの空の表では-1 |
| `is_prime(x)` | `0<=x<=M`の素数判定。O(1) |
| `prime_factorization(x)` | `1<=x<=M^2`の素因数と指数のmap。`x=1`は空の表でも可 |
| `enumerate_divisors(x)` / `sorted_divisors(x)` | 同じ範囲の全約数。後者は昇順 |
| `divisor_totients(x)` | 同じ範囲の各約数`d`から`phi(d)`へのmap |
| `mobius_table()` / `totient_table()` | `0..M`のメビウス関数／Eulerのφ関数。いずれもO(M)時間・領域 |
| `power_table<Mint>(k,nmax)` | `0<=nmax<=M`, `k>=0`で`0^k..nmax^k`を返す。`0^0=1` |
| `swap(other)` | 表と素数列をO(1)で交換 |

素因数分解は表内ならO(log x)、表外なら最悪O(pi(M)+log x)時間です。`pi(M)`はM以下の素数の個数です。約数の個数を`d`とすると、約数列挙は素因数分解に加えてO(d)、昇順列挙とφ値のmap構築はO(d log(d+1))時間を使います。

`power_table`は素数のべき乗を求め、合成数の値を最小素因数から組み立てます。Mintの乗算をO(1)、`pow(k)`をO(log(k+1))としたとき、O(nmax+pi(nmax)log(k+1))時間・O(nmax)領域です。Mintは利用側で用意します。

コピーは独立した表を持ちます。ムーブ元の表と素数列は空になります。

[整数篩の使用例](../Examples/zeta_sieve_example.cpp)

## 約数・倍数の変換

素数ごとに添字間を伝播し、正の添字に対して変換します。`f[0]`は変えず、空配列も扱います。

| API | 処理 |
| --- | --- |
| `multiple_zeta_transform(f)` | `f[d]=sum_{d\|k} f[k]`へ更新 |
| `multiple_mobius_transform(f)` | 倍数ゼータ変換を元に戻す |
| `divisor_zeta_transform(f)` | `f[k]=sum_{d\|k} f[d]`へ更新 |
| `divisor_mobius_transform(f)` | 約数ゼータ変換を元に戻す |

各APIには`(f,sieve)`版もあります。`n=f.size()`として、`sieve.max_value()>=n-1`の表を再利用し、O(n log log n)時間・O(1)追加領域です。`(f)`版は素数表の作成も行い、`n-1<=INT_MAX`が必要で、追加領域はO(n)です。

[整数変換の使用例](../Examples/zeta_integer_example.cpp)

## GCD・LCM畳み込み

`gcd_convolution(f,g)`は`h[k]=sum_{gcd(i,j)=k} f[i]g[j]`、`lcm_convolution(f,g)`は`h[k]=sum_{lcm(i,j)=k} f[i]g[j]`を返します。配列は同じ長さ`n`で、空配列も扱います。LCMが`n`以上になる組は結果に含めません。

約数・倍数変換と各点の積を使い、0を含む組の寄与を別途加えます。`gcd(0,k)=k`、`gcd(0,0)=0`、`lcm(0,k)=0`として計算します。時間はO(n log log n)、追加領域はO(n)です。

両APIの`(f,g,sieve)`版は`max_value()>=n-1`の表を再利用します。`(f,g)`版は素数表も作り、`n-1<=INT_MAX`が必要です。

[整数畳み込みの使用例](../Examples/zeta_convolution_example.cpp)

## 疎な倍数変換

`sparse_multiple_zeta_transform(f,sieve)`は`vector<pair<Int,Val>>`の値を倍数和へ更新し、`sparse_multiple_mobius_transform(f,sieve)`はその逆変換を行います。両者ともkeyの昇順に並べ替えます。

keyは正で一意な整数で、各keyの全約数もkeyとして含み、`key<=sieve.max_value()`を満たす必要があります。空配列も扱います。密な範囲では位置配列を使い、疎な範囲では位置検索と素数ごとの伝播列を使います。

要素数を`m`、各keyの素因数を重複込みで数えた総数を`r`とすると、O((m+r)log(m+1))時間・O(m+r)追加領域です。

[疎変換の使用例](../Examples/zeta_sparse_example.cpp)

## 値型と入力検査

値型は各APIで使う加算・減算・乗算に対応し、全ての中間値が表現範囲に収まる必要があります。整数変換や畳み込みでは既定構築した値を0として使います。

入力検査は既定で有効です。`SKIP_RUNTIME_CHECKS`を定義する場合も、配列長・表の上限・疎変換のkey集合などの前提を満たしてください。公開配列の整合性など、未参照部分を含めた全入力の検査は行いません。
