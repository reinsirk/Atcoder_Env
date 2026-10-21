# 二項係数ライブラリの選び方

各ファイルは単独で貼り付けられます。Mintを使う版では、利用側でAtCoder LibraryなどのMint実装を用意してください。

| ファイル | API・用途 |
| --- | --- |
| [Binomial_Mod.cpp](Binomial_Mod.cpp) | `BinomialMod`：法1〜UINT32_MAX、整数を返す |
| [ModInt_Binomial.cpp](ModInt_Binomial.cpp) | `ModIntBinomial<Mint>`：素数・合成数の法、Mintを返す |
| [Prime_Binomial.cpp](Prime_Binomial.cpp) | `PrimeBinomial<Mint>`：素数の法専用、Mintを返す |

全機能版は[Combination_Mod.cpp](../Combination_Mod.cpp)、素数専用版は[Combination_AtCoderMod.cpp](../Combination_AtCoderMod.cpp)からも使えます。
方式と計算量は[アルゴリズムの説明](../../Document/binomial_coefficient.md)、関数の引数・戻り値は各クラス先頭を参照してください。
使用例は[整数版](../../Examples/combination_integer_example.cpp)・[任意法Mint版](../../Examples/combination_modint_example.cpp)・[素数専用版](../../Examples/combination_prime_example.cpp)にあります。

## 前計算と上限

既定では最大500万まで前計算します。必要な範囲をコンストラクターの`max_n`で指定してください。
任意法版の`BinomialOptions.table_limit`はテーブルの最大添字、`max_multiplicative_steps`は1照会の積の計算予算です。素数専用版は`max_n`と`max_steps`で指定します。
`max_n=0`は構築時の前計算を抑えます。任意法版の照会時の自動拡張も止める場合は、`table_limit=0`を併用してください。
動的なMintの法を変えた場合は計算器を作り直します。

現在の提出コード・検証範囲・判定は[Verify一覧](../../Verify/current/README.md)と[manifest](../../Verify/current/manifest.json)を参照してください。

## 編集と生成

編集元は`components/`です。リポジトリ直下で`python3 Scripts/generate_combination.py`を実行すると、共通部品から全機能版・用途別版を生成します。`--check`で同期状態を確認できます。
利用時にcomponentsや生成器は不要です。現行の本体・部品・生成器はCC0-1.0で、出典確認は[監査資料](../../Audit/license-audit-2026-10-17.json)を参照してください。
