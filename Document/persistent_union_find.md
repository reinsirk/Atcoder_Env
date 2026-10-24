# 完全永続Union Find

保存した過去の版を参照し、その版から新しく結合できます。各版は非所有の`Node*`で表し、変更前の版を維持します。[使用例](../Examples/persistent_union_find_example.cpp)を参照してください。

各頂点に親、根には成分サイズの負数を保持します。成分サイズの小さい根を大きい根へ接続し、経路圧縮は行いません。親とサイズは永続配列に保存し、更新経路だけをコピーします。結合時の2点更新は共通経路を一度だけ複製します。同一成分照会・結合では2頂点の親を並行して参照し、独立な経路の読み出しを重ねます。任意の保存版から分岐できる完全永続構造です。

## Union FindのAPI

| API | 動作 |
| --- | --- |
| `PersistentUnionFind(n=0)` | `n`頂点を別々の成分として構築する。`n<=INT_MAX`。 |
| `find(i[,version])`、`leader`、`root_of` | 指定版、または現在版での代表頂点。 |
| `find_with_size(i[,version])` | `{代表頂点,負の成分サイズ}`。 |
| `size(i[,version])` | 指定頂点の成分サイズ。 |
| `same(i,j[,version])` | 同じ成分か。 |
| `unite(i,j[,version])`、`merge` | 指定版から結合し、結果を現在版にする。新しく結合したときだけ`true`。 |
| `get_root()`、`restore(version)` | 現在版の取得、現在版の選択。 |
| `size()` | 全頂点数。 |
| `component_count([version])` | 成分数。 |
| `groups([version])` | 成分ごとの頂点一覧。各一覧の頂点は昇順。 |

`[,version]`は版を末尾に指定するオーバーロードがあることを表します。頂点は`0<=i<n`です。結合が不要な場合も、渡した版が現在版になります。

代表探索は親を最大O(log(n+1))回たどり、そのたびに永続配列を参照するため、`find`、`size(i)`、`same`はO(log²(n+1))時間、`unite`はO(log²(n+1))償却時間です。成功した結合はO(log(n+1))個のノードを追加します。`get_root`、`restore`、引数なし`size`はO(1)時間です。`component_count`はO(n log(n+1))、`groups`はO(n log²(n+1))時間です。

```cpp
PersistentUnionFind uf(4);
auto initial = uf.get_root();
uf.unite(0, 1);
auto left = uf.get_root();
uf.unite(2, 3, initial); // initialから別の枝へ
assert(!uf.same(0, 1));
assert(uf.same(0, 1, left));
uf.restore(left);
assert(uf.size(0) == 2);
```

## 永続配列のAPI

`PersistentArray<T,shift=4>`も単独で使えます。分岐数を`B=2^shift`とし、`1<=shift<=8`です。座標は`int64_t`で表す添字で、サイズは`0<=n<=INT64_MAX`です。値型にはコピー構築とコピー代入を要求します。既定引数の`T{}`を使う場合は、デフォルト構築も必要です。

| API | 動作 |
| --- | --- |
| `PersistentArray(n=0,ID=T{})` | 全要素の初期値が`ID`の疎な配列。 |
| `PersistentArray(vector,ID=T{})` | `vector`の値から構築する。 |
| `get(k)`、`get(version,k)` | 現在版、指定版の要素を値で返す。 |
| `get_two(a,b)`、`get_two(version,a,b)` | 2要素を `pair<T,T>` で返す。指定順を保ち、同じ添字も許す。 |
| `update(k,value)`、`update(version,k,value)` | 1点を変更して現在版にし、その`Node*`を返す。 |
| `update_two(a,x,b,y)`、`update_two(version,a,x,b,y)` | 2点を同時に変更し、結果を現在版にして`Node*`を返す。同じ添字なら`y`を優先する。 |
| `restore(version)`、`root` | 現在版の選択、現在版のハンドル。 |
| `size()` | 要素数。 |

配列の`nullptr`版は、全要素が`ID`の版です。Union Findの非空インスタンスでは、公開APIから得た非null版を使ってください。

`get_two`は2本の経路を同じ反復で参照します。片方が疎な未確保部分なら、その要素だけ初期値を返します。`get`と`get_two`はO(log_B(n+1))時間、`update`と`update_two`はO(B log_B(n+1))償却時間・償却追加領域です。保存ブロックを管理するvectorの拡張費用を含みます。定数配列の構築はO(B+log_B(n+1))時間で、全要素を確保しません。`vector`からの構築は葉ごとに値を続けて代入し、根から葉までの再走査を減らします。O(B+n+(n/B+1)log_B(n+1))時間、O(n+B)領域です。

```cpp
PersistentArray<int> a(4, 0);
auto initial = a.root;
a.update_two(0, 7, 3, 9);
assert(a.get(0) == 7 && a.get(3) == 9);
assert(a.get(initial, 0) == 0);
auto pair = a.get_two(0, 3); // {7,9}
auto previous = a.get_two(initial, 0, 3); // {0,0}
a.update_two(initial, 1, 4, 1, 8);
assert(a.get(1) == 8 && a.get(0) == 0);
```

## 版と所有権

ノードは小さいブロックから段階的に確保します。各ブロックの容量内でだけ追加し、以後の更新で過去のノードのアドレスを変えません。

葉と枝は木の深さから決まります。GCCでは、自明にコピーできる値型のノードをタグのないunionで保持します。Clang・その他のコンパイラと、自明にコピーできない値型にはvariantを使います。どちらも同じAPIと版の所有権を保ちます。

コピーは保存領域を共有し、現在版のハンドルをそれぞれ保持します。一方のコピーで更新しても、もう一方の現在版や保存済みの版は変わりません。移動元は空になります。

`Node*`は非所有ハンドルです。同じ保存領域を所有するインスタンスが1つ以上残っている間だけ使えます。最後の所有者の破棄で全版をまとめて解放し、木の深さに応じた再帰解放はしません。別の保存領域のハンドル、寿命切れのハンドルは渡せません。並行更新には外部同期が必要です。

更新が例外で失敗しても、現在版と既存の版の内容は維持します。作成途中の未公開ノードを保存領域に保持する場合があり、失敗時に保存領域の容量まで巻き戻す保証はありません。

Union Findの`arr`と`root`は互換用の公開フィールドです。`arr`を直接書き換えると親とサイズの不変条件を壊せるため、通常はUnion FindのAPIを使います。

## 入力検査

既定では添字の範囲と版の所有元を、対象への操作中に確認します。Union Findの非空インスタンスにnull版を渡すと`invalid_argument`、範囲外の添字は`out_of_range`です。寿命切れポインタを検査できる機能ではありません。

`SKIP_RUNTIME_CHECKS`を定義すると入力検査と検査専用の所有元情報を除外します。サイズ・ノード容量の内部上限は維持します。検査省略時も、添字と版の前提を満たす必要があります。

出典とライセンスの根拠は[来歴監査](../Audit/provenance-persistent-union-find-second-pass-2026-10-22.json)を参照してください。
