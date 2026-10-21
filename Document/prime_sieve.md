# エラトステネスの篩

[実装](../Math/Eratosthenes.cpp)・[使用例](../Examples/eratosthenes_example.cpp)

`PrimeSieve(limit)`は奇数の素数判定をビット列で保持します。
構築はO(limit log log limit)、保持量はO(limit) bitです。
`is_prime(n)`は構築した上限までの値をO(1)で判定します。
`primes()`は素数列を返し、`append_to(result)`は既存の列へ素数を追記します。
`prime_count()`はビット列の立っているbit数と、素数2の分を数えます。

## SIMDによる個数集計

AVX2対応環境では、各byteを上下4bitに分けて小さな表でbit数を数えます。
byte単位の集計があふれない範囲でまとめ、64bitの個数へ加算します。
端数のwordは通常のpopcountで処理します。

小さいビット列と専用処理の対象外の環境では、通常のpopcountを使います。
ClangでAVX2がコンパイル対象に含まれる場合も、コンパイラの自動ベクトル化を使います。
この処理は個数集計に用い、篩の構築・単点判定・素数列挙の計算手順は共通です。

環境条件と無効化の指定は[SIMDによる内部処理](simd.md)を参照してください。
