# 二項係数Prime Mod：C++上位50提出の入出力調査

2026-10-20確認。対象は[Binomial Coefficient (Prime Mod)](https://judge.yosupo.jp/problem/binomial_coefficient_prime_mod)。公開APIのAC・時間昇順からC++23/17/20を選んだ50提出です。利用者別・同一ソースの重複排除はしていません。

## 集計

| 項目 | 件数 |
| --- | ---: |
| 独自の入力または出力 | 42 |
| 標準cin/coutのみ | 8 |
| 問題側main/solveで特定法の専用処理・実装切り替え | 19 |
| 全照会を蓄積してから計算 | 5 |
| 照会の並べ替え | 0 |
| 最適化pragma（条件付き指定を含む） | 11 |
| APIのis_latest=false | 10 |
| 同一ソースをまとめた件数 | 47 |

独自入出力42件のうち2件は自作入力＋std::cout、1件はgetcharによる自作整数入力＋printfです。標準cin/coutの8件中5件は全照会・全結果を配列へ保持し、入力・計算・出力を分けています。残り3件は通常の逐次mainです。入出力内部のバッファリングと照会配列の蓄積は区別しました。

同じ人の複数提出、当方の過去の提出、完全同一ソースを含みます。規格・pragma・判定版・計算実装も異なり、入出力別の時間差を高速入出力の効果とは解釈できません。未使用のテンプレートを数えず、実際に呼ぶ経路を確認しています。

C++23（APIのlang=cpp）だけの上位50件も補足調査し、独自入出力35件・標準cin/cout15件でした。以下の表はC++17/20を含む50件です。

## 全50提出

| 順位 | 提出 | 規格 | ms | 実使用入出力 | 法別切替 | 照会蓄積 | 判定版 |
| ---: | --- | --- | ---: | --- | --- | --- | --- |
| 1 | [223248](https://judge.yosupo.jp/submission/223248) | C++23 | 182 | gsh::BasicReader/BasicWriterの独自数値変換、POSIX read/write。入力・出力は各256KiBバッファ。 | あり | なし | latest |
| 2 | [196412](https://judge.yosupo.jp/submission/196412) | C++23 | 186 | FastIstream fin / FastOstream foutによる独自数値変換、POSIX read/write。 | あり | なし | latest |
| 3 | [196634](https://judge.yosupo.jp/submission/196634) | C++23 | 187 | FastIstream fin / FastOstream foutによる独自数値変換、POSIX read/write。 | あり | なし | latest |
| 4 | [171539](https://judge.yosupo.jp/submission/171539) | C++23 | 194 | fastio::read/printで独自整数変換とfread/fwriteバッファ。 | あり | なし | latest |
| 5 | [406024](https://judge.yosupo.jp/submission/406024) | C++23 | 197 | gsh::BasicReader/BasicWriterの独自数値変換、POSIX read/write。入力・出力は各256KiBバッファ。 | あり | なし | latest |
| 6 | [196406](https://judge.yosupo.jp/submission/196406) | C++23 | 203 | FastIstream fin / FastOstream foutによる独自数値変換、POSIX read/write。 | なし | なし | latest |
| 7 | [411936](https://judge.yosupo.jp/submission/411936) | C++23 | 230 | main内の独自パーサ、64KiB freadバッファ、to_charsと64KiB fwrite出力。 | なし | なし | latest |
| 8 | [139188](https://judge.yosupo.jp/submission/139188) | C++17 | 232 | fread → fwrite | なし | なし | latest |
| 9 | [331410](https://judge.yosupo.jp/submission/331410) | C++20 | 240 | mmapまたはsyscall → POSIX write | あり | なし | latest |
| 10 | [331407](https://judge.yosupo.jp/submission/331407) | C++20 | 241 | mmapまたはsyscall → POSIX write | あり | なし | latest |
| 11 | [331409](https://judge.yosupo.jp/submission/331409) | C++20 | 245 | mmapまたはsyscall → POSIX write | あり | なし | latest |
| 12 | [219446](https://judge.yosupo.jp/submission/219446) | C++23 | 252 | 独自IO、Linux本番ではmmap入力、fwriteによる出力バッファ。 | あり | なし | latest |
| 13 | [315722](https://judge.yosupo.jp/submission/315722) | C++23 | 256 | std::cin/coutという記述だがマクロで独自i$/o$へ差替。通常ファイルはmmap、その他は直接syscallの入力。出力はmmap確保したバッファへ蓄積しwrite。 | あり | なし | latest |
| 14 | [267167](https://judge.yosupo.jp/submission/267167) | C++23 | 260 | FAST_IOが有効。LL→internal::READnodump→fastio::read、PRINT→fastio::print、fread/fwrite。 | なし | なし | latest |
| 15 | [251189](https://judge.yosupo.jp/submission/251189) | C++23 | 265 | 標準cin/cout、同期解除・tie解除。 | なし | あり | latest |
| 16 | [251181](https://judge.yosupo.jp/submission/251181) | C++23 | 265 | 標準cin/cout、同期解除・tie解除。 | なし | あり | latest |
| 17 | [251180](https://judge.yosupo.jp/submission/251180) | C++23 | 265 | 標準cin/cout、同期解除・tie解除。 | なし | あり | latest |
| 18 | [251169](https://judge.yosupo.jp/submission/251169) | C++23 | 265 | 標準cin/cout、同期解除・tie解除。 | あり | あり | latest |
| 19 | [251170](https://judge.yosupo.jp/submission/251170) | C++23 | 266 | 標準cin/cout、同期解除・tie解除。 | あり | あり | latest |
| 20 | [411353](https://judge.yosupo.jp/submission/411353) | C++20 | 270 | POSIX read → POSIX write | なし | なし | latest |
| 21 | [264041](https://judge.yosupo.jp/submission/264041) | C++23 | 272 | fastio::rd/wtによる独自整数変換、fread/fwriteバッファ。 | なし | なし | latest |
| 22 | [314336](https://judge.yosupo.jp/submission/314336) | C++23 | 277 | cin/coutはマクロでFIstream/FOstreamへ差替、fread/fwrite各128KiBバッファ。 | なし | なし | latest |
| 23 | [267160](https://judge.yosupo.jp/submission/267160) | C++23 | 277 | fread → fwrite | なし | なし | latest |
| 24 | [243809](https://judge.yosupo.jp/submission/243809) | C++20 | 278 | fread → fwrite | なし | なし | latest |
| 25 | [217008](https://judge.yosupo.jp/submission/217008) | C++23 | 284 | fread → fwrite | なし | なし | latest |
| 26 | [217005](https://judge.yosupo.jp/submission/217005) | C++23 | 291 | fread → fwrite | なし | なし | latest |
| 27 | [171538](https://judge.yosupo.jp/submission/171538) | C++23 | 291 | fread → fwrite | あり | なし | 旧判定 |
| 28 | [267165](https://judge.yosupo.jp/submission/267165) | C++23 | 292 | fread → fwrite | なし | なし | latest |
| 29 | [196632](https://judge.yosupo.jp/submission/196632) | C++23 | 296 | POSIX read → POSIX write | あり | なし | 旧判定 |
| 30 | [220423](https://judge.yosupo.jp/submission/220423) | C++23 | 298 | fread → fwrite | なし | なし | latest |
| 31 | [220422](https://judge.yosupo.jp/submission/220422) | C++23 | 298 | fread → fwrite | なし | なし | latest |
| 32 | [154448](https://judge.yosupo.jp/submission/154448) | C++17 | 298 | fread → fwrite | なし | なし | 旧判定 |
| 33 | [140225](https://judge.yosupo.jp/submission/140225) | C++23 | 298 | getchar_unlocked → putchar_unlocked | なし | なし | 旧判定 |
| 34 | [146479](https://judge.yosupo.jp/submission/146479) | C++23 | 300 | getchar_unlocked → putchar_unlocked | なし | なし | 旧判定 |
| 35 | [143887](https://judge.yosupo.jp/submission/143887) | C++17 | 303 | fread → fwrite | あり | なし | 旧判定 |
| 36 | [166836](https://judge.yosupo.jp/submission/166836) | C++23 | 306 | mmap (Unix; その他fread) → fwrite | あり | なし | 旧判定 |
| 37 | [358769](https://judge.yosupo.jp/submission/358769) | C++23 | 309 | fread → std::cout | なし | なし | latest |
| 38 | [143945](https://judge.yosupo.jp/submission/143945) | C++17 | 312 | fread → fwrite | あり | なし | 旧判定 |
| 39 | [143943](https://judge.yosupo.jp/submission/143943) | C++17 | 312 | fread → fwrite | あり | なし | 旧判定 |
| 40 | [210037](https://judge.yosupo.jp/submission/210037) | C++23 | 313 | POSIX read → POSIX write | なし | なし | latest |
| 41 | [210469](https://judge.yosupo.jp/submission/210469) | C++23 | 315 | POSIX read → POSIX write | なし | なし | latest |
| 42 | [358768](https://judge.yosupo.jp/submission/358768) | C++23 | 322 | fread → std::cout | なし | なし | latest |
| 43 | [411962](https://judge.yosupo.jp/submission/411962) | C++23 | 323 | std::cin → std::cout | なし | なし | latest |
| 44 | [267162](https://judge.yosupo.jp/submission/267162) | C++23 | 323 | fread → fwrite | なし | なし | latest |
| 45 | [265146](https://judge.yosupo.jp/submission/265146) | C++23 | 326 | 自作fread/fwriteバッファ | なし | なし | latest |
| 46 | [210445](https://judge.yosupo.jp/submission/210445) | C++23 | 328 | 自作POSIX read/writeバッファ（yosupo Scanner/Printer） | なし | なし | latest |
| 47 | [411965](https://judge.yosupo.jp/submission/411965) | C++23 | 330 | std::cin/std::cout（同期解除・tie解除） | なし | なし | latest |
| 48 | [272638](https://judge.yosupo.jp/submission/272638) | C++23 | 332 | 自作getchar整数入力＋printf出力 | なし | なし | latest |
| 49 | [196633](https://judge.yosupo.jp/submission/196633) | C++23 | 333 | 自作POSIX read/writeバッファ（fin/fout） | あり | なし | 旧判定 |
| 50 | [411941](https://judge.yosupo.jp/submission/411941) | C++23 | 334 | std::cin/std::cout（同期解除・tie解除） | なし | なし | latest |

「法別切替」は問題側main/solveの法2専用計算や固定法型への切り替えです。前計算上限minやライブラリ内部の処理は数えていません。「旧判定」はAPIのis_latest=falseを表し、現行版でのACとは扱いません。

行番号・ソースSHA256・個別注意点・C++23のみの補足集計は[JSON記録](library-checker-binomial-prime-top50-2026-10-20.json)に保存しています。公開ソースは調査に使用し、実装へ取り込んでいません。

## 測定方針への反映

共通の高速入出力と素直なAPI呼び出しを使う方針は妥当です。自分の改修前後を比べるときは、同じmain・入出力・コンパイラ設定・検査設定を使います。ランキングの最速値は提出全体の参考値とし、本体だけの比較には同じmainへ載せ替えたローカル測定を使います。
