# sirkit-cpp-library

競技プログラミング向けのC++ライブラリ集です。

各ソースの冒頭に、公開APIの引数・戻り値・計算量・前提をまとめています。詳しい説明は、その直上にあるDocumentへのリンクから参照できます。

## ライブラリの配置とVerify

| フォルダ | 内容 |
| --- | --- |
| [Graph](Graph/README.md) | 最短路・LCA・トポロジカルソート・Project Selection |
| [DataStructure](DataStructure/README.md) | BIT・Trie・ヒープ・Union-Find・直線集合・ハッシュ表 |
| [Math](Math/README.md) | 二項係数・素数判定・篩・ゼータ／メビウス変換 |
| [String](String/README.md) | RollingHash・文字列Trie |
| [Geometry](Geometry/README.md) | 点・直線／線分・多角形・円・最近点対・偏角ソート |
| [Algorithm](Algorithm/README.md) | LIS・Radix Sort・組合せ列挙・行列変換 |
| [Internal](Internal/README.md) | 内部用の軽量ハッシュ表 |
| [Document](Document/README.md) | アルゴリズムの説明・計算量・利用前提 |

現在版の再Verifyは[Verify/current](Verify/current/README.md)で管理します。提出先・選定理由・ライブラリ込みのコード・ローカル検査結果をまとめています。
速度確認のため、外部提出用コードは先頭で`SKIP_RUNTIME_CHECKS`を定義しています。
直下の`Verify/*.md`は過去の提出記録です。今回のオンライン判定結果とは区別します。

Verify提出用コードは[共通高速入出力](Verify/current/common/fast_io.hpp)を使い、mainは素直なAPI呼び出しに留めます。本体の速度比較では、main・入出力・コンパイラ設定・検査設定をそろえます。

使用例は[Examples](Examples)、二項係数のテーブル上限などの設定は[二項係数の説明](Math/Combination/README.md)を参照してください。

先頭で次を定義すると、入力検査をコンパイル時に省けます。

```cpp
#define SKIP_RUNTIME_CHECKS
```

## License

My original contributions to this repository are dedicated to the public domain under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/).

Third-party provenance and licensing are recorded in the [license audit](Audit/license-audit-2026-10-17.json). Confirmed sources include CC0-1.0 code and an explicit public-domain dedication with unconditional permission for the SplitMix64 finalizer. Some attributions and permission terms for historical implementations remain unverified; they are outside the current reviewed-source declaration. Earlier distributions of Zeta_Mebius.cpp contained MIT-derived code and retain their accompanying MIT notices.

The original public domain dedications and license terms remain applicable to those third-party works. I do not claim ownership of or purport to waive any rights held by third parties.

Where possible, the source and original license of third-party code are identified in the corresponding source files.

All code is provided without warranty, to the extent permitted by applicable law.
