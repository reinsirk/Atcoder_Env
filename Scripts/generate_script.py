# SPDX-License-Identifier: CC0-1.0
# Coding by Codex.
import sys
import os

# Python 3.11以降で標準搭載されているTOMLパーサー
try:
    import tomllib
except ImportError:
    print("エラー: このプログラムの実行には Python 3.11 以上が必要です。")
    sys.exit(1)

def main():
    if len(sys.argv) < 2:
        print("使い方: python generate_scripts.py <設定ファイル.toml>")
        sys.exit(1)

    toml_file = sys.argv[1]

    # TOMLファイルの読み込み
    try:
        with open(toml_file, "rb") as f:
            data = tomllib.load(f)
    except FileNotFoundError:
        print(f"エラー: ファイル '{toml_file}' が見つかりません。")
        sys.exit(1)
    except Exception as e:
        print(f"TOMLファイルの解析に失敗しました: {e}")
        sys.exit(1)

    # install.sh の抽出と生成
    if "install" in data:
        with open("install.sh", "w", encoding="utf-8") as f:
            f.write(data["install"])
        # os.chmod("install.sh", 0o755) # 実行権限を付与
        # print("✅ install.sh を生成し、実行権限を付与しました。")
    else:
        print("⚠️ 警告: TOMLファイル内に 'install' キーが見つかりません。")

    # compile.sh の抽出と生成
    if "compile" in data:
        with open("compile.sh", "w", encoding="utf-8") as f:
            f.write(data["compile"])
        # os.chmod("compile.sh", 0o755) # 実行権限を付与
        # print("✅ compile.sh を生成し、実行権限を付与しました。")
    else:
        print("⚠️ 警告: TOMLファイル内に 'compile' キーが見つかりません。")

if __name__ == "__main__":
    main()