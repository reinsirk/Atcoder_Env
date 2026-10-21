#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""共通部品から用途別・全機能版のゼータ／メビウス変換を生成する。"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

# 依存は生成時に展開し、生成したC++に自作includeを残さない。
PARTS: dict[str, tuple[str, ...]] = {
    "set_transform": (),
    "bitwise_convolution": ("set_transform",),
    "sieve": (),
    "integer_transform_detail": ("sieve",),
    "divisor_multiple_transform": ("integer_transform_detail",),
    "integer_convolution_detail": ("integer_transform_detail",),
    "gcd_lcm_convolution": ("integer_convolution_detail",),
    "sparse_transform": ("sieve",),
}
TARGETS: dict[str, tuple[str, ...]] = {
    "Math/Zeta_Mebius.cpp": tuple(PARTS),
    "Math/Zeta/Set_Transform.cpp": ("set_transform",),
    "Math/Zeta/Bitwise_Convolution.cpp": ("bitwise_convolution",),
    "Math/Zeta/Sieve.cpp": ("sieve",),
    "Math/Zeta/Divisor_Multiple_Transform.cpp": ("divisor_multiple_transform",),
    "Math/Zeta/Gcd_Lcm_Convolution.cpp": ("gcd_lcm_convolution",),
    "Math/Zeta/Sparse_Transform.cpp": ("sparse_transform",),
}
HEADERS: dict[str, tuple[str, ...]] = {
    "set_transform": ("bit", "cstddef", "cstring", "stdexcept", "type_traits", "utility", "vector"),
    "bitwise_convolution": (),
    "sieve": ("algorithm", "concepts", "cstddef", "cstdint", "limits", "map", "stdexcept", "utility", "vector"),
    "integer_transform_detail": ("array", "span"),
    "divisor_multiple_transform": (),
    "integer_convolution_detail": (),
    "gcd_lcm_convolution": (),
    "sparse_transform": ("array", "bit", "type_traits"),
}


def dependency_order(roots: tuple[str, ...]) -> list[str]:
    order: list[str] = []
    complete: set[str] = set()
    active: set[str] = set()

    def visit(part: str) -> None:
        if part in complete:
            return
        if part in active:
            raise ValueError(f"依存が循環しています: {part}")
        active.add(part)
        for dependency in PARTS[part]:
            visit(dependency)
        active.remove(part)
        complete.add(part)
        order.append(part)

    for root in roots:
        visit(root)
    return order


def load_sources(directory: Path) -> tuple[str, list[str], dict[str, str]]:
    headers = (directory / "includes.txt").read_text(encoding="utf-8")
    if not headers.endswith("\n") or any(
        not re.fullmatch(r"#include <[a-z0-9_]+>", line)
        for line in headers.splitlines()
    ):
        raise ValueError("includes.txt は標準ヘッダーのincludeだけを記載します")
    references = (directory / "references.txt").read_text(encoding="utf-8").splitlines()
    if len(references) != 2 or any(not line.startswith("// 参考: https://") for line in references):
        raise ValueError("references.txt の参考URLが不正です")
    pieces: dict[str, str] = {}
    guards: set[str] = set()
    for name in PARTS:
        piece = (directory / f"{name}.inc").read_text(encoding="utf-8")
        found = re.findall(r"^#ifndef (SIRKIT_[A-Z_]+)\n#define \1$", piece, re.MULTILINE)
        if not found or not piece.rstrip().endswith("#endif"):
            raise ValueError(f"部品のガード形式が不正です: {name}")
        if guards.intersection(found) or len(found) != len(set(found)):
            raise ValueError(f"部品間でガードが重複しています: {name}")
        if re.search(r"^#include\s", piece, re.MULTILINE):
            raise ValueError(f"部品内のincludeは禁止です: {name}")
        guards.update(found)
        pieces[name] = piece
    return headers, references, pieces


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    base = Path(__file__).resolve().parent
    parser.add_argument("--components-dir", type=Path, default=base.parent / "Math" / "Zeta" / "components")
    parser.add_argument("--output-dir", type=Path, default=base.parent)
    parser.add_argument("--check", action="store_true", help="書き込まず全生成物の一致を確認する")
    options = parser.parse_args()
    try:
        headers, references, pieces = load_sources(options.components_dir)
        failures = []
        for relative, roots in TARGETS.items():
            order = dependency_order(roots)
            needed_headers = {header for part in order for header in HEADERS[part]}
            content = "// SPDX-License-Identifier: CC0-1.0\n"
            if "set_transform" in order:
                content += references[0] + "\n"
            if "integer_transform_detail" in order:
                content += references[1] + "\n"
            content += "".join(line + "\n" for line in headers.splitlines()
                               if line.removeprefix("#include <").removesuffix(">") in needed_headers)
            content += "\n" + "".join(pieces[part] for part in order)
            target = options.output_dir / relative
            if options.check:
                if not target.is_file() or target.read_bytes() != content.encode("utf-8"):
                    failures.append(str(target))
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(content, encoding="utf-8", newline="\n")
        if failures:
            print("生成物が編集元と一致しません:\n" + "\n".join(failures), file=sys.stderr)
            return 1
        print(f"ゼータ／メビウス変換{len(TARGETS)}ファイル: " + ("同期確認済み" if options.check else "生成済み"))
        return 0
    except (OSError, ValueError, KeyError) as error:
        print(f"ゼータ／メビウス変換の生成に失敗しました: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
